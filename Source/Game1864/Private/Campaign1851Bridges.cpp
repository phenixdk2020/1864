// Bridges (Docs/Bridges1851.md). Where a road crosses water on the way between two towns there is a bridge;
// where a short ferry crosses a sound (Alssund, the narrow waters) a pontoon bridge can be laid. A bridge can
// be blown: the road is cut for both sides (the march planner goes round or not at all) until it is
// rebuilt. A bridge over a sound lets the enemy cross without boats, whatever the fleet does, unless blown.

#include "Campaign1851Map.h"

namespace Campaign1851Bridge
{
	constexpr double BlowCost = 500.0;
	constexpr double RebuildCost = 4000.0;
	constexpr float RebuildDays = 20.f;
	constexpr double PontoonCost = 25000.0;
	constexpr float PontoonDays = 45.f;
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
	for (int32 l = 0; l < Links.Num(); ++l)
	{
		FCampaign1851Link& L = Links[l];
		L.bBlocked = false;
		const FString Near = Cities.IsValidIndex(L.A) && Cities.IsValidIndex(L.B) ? FString::Printf(TEXT("%s–%s"), *Cities[L.A].Name, *Cities[L.B].Name) : FString();
		if (L.HasFerry())
		{
			// A short ferry: a site for a pontoon bridge (the sound's middle, where the road meets the water).
			if (L.FerryKm <= Campaign1851Bridge::MaxFerryKm && L.Km.Num() > 1)
			{
				FCampaign1851Bridge B;
				B.Id = Next++;
				B.Link = l;
				B.Name = L.Ferry.IsEmpty() ? Near : L.Ferry;
				B.Km = AlongLine(L.Km, LineLength(L.Km) * 0.5);
				B.LengthM = L.FerryKm * 1000.f;
				B.State = EBridgeState::Site;
				B.FerryKm = L.FerryKm;
				Bridges.Add(B);
			}
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
					B.Id = Next++;
					B.Link = l;
					const int32 Town = NearestTown(AlongLine(L.Km, (WetFrom + d) * 0.5f));
					B.Name = FString::Printf(TEXT("Broen ved %s"), Cities.IsValidIndex(Town) ? *Cities[Town].Name : *Near);
					B.Km = AlongLine(L.Km, (WetFrom + d) * 0.5f);
					B.LengthM = Span * 1000.f;
					B.State = EBridgeState::Intact;
					Bridges.Add(B);
				}
				WetFrom = -1.f;
			}
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|bridges|%d (%d pontoon sites)"), Bridges.Num(), Bridges.FilterByPredicate([](const FCampaign1851Bridge& B) { return B.State == EBridgeState::Site; }).Num());
}

int32 ACampaign1851Map::BridgeIndex(int32 Id) const
{
	return Bridges.IndexOfByPredicate([Id](const FCampaign1851Bridge& B) { return B.Id == Id; });
}

void ACampaign1851Map::ApplyBridge(const FCampaign1851Bridge& B)
{
	if (!Links.IsValidIndex(B.Link))
	{
		return;
	}
	FCampaign1851Link& L = Links[B.Link];
	// Blown or being rebuilt: the road is cut. A pontoon bridge standing: no ferry any more.
	const bool bCut = B.State == EBridgeState::Blown || (B.State == EBridgeState::Building && B.FerryKm <= 0.f);
	L.bBlocked = bCut;
	if (B.FerryKm > 0.f)
	{
		L.FerryKm = B.State == EBridgeState::Intact ? 0.f : B.FerryKm;
		L.bBlocked = B.State == EBridgeState::Blown;
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
		if (Treasury < Campaign1851Bridge::PontoonCost) return TEXT("ikke råd");
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
		AddTransaction(-Campaign1851Bridge::PontoonCost, FString::Printf(TEXT("Pontonbro: %s"), *B.Name));
		B.State = EBridgeState::Building;
		B.DaysLeft = Campaign1851Bridge::PontoonDays;
		break;
	}
	ApplyBridge(B);
	return true;
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
}
