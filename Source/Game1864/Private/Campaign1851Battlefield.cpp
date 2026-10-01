// The battlefield generator (Docs/Battlefield1851.md). From a place on the campaign map it builds the ground
// for a 3D battle: a square of 4-12 km around the point on a 256 x 256 grid, with the height (the map's height
// data and a fine relief from the seed), the sea and the meadows by the coast, woods, towns with their
// streets and houses, farms in the fields, the roads and chausséer, the railways, the garrison buildings and
// the forts with their trenches, the season and the day's weather. It writes Saved/Battle/Battlefield_*.json
// (for the battle game) and a picture of the ground (a PNG and the preview in the SLAGMARK window).
// The units come later.

#include "Campaign1851Map.h"
#include "Campaign1851ConstructionSite.h"

#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	/** The height data's full scale in metres (Denmark's highest ground is about 170 m; estimate). */
	constexpr float BfMaxHeightM = 172.f;
	constexpr int32 BfGrid = 256;
	constexpr int32 BfImage = 512;

	float BfHash(uint32 A, uint32 B)
	{
		uint32 H = HashCombine(A * 747796405u + 2891336453u, B);
		H ^= H >> 16; H *= 0x7feb352du; H ^= H >> 15; H *= 0x846ca68bu; H ^= H >> 16;
		return (H & 0xffffff) / float(0xffffff);
	}

	/** Smooth value noise in [0, 1] over a lattice of Lattice metres. */
	float BfNoise(uint32 Seed, double X, double Y, double Lattice)
	{
		const double Fx = X / Lattice, Fy = Y / Lattice;
		const int32 Ix = FMath::FloorToInt(Fx), Iy = FMath::FloorToInt(Fy);
		const float Tx = float(Fx - Ix), Ty = float(Fy - Iy);
		auto L = [&](int32 A, int32 B) { return BfHash(Seed, uint32(A * 73856093) ^ uint32(B * 19349663)); };
		const float Sx = Tx * Tx * (3.f - 2.f * Tx), Sy = Ty * Ty * (3.f - 2.f * Ty);
		return FMath::Lerp(FMath::Lerp(L(Ix, Iy), L(Ix + 1, Iy), Sx), FMath::Lerp(L(Ix, Iy + 1), L(Ix + 1, Iy + 1), Sx), Sy);
	}

	float BfFractal(uint32 Seed, double X, double Y, double Lattice)
	{
		return 0.55f * BfNoise(Seed, X, Y, Lattice) + 0.3f * BfNoise(Seed + 1, X, Y, Lattice / 2.3) + 0.15f * BfNoise(Seed + 2, X, Y, Lattice / 5.1);
	}

	FColor BfMix(const FColor& A, const FColor& B, float T)
	{
		return FColor(uint8(FMath::Lerp(float(A.R), float(B.R), T)), uint8(FMath::Lerp(float(A.G), float(B.G), T)), uint8(FMath::Lerp(float(A.B), float(B.B), T)), 255);
	}

	FColor BfScale(const FColor& C, float F)
	{
		return FColor(uint8(FMath::Clamp(C.R * F, 0.f, 255.f)), uint8(FMath::Clamp(C.G * F, 0.f, 255.f)), uint8(FMath::Clamp(C.B * F, 0.f, 255.f)), 255);
	}

	/** Clips a polyline (km) to the square and turns it into local metres; pieces outside are dropped. */
	void AddClipped(const TArray<FVector2D>& Km, const FVector2D& Origin, float SizeM, TArray<TArray<FVector2D>>& Out)
	{
		TArray<FVector2D> Piece;
		auto Inside = [SizeM](const FVector2D& M) { return M.X >= -50.f && M.Y >= -50.f && M.X <= SizeM + 50.f && M.Y <= SizeM + 50.f; };
		// A segment counts when its bounding box touches the square (long straight roads cross it end to end).
		auto Touches = [SizeM](const FVector2D& A, const FVector2D& B)
		{
			return FMath::Max(A.X, B.X) >= -50.f && FMath::Min(A.X, B.X) <= SizeM + 50.f && FMath::Max(A.Y, B.Y) >= -50.f && FMath::Min(A.Y, B.Y) <= SizeM + 50.f;
		};
		for (int32 i = 0; i < Km.Num(); ++i)
		{
			const FVector2D M = (Km[i] - Origin) * 1000.0;
			const bool bIn = Inside(M);
			const bool bNear = bIn || (i > 0 && Touches((Km[i - 1] - Origin) * 1000.0, M)) || (i + 1 < Km.Num() && Touches(M, (Km[i + 1] - Origin) * 1000.0));
			if (bNear)
			{
				Piece.Add(M);
			}
			else if (Piece.Num() > 0)
			{
				if (Piece.Num() > 1) { Out.Add(Piece); }
				Piece.Reset();
			}
		}
		if (Piece.Num() > 1)
		{
			Out.Add(Piece);
		}
	}
}

