// The ministers and the new portfolios (Docs/Ministers1851.md). Every portfolio has a minister with a name,
// a political current and three qualities: skill (how well the ministry chooses and how much it achieves),
// thrift (how much of the money it dares spend) and caution (how bold its foreign, naval and financial
// steps are). A new government brings its own ministers; the player may dismiss one. The portfolios for
// foreign affairs, the navy and finance act like the others: MANUAL, ADVISORY (they recommend) or AUTO.

#include "Campaign1851Map.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	struct FCampaignPoliticsMinisterName { const TCHAR* Name; ECampaign1851Current Line; };
	// Names from the period where they fit the office (several are the game's choice), by portfolio.
	const TArray<FCampaignPoliticsMinisterName>& CampaignPoliticsMinisterPool(ECampaign1851Portfolio P, bool bEarly)
	{
		using C = ECampaign1851Current;
		// Fictional advisers until constitutional ministries exist; no later politicians in 1825.
		static const TArray<FCampaignPoliticsMinisterName> Early = {
			{ TEXT("Helstatsrådgiver"), C::Helstat }, { TEXT("Helstatsembedsmand"), C::Helstat },
			{ TEXT("Slesvigrådgiver"), C::Ejder }, { TEXT("Slesvigembedsmand"), C::Ejder },
			{ TEXT("Nordisk rådgiver"), C::Scandinavian }
		};
		if (bEarly) { return Early; }
		static const TArray<FCampaignPoliticsMinisterName> Pools[int32(ECampaign1851Portfolio::Count)] = {
			{ { TEXT("F.F. Tillisch"), C::Helstat }, { TEXT("C.E. Rotwitt"), C::Helstat }, { TEXT("A.F. Krieger"), C::Ejder }, { TEXT("Orla Lehmann"), C::Ejder }, { TEXT("Carl Ploug"), C::Scandinavian } },
			{ { TEXT("A.S. Ørsted"), C::Helstat }, { TEXT("C.A. Fonnesbech"), C::Helstat }, { TEXT("J.A. Hansen"), C::Ejder }, { TEXT("C.F. Tietgen"), C::Ejder }, { TEXT("J.F. Schouw"), C::Scandinavian } },
			{ { TEXT("C.F. Hansen"), C::Helstat }, { TEXT("W. Lüttichau"), C::Helstat }, { TEXT("J.J.G. Hansen"), C::Ejder }, { TEXT("C.C. Lundbye"), C::Ejder }, { TEXT("M.R. Raasløff"), C::Scandinavian } },
			{ { TEXT("P.F. Steinmann"), C::Helstat }, { TEXT("W. Holstein"), C::Helstat }, { TEXT("J.C. Christensen"), C::Ejder }, { TEXT("L. Bramsen"), C::Ejder }, { TEXT("E. Hammerich"), C::Scandinavian } },
			{ { TEXT("Oberst Wegener"), C::Helstat }, { TEXT("Oberst Thestrup"), C::Helstat }, { TEXT("Oberst Kauffmann"), C::Ejder }, { TEXT("Major Rosen"), C::Ejder }, { TEXT("Major Bauditz"), C::Scandinavian } },
			{ { TEXT("C.A. Bluhme"), C::Helstat }, { TEXT("L.N. Scheele"), C::Helstat }, { TEXT("C.C. Hall"), C::Ejder }, { TEXT("C.E. Fenger"), C::Ejder }, { TEXT("J. Hegermann"), C::Scandinavian } },
			{ { TEXT("Steen Bille"), C::Helstat }, { TEXT("C.E. van Dockum"), C::Helstat }, { TEXT("O.W. Michelsen"), C::Ejder }, { TEXT("E. Suenson"), C::Ejder }, { TEXT("H. Gerner"), C::Scandinavian } },
			{ { TEXT("W. Sponneck"), C::Helstat }, { TEXT("C.G. Andræ"), C::Helstat }, { TEXT("C.E. Fenger"), C::Ejder }, { TEXT("A.F. Krieger"), C::Ejder }, { TEXT("C. Hostrup"), C::Scandinavian } },
		};
		return Pools[FMath::Clamp(int32(P), 0, int32(ECampaign1851Portfolio::Count) - 1)];
	}

	uint8 CampaignPoliticsMinisterQuality(uint32 SeedValue, const TCHAR* Name, uint32 Which)
	{
		// Two dice: most ministers are middling.
		FRandomStream Rng(int32(HashCombine(HashCombine(SeedValue, GetTypeHash(FString(Name))), Which)));
		return uint8(FMath::Clamp((Rng.RandRange(1, 10) + Rng.RandRange(1, 10) + 1) / 2, 1, 10));
	}
}

