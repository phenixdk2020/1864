// Reconnaissance, the fog of war and the enemy's decisions (Docs/Intel1851.md, backlog items 1 and 2).
// In war the enemy corps are seen only by Danish troops within sight (the cavalry sees furthest) or reported
// by the towns near them, late by courier and at once by telegraph. The enemy in turn knows the Danes only
// near its corps exactly, elsewhere by rumour, and chooses its objectives by what it believes.

#include "Campaign1851Map.h"

namespace
{
	/** Sight of a unit (km): cavalry scouts far ahead; forts watch from their parapets. */
	float IntelSightKm(ECampaign1851Arm Arm)
	{
		return Arm == ECampaign1851Arm::Cavalry || Arm == ECampaign1851Arm::HorseArtillery ? 20.f : 8.f;
	}
	constexpr float IntelFortSightKm = 10.f;
	/** Towns of the monarchy report enemy columns within this distance. */
	constexpr float IntelTownReportKm = 12.f;
	/** How the enemy weighs a corps (the needle gun). */
	float IntelCorpsQuality(const FString& Nation) { return Nation == TEXT("PR") ? 1.3f : Nation == TEXT("AT") ? 1.05f : 0.9f; }
	/** A stable error in [-1, 1] for a sighting (the same count for the same corps on the same day). */
	float IntelNoise(int32 A, int32 B)
	{
		return FRandomStream(int32(HashCombine(uint32(A) * 7919u, uint32(B)))).FRandRange(-1.f, 1.f);
	}
}

void ACampaign1851Map::UpdateIntel()
{
	const int32 Day = FMath::FloorToInt(CampaignDays);
	const float SightRounding = ActiveScenario().Year < 1850 ? 10.f : 100.f;
	const float ReportRounding = ActiveScenario().Year < 1850 ? 100.f : 1000.f;
	for (FCampaign1851EnemyCorps& C : EnemyCorps)
	{
		// In peace the attachés and the newspapers tell where every corps stands.
		if (!bAtWar)
		{
			C.bSeen = true;
			C.SeenKm = C.Km;
			C.SeenDay = CampaignDays;
			C.SeenMen = C.Men;
			continue;
		}
		// Seen by troops or forts: now, and the better the closer the cavalry.
		float Error = -1.f;
		for (const FCampaign1851Regiment& R : Regiments)
		{
			// Rytterspejdning: the cavalry sees half as far again.
			const float Sight = IntelSightKm(R.Arm) * (R.Arm == ECampaign1851Arm::Cavalry && HasResearch(TEXT("recon")) ? 1.5f : 1.f);
			if (R.Men > 0 && FVector2D::Distance(R.Km, C.Km) < Sight)
			{
				const float E = Sight > 10.f ? 0.05f : 0.25f;
				Error = Error < 0.f ? E : FMath::Min(Error, E);
			}
		}
		for (const FCampaign1851Fort& F : Forts)
		{
			if (F.bBuilt && FVector2D::Distance(F.Km, C.Km) < IntelFortSightKm)
			{
				Error = Error < 0.f ? 0.25f : FMath::Min(Error, 0.25f);
			}
		}
		C.bSeen = Error >= 0.f;
		if (C.bSeen)
		{
			C.SeenKm = C.Km;
			C.SeenDay = CampaignDays;
			C.SeenMen = FMath::RoundToInt(C.Men * (1.f + Error * IntelNoise(C.Id, Day)) / SightRounding) * int32(SightRounding);
			C.ReportArrive = -1.0;
			continue;
		}
		// A report on the way arrives.
		if (C.ReportArrive >= 0.0 && CampaignDays >= C.ReportArrive)
		{
			if (C.ReportDay > C.SeenDay)
			{
				C.SeenKm = C.ReportKm;
				C.SeenDay = C.ReportDay;
				C.SeenMen = C.ReportMen;
			}
			C.ReportArrive = -1.0;
		}
		// A town of the monarchy near the column sends word: by telegraph (research, or a station on the
		// railway's line) within hours, else by courier in a day and a half.
		if (C.ReportArrive < 0.0)
		{
			for (int32 t = 0; t < Cities.Num(); ++t)
			{
				const FCampaign1851City& Town = Cities[t];
				if (Town.bForeign || !Town.Occupier.IsEmpty() || FVector2D::Distance(TownKm(t), C.Km) > IntelTownReportKm)
				{
					continue;
				}
				const double Delay = HasResearch(TEXT("telegraph")) ? 0.25 : HasStation(t) ? 0.5 : 1.5;
				C.ReportKm = C.Km;
				C.ReportDay = CampaignDays;
				C.ReportArrive = CampaignDays + Delay;
				C.ReportMen = FMath::RoundToInt(C.Men * (1.f + 0.4f * IntelNoise(C.Id + 77, Day)) / ReportRounding) * int32(ReportRounding);
				break;
			}
		}
	}
}

