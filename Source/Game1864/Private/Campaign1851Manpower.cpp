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
		TakeRifles(Take, TEXT("rekrutter"));   // from the store; imports are booked there
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
		Total += R.bRaised && !R.bDetached && !R.Id.StartsWith(TEXT("SE")) ? Campaign1851Army::RaisedUpkeepPerMonth : 0.0;   // Sweden-Norway pays its own; a split unit is no new unit
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
	TakeRifles(Campaign1851Army::RaiseMen, R.Name);
	TakeHorses(R.Horses, R.Name);
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

int32 ACampaign1851Map::SplitRegiment(int32 RegimentIndex, FString* OutWhy)
{
	auto Fail = [OutWhy](const FString& Why) { if (OutWhy) { *OutWhy = Why; } return INDEX_NONE; };
	if (!Regiments.IsValidIndex(RegimentIndex))
	{
		return Fail(TEXT("Ingen enhed"));
	}
	const FCampaign1851Regiment Old = Regiments[RegimentIndex];
	if (Old.IsMarching())
	{
		return Fail(TEXT("Enheden skal stå stille for at blive delt"));
	}
	if (Old.Captains.Num() < 2 || Old.Men < 100)
	{
		return Fail(TEXT("For lille til at dele (mindst to kompagnier)"));
	}
	const int32 Moved = Old.Captains.Num() / 2;
	const int32 Keep = Old.Captains.Num() - Moved;
	const float Share = float(Moved) / Old.Captains.Num();
	// A free id: the old one with a letter.
	FString Id;
	for (TCHAR L = TEXT('b'); L <= TEXT('z'); ++L)
	{
		Id = Old.Id + FString::Chr(L);
		if (FindRegiment(Id) == INDEX_NONE) { break; }
	}
	const int32 New = AddRaisedRegiment(Id, Old.Name + TEXT(" (2. halvbataljon)"), Old.Arm, Old.Home, FMath::RoundToInt(Old.MaxMen * Share));
	FCampaign1851Regiment& N = Regiments[New];
	FCampaign1851Regiment& R = Regiments[RegimentIndex];
	N.bDetached = true;
	N.Men = FMath::RoundToInt(R.Men * Share);
	N.Sick = FMath::RoundToInt(R.Sick * Share);
	N.Horses = FMath::RoundToInt(R.Horses * Share);
	N.MaxHorses = FMath::RoundToInt(R.MaxHorses * Share);
	N.Guns = FMath::RoundToInt(R.Guns * Share);
	N.Experience = R.Experience;
	FMemory::Memcpy(N.Skills, R.Skills, sizeof(N.Skills));
	FMemory::Memcpy(N.FireDrills, R.FireDrills, sizeof(N.FireDrills));
	N.Program = R.Program;
	N.Morale = R.Morale;
	N.Cohesion = FMath::Max(10.f, R.Cohesion - 10.f);
	N.Present = R.Present;
	N.Food = R.Food;
	N.Fodder = R.Fodder;
	N.Ammo = R.Ammo;
	N.Formation = R.Formation;
	N.Command = R.Command;
	N.Town = R.Town;
	N.Km = R.Km;
	// The last companies go with their captains.
	N.Captains.Reset();
	N.CompanyFort.Reset();
	for (int32 k = Keep; k < R.Captains.Num(); ++k)
	{
		const int32 Captain = R.Captains[k];
		N.Captains.Add(Captain);
		N.CompanyFort.Add(R.CompanyFort.IsValidIndex(k) ? R.CompanyFort[k] : 0);
		if (Officers.IsValidIndex(Captain))
		{
			Officers[Captain].CaptainOf = New;
			Officers[Captain].Company = N.Captains.Num() - 1;
		}
	}
	R.Captains.SetNum(Keep);
	R.CompanyFort.SetNum(Keep);
	R.Men -= N.Men;
	R.Sick -= N.Sick;
	R.Horses -= N.Horses;
	R.MaxHorses -= N.MaxHorses;
	R.Guns -= N.Guns;
	R.MaxMen -= N.MaxMen;
	R.Cohesion = FMath::Max(10.f, R.Cohesion - 10.f);
	UpdateRegimentPiece(RegimentIndex);
	UpdateRegimentPiece(New);
	News.Add(FString::Printf(TEXT("%s er delt: %d kompagnier danner %s"), *R.Name, Moved, *N.Name));
	return New;
}