bool ACampaign1851Map::LoadScenarioMinisters()
{
	for (TArray<FCampaign1851Minister>& Pool : ScenarioMinisterPools) { Pool.Reset(); }
	if (ActiveScenario().Id != TEXT("1825")) { return true; }
	FString Text;
	TSharedPtr<FJsonObject> Json;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Campaign1851/Ministers_1825.json")))
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN|1825|ministers|cannot load Ministers_1825.json"));
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>& Portfolios = Json->GetArrayField(TEXT("portfolios"));
	if (Portfolios.Num() != int32(ECampaign1851Portfolio::Count)) { return false; }
	for (int32 p = 0; p < Portfolios.Num(); ++p)
	{
		for (const TSharedPtr<FJsonValue>& Value : Portfolios[p]->AsObject()->GetArrayField(TEXT("candidates")))
		{
			const TSharedPtr<FJsonObject> Entry = Value->AsObject();
			FCampaign1851Minister M;
			M.Name = Entry->GetStringField(TEXT("name"));
			M.Line = ECampaign1851Current::Helstat;
			M.Skill = uint8(FMath::Clamp(FMath::RoundToInt(Entry->GetNumberField(TEXT("skill")) / 10.0), 1, 10));
			M.Thrift = uint8(FMath::Clamp(FMath::RoundToInt(Entry->GetNumberField(TEXT("thrift")) / 10.0), 1, 10));
			M.Caution = uint8(FMath::Clamp(FMath::RoundToInt(Entry->GetNumberField(TEXT("caution")) / 10.0), 1, 10));
			ScenarioMinisterPools[p].Add(M);
		}
		if (ScenarioMinisterPools[p].IsEmpty()) { return false; }
	}
	return true;
}

FCampaign1851Minister ACampaign1851Map::MakeMinister(ECampaign1851Portfolio P, ECampaign1851Current Line, const FString& Avoid) const
{
	if (ActiveScenario().Id == TEXT("1825") && GetDate() < FDateTime(1848, 3, 22) && int32(P) >= 0 && int32(P) < int32(ECampaign1851Portfolio::Count))
	{
		const TArray<FCampaign1851Minister>& Pool = ScenarioMinisterPools[int32(P)];
		for (const FCampaign1851Minister& Candidate : Pool)
		{
			if (Candidate.Name != Avoid)
			{
				FCampaign1851Minister M = Candidate;
				M.Since = CampaignDays;
				return M;
			}
		}
	}
	const TArray<FCampaignPoliticsMinisterName>& Names = CampaignPoliticsMinisterPool(P, ActiveScenario().Year < 1850 && GetDate().GetYear() < 1848);
	TArray<int32> Fit;
	for (int32 i = 0; i < Names.Num(); ++i)
	{
		if (Names[i].Line == Line && Avoid != Names[i].Name)
		{
			Fit.Add(i);
		}
	}
	if (Fit.Num() == 0)
	{
		for (int32 i = 0; i < Names.Num(); ++i)
		{
			if (Avoid != Names[i].Name)
			{
				Fit.Add(i);
			}
		}
	}
	FRandomStream Rng(int32(HashCombine(uint32(Seed), uint32(FMath::FloorToInt(CampaignDays)) * 7u + uint32(P))));
	const FCampaignPoliticsMinisterName& N = Names[Fit[Rng.RandHelper(Fit.Num())]];
	FCampaign1851Minister M;
	M.Name = N.Name;
	M.Line = N.Line;
	M.Skill = CampaignPoliticsMinisterQuality(uint32(Seed), N.Name, 1);
	M.Thrift = CampaignPoliticsMinisterQuality(uint32(Seed), N.Name, 2);
	M.Caution = CampaignPoliticsMinisterQuality(uint32(Seed), N.Name, 3);
	M.Since = CampaignDays;
	return M;
}

void ACampaign1851Map::AppointCabinet(ECampaign1851Current Line)
{
	for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
	{
		// A minister of the government's own current stays in office.
		if (Ministers[p].Name.IsEmpty() || Ministers[p].Line != Line)
		{
			Ministers[p] = MakeMinister(ECampaign1851Portfolio(p), Line, Ministers[p].Name);
		}
	}
}

