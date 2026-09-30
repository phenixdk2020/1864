// Nations, growth and the national AI of the 1851 campaign (design manual: multi-nation AI parity,
// granular AI delegation, historically plausible replay variation). ACampaign1851Map's world layer.

#include "Campaign1851Map.h"

#include "Campaign1851Buildings.h"
#include "Campaign1851ConstructionSite.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace Campaign1851Nations
{
	const TCHAR* PortfolioName(ECampaign1851Portfolio P)
	{
		switch (P)
		{
		case ECampaign1851Portfolio::Interior: return TEXT("Indenrigs");
		case ECampaign1851Portfolio::PublicWorks: return TEXT("Offentlige arbejder");
		case ECampaign1851Portfolio::War: return TEXT("Krigsministeriet");
		case ECampaign1851Portfolio::Intendance: return TEXT("Intendanturen");
		case ECampaign1851Portfolio::Foreign: return TEXT("Udenrigs");
		case ECampaign1851Portfolio::Navy: return TEXT("Marinen");
		case ECampaign1851Portfolio::Finance: return TEXT("Finanserne");
		default: return TEXT("Transport");
		}
	}

	const TCHAR* PortfolioScope(ECampaign1851Portfolio P)
	{
		switch (P)
		{
		case ECampaign1851Portfolio::Interior: return TEXT("skoler, rådhuse, handel og industri i byerne");
		case ECampaign1851Portfolio::PublicWorks: return TEXT("chausséer og jernbaner");
		case ECampaign1851Portfolio::War: return TEXT("øvelser, officerer og ledige poster");
		case ECampaign1851Portfolio::Intendance: return TEXT("depoter og trænkolonner");
		case ECampaign1851Portfolio::Foreign: return TEXT("gesandter, traktater, alliance, garantier, fred");
		case ECampaign1851Portfolio::Navy: return TEXT("skibe og blokade");
		case ECampaign1851Portfolio::Finance: return TEXT("statslån og afdrag");
		default: return TEXT("troppetog");
		}
	}

	const TCHAR* DelegationName(ECampaign1851Delegation D)
	{
		return D == ECampaign1851Delegation::Manual ? TEXT("MANUEL") : D == ECampaign1851Delegation::Advisory ? TEXT("RÅDGIVER") : TEXT("AUTO");
	}

	const FCampaign1851CivilEffect* CivilEffect(const FString& Key)
	{
		// Estimates for play: growth in percentage points a year, income in rigsdaler a year.
		static const TMap<FString, FCampaign1851CivilEffect> Effects = {
			{ TEXT("Schoolhouse"),      { 0.15f, 0.05f, 0, false } },
			{ TEXT("Town_Hall"),        { 0.10f, 0.f, 500, false } },
			{ TEXT("Post_Office"),      { 0.15f, 0.02f, 300, false } },
			{ TEXT("Hospital"),         { 0.30f, 0.03f, 0, false } },
			{ TEXT("Harbor_Building"),  { 0.15f, 0.f, 900, false } },
			{ TEXT("Lighthouse"),       { 0.05f, 0.f, 300, false } },
			{ TEXT("Merchant_House"),   { 0.30f, 0.10f, 500, true } },
			{ TEXT("Brewery"),          { 0.20f, 0.05f, 700, true } },
			{ TEXT("Brickworks"),       { 0.20f, 0.f, 400, true } },
			{ TEXT("Sawmill"),          { 0.10f, 0.02f, 250, true } },
			{ TEXT("Machine_Workshop"), { 0.40f, 0.f, 1200, true } },
			{ TEXT("Textile_Mill"),     { 0.60f, 0.05f, 2500, true } },
			{ TEXT("Inn"),              { 0.10f, 0.05f, 150, true } },
		};
		return Effects.Find(Key);
	}
}

namespace
{
	/** A deterministic number in [-1, 1] for the campaign seed and a salt (same seed, same world). */
	float SeedJitter(int32 Seed, uint32 Salt)
	{
		FRandomStream Rng(int32(HashCombine(uint32(Seed), Salt)));
		return Rng.FRandRange(-1.f, 1.f);
	}

	FString Rd(double V)
	{
		const int64 N = FMath::RoundToInt64(FMath::Abs(V));
		FString Digits = FString::Printf(TEXT("%lld"), N);
		for (int32 i = Digits.Len() - 3; i > 0; i -= 3)
		{
			Digits.InsertAt(i, TEXT('.'));
		}
		return (V < 0.0 ? TEXT("-") : TEXT("")) + Digits;
	}
}

bool ACampaign1851Map::LoadNations()
{
	FString Text;
	TSharedPtr<FJsonObject> Json;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Campaign1851/Nations1851.json")))
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|nations|no Data/Campaign1851/Nations1851.json; Denmark alone"));
		FCampaign1851Nation Dk;
		Dk.Id = TEXT("DK");
		Dk.Name = TEXT("Danmark");
		Dk.bOnMap = true;
		Dk.Controller = ECampaign1851Controller::Player;
		NationsAtStart = { Dk };
		return false;
	}
	NationsAtStart.Reset();
	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("nations")))
	{
		const TSharedPtr<FJsonObject> O = Value->AsObject();
		FCampaign1851Nation N;
		N.Id = O->GetStringField(TEXT("id"));
		N.Name = O->GetStringField(TEXT("name"));
		O->TryGetBoolField(TEXT("onMap"), N.bOnMap);
		N.Controller = O->GetStringField(TEXT("controller")) == TEXT("player") ? ECampaign1851Controller::Player : ECampaign1851Controller::AI;
		const TArray<TSharedPtr<FJsonValue>>* Weights = nullptr;
		if (O->TryGetArrayField(TEXT("weights"), Weights))
		{
			for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
			{
				N.BaseWeights[p] = p < Weights->Num() ? float((*Weights)[p]->AsNumber()) : 1.f;
			}
		}
		double V = 0.0;
		if (O->TryGetNumberField(TEXT("caution"), V)) { N.Caution = float(V); }
		O->TryGetNumberField(TEXT("reserve"), N.Reserve);
		O->TryGetNumberField(TEXT("population"), N.Population);
		O->TryGetNumberField(TEXT("railKm"), N.RailKm);
		O->TryGetNumberField(TEXT("armyMen"), N.ArmyMen);
		O->TryGetNumberField(TEXT("taxPerHead"), N.TaxPerHead);
		if (O->TryGetNumberField(TEXT("growth"), V)) { N.BaseGrowth = float(V); }
		O->TryGetStringField(TEXT("note"), N.Note);
		if (O->TryGetNumberField(TEXT("relation"), V)) { N.BaseRelation = float(V); }
		O->TryGetNumberField(TEXT("tradeValue"), N.TradeValue);
		O->TryGetBoolField(TEXT("canAlly"), N.bCanAlly);
		O->TryGetBoolField(TEXT("canGuarantee"), N.bCanGuarantee);
		// The player's own nation starts with every portfolio in his hands.
		for (ECampaign1851Delegation& M : N.Modes)
		{
			M = N.IsPlayer() ? ECampaign1851Delegation::Manual : ECampaign1851Delegation::Auto;
		}
		NationsAtStart.Add(N);
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|nations|%d nations"), NationsAtStart.Num());
	return NationsAtStart.Num() > 0;
}

