// Manpower of the 1851 campaign: each amt's reserve of trained men (the yearly class under the 1849
// conscription), replacements for the battalions from their home amt, and new battalions raised at a
// finished barracks. ACampaign1851Map's manpower layer.

#include "Campaign1851Map.h"

#include "Campaign1851ConstructionSite.h"

int32 ACampaign1851Map::AmtIndexOfTown(int32 CityIndex) const
{
	if (!Cities.IsValidIndex(CityIndex))
	{
		return INDEX_NONE;
	}
	const int32 Id = Cities[CityIndex].AmtId;
	return Amter.IndexOfByPredicate([Id](const FCampaign1851Amt& A) { return A.Id == Id; });
}

void ACampaign1851Map::ResetManpower()
{
	// After the war of 1848-50 every amt has men who have served; the rest of the fit men are the ceiling.
	AmtManpower.Reset();
	for (const FCampaign1851Amt& A : Amter)
	{
		AmtManpower.Add(float(A.Population) * Campaign1851Army::ManpowerStartShare);
	}
}

float ACampaign1851Map::ManpowerCap(int32 AmtIndex) const
{
	return Amter.IsValidIndex(AmtIndex) ? float(Amter[AmtIndex].Population) * Campaign1851Army::ManpowerCapShare : 0.f;
}

float ACampaign1851Map::YearlyClass(int32 AmtIndex) const
{
	return Amter.IsValidIndex(AmtIndex) ? float(Amter[AmtIndex].Population) * Campaign1851Army::YearlyClassShare : 0.f;
}

int32 ACampaign1851Map::BattalionTarget(int32 Regiment) const
{
	// The battalion's full strength less the companies away in forts (they keep their own men there).
	if (!Regiments.IsValidIndex(Regiment))
	{
		return 0;
	}
	const FCampaign1851Regiment& R = Regiments[Regiment];
	int32 Away = 0;
	for (int32 k = 0; k < R.CompanyFort.Num(); ++k)
	{
		Away += R.CompanyFort[k] != 0 ? R.MaxMen / FMath::Max(1, R.Captains.Num()) : 0;
	}
	return FMath::Max(0, R.MaxMen - Away);
}

void ACampaign1851Map::MonthlyManpower()
{
	// The yearly class comes in a twelfth at a time, up to the amt's fit men.
	for (int32 a = 0; a < Amter.Num() && a < AmtManpower.Num(); ++a)
	{
		AmtManpower[a] = FMath::Min(AmtManpower[a] + YearlyClass(a) / 12.f, ManpowerCap(a));
	}
	// Replacements: battalions in garrison or halted are filled from their home amt's reserve (depots),
	// a few per cent a month; the recruits dilute the experience a little.
	double Cost = 0.0;
	int32 Men = 0;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		FCampaign1851Regiment& R = Regiments[i];
		const int32 AmtIndex = AmtIndexOfTown(R.Home);
		if (R.IsMarching() || !AmtManpower.IsValidIndex(AmtIndex))
		{
			continue;
		}
		const int32 Need = BattalionTarget(i) - R.Men;
		const int32 Take = FMath::Min3(Need, FMath::CeilToInt(R.MaxMen * Campaign1851Army::ReplacementShare), FMath::FloorToInt(AmtManpower[AmtIndex]));
		if (Take <= 0 || Take * Campaign1851Army::ReplacementCostPerMan > Treasury)
		{
			continue;
		}
		R.Experience = (R.Experience * R.Men + Campaign1851Army::RecruitExperience * Take) / float(R.Men + Take);
		R.Men += Take;
		AmtManpower[AmtIndex] -= Take;
		Men += Take;
		Cost += Take * Campaign1851Army::ReplacementCostPerMan;
	}
	if (Men > 0)
	{
		AddTransaction(-Cost, FString::Printf(TEXT("Genopfyldning: %d rekrutter udrustet"), Men));
	}
	// The battalions raised since 1851 are paid for from the development budget.
	if (RaisedUpkeepPerMonth() > 0.5)
	{
		AddTransaction(-RaisedUpkeepPerMonth(), TEXT("Nye bataljoners underhold"));
	}
}

double ACampaign1851Map::RaisedUpkeepPerMonth() const
{
	double Total = 0.0;
	for (const FCampaign1851Regiment& R : Regiments)
	{
		Total += R.bRaised ? Campaign1851Army::RaisedUpkeepPerMonth : 0.0;
	}
	return Total;
}