bool ACampaign1851Map::IsSplitPair(int32 A, int32 B) const
{
	if (!Regiments.IsValidIndex(A) || !Regiments.IsValidIndex(B) || A == B)
	{
		return false;
	}
	const FCampaign1851Regiment& RA = Regiments[A];
	const FCampaign1851Regiment& RB = Regiments[B];
	if (RA.Arm != RB.Arm || (!RA.bDetached && !RB.bDetached))
	{
		return false;
	}
	// A split-off half has the old id with a letter (1b, 1c, ...).
	auto Base = [](const FCampaign1851Regiment& R) { return R.bDetached && R.Id.Len() > 1 ? R.Id.LeftChop(1) : R.Id; };
	return Base(RA) == Base(RB);
}

int32 ACampaign1851Map::MergePartner(int32 RegimentIndex) const
{
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		if (IsSplitPair(RegimentIndex, i) && CanMerge(RegimentIndex, i))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

namespace
{
	bool StandTogether(const FCampaign1851Regiment& A, const FCampaign1851Regiment& B)
	{
		if (A.IsMarching() || B.IsMarching())
		{
			return false;
		}
		return (A.Town != INDEX_NONE && A.Town == B.Town) || FVector2D::Distance(A.Km, B.Km) < 2.0;
	}
}

bool ACampaign1851Map::CanMerge(int32 Keep, int32 Absorb, FString* OutWhy) const
{
	auto Fail = [OutWhy](const FString& Why) { if (OutWhy) { *OutWhy = Why; } return false; };
	if (!IsSplitPair(Keep, Absorb))
	{
		return Fail(TEXT("Kun to halvdele af samme enhed kan samles"));
	}
	if (!StandTogether(Regiments[Keep], Regiments[Absorb]))
	{
		return Fail(TEXT("De to halvdele skal stå samme sted (ikke på march)"));
	}
	for (const FCampaign1851Battle& B : Battles)
	{
		if (B.Regiments.Contains(Keep) || B.Regiments.Contains(Absorb))
		{
			return Fail(TEXT("Ikke midt i et slag"));
		}
	}
	if (Regiments[Keep].Captains.Num() + Regiments[Absorb].Captains.Num() > 10)
	{
		return Fail(TEXT("For mange kompagnier i én enhed"));
	}
	return true;
}

int32 ACampaign1851Map::MergeRegiments(int32 Keep, int32 Absorb, FString* OutWhy)
{
	if (!CanMerge(Keep, Absorb, OutWhy))
	{
		return INDEX_NONE;
	}
	const FCampaign1851Regiment A = Regiments[Absorb];
	FCampaign1851Regiment& K = Regiments[Keep];
	const float WK = float(FMath::Max(K.Men, 1)), WA = float(FMath::Max(A.Men, 1));
	auto Mix = [WK, WA](float X, float Y) { return (X * WK + Y * WA) / (WK + WA); };
	K.Experience = Mix(K.Experience, A.Experience);
	for (int32 s = 0; s < int32(ECampaign1851Skill::Count); ++s) { K.Skills[s] = Mix(K.Skills[s], A.Skills[s]); }
	for (int32 d = 0; d < 4; ++d) { K.FireDrills[d] = Mix(K.FireDrills[d], A.FireDrills[d]); }
	K.Morale = Mix(K.Morale, A.Morale);
	K.Cohesion = FMath::Max(10.f, Mix(K.Cohesion, A.Cohesion) - 5.f);
	K.Food = Mix(K.Food, A.Food);
	K.Fodder = Mix(K.Fodder, A.Fodder);
	K.Ammo = Mix(K.Ammo, A.Ammo);
	K.Present = Mix(K.Present, A.Present);
	K.Men += A.Men;
	K.MaxMen += A.MaxMen;
	K.Sick += A.Sick;
	K.Horses += A.Horses;
	K.MaxHorses += A.MaxHorses;
	K.Guns += A.Guns;
	K.Mortars += A.Mortars;
	K.Wagons += A.Wagons;
	K.TotalKilled += A.TotalKilled;
	K.TotalWounded += A.TotalWounded;
	K.TotalCaptured += A.TotalCaptured;
	K.TotalEnemyKilled += A.TotalEnemyKilled;
	K.Service.Append(A.Service);
	K.Service.Sort([](const FCampaign1851ServiceEntry& X, const FCampaign1851ServiceEntry& Y) { return X.Day < Y.Day; });
	// Its companies come over with their captains (and those in a fort stay there, now of this unit).
	const int32 Base = K.Captains.Num();
	for (int32 k = 0; k < A.Captains.Num(); ++k)
	{
		K.Captains.Add(A.Captains[k]);
		K.CompanyFort.Add(A.CompanyFort.IsValidIndex(k) ? A.CompanyFort[k] : 0);
		if (Officers.IsValidIndex(A.Captains[k]))
		{
			Officers[A.Captains[k]].CaptainOf = Keep;
			Officers[A.Captains[k]].Company = Base + k;
		}
	}
	for (FCampaign1851Fort& F : Forts)
	{
		for (FCampaign1851FortCompany& C : F.Companies)
		{
			if (C.Regiment == Absorb)
			{
				C.Regiment = Keep;
				C.Company += Base;
			}
		}
	}
	// Its chief is free for another post; a general riding with it stays with the joined unit if it has none.
	if (Officers.IsValidIndex(A.Chief))
	{
		Officers[A.Chief].Regiment = INDEX_NONE;
	}
	if (Officers.IsValidIndex(A.General))
	{
		if (K.General == INDEX_NONE)
		{
			K.General = A.General;
			Officers[A.General].Regiment = Keep;
		}
		else
		{
			Officers[A.General].Regiment = INDEX_NONE;
		}
	}
	if (!A.bDetached)
	{
		// The original was the half absorbed: the joined unit takes its name and is a whole unit again.
		K.Id = A.Id;
		K.Name = A.Name;
		K.bDetached = false;
		K.bRaised = A.bRaised;
	}
	else if (!K.bDetached)
	{
		K.Name = K.Name.Replace(TEXT(" (2. halvbataljon)"), TEXT(""));
	}
	News.Add(FString::Printf(TEXT("%s er samlet igen (%d kompagnier, %d mand)"), *K.Name, K.Captains.Num(), K.Men));
	RemoveRegimentAt(Absorb);
	const int32 NewKeep = Keep > Absorb ? Keep - 1 : Keep;
	UpdateRegimentPiece(NewKeep);
	return NewKeep;
}

bool ACampaign1851Map::MoveCompany(int32 From, int32 Company, int32 To, FString* OutWhy)
{
	auto Fail = [OutWhy](const FString& Why) { if (OutWhy) { *OutWhy = Why; } return false; };
	if (!Regiments.IsValidIndex(From) || !Regiments.IsValidIndex(To) || From == To || !Regiments[From].Captains.IsValidIndex(Company))
	{
		return Fail(TEXT("Ingen enhed"));
	}
	FCampaign1851Regiment& F = Regiments[From];
	FCampaign1851Regiment& T = Regiments[To];
	if (Campaign1851Army::CompaniesFor(T.Arm) == 0 || (F.Arm != T.Arm && !(Campaign1851Army::CompaniesFor(F.Arm) > 0)))
	{
		return Fail(TEXT("Kompagniet kan kun gå over til en bataljon"));
	}
	if (!StandTogether(F, T))
	{
		return Fail(TEXT("De to enheder skal stå samme sted (ikke på march)"));
	}
	if (F.Captains.Num() <= 1)
	{
		return Fail(TEXT("Enheden kan ikke afgive sit sidste kompagni"));
	}
	if (T.Captains.Num() >= 10)
	{
		return Fail(TEXT("Enheden har allerede ti kompagnier"));
	}
	if (F.CompanyFort.IsValidIndex(Company) && F.CompanyFort[Company] != 0)
	{
		return Fail(TEXT("Kompagniet ligger i en skanse; kald det hjem først"));
	}
	for (const FCampaign1851Battle& B : Battles)
	{
		if (B.Regiments.Contains(From) || B.Regiments.Contains(To))
		{
			return Fail(TEXT("Ikke midt i et slag"));
		}
	}
	// Its share of the battalion: the men with the colours spread over the companies not in a fort.
	int32 WithIt = 0;
	for (int32 k = 0; k < F.Captains.Num(); ++k)
	{
		WithIt += F.CompanyFort.IsValidIndex(k) && F.CompanyFort[k] != 0 ? 0 : 1;
	}
	const float Share = 1.f / float(FMath::Max(WithIt, 1));
	const int32 Men = FMath::RoundToInt(F.Men * Share), Sick = FMath::RoundToInt(F.Sick * Share), Max = FMath::RoundToInt(F.MaxMen / float(F.Captains.Num()));
	const float WT = float(FMath::Max(T.Men, 1)), WF = float(FMath::Max(Men, 1));
	auto Mix = [WT, WF](float X, float Y) { return (X * WT + Y * WF) / (WT + WF); };
	T.Experience = Mix(T.Experience, F.Experience);
	for (int32 s = 0; s < int32(ECampaign1851Skill::Count); ++s) { T.Skills[s] = Mix(T.Skills[s], F.Skills[s]); }
	for (int32 d = 0; d < 4; ++d) { T.FireDrills[d] = Mix(T.FireDrills[d], F.FireDrills[d]); }
	T.Morale = Mix(T.Morale, F.Morale);
	T.Men += Men; F.Men -= Men;
	T.Sick += Sick; F.Sick -= Sick;
	T.MaxMen += Max; F.MaxMen -= Max;
	T.Cohesion = FMath::Max(10.f, T.Cohesion - 3.f);
	const int32 Captain = F.Captains[Company];
	T.Captains.Add(Captain);
	T.CompanyFort.Add(0);
	F.Captains.RemoveAt(Company);
	if (F.CompanyFort.IsValidIndex(Company)) { F.CompanyFort.RemoveAt(Company); }
	if (Officers.IsValidIndex(Captain))
	{
		Officers[Captain].CaptainOf = To;
		Officers[Captain].Company = T.Captains.Num() - 1;
	}
	// The companies after it move up one place in their battalion.
	for (int32 k = Company; k < F.Captains.Num(); ++k)
	{
		if (Officers.IsValidIndex(F.Captains[k])) { Officers[F.Captains[k]].Company = k; }
	}
	for (FCampaign1851Fort& Fort : Forts)
	{
		for (FCampaign1851FortCompany& C : Fort.Companies)
		{
			if (C.Regiment == From && C.Company > Company) { --C.Company; }
		}
	}
	UpdateRegimentPiece(From);
	UpdateRegimentPiece(To);
	return true;
}

void ACampaign1851Map::RemoveRegimentAt(int32 Index)
{
	if (!Regiments.IsValidIndex(Index))
	{
		return;
	}
	if (RegimentPieces.IsValidIndex(Index))
	{
		if (RegimentPieces[Index]) { RegimentPieces[Index]->DestroyComponent(); }
		RegimentPieces.RemoveAt(Index);
	}
	if (RegimentCars.Num() >= (Index + 1) * 3)
	{
		for (int32 c = 2; c >= 0; --c)
		{
			if (RegimentCars[Index * 3 + c]) { RegimentCars[Index * 3 + c]->DestroyComponent(); }
			RegimentCars.RemoveAt(Index * 3 + c);
		}
	}
	Regiments.RemoveAt(Index);
	auto Fix = [Index](int32& R) { if (R == Index) { R = INDEX_NONE; } else if (R > Index) { --R; } };
	for (FCampaign1851Officer& O : Officers)
	{
		Fix(O.Regiment);
		const bool bWasCaptain = O.CaptainOf == Index;
		Fix(O.CaptainOf);
		if (bWasCaptain) { O.Company = INDEX_NONE; }
	}
	for (FCampaign1851Fort& F : Forts)
	{
		F.Companies.RemoveAll([Index](const FCampaign1851FortCompany& C) { return C.Regiment == Index; });
		for (FCampaign1851FortCompany& C : F.Companies) { Fix(C.Regiment); }
	}
	for (FCampaign1851Battle& B : Battles)
	{
		B.Regiments.Remove(Index);
		for (int32& R : B.Regiments) { Fix(R); }
	}
	for (FCampaign1851TroopTrain& T : TroopTrainList)
	{
		Fix(T.Lead);
	}
	for (FCampaign1851SupplyColumn& S : SupplyColumns)
	{
		if (!S.bFort) { Fix(S.Target); }
	}
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
	R.Horses = R.MaxHorses = 14;
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