void ACampaign1851Map::ResetWorld(int32 InSeed, float InDeviation)
{
	Seed = InSeed;
	Deviation = FMath::Clamp(InDeviation, 0.f, 0.5f);
	Decisions.Reset();
	// The towns and amter as in 1851 (kept from the first reset; growth starts from them).
	if (CityBasePopulation.Num() != Cities.Num())
	{
		CityBasePopulation.Reset();
		for (const FCampaign1851City& C : Cities) { CityBasePopulation.Add(C.Population); }
		AmtBase.Reset();
		for (const FCampaign1851Amt& A : Amter) { AmtBase.Add(FIntPoint(A.Urban, A.Rural)); }
	}
	for (int32 c = 0; c < Cities.Num(); ++c) { Cities[c].Population = CityBasePopulation[c]; }
	for (int32 a = 0; a < Amter.Num(); ++a)
	{
		Amter[a].Urban = AmtBase[a].X;
		Amter[a].Rural = AmtBase[a].Y;
		Amter[a].Population = Amter[a].Urban + Amter[a].Rural;
	}
	// Each amt's growth differs from the historical trend by up to the deviation (and a half again).
	AmtGrowthMul.Reset();
	for (int32 a = 0; a < Amter.Num(); ++a)
	{
		AmtGrowthMul.Add(FMath::Max(0.1f, 1.f + 1.5f * Deviation * SeedJitter(Seed, 1000u + uint32(Amter[a].Id))));
	}
	// The nations: their governments' priorities vary with the seed.
	Nations = NationsAtStart;
	for (int32 n = 0; n < Nations.Num(); ++n)
	{
		FCampaign1851Nation& N = Nations[n];
		for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
		{
			N.Weights[p] = FMath::Max(0.1f, N.BaseWeights[p] * (1.f + 1.5f * Deviation * SeedJitter(Seed, 2000u + uint32(n * 16 + p))));
		}
		N.Caution = FMath::Clamp(N.Caution + Deviation * SeedJitter(Seed, 2500u + uint32(n)), 0.05f, 0.95f);
		N.GrowthMul = FMath::Max(0.2f, 1.f + Deviation * SeedJitter(Seed, 2600u + uint32(n)));
		N.Treasury = N.bOnMap ? 0.0 : N.Population * N.TaxPerHead * 0.5;
		N.Industry = 1.0;
	}
	// Railways still being built in 1851 open earlier or later (up to about eight months at 35 %).
	for (FCampaign1851Railway& R : Railways)
	{
		if (R.Link != INDEX_NONE)
		{
			continue;
		}
		if (!RailwayBaseDates.Contains(R.Name))
		{
			RailwayBaseDates.Add(R.Name, TPair<FDateTime, FDateTime>(R.Begun, R.Opened));
		}
		const TPair<FDateTime, FDateTime>& Base = RailwayBaseDates[R.Name];
		R.Begun = Base.Key;
		R.Opened = Base.Value;
		if (Base.Value > StartDate() + FTimespan::FromDays(30.0))
		{
			const double Shift = FMath::RoundToDouble(Deviation * 700.0 * SeedJitter(Seed, GetTypeHash(R.Name)));
			R.Opened = FMath::Max(StartDate() + FTimespan::FromDays(30.0), Base.Value + FTimespan::FromDays(Shift));
		}
	}
	ResetManpower();
	ResetWar();
	ResetDiplomacy();
	ResetResearch();
	ResetNavy();
	ResetPolitics();
	RestoreEconomy(TArray<FString>());
	ResetFortProgrammes();
	ResetResources();
	bEndPending = bEndShown = false;
	ApplyNewGameNation();
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|world|seed %d|deviation %.0f %%"), Seed, Deviation * 100.f);
}

// ------------------------------------------------------------------ growth

float ACampaign1851Map::UrbanGrowthRate(int32 CityIndex) const
{
	if (!Cities.IsValidIndex(CityIndex) || Cities[CityIndex].bForeign)
	{
		return 0.f;
	}
	const FCampaign1851City& City = Cities[CityIndex];
	const int32 AmtIndex = Amter.IndexOfByPredicate([&City](const FCampaign1851Amt& A) { return A.Id == City.AmtId; });
	float Rate = Campaign1851Nations::BaseUrbanGrowth * (AmtGrowthMul.IsValidIndex(AmtIndex) ? AmtGrowthMul[AmtIndex] : 1.f);
	if (HasStation(CityIndex))
	{
		Rate += Campaign1851Nations::StationGrowth;
	}
	for (int32 l : LinksOf(CityIndex))
	{
		if (Links[l].bChaussee)
		{
			Rate += Campaign1851Nations::ChausseeGrowth;
			break;
		}
	}
	// Trade treaties quicken the trading towns.
	if (City.Population >= 4000)
	{
		Rate += 0.05f * TradeTreaties();
	}
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && !Site->IsGarrison() && !Site->IsHistoric() && Site->GetCityIndex() == CityIndex && Site->IsModuleDone(0))
		{
			if (const FCampaign1851CivilEffect* E = Campaign1851Nations::CivilEffect(Site->GetKind()))
			{
				Rate += E->UrbanGrowth;
			}
		}
	}
	return Rate;
}

