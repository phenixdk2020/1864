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
		N.Relation = &N == &Nations[PlayerNation] ? 100.f : FMath::Clamp(N.BaseRelation + 20.f * Deviation * Rng.FRandRange(-1.f, 1.f), -100.f, 100.f);
		N.bTrade = N.bAlliance = N.bGuarantee = false;
		N.LastEnvoyDay = -1000.0;
	}
}

FString ACampaign1851Map::DiplomacyBlockReason(int32 NationIndex, EDiplomacyAction Action) const
{
	using namespace Campaign1851Diplomacy;
	if (!Nations.IsValidIndex(NationIndex) || NationIndex == PlayerNation || !Nations[NationIndex].bActive)
	{
		return TEXT("-");
	}
	const FCampaign1851Nation& N = Nations[NationIndex];
	const bool bEnemy = bAtWar && (N.Id == TEXT("PR") || N.Id == TEXT("AT"));
	switch (Action)
	{
	case EDiplomacyAction::Envoy:
		if (CampaignDays - N.LastEnvoyDay < EnvoyCooldownDays) return FString::Printf(TEXT("igen om %.0f dage"), EnvoyCooldownDays - (CampaignDays - N.LastEnvoyDay));
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
		if (!N.bTrade) return TEXT("kræver traktat");
		if (N.Relation < 60.f) return TEXT("kræver forhold 60");
		if (Treasury < AllianceCostNow()) return TEXT("ikke råd");
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
		AddTransaction(-AllianceCostNow(), FString::Printf(TEXT("Forsvarsalliance med %s"), *N.Name));
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
	D.Key = TEXT("forhold.") + N.Id;
	if (N.Id == TEXT("SE")) D.Key += TEXT(",opinion.skandinavisk,opinion.helstat,opinion.ejder");
	if (Action == EDiplomacyAction::Guarantee) D.Key += TEXT(",garant.") + N.Id;
	if (Action == EDiplomacyAction::Alliance) D.Key += TEXT(",spaending,opinion.ejder,opinion.helstat,opinion.skandinavisk");
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
		if (&N == &Nations[PlayerNation])
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
		const double TalksAfter = bBlockade && HasSeaControl() ? 45.0 : 60.0;   // the blockade hurts Hamburg and Stettin
		if (bGuaranteed && PeaceTalksDay < 0.0 && CampaignDays - WarStartDay > TalksAfter)
		{
			PeaceTalksDay = CampaignDays;
			News.Add(TEXT("Stormagterne kræver en fredskonference i London: fredsforhandling er mulig på bedre vilkår"));
		}
	}
}

float ACampaign1851Map::WarScore() const
{
	// -1 .. 1: the losses traded, the towns held, the sea.
	int32 Occupied = 0;
	for (const FCampaign1851City& C : Cities)
	{
		Occupied += !C.Occupier.IsEmpty() && !C.bForeign ? 1 : 0;
	}
	const float Losses = float(EnemyWarLosses - DanishWarLosses) / float(FMath::Max(10000, EnemyWarLosses + DanishWarLosses));
	return FMath::Clamp(Losses - 0.04f * Occupied + (bBlockade && HasSeaControl() ? 0.1f : 0.f), -1.f, 1.f);
}