TArray<FCampaign1851Minister> ACampaign1851Map::MinisterCandidates(ECampaign1851Portfolio P) const
{
	TArray<FCampaign1851Minister> Out;
	if (int32(P) < 0 || int32(P) >= int32(ECampaign1851Portfolio::Count)) { return Out; }
	if (ActiveScenario().Id == TEXT("1825") && GetDate() < FDateTime(1848, 3, 22) && !ScenarioMinisterPools[int32(P)].IsEmpty())
	{
		for (FCampaign1851Minister M : ScenarioMinisterPools[int32(P)])
		{
			if (M.Name != Ministers[int32(P)].Name) { M.Since = CampaignDays; Out.Add(M); }
		}
		return Out;
	}
	for (const FCampaignPoliticsMinisterName& N : CampaignPoliticsMinisterPool(P, ActiveScenario().Year < 1850 && GetDate().GetYear() < 1848))
	{
		if (Ministers[int32(P)].Name == N.Name)
		{
			continue;
		}
		FCampaign1851Minister M;
		M.Name = N.Name;
		M.Line = N.Line;
		M.Skill = CampaignPoliticsMinisterQuality(uint32(Seed), N.Name, 1);
		M.Thrift = CampaignPoliticsMinisterQuality(uint32(Seed), N.Name, 2);
		M.Caution = CampaignPoliticsMinisterQuality(uint32(Seed), N.Name, 3);
		M.Since = CampaignDays;
		Out.Add(M);
	}
	return Out;
}

bool ACampaign1851Map::AppointMinister(int32 Portfolio, int32 Candidate)
{
	if (Portfolio < 0 || Portfolio >= int32(ECampaign1851Portfolio::Count))
	{
		return false;
	}
	const TArray<FCampaign1851Minister> Candidates = MinisterCandidates(ECampaign1851Portfolio(Portfolio));
	if (!Candidates.IsValidIndex(Candidate))
	{
		return false;
	}
	const FString Old = Ministers[Portfolio].Name;
	Ministers[Portfolio] = Candidates[Candidate];
	News.Add(FString::Printf(TEXT("%s afløses af %s som minister for %s"), *Old, *Ministers[Portfolio].Name, Campaign1851Nations::PortfolioName(ECampaign1851Portfolio(Portfolio))));
	return true;
}

bool ACampaign1851Map::DismissMinister(int32 Portfolio)
{
	if (Portfolio < 0 || Portfolio >= int32(ECampaign1851Portfolio::Count))
	{
		return false;
	}
	const FString Old = Ministers[Portfolio].Name;
	Ministers[Portfolio] = MakeMinister(ECampaign1851Portfolio(Portfolio), Government, Old);
	News.Add(FString::Printf(TEXT("%s afskediget; ny minister for %s: %s"), *Old, Campaign1851Nations::PortfolioName(ECampaign1851Portfolio(Portfolio)), *Ministers[Portfolio].Name));
	return true;
}

float ACampaign1851Map::MinisterBudgetFactor(ECampaign1851Portfolio P) const
{
	const FCampaign1851Minister& M = Ministers[int32(P)];
	return (0.7f + 0.06f * M.Skill) * (1.25f - 0.05f * M.Thrift);
}

void ACampaign1851Map::SetAllDelegation(ECampaign1851Delegation Mode)
{
	for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
	{
		SetDelegation(ECampaign1851Portfolio(p), Mode);
	}
}

