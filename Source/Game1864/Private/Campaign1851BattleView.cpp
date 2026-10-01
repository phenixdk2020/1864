// The battlefield in 3D (Docs/Battlefield1851.md): the generated ground built as a model far out beside the
// campaign map, where the camera can go and look at it. The ground takes the picture's colours (fields,
// meadows, woods, water, towns, contours) on the height grid, with the roads, railways, rivers, houses,
// farms, churches, woods and field boundaries standing on it. No units yet: the 3D battle game is to put
// them there.

#include "Campaign1851Map.h"
#include "Campaign1851Scenery.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	const TCHAR* BattleViewMaterialPath = TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery");
	/** The model: 5 units a metre, the height 1.5 times. */
	constexpr double ViewUnitsPerM = 5.0;
	constexpr double ViewHeightScale = 1.5;
	constexpr int32 ViewGrid = 256;
	constexpr int32 ViewImage = 512;

	float ViewHash01(uint32 A, uint32 B)
	{
		uint32 H = HashCombine(A * 2246822519u + 3266489917u, B);
		H ^= H >> 13; H *= 0x5bd1e995u; H ^= H >> 15;
		return (H & 0xffffff) / float(0xffffff);
	}
}

FVector ACampaign1851Map::BattleViewOrigin() const
{
	// Well beyond the map sheet and its backdrop.
	return FVector(3000000.0, 0.0, 0.0);
}

FVector2D ACampaign1851Map::BattleViewHalfExtent() const
{
	const double Half = Battlefield.SizeKm * 500.0 * ViewUnitsPerM;
	return FVector2D(Half, Half);
}

