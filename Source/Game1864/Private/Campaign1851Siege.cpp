// The historical fortification works and sieges (Docs/Siege1851.md, backlog items 13 and 14).
// The War Ministry proposes the great works on their historical dates (varied by the campaign, some never
// come): the Dannevirke position (1861), the Dybbøl position (1861) and Fredericia's ramparts (1862). On AUTO
// it builds them when the treasury allows; otherwise they are advice to carry out or leave.
// An enemy corps that finds a position too strong to storm lays siege to it: it digs in out of reach of the
// forts, its batteries wear down the parapets and the powder, and it storms when the works are breached,
// its strength has grown enough, or after five weeks.

#include "Campaign1851Map.h"
#include "Misc/DefaultValueHelper.h"

namespace
{
	struct FWorkFort { double Lat, Lon; bool bLarge; };
	struct FWorkProgramme
	{
		const TCHAR* Name;
		int32 Year, Month;
		float Bearing;   // the front
		TArray<FWorkFort> Forts;
	};
	const TArray<FWorkProgramme>& Programmes()
	{
		// Positions approximate (from the historical lines); the game places each fort where the ground allows.
		static const TArray<FWorkProgramme> List = {
			{ TEXT("Dannevirkestillingen"), 1861, 3, 180.f, {
				{ 54.466, 9.345, false }, { 54.470, 9.390, false }, { 54.474, 9.435, true }, { 54.478, 9.475, true },
				{ 54.482, 9.510, true }, { 54.487, 9.545, true }, { 54.492, 9.575, false }, { 54.500, 9.600, false } } },
			{ TEXT("Dybbølstillingen"), 1861, 6, 270.f, {
				{ 54.900, 9.740, false }, { 54.902, 9.747, true }, { 54.906, 9.749, true }, { 54.910, 9.751, true },
				{ 54.914, 9.753, false }, { 54.918, 9.756, true }, { 54.919, 9.750, false } } },
			{ TEXT("Fredericias volde"), 1862, 5, 250.f, {
				{ 55.578, 9.725, true }, { 55.567, 9.720, true }, { 55.560, 9.712, true } } },
		};
		return List;
	}
}

void ACampaign1851Map::ResetFortProgrammes()
{
	ProgrammeDay.Reset();
	ProgrammeState.Reset();
	FRandomStream Rng(int32(HashCombine(uint32(Seed), 0xF077u)));
	for (const FWorkProgramme& P : Programmes())
	{
		// The date varies by the deviation; a few are never proposed.
		const double Day = (FDateTime(P.Year, P.Month, 1) - StartDate()).GetTotalDays() + Deviation * 730.0 * Rng.FRandRange(-1.f, 1.f);
		ProgrammeDay.Add(FMath::Max(60.0, Day));
		ProgrammeState.Add(Rng.FRand() < Deviation * 0.3f ? 3 : 0);   // 0 waiting, 1 proposed, 2 built, 3 never
	}
}

double ACampaign1851Map::ProgrammeCost(int32 Index) const
{
	double Cost = 0.0;
	if (Programmes().IsValidIndex(Index))
	{
		for (const FWorkFort& F : Programmes()[Index].Forts)
		{
			Cost += Campaign1851Forts::BuildCost(F.bLarge);
		}
	}
	return Cost;
}