float ACampaign1851Map::RuralGrowthRate(int32 AmtIndex) const
{
	if (!Amter.IsValidIndex(AmtIndex))
	{
		return 0.f;
	}
	float Rate = Campaign1851Nations::BaseRuralGrowth * (AmtGrowthMul.IsValidIndex(AmtIndex) ? AmtGrowthMul[AmtIndex] : 1.f);
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && !Site->IsGarrison() && !Site->IsHistoric() && Site->IsModuleDone(0) && Cities.IsValidIndex(Site->GetCityIndex()) && Cities[Site->GetCityIndex()].AmtId == Amter[AmtIndex].Id)
		{
			if (const FCampaign1851CivilEffect* E = Campaign1851Nations::CivilEffect(Site->GetKind()))
			{
				Rate += E->RuralGrowth;
			}
		}
	}
	return Rate;
}

void ACampaign1851Map::GrowMonth()
{
	// Towns and countryside grow a twelfth of their yearly rate; the amt's town figures follow its towns.
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		FCampaign1851City& City = Cities[c];
		if (City.bForeign)
		{
			continue;
		}
		const int32 Before = City.Population;
		City.Population = FMath::RoundToInt(City.Population * FMath::Pow(1.0 + UrbanGrowthRate(c) / 100.0, 1.0 / 12.0));
		FCampaign1851Amt* Amt = Amter.FindByPredicate([&City](const FCampaign1851Amt& A) { return A.Id == City.AmtId; });
		if (Amt)
		{
			Amt->Urban += City.Population - Before;
		}
	}
	for (int32 a = 0; a < Amter.Num(); ++a)
	{
		FCampaign1851Amt& A = Amter[a];
		A.Rural = FMath::RoundToInt(A.Rural * FMath::Pow(1.0 + RuralGrowthRate(a) / 100.0, 1.0 / 12.0));
		A.Population = A.Urban + A.Rural;
	}
	// The abstract nations grow on their own model.
	for (FCampaign1851Nation& N : Nations)
	{
		if (!N.bOnMap)
		{
			N.Population *= FMath::Pow(1.0 + (N.BaseGrowth * N.GrowthMul + (N.Industry - 1.0) * 0.5) / 100.0, 1.0 / 12.0);
		}
	}
}

double ACampaign1851Map::CivilIncomePerYear() const
{
	double Total = 0.0;
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && !Site->IsGarrison() && !Site->IsHistoric() && Site->IsModuleDone(0))
		{
			if (const FCampaign1851CivilEffect* E = Campaign1851Nations::CivilEffect(Site->GetKind()))
			{
				Total += E->IncomeRd;
			}
		}
	}
	return Total;
}

void ACampaign1851Map::PrivateInvestment()
{
	// Merchants and manufacturers build where towns grow and trains stop: no cost to the state.
	FRandomStream Rng(int32(HashCombine(uint32(Seed), uint32(FMath::FloorToInt(CampaignDays)))));
	const TArray<FCampaign1851SiteModule>& Types = ACampaign1851ConstructionSite::TownBuildings();
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		const FCampaign1851City& City = Cities[c];
		if (City.bForeign || City.bBornholm || City.Population < 1500)
		{
			continue;
		}
		const float Chance = 0.006f * FMath::Sqrt(City.Population / 5000.f) * (HasStation(c) ? 2.f : 1.f) * FMath::Max(0.2f, UrbanGrowthRate(c) / 1.4f);
		if (Rng.FRand() > Chance)
		{
			continue;
		}
		TArray<FString> Options;
		for (const FCampaign1851SiteModule& T : Types)
		{
			const FCampaign1851CivilEffect* E = Campaign1851Nations::CivilEffect(T.Key);
			if (E && E->bPrivate && !FindBuilding(c, T.Key) && BuildingBlockReason(c, T.Key).IsEmpty())
			{
				Options.Add(T.Key);
			}
		}
		if (Options.Num() == 0)
		{
			continue;
		}
		const FString Key = Options[Rng.RandHelper(Options.Num())];
		if (ACampaign1851ConstructionSite* Site = StartBuilding(c, Key, false))
		{
			Site->SetPrivate(true);
			const FCampaign1851SiteModule* Def = ACampaign1851ConstructionSite::FindTownBuilding(Key);
			News.Add(FString::Printf(TEXT("Private investorer bygger %s i %s"), Def ? *Def->Name.ToLower() : *Key, *City.Name));
			FCampaign1851Decision D;
			D.Day = CampaignDays;
			D.Nation = PlayerNation;
			D.Portfolio = ECampaign1851Portfolio::Interior;
			D.Action = FString::Printf(TEXT("Privat: %s i %s"), Def ? *Def->Name : *Key, *City.Name);
			D.Reasons = FString::Printf(TEXT("byen vokser %.1f %% om året%s  ·  ingen udgift for staten"), UrbanGrowthRate(c), HasStation(c) ? TEXT(", jernbanestation") : TEXT(""));
			D.bDone = true;
			AddDecision(D);
		}
	}
}

void ACampaign1851Map::AddDecision(const FCampaign1851Decision& D)
{
	Decisions.Add(D);
	if (Decisions.Num() > 300)
	{
		Decisions.RemoveAt(0, Decisions.Num() - 300);
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|ai|%s|%s|%s|%s|%s"), Nations.IsValidIndex(D.Nation) ? *Nations[D.Nation].Id : TEXT("?"),
		Campaign1851Nations::PortfolioName(D.Portfolio), D.bAdvice ? TEXT("råd") : D.bDone ? TEXT("udført") : TEXT("-"), *D.Action, *D.Reasons);
}

// ------------------------------------------------------------------ the national AI

