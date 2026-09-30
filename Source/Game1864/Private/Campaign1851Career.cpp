// Officers' careers (Docs/Army1851.md, backlog item 6): every New Year the officers grow a year older; the old
// retire, some die, and the Army's officer school sends a class of new captains (better staff officers when
// the staff college has been founded). A vacant post is left for the War Ministry (or the player) to fill.

#include "Campaign1851Map.h"

void ACampaign1851Map::VacateOfficer(int32 Officer)
{
	for (FCampaign1851Regiment& R : Regiments)
	{
		R.Chief = R.Chief == Officer ? INDEX_NONE : R.Chief;
		R.General = R.General == Officer ? INDEX_NONE : R.General;
		for (int32& Post : R.Captains)
		{
			Post = Post == Officer ? INDEX_NONE : Post;
		}
	}
	for (FCampaign1851Command& C : Commands)
	{
		C.General = C.General == Officer ? INDEX_NONE : C.General;
	}
	for (FCampaign1851Formation& F : Formations)
	{
		F.Commander = F.Commander == Officer ? INDEX_NONE : F.Commander;
		F.Deputy = F.Deputy == Officer ? INDEX_NONE : F.Deputy;
		F.StaffChief = F.StaffChief == Officer ? INDEX_NONE : F.StaffChief;
	}
	FCampaign1851Officer& O = Officers[Officer];
	O.Regiment = INDEX_NONE;
	O.Command = INDEX_NONE;
	O.Formation = 0;
	O.CaptainOf = INDEX_NONE;
	O.Company = INDEX_NONE;
	O.StaffOf = 0;
	O.StaffPost = 0;
}

void ACampaign1851Map::YearlyOfficers()
{
	const int32 Year = GetDate().GetYear();
	FRandomStream Rng(int32(HashCombine(uint32(Seed), uint32(Year) * 131u)));
	int32 Retired = 0, Died = 0;
	TArray<FString> Notable;
	for (int32 o = Officers.Num() - 1; o >= 0; --o)
	{
		const FCampaign1851Officer& O = Officers[o];
		const int32 Age = Year - O.Born;
		const bool bRetire = O.bGeneral ? (Age >= 70 || (Age >= 66 && Rng.FRand() < 0.3f)) : (Age >= 64 || (Age >= 60 && Rng.FRand() < 0.3f));
		const bool bDies = !bRetire && Rng.FRand() < 0.004f + FMath::Max(0, Age - 45) * 0.002f;
		if (!bRetire && !bDies)
		{
			continue;
		}
		const bool bPost = !O.IsFree();
		if (O.bGeneral || O.Regiment != INDEX_NONE)
		{
			Notable.Add(FString::Printf(TEXT("%s %s (%d år) %s"), *O.Rank, *O.Name, Age, bDies ? TEXT("er død") : TEXT("går på pension")));
		}
		if (bPost)
		{
			VacateOfficer(o);
		}
		DismissOfficer(o);
		(bDies ? Died : Retired)++;
	}
	// The officer school's class of the year (the staff college makes better staff officers).
	const int32 Class = 8;
	for (int32 k = 0; k < Class; ++k)
	{
		FCampaign1851Officer O = MakeOfficer(Rng, false, TEXT("Kaptajn"));
		O.Id = FString::Printf(TEXT("K%d"), NextOfficerNumber++);
		O.Born = Year - Rng.RandRange(24, 30);
		O.Experience = Rng.FRandRange(5.f, 20.f);
		O.bRecruited = true;
		if (HasResearch(TEXT("staff")))
		{
			O.Stats[int32(ECampaign1851OfficerStat::Staff)] = uint8(FMath::Min(10, O.Stats[int32(ECampaign1851OfficerStat::Staff)] + 2));
			O.Stats[int32(ECampaign1851OfficerStat::Tactical)] = uint8(FMath::Min(10, O.Stats[int32(ECampaign1851OfficerStat::Tactical)] + 1));
		}
		Officers.Add(O);
	}
	News.Add(FString::Printf(TEXT("Nytår: %d officerer går på pension, %d er døde; officersskolen udnævner %d nye kaptajner"), Retired, Died, Class));
	for (const FString& N : Notable)
	{
		News.Add(N);
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|officers|year %d|retired %d|died %d|new %d|total %d"), Year, Retired, Died, Class, Officers.Num());
}