bool ACampaign1851Map::BuildProgramme(int32 Index)
{
	if (!Programmes().IsValidIndex(Index) || !ProgrammeState.IsValidIndex(Index) || ProgrammeState[Index] == 2)
	{
		return false;
	}
	const FWorkProgramme& P = Programmes()[Index];
	int32 Started = 0;
	TArray<FString> Refused;
	for (const FWorkFort& F : P.Forts)
	{
		FString Why;
		if (StartFort(KmAtWorld(Project(F.Lat, F.Lon)), F.bLarge, P.Bearing - 90.f, &Why) != INDEX_NONE)
		{
			++Started;
		}
		else
		{
			Refused.AddUnique(Why);
		}
	}
	if (Started > 0)
	{
		ProgrammeState[Index] = 2;
		News.Add(FString::Printf(TEXT("%s påbegyndes: %d skanser"), P.Name, Started));
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|works|%s|%d of %d forts|%s"), P.Name, Started, P.Forts.Num(), *FString::Join(Refused, TEXT("; ")));
	return Started > 0;
}

void ACampaign1851Map::MonthlyFortProgrammes()
{
	const bool bAuto = Nations.IsValidIndex(PlayerNation) && Nations[PlayerNation].Mode(ECampaign1851Portfolio::War) == ECampaign1851Delegation::Auto;
	const double Reserve = Nations.IsValidIndex(PlayerNation) ? Nations[PlayerNation].Reserve : 0.0;
	for (int32 i = 0; i < Programmes().Num(); ++i)
	{
		if (!ProgrammeState.IsValidIndex(i) || ProgrammeState[i] >= 2 || CampaignDays < ProgrammeDay[i])
		{
			continue;
		}
		const FWorkProgramme& P = Programmes()[i];
		const double Cost = ProgrammeCost(i);
		if (bAuto)
		{
			if (Treasury - Cost > Reserve && MinistryCanSpend(ECampaign1851Portfolio::War, Cost) && BuildProgramme(i))
			{
				MinistrySpend(ECampaign1851Portfolio::War, Cost);
				FCampaign1851Decision D;
				D.Day = CampaignDays;
				D.Nation = PlayerNation;
				D.Portfolio = ECampaign1851Portfolio::War;
				D.Action = FString::Printf(TEXT("Befæstning: %s"), P.Name);
				D.Reasons = FString::Printf(TEXT("spænding %.0f; kassen kan bære %s rd."), Tension, *FString::FromInt(int32(Cost)));
				D.Cost = Cost;
				D.bDone = true;
				AddDecision(D);
			}
			continue;
		}
		if (ProgrammeState[i] == 0)
		{
			// The ministry proposes it; the player decides (UDFØR in the council).
			ProgrammeState[i] = 1;
			FCampaign1851Decision D;
			D.Day = CampaignDays;
			D.Nation = PlayerNation;
			D.Portfolio = ECampaign1851Portfolio::War;
			D.Kind = ECampaign1851DecisionKind::FortProgramme;
			D.A = i;
			D.Action = FString::Printf(TEXT("Forslag: %s (%d skanser)"), P.Name, P.Forts.Num());
			D.Reasons = FString::Printf(TEXT("Krigsministeriet anbefaler værket; pris ca. %s rd."), *FString::FromInt(int32(Cost)));
			D.Cost = Cost;
			D.bAdvice = true;
			AddDecision(D);
			News.Add(FString::Printf(TEXT("Krigsministeriet foreslår %s"), P.Name));
		}
	}
}

TArray<FString> ACampaign1851Map::SaveFortProgrammes() const
{
	TArray<FString> Out;
	for (int32 i = 0; i < ProgrammeState.Num(); ++i)
	{
		Out.Add(FString::Printf(TEXT("prog|%d|%d|%.1f"), i, ProgrammeState[i], ProgrammeDay[i]));
	}
	return Out;
}

void ACampaign1851Map::RestoreFortProgramme(int32 Index, int32 State, double Day)
{
	if (ProgrammeState.IsValidIndex(Index))
	{
		ProgrammeState[Index] = State;
		ProgrammeDay[Index] = Day;
	}
}

// ------------------------------------------------------------------ sieges

bool ACampaign1851Map::HasFortsNear(int32 Town) const
{
	return Forts.ContainsByPredicate([this, Town](const FCampaign1851Fort& F) { return F.bBuilt && FVector2D::Distance(F.Km, TownKm(Town)) < 10.0; });
}

void ACampaign1851Map::DailySieges()
{
	for (int32 k = 0; k < EnemyCorps.Num(); ++k)
	{
		FCampaign1851EnemyCorps& C = EnemyCorps[k];
		if (!C.bSieging || !Cities.IsValidIndex(C.SiegeTown) || C.bEngaged)
		{
			continue;
		}
		const FVector2D At = TownKm(C.SiegeTown);
		const int32 Days = FMath::FloorToInt(CampaignDays - C.SiegeStart);
		bool bBreach = false;
		// The siege batteries: powder spent in the duel, parapets breached every ten days.
		for (FCampaign1851Fort& F : Forts)
		{
			if (!F.bBuilt || FVector2D::Distance(F.Km, At) > 10.0)
			{
				continue;
			}
			F.RoundsPerGun = FMath::Max(0.f, F.RoundsPerGun - 6.f);
			F.FoodDays = FMath::Max(0.f, F.FoodDays - 0.3f);
			if (Days > 0 && Days % 10 == 0 && F.Defence > 1)
			{
				--F.Defence;
				News.Add(FString::Printf(TEXT("Belejringen: %s er skudt i stykker (forsvar %d)"), *F.Name, F.Defence));
			}
			bBreach |= F.Defence <= 1;
		}
		// The bombardment costs the besieged men.
		for (int32 i = 0; i < Regiments.Num(); ++i)
		{
			FCampaign1851Regiment& R = Regiments[i];
			if (R.Men > 0 && FVector2D::Distance(R.Km, At) < 10.0)
			{
				const int32 Hit = FMath::RoundToInt(R.PresentMen() * 0.002f);
				R.Men -= Hit;
				R.Sick += Hit / 2;
				DanishWarLosses += Hit;
			}
		}
		// Storm: breached, strong enough now, or after five weeks.
		const float Strength = C.Men * (C.Nation == TEXT("PR") ? 1.3f : 1.05f) + C.Guns * 60.f;
		const float Defence = FMath::Max(1.f, EnemyEstimateOfDefence(C.SiegeTown));
		if (bBreach || Strength / Defence >= 1.3f || Days >= 35)
		{
			C.bSieging = false;
			C.bEngaged = true;
			News.Add(FString::Printf(TEXT("%s stormer stillingen ved %s efter %d dages belejring"), *C.Name, *Cities[C.SiegeTown].Name, Days));
			C.Km = At;
			C.Town = C.SiegeTown;
			C.SiegeTown = INDEX_NONE;
			CreateBattle(k);
		}
	}
}

// A reproducible new-campaign fixture; defenders and works are the scenario's existing ones.
bool ACampaign1851Map::StartSiegeTest(const FString& Request)
{
	FString SiegeTownName = Request, SiegeDaysText;
	int32 SiegeTestDays = 45;
	if (Request.Split(TEXT(":"), &SiegeTownName, &SiegeDaysText)
		&& (!FDefaultValueHelper::ParseInt(SiegeDaysText, SiegeTestDays) || SiegeTestDays < 1 || SiegeTestDays > 365))
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|siege|afvist: dage skal være 1..365"));
		return false;
	}
	const int32 SiegeTarget = FindCity(SiegeTownName);
	if (!Cities.IsValidIndex(SiegeTarget) || Cities[SiegeTarget].bForeign || !Cities[SiegeTarget].Occupier.IsEmpty()
		|| (!HasFortsNear(SiegeTarget) && EnemyEstimateOfDefence(SiegeTarget) <= 1.f))
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|siege|afvist: %s skal være en ubesat egen by med garnison eller færdig skanse"), *SiegeTownName);
		return false;
	}
	if (!bAtWar) { ForceWar(); }
	bAtWar = true;
	const int32 SiegeBefore = EnemyCorps.Num();
	SpawnCorps(TEXT("Belejringstestkorps"), TEXT("PR"), 0.01f, Cities[SiegeTarget].Name, { Cities[SiegeTarget].Name }, 0.f);
	if (EnemyCorps.Num() == SiegeBefore) { return false; }
	FCampaign1851EnemyCorps& SiegeCorps = EnemyCorps.Last();
	SiegeCorps.Km = TownKm(SiegeTarget) + FVector2D(0.0, -8.9);
	SiegeCorps.Town = INDEX_NONE;
	SiegeCorps.SiegeTown = SiegeTarget;
	SiegeCorps.Guns = 0;
	// Comparable strength forces the regular siege-start path, rather than an immediate storm.
	SiegeCorps.Men = FMath::Max(1, FMath::RoundToInt(EnemyEstimateOfDefence(SiegeTarget) / 1.3f));
	SiegeCorps.StartMen = SiegeCorps.Men;
	SiegeTestTown = SiegeTarget;
	SiegeTestCorpsId = SiegeCorps.Id;
	SiegeTestEndDay = CampaignDays + SiegeTestDays;
	SetSpeed(NumSpeeds() - 1);
	LogSiegeTest(TEXT("opstilling"));
	return true;
}

