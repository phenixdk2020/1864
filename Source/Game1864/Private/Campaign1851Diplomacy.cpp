// Foreign affairs of the 1851 campaign: relations with the other nations, envoys, trade treaties (customs
// income, growth in the ports), an alliance with Sweden-Norway, guarantees from the great powers (they
// damp the tension and call a conference in war), the Sound Dues and their redemption in 1857, and peace.

#include "Campaign1851Map.h"

namespace Campaign1851Diplomacy
{
	constexpr double EnvoyCooldownDays = 90.0;
	/** The Sound Dues (development share, estimate) until the Copenhagen Convention, then the redemption paid over 20 years. */
	constexpr double SoundDuesPerYear = 60000.0;
	constexpr double RedemptionPerYear = 30000.0;
}

void ACampaign1851Map::ResetDiplomacy()
{
	bSoundDuesAbolished = false;
	RedemptionYearsLeft = 0;
	PeaceTalksDay = -1.0;
	for (FCampaign1851City& C : Cities)
	{
		if (C.bCeded)
		{
			C.bCeded = C.bForeign = false;
		}
	}
	// Relations as the campaign starts: the historical tendency, varied by the seed.
	FRandomStream Rng(int32(HashCombine(uint32(Seed), 0xD1Au)));
	for (FCampaign1851Nation& N : Nations)
	{
		N.Relation = N.IsPlayer() ? 100.f : FMath::Clamp(N.BaseRelation + 20.f * Deviation * Rng.FRandRange(-1.f, 1.f), -100.f, 100.f);
		N.bTrade = N.bAlliance = N.bGuarantee = false;
		N.LastEnvoyDay = -1000.0;
	}
}

FString ACampaign1851Map::DiplomacyBlockReason(int32 NationIndex, EDiplomacyAction Action) const
{
	using namespace Campaign1851Diplomacy;
	if (!Nations.IsValidIndex(NationIndex) || Nations[NationIndex].IsPlayer())
	{
		return TEXT("-");
	}
	const FCampaign1851Nation& N = Nations[NationIndex];
	const bool bEnemy = bAtWar && (N.Id == TEXT("PR") || N.Id == TEXT("AT"));
	switch (Action)
	{
	case EDiplomacyAction::Envoy:
		if (CampaignDays - N.LastEnvoyDay < EnvoyCooldownDays) return FString::Printf(TEXT("gesandten er lige rejst (%.0f dage)"), EnvoyCooldownDays - (CampaignDays - N.LastEnvoyDay));
		if (Treasury < EnvoyCost) return TEXT("ikke råd");
		return FString();
	case EDiplomacyAction::Trade:
		if (N.bTrade) return TEXT("traktat indgået");
		if (bEnemy) return TEXT("i krig");
		if (N.Relation < 20.f) return TEXT("kræver forhold 20");
		if (Treasury < TreatyCost) return TEXT("ikke råd");
		return FString();
	case EDiplomacyAction::Alliance:
		if (!N.bCanAlly) return TEXT("-");
		if (N.bAlliance) return TEXT("allieret");
		if (!N.bTrade) return TEXT("kræver handelstraktat");
		if (N.Relation < 60.f) return TEXT("kræver forhold 60");
		if (Treasury < AllianceCost) return TEXT("ikke råd");
		return FString();
	case EDiplomacyAction::Guarantee:
		if (!N.bCanGuarantee) return TEXT("-");
		if (N.bGuarantee) return TEXT("garanti givet");
		if (N.Relation < 50.f) return TEXT("kræver forhold 50");
		if (Treasury < GuaranteeCost) return TEXT("ikke råd");
		return FString();
	}
	return TEXT("-");
}