void ACampaign1851Map::RunNationalAI()
{
	FRandomStream Rng(int32(HashCombine(uint32(Seed) * 31u, uint32(FMath::FloorToInt(CampaignDays)))));
	for (int32 n = 0; n < Nations.Num(); ++n)
	{
		FCampaign1851Nation& N = Nations[n];
		if (!N.bOnMap)
		{
			RunAbstractNation(n);
			continue;
		}
		// The money the ministries may commit this month: part of what lies above the reserve.
		const double Surplus = FMath::Max(0.0, Treasury - N.Reserve);
		float WeightSum = 0.f;
		for (float W : N.Weights) { WeightSum += W; }
		for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
		{
			const ECampaign1851Portfolio P = ECampaign1851Portfolio(p);
			const ECampaign1851Delegation Mode = N.Mode(P);
			if (Mode == ECampaign1851Delegation::Manual)
			{
				continue;
			}
			// Eight portfolios share it (the minister's skill and thrift move his share).
			const double Budget = Surplus * 0.35 * (1.0 - 0.6 * N.Caution) * N.Weights[p] / FMath::Max(WeightSum, 0.1f) * 6.4 * (n == PlayerNation ? MinisterBudgetFactor(P) : 1.f);
			TArray<FCampaign1851Decision> Options = DecisionOptions(n, P, Budget, Rng);
			Options.Sort([](const FCampaign1851Decision& A, const FCampaign1851Decision& B) { return A.Score > B.Score; });
			// A ministry acts on its best few options a month (the War ministry fills several posts at once).
			const int32 Take = P == ECampaign1851Portfolio::War ? 6 : 1;
			for (int32 i = 0; i < Options.Num() && i < Take; ++i)
			{
				FCampaign1851Decision D = Options[i];
				D.Day = CampaignDays;
				D.Nation = n;
				D.Portfolio = P;
				if (Mode == ECampaign1851Delegation::Auto)
				{
					D.bDone = CarryOut(D);
					if (D.bDone)
					{
						AddDecision(D);
					}
				}
				else
				{
					// An advice is given once; the same advice is not repeated while it stands.
					const bool bStanding = Decisions.ContainsByPredicate([&D](const FCampaign1851Decision& X) { return X.bAdvice && !X.bDone && X.Action == D.Action; });
					if (!bStanding)
					{
						D.bAdvice = true;
						AddDecision(D);
					}
				}
			}
		}
	}
}