void ACampaign1851Map::LogSiegeTest(const TCHAR* Phase)
{
	if (!Cities.IsValidIndex(SiegeTestTown)) { return; }
	const FCampaign1851EnemyCorps* SiegeCorps = EnemyCorps.FindByPredicate([this](const FCampaign1851EnemyCorps& SiegeEntry) { return SiegeEntry.Id == SiegeTestCorpsId; });
	int32 SiegeGarrison = 0, SiegeFortMen = 0, SiegeFortGuns = 0;
	float SiegeFood = 0.f, SiegeAmmo = 0.f;
	FString SiegeWorks;
	for (const FCampaign1851Regiment& SiegeRegiment : Regiments)
	{
		if (FVector2D::Distance(SiegeRegiment.Km, TownKm(SiegeTestTown)) < 10.0)
		{
			SiegeGarrison += SiegeRegiment.PresentMen();
			SiegeFood += SiegeRegiment.Food;
			SiegeAmmo += SiegeRegiment.Ammo;
		}
	}
	for (const FCampaign1851Fort& SiegeFort : Forts)
	{
		if (SiegeFort.bBuilt && FVector2D::Distance(SiegeFort.Km, TownKm(SiegeTestTown)) < 10.0)
		{
			SiegeFortMen += SiegeFort.Garrison;
			SiegeFortGuns += SiegeFort.Guns;
			SiegeWorks += FString::Printf(TEXT("%d:forsvar=%d,proviant=%.1f,skud=%.1f;"), SiegeFort.Id, SiegeFort.Defence, SiegeFort.FoodDays, SiegeFort.RoundsPerGun);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|siege|%s|dag=%.0f|korps=%d:%s|mand=%d|by=%s|garnison=%d|skansemænd=%d|kanoner=%d|belejrer=%d|slag=%d|forløb=%.0f/35|regimentsproviant-sum=%.1f|ammo-sum=%.2f|skanser=%s|faldet=%d|besættelse=%s|prestige=ikke implementeret|stemning=%.1f"),
		Phase, CampaignDays, SiegeTestCorpsId, SiegeCorps ? *SiegeCorps->Name : TEXT("opløst"), SiegeCorps ? SiegeCorps->Men : 0,
		*Cities[SiegeTestTown].Name, SiegeGarrison, SiegeFortMen, SiegeFortGuns,
		SiegeCorps && SiegeCorps->bSieging ? 1 : 0, SiegeCorps && SiegeCorps->bEngaged ? 1 : 0,
		SiegeCorps ? CampaignDays - SiegeCorps->SiegeStart : 0.0, SiegeFood, SiegeAmmo, *SiegeWorks,
		Cities[SiegeTestTown].Occupier.IsEmpty() ? 0 : 1, *Cities[SiegeTestTown].Occupier, Mood);
	if (CampaignDays >= SiegeTestEndDay)
	{
		SetSpeed(0);
		SiegeTestTown = INDEX_NONE;
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|siege|test afsluttet; kampagnen er sat på pause"));
	}
}