bool ACampaign1851Map::GenerateBattlefield(FVector2D CentreKm, float InSizeKm, FString Name)
{
	FCampaign1851Battlefield& B = Battlefield;
	B = FCampaign1851Battlefield();
	B.CentreKm = CentreKm;
	B.SizeKm = FMath::Clamp(InSizeKm, 2.f, 16.f);
	B.Name = Name;
	B.Day = CampaignDays;
	const float SizeM = B.SizeKm * 1000.f;
	const float Cell = SizeM / BfGrid;
	const FVector2D Origin = CentreKm - FVector2D(B.SizeKm * 0.5, B.SizeKm * 0.5);
	// The detail is the same every time for the same place (the seed and where it lies).
	const uint32 PlaceSeed = HashCombine(uint32(Seed), HashCombine(uint32(FMath::RoundToInt(CentreKm.X * 10.0)), uint32(FMath::RoundToInt(CentreKm.Y * 10.0))));
	const int32 Near = NearestTown(CentreKm);
	B.Place = Cities.IsValidIndex(Near) ? Cities[Near].Name : FString(TEXT("?"));
	const FVector2D LatLon = Extent.Projection.Inverse(CentreKm);
	B.Lat = LatLon.X;
	B.Lon = LatLon.Y;
	const int32 Month = GetDate().GetMonth();
	B.bSnow = GetWeather() == ECampaign1851Weather::Snow || ((Month == 12 || Month <= 2) && GetTemperature() < 0.f);

	// ---- the grid: height, sea, woods
	B.HeightM.SetNumZeroed(BfGrid * BfGrid);
	B.Kind.SetNumZeroed(BfGrid * BfGrid);
	B.Wood.SetNumZeroed(BfGrid * BfGrid);
	for (int32 j = 0; j < BfGrid; ++j)
	{
		for (int32 i = 0; i < BfGrid; ++i)
		{
			const FVector2D M((i + 0.5f) * Cell, (j + 0.5f) * Cell);
			const FVector2D Km = Origin + M / 1000.0;
			const int32 k = j * BfGrid + i;
			if (IsSea(Km))
			{
				B.Kind[k] = uint8(EBattlefieldCell::Sea);
				B.HeightM[k] = -2.f;
				continue;
			}
			if (IsLake(Km))
			{
				B.Kind[k] = uint8(EBattlefieldCell::Water);
				B.HeightM[k] = FMath::Max(0.3f, SampleHeight01(UvFromKm(Km)) * BfMaxHeightM - 1.5f);
				continue;
			}
			// The map's height (about 126 m a pixel) and a fine relief: small knolls and hollows.
			const float Base = SampleHeight01(UvFromKm(Km)) * BfMaxHeightM;
			const float Relief = (BfFractal(PlaceSeed, M.X, M.Y, 420.0) - 0.5f) * FMath::Min(6.f, 1.f + Base * 0.15f);
			B.HeightM[k] = FMath::Max(0.3f, Base + Relief);
			// Woods: the map's woodland, given ragged edges and clearings.
			const float W = Woodland(Km) + 0.45f * (BfFractal(PlaceSeed + 11u, M.X, M.Y, 300.0) - 0.5f);
			B.Wood[k] = uint8(FMath::Clamp(W, 0.f, 1.f) * 100.f);
			B.Kind[k] = uint8(W > 0.5f ? EBattlefieldCell::Wood : EBattlefieldCell::Field);
		}
	}
	// Meadows and marsh: the low ground by the shore.
	for (int32 j = 0; j < BfGrid; ++j)
	{
		for (int32 i = 0; i < BfGrid; ++i)
		{
			const int32 k = j * BfGrid + i;
			if (B.Kind[k] != uint8(EBattlefieldCell::Field) || B.HeightM[k] > 3.f)
			{
				continue;
			}
			bool bShore = false;
			for (int32 d = 0; d < 8 && !bShore; ++d)
			{
				const int32 R = FMath::Max(2, int32(350.f / Cell));
				const int32 x = i + FMath::RoundToInt(FMath::Cos(d * UE_PI / 4.f) * R), y = j + FMath::RoundToInt(FMath::Sin(d * UE_PI / 4.f) * R);
				bShore = x >= 0 && y >= 0 && x < BfGrid && y < BfGrid && B.Kind[y * BfGrid + x] == uint8(EBattlefieldCell::Sea);
			}
			if (bShore)
			{
				B.Kind[k] = uint8(EBattlefieldCell::Meadow);
			}
		}
	}

	// ---- rivers and lakes (Campaign1851Hydro.cpp): the course in metres, water in the cells it fills (wide
	// rivers), wet meadows along the banks; the lakes' shores as polygons.
	for (const FCampaign1851River& Rv : Rivers)
	{
		TArray<TArray<FVector2D>> Pieces;
		AddClipped(Rv.Km, Origin, SizeM, Pieces);
		for (TArray<FVector2D>& P : Pieces)
		{
			// A finer course: small bends every 40 m on top of the map's.
			TArray<FVector2D> Fine;
			for (int32 i = 0; i + 1 < P.Num(); ++i)
			{
				const FVector2D D = P[i + 1] - P[i];
				const float Len = D.Size();
				const FVector2D N = Len > 0.f ? FVector2D(-D.Y, D.X) / Len : FVector2D::ZeroVector;
				for (float t = 0.f; t < Len; t += 40.f)
				{
					const FVector2D Q = P[i] + D * (t / FMath::Max(Len, 1.f));
					const float Wig = (BfNoise(PlaceSeed + 31u, Q.X, Q.Y, 180.0) - 0.5f) * FMath::Min(60.f, 6.f * Rv.WidthM);
					Fine.Add(Q + N * Wig);
				}
			}
			Fine.Add(P.Last());
			B.Rivers.Add({ Rv.Name, Rv.WidthM, Fine });
		}
	}
	for (const FCampaign1851Lake& Lk : Lakes)
	{
		TArray<TArray<FVector2D>> Shore;
		TArray<FVector2D> Closed = Lk.Km;
		Closed.Add(Lk.Km[0]);
		AddClipped(Closed, Origin, SizeM, Shore);
		if (Shore.Num() > 0)
		{
			TArray<FVector2D> Poly;
			for (const FVector2D& P : Closed) { Poly.Add((P - Origin) * 1000.0); }
			B.Lakes.Add(Poly);
		}
	}
	for (int32 j = 0; j < BfGrid; ++j)
	{
		for (int32 i = 0; i < BfGrid; ++i)
		{
			const int32 k = j * BfGrid + i;
			if (B.Kind[k] == uint8(EBattlefieldCell::Sea) || B.Kind[k] == uint8(EBattlefieldCell::Water))
			{
				continue;
			}
			const FVector2D M((i + 0.5f) * Cell, (j + 0.5f) * Cell);
			for (const FCampaign1851BattleRiver& Rv : B.Rivers)
			{
				float Best = 1e9f;
				for (int32 s = 0; s + 1 < Rv.M.Num(); ++s)
				{
					const FVector2D D = Rv.M[s + 1] - Rv.M[s];
					const float T = FMath::Clamp(float(FVector2D::DotProduct(M - Rv.M[s], D) / FMath::Max(D.SizeSquared(), 1.0)), 0.f, 1.f);
					Best = FMath::Min(Best, float(FVector2D::Distance(M, Rv.M[s] + D * T)));
				}
				if (Best < Rv.WidthM * 0.5f && Rv.WidthM > Cell * 0.6f)
				{
					B.Kind[k] = uint8(EBattlefieldCell::Water);
					B.HeightM[k] = FMath::Max(0.2f, B.HeightM[k] - 1.5f);
					break;
				}
				if (Best < 60.f + Rv.WidthM * 2.f && B.Kind[k] == uint8(EBattlefieldCell::Field))
				{
					B.Kind[k] = uint8(EBattlefieldCell::Meadow);   // wet meadow (eng) by the water
					B.HeightM[k] = FMath::Max(0.3f, B.HeightM[k] - 0.6f * (1.f - Best / (60.f + Rv.WidthM * 2.f)));
				}
			}
		}
	}

	// ---- towns: the built-up area, streets and houses
	FRandomStream Rng{ int32(PlaceSeed) };
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		const FVector2D TM = (TownKm(c) - Origin) * 1000.0;
		const float RadiusM = 1000.f * FMath::Clamp(0.25f * FMath::Sqrt(Cities[c].Population / 1000.f), 0.25f, 2.5f);
		if (TM.X < -RadiusM || TM.Y < -RadiusM || TM.X > SizeM + RadiusM || TM.Y > SizeM + RadiusM)
		{
			continue;
		}
		B.Towns.Add({ Cities[c].Name, TM, RadiusM });
		for (int32 j = 0; j < BfGrid; ++j)
		{
			for (int32 i = 0; i < BfGrid; ++i)
			{
				const FVector2D M((i + 0.5f) * Cell, (j + 0.5f) * Cell);
				const int32 k = j * BfGrid + i;
				const float Edge = RadiusM * (0.85f + 0.3f * BfFractal(PlaceSeed + 23u, M.X, M.Y, 160.0));
				if (B.Kind[k] != uint8(EBattlefieldCell::Sea) && FVector2D::Distance(M, TM) < Edge)
				{
					B.Kind[k] = uint8(EBattlefieldCell::Town);
				}
			}
		}
		// Houses along a street grid, denser towards the centre.
		const float Step = 34.f;
		for (float y = -RadiusM; y <= RadiusM && B.Buildings.Num() < 4000; y += Step)
		{
			for (float x = -RadiusM; x <= RadiusM && B.Buildings.Num() < 4000; x += Step)
			{
				const FVector2D M = TM + FVector2D(x, y);
				const float D = FVector2D(x, y).Size() / RadiusM;
				if (D > 1.f || M.X < 0.f || M.Y < 0.f || M.X > SizeM || M.Y > SizeM || Rng.FRand() > 0.8f - 0.45f * D)
				{
					continue;
				}
				const int32 k = FMath::Clamp(int32(M.Y / Cell), 0, BfGrid - 1) * BfGrid + FMath::Clamp(int32(M.X / Cell), 0, BfGrid - 1);
				if (B.Kind[k] != uint8(EBattlefieldCell::Town) || FMath::Abs(FMath::Fmod(x + 1000.f, Step * 4.f)) < 6.f)
				{
					continue;   // not built on, or the street itself
				}
				B.Buildings.Add({ M + FVector2D(Rng.FRandRange(-4.f, 4.f), Rng.FRandRange(-4.f, 4.f)), FVector2D(Rng.FRandRange(10.f, 22.f), Rng.FRandRange(8.f, 12.f)), 0.f, TEXT("house") });
			}
		}
	}

	// ---- the map's own villages, farms and cottages (the same as on the campaign map)
	auto Farmstead = [&](const FVector2D& M, float FarmYaw)
	{
		B.FarmM.Add(M);
		const FVector2D Ax(FMath::Cos(FMath::DegreesToRadians(FarmYaw)), FMath::Sin(FMath::DegreesToRadians(FarmYaw)));
		const FVector2D Ay(-Ax.Y, Ax.X);
		B.Buildings.Add({ M + Ay * 14.f, FVector2D(34.f, 8.f), FarmYaw, TEXT("farm") });
		B.Buildings.Add({ M - Ay * 14.f, FVector2D(34.f, 8.f), FarmYaw, TEXT("farm") });
		B.Buildings.Add({ M + Ax * 14.f, FVector2D(8.f, 20.f), FarmYaw, TEXT("farm") });
		B.Buildings.Add({ M - Ax * 14.f, FVector2D(8.f, 20.f), FarmYaw, TEXT("farm") });
		++B.Farms;
	};
	for (const FVector& S : CountrySites)
	{
		const FVector2D M = (FVector2D(S.X, S.Y) - Origin) * 1000.0;
		if (M.X < 0.f || M.Y < 0.f || M.X > SizeM || M.Y > SizeM)
		{
			continue;
		}
		if (S.Z < 0.5f)
		{
			// A village: the church and a score of houses and farms around the green.
			B.Buildings.Add({ M, FVector2D(30.f, 12.f), float(Rng.FRandRange(0.f, 20.f)), TEXT("church") });
			B.Villages.Add(M);
			for (int32 h = 0; h < 18; ++h)
			{
				const float A = Rng.FRandRange(0.f, UE_TWO_PI), R = Rng.FRandRange(50.f, 260.f);
				const FVector2D P = M + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
				if (h % 5 == 0)
				{
					Farmstead(P, Rng.FRandRange(0.f, 90.f));
				}
				else
				{
					B.Buildings.Add({ P, FVector2D(Rng.FRandRange(10.f, 16.f), 8.f), float(Rng.FRandRange(0.f, 180.f)), TEXT("house") });
				}
			}
		}
		else if (S.Z < 1.5f)
		{
			Farmstead(M, Rng.FRandRange(0.f, 90.f));
		}
		else
		{
			B.Buildings.Add({ M, FVector2D(12.f, 7.f), float(Rng.FRandRange(0.f, 180.f)), TEXT("house") });
		}
	}

	// ---- farms: four-winged farmsteads in the fields
	for (float y = 60.f; y < SizeM; y += 260.f)
	{
		for (float x = 60.f; x < SizeM; x += 260.f)
		{
			const FVector2D M(x + Rng.FRandRange(-90.f, 90.f), y + Rng.FRandRange(-90.f, 90.f));
			const int32 k = FMath::Clamp(int32(M.Y / Cell), 0, BfGrid - 1) * BfGrid + FMath::Clamp(int32(M.X / Cell), 0, BfGrid - 1);
			if (B.Kind[k] != uint8(EBattlefieldCell::Field) || Rng.FRand() > 0.16f)
			{
				continue;
			}
			const float FarmYaw = Rng.FRandRange(0.f, 90.f);
			const FVector2D Ax(FMath::Cos(FMath::DegreesToRadians(FarmYaw)), FMath::Sin(FMath::DegreesToRadians(FarmYaw)));
			const FVector2D Ay(-Ax.Y, Ax.X);
			B.Buildings.Add({ M + Ay * 14.f, FVector2D(34.f, 8.f), FarmYaw, TEXT("farm") });
			B.Buildings.Add({ M - Ay * 14.f, FVector2D(34.f, 8.f), FarmYaw, TEXT("farm") });
			B.Buildings.Add({ M + Ax * 14.f, FVector2D(8.f, 20.f), FarmYaw, TEXT("farm") });
			B.Buildings.Add({ M - Ax * 14.f, FVector2D(8.f, 20.f), FarmYaw, TEXT("farm") });
			B.FarmM.Add(M);
			++B.Farms;
		}
	}

	// ---- roads, railways, the garrison and civil buildings, the forts
	// The map's main roads and lanes; the chausséer (paved links) over them.
	for (const TArray<FVector2D>& L : RoadLines)
	{
		AddClipped(L, Origin, SizeM, B.Roads);
	}
	for (const TArray<FVector2D>& L : LaneLinesKm)
	{
		AddClipped(L, Origin, SizeM, B.Lanes);
	}
	// The local lanes the campaign map leaves out: village to village, village and town to the road,
	// and a track from every farm to the nearest way.
	{
		TArray<FVector2D> WayPoints;
		auto Collect = [&WayPoints](const TArray<TArray<FVector2D>>& Lines)
		{
			for (const TArray<FVector2D>& L : Lines)
			{
				for (int32 i = 0; i + 1 < L.Num(); ++i)
				{
					const float Len = FVector2D::Distance(L[i], L[i + 1]);
					for (float t = 0.f; t < Len; t += 100.f)
					{
						WayPoints.Add(L[i] + (L[i + 1] - L[i]) * (t / FMath::Max(Len, 1.f)));
					}
				}
			}
		};
		auto Nearest = [&WayPoints](const FVector2D& P, float MaxM, FVector2D& Out)
		{
			float Best = MaxM;
			for (const FVector2D& W : WayPoints)
			{
				const float D = FVector2D::Distance(P, W);
				if (D < Best) { Best = D; Out = W; }
			}
			return Best < MaxM;
		};
		auto Lane = [&](const FVector2D& A, const FVector2D& Bp, TArray<TArray<FVector2D>>& Into)
		{
			const FVector2D D = Bp - A;
			const float Len = D.Size();
			if (Len < 40.f)
			{
				return;
			}
			const FVector2D N(-D.Y / Len, D.X / Len);
			const float Bend = Rng.FRandRange(-0.08f, 0.08f) * Len;
			TArray<FVector2D> Pts;
			const int32 Steps = FMath::Max(2, int32(Len / 80.f));
			for (int32 s = 0; s <= Steps; ++s)
			{
				const float T = float(s) / Steps;
				Pts.Add(A + D * T + N * (Bend * FMath::Sin(T * UE_PI)));
			}
			Into.Add(Pts);
		};
		Collect(B.Roads);
		Collect(B.Chaussees);
		Collect(B.Lanes);
		TArray<FVector2D> Anchors = B.Villages;
		for (const FCampaign1851BattleTown& T : B.Towns)
		{
			Anchors.Add(T.M);
		}
		for (int32 a = 0; a < Anchors.Num(); ++a)
		{
			// To the nearest other village or town, and to the road.
			int32 Other = INDEX_NONE;
			float Best = 3500.f;
			for (int32 o = 0; o < Anchors.Num(); ++o)
			{
				const float D = FVector2D::Distance(Anchors[a], Anchors[o]);
				if (o != a && D < Best) { Best = D; Other = o; }
			}
			if (Other != INDEX_NONE)
			{
				Lane(Anchors[a], Anchors[Other], B.Lanes);
			}
			FVector2D Road;
			if (Nearest(Anchors[a], 3000.f, Road))
			{
				Lane(Anchors[a], Road, B.Lanes);
			}
		}
		Collect(B.Lanes);
		for (const FVector2D& F : B.FarmM)
		{
			FVector2D Road;
			if (Nearest(F, 700.f, Road))
			{
				Lane(F, Road, B.Tracks);
			}
		}
	}
	for (const FCampaign1851Link& L : Links)
	{
		if (L.Km.Num() > 1 && L.bChaussee && !L.HasFerry())
		{
			AddClipped(L.Km, Origin, SizeM, B.Chaussees);
		}
	}
	for (const FCampaign1851Railway& R : Railways)
	{
		if (R.IsOpen(GetDate()) && R.Km.Num() > 1)
		{
			AddClipped(R.Km, Origin, SizeM, B.Rails);
		}
	}
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (!Site)
		{
			continue;
		}
		const FVector2D M = (KmAtWorld(Site->GetActorLocation()) - Origin) * 1000.0;
		if (M.X >= 0.f && M.Y >= 0.f && M.X <= SizeM && M.Y <= SizeM)
		{
			B.Buildings.Add({ M, Site->IsGarrison() ? FVector2D(60.f, 40.f) : FVector2D(26.f, 16.f), 0.f, Site->IsGarrison() ? TEXT("garrison") : Site->GetKind() });
		}
	}
	for (const FCampaign1851Fort& F : Forts)
	{
		const FVector2D M = (F.Km - Origin) * 1000.0;
		if (M.X >= 0.f && M.Y >= 0.f && M.X <= SizeM && M.Y <= SizeM)
		{
			B.Forts.Add(F);
			B.FortM.Add(M);
		}
	}
	for (const FCampaign1851Bridge& Bd : Bridges)
	{
		const FVector2D M = (Bd.Km - Origin) * 1000.0;
		if (Bd.State != EBridgeState::Site && M.X >= 0.f && M.Y >= 0.f && M.X <= SizeM && M.Y <= SizeM)
		{
			B.Bridges.Add(Bd);
			B.BridgeM.Add(M);
		}
	}

	// ---- field boundaries: the sides of the field parcels (the same as the picture's), most with a knick,
	// dike or ditch after the region; gaps for gates, none across water, woods, towns or the ways.
	{
		auto KindAtM = [&](const FVector2D& M)
		{
			const int32 k = FMath::Clamp(int32(M.Y / Cell), 0, BfGrid - 1) * BfGrid + FMath::Clamp(int32(M.X / Cell), 0, BfGrid - 1);
			return EBattlefieldCell(B.Kind[k]);
		};
		auto Open = [&](const FVector2D& M)
		{
			const EBattlefieldCell K = KindAtM(M);
			return K == EBattlefieldCell::Field || K == EBattlefieldCell::Meadow;
		};
		auto NearWay = [&](const FVector2D& M)
		{
			for (const TArray<TArray<FVector2D>>* Set : { &B.Roads, &B.Lanes, &B.Chaussees, &B.Rails })
			{
				for (const TArray<FVector2D>& L : *Set)
				{
					for (int32 s = 0; s + 1 < L.Num(); ++s)
					{
						const FVector2D D = L[s + 1] - L[s];
						if (FMath::Abs(D.X) + FMath::Abs(D.Y) < 1.f) { continue; }
						const float T = FMath::Clamp(float(FVector2D::DotProduct(M - L[s], D) / D.SizeSquared()), 0.f, 1.f);
						if (FVector2D::DistSquared(M, L[s] + D * T) < 14.f * 14.f) { return true; }
					}
				}
			}
			return false;
		};
		const EHedgeKind Region = HedgeKindAt(CentreKm);
		const float Share = Region == EHedgeKind::Knick ? 0.7f : Region == EHedgeKind::Ditch ? 0.6f : 0.45f;
		constexpr float PW = 140.f, PH = 110.f;   // the parcels of RenderBattlefield
		auto RowY = [](float X, int32 Row) { return Row * 110.f - 37.f * FMath::Sin(X / 400.f); };
		// Vertical sides at x = 140 i, horizontal ones along the gently bent rows; segments of half a parcel.
		for (int32 i = 1; i * PW < SizeM && B.Hedges.Num() < 20000; ++i)
		{
			const float X = i * PW;
			TArray<FVector2D> Run;
			for (float Y = 0.f; Y <= SizeM; Y += PH * 0.5f)
			{
				const FVector2D M(X, Y + PH * 0.25f);
				const bool bKeep = Open(M + FVector2D(-20.f, 0.f)) && Open(M + FVector2D(20.f, 0.f)) && !NearWay(M) && BfHash(PlaceSeed + 41u, uint32(i * 7919 + int32(Y))) < Share;
				if (bKeep) { if (Run.Num() == 0) { Run.Add(FVector2D(X, Y)); } Run.Add(FVector2D(X, Y + PH * 0.5f)); }
				else if (Run.Num() > 1) { B.Hedges.Add({ Region, Run }); Run.Reset(); }
				else { Run.Reset(); }
			}
			if (Run.Num() > 1) { B.Hedges.Add({ Region, Run }); }
		}
		for (int32 Row = 0; RowY(0.f, Row) < SizeM + 60.f && B.Hedges.Num() < 20000; ++Row)
		{
			TArray<FVector2D> Run;
			for (float X = 0.f; X <= SizeM; X += PW * 0.5f)
			{
				const FVector2D M(X + PW * 0.25f, RowY(X + PW * 0.25f, Row));
				const bool bKeep = M.Y > 0.f && M.Y < SizeM && Open(M + FVector2D(0.f, -20.f)) && Open(M + FVector2D(0.f, 20.f)) && !NearWay(M)
					&& BfHash(PlaceSeed + 43u, uint32(Row * 7919 + int32(X))) < Share;
				if (bKeep)
				{
					if (Run.Num() == 0) { Run.Add(FVector2D(X, RowY(X, Row))); }
					Run.Add(FVector2D(X + PW * 0.25f, M.Y));
					Run.Add(FVector2D(X + PW * 0.5f, RowY(X + PW * 0.5f, Row)));
				}
				else if (Run.Num() > 1) { B.Hedges.Add({ Region, Run }); Run.Reset(); }
				else { Run.Reset(); }
			}
			if (Run.Num() > 1) { B.Hedges.Add({ Region, Run }); }
		}
	}
	// ---- crossings: a lane or road over a river gets a small bridge (unless the campaign map has one there),
	// a farm track goes through a brook at a ford.
	{
		auto Cross = [&](const TArray<TArray<FVector2D>>& Ways, bool bTrack)
		{
			for (const TArray<FVector2D>& W : Ways)
			{
				for (int32 a = 0; a + 1 < W.Num(); ++a)
				{
					for (const FCampaign1851BattleRiver& Rv : B.Rivers)
					{
						for (int32 s = 0; s + 1 < Rv.M.Num(); ++s)
						{
							const FVector2D P = W[a], R = W[a + 1] - W[a], Q = Rv.M[s], S = Rv.M[s + 1] - Rv.M[s];
							const double Den = R.X * S.Y - R.Y * S.X;
							if (FMath::Abs(Den) < 1e-9) { continue; }
							const double T = ((Q.X - P.X) * S.Y - (Q.Y - P.Y) * S.X) / Den, U = ((Q.X - P.X) * R.Y - (Q.Y - P.Y) * R.X) / Den;
							if (T < 0.0 || T > 1.0 || U < 0.0 || U > 1.0) { continue; }
							const FVector2D X = P + R * T;
							if (bTrack && Rv.WidthM > 10.f) { continue; }   // no ford through a river
							const bool bTaken = B.BridgeM.ContainsByPredicate([&X](const FVector2D& M) { return FVector2D::Distance(M, X) < 150.f; })
								|| B.Crossings.ContainsByPredicate([&X](const FCampaign1851BattleCrossing& C) { return FVector2D::Distance(C.M, X) < 60.f; });
							if (!bTaken)
							{
								B.Crossings.Add({ bTrack ? TEXT("ford") : TEXT("bridge"), Rv.Name, X });
							}
						}
					}
				}
			}
		};
		Cross(B.Roads, false);
		Cross(B.Chaussees, false);
		Cross(B.Lanes, false);
		Cross(B.Tracks, true);
	}

	RenderBattlefield();
	WriteBattlefield();
	double NearestRoad = 1e9;
	for (const TArray<FVector2D>& L : RoadLines)
	{
		for (const FVector2D& P : L)
		{
			NearestRoad = FMath::Min(NearestRoad, FVector2D::Distance(P, CentreKm));
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|battlefield|%s|%.4f,%.4f|%.0f km|%d buildings|%d farms|%d roads|%d lanes|%d rail|%d forts|of %d main, %d lanes; nearest road point %.1f km"), *B.Place, B.Lat, B.Lon, B.SizeKm,
		B.Buildings.Num(), B.Farms, B.Roads.Num() + B.Chaussees.Num(), B.Lanes.Num(), B.Rails.Num(), B.Forts.Num(), RoadLines.Num(), LaneLinesKm.Num(), NearestRoad);
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|battlefield|water|%d rivers|%d lakes|%d hedges|%d crossings"), B.Rivers.Num(), B.Lakes.Num(), B.Hedges.Num(), B.Crossings.Num());
	return true;
}

