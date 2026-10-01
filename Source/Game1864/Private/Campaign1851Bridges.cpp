// Bridges (Docs/Bridges1851.md). Where a road crosses water on the way between two towns there is a bridge;
// where a short ferry crosses a sound (Alssund, the narrow waters) a pontoon bridge can be laid. A bridge can
// be blown: the road is cut for both sides (the march planner goes round or not at all) until it is
// rebuilt. A bridge over a sound lets the enemy cross without boats, whatever the fleet does, unless blown.

#include "Campaign1851Map.h"
#include "Campaign1851Scenery.h"
#include "Components/StaticMeshComponent.h"

namespace Campaign1851Bridge
{
	constexpr double BlowCost = 500.0;
	constexpr double RebuildCost = 4000.0;
	constexpr float RebuildDays = 20.f;
	constexpr float MaxFerryKm = 1.2f;   // a sound narrow enough for a pontoon bridge
}

void ACampaign1851Map::DetectBridges()
{
	Bridges.Reset();
	int32 Next = 1;
	// The ferries as the map has them (a pontoon bridge laid in an earlier game changed them).
	if (LinkFerryKm0.Num() != Links.Num())
	{
		LinkFerryKm0.Reset();
		for (const FCampaign1851Link& L : Links) { LinkFerryKm0.Add(L.FerryKm); }
	}
	for (int32 l = 0; l < Links.Num(); ++l)
	{
		Links[l].FerryKm = LinkFerryKm0[l];
	}
	// One bridge for all the roads over the same water: a new find near an old one joins it.
	auto AddBridge = [&](FCampaign1851Bridge B, bool bOwnName = false)
	{
		// A named town bridge (Knippelsbro, Langebro) stays a bridge of its own even close to another.
		FCampaign1851Bridge* Same = bOwnName ? nullptr : Bridges.FindByPredicate([&B](const FCampaign1851Bridge& X) { return FVector2D::Distance(X.Km, B.Km) < 0.5 && (X.FerryKm > 0.f) == (B.FerryKm > 0.f); });
		if (Same)
		{
			if (B.Link != INDEX_NONE) { Same->Links.AddUnique(B.Link); }
			return;
		}
		B.Id = Next++;
		if (B.Link != INDEX_NONE) { B.Links.Add(B.Link); }
		Bridges.Add(B);
	};
	for (int32 l = 0; l < Links.Num(); ++l)
	{
		FCampaign1851Link& L = Links[l];
		L.bBlocked = false;
		const FString Near = Cities.IsValidIndex(L.A) && Cities.IsValidIndex(L.B) ? FString::Printf(TEXT("%s–%s"), *Cities[L.A].Name, *Cities[L.B].Name) : FString();
		if (L.HasFerry())
		{
			// A short ferry: a site for a pontoon bridge, where the map's ferry crosses (the ferry line nearest the road).
			if (L.FerryKm > Campaign1851Bridge::MaxFerryKm || L.Km.Num() < 2)
			{
				continue;
			}
			const TArray<FVector2D>* Best = nullptr;
			double BestD = 3.0;
			for (const TArray<FVector2D>& F : FerryLines)
			{
				if (F.Num() < 2)
				{
					continue;
				}
				const FVector2D Mid = (F[0] + F.Last()) * 0.5;
				for (const FVector2D& P : L.Km)
				{
					const double D = FVector2D::Distance(P, Mid);
					if (D < BestD) { BestD = D; Best = &F; }
				}
			}
			FCampaign1851Bridge B;
			B.Link = l;
			B.Name = L.Ferry.IsEmpty() ? Near : L.Ferry;
			B.State = EBridgeState::Site;
			B.FerryKm = L.FerryKm;
			if (Best)
			{
				B.EndA = (*Best)[0];
				B.EndB = Best->Last();
			}
			else
			{
				const double Total = LineLength(L.Km);
				B.EndA = AlongLine(L.Km, Total * 0.5 - L.FerryKm * 0.5);
				B.EndB = AlongLine(L.Km, Total * 0.5 + L.FerryKm * 0.5);
			}
			B.Km = (B.EndA + B.EndB) * 0.5;
			B.LengthM = float(FVector2D::Distance(B.EndA, B.EndB) * 1000.0);
			AddBridge(B);
			continue;
		}
		// Over land with water on the way: every stretch of 20 - 900 m of water is a bridge.
		const float Len = float(LineLength(L.Km));
		float WetFrom = -1.f;
		for (float d = 0.f; d <= Len; d += 0.04f)
		{
			const bool bWet = IsSea(AlongLine(L.Km, d));
			if (bWet && WetFrom < 0.f)
			{
				WetFrom = d;
			}
			else if (!bWet && WetFrom >= 0.f)
			{
				const float Span = d - WetFrom;
				if (Span >= 0.02f && Span <= 0.9f)
				{
					FCampaign1851Bridge B;
					B.Link = l;
					const int32 Town = NearestTown(AlongLine(L.Km, (WetFrom + d) * 0.5f));
					B.Name = FString::Printf(TEXT("Broen ved %s"), Cities.IsValidIndex(Town) ? *Cities[Town].Name : *Near);
					B.Km = AlongLine(L.Km, (WetFrom + d) * 0.5f);
					B.EndA = AlongLine(L.Km, FMath::Max(0.f, WetFrom - 0.03f));
					B.EndB = AlongLine(L.Km, FMath::Min(Len, d + 0.03f));
					B.LengthM = Span * 1000.f;
					B.State = EBridgeState::Intact;
					AddBridge(B);
				}
				WetFrom = -1.f;
			}
		}
	}
	// The town bridges the map's roads do not cross (positions approximate).
	struct FTownBridge { const TCHAR* Name; double LatA, LonA, LatB, LonB; };
	const FTownBridge TownBridges[] = {
		{ TEXT("Knippelsbro"), 55.6753, 12.5846, 55.6735, 12.5893 },
		{ TEXT("Langebro"),    55.6701, 12.5784, 55.6680, 12.5824 },
	};
	for (const FTownBridge& T : TownBridges)
	{
		FCampaign1851Bridge B;
		B.Name = T.Name;
		// The map's coast is simplified: along the bridge's line, the stretch of water nearest its middle.
		const FVector2D A = Extent.Projection.Forward(T.LatA, T.LonA), C = Extent.Projection.Forward(T.LatB, T.LonB);
		const FVector2D Mid = (A + C) * 0.5, Dir = (C - A).GetSafeNormal();
		double BestFrom = 0.0, BestTo = 0.0, BestGap = 1e9, From = -1e9;
		for (double t = -1.5; t <= 1.5; t += 0.02)
		{
			const bool bWet = IsSea(Mid + Dir * t);
			if (bWet && From < -1e8) { From = t; }
			if (!bWet && From > -1e8)
			{
				const double Gap = FMath::Abs((From + t) * 0.5);
				if (Gap < BestGap && t - From < 1.2) { BestGap = Gap; BestFrom = From; BestTo = t; }
				From = -1e9;
			}
		}
		if (BestGap > 1e8)
		{
			continue;   // no water there on the map
		}
		B.EndA = Mid + Dir * (BestFrom - 0.03);
		B.EndB = Mid + Dir * (BestTo + 0.03);
		B.Km = (B.EndA + B.EndB) * 0.5;
		B.LengthM = float(FVector2D::Distance(B.EndA, B.EndB) * 1000.0);
		B.State = EBridgeState::Intact;
		AddBridge(B, true);
	}
	// Over the rivers and the canal (Campaign1851Hydro.cpp): where a road between two towns crosses one.
	// After the others, so the bridges of older saves keep their numbers. The Elbe had no bridge in 1851.
	auto Hit = [](const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D, FVector2D& Out)
	{
		const FVector2D R = B - A, S = D - C;
		const double Den = R.X * S.Y - R.Y * S.X;
		if (FMath::Abs(Den) < 1e-12)
		{
			return false;
		}
		const double T = ((C.X - A.X) * S.Y - (C.Y - A.Y) * S.X) / Den, U = ((C.X - A.X) * R.Y - (C.Y - A.Y) * R.X) / Den;
		if (T < 0.0 || T > 1.0 || U < 0.0 || U > 1.0)
		{
			return false;
		}
		Out = A + R * T;
		return true;
	};
	int32 RiverBridges = 0;
	for (int32 r = 0; r < Rivers.Num(); ++r)
	{
		const FCampaign1851River& Rv = Rivers[r];
		if (Rv.Class >= 3)
		{
			continue;
		}
		FBox2D RBox(ForceInit);
		for (const FVector2D& P : Rv.Km) { RBox += P; }
		for (int32 l = 0; l < Links.Num(); ++l)
		{
			const FCampaign1851Link& L = Links[l];
			if (L.HasFerry() || L.Km.Num() < 2)
			{
				continue;
			}
			for (int32 i = 0; i + 1 < L.Km.Num(); ++i)
			{
				const FVector2D A = L.Km[i], C = L.Km[i + 1];
				if (FMath::Max(A.X, C.X) < RBox.Min.X || FMath::Min(A.X, C.X) > RBox.Max.X || FMath::Max(A.Y, C.Y) < RBox.Min.Y || FMath::Min(A.Y, C.Y) > RBox.Max.Y)
				{
					continue;
				}
				for (int32 s = 0; s + 1 < Rv.Km.Num(); ++s)
				{
					FVector2D X;
					if (!Hit(A, C, Rv.Km[s], Rv.Km[s + 1], X))
					{
						continue;
					}
					const FVector2D Dir = (C - A).GetSafeNormal();
					const double Half = FMath::Max(double(RiverDrawnKm(Rv.Class)) * 0.5 + 0.04, 0.06);
					FCampaign1851Bridge B;
					B.Link = l;
					const int32 Town = NearestTown(X);
					B.Name = FString::Printf(TEXT("Broen over %s ved %s"), *Rv.Name, Cities.IsValidIndex(Town) ? *Cities[Town].Name : TEXT("?"));
					B.Km = X;
					B.EndA = X - Dir * Half;
					B.EndB = X + Dir * Half;
					B.LengthM = Rv.WidthM + 10.f;
					B.State = EBridgeState::Intact;
					const int32 Before = Bridges.Num();
					AddBridge(B);
					RiverBridges += Bridges.Num() - Before;
				}
			}
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|bridges|over rivers=%d"), RiverBridges);
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|bridges|%d (%d pontoon sites)"), Bridges.Num(), Bridges.FilterByPredicate([](const FCampaign1851Bridge& B) { return B.State == EBridgeState::Site; }).Num());
	for (const FCampaign1851Bridge& B : Bridges)
	{
		const FVector2D LatLon = Extent.Projection.Inverse(B.Km);
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|bridge|%d|%s|%.0f m|%s|%.4f,%.4f"), B.Id, *B.Name, B.LengthM, B.State == EBridgeState::Site ? TEXT("pontonsted") : TEXT("bro"), LatLon.X, LatLon.Y);
	}
	RebuildBridgeMeshes();
}

int32 ACampaign1851Map::BridgeIndex(int32 Id) const
{
	return Bridges.IndexOfByPredicate([Id](const FCampaign1851Bridge& B) { return B.Id == Id; });
}

void ACampaign1851Map::ApplyBridge(const FCampaign1851Bridge& B)
{
	// Blown or being rebuilt: the roads over it are cut. A pontoon bridge standing: no ferry any more.
	for (int32 l : B.Links)
	{
		if (!Links.IsValidIndex(l))
		{
			continue;
		}
		FCampaign1851Link& L = Links[l];
		L.bBlocked = B.State == EBridgeState::Blown || (B.State == EBridgeState::Building && B.FerryKm <= 0.f);
		if (B.FerryKm > 0.f)
		{
			L.FerryKm = B.State == EBridgeState::Intact ? 0.f : LinkFerryKm0.IsValidIndex(l) ? LinkFerryKm0[l] : B.FerryKm;
			L.bBlocked = B.State == EBridgeState::Blown;
		}
	}
}

FString ACampaign1851Map::BridgeBlockReason(int32 Id, EBridgeAction Action) const
{
	const int32 i = BridgeIndex(Id);
	if (i == INDEX_NONE)
	{
		return TEXT("-");
	}
	const FCampaign1851Bridge& B = Bridges[i];
	switch (Action)
	{
	case EBridgeAction::Blow:
		if (B.State != EBridgeState::Intact) return TEXT("ingen bro at sprænge");
		if (Treasury < Campaign1851Bridge::BlowCost) return TEXT("ikke råd");
		return FString();
	case EBridgeAction::Rebuild:
		if (B.State != EBridgeState::Blown) return TEXT("broen står");
		if (Treasury < Campaign1851Bridge::RebuildCost) return TEXT("ikke råd");
		return FString();
	case EBridgeAction::Build:
		if (B.State != EBridgeState::Site) return TEXT("-");
		if (Treasury < PontoonCost()) return TEXT("ikke råd");
		return FString();
	}
	return TEXT("-");
}

bool ACampaign1851Map::BridgeAction(int32 Id, EBridgeAction Action, FString* OutReason)
{
	const FString Why = BridgeBlockReason(Id, Action);
	if (!Why.IsEmpty())
	{
		if (OutReason) { *OutReason = Why; }
		return false;
	}
	FCampaign1851Bridge& B = Bridges[BridgeIndex(Id)];
	switch (Action)
	{
	case EBridgeAction::Blow:
		AddTransaction(-Campaign1851Bridge::BlowCost, FString::Printf(TEXT("Sprængning: %s"), *B.Name));
		B.State = EBridgeState::Blown;
		News.Add(FString::Printf(TEXT("%s er sprængt: vejen er afskåret"), *B.Name));
		break;
	case EBridgeAction::Rebuild:
		AddTransaction(-Campaign1851Bridge::RebuildCost, FString::Printf(TEXT("Genopbygning: %s"), *B.Name));
		B.State = EBridgeState::Building;
		B.DaysLeft = Campaign1851Bridge::RebuildDays;
		break;
	case EBridgeAction::Build:
		AddTransaction(-PontoonCost(), FString::Printf(TEXT("Pontonbro: %s"), *B.Name));
		B.State = EBridgeState::Building;
		B.DaysLeft = PontoonDays();
		break;
	}
	ApplyBridge(B);
	RebuildBridgeMeshes();
	return true;
}

void ACampaign1851Map::RebuildBridgeMeshes()
{
	for (TObjectPtr<UStaticMeshComponent>& M : BridgeMeshes)
	{
		if (M) { M->DestroyComponent(); }
	}
	BridgeMeshes.Reset();
	if (GridZ.Num() == 0)
	{
		return;
	}
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery"));
	if (!Material)
	{
		return;
	}
	TArray<TArray<FVector>> Decks, Rails, Pontoons;
	for (const FCampaign1851Bridge& B : Bridges)
	{
		if (B.State == EBridgeState::Site)
		{
			continue;
		}
		// A flat deck a little above the higher bank; railings along both sides.
		const FVector A = LocalAtKm(B.EndA), C = LocalAtKm(B.EndB);
		const double Z = FMath::Max(A.Z, C.Z) + 2.5;
		const FVector2D Dir = (B.EndB - B.EndA).GetSafeNormal();
		const FVector2D Side(-Dir.Y, Dir.X);
		auto Span = [&](float T0, float T1, const FVector2D& Offset, double Lift)
		{
			TArray<FVector> Pts;
			for (int32 s = 0; s <= 8; ++s)
			{
				const float T = FMath::Lerp(T0, T1, s / 8.f);
				FVector P = LocalAtKm(FMath::Lerp(B.EndA, B.EndB, double(T)) + Offset);
				P.Z = Z + Lift;
				Pts.Add(P);
			}
			return Pts;
		};
		const bool bPontoon = B.FerryKm > 0.f;
		// Blown: the two ends stand, the middle is gone; being built: the ends grow towards each other.
		TArray<FVector2f> Parts;
		if (B.State == EBridgeState::Blown) { Parts = { FVector2f(0.f, 0.35f), FVector2f(0.65f, 1.f) }; }
		else if (B.State == EBridgeState::Building) { Parts = { FVector2f(0.f, 0.25f), FVector2f(0.75f, 1.f) }; }
		else { Parts = { FVector2f(0.f, 1.f) }; }
		for (const FVector2f& P : Parts)
		{
			(bPontoon ? Pontoons : Decks).Add(Span(P.X, P.Y, FVector2D::ZeroVector, 0.0));
			Rails.Add(Span(P.X, P.Y, Side * 0.095, 0.8));
			Rails.Add(Span(P.X, P.Y, -Side * 0.095, 0.8));
		}
	}
	auto Make = [&](const TArray<TArray<FVector>>& Lines, float WidthKm, const FLinearColor& Colour, const TCHAR* Name)
	{
		if (Lines.Num() == 0)
		{
			return;
		}
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Name));
		Mesh->SetupAttachment(Root);
		Mesh->SetStaticMesh(Campaign1851Scenery::BuildRibbons(Lines, WidthKm * 0.5f * float(KmToUnits), Colour, Material, *FString::Printf(TEXT("SM_Campaign1851_%s"), Name)));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->RegisterComponent();
		BridgeMeshes.Add(Mesh);
	};
	Make(Decks, 0.2f, FLinearColor::FromSRGBColor(FColor(132, 116, 96)), TEXT("BridgeDecks"));
	Make(Pontoons, 0.16f, FLinearColor::FromSRGBColor(FColor(96, 70, 48)), TEXT("BridgePontoons"));
	Make(Rails, 0.025f, FLinearColor::FromSRGBColor(FColor(52, 42, 34)), TEXT("BridgeRails"));
}