void ACampaign1851Map::BuildBattleView()
{
	for (UActorComponent* C : BattleViewParts)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	BattleViewParts.Reset();
	const FCampaign1851Battlefield& B = Battlefield;
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, BattleViewMaterialPath);
	if (!B.IsValid() || !Material)
	{
		return;
	}
	const double SizeM = B.SizeKm * 1000.0;
	const double Cell = SizeM / ViewGrid;
	const FVector Origin = BattleViewOrigin();
	float MinH = TNumericLimits<float>::Max();
	for (float H : B.HeightM) { MinH = FMath::Min(MinH, H); }
	// Height in metres at a point (bilinear over the grid; row 0 at the south).
	auto HeightAt = [&](double X, double Y)
	{
		const double Gx = FMath::Clamp(X / Cell - 0.5, 0.0, ViewGrid - 1.001), Gy = FMath::Clamp(Y / Cell - 0.5, 0.0, ViewGrid - 1.001);
		const int32 I = int32(Gx), J = int32(Gy);
		const float Ax = float(Gx - I), Ay = float(Gy - J);
		auto H = [&](int32 x, int32 y) { return B.HeightM[FMath::Min(y, ViewGrid - 1) * ViewGrid + FMath::Min(x, ViewGrid - 1)]; };
		return FMath::Lerp(FMath::Lerp(H(I, J), H(I + 1, J), Ax), FMath::Lerp(H(I, J + 1), H(I + 1, J + 1), Ax), Ay);
	};
	// Battlefield metres (x east, y north, origin south-west) to map-local units (world Y runs south).
	auto Local = [&](double X, double Y, double Lift = 0.0)
	{
		return Origin + FVector((X - SizeM * 0.5) * ViewUnitsPerM, -(Y - SizeM * 0.5) * ViewUnitsPerM, (HeightAt(X, Y) - MinH) * ViewUnitsPerM * ViewHeightScale + Lift);
	};
	auto Pixel = [&](double X, double Y)
	{
		const int32 Px = FMath::Clamp(int32(X / SizeM * ViewImage), 0, ViewImage - 1), Py = FMath::Clamp(int32((SizeM - Y) / SizeM * ViewImage), 0, ViewImage - 1);
		const TArray<FColor>& Src = B.GroundPixels.Num() > 0 ? B.GroundPixels : B.Pixels;
		return Src.IsValidIndex(Py * ViewImage + Px) ? Src[Py * ViewImage + Px] : FColor(140, 150, 90);
	};
	auto AddMesh = [&](UStaticMesh* Mesh, const TCHAR* Name)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Name));
		C->SetupAttachment(Root);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->RegisterComponent();
		BattleViewParts.Add(C);
	};

	// ---- the ground: two triangles a cell, coloured from the picture (it has the contours and shade)
	{
		const int32 N = ViewImage;   // a quad a pixel of the picture (8-23 m), so the field edges stay sharp
		const double Step = SizeM / N;
		TArray<FVector> Tris;
		TArray<FLinearColor> Colours;
		Tris.Reserve(N * N * 6);
		Colours.Reserve(N * N * 2);
		for (int32 j = 0; j < N; ++j)
		{
			for (int32 i = 0; i < N; ++i)
			{
				const double X0 = i * Step, Y0 = j * Step, X1 = X0 + Step, Y1 = Y0 + Step;
				const FLinearColor C = FLinearColor::FromSRGBColor(Pixel((X0 + X1) * 0.5, (Y0 + Y1) * 0.5));
				Tris.Append({ Local(X0, Y0), Local(X1, Y0), Local(X1, Y1) });
				Tris.Append({ Local(X0, Y0), Local(X1, Y1), Local(X0, Y1) });
				Colours.Add(C);
				Colours.Add(C);
			}
		}
		AddMesh(Campaign1851Scenery::BuildColoured(Tris, Colours, Material, TEXT("SM_Campaign1851_BattleGround")), TEXT("BattleGround"));
	}

	// ---- ribbons: rivers, roads, chausséer, lanes, tracks, railways
	auto Ribbons = [&](const TArray<TArray<FVector2D>>& Lines, double WidthM, const FLinearColor& Colour, double LiftM, const TCHAR* Name)
	{
		TArray<TArray<FVector>> Pts;
		for (const TArray<FVector2D>& L : Lines)
		{
			TArray<FVector>& Out = Pts.AddDefaulted_GetRef();
			for (int32 i = 0; i + 1 < L.Num(); ++i)
			{
				const double Len = FVector2D::Distance(L[i], L[i + 1]);
				const int32 Steps = FMath::Max(1, FMath::CeilToInt(Len / 15.0));
				for (int32 s = 0; s < Steps; ++s)
				{
					const FVector2D P = FMath::Lerp(L[i], L[i + 1], double(s) / Steps);
					Out.Add(Local(P.X, P.Y, (LiftM + 1.5) * ViewUnitsPerM));
				}
			}
			if (L.Num() > 0)
			{
				Out.Add(Local(L.Last().X, L.Last().Y, (LiftM + 1.5) * ViewUnitsPerM));
			}
		}
		if (Pts.Num() > 0)
		{
			AddMesh(Campaign1851Scenery::BuildRibbons(Pts, float(WidthM * 0.5 * ViewUnitsPerM), Colour, Material, *FString::Printf(TEXT("SM_Campaign1851_Battle%s"), Name)), Name);
		}
	};
	{
		TArray<TArray<FVector2D>> Water;
		for (const FCampaign1851BattleRiver& R : B.Rivers)
		{
			Water.Add(R.M);
		}
		Ribbons(Water, 14.0, FLinearColor::FromSRGBColor(FColor(74, 112, 140)), 0.4, TEXT("BattleRivers"));
	}
	Ribbons(B.Tracks, 3.0, FLinearColor::FromSRGBColor(FColor(150, 128, 92)), 0.5, TEXT("BattleTracks"));
	Ribbons(B.Lanes, 5.0, FLinearColor::FromSRGBColor(FColor(196, 172, 128)), 0.6, TEXT("BattleLanes"));
	Ribbons(B.Roads, 8.0, FLinearColor::FromSRGBColor(FColor(222, 202, 156)), 0.7, TEXT("BattleRoads"));
	Ribbons(B.Chaussees, 10.0, FLinearColor::FromSRGBColor(FColor(240, 232, 206)), 0.8, TEXT("BattleChaussees"));
	Ribbons(B.Rails, 5.0, FLinearColor::FromSRGBColor(FColor(52, 46, 42)), 0.9, TEXT("BattleRails"));

	// ---- where no tree may stand: the ways, the water and round the buildings (a mask of 6 m squares)
	const double MaskM = 6.0;
	const int32 MaskN = FMath::CeilToInt(SizeM / MaskM);
	TArray<uint8> Clear;
	Clear.SetNumZeroed(MaskN * MaskN);
	auto ClearDisc = [&](const FVector2D& P, double R)
	{
		const int32 X0 = FMath::FloorToInt((P.X - R) / MaskM), X1 = FMath::FloorToInt((P.X + R) / MaskM);
		const int32 Y0 = FMath::FloorToInt((P.Y - R) / MaskM), Y1 = FMath::FloorToInt((P.Y + R) / MaskM);
		for (int32 y = FMath::Max(Y0, 0); y <= FMath::Min(Y1, MaskN - 1); ++y)
		{
			for (int32 x = FMath::Max(X0, 0); x <= FMath::Min(X1, MaskN - 1); ++x)
			{
				Clear[y * MaskN + x] = 1;
			}
		}
	};
	auto ClearLines = [&](const TArray<TArray<FVector2D>>& Lines, double HalfM)
	{
		for (const TArray<FVector2D>& L : Lines)
		{
			for (int32 i = 0; i + 1 < L.Num(); ++i)
			{
				const double Len = FVector2D::Distance(L[i], L[i + 1]);
				for (double t = 0.0; t <= Len; t += MaskM * 0.5)
				{
					ClearDisc(FMath::Lerp(L[i], L[i + 1], Len > 0.0 ? t / Len : 0.0), HalfM);
				}
			}
		}
	};
	ClearLines(B.Roads, 12.0);
	ClearLines(B.Chaussees, 14.0);
	ClearLines(B.Lanes, 9.0);
	ClearLines(B.Tracks, 6.0);
	ClearLines(B.Rails, 12.0);
	for (const FCampaign1851BattleRiver& R : B.Rivers)
	{
		ClearLines({ R.M }, R.WidthM * 0.5 + 8.0);
	}
	for (const FCampaign1851BattleBuilding& Bd : B.Buildings)
	{
		ClearDisc(Bd.M, FMath::Max(Bd.Size.X, Bd.Size.Y) * 0.5 + 6.0);
	}
	auto IsClear = [&](double X, double Y)
	{
		const int32 x = FMath::Clamp(int32(X / MaskM), 0, MaskN - 1), y = FMath::Clamp(int32(Y / MaskM), 0, MaskN - 1);
		return Clear[y * MaskN + x] != 0;
	};

	// ---- instanced pieces: buildings, woods, field boundaries
	using Campaign1851Scenery::EPiece;
	TMap<int32, TArray<FTransform>> Items;
	auto Yaw = [](double DegNorthOfEast) { return float(-DegNorthOfEast); };   // battlefield yaw (x east, y north) to Unreal's
	for (const FCampaign1851BattleBuilding& Bd : B.Buildings)
	{
		// The scenery pieces stretched to the building's footprint (a cottage is 5.5 x 3.4 piece units).
		const EPiece Piece = Bd.Kind == TEXT("church") ? EPiece::Church : Bd.Kind == TEXT("farm") ? EPiece::Cottage : Bd.Kind == TEXT("house") ? EPiece::TownHouse : EPiece::MerchantHouse;
		const double PieceW = Piece == EPiece::Church ? 14.0 : Piece == EPiece::Cottage ? 5.5 : 6.0;
		const double PieceD = Piece == EPiece::Church ? 6.0 : Piece == EPiece::Cottage ? 3.4 : 4.0;
		const double Sx = Bd.Size.X * ViewUnitsPerM / PieceW, Sy = Bd.Size.Y * ViewUnitsPerM / PieceD;
		Items.FindOrAdd(int32(Piece)).Emplace(FRotator(0.f, Yaw(Bd.Yaw), 0.f), Local(Bd.M.X, Bd.M.Y) - Origin - FVector(0.0, 0.0, 0.5), FVector(Sx, Sy, (Sx + Sy) * 0.5));
	}
	// Trees: in the wood cells (one in about every 15 m square), and a few in the hedges.
	const uint32 Seed32 = HashCombine(uint32(FMath::RoundToInt(B.CentreKm.X * 10.0)), uint32(FMath::RoundToInt(B.CentreKm.Y * 10.0)));
	for (int32 j = 0; j < ViewGrid; ++j)
	{
		for (int32 i = 0; i < ViewGrid; ++i)
		{
			const int32 k = j * ViewGrid + i;
			if (B.Kind[k] != uint8(EBattlefieldCell::Wood))
			{
				continue;
			}
			const int32 Per = FMath::Clamp(int32(Cell * Cell / 225.0 * B.Wood[k] / 100.0), 1, 8);
			for (int32 t = 0; t < Per; ++t)
			{
				const double X = (i + ViewHash01(Seed32, k * 16 + t)) * Cell, Y = (j + ViewHash01(Seed32 + 7u, k * 16 + t)) * Cell;
				if (IsClear(X, Y))
				{
					continue;
				}
				const float Size = 1.6f + 0.8f * ViewHash01(Seed32 + 3u, k * 16 + t);
				const EPiece Tree = ViewHash01(Seed32 + 5u, k * 16 + t) < 0.25f ? EPiece::Conifer : ViewHash01(Seed32 + 9u, k) < 0.3f ? EPiece::Oak : EPiece::Broadleaf;
				Items.FindOrAdd(int32(Tree)).Emplace(FRotator(0.f, 360.f * ViewHash01(Seed32 + 11u, k * 16 + t), 0.f), Local(X, Y) - Origin, FVector(Size * ViewUnitsPerM));
			}
		}
	}
	for (const FCampaign1851BattleHedge& H : B.Hedges)
	{
		const EPiece Piece = H.Kind == EHedgeKind::Knick ? EPiece::Knick : H.Kind == EHedgeKind::Dike ? EPiece::StoneDike : EPiece::Ditch;
		for (int32 s = 0; s + 1 < H.M.Num(); ++s)
		{
			const FVector2D A = H.M[s], C = H.M[s + 1];
			const double Len = FVector2D::Distance(A, C);
			if (Len < 1.0)
			{
				continue;
			}
			const FVector2D Mid = (A + C) * 0.5;
			const double Deg = FMath::RadiansToDegrees(FMath::Atan2(C.Y - A.Y, C.X - A.X));
			Items.FindOrAdd(int32(Piece)).Emplace(FRotator(0.f, Yaw(Deg), 0.f), Local(Mid.X, Mid.Y) - Origin, FVector(Len * ViewUnitsPerM / 4.0, 1.4 * ViewUnitsPerM, 1.4 * ViewUnitsPerM));
		}
	}
	for (const TPair<int32, TArray<FTransform>>& It : Items)
	{
		UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("BattleView")));
		C->SetupAttachment(Root);
		C->SetStaticMesh(Campaign1851Scenery::Build(EPiece(It.Key), Material));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetRelativeLocation(Origin);
		C->RegisterComponent();
		C->AddInstances(It.Value, false, false);
		BattleViewParts.Add(C);
	}
	BattleViewVersion = BattlefieldVersion;
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|battleview|%s|%d parts|%d buildings|%d hedges"), *B.Place, BattleViewParts.Num(), B.Buildings.Num(), B.Hedges.Num());
}

bool ACampaign1851Map::EnterBattleView()
{
	if (!Battlefield.IsValid())
	{
		return false;
	}
	if (BattleViewVersion != BattlefieldVersion || BattleViewParts.Num() == 0)
	{
		BuildBattleView();
	}
	for (UActorComponent* C : BattleViewParts)
	{
		if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C)) { P->SetVisibility(true); }
	}
	bBattleView = true;
	return true;
}

void ACampaign1851Map::LeaveBattleView()
{
	bBattleView = false;
	for (UActorComponent* C : BattleViewParts)
	{
		if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C)) { P->SetVisibility(false); }
	}
}

float ACampaign1851Map::BattleViewGroundZ() const
{
	// The middle of the model, about where the camera looks.
	const FCampaign1851Battlefield& B = Battlefield;
	if (!B.IsValid())
	{
		return 0.f;
	}
	float MinH = TNumericLimits<float>::Max(), Sum = 0.f;
	for (float H : B.HeightM) { MinH = FMath::Min(MinH, H); Sum += H; }
	return float((Sum / B.HeightM.Num() - MinH) * ViewUnitsPerM * ViewHeightScale);
}