void ACampaign1851Map::RenderBattlefield()
{
	FCampaign1851Battlefield& B = Battlefield;
	const float SizeM = B.SizeKm * 1000.f;
	const float Cell = SizeM / BfGrid;
	const float Px = SizeM / BfImage;   // metres a pixel
	const uint32 PlaceSeed = HashCombine(uint32(Seed), HashCombine(uint32(FMath::RoundToInt(B.CentreKm.X * 10.0)), uint32(FMath::RoundToInt(B.CentreKm.Y * 10.0))));
	TArray<FColor>& Img = B.Pixels;
	Img.SetNumUninitialized(BfImage * BfImage);
	auto HeightAt = [&](float X, float Y)
	{
		const float Gx = FMath::Clamp(X / Cell - 0.5f, 0.f, BfGrid - 1.001f), Gy = FMath::Clamp(Y / Cell - 0.5f, 0.f, BfGrid - 1.001f);
		const int32 I = int32(Gx), J = int32(Gy);
		const float Ax = Gx - I, Ay = Gy - J;
		auto H = [&](int32 x, int32 y) { return B.HeightM[FMath::Min(y, BfGrid - 1) * BfGrid + FMath::Min(x, BfGrid - 1)]; };
		return FMath::Lerp(FMath::Lerp(H(I, J), H(I + 1, J), Ax), FMath::Lerp(H(I, J + 1), H(I + 1, J + 1), Ax), Ay);
	};
	const bool bWinter = B.bSnow;
	const FColor FieldColours[] = { FColor(196, 178, 112), FColor(142, 162, 92), FColor(172, 150, 108), FColor(122, 152, 82), FColor(184, 170, 96) };
	for (int32 py = 0; py < BfImage; ++py)
	{
		for (int32 px = 0; px < BfImage; ++px)
		{
			// Image row 0 is north.
			const float X = (px + 0.5f) * Px, Y = SizeM - (py + 0.5f) * Px;
			const int32 k = FMath::Clamp(int32(Y / Cell), 0, BfGrid - 1) * BfGrid + FMath::Clamp(int32(X / Cell), 0, BfGrid - 1);
			const EBattlefieldCell K = EBattlefieldCell(B.Kind[k]);
			const float H = HeightAt(X, Y);
			FColor C;
			if (K == EBattlefieldCell::Sea)
			{
				C = FColor(62, 96, 124);
			}
			else if (K == EBattlefieldCell::Water)
			{
				C = FColor(74, 112, 140);
			}
			else
			{
				// Fields in parcels (hedged plots of the enclosures), woods, meadows, the town.
				const uint32 Parcel = uint32(FMath::FloorToInt(X / 140.f)) * 92821u ^ uint32(FMath::FloorToInt((Y + 37.f * FMath::Sin(X / 400.f)) / 110.f)) * 68917u;
				C = FieldColours[int32(BfHash(PlaceSeed, Parcel) * 4.99f)];
				if (K == EBattlefieldCell::Meadow) { C = FColor(118, 146, 104); }
				if (K == EBattlefieldCell::Wood) { C = BfMix(FColor(46, 76, 42), FColor(64, 96, 52), BfFractal(PlaceSeed + 5u, X, Y, 40.0)); }
				if (K == EBattlefieldCell::Town) { C = FColor(158, 130, 104); }
				if (bWinter && K != EBattlefieldCell::Wood) { C = BfMix(C, FColor(236, 238, 242), 0.8f); }
				// Hillshade from the north-west and contours every 5 m.
				const float Dx = HeightAt(X + Px, Y) - HeightAt(X - Px, Y);
				const float Dy = HeightAt(X, Y + Px) - HeightAt(X, Y - Px);
				const float Shade = FMath::Clamp(1.f + (-Dx + Dy) / (2.f * Px) * 6.f, 0.7f, 1.25f);
				C = BfScale(C, Shade);
				if (FMath::FloorToInt(H / 5.f) != FMath::FloorToInt(HeightAt(X + Px, Y) / 5.f) || FMath::FloorToInt(H / 5.f) != FMath::FloorToInt(HeightAt(X, Y - Px) / 5.f))
				{
					C = BfScale(C, 0.8f);
				}
			}
			Img[py * BfImage + px] = C;
		}
	}
	// Lines and shapes in metres.
	auto Plot = [&](const FVector2D& M, const FColor& C)
	{
		const int32 px = FMath::FloorToInt(M.X / Px), py = FMath::FloorToInt((SizeM - M.Y) / Px);
		if (px >= 0 && py >= 0 && px < BfImage && py < BfImage)
		{
			Img[py * BfImage + px] = C;
		}
	};
	auto Line = [&](const TArray<FVector2D>& Pts, float WidthM, const FColor& C)
	{
		for (int32 i = 0; i + 1 < Pts.Num(); ++i)
		{
			const FVector2D A = Pts[i], D = Pts[i + 1] - Pts[i];
			const float Len = D.Size();
			const FVector2D N = Len > 0.f ? FVector2D(-D.Y, D.X) / Len : FVector2D::ZeroVector;
			for (float t = 0.f; t <= Len; t += Px * 0.5f)
			{
				for (float w = -WidthM * 0.5f; w <= WidthM * 0.5f; w += Px * 0.5f)
				{
					Plot(A + D * (t / FMath::Max(Len, 1.f)) + N * w, C);
				}
			}
		}
	};
	// Widths at least a pixel or two, so the ways read at every size; a dark edge under each.
	const auto W = [Px](float Metres, float MinPx) { return FMath::Max(Metres, MinPx * Px); };
	// Field boundaries under the ways, a thin line blended into the fields (each pixel once): knicks dark
	// green, dikes grey, ditches blue-grey.
	{
		TArray<uint8> Mask;
		Mask.SetNumZeroed(BfImage * BfImage);
		for (const FCampaign1851BattleHedge& H : B.Hedges)
		{
			for (int32 i = 0; i + 1 < H.M.Num(); ++i)
			{
				const FVector2D D = H.M[i + 1] - H.M[i];
				const float Len = D.Size();
				for (float t = 0.f; t <= Len; t += Px * 0.5f)
				{
					const FVector2D P = H.M[i] + D * (t / FMath::Max(Len, 1.f));
					const int32 px = FMath::FloorToInt(P.X / Px), py = FMath::FloorToInt((SizeM - P.Y) / Px);
					if (px >= 0 && py >= 0 && px < BfImage && py < BfImage)
					{
						Mask[py * BfImage + px] = uint8(H.Kind) + 1;
					}
				}
			}
		}
		const FColor HedgeInk[] = { FColor(40, 66, 30), FColor(160, 156, 146), FColor(70, 104, 128) };
		const float HedgeAlpha[] = { 0.5f, 0.55f, 0.6f };
		for (int32 p = 0; p < Mask.Num(); ++p)
		{
			if (Mask[p])
			{
				Img[p] = BfMix(Img[p], HedgeInk[Mask[p] - 1], HedgeAlpha[Mask[p] - 1]);
			}
		}
	}
	// Rivers: a dark bank and the water, at least a pixel or two wide.
	for (const FCampaign1851BattleRiver& Rv : B.Rivers)
	{
		Line(Rv.M, W(Rv.WidthM + 6.f, 2.4f), FColor(84, 96, 70));
		Line(Rv.M, W(Rv.WidthM, 1.5f), FColor(74, 112, 140));
	}
	for (const TArray<FVector2D>& L : B.Tracks) { Line(L, W(3.f, 1.f), FColor(150, 128, 92)); }
	for (const TArray<FVector2D>& L : B.Lanes) { Line(L, W(6.f, 2.5f), FColor(110, 90, 66)); Line(L, W(6.f, 1.5f), FColor(222, 202, 156)); }
	for (const TArray<FVector2D>& L : B.Roads) { Line(L, W(10.f, 3.5f), FColor(96, 78, 58)); Line(L, W(10.f, 2.2f), FColor(236, 220, 176)); }
	for (const TArray<FVector2D>& L : B.Chaussees) { Line(L, W(18.f, 4.5f), FColor(80, 70, 60)); Line(L, W(12.f, 3.f), FColor(246, 238, 210)); }
	for (const TArray<FVector2D>& L : B.Rails) { Line(L, W(12.f, 2.5f), FColor(40, 34, 30)); }
	// Bridges: a dark deck over the water; a blown one leaves the water showing.
	for (int32 b = 0; b < B.Bridges.Num(); ++b)
	{
		const FVector2D M = B.BridgeM[b];
		const float R = FMath::Max(B.Bridges[b].LengthM * 0.5f, 3.f * Px);
		for (float y = -R; y <= R; y += Px * 0.5f)
		{
			for (float x = -2.f * Px; x <= 2.f * Px; x += Px * 0.5f)
			{
				const int32 px = FMath::FloorToInt((M.X + x) / Px), py = FMath::FloorToInt((SizeM - M.Y - y) / Px);
				if (px >= 0 && py >= 0 && px < BfImage && py < BfImage && B.Bridges[b].State == EBridgeState::Blown)
				{
					Img[py * BfImage + px] = FColor(62, 96, 124);
				}
			}
		}
	}
	// Crossings: a light deck over the river, a ford as a pale gravel patch.
	for (const FCampaign1851BattleCrossing& Cx : B.Crossings)
	{
		const bool bFord = Cx.Kind == TEXT("ford");
		for (float y = -2.f * Px; y <= 2.f * Px; y += Px * 0.5f)
		{
			for (float x = -2.f * Px; x <= 2.f * Px; x += Px * 0.5f)
			{
				Plot(Cx.M + FVector2D(x, y), bFord ? FColor(196, 186, 150) : FColor(120, 96, 70));
			}
		}
	}
	auto Rect = [&](const FCampaign1851BattleBuilding& Bd, const FColor& C)
	{
		const float BYaw = FMath::DegreesToRadians(Bd.Yaw);
		const FVector2D Ax(FMath::Cos(BYaw), FMath::Sin(BYaw)), Ay(-Ax.Y, Ax.X);
		for (float a = -Bd.Size.X * 0.5f; a <= Bd.Size.X * 0.5f; a += Px * 0.5f)
		{
			for (float b = -Bd.Size.Y * 0.5f; b <= Bd.Size.Y * 0.5f; b += Px * 0.5f)
			{
				Plot(Bd.M + Ax * a + Ay * b, C);
			}
		}
	};
	for (const FCampaign1851BattleBuilding& Bd : B.Buildings)
	{
		FCampaign1851BattleBuilding Shown = Bd;
		Shown.Size = FVector2D(FMath::Max(Bd.Size.X, Px * 1.2), FMath::Max(Bd.Size.Y, Px * 1.2));
		Rect(Shown, Bd.Kind == TEXT("farm") ? FColor(150, 72, 52) : Bd.Kind == TEXT("house") ? FColor(176, 60, 44) : Bd.Kind == TEXT("church") ? FColor(214, 210, 200) : FColor(90, 60, 50));
	}
	// Forts: the earthwork as a pointed redoubt facing its front, the ditch dark; trenches between them.
	for (int32 f = 0; f < B.Forts.Num(); ++f)
	{
		const FCampaign1851Fort& F = B.Forts[f];
		const FVector2D M = B.FortM[f];
		const float R = F.bLarge ? 70.f : 45.f;
		const float FYaw = FMath::DegreesToRadians(F.Yaw + 90.f);
		const FVector2D Front(FMath::Sin(FYaw), FMath::Cos(FYaw));
		const FVector2D Side(-Front.Y, Front.X);
		TArray<FVector2D> Outline = { M - Front * R * 0.6f - Side * R, M + Front * R * 0.1f - Side * R, M + Front * R, M + Front * R * 0.1f + Side * R, M - Front * R * 0.6f + Side * R, M - Front * R * 0.6f - Side * R };
		Line(Outline, 14.f, FColor(64, 54, 40));
		Line(Outline, 8.f, FColor(150, 128, 84));
	}
	for (int32 f = 0; f < B.Forts.Num(); ++f)
	{
		for (int32 Other : TrenchLinks(B.Forts[f].Id))
		{
			const int32 o = B.Forts.IndexOfByPredicate([Other](const FCampaign1851Fort& X) { return X.Id == Other; });
			if (o > f)
			{
				Line({ B.FortM[f], B.FortM[o] }, 5.f, FColor(84, 68, 48));
			}
		}
	}
	// The texture for the preview.
	if (!BattlefieldTexture)
	{
		BattlefieldTexture = UTexture2D::CreateTransient(BfImage, BfImage, PF_B8G8R8A8);
		BattlefieldTexture->SRGB = true;
		BattlefieldTexture->Filter = TF_Bilinear;
	}
	if (BattlefieldTexture && BattlefieldTexture->GetPlatformData())
	{
		void* Data = BattlefieldTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, Img.GetData(), Img.Num() * sizeof(FColor));
		BattlefieldTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
		BattlefieldTexture->UpdateResource();
	}
	++BattlefieldVersion;
}