TArray<FCampaign1851Decision> ACampaign1851Map::MinisterOptions(ECampaign1851Portfolio P, double Budget) const
{
	TArray<FCampaign1851Decision> Out;
	const FCampaign1851Minister& M = Ministers[int32(P)];
	const float Bold = (11.f - M.Caution) / 10.f;   // 0.1 careful .. 1 bold
	auto Add = [&](ECampaign1851DecisionKind Kind, int32 A, int32 B, double Cost, float Score, const FString& Action, const FString& Why)
	{
		FCampaign1851Decision D;
		D.Kind = Kind;
		D.A = A;
		D.B = B;
		D.Cost = Cost;
		D.Score = Score * (0.8f + 0.04f * M.Skill);
		D.Action = Action;
		D.Reasons = FString::Printf(TEXT("%s  ·  minister %s"), *Why, *M.Name);
		Out.Add(D);
	};
	if (P == ECampaign1851Portfolio::Foreign)
	{
		// Peace first when the war goes badly or drags on: the best terms the enemy would take.
		if (bAtWar && ((WarScore() < -0.4f && CampaignDays - WarStartDay > 90.0) || CampaignDays - WarStartDay > 240.0))
		{
			const TArray<FCampaign1851PeaceOffer> Offers = PeaceOffers();
			for (int32 o = 0; o < Offers.Num(); ++o)
			{
				if (Offers[o].bAccepted)
				{
					Add(ECampaign1851DecisionKind::Peace, o, 0, 0.0, 100.f, FString::Printf(TEXT("Tilbyd fred: %s"), *Offers[o].Name),
						FString::Printf(TEXT("krigsstilling %.2f; fjenden vil tage imod (%d byer afstås)"), WarScore(), Offers[o].Ceded.Num()));
					break;
				}
			}
		}
		for (int32 n = 0; n < Nations.Num(); ++n)
		{
			if (n == PlayerNation)
			{
				continue;
			}
			const FCampaign1851Nation& N = Nations[n];
			auto Can = [&](EDiplomacyAction A, double Cost) { return DiplomacyBlockReason(n, A).IsEmpty() && Cost <= Budget; };
			if (Can(EDiplomacyAction::Guarantee, GuaranteeCost))
			{
				Add(ECampaign1851DecisionKind::Diplomacy, n, int32(EDiplomacyAction::Guarantee), GuaranteeCost, 8.f + Tension / 10.f,
					FString::Printf(TEXT("Søg garanti hos %s"), *N.Name), FString::Printf(TEXT("forhold %.0f; spænding %.0f"), N.Relation, Tension));
			}
			if (Can(EDiplomacyAction::Alliance, AllianceCostNow()) && M.Caution <= 6)
			{
				Add(ECampaign1851DecisionKind::Diplomacy, n, int32(EDiplomacyAction::Alliance), AllianceCostNow(), 6.f * Bold + Tension / 15.f,
					FString::Printf(TEXT("Forsvarsalliance med %s"), *N.Name), FString::Printf(TEXT("forhold %.0f"), N.Relation));
			}
			if (Can(EDiplomacyAction::Trade, TreatyCost))
			{
				Add(ECampaign1851DecisionKind::Diplomacy, n, int32(EDiplomacyAction::Trade), TreatyCost, 3.f + N.TradeValue / 2000.f,
					FString::Printf(TEXT("Handelstraktat med %s"), *N.Name), FString::Printf(TEXT("+%d rd. om året"), N.TradeValue));
			}
			// Envoys where a relation matters and falls short: the guarantors, Sweden-Norway, the German powers in tension.
			const float Want = N.bCanGuarantee ? 50.f : N.bCanAlly ? 60.f : (N.Id == TEXT("PR") || N.Id == TEXT("AT")) && Tension > 50.f ? 0.f : -1000.f;
			if (N.Relation < Want && Can(EDiplomacyAction::Envoy, EnvoyCost))
			{
				Add(ECampaign1851DecisionKind::Diplomacy, n, int32(EDiplomacyAction::Envoy), EnvoyCost, 2.f + (Want - N.Relation) / 10.f,
					FString::Printf(TEXT("Send gesandt til %s"), *N.Name), FString::Printf(TEXT("forhold %.0f, ønsket %.0f"), N.Relation, Want));
			}
		}
	}
	else if (P == ECampaign1851Portfolio::Navy)
	{
		// The blockade in war when the sea is safely held; off again when it is not.
		if (bAtWar && !bBlockade && DanishSeaStrength() > EnemySeaStrength() * (1.1f + 0.05f * M.Caution))
		{
			Add(ECampaign1851DecisionKind::Blockade, 0, 1, 0.0, 50.f, TEXT("Bloker de tyske havne"), FString::Printf(TEXT("søstyrke %.0f mod %.0f"), DanishSeaStrength(), EnemySeaStrength()));
		}
		if (bBlockade && (!bAtWar || !HasSeaControl()))
		{
			Add(ECampaign1851DecisionKind::Blockade, 0, 0, 0.0, 50.f, TEXT("Hæv blokaden"), bAtWar ? FString(TEXT("fjenden er overlegen til søs")) : FString(TEXT("freden er sluttet")));
		}
		// Ships while the enemy's navy grows (keep a margin of half again his strength, in war his and Austria's).
		const float PeaceThreat = ActiveScenario().Year < 1850
			? 4.f + 1.2f * FMath::Max(0.f, float((GetDate() - FDateTime(1851, 7, 1)).GetTotalDays() / 365.0))
			: 4.f + 1.2f * float(CampaignDays / 365.0) + 22.f;
		const float Want = FMath::Max(ActiveScenario().Year < 1850 && !bAtWar ? 6.f : 40.f, (bAtWar ? EnemySeaStrength() : PeaceThreat) * 1.5f);
		if (DanishSeaStrength() < Want)
		{
			int32 Best = INDEX_NONE;
			float BestValue = 0.f;
			for (int32 c = 0; c < Campaign1851Navy::Classes().Num(); ++c)
			{
				const FCampaign1851ShipClass& K = Campaign1851Navy::Classes()[c];
				if (ShipBlockReason(c).IsEmpty() && K.Cost <= Budget && K.Strength / float(K.Cost) * 100000.f > BestValue)
				{
					Best = c;
					BestValue = K.Strength / float(K.Cost) * 100000.f;
				}
			}
			if (Best != INDEX_NONE)
			{
				const FCampaign1851ShipClass& K = Campaign1851Navy::Classes()[Best];
				Add(ECampaign1851DecisionKind::Ship, Best, 0, K.Cost, 5.f + (Want - DanishSeaStrength()) / 5.f, FString::Printf(TEXT("Byg en %s"), K.Name),
					FString::Printf(TEXT("søstyrke %.0f, ønsket %.0f"), DanishSeaStrength(), Want));
			}
		}
	}
	else if (P == ECampaign1851Portfolio::Finance)
	{
		const double Reserve = Nations.IsValidIndex(PlayerNation) ? Nations[PlayerNation].Reserve : 0.0;
		// Borrow when the cash runs below the reserve and credit is cheap enough for this minister.
		if (Treasury < Reserve && CreditRate() < 0.05f + 0.004f * (10 - M.Caution))
		{
			const int32 Size = Treasury < Reserve * 0.3 ? 1 : 0;
			if (LoanBlockReason(Size == 1 ? 250000.0 : 100000.0).IsEmpty())
			{
				Add(ECampaign1851DecisionKind::Loan, 0, Size, 0.0, 60.f, Size == 1 ? TEXT("Optag statslån på 250.000 rd.") : TEXT("Optag statslån på 100.000 rd."),
					FString::Printf(TEXT("kassen %s rd. under reserven; rente %.1f %%"), *FString::FromInt(int32(Reserve - Treasury)), CreditRate() * 100.f));
			}
		}
		// Repay when the cash is ample (the thrifty minister sooner).
		if (Debt > 1.0 && Treasury > Reserve * (4.0 - 0.2 * M.Thrift) + 100000.0)
		{
			Add(ECampaign1851DecisionKind::Loan, 0, 2, 100000.0, 20.f, TEXT("Afdrag 100.000 rd. på statsgælden"), FString::Printf(TEXT("gæld %s rd. til %.1f %%"), *FString::FromInt(int32(Debt)), DebtRate * 100.f));
		}
	}
	else if (P == ECampaign1851Portfolio::Intendance && CanImport())
	{
		// Three months of iron and coal for the works; cloth and leather for a new battalion or two.
		float Made[int32(ECampaign1851Raw::Count)], Used[int32(ECampaign1851Raw::Count)];
		RawFlow(Made, Used);
		for (int32 r = 0; r < int32(ECampaign1851Raw::Count); ++r)
		{
			const ECampaign1851Raw R = ECampaign1851Raw(r);
			const float Need = R == ECampaign1851Raw::Cloth || R == ECampaign1851Raw::Leather ? 1600.f * (ActiveScenario().Year < 1850 ? ActiveScenario().ArmyFactor : 1.f) : R == ECampaign1851Raw::Powder ? 200.f : 3.f * FMath::Max(0.f, Used[r] - Made[r]);
			const float Short = Need - RawStock[r];
			const int32 Amount = FMath::CeilToInt(Short);
			if (Short > 0.5f && RawPrice(R) * Amount <= Budget)
			{
				const Campaign1851Resources::FRawInfo& I = Campaign1851Resources::Info(R);
				Add(ECampaign1851DecisionKind::BuyRaw, r, Amount, RawPrice(R) * Amount, 8.f + Short / FMath::Max(Need, 1.f) * 10.f,
					FString::Printf(TEXT("Køb %d %s %s"), Amount, I.Unit, I.Name), FString::Printf(TEXT("lager %.0f, ønsket %.0f"), RawStock[r], Need));
			}
		}
	}
	else if (P == ECampaign1851Portfolio::War && !IsDoctrineChanging())
	{
		// The doctrine to the situation: no bayonet against the needle gun; the fortress with great works.
		if (bAtWar && Doctrine[2] == 1 && EnemyCorps.ContainsByPredicate([](const FCampaign1851EnemyCorps& C) { return C.Nation == TEXT("PR"); }))
		{
			Add(ECampaign1851DecisionKind::Doctrine, 2, Forts.Num() > 3 ? 0 : 2, DoctrineChangeCost, 30.f, Forts.Num() > 3 ? TEXT("Ny taktisk doktrin: ildkamp bag skanserne") : TEXT("Ny taktisk doktrin: spredt orden"),
				TEXT("bajonetten er håbløs mod tændnålsgeværet"));
		}
		const int32 Built = Forts.FilterByPredicate([](const FCampaign1851Fort& F) { return F.bBuilt; }).Num();
		if (Doctrine[0] == 1 && Built >= 6)
		{
			Add(ECampaign1851DecisionKind::Doctrine, 0, 0, DoctrineChangeCost, 10.f, TEXT("Ny strategisk doktrin: fæstningen"), FString::Printf(TEXT("%d skanser står færdige"), Built));
		}
	}
	return Out;
}