TArray<FCampaign1851Decision> ACampaign1851Map::DecisionOptions(int32 NationIndex, ECampaign1851Portfolio P, double Budget, FRandomStream& Rng) const
{
	const FCampaign1851Nation& N = Nations[NationIndex];
	const float W = N.Weights[int32(P)];
	auto Noise = [&]() { return 1.f + Deviation * Rng.FRandRange(-1.f, 1.f); };
	TArray<FCampaign1851Decision> Out;
	const double Spendable = Treasury - N.Reserve;
	if (P == ECampaign1851Portfolio::Interior)
	{
		// Civil buildings: yearly return (income, and taxes from the growth they bring) on the price.
		int32 Running = 0;
		for (const ACampaign1851ConstructionSite* Site : Projects)
		{
			Running += Site && !Site->IsGarrison() && !Site->IsPrivate() && !Site->IsModuleDone(0) && Campaign1851Nations::CivilEffect(Site->GetKind()) ? 1 : 0;
		}
		if (Running >= 3)
		{
			return Out;
		}
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			const FCampaign1851City& City = Cities[c];
			const FCampaign1851Amt* Amt = FindAmt(City.AmtId);
			for (const FCampaign1851SiteModule& T : ACampaign1851ConstructionSite::TownBuildings())
			{
				const FCampaign1851CivilEffect* E = Campaign1851Nations::CivilEffect(T.Key);
				if (!E || FindBuilding(c, T.Key) || !BuildingBlockReason(c, T.Key).IsEmpty())
				{
					continue;
				}
				const double Cost = T.Cost();
				if (Cost > Budget || Cost * Campaign1851Buildings::DownPayment > Spendable)
				{
					continue;
				}
				const double Yearly = E->IncomeRd + E->UrbanGrowth / 100.0 * City.Population * UrbanTaxPerHead * 6.0
					+ (Amt ? E->RuralGrowth / 100.0 * Amt->Rural * RuralTaxPerHead * 6.0 : 0.0);
				FCampaign1851Decision D;
				D.Kind = ECampaign1851DecisionKind::CivilBuilding;
				D.A = c;
				D.Key = T.Key;
				D.Cost = Cost;
				D.Score = float(Yearly / Cost) * W * Noise();
				D.Action = FString::Printf(TEXT("Byg %s i %s"), *T.Name.ToLower(), *City.Name);
				D.Reasons = FString::Printf(TEXT("+%.2f %% vækst om året i byen (%s indb.)%s  ·  afkast %.1f %% om året  ·  pris %s rd."),
					E->UrbanGrowth, *Rd(City.Population), E->IncomeRd > 0 ? *FString::Printf(TEXT("  ·  +%s rd./år i indtægt"), *Rd(E->IncomeRd)) : TEXT(""),
					100.0 * Yearly / Cost, *Rd(Cost));
				Out.Add(D);
			}
		}
	}
	else if (P == ECampaign1851Portfolio::PublicWorks)
	{
		int32 Running = 0;
		for (const FCampaign1851Link& L : Links) { Running += L.Work != ECampaign1851LinkWork::None ? 1 : 0; }
		if (Running >= 2)
		{
			return Out;
		}
		for (int32 l = 0; l < Links.Num(); ++l)
		{
			const FCampaign1851Link& L = Links[l];
			if (!Cities.IsValidIndex(L.A) || !Cities.IsValidIndex(L.B))
			{
				continue;
			}
			for (const ECampaign1851LinkWork Work : { ECampaign1851LinkWork::Chaussee, ECampaign1851LinkWork::Railway })
			{
				if (!LinkBlockReason(l, Work).IsEmpty())
				{
					continue;
				}
				const double Cost = LinkWorkCost(l, Work);
				if (Cost <= 0.0 || Cost > Budget || Cost * Campaign1851Buildings::DownPayment > Spendable)
				{
					continue;
				}
				const bool bRail = Work == ECampaign1851LinkWork::Railway;
				const double Pop = double(Cities[L.A].Population + Cities[L.B].Population);
				// Trade and taxes along the line, and the growth a station or paved road gives both towns.
				const double Yearly = Pop * (bRail ? 0.05 : 0.02) + Pop * (bRail ? Campaign1851Nations::StationGrowth : Campaign1851Nations::ChausseeGrowth) / 100.0 * UrbanTaxPerHead * 6.0;
				FCampaign1851Decision D;
				D.Kind = ECampaign1851DecisionKind::LinkWork;
				D.A = l;
				D.B = int32(Work);
				D.Cost = Cost;
				D.Score = float(Yearly / Cost) * W * Noise();
				D.Action = FString::Printf(TEXT("%s %s - %s"), bRail ? TEXT("Anlæg jernbane") : TEXT("Byg chaussé"), *Cities[L.A].Name, *Cities[L.B].Name);
				D.Reasons = FString::Printf(TEXT("forbinder %s indb.  ·  %.0f km  ·  afkast %.1f %% om året  ·  pris %s rd."),
					*Rd(Pop), bRail ? L.RailKm : L.RoadKm, 100.0 * Yearly / Cost, *Rd(Cost));
				Out.Add(D);
			}
		}
	}
	else if (P == ECampaign1851Portfolio::War)
	{
		// Empty posts first: free officers go where they are missing (no cost), else hire one.
		TArray<int32> Free = OfficerPool(false);
		int32 Vacancies = 0;
		auto Fill = [&](const FString& Post, int32 Code, const FString& Target, float Urgency)
		{
			++Vacancies;
			if (Free.Num() == 0)
			{
				return;
			}
			const int32 O = Free[0];   // the pool is sorted best first
			Free.RemoveAt(0);
			FCampaign1851Decision D;
			D.Kind = ECampaign1851DecisionKind::FillPost;
			D.A = O;
			D.B = Code;
			D.Key = Target;
			D.Score = Urgency * W;
			D.Action = FString::Printf(TEXT("Udnævn %s %s til %s"), *Officers[O].Rank, *Officers[O].Name, *Post);
			D.Reasons = FString::Printf(TEXT("posten er ubesat  ·  føring %d, erfaring %.0f"), Officers[O].Stat(ECampaign1851OfficerStat::Leadership), Officers[O].Experience);
			Out.Add(D);
		};
		for (int32 i = 0; i < Regiments.Num(); ++i)
		{
			if (!Officers.IsValidIndex(Regiments[i].Chief))
			{
				Fill(FString::Printf(TEXT("chef for %s"), *Regiments[i].Name), 0, FString::FromInt(i), 3.f);
			}
			for (int32 k = 0; k < Regiments[i].Captains.Num(); ++k)
			{
				if (!Officers.IsValidIndex(Regiments[i].Captains[k]))
				{
					Fill(FString::Printf(TEXT("kaptajn for %d. Kompagni (%s)"), CompanyNumber(i, k), *Regiments[i].Name), 1, FString::Printf(TEXT("%d:%d"), i, k), 1.f);
				}
			}
		}
		for (const FCampaign1851Formation& F : Formations)
		{
			if (!Officers.IsValidIndex(F.Deputy))
			{
				Fill(FString::Printf(TEXT("næstkommanderende i %s"), *F.Name), 2, FString::Printf(TEXT("%d:1"), F.Id), 2.f);
			}
			if (!Officers.IsValidIndex(F.StaffChief))
			{
				Fill(FString::Printf(TEXT("%s i %s"), *FString(Campaign1851Army::StaffPostName(F.Echelon, 2)).ToLower(), *F.Name), 2, FString::Printf(TEXT("%d:2"), F.Id), 1.5f);
			}
		}
		if (Vacancies > 0 && Free.Num() == 0 && OfficerRecruitCost <= Spendable)
		{
			FCampaign1851Decision D;
			D.Kind = ECampaign1851DecisionKind::Recruit;
			D.Cost = OfficerRecruitCost;
			D.Score = 0.5f * W;
			D.Action = TEXT("Ansæt en ny officer");
			D.Reasons = FString::Printf(TEXT("%d ubesatte poster og ingen ledige officerer  ·  pris %s rd."), Vacancies, *Rd(OfficerRecruitCost));
			Out.Add(D);
		}
		// A new battalion where a barracks stands and the amt has the men, while the army is small for the kingdom.
		int32 ArmyMen = 0;
		for (const FCampaign1851Regiment& R : Regiments) { ArmyMen += R.Men; }
		double People = 0.0;
		for (const FCampaign1851Amt& A : Amter) { People += A.Population; }
		if (ArmyMen < People * 0.009 && Campaign1851Army::RaiseCost() <= Budget)
		{
			for (int32 c = 0; c < Cities.Num(); ++c)
			{
				if (RaiseBlockReason(c).IsEmpty())
				{
					FCampaign1851Decision D;
					D.Kind = ECampaign1851DecisionKind::Raise;
					D.A = c;
					D.Cost = Campaign1851Army::RaiseCost();
					D.Score = 1.2f * W * float(1.0 - ArmyMen / (People * 0.009)) * 4.f;
					D.Action = FString::Printf(TEXT("Opret en ny bataljon i %s"), *Cities[c].Name);
					D.Reasons = FString::Printf(TEXT("hæren %s mand er lille for %s indbyggere  ·  amtet har %d mand i reserve  ·  pris %s rd. + %s rd./md."),
						*Rd(ArmyMen), *Rd(People), FMath::FloorToInt(GetManpower(AmtIndexOfTown(c))), *Rd(Campaign1851Army::RaiseCost()), *Rd(Campaign1851Army::RaisedUpkeepPerMonth));
					Out.Add(D);
					break;
				}
			}
		}
		// Training: units drilling by habit are set to train their weakest skill, as far as the budget allows;
		// all training together may take at most a share of the monthly taxes (by the ministry's weight).
		double Monthly = 0.0;
		for (const FCampaign1851Regiment& R : Regiments)
		{
			Monthly += R.IsMarching() ? 0.0 : Campaign1851Army::ProgramCostPerMonth(R.Program) * R.Men / 760.0;
		}
		const double TrainingCap = FMath::Min(Budget / 12.0, YearlyTax() / 12.0 * 0.25 * W);
		for (int32 i = 0; i < Regiments.Num(); ++i)
		{
			const FCampaign1851Regiment& R = Regiments[i];
			if (R.IsMarching() || R.Program != ECampaign1851Program::Drill)
			{
				continue;
			}
			int32 Weakest = 0;
			for (int32 s = 1; s < int32(ECampaign1851Skill::Count); ++s)
			{
				Weakest = R.Skills[s] < R.Skills[Weakest] ? s : Weakest;
			}
			if (R.Skills[Weakest] >= 55.f)
			{
				continue;
			}
			const ECampaign1851Skill S = ECampaign1851Skill(Weakest);
			const ECampaign1851Program Program = S == ECampaign1851Skill::Loading || S == ECampaign1851Skill::Marksmanship ? ECampaign1851Program::LiveFire
				: S == ECampaign1851Skill::Fieldcraft ? ECampaign1851Program::Field : S == ECampaign1851Skill::Endurance ? ECampaign1851Program::March
				: S == ECampaign1851Skill::Assault ? ECampaign1851Program::Assault : ECampaign1851Program::Drill;
			if (Program == ECampaign1851Program::Drill)
			{
				continue;
			}
			const double Cost = Campaign1851Army::ProgramCostPerMonth(Program) * R.Men / 760.0;
			if (Monthly + Cost > TrainingCap)
			{
				continue;
			}
			Monthly += Cost;
			FCampaign1851Decision D;
			D.Kind = ECampaign1851DecisionKind::Training;
			D.A = i;
			D.B = int32(Program);
			D.Cost = Cost;
			D.Score = (60.f - R.Skills[Weakest]) / 20.f * W * Noise();
			D.Action = FString::Printf(TEXT("%s: %s"), *R.Name, Campaign1851Army::ProgramName(Program));
			D.Reasons = FString::Printf(TEXT("svageste evne %s %.0f  ·  %s rd. om måneden"), Campaign1851Army::SkillName(S), R.Skills[Weakest], *Rd(Cost));
			Out.Add(D);
		}
	}
	else if (P == ECampaign1851Portfolio::Intendance)
	{
		// More columns when units in the field outnumber the free ones.
		int32 InField = 0;
		for (const FCampaign1851Regiment& R : Regiments)
		{
			InField += (R.IsMarching() || R.IsInField()) && DepotFor(R.Km) == INDEX_NONE ? 1 : 0;
		}
		if (InField > FreeSupplyColumns() && Campaign1851Supply::ColumnCost <= Spendable && Campaign1851Supply::ColumnCost <= Budget)
		{
			FCampaign1851Decision D;
			D.Kind = ECampaign1851DecisionKind::SupplyColumn;
			D.Cost = Campaign1851Supply::ColumnCost;
			D.Score = float(InField - FreeSupplyColumns()) * W * Noise();
			D.Action = TEXT("Køb en trænkolonne");
			D.Reasons = FString::Printf(TEXT("%d enheder i felten uden depot, %d ledige kolonner  ·  pris %s rd."), InField, FreeSupplyColumns(), *Rd(Campaign1851Supply::ColumnCost));
			Out.Add(D);
		}
		// A grain store where many regiments stand without a depot near (their garrisons and the ground they cover).
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			if (!BuildingBlockReason(c, TEXT("Grain_Warehouse")).IsEmpty() || FindBuilding(c, TEXT("Grain_Warehouse")))
			{
				continue;
			}
			const FCampaign1851DepotCapacity Cap = DepotCapacity(c);
			if (Cap.Food > 0.f)
			{
				continue;
			}
			int32 Near = 0;
			for (const FCampaign1851Regiment& R : Regiments)
			{
				Near += FVector2D::Distance(R.Km, TownKm(c)) < 40.0 && DepotFor(R.Km) == INDEX_NONE ? 1 : 0;
			}
			const FCampaign1851SiteModule* Def = ACampaign1851ConstructionSite::FindTownBuilding(TEXT("Grain_Warehouse"));
			if (Near < 2 || !Def || Def->Cost() > Budget || Def->Cost() * Campaign1851Buildings::DownPayment > Spendable)
			{
				continue;
			}
			FCampaign1851Decision D;
			D.Kind = ECampaign1851DecisionKind::CivilBuilding;
			D.A = c;
			D.Key = TEXT("Grain_Warehouse");
			D.Cost = Def->Cost();
			D.Score = float(Near) * 0.3f * W * Noise();
			D.Action = FString::Printf(TEXT("Byg kornmagasin (depot) i %s"), *Cities[c].Name);
			D.Reasons = FString::Printf(TEXT("%d enheder inden for 40 km uden depot  ·  60.000 rationer og foder  ·  pris %s rd."), Near, *Rd(D.Cost));
			Out.Add(D);
		}
	}
	else if (P == ECampaign1851Portfolio::Transport)
	{
		// Enough trains to move a quarter of the army at once (a train takes a battalion).
		int32 Ordered = 0;
		for (const FVector2D& O : TrainOrders) { Ordered += int32(O.X); }
		const int32 Need = FMath::CeilToInt(Regiments.Num() / 4.f);
		if (GetTroopTrains() + Ordered < Need && TroopTrainCost <= Budget && TroopTrainCost <= Spendable)
		{
			FCampaign1851Decision D;
			D.Kind = ECampaign1851DecisionKind::TroopTrain;
			D.Cost = TroopTrainCost;
			D.Score = float(Need - GetTroopTrains() - Ordered) * W * Noise();
			D.Action = TEXT("Bestil et troppetog");
			D.Reasons = FString::Printf(TEXT("%d tog og %d i ordre, %d ønskes til hæren  ·  pris %s rd."), GetTroopTrains(), Ordered, Need, *Rd(TroopTrainCost));
			Out.Add(D);
		}
	}
	if (NationIndex == PlayerNation)
	{
		Out.Append(MinisterOptions(P, Budget));
	}
	return Out;
}