float ACampaign1851Map::EnemyEstimateOfDefence(int32 Town) const
{
	// What the enemy believes stands at a town: exact near its own corps, else rumour (±30 %).
	const FVector2D At = TownKm(Town);
	float Men = 0.f;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		const FCampaign1851Regiment& R = Regiments[i];
		if (R.Men <= 0 || FVector2D::Distance(R.Km, At) > 8.0)
		{
			continue;
		}
		const bool bWatched = EnemyCorps.ContainsByPredicate([&R](const FCampaign1851EnemyCorps& C) { return FVector2D::Distance(C.Km, R.Km) < 15.0; });
		Men += R.PresentMen() * (bWatched ? 1.f : 1.f + 0.3f * IntelNoise(i, Town)) + R.Guns * 60.f;
	}
	for (const FCampaign1851Fort& F : Forts)
	{
		if (F.bBuilt && FVector2D::Distance(F.Km, At) < 8.0)
		{
			int32 Inside = 0, Reserve = 0;
			FortMen(F, Inside, Reserve);
			Men += Inside * 2.f + F.Guns * 60.f;
		}
	}
	return Men;
}

bool ACampaign1851Map::ChooseCorpsObjective(int32 CorpsIndex)
{
	FCampaign1851EnemyCorps& C = EnemyCorps[CorpsIndex];
	if (CampaignDays < C.NextThink)
	{
		return C.Objectives.Num() > 0;
	}
	C.NextThink = CampaignDays + 1.0;
	const float Strength = C.Men * IntelCorpsQuality(C.Nation) + C.Guns * 60.f;
	// Towns over water are open again when the fleet no longer holds the sea (or the sounds freeze).
	if (!HasSeaControl() || IsIceWinter())
	{
		C.Barred.Reset();
	}
	// Candidates: the plan's objectives and every town of the monarchy within 120 km.
	int32 Best = INDEX_NONE, Siege = INDEX_NONE;
	float BestScore = 0.f, BestDefence = 0.f;
	float FirstDefence = -1.f;
	for (int32 t = 0; t < Cities.Num(); ++t)
	{
		const FCampaign1851City& Town = Cities[t];
		const int32 Planned = C.Objectives.IndexOfByKey(t);
		const double Dist = FVector2D::Distance(TownKm(t), C.Km);
		if (Town.bForeign || !Town.Occupier.IsEmpty() || t == C.Town || (Planned == INDEX_NONE && Dist > 120.0) || Town.bBornholm || C.Barred.Contains(t))
		{
			continue;
		}
		const float Defence = FMath::Max(1.f, EnemyEstimateOfDefence(t));
		if (Planned == 0)
		{
			FirstDefence = Defence;
		}
		const float Odds = Strength / Defence;
		if (Odds < 1.3f)
		{
			// Too strong to storm; a fortified objective of the plan can be besieged.
			if (Planned == 0 && Odds >= 0.6f && HasFortsNear(t))
			{
				Siege = t;
			}
			continue;
		}
		const float Value = 1.f + Town.Population / 5000.f + (Planned == 0 ? 4.f : Planned != INDEX_NONE ? 1.5f : 0.f);
		const float Score = Value * FMath::Min(Odds, 3.f) / (1.f + float(Dist) / 40.f);
		if (Score > BestScore)
		{
			Best = t;
			BestScore = Score;
			BestDefence = Defence;
		}
	}
	const int32 Nation = Nations.IndexOfByPredicate([&C](const FCampaign1851Nation& N) { return N.Id == (C.Nation == TEXT("DE") ? TEXT("PR") : C.Nation); });
	auto Decide = [&](const FString& Action, const FString& Reasons)
	{
		FCampaign1851Decision D;
		D.Day = CampaignDays;
		D.Nation = Nation;
		D.Portfolio = ECampaign1851Portfolio::War;
		D.Action = Action;
		D.Reasons = Reasons;
		D.bDone = true;
		AddDecision(D);
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|enemy-ai|%s|%s|%s"), *C.Name, *Action, *Reasons);
	};
	if (Best == INDEX_NONE && Siege != INDEX_NONE)
	{
		if (C.SiegeTown != Siege)
		{
			C.SiegeTown = Siege;
			Decide(FString::Printf(TEXT("%s går mod %s for at belejre stillingen"), *C.Name, *Cities[Siege].Name),
				FString::Printf(TEXT("for stærk til storm (ca. %s mand bag skanser)"), *FString::FromInt(int32(EnemyEstimateOfDefence(Siege)))));
		}
		return true;
	}
	if (Best == INDEX_NONE)
	{
		// Nothing it dares: it waits for reinforcements where it stands.
		if (!C.bWaitingNoted)
		{
			C.bWaitingNoted = true;
			const int32 Here = NearestTown(C.Km);
			Decide(FString::Printf(TEXT("%s venter på forstærkninger ved %s"), *C.Name, Cities.IsValidIndex(Here) ? *Cities[Here].Name : TEXT("?")),
				FString::Printf(TEXT("egen styrke ca. %s; alle mål forsvares for stærkt"), *FString::FromInt(int32(Strength))));
		}
		C.RestUntil = CampaignDays + 5.0;
		return false;
	}
	C.bWaitingNoted = false;
	C.SiegeTown = INDEX_NONE; // A new storm/march objective cancels the old siege intention.
	if (C.Objectives.Num() == 0 || C.Objectives[0] != Best)
	{
		const FString Was = C.Objectives.Num() > 0 && Cities.IsValidIndex(C.Objectives[0]) ? Cities[C.Objectives[0]].Name : FString();
		C.Objectives.Remove(Best);
		C.Objectives.Insert(Best, 0);
		Decide(Was.IsEmpty() ? FString::Printf(TEXT("%s går mod %s"), *C.Name, *Cities[Best].Name)
			: FString::Printf(TEXT("%s omgår %s og går mod %s"), *C.Name, *Was, *Cities[Best].Name),
			BestDefence < 50.f ? FString::Printf(TEXT("%s menes uforsvaret"), *Cities[Best].Name)
			: Was.IsEmpty() || FirstDefence < 0.f ? FString::Printf(TEXT("%s forsvares af ca. %s"), *Cities[Best].Name, *FString::FromInt(int32(BestDefence)))
			: FString::Printf(TEXT("%s menes forsvaret af ca. %s, %s kun af ca. %s"), *Was, *FString::FromInt(int32(FirstDefence)), *Cities[Best].Name, *FString::FromInt(int32(BestDefence))));
	}
	return true;
}

void ACampaign1851Map::EnemyReinforcements()
{
	// Every 30 days of war the allies send drafts to their corps (up to half again their first strength).
	const int32 WarDay = FMath::FloorToInt(CampaignDays - WarStartDay);
	if (!bAtWar || WarDay <= 0 || WarDay % 30 != 0)
	{
		return;
	}
	for (FCampaign1851EnemyCorps& C : EnemyCorps)
	{
		// A blockade with the sea held halves the drafts (the Baltic ports closed).
		const int32 Draft = (C.Nation == TEXT("PR") ? 3000 : 1500) / (bBlockade && HasSeaControl() ? 2 : 1);
		const int32 Add = FMath::Min(Draft, FMath::Max(0, C.StartMen * 3 / 2 - C.Men));
		if (Add > 0)
		{
			C.Men += Add;
			C.Guns += Add / 400;
			C.bWaitingNoted = false;
			C.RestUntil = 0.0;
			News.Add(FString::Printf(TEXT("Efterretning: %s forstærkes med ca. %s mand"), *C.Name, *FString::FromInt(Add)));
		}
	}
}