void ACampaign1851Map::DailyBridges()
{
	for (FCampaign1851Bridge& B : Bridges)
	{
		if (B.State == EBridgeState::Building && (B.DaysLeft -= GetWeather() == ECampaign1851Weather::Frost ? 0.5f : 1.f) <= 0.f)
		{
			B.State = EBridgeState::Intact;
			News.Add(FString::Printf(TEXT("%s står færdig"), *B.Name));
			ApplyBridge(B);
			RebuildBridgeMeshes();
		}
	}
}

TArray<FString> ACampaign1851Map::SaveBridges() const
{
	TArray<FString> Out;
	for (const FCampaign1851Bridge& B : Bridges)
	{
		// Only what differs from the map as it was found.
		const bool bChanged = B.FerryKm > 0.f ? B.State != EBridgeState::Site : B.State != EBridgeState::Intact;
		if (bChanged)
		{
			Out.Add(FString::Printf(TEXT("bridge|%d|%d|%.2f"), B.Id, int32(B.State), B.DaysLeft));
		}
	}
	return Out;
}

void ACampaign1851Map::RestoreBridges(const TArray<FString>& Lines)
{
	DetectBridges();
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		const int32 i = P.Num() == 4 && P[0] == TEXT("bridge") ? BridgeIndex(FCString::Atoi(*P[1])) : INDEX_NONE;
		if (i != INDEX_NONE)
		{
			Bridges[i].State = EBridgeState(FMath::Clamp(FCString::Atoi(*P[2]), 0, 3));
			Bridges[i].DaysLeft = FCString::Atof(*P[3]);
			ApplyBridge(Bridges[i]);
		}
	}
	RebuildBridgeMeshes();
}