bool ACampaign1851Map::CarryOut(const FCampaign1851Decision& D)
{
	switch (D.Kind)
	{
	case ECampaign1851DecisionKind::CivilBuilding:
		return StartBuilding(D.A, D.Key, true) != nullptr;
	case ECampaign1851DecisionKind::LinkWork:
		return StartLinkWork(D.A, ECampaign1851LinkWork(D.B), true);
	case ECampaign1851DecisionKind::TroopTrain:
		return OrderTroopTrain();
	case ECampaign1851DecisionKind::SupplyColumn:
		return BuySupplyColumn();
	case ECampaign1851DecisionKind::Training:
		SetProgram(D.A, ECampaign1851Program(D.B));
		return Regiments.IsValidIndex(D.A);
	case ECampaign1851DecisionKind::Recruit:
		return RecruitOfficer(false) != INDEX_NONE;
	case ECampaign1851DecisionKind::Raise:
		return RaiseBattalion(D.A) != INDEX_NONE;
	case ECampaign1851DecisionKind::FortProgramme:
		return BuildProgramme(D.A);
	case ECampaign1851DecisionKind::Diplomacy:
	case ECampaign1851DecisionKind::Peace:
	case ECampaign1851DecisionKind::Ship:
	case ECampaign1851DecisionKind::Blockade:
	case ECampaign1851DecisionKind::Loan:
	case ECampaign1851DecisionKind::Doctrine:
	case ECampaign1851DecisionKind::BuyRaw:
		return CarryOutMinister(D);
	case ECampaign1851DecisionKind::FillPost:
	{
		if (!Officers.IsValidIndex(D.A) || !Officers[D.A].IsFree())
		{
			return false;
		}
		FString Left, Right;
		D.Key.Split(TEXT(":"), &Left, &Right);
		if (D.B == 0)
		{
			return AssignOfficer(D.A, FCString::Atoi(*D.Key));
		}
		if (D.B == 1)
		{
			// A captain for a company: he takes the empty company of that battalion.
			const int32 R = FCString::Atoi(*Left), K = FCString::Atoi(*Right);
			if (!Regiments.IsValidIndex(R) || !Regiments[R].Captains.IsValidIndex(K) || Officers.IsValidIndex(Regiments[R].Captains[K]))
			{
				return false;
			}
			Regiments[R].Captains[K] = D.A;
			Officers[D.A].CaptainOf = R;
			Officers[D.A].Company = K;
			return true;
		}
		return AssignFormationStaff(D.A, FCString::Atoi(*Left), FCString::Atoi(*Right));
	}
	default:
		return false;
	}
}