void ACampaign1851Map::WriteBattlefield() const
{
	const FCampaign1851Battlefield& B = Battlefield;
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Battle");
	const FString Base = FString::Printf(TEXT("Battlefield_%s"), *B.Name);
	// The picture.
	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Png = Module.CreateImageWrapper(EImageFormat::PNG);
	if (Png.IsValid() && Png->SetRaw(B.Pixels.GetData(), B.Pixels.Num() * sizeof(FColor), BfImage, BfImage, ERGBFormat::BGRA, 8))
	{
		FFileHelper::SaveArrayToFile(Png->GetCompressed(), *(Dir / Base + TEXT(".png")));
	}
	// The data: origin at the south-west corner, x east, y north, metres; the grid row 0 at the south.
	TSharedRef<FJsonObject> Doc = MakeShared<FJsonObject>();
	Doc->SetStringField(TEXT("format"), TEXT("PROJECT1864-Battlefield-1"));
	Doc->SetStringField(TEXT("place"), B.Place);
	Doc->SetNumberField(TEXT("lat"), B.Lat);
	Doc->SetNumberField(TEXT("lon"), B.Lon);
	Doc->SetNumberField(TEXT("sizeM"), B.SizeKm * 1000.f);
	Doc->SetNumberField(TEXT("grid"), BfGrid);
	Doc->SetNumberField(TEXT("cellM"), B.SizeKm * 1000.f / BfGrid);
	Doc->SetStringField(TEXT("date"), (StartDate() + FTimespan::FromDays(B.Day)).ToIso8601());
	Doc->SetStringField(TEXT("season"), GetSeasonName());
	Doc->SetStringField(TEXT("weather"), Campaign1851Weather::Name(GetWeather()));
	Doc->SetNumberField(TEXT("temperatureC"), FMath::RoundToInt(GetTemperature()));
	Doc->SetBoolField(TEXT("snow"), B.bSnow);
	Doc->SetStringField(TEXT("image"), Base + TEXT(".png"));
	TArray<TSharedPtr<FJsonValue>> Heights;
	Heights.Reserve(B.HeightM.Num());
	for (float H : B.HeightM)
	{
		Heights.Add(MakeShared<FJsonValueNumber>(FMath::RoundToInt(H * 10.f)));
	}
	Doc->SetArrayField(TEXT("heightDm"), Heights);
	// Kinds and woods as strings, one character a cell: . field, ~ sea, m meadow, w wood, t town; 0-9 wood density.
	FString Kinds, Woods;
	for (int32 k = 0; k < B.Kind.Num(); ++k)
	{
		static const TCHAR Chars[] = TEXT(".~mwto");
		Kinds.AppendChar(Chars[FMath::Clamp(int32(B.Kind[k]), 0, 5)]);
		Woods.AppendChar(TCHAR('0' + FMath::Min(9, B.Wood[k] / 10)));
	}
	Doc->SetStringField(TEXT("kinds"), Kinds);
	Doc->SetStringField(TEXT("woodDensity"), Woods);
	auto Lines = [&](const TArray<TArray<FVector2D>>& Src)
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (const TArray<FVector2D>& L : Src)
		{
			TArray<TSharedPtr<FJsonValue>> Pts;
			for (const FVector2D& P : L)
			{
				Pts.Add(MakeShared<FJsonValueNumber>(FMath::RoundToInt(P.X)));
				Pts.Add(MakeShared<FJsonValueNumber>(FMath::RoundToInt(P.Y)));
			}
			Out.Add(MakeShared<FJsonValueArray>(Pts));
		}
		return Out;
	};
	Doc->SetArrayField(TEXT("roads"), Lines(B.Roads));
	Doc->SetArrayField(TEXT("lanes"), Lines(B.Lanes));
	Doc->SetArrayField(TEXT("tracks"), Lines(B.Tracks));
	Doc->SetArrayField(TEXT("chaussees"), Lines(B.Chaussees));
	Doc->SetArrayField(TEXT("railways"), Lines(B.Rails));
	TArray<TSharedPtr<FJsonValue>> Buildings;
	for (const FCampaign1851BattleBuilding& Bd : B.Buildings)
	{
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("kind"), Bd.Kind);
		O->SetNumberField(TEXT("x"), FMath::RoundToInt(Bd.M.X));
		O->SetNumberField(TEXT("y"), FMath::RoundToInt(Bd.M.Y));
		O->SetNumberField(TEXT("w"), FMath::RoundToInt(Bd.Size.X));
		O->SetNumberField(TEXT("d"), FMath::RoundToInt(Bd.Size.Y));
		O->SetNumberField(TEXT("yaw"), FMath::RoundToInt(Bd.Yaw));
		Buildings.Add(MakeShared<FJsonValueObject>(O));
	}
	Doc->SetArrayField(TEXT("buildings"), Buildings);
	TArray<TSharedPtr<FJsonValue>> Towns;
	for (const FCampaign1851BattleTown& T : B.Towns)
	{
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("name"), T.Name);
		O->SetNumberField(TEXT("x"), FMath::RoundToInt(T.M.X));
		O->SetNumberField(TEXT("y"), FMath::RoundToInt(T.M.Y));
		O->SetNumberField(TEXT("radius"), FMath::RoundToInt(T.RadiusM));
		Towns.Add(MakeShared<FJsonValueObject>(O));
	}
	Doc->SetArrayField(TEXT("towns"), Towns);
	TArray<TSharedPtr<FJsonValue>> FortList;
	for (int32 f = 0; f < B.Forts.Num(); ++f)
	{
		const FCampaign1851Fort& F = B.Forts[f];
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetNumberField(TEXT("id"), F.Id);
		O->SetStringField(TEXT("name"), F.Name);
		O->SetNumberField(TEXT("x"), FMath::RoundToInt(B.FortM[f].X));
		O->SetNumberField(TEXT("y"), FMath::RoundToInt(B.FortM[f].Y));
		O->SetNumberField(TEXT("frontYaw"), FMath::RoundToInt(F.Yaw));
		O->SetBoolField(TEXT("large"), F.bLarge);
		O->SetNumberField(TEXT("guns"), F.Guns);
		O->SetNumberField(TEXT("defence"), F.Defence);
		O->SetBoolField(TEXT("built"), F.bBuilt);
		TArray<TSharedPtr<FJsonValue>> TrenchTo;
		for (int32 Other : TrenchLinks(F.Id))
		{
			TrenchTo.Add(MakeShared<FJsonValueNumber>(Other));
		}
		O->SetArrayField(TEXT("trenchesTo"), TrenchTo);
		FortList.Add(MakeShared<FJsonValueObject>(O));
	}
	Doc->SetArrayField(TEXT("forts"), FortList);
	TArray<TSharedPtr<FJsonValue>> BridgeList;
	for (int32 b = 0; b < B.Bridges.Num(); ++b)
	{
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("name"), B.Bridges[b].Name);
		O->SetNumberField(TEXT("x"), FMath::RoundToInt(B.BridgeM[b].X));
		O->SetNumberField(TEXT("y"), FMath::RoundToInt(B.BridgeM[b].Y));
		O->SetNumberField(TEXT("lengthM"), FMath::RoundToInt(B.Bridges[b].LengthM));
		O->SetStringField(TEXT("state"), B.Bridges[b].State == EBridgeState::Blown ? TEXT("blown") : B.Bridges[b].State == EBridgeState::Building ? TEXT("building") : TEXT("intact"));
		BridgeList.Add(MakeShared<FJsonValueObject>(O));
	}
	Doc->SetArrayField(TEXT("bridges"), BridgeList);
	TArray<TSharedPtr<FJsonValue>> RiverList;
	for (const FCampaign1851BattleRiver& Rv : B.Rivers)
	{
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("name"), Rv.Name);
		O->SetNumberField(TEXT("widthM"), FMath::RoundToInt(Rv.WidthM));
		O->SetArrayField(TEXT("points"), Lines({ Rv.M })[0]->AsArray());
		RiverList.Add(MakeShared<FJsonValueObject>(O));
	}
	Doc->SetArrayField(TEXT("rivers"), RiverList);
	Doc->SetArrayField(TEXT("lakes"), Lines(B.Lakes));
	TArray<TSharedPtr<FJsonValue>> HedgeList;
	for (const FCampaign1851BattleHedge& H : B.Hedges)
	{
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("kind"), H.Kind == EHedgeKind::Knick ? TEXT("knick") : H.Kind == EHedgeKind::Dike ? TEXT("dike") : TEXT("ditch"));
		O->SetArrayField(TEXT("points"), Lines({ H.M })[0]->AsArray());
		HedgeList.Add(MakeShared<FJsonValueObject>(O));
	}
	Doc->SetArrayField(TEXT("hedges"), HedgeList);
	TArray<TSharedPtr<FJsonValue>> CrossList;
	for (const FCampaign1851BattleCrossing& Cx : B.Crossings)
	{
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("kind"), Cx.Kind);
		O->SetStringField(TEXT("river"), Cx.River);
		O->SetNumberField(TEXT("x"), FMath::RoundToInt(Cx.M.X));
		O->SetNumberField(TEXT("y"), FMath::RoundToInt(Cx.M.Y));
		CrossList.Add(MakeShared<FJsonValueObject>(O));
	}
	Doc->SetArrayField(TEXT("crossings"), CrossList);
	FString Text;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
	FJsonSerializer::Serialize(Doc, Writer);
	FFileHelper::SaveStringToFile(Text, *(Dir / Base + TEXT(".json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

FString ACampaign1851Map::BattlefieldFile() const
{
	return FString::Printf(TEXT("Battlefield_%s.json"), *Battlefield.Name);
}