bool ACampaign1851Map::CarryOutMinister(const FCampaign1851Decision& D)
{
	switch (D.Kind)
	{
	case ECampaign1851DecisionKind::Diplomacy:
		return DoDiplomacy(D.A, EDiplomacyAction(D.B));
	case ECampaign1851DecisionKind::Peace:
		return MakePeace(D.A);
	case ECampaign1851DecisionKind::Ship:
		return OrderShip(D.A);
	case ECampaign1851DecisionKind::Blockade:
		if (bBlockade == (D.B == 1))
		{
			return false;
		}
		SetBlockade(D.B == 1);
		return true;
	case ECampaign1851DecisionKind::Loan:
		return D.B == 2 ? RepayLoan(100000.0) : TakeLoan(D.B == 1 ? 250000.0 : 100000.0);
	case ECampaign1851DecisionKind::Doctrine:
		return SetDoctrine(D.A, D.B);
	case ECampaign1851DecisionKind::BuyRaw:
		return BuyRaw(ECampaign1851Raw(D.A), float(D.B));
	default:
		return false;
	}
}

void ACampaign1851Map::StepMinistryBudget(ECampaign1851Portfolio P, int32 Dir)
{
	static const double Steps[] = { 0.0, 1000.0, 2000.0, 5000.0, 10000.0, 15000.0, 20000.0, 30000.0, 50000.0, 75000.0, 100000.0 };
	double& B = MinistryBudget[int32(P)];
	int32 At = 0;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Steps); ++i)
	{
		if (Steps[i] <= B + 0.5) { At = i; }
	}
	B = Steps[FMath::Clamp(At + Dir, 0, int32(UE_ARRAY_COUNT(Steps)) - 1)];
	MinistryPot[int32(P)] = FMath::Min(MinistryPot[int32(P)], B * 3.0);
}