bool ACampaign1851Map::ExecuteDecision(int32 Index)
{
	if (!Decisions.IsValidIndex(Index) || Decisions[Index].bDone || !Decisions[Index].bAdvice)
	{
		return false;
	}
	Decisions[Index].bDone = CarryOut(Decisions[Index]);
	return Decisions[Index].bDone;
}

void ACampaign1851Map::RunAbstractNation(int32 NationIndex)
{
	// A nation without a map: its development budget comes from its people, its ministries spend it
	// by their priorities on railways, the army and industry. Milestones go into the decision log.
	FCampaign1851Nation& N = Nations[NationIndex];
	N.Treasury += N.Population * N.TaxPerHead * (0.9 + 0.1 * N.Industry) / 12.0;
	const double Spend = FMath::Max(0.0, N.Treasury - N.Reserve) * 0.5 * (1.0 - 0.5 * N.Caution);
	// The abstract model spends on the first five portfolios (the others have nothing to buy there).
	float WeightSum = 0.f;
	for (int32 p = 0; p <= int32(ECampaign1851Portfolio::Intendance); ++p) { WeightSum += N.Weights[p]; }
	auto Share = [&](ECampaign1851Portfolio P) { return Spend * N.Weights[int32(P)] / FMath::Max(WeightSum, 0.1f); };
	const double RailBefore = N.RailKm, ArmyBefore = N.ArmyMen;
	N.RailKm += Share(ECampaign1851Portfolio::PublicWorks) / 14000.0 + Share(ECampaign1851Portfolio::Transport) / 40000.0;
	N.ArmyMen = FMath::Min(N.ArmyMen + Share(ECampaign1851Portfolio::War) / 90.0, N.Population * 0.015);
	N.Industry += Share(ECampaign1851Portfolio::Interior) / FMath::Max(N.Population * 3.0, 1.0);
	N.Treasury -= Spend;
	auto Milestone = [&](double Before, double After, double Step, ECampaign1851Portfolio P, const TCHAR* What)
	{
		if (FMath::FloorToDouble(After / Step) > FMath::FloorToDouble(Before / Step))
		{
			FCampaign1851Decision D;
			D.Day = CampaignDays;
			D.Nation = NationIndex;
			D.Portfolio = P;
			D.bDone = true;
			D.Action = FString::Printf(TEXT("%s: %s %s"), *N.Name, What, *Rd(FMath::FloorToDouble(After / Step) * Step));
			D.Reasons = FString::Printf(TEXT("abstrakt model (landet har endnu intet kort)  ·  befolkning %s  ·  industri %.2f"), *Rd(N.Population), N.Industry);
			AddDecision(D);
		}
	};
	Milestone(RailBefore, N.RailKm, 100.0, ECampaign1851Portfolio::PublicWorks, TEXT("jernbanenettet når km"));
	Milestone(ArmyBefore, N.ArmyMen, 10000.0, ECampaign1851Portfolio::War, TEXT("hæren tæller nu mand"));
}

// ------------------------------------------------------------------ figures for the council window

FCampaign1851NationFigures ACampaign1851Map::NationFigures(int32 NationIndex) const
{
	FCampaign1851NationFigures F;
	if (!Nations.IsValidIndex(NationIndex))
	{
		return F;
	}
	const FCampaign1851Nation& N = Nations[NationIndex];
	if (!N.bOnMap)
	{
		F.Population = N.Population;
		F.YearlyBudget = N.Population * N.TaxPerHead * (0.9 + 0.1 * N.Industry);
		F.ArmyMen = N.ArmyMen;
		F.RailKm = N.RailKm;
		F.Growth = N.BaseGrowth * N.GrowthMul + float(N.Industry - 1.0) * 0.5f;
		return F;
	}
	double Weighted = 0.0;
	for (int32 a = 0; a < Amter.Num(); ++a)
	{
		F.Population += Amter[a].Population;
		Weighted += Amter[a].Rural * RuralGrowthRate(a);
	}
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		Weighted += Cities[c].bForeign ? 0.0 : Cities[c].Population * UrbanGrowthRate(c);
	}
	F.Growth = F.Population > 0.0 ? float(Weighted / F.Population) : 0.f;
	F.YearlyBudget = YearlyTax() + CivilIncomePerYear();
	for (const FCampaign1851Regiment& R : Regiments) { F.ArmyMen += R.Men; }
	const FDateTime Now = GetDate();
	for (const FCampaign1851Railway& R : Railways) { F.RailKm += R.IsOpen(Now) ? R.LengthKm : 0.f; }
	return F;
}

// ------------------------------------------------------------------ save