bool ACampaign1851Map::DoDiplomacy(int32 NationIndex, EDiplomacyAction Action, FString* OutReason)
{
	using namespace Campaign1851Diplomacy;
	const FString Why = DiplomacyBlockReason(NationIndex, Action);
	if (!Why.IsEmpty())
	{
		if (OutReason) { *OutReason = Why == TEXT("-") ? FString(TEXT("Ikke muligt")) : Why; }
		return false;
	}
	FCampaign1851Nation& N = Nations[NationIndex];
	switch (Action)
	{
	case EDiplomacyAction::Envoy:
		// Gifts and good words: the closer already, the less there is to win.
		AddTransaction(-EnvoyCost, FString::Printf(TEXT("Gesandt til %s"), *N.Name));
		N.Relation = FMath::Min(100.f, N.Relation + 8.f * (1.f - FMath::Max(0.f, N.Relation) / 120.f));
		N.LastEnvoyDay = CampaignDays;
		break;
	case EDiplomacyAction::Trade:
		AddTransaction(-TreatyCost, FString::Printf(TEXT("Handelstraktat med %s"), *N.Name));
		N.bTrade = true;
		N.Relation = FMath::Min(100.f, N.Relation + 5.f);
		News.Add(FString::Printf(TEXT("Handelstraktat med %s: +%s rd. om året i told og handel"), *N.Name, *FString::FromInt(N.TradeValue)));
		break;
	case EDiplomacyAction::Alliance:
		AddTransaction(-AllianceCost, FString::Printf(TEXT("Forsvarsalliance med %s"), *N.Name));
		N.bAlliance = true;
		News.Add(FString::Printf(TEXT("Forsvarsalliance med %s: ved krig sendes en hjælpebrigade"), *N.Name));
		// Berlin and Vienna do not like it.
		Tension = FMath::Min(100.f, Tension + 4.f);
		break;
	case EDiplomacyAction::Guarantee:
		AddTransaction(-GuaranteeCost, FString::Printf(TEXT("Garanti fra %s"), *N.Name));
		N.bGuarantee = true;
		News.Add(FString::Printf(TEXT("%s garanterer helstaten: spændingen dæmpes, og i krig kræves en konference"), *N.Name));
		break;
	}
	FCampaign1851Decision D;
	D.Day = CampaignDays;
	D.Nation = PlayerNation;
	D.Portfolio = ECampaign1851Portfolio::Interior;
	D.Action = FString::Printf(TEXT("Udenrigs: %s  ·  %s"), Action == EDiplomacyAction::Envoy ? TEXT("gesandt") : Action == EDiplomacyAction::Trade ? TEXT("handelstraktat")
		: Action == EDiplomacyAction::Alliance ? TEXT("alliance") : TEXT("garanti"), *N.Name);
	D.Reasons = FString::Printf(TEXT("forholdet er nu %.0f"), N.Relation);
	D.bDone = true;
	AddDecision(D);
	return true;
}

float ACampaign1851Map::GuaranteeDamping() const
{
	// Each guaranteeing great power takes a fifth off the rise of the tension.
	int32 Guarantors = 0;
	for (const FCampaign1851Nation& N : Nations)
	{
		Guarantors += N.bGuarantee ? 1 : 0;
	}
	return FMath::Max(0.3f, 1.f - 0.2f * Guarantors);
}

double ACampaign1851Map::ForeignIncomePerYear() const
{
	double Total = bSoundDuesAbolished ? (RedemptionYearsLeft > 0 ? Campaign1851Diplomacy::RedemptionPerYear : 0.0) : Campaign1851Diplomacy::SoundDuesPerYear;
	for (const FCampaign1851Nation& N : Nations)
	{
		Total += N.bTrade && !(bAtWar && (N.Id == TEXT("PR") || N.Id == TEXT("AT"))) ? N.TradeValue : 0.0;
	}
	return Total;
}

int32 ACampaign1851Map::TradeTreaties() const
{
	int32 Count = 0;
	for (const FCampaign1851Nation& N : Nations)
	{
		Count += N.bTrade ? 1 : 0;
	}
	return Count;
}

void ACampaign1851Map::MonthlyDiplomacy()
{
	// The Sound Dues are redeemed by the maritime nations (the Copenhagen Convention, 14 March 1857).
	if (!bSoundDuesAbolished && GetDate() >= FDateTime(1857, 4, 1))
	{
		bSoundDuesAbolished = true;
		RedemptionYearsLeft = 20;
		News.Add(TEXT("Øresundstolden ophæves (Københavnstraktaten): søfartsnationerne betaler en kapitalisering over 20 år"));
	}
	if (bSoundDuesAbolished && RedemptionYearsLeft > 0 && GetDate().GetMonth() == 4)
	{
		--RedemptionYearsLeft;
	}
	const double Income = ForeignIncomePerYear() / 12.0;
	if (Income >= 1.0)
	{
		AddTransaction(Income, bSoundDuesAbolished ? TEXT("Told, handel og Øresundskapitalisering") : TEXT("Øresundstold og handelstraktater"));
	}
	// Relations drift back to their tendency; tension sours Berlin and Vienna; war ends alliances' patience.
	for (FCampaign1851Nation& N : Nations)
	{
		if (N.IsPlayer())
		{
			continue;
		}
		N.Relation += (N.BaseRelation - N.Relation) * 0.02f;
		if (N.Id == TEXT("PR") || N.Id == TEXT("AT"))
		{
			N.Relation -= (Tension - 25.f) / 40.f;
		}
		N.Relation = FMath::Clamp(N.Relation, -100.f, 100.f);
	}
	// In war: the ally sends its brigade once; the guarantors call a conference after two months.
	if (bAtWar)
	{
		for (FCampaign1851Nation& N : Nations)
		{
			if (N.bAlliance && !N.bAllyArrived)
			{
				N.bAllyArrived = true;
				const int32 Copenhagen = FindCity(TEXT("København"));
				for (int32 b = 0; b < 3 && Copenhagen != INDEX_NONE; ++b)
				{
					int32 Number = 1;
					while (FindRegiment(FString::Printf(TEXT("SE%d"), Number)) != INDEX_NONE) { ++Number; }
					const int32 R = AddRaisedRegiment(FString::Printf(TEXT("SE%d"), Number), FString::Printf(TEXT("%d. svensk-norske bataljon"), Number), ECampaign1851Arm::Infantry, Copenhagen, 760);
					Regiments[R].Present = 1.f;
					Regiments[R].Experience = 40.f;
				}
				News.Add(FString::Printf(TEXT("%s sender en hjælpebrigade til København"), *N.Name));
			}
		}
		const bool bGuaranteed = Nations.ContainsByPredicate([](const FCampaign1851Nation& N) { return N.bGuarantee && N.Relation >= 40.f; });
		if (bGuaranteed && PeaceTalksDay < 0.0 && CampaignDays - WarStartDay > 60.0)
		{
			PeaceTalksDay = CampaignDays;
			News.Add(TEXT("Stormagterne kræver en fredskonference i London: fredsforhandling er mulig på bedre vilkår"));
		}
	}
}