FString ACampaign1851Map::RaiseBlockReason(int32 CityIndex) const
{
	if (!Cities.IsValidIndex(CityIndex) || Cities[CityIndex].bForeign)
	{
		return TEXT("-");
	}
	const ACampaign1851ConstructionSite* Site = FindProject(CityIndex);
	if (!Site || !Site->IsBarracksDone())
	{
		return TEXT("kræver en færdig kaserne");
	}
	const int32 AmtIndex = AmtIndexOfTown(CityIndex);
	if (!AmtManpower.IsValidIndex(AmtIndex) || AmtManpower[AmtIndex] < Campaign1851Army::RaiseMen)
	{
		return FString::Printf(TEXT("amtet har kun %d mand i reserve (kræver %d)"), AmtManpower.IsValidIndex(AmtIndex) ? FMath::FloorToInt(AmtManpower[AmtIndex]) : 0, Campaign1851Army::RaiseMen);
	}
	if (Treasury < Campaign1851Army::RaiseCost())
	{
		return TEXT("ikke råd");
	}
	return FString();
}

int32 ACampaign1851Map::RaiseBattalion(int32 CityIndex, FString* OutReason)
{
	const FString Why = RaiseBlockReason(CityIndex);
	if (!Why.IsEmpty())
	{
		if (OutReason) { *OutReason = Why; }
		return INDEX_NONE;
	}
	// The next free battalion number (the 1851 army has 1-14).
	int32 Number = 1;
	while (FindRegiment(FString::Printf(TEXT("B%d"), Number)) != INDEX_NONE)
	{
		++Number;
	}
	const int32 Index = AddRaisedRegiment(FString::Printf(TEXT("B%d"), Number), FString::Printf(TEXT("%d. Bataillon"), Number), ECampaign1851Arm::Infantry, CityIndex, Campaign1851Army::RaiseMen);
	FCampaign1851Regiment& R = Regiments[Index];
	R.Experience = Campaign1851Army::RecruitExperience;
	for (float& S : R.Skills)
	{
		S = Campaign1851Army::RecruitSkill;
	}
	AmtManpower[AmtIndexOfTown(CityIndex)] -= Campaign1851Army::RaiseMen;
	AddTransaction(-Campaign1851Army::RaiseCost(), FString::Printf(TEXT("%s oprettet i %s (udrustning)"), *R.Name, *Cities[CityIndex].Name));
	// Its officers: a major and four captains, hired.
	FRandomStream Rng(int32(HashCombine(uint32(Seed), uint32(Number * 977))));
	FCampaign1851Officer Major = MakeOfficer(Rng, false, TEXT("Major"));
	Major.Id = FString::Printf(TEXT("R%d"), NextOfficerNumber++);
	Major.bRecruited = true;
	const int32 MajorIndex = Officers.Add(Major);
	AssignOfficer(MajorIndex, Index);
	for (int32 k = 0; k < R.Captains.Num(); ++k)
	{
		FCampaign1851Officer Captain = MakeOfficer(Rng, false, TEXT("Kaptajn"));
		Captain.Id = FString::Printf(TEXT("R%d"), NextOfficerNumber++);
		Captain.bRecruited = true;
		Captain.CaptainOf = Index;
		Captain.Company = k;
		R.Captains[k] = Officers.Add(Captain);
	}
	News.Add(FString::Printf(TEXT("%s er oprettet i %s: 760 rekrutter"), *R.Name, *Cities[CityIndex].Name));
	return Index;
}

int32 ACampaign1851Map::AddRaisedRegiment(const FString& Id, const FString& Name, ECampaign1851Arm Arm, int32 Home, int32 MaxMen)
{
	FCampaign1851Regiment R;
	R.Id = Id;
	R.Name = Name;
	R.Arm = Arm;
	R.Home = Home;
	R.Town = Home;
	R.Men = R.MaxMen = MaxMen;
	R.Horses = 14;
	R.bRaised = true;
	R.Command = CommandsAtStart.IndexOfByPredicate([Home](const FCampaign1851Command& C) { return C.Towns.Contains(Home); });
	R.Captains.Init(INDEX_NONE, Campaign1851Army::CompaniesFor(Arm));
	R.CompanyFort.Init(0, R.Captains.Num());
	R.PaceKmPerDay = Campaign1851Army::MarchKmPerDay(Arm);
	const int32 Index = Regiments.Add(R);
	PlaceInTown(Index);
	UpdateRegimentPiece(Index);
	return Index;
}