void ACampaign1851Map::SaveWorld(UCampaign1851SaveGame* Save) const
{
	Save->Seed = Seed;
	Save->Deviation = Deviation;
	Save->CityPopulation.Reset();
	for (const FCampaign1851City& C : Cities) { Save->CityPopulation.Add(C.Population); }
	Save->AmtUrban.Reset();
	Save->AmtRural.Reset();
	for (const FCampaign1851Amt& A : Amter) { Save->AmtUrban.Add(A.Urban); Save->AmtRural.Add(A.Rural); }
	Save->Nations.Reset();
	for (const FCampaign1851Nation& N : Nations)
	{
		FCampaign1851NationSave& S = Save->Nations.AddDefaulted_GetRef();
		S.Id = N.Id;
		S.bPlayer = N.IsPlayer();
		for (ECampaign1851Delegation M : N.Modes) { S.Modes.Add(uint8(M)); }
		S.Reserve = N.Reserve;
		S.Population = N.Population;
		S.Treasury = N.Treasury;
		S.RailKm = N.RailKm;
		S.ArmyMen = N.ArmyMen;
		S.Industry = N.Industry;
	}
	Save->Decisions.Reset();
	for (const FCampaign1851Decision& D : Decisions)
	{
		FCampaign1851DecisionSave& S = Save->Decisions.AddDefaulted_GetRef();
		S.Day = D.Day;
		S.Nation = D.Nation;
		S.Portfolio = uint8(D.Portfolio);
		S.Kind = uint8(D.Kind);
		S.Action = D.Action;
		S.Reasons = D.Reasons;
		S.Cost = D.Cost;
		S.bDone = D.bDone;
		S.bAdvice = D.bAdvice;
		S.A = D.A;
		S.B = D.B;
		S.Key = D.Key;
	}
}

void ACampaign1851Map::RestoreWorld(const UCampaign1851SaveGame* Save)
{
	for (int32 c = 0; c < Cities.Num() && c < Save->CityPopulation.Num(); ++c) { Cities[c].Population = Save->CityPopulation[c]; }
	for (int32 a = 0; a < Amter.Num() && a < Save->AmtUrban.Num() && a < Save->AmtRural.Num(); ++a)
	{
		Amter[a].Urban = Save->AmtUrban[a];
		Amter[a].Rural = Save->AmtRural[a];
		Amter[a].Population = Amter[a].Urban + Amter[a].Rural;
	}
	for (const FCampaign1851NationSave& S : Save->Nations)
	{
		FCampaign1851Nation* N = Nations.FindByPredicate([&S](const FCampaign1851Nation& X) { return X.Id == S.Id; });
		if (!N)
		{
			continue;
		}
		N->Controller = S.bPlayer ? ECampaign1851Controller::Player : ECampaign1851Controller::AI;
		for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count) && p < S.Modes.Num(); ++p) { N->Modes[p] = ECampaign1851Delegation(S.Modes[p]); }
		N->Reserve = S.Reserve;
		if (!N->bOnMap)
		{
			N->Population = S.Population;
			N->Treasury = S.Treasury;
			N->RailKm = S.RailKm;
			N->ArmyMen = S.ArmyMen;
			N->Industry = S.Industry;
		}
	}
	Decisions.Reset();
	for (const FCampaign1851DecisionSave& S : Save->Decisions)
	{
		FCampaign1851Decision D;
		D.Day = S.Day;
		D.Nation = S.Nation;
		D.Portfolio = ECampaign1851Portfolio(FMath::Min<uint8>(S.Portfolio, uint8(ECampaign1851Portfolio::Count) - 1));
		D.Kind = ECampaign1851DecisionKind(S.Kind);
		D.Action = S.Action;
		D.Reasons = S.Reasons;
		D.Cost = S.Cost;
		D.bDone = S.bDone;
		D.bAdvice = S.bAdvice;
		D.A = S.A;
		D.B = S.B;
		D.Key = S.Key;
		Decisions.Add(D);
	}
}

// ------------------------------------------------------------------ the towns of 1851

void ACampaign1851Map::SeedHistoricBuildings()
{
	// Rules by the size and place of each town (estimates), and the known industry of the big towns; the
	// campaign's deviation leaves one out here and there. They stand finished, as part of the 1851 town.
	FRandomStream Rng(int32(HashCombine(uint32(Seed), 0x1851u)));
	auto Keep = [&]() { return Rng.FRand() >= Deviation * 0.5f; };
	struct FRule { const TCHAR* Key; int32 MinPop; bool bCoast; };
	static const FRule Rules[] = {
		{ TEXT("Schoolhouse"), 0, false },          // every town had its borgerskole
		{ TEXT("Town_Hall"), 1000, false },         // every købstad its rådhus
		{ TEXT("Merchant_House"), 1500, false },
		{ TEXT("Post_Office"), 2000, false },
		{ TEXT("Harbor_Building"), 2000, true },    // the customs house of a port town
		{ TEXT("Brewery"), 4000, false },
		{ TEXT("Brickworks"), 6000, false },
		{ TEXT("Hospital"), 9000, false },
	};
	struct FNamed { const TCHAR* Town; const TCHAR* Key; };
	static const FNamed Named[] = {
		{ TEXT("København"), TEXT("Machine_Workshop") }, { TEXT("København"), TEXT("Textile_Mill") },
		{ TEXT("Odense"), TEXT("Machine_Workshop") },     { TEXT("Flensborg"), TEXT("Machine_Workshop") },
		{ TEXT("Neumünster"), TEXT("Textile_Mill") },     { TEXT("Flensborg"), TEXT("Textile_Mill") },
		{ TEXT("Altona"), TEXT("Machine_Workshop") },     { TEXT("Skagen"), TEXT("Lighthouse") },
		{ TEXT("København"), TEXT("Arsenal") },           { TEXT("København"), TEXT("Cannon_Foundry") },
		{ TEXT("Helsingør"), TEXT("Rifle_Workshop") },    { TEXT("Randers"), TEXT("Remount_Depot") },
		{ TEXT("Rendsborg"), TEXT("Arsenal") },
		{ TEXT("Helsingør"), TEXT("Lighthouse") },
	};
	int32 Placed = 0;
	auto Place = [&](int32 c, const TCHAR* Key)
	{
		if (!Cities.IsValidIndex(c) || FindBuilding(c, Key))
		{
			return;
		}
		if (ACampaign1851ConstructionSite* Site = StartBuilding(c, Key, false))
		{
			Site->RestoreState({ Site->ModuleDays(0) }, INDEX_NONE);
			Site->SetHistoric(true);
			++Placed;
		}
	};
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		const FCampaign1851City& City = Cities[c];
		if (City.bForeign || City.bBornholm)
		{
			continue;
		}
		for (const FRule& R : Rules)
		{
			if (City.Population >= R.MinPop && (!R.bCoast || IsCoastalTown(c)) && Keep())
			{
				Place(c, R.Key);
			}
		}
	}
	for (const FNamed& N : Named)
	{
		if (Keep())
		{
			Place(FindCity(N.Town), N.Key);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|world|%d historic town buildings"), Placed);
}
