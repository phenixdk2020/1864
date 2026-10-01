// Rivers, lakes, canals and the field boundaries (Docs/Hydro1851.md). The map's height data is too smooth to
// find the rivers in, so they come from Data/Campaign1851/Hydro1851.json: the real rivers (source to mouth)
// and lakes in latitude and longitude, projected like the rest of the map. Each river gets gentle bends from
// its name and runs on to the sea; lakes are wobbled ellipses. Everything later added to the map (more
// countries) is drawn the same way from the same file. The roads between the towns get a bridge where they
// cross a river (Campaign1851Bridges.cpp), and the battlefield generator takes rivers, lakes, fords and the
// hedges, dikes and ditches of the fields.

#include "Campaign1851Map.h"
#include "Campaign1851Scenery.h"

#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	constexpr double HydroCellKm = 1.0;
	const TCHAR* HydroMaterialPath = TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery");

	float HydroHash01(uint32 A, uint32 B)
	{
		uint32 H = HashCombine(A * 2654435761u + 40503u, B);
		H ^= H >> 15; H *= 0x2c1b3c6du; H ^= H >> 12; H *= 0x297a2d39u; H ^= H >> 15;
		return (H & 0xffffff) / float(0xffffff);
	}

	/** The ragged shore of a lake: a radius factor round the ellipse. */
	float LakeWobble(uint32 Seed, double T)
	{
		return float(1.0 + 0.10 * FMath::Sin(3.0 * T + 6.28 * HydroHash01(Seed, 1)) + 0.06 * FMath::Sin(5.0 * T + 6.28 * HydroHash01(Seed, 2))
			+ 0.03 * FMath::Sin(9.0 * T + 6.28 * HydroHash01(Seed, 3)));
	}

	FIntPoint HydroCell(const FVector2D& Km)
	{
		return FIntPoint(FMath::FloorToInt(Km.X / HydroCellKm), FMath::FloorToInt(Km.Y / HydroCellKm));
	}

	double SegmentDistance(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D D = B - A;
		const double L2 = D.SizeSquared();
		const double T = L2 > 0.0 ? FMath::Clamp(FVector2D::DotProduct(P - A, D) / L2, 0.0, 1.0) : 0.0;
		return FVector2D::Distance(P, A + D * T);
	}
}

float ACampaign1851Map::RiverWidthM(int32 Class)
{
	static const float Widths[] = { 8.f, 20.f, 50.f, 400.f };
	return Widths[FMath::Clamp(Class, 0, 3)];
}

float ACampaign1851Map::RiverDrawnKm(int32 Class)
{
	// Drawn wider than life (like the roads), so they read on the map.
	static const float Widths[] = { 0.09f, 0.14f, 0.22f, 0.6f };
	return Widths[FMath::Clamp(Class, 0, 3)];
}