TArray<FCampaign1851PeaceOffer> ACampaign1851Map::PeaceOffers() const
{
	TArray<FCampaign1851PeaceOffer> Offers;
	const float Score = WarScore();
	const bool bConference = PeaceTalksDay >= 0.0;
	auto Towns = [this](TFunctionRef<bool(const FCampaign1851City&)> Pred)
	{
		TArray<int32> Out;
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			if (!Cities[c].bForeign && Pred(Cities[c]))
			{
				Out.Add(c);
			}
		}
		return Out;
	};
	{
		FCampaign1851PeaceOffer O;
		O.Name = TEXT("Status quo");
		O.bAccepted = Score >= 0.25f;
		O.Why = O.bAccepted ? TEXT("fjenden er slået nok til at opgive") : FString::Printf(TEXT("kræver krigsstilling 0,25 (nu %.2f)"), Score);
		Offers.Add(O);
	}
	{
		// The line of language (roughly the Flensborg fjord): Holstein, Lauenburg and southern Schleswig go.
		FCampaign1851PeaceOffer O;
		O.Name = TEXT("Deling efter sprog");
		O.Ceded = Towns([](const FCampaign1851City& C) { return C.Region == TEXT("H") || (C.Region == TEXT("S") && C.Lat < 54.75); });
		O.bAccepted = bConference || Score >= -0.2f;
		O.Why = O.bAccepted ? (bConference ? TEXT("stormagterne på konferencen støtter delingen") : TEXT("fjenden kan godtage det")) : FString::Printf(TEXT("kræver konference eller krigsstilling −0,2 (nu %.2f)"), Score);
		Offers.Add(O);
	}
	{
		FCampaign1851PeaceOffer O;
		O.Name = TEXT("De besatte byer");
		O.Ceded = Towns([](const FCampaign1851City& C) { return !C.Occupier.IsEmpty(); });
		O.bAccepted = Score >= -0.6f;
		O.Why = O.bAccepted ? TEXT("fjenden beholder, hvad han har taget") : FString::Printf(TEXT("fjenden vil have mere (krigsstilling %.2f)"), Score);
		Offers.Add(O);
	}
	{
		FCampaign1851PeaceOffer O;
		O.Name = TEXT("Hertugdømmerne");
		O.Ceded = Towns([](const FCampaign1851City& C) { return C.Region == TEXT("H") || C.Region == TEXT("S"); });
		O.bAccepted = true;
		O.Why = TEXT("som i Wienfreden 1864: Slesvig, Holsten og Lauenborg afstås");
		Offers.Add(O);
	}
	return Offers;
}

bool ACampaign1851Map::MakePeace(int32 Offer, FString* OutReason)
{
	const TArray<FCampaign1851PeaceOffer> Offers = PeaceOffers();
	if (!bAtWar || !Offers.IsValidIndex(Offer))
	{
		return false;
	}
	const FCampaign1851PeaceOffer& O = Offers[Offer];
	if (!O.bAccepted)
	{
		if (OutReason) { *OutReason = FString::Printf(TEXT("Fjenden afviser: %s"), *O.Why); }
		return false;
	}
	LastWarScore = WarScore();
	// The ceded towns leave the monarchy (their amter no longer pay).
	TArray<FString> Ceded;
	for (int32 c : O.Ceded)
	{
		FCampaign1851City& C = Cities[c];
		C.bForeign = true;
		C.bCeded = true;
		if (C.Occupier.IsEmpty())
		{
			C.Occupier = TEXT("PR");
		}
		Ceded.Add(C.Name);
	}
	// Occupied towns not ceded come back.
	for (FCampaign1851City& C : Cities)
	{
		if (!C.bCeded && !C.Occupier.IsEmpty())
		{
			C.Occupier.Reset();
		}
	}
	bAtWar = false;
	ExchangePrisoners();
	EnemyCorps.Reset();
	ResetNeighbourArmies();
	Battles.Reset();
	Tension = 30.f;
	PeaceTalksDay = -1.0;
	PoliticalShock(Ceded.Num() == 0 ? 10.f : -FMath::Min(25.f, Ceded.Num() * 1.5f), Ceded.Num() == 0 ? 3.f : -6.f);
	News.Add(Ceded.Num() > 0 ? FString::Printf(TEXT("Fred (%s): %d byer afstås"), *O.Name, Ceded.Num()) : FString(TEXT("Fred på status quo")));
	FCampaign1851Decision D;
	D.Day = CampaignDays;
	D.Nation = PlayerNation;
	D.Portfolio = ECampaign1851Portfolio::War;
	D.Action = FString::Printf(TEXT("Fredsslutning: %s"), *O.Name);
	bEndPending = true;   // the war decided: the campaign's result
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