FString ACampaign1851Map::PeaceTerms() const
{
	TArray<FString> Lost;
	for (const FCampaign1851City& C : Cities)
	{
		if (!C.Occupier.IsEmpty() && !C.bForeign)
		{
			Lost.Add(C.Name);
		}
	}
	// A conference in London lets the kingdom keep what the enemy holds only in Holstein's south.
	if (PeaceTalksDay >= 0.0 && Lost.Num() > 0)
	{
		return FString::Printf(TEXT("Konferencen: de besatte byer (%s) afstås, men resten af Slesvig bevares"), *FString::Join(Lost, TEXT(", ")));
	}
	return Lost.Num() == 0 ? FString(TEXT("Status quo: intet afstås")) : FString::Printf(TEXT("De besatte byer afstås: %s"), *FString::Join(Lost, TEXT(", ")));
}

bool ACampaign1851Map::MakePeace()
{
	if (!bAtWar)
	{
		return false;
	}
	// The occupied towns are ceded: they leave the monarchy (their amter no longer pay).
	TArray<FString> Ceded;
	for (FCampaign1851City& C : Cities)
	{
		if (!C.Occupier.IsEmpty() && !C.bForeign)
		{
			C.bForeign = true;
			C.bCeded = true;
			Ceded.Add(C.Name);
		}
	}
	bAtWar = false;
	EnemyCorps.Reset();
	Battles.Reset();
	Tension = 30.f;
	PeaceTalksDay = -1.0;
	News.Add(Ceded.Num() > 0 ? FString::Printf(TEXT("Fred: %s afstås"), *FString::Join(Ceded, TEXT(", "))) : FString(TEXT("Fred på status quo")));
	FCampaign1851Decision D;
	D.Day = CampaignDays;
	D.Nation = PlayerNation;
	D.Portfolio = ECampaign1851Portfolio::War;
	D.Action = TEXT("Fredsslutning");
	D.Reasons = Ceded.Num() > 0 ? FString::Printf(TEXT("afstået: %s"), *FString::Join(Ceded, TEXT(", "))) : FString(TEXT("intet afstået"));
	D.bDone = true;
	AddDecision(D);
	return true;
}

TArray<FString> ACampaign1851Map::SaveDiplomacy() const
{
	TArray<FString> Out;
	Out.Add(FString::Printf(TEXT("state|%d|%d|%.2f|%.2f"), bSoundDuesAbolished ? 1 : 0, RedemptionYearsLeft, PeaceTalksDay, WarStartDay));
	for (const FCampaign1851Nation& N : Nations)
	{
		Out.Add(FString::Printf(TEXT("nation|%s|%.2f|%d|%d|%d|%d|%.2f"), *N.Id, N.Relation, N.bTrade ? 1 : 0, N.bAlliance ? 1 : 0, N.bGuarantee ? 1 : 0, N.bAllyArrived ? 1 : 0, N.LastEnvoyDay));
	}
	for (const FCampaign1851City& C : Cities)
	{
		if (C.bCeded)
		{
			Out.Add(FString::Printf(TEXT("ceded|%s"), *C.Name));
		}
	}
	return Out;
}

void ACampaign1851Map::RestoreDiplomacy(const TArray<FString>& Lines)
{
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 5 && P[0] == TEXT("state"))
		{
			bSoundDuesAbolished = P[1] == TEXT("1");
			RedemptionYearsLeft = FCString::Atoi(*P[2]);
			PeaceTalksDay = FCString::Atod(*P[3]);
			WarStartDay = FCString::Atod(*P[4]);
		}
		else if (P.Num() == 8 && P[0] == TEXT("nation"))
		{
			FCampaign1851Nation* N = Nations.FindByPredicate([&P](const FCampaign1851Nation& X) { return X.Id == P[1]; });
			if (N)
			{
				N->Relation = FCString::Atof(*P[2]);
				N->bTrade = P[3] == TEXT("1");
				N->bAlliance = P[4] == TEXT("1");
				N->bGuarantee = P[5] == TEXT("1");
				N->bAllyArrived = P[6] == TEXT("1");
				N->LastEnvoyDay = FCString::Atod(*P[7]);
			}
		}
		else if (P.Num() == 2 && P[0] == TEXT("ceded") && FindCity(P[1]) != INDEX_NONE)
		{
			FCampaign1851City& C = Cities[FindCity(P[1])];
			C.bForeign = true;
			C.bCeded = true;
		}
	}
}