bool ACampaign1851Map::LoadHydro()
{
	Rivers.Reset();
	Lakes.Reset();
	RiverCells.Reset();
	FString Text;
	TSharedPtr<FJsonObject> Json;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Campaign1851/Hydro1851.json")))
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|no Data/Campaign1851/Hydro1851.json; the map has no rivers or lakes"));
		return false;
	}
	const FCampaign1851Projection& Proj = Extent.Projection;
	auto Inside = [this](const FVector2D& Km) { return Km.X > Extent.XMin && Km.X < Extent.XMax && Km.Y > Extent.YMin && Km.Y < Extent.YMax; };

	// ---- lakes first (a river is not cut where it runs through one)
	const TArray<TSharedPtr<FJsonValue>>* LakeArray = nullptr;
	if (Json->TryGetArrayField(TEXT("lakes"), LakeArray))
	{
		for (const TSharedPtr<FJsonValue>& V : *LakeArray)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			FCampaign1851Lake L;
			L.Name = O->GetStringField(TEXT("name"));
			L.CentreKm = Proj.Forward(O->GetNumberField(TEXT("lat")), O->GetNumberField(TEXT("lon")));
			L.A = float(O->GetNumberField(TEXT("a")));
			L.B = float(O->GetNumberField(TEXT("b")));
			L.RotDeg = float(O->GetNumberField(TEXT("rot")));
			L.Seed = GetTypeHash(L.Name);
			if (!Inside(L.CentreKm))
			{
				continue;
			}
			const FVector2D U(FMath::Cos(FMath::DegreesToRadians(L.RotDeg)), FMath::Sin(FMath::DegreesToRadians(L.RotDeg)));
			const FVector2D Vv(-U.Y, U.X);
			for (int32 k = 0; k < 48; ++k)
			{
				const double T = k / 48.0 * UE_DOUBLE_TWO_PI;
				const float W = LakeWobble(L.Seed, T);
				L.Km.Add(L.CentreKm + U * (L.A * W * FMath::Cos(T)) + Vv * (L.B * W * FMath::Sin(T)));
			}
			Lakes.Add(MoveTemp(L));
		}
	}

	// ---- rivers and canals
	const TArray<TSharedPtr<FJsonValue>>* RiverArray = nullptr;
	if (Json->TryGetArrayField(TEXT("rivers"), RiverArray))
	{
		for (const TSharedPtr<FJsonValue>& V : *RiverArray)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			FCampaign1851River R;
			R.Name = O->GetStringField(TEXT("name"));
			R.Class = FMath::Clamp(int32(O->GetNumberField(TEXT("class"))), 0, 3);
			R.bCanal = O->GetStringField(TEXT("kind")) == TEXT("canal");
			TArray<FVector2D> Raw;
			for (const TSharedPtr<FJsonValue>& P : O->GetArrayField(TEXT("points")))
			{
				const TArray<TSharedPtr<FJsonValue>>& LL = P->AsArray();
				if (LL.Num() >= 2)
				{
					Raw.Add(Proj.Forward(LL[0]->AsNumber(), LL[1]->AsNumber()));
				}
			}
			if (Raw.Num() < 2)
			{
				continue;
			}
			// Bends: two waves of different length across the course, none at the ends; a canal runs straight.
			const uint32 RSeed = GetTypeHash(R.Name);
			const double Amp = R.bCanal ? 0.0 : (R.Class == 0 ? 0.22 : R.Class == 1 ? 0.35 : R.Class == 2 ? 0.5 : 0.25);
			const double P1 = 6.28 * HydroHash01(RSeed, 7), P2 = 6.28 * HydroHash01(RSeed, 8);
			const double Total = LineLength(Raw);
			TArray<FVector2D> Course;
			for (double d = 0.0; d <= Total; d += 0.2)
			{
				FVector2D Dir;
				const FVector2D P = AlongLine(Raw, d, &Dir);
				const FVector2D N(-Dir.Y, Dir.X);
				const double Fade = FMath::Clamp(FMath::Min(d, Total - d) / 1.5, 0.0, 1.0);
				Course.Add(P + N * (Amp * Fade * (0.65 * FMath::Sin(d / 2.6 * UE_DOUBLE_TWO_PI / 2.0 + P1) + 0.35 * FMath::Sin(d / 1.1 * UE_DOUBLE_TWO_PI / 2.0 + P2))));
			}
			Course.Add(Raw.Last());
			// On to the sea: straight on from the mouth, else to the nearest water within 4 km.
			if (!IsSea(Course.Last()))
			{
				const FVector2D Dir = (Course.Last() - Course[FMath::Max(0, Course.Num() - 3)]).GetSafeNormal();
				bool bFound = false;
				for (double t = 0.1; t <= 5.0 && !bFound; t += 0.1)
				{
					if (IsSea(Course.Last() + Dir * t))
					{
						const FVector2D From = Course.Last();
						for (double s = 0.2; s < t; s += 0.2) { Course.Add(From + Dir * s); }
						Course.Add(From + Dir * (t + 0.05));
						bFound = true;
					}
				}
				for (double Rad = 0.2; Rad <= 4.0 && !bFound; Rad += 0.2)
				{
					for (int32 a = 0; a < 24 && !bFound; ++a)
					{
						const FVector2D Q = Course.Last() + FVector2D(FMath::Cos(a * UE_PI / 12.0), FMath::Sin(a * UE_PI / 12.0)) * Rad;
						if (IsSea(Q))
						{
							Course.Add(Q);
							bFound = true;
						}
					}
				}
			}
			// The course ends where it first meets the sea (the map's coast; a fjord the map has as sea).
			for (int32 i = 1; i < Course.Num(); ++i)
			{
				if (IsSea(Course[i]))
				{
					Course.SetNum(i + 1);
					break;
				}
			}
			// Only what lies on the map.
			TArray<FVector2D> Piece;
			for (const FVector2D& P : Course)
			{
				if (Inside(P))
				{
					Piece.Add(P);
				}
				else if (Piece.Num() > 0)
				{
					break;
				}
			}
			if (Piece.Num() < 2)
			{
				continue;
			}
			R.Km = MoveTemp(Piece);
			R.WidthM = RiverWidthM(R.Class) * (R.bCanal ? 1.2f : 1.f);
			const int32 Index = Rivers.Add(MoveTemp(R));
			const FCampaign1851River& Added = Rivers[Index];
			for (int32 s = 0; s + 1 < Added.Km.Num(); ++s)
			{
				const FIntPoint A = HydroCell(Added.Km[s]), B = HydroCell(Added.Km[s + 1]);
				for (int32 y = FMath::Min(A.Y, B.Y); y <= FMath::Max(A.Y, B.Y); ++y)
				{
					for (int32 x = FMath::Min(A.X, B.X); x <= FMath::Max(A.X, B.X); ++x)
					{
						RiverCells.FindOrAdd(FIntPoint(x, y)).Add(FIntPoint(Index, s));
					}
				}
			}
		}
	}
	double Length = 0.0;
	for (const FCampaign1851River& R : Rivers)
	{
		Length += LineLength(R.Km);
		// The larger rivers are named on the map, half-way along.
		if (R.Class >= 1 && LineLength(R.Km) > 12.0)
		{
			FCampaign1851Label Label;
			Label.Text = R.Name;
			Label.Kind = TEXT("river");
			const FVector2D LatLon = Proj.Inverse(AlongLine(R.Km, LineLength(R.Km) * 0.45));
			Label.Lat = LatLon.X;
			Label.Lon = LatLon.Y;
			Label.World = Project(Label.Lat, Label.Lon);
			Labels.Add(Label);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|hydro|rivers=%d|%.0f km|lakes=%d"), Rivers.Num(), Length, Lakes.Num());
	return true;
}

bool ACampaign1851Map::IsLake(const FVector2D& Km, float MarginKm, int32* OutLake) const
{
	for (int32 i = 0; i < Lakes.Num(); ++i)
	{
		const FCampaign1851Lake& L = Lakes[i];
		const FVector2D D = Km - L.CentreKm;
		if (D.SizeSquared() > FMath::Square(L.A * 1.25f + MarginKm))
		{
			continue;
		}
		const double R = FMath::DegreesToRadians(L.RotDeg);
		const double U = D.X * FMath::Cos(R) + D.Y * FMath::Sin(R), V = -D.X * FMath::Sin(R) + D.Y * FMath::Cos(R);
		const double T = FMath::Atan2(V / L.B, U / L.A);
		const double Rho = FMath::Sqrt(FMath::Square(U / L.A) + FMath::Square(V / L.B));
		if (Rho < LakeWobble(L.Seed, T) + MarginKm / FMath::Min(L.A, L.B))
		{
			if (OutLake) { *OutLake = i; }
			return true;
		}
	}
	return false;
}

double ACampaign1851Map::RiverDistanceKm(const FVector2D& Km, int32* OutRiver) const
{
	double Best = 1e9;
	if (const TArray<FIntPoint>* List = RiverCells.Find(HydroCell(Km)))
	{
		for (const FIntPoint& RS : *List)
		{
			const FCampaign1851River& R = Rivers[RS.X];
			const double D = SegmentDistance(Km, R.Km[RS.Y], R.Km[RS.Y + 1]);
			if (D < Best)
			{
				Best = D;
				if (OutRiver) { *OutRiver = RS.X; }
			}
		}
	}
	return Best;
}

bool ACampaign1851Map::IsFreshWater(const FVector2D& Km, float MarginKm) const
{
	if (IsLake(Km, MarginKm))
	{
		return true;
	}
	int32 River = INDEX_NONE;
	const double D = RiverDistanceKm(Km, &River);
	return River != INDEX_NONE && D < RiverDrawnKm(Rivers[River].Class) * 0.5f + MarginKm;
}

EHedgeKind ACampaign1851Map::HedgeKindAt(const FVector2D& Km) const
{
	const FVector2D LatLon = Extent.Projection.Inverse(Km);
	const double Lat = LatLon.X, Lon = LatLon.Y;
	const float HeightM = SampleHeight01(UvFromKm(Km)) * 172.f;
	// The marsh of the west coast (Tønder, Ribe, Eiderstedt, Dithmarschen): ditches, no hedges.
	if (HeightM < 3.f && Lon < 9.25 && Lat < 55.6)
	{
		return EHedgeKind::Ditch;
	}
	// The knicks: hedges on earth banks in the duchies and the east of Jutland.
	if ((Lat < 55.55 && Lon < 10.05) || (Lat < 54.6 && Lon < 11.3) || (Lat < 56.6 && Lon > 9.35 && Lon < 10.7 && Lat > 55.5))
	{
		return EHedgeKind::Knick;
	}
	// Stone dikes on the islands, earth dikes on the heath.
	return EHedgeKind::Dike;
}

void ACampaign1851Map::BuildHydroMeshes()
{
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, HydroMaterialPath);
	if (!Material)
	{
		return;
	}
	const FLinearColor Water = FLinearColor::FromSRGBColor(FColor(78, 124, 160));
	// Rivers by class (one ribbon mesh each); small ones join the roads' zoom, large ones always show.
	for (int32 c = 0; c < 4; ++c)
	{
		TArray<TArray<FVector>> Local;
		for (const FCampaign1851River& R : Rivers)
		{
			if (R.Class != c)
			{
				continue;
			}
			TArray<FVector>& Out = Local.AddDefaulted_GetRef();
			for (const FVector2D& P : R.Km)
			{
				Out.Add(LocalAtKm(P) + FVector(0.0, 0.0, 0.9));
			}
		}
		if (Local.Num() == 0)
		{
			continue;
		}
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("Rivers%d"), c));
		Mesh->SetupAttachment(Root);
		Mesh->SetStaticMesh(Campaign1851Scenery::BuildRibbons(Local, RiverDrawnKm(c) * 0.5f * float(KmToUnits), c == 3 ? Water * 0.9f : Water, Material, *FString::Printf(TEXT("SM_Campaign1851_Rivers%d"), c)));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->RegisterComponent();
		RiverMeshes.Add(Mesh);
	}
	// Lakes: draped on the ground in rings, a lighter shore band round darker deep water.
	TArray<FVector> Shore, Deep;
	for (const FCampaign1851Lake& L : Lakes)
	{
		const int32 N = L.Km.Num();
		const float Rings[] = { 1.f, 0.8f, 0.5f, 0.2f };
		auto At = [&](int32 Ring, int32 k)
		{
			const FVector2D P = L.CentreKm + (L.Km[k % N] - L.CentreKm) * Rings[Ring];
			return LocalAtKm(P) + FVector(0.0, 0.0, 1.1 + 0.15 * Ring);
		};
		for (int32 Ring = 0; Ring < 3; ++Ring)
		{
			TArray<FVector>& Into = Ring == 0 ? Shore : Deep;
			for (int32 k = 0; k < N; ++k)
			{
				Into.Append({ At(Ring, k), At(Ring, k + 1), At(Ring + 1, k + 1) });
				Into.Append({ At(Ring, k), At(Ring + 1, k + 1), At(Ring + 1, k) });
			}
		}
		const FVector Centre = LocalAtKm(L.CentreKm) + FVector(0.0, 0.0, 1.6);
		for (int32 k = 0; k < N; ++k)
		{
			Deep.Append({ At(3, k), At(3, k + 1), Centre });
		}
	}
	int32 Part = 0;
	for (const TArray<FVector>* Tris : { &Shore, &Deep })
	{
		if (Tris->Num() == 0)
		{
			continue;
		}
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("Lakes%d"), Part));
		Mesh->SetupAttachment(Root);
		Mesh->SetStaticMesh(Campaign1851Scenery::BuildFlat(*Tris, Part == 0 ? Water * 1.12f : Water * 0.92f, Material, *FString::Printf(TEXT("SM_Campaign1851_Lakes%d"), Part)));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->RegisterComponent();
		RiverMeshes.Add(Mesh);
		++Part;
	}
}

void ACampaign1851Map::UpdateHydroVisibility(float CameraDistanceKm)
{
	// Rivers0 and Rivers1 (brooks and rivers) with the roads; the large rivers and the lakes always.
	for (UStaticMeshComponent* Mesh : RiverMeshes)
	{
		if (Mesh)
		{
			const FString N = Mesh->GetName();
			const bool bSmall = N == TEXT("Rivers0") || N == TEXT("Rivers1");
			Mesh->SetVisibility(!bSmall || CameraDistanceKm < RoadsMaxDistanceKm);
		}
	}
}