void ACampaign1851Map::RefillMinistryBudgets()
{
	for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
	{
		MinistryPot[p] = FMath::Min(MinistryPot[p] + MinistryBudget[p], MinistryBudget[p] * 3.0);
	}
}

TArray<FString> ACampaign1851Map::SaveMinisters() const
{
	TArray<FString> Out;
	for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
	{
		Out.Add(FString::Printf(TEXT("budget|%d|%.0f|%.0f"), p, MinistryBudget[p], MinistryPot[p]));
	}
	for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
	{
		const FCampaign1851Minister& M = Ministers[p];
		Out.Add(FString::Printf(TEXT("min|%d|%s|%d|%d|%d|%d|%.1f"), p, *M.Name, int32(M.Line), M.Skill, M.Thrift, M.Caution, M.Since));
	}
	return Out;
}

void ACampaign1851Map::RestoreMinister(const TArray<FString>& P)
{
	const int32 p = FCString::Atoi(*P[1]);
	if (p < 0 || p >= int32(ECampaign1851Portfolio::Count))
	{
		return;
	}
	FCampaign1851Minister& M = Ministers[p];
	M.Name = P[2];
	M.Line = ECampaign1851Current(FMath::Clamp(FCString::Atoi(*P[3]), 0, 2));
	M.Skill = uint8(FMath::Clamp(FCString::Atoi(*P[4]), 1, 10));
	M.Thrift = uint8(FMath::Clamp(FCString::Atoi(*P[5]), 1, 10));
	M.Caution = uint8(FMath::Clamp(FCString::Atoi(*P[6]), 1, 10));
	M.Since = FCString::Atod(*P[7]);
}
