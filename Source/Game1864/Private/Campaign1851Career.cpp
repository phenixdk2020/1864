// Monthly officer careers and battle hooks; annual fictional graduates refill national pools.
// Rules and estimated tuning: Docs/Officerskarrierer.md.

#include "Campaign1851Map.h"
#include "Dom/JsonObject.h"

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
		LogOfficerCareer(O, TEXT("Fiktiv officersskolekandidat til reservepuljen"));
		Officers.Add(O);
	}
	for (auto& CareerNation : ForeignOfficers)
	{
		FCampaign1851Officer CareerGraduate;
		CareerGraduate.Nation = CareerNation.Key;
		CareerGraduate.Id = FString::Printf(TEXT("F_CAREER_%s_%d"), *CareerNation.Key, Year);
		if (CareerNation.Value.ContainsByPredicate([&](const FCampaign1851Officer& CareerCandidate) { return CareerCandidate.Id == CareerGraduate.Id; })) { continue; }
		CareerGraduate.Name = FString::Printf(TEXT("Fiktiv officer %s årgang %d"), *CareerNation.Key, Year);
		CareerGraduate.Rank = TEXT("Kaptajn");
		CareerGraduate.Born = Year - 26;
		CareerGraduate.bRecruited = true;
		CareerGraduate.Experience = 20.f;
		LogOfficerCareer(CareerGraduate, TEXT("Fiktiv officersskolekandidat til reservepuljen"));
		CareerNation.Value.Add(CareerGraduate);
	}
	LogCampaignDecision(TEXT("DK"), TEXT("other"), TEXT("OfficerSchool"), TEXT("Officersskolen udnævner otte fiktive kaptajner"), FString(), {{TEXT("count"), 8.0}});
}

void ACampaign1851Map::RecordBattleOfficerCareers(const TArray<TSharedPtr<FJsonValue>>& Participants, const FString& Place)
{
    TSet<FString> ReportSeenOfficers;
    for (const TSharedPtr<FJsonValue>& ReportValue : Participants)
    {
        const TSharedPtr<FJsonObject> ReportOfficer = ReportValue.IsValid() && ReportValue->Type == EJson::Object ? ReportValue->AsObject() : nullptr;
        FString ReportId, ReportFate;
        if (!ReportOfficer.IsValid() || !ReportOfficer->TryGetStringField(TEXT("id"), ReportId) || ReportSeenOfficers.Contains(ReportId)) { continue; }
        const int32 ReportIndex = Officers.IndexOfByPredicate([&ReportId](const FCampaign1851Officer& ReportCandidate) { return ReportCandidate.Id == ReportId; });
        if (ReportIndex == INDEX_NONE) { continue; }
        ReportSeenOfficers.Add(ReportId);
        ReportOfficer->TryGetStringField(TEXT("fate"), ReportFate);
        bool bReportDistinction = false;
        ReportOfficer->TryGetBoolField(TEXT("distinction"), bReportDistinction);
        if (bReportDistinction) { GainOfficerExperience(Officers[ReportIndex], 2.f); LogOfficerCareer(Officers[ReportIndex], TEXT("Udmærkelse i slag")); }
        Officers[ReportIndex].Career.Add(FString::Printf(TEXT("%s — %s: deltog%s%s%s"),
            *GetDate().ToString(TEXT("%d.%m.%Y")), *Place,
            ReportFate == TEXT("wounded") ? TEXT("; såret") : TEXT(""),
            ReportFate == TEXT("captured") ? TEXT("; taget til fange") : TEXT(""),
            bReportDistinction ? TEXT("; udmærkelse for indsats i kamp") : TEXT("")));
    }
}

// All numeric career rules below are gameplay estimates, not historical service regulations.
namespace
{
	int32 CareerRankIndex(const FCampaign1851Officer& CareerOfficer)
	{
		if (CareerOfficer.Rank.StartsWith(TEXT("General af"))) { return 6; }
		if (CareerOfficer.Rank.StartsWith(TEXT("Generalmajor"))) { return 4; }
		return Campaign1851Army::RankIndex(CareerOfficer.Rank);
	}

	const TArray<FString>& CareerNationRanks(const FString& CareerNation)
	{
		// Danish display translations; separate national top ranks retain archive spelling.
		static const TArray<FString> CareerSwedish = {TEXT("Kaptajn"), TEXT("Major"), TEXT("Oberstløjtnant"), TEXT("Oberst"), TEXT("Generalmajor"), TEXT("Generalløjtnant"), TEXT("General af kavaleriet")};
		static const TArray<FString> CareerPrussian = {TEXT("Kaptajn"), TEXT("Major"), TEXT("Oberstløjtnant"), TEXT("Oberst"), TEXT("Generalmajor"), TEXT("Generalløjtnant"), TEXT("General af kavalleriet")};
		return CareerNation == TEXT("SE") ? CareerSwedish : CareerNation == TEXT("PR") ? CareerPrussian : Campaign1851Army::Ranks();
	}
}

void ACampaign1851Map::GainOfficerExperience(FCampaign1851Officer& CareerOfficer, float CareerGain)
{
	if (CareerOfficer.CareerStatus != 0 || CareerOfficer.Away != 0) { return; }
	CareerOfficer.Experience = FMath::Clamp(CareerOfficer.Experience + CareerGain, 0.f, 100.f);
	CareerOfficer.CareerExperience = FMath::Clamp(CareerOfficer.CareerExperience + CareerGain, 0.f, 100.f);
}

void ACampaign1851Map::LogOfficerCareer(FCampaign1851Officer& CareerOfficer, const FString& CareerAction)
{
	CareerOfficer.Career.Add(GetDate().ToString(TEXT("%d.%m.%Y")) + TEXT(" — ") + CareerAction);
	LogCampaignDecision(CareerOfficer.Nation, TEXT("other"), TEXT("OfficerCareer"), CareerAction, FString(),
		{{TEXT("experience"), double(CareerOfficer.Experience)}, {TEXT("seniorityMonths"), double(CareerOfficer.RankMonths)},
		 {TEXT("status"), double(CareerOfficer.CareerStatus)}, {TEXT("commandMonths"), double(CareerOfficer.CommandMonths)},
		 {TEXT("battles"), double(CareerOfficer.BattlesServed)}, {TEXT("wounds"), double(CareerOfficer.Wounds)},
		 {TEXT("earnedExperience"), double(CareerOfficer.CareerExperience)}},
		{{TEXT("officer"), CareerOfficer.Id}, {TEXT("name"), CareerOfficer.Name}, {TEXT("rank"), CareerOfficer.Rank},
		 {TEXT("identity"), CareerOfficer.bHistorical ? TEXT("historical") : TEXT("fictional")}});
}

bool ACampaign1851Map::HasOfficerVacancy(const FString& CareerRank) const
{
	for (const FCampaign1851Regiment& CareerRegiment : Regiments)
	{
		const TCHAR* CareerChiefRank = CareerRegiment.Arm == ECampaign1851Arm::Cavalry ? TEXT("Oberst") :
			(CareerRegiment.Arm == ECampaign1851Arm::Artillery || CareerRegiment.Arm == ECampaign1851Arm::HorseArtillery) ? TEXT("Kaptajn") : TEXT("Major");
		if (CareerRegiment.Chief == INDEX_NONE && CareerRank == CareerChiefRank) { return true; }
		if (CareerRank == TEXT("Kaptajn") && CareerRegiment.Captains.Contains(INDEX_NONE)) { return true; }
	}
	for (const FCampaign1851Command& CareerCommand : Commands)
	{
		if (CareerCommand.General == INDEX_NONE && CareerRank == TEXT("Generalmajor")) { return true; }
	}
	for (const FCampaign1851Formation& CareerFormation : Formations)
	{
		const int32 CareerPosts[] = {CareerFormation.Commander, CareerFormation.Deputy, CareerFormation.StaffChief};
		for (int32 CareerPost = 0; CareerPost < 3; ++CareerPost)
		{
			const TCHAR* CareerRequired = FormationPostRank(CareerFormation.Id, CareerPost);
			if (CareerPosts[CareerPost] == INDEX_NONE && CareerRequired && CareerRank == CareerRequired) { return true; }
		}
	}
	return false;
}

void ACampaign1851Map::BattleOfficerCareer(FCampaign1851Officer& CareerOfficer, const FString& CareerPlace, float CareerLossShare, bool bCareerRisk)
{
	if (CareerOfficer.CareerStatus != 0 || CareerOfficer.Away != 0) { return; }
	++CareerOfficer.BattlesServed;
	GainOfficerExperience(CareerOfficer, 4.f);
	LogOfficerCareer(CareerOfficer, TEXT("Slagerfaring ved ") + CareerPlace);
	if (!bCareerRisk || CareerLossShare <= 0.f) { return; }
	FRandomStream CareerRng(int32(HashCombine(GetTypeHash(CareerOfficer.Id), uint32(Seed) ^ uint32(CareerOfficer.BattlesServed * 7919))));
	const float CareerRisk = FMath::Clamp(CareerLossShare, 0.f, 1.f);
	const float CareerRoll = CareerRng.FRand();
	if (CareerRoll >= 0.02f + 0.10f * CareerRisk) { return; }
	const int32 CareerIndex = CareerOfficer.Nation == TEXT("DK") ? Officers.IndexOfByPredicate([&](const FCampaign1851Officer& CareerCandidate) { return CareerCandidate.Id == CareerOfficer.Id; }) : INDEX_NONE;
	if (CareerIndex != INDEX_NONE) { VacateOfficer(CareerIndex); }
	if (CareerRoll < 0.002f + 0.02f * CareerRisk)
	{
		CareerOfficer.CareerStatus = 2;
		LogOfficerCareer(CareerOfficer, TEXT("Faldet i slag ved ") + CareerPlace);
	}
	else
	{
		++CareerOfficer.Wounds;
		CareerOfficer.Away = 1;
		CareerOfficer.AwayUntil = GetDate() + FTimespan::FromDays(CareerRng.RandRange(21, 84));
		LogOfficerCareer(CareerOfficer, TEXT("Såret i slag ved ") + CareerPlace);
	}
}

void ACampaign1851Map::ReplaceForeignOfficers()
{
	for (auto& CareerNation : ForeignOfficers)
	{
		auto CareerChoose = [&](const FString& CareerOldId) -> FString
		{
			const FCampaign1851Officer* CareerOld = ForeignCommander(CareerNation.Key, CareerOldId);
			if (CareerOld && CareerOld->bGeneral) { return CareerOld->Id; }
			int32 CareerBest = INDEX_NONE;
			for (int32 CareerIndex = 0; CareerIndex < CareerNation.Value.Num(); ++CareerIndex)
			{
				const FCampaign1851Officer& CareerCandidate = CareerNation.Value[CareerIndex];
				if (CareerCandidate.CareerStatus != 0 || CareerCandidate.Away != 0 || !CareerCandidate.bGeneral) { continue; }
				if (CareerBest == INDEX_NONE || CareerCandidate.Experience + CareerCandidate.RankMonths * 0.1f > CareerNation.Value[CareerBest].Experience + CareerNation.Value[CareerBest].RankMonths * 0.1f) { CareerBest = CareerIndex; }
			}
			if (CareerBest == INDEX_NONE) { return FString(); }
			LogOfficerCareer(CareerNation.Value[CareerBest], TEXT("Udnævnt til ledig feltkommando"));
			return CareerNation.Value[CareerBest].Id;
		};
		FString& CareerDefault = ForeignCommanderIds.FindOrAdd(CareerNation.Key);
		CareerDefault = CareerChoose(CareerDefault);
		for (FCampaign1851NeighbourArmy& CareerArmy : NeighbourArmies)
		{
			if (CareerArmy.Nation == CareerNation.Key)
			{
				CareerArmy.OfficerId = CareerChoose(CareerArmy.OfficerId);
				const FCampaign1851Officer* CareerChief = ForeignCommander(CareerNation.Key, CareerArmy.OfficerId);
				CareerArmy.Chief = CareerChief ? CareerChief->Rank + TEXT(" ") + CareerChief->Name : FString(TEXT("Ledig chefspost"));
			}
		}
	}
}

void ACampaign1851Map::MonthlyOfficerCareers()
{
	const int32 CareerYear = GetDate().GetYear();
	const int32 CareerMonthKey = CareerYear * 12 + GetDate().GetMonth();
	auto CareerTick = [&](FCampaign1851Officer& CareerOfficer, bool bCareerCommand, bool bCareerField)
	{
		if (CareerOfficer.CareerStatus != 0) { return; }
		++CareerOfficer.RankMonths;
		if (CareerOfficer.Nation != TEXT("DK") && CareerOfficer.Away != 0 && GetDate() >= CareerOfficer.AwayUntil)
		{
			CareerOfficer.Away = 0;
			LogOfficerCareer(CareerOfficer, TEXT("Tilbage i tjeneste"));
		}
		if (bCareerCommand && CareerOfficer.Away == 0)
		{
			++CareerOfficer.CommandMonths;
			GainOfficerExperience(CareerOfficer, bCareerField ? 0.75f : 0.25f);
		}
		if (!bCareerCommand) { GainOfficerExperience(CareerOfficer, 0.1f); }
		FRandomStream CareerRng(int32(HashCombine(GetTypeHash(CareerOfficer.Id), uint32(Seed) ^ uint32(CareerMonthKey))));
		const int32 CareerAge = CareerOfficer.Born > 0 ? CareerYear - CareerOfficer.Born : 0;
		const float CareerRetireChance = (CareerAge >= 60 ? FMath::Min(0.20f, (CareerAge - 59) * 0.004f) : 0.f)
			+ (CareerOfficer.Away == 1 && CareerOfficer.Wounds >= 2 ? 0.02f : 0.f);
		const bool bCareerDies = CareerAge >= 45 && CareerRng.FRand() < (0.004f + (CareerAge - 45) * 0.002f) / 12.f;
		const bool bCareerRetires = !bCareerDies && CareerRng.FRand() < CareerRetireChance;
		if (!bCareerDies && !bCareerRetires)
		{
			if (CareerOfficer.Away == 0) { LogOfficerCareer(CareerOfficer, bCareerCommand ? TEXT("Månedlig kommandotjeneste") : TEXT("Månedlig reservetjeneste")); }
			return;
		}
		if (CareerOfficer.Nation == TEXT("DK"))
		{
			const int32 CareerIndex = Officers.IndexOfByPredicate([&](const FCampaign1851Officer& CareerCandidate) { return CareerCandidate.Id == CareerOfficer.Id; });
			if (CareerIndex != INDEX_NONE) { VacateOfficer(CareerIndex); }
		}
		CareerOfficer.CareerStatus = bCareerDies ? 2 : 1;
		CareerOfficer.Away = 0;
		LogOfficerCareer(CareerOfficer, bCareerDies ? TEXT("Død i tjenesten") : TEXT("Pensioneret på grund af alder eller sår"));
	};
	for (FCampaign1851Officer& CareerOfficer : Officers)
	{
		const bool bCareerCommand = CareerOfficer.Regiment != INDEX_NONE || CareerOfficer.Command != INDEX_NONE || CareerOfficer.Formation != 0 || CareerOfficer.CaptainOf != INDEX_NONE || CareerOfficer.StaffOf != 0;
		const int32 CareerRegiment = CareerOfficer.Regiment != INDEX_NONE ? CareerOfficer.Regiment : CareerOfficer.CaptainOf;
		bool bCareerField = Regiments.IsValidIndex(CareerRegiment) && Regiments[CareerRegiment].IsMarching();
		for (const FCampaign1851Regiment& CareerUnit : Regiments)
		{
			if (!CareerUnit.IsMarching()) { continue; }
			bCareerField |= (CareerOfficer.Command != INDEX_NONE && CareerUnit.Command == CareerOfficer.Command)
				|| (CareerOfficer.Formation != 0 && IsInside(CareerUnit.Formation, CareerOfficer.Formation))
				|| (CareerOfficer.StaffOf != 0 && IsInside(CareerUnit.Formation, CareerOfficer.StaffOf));
		}
		CareerTick(CareerOfficer, bCareerCommand, bCareerField);
	}
	// Ministry fills actual Danish vacancies from the best qualified reserve, then promotes
	// eligible officers one step. The player can use the same functions between month ends.
	auto CareerReserve = [&](const FString& CareerRequired) -> int32
	{
		int32 CareerBest = INDEX_NONE;
		for (int32 CareerIndex = 0; CareerIndex < Officers.Num(); ++CareerIndex)
		{
			const FCampaign1851Officer& CareerCandidate = Officers[CareerIndex];
			if (!CareerCandidate.IsFree() || CareerCandidate.Rank != CareerRequired) { continue; }
			if (CareerBest == INDEX_NONE || CareerCandidate.Experience + CareerCandidate.RankMonths * 0.1f > Officers[CareerBest].Experience + Officers[CareerBest].RankMonths * 0.1f) { CareerBest = CareerIndex; }
		}
		return CareerBest;
	};
	for (int32 CareerRegiment = 0; CareerRegiment < Regiments.Num(); ++CareerRegiment)
	{
		FCampaign1851Regiment& CareerUnit = Regiments[CareerRegiment];
		if (IsInBattle(CareerRegiment)) { continue; }
		if (CareerUnit.Chief == INDEX_NONE)
		{
			const FString CareerRequired = CareerUnit.Arm == ECampaign1851Arm::Cavalry ? TEXT("Oberst") :
				(CareerUnit.Arm == ECampaign1851Arm::Artillery || CareerUnit.Arm == ECampaign1851Arm::HorseArtillery) ? TEXT("Kaptajn") : TEXT("Major");
			const int32 CareerChoice = CareerReserve(CareerRequired);
			if (CareerChoice != INDEX_NONE) { AssignOfficer(CareerChoice, CareerRegiment); }
		}
		for (int32 CareerCompany = 0; CareerCompany < CareerUnit.Captains.Num(); ++CareerCompany)
		{
			if (CareerUnit.Captains[CareerCompany] != INDEX_NONE) { continue; }
			const int32 CareerChoice = CareerReserve(TEXT("Kaptajn"));
			if (CareerChoice == INDEX_NONE) { continue; }
			CareerUnit.Captains[CareerCompany] = CareerChoice;
			Officers[CareerChoice].CaptainOf = CareerRegiment;
			Officers[CareerChoice].Company = CareerCompany;
			LogOfficerCareer(Officers[CareerChoice], TEXT("Udnævnt til kompagnichef ved ") + CareerUnit.Name);
		}
	}
	for (int32 CareerCommand = 0; CareerCommand < Commands.Num(); ++CareerCommand)
	{
		if (Commands[CareerCommand].General != INDEX_NONE) { continue; }
		const int32 CareerChoice = CareerReserve(TEXT("Generalmajor"));
		if (CareerChoice != INDEX_NONE)
		{
			AssignCommandGeneral(CareerChoice, CareerCommand);
			LogOfficerCareer(Officers[CareerChoice], TEXT("Udnævnt til ledig generalkommando"));
		}
	}
	for (const FCampaign1851Formation& CareerFormation : Formations)
	{
		for (int32 CareerPost = 0; CareerPost < 3; ++CareerPost)
		{
			const int32 CareerPosts[] = {CareerFormation.Commander, CareerFormation.Deputy, CareerFormation.StaffChief};
			if (CareerPosts[CareerPost] != INDEX_NONE) { continue; }
			const TCHAR* CareerRequired = FormationPostRank(CareerFormation.Id, CareerPost);
			if (!CareerRequired) { continue; }
			const int32 CareerChoice = CareerReserve(CareerRequired);
			if (CareerChoice != INDEX_NONE)
			{
				AssignFormationStaff(CareerChoice, CareerFormation.Id, CareerPost);
				LogOfficerCareer(Officers[CareerChoice], TEXT("Udnævnt ved ") + CareerFormation.Name);
			}
		}
	}
	TArray<int32> CareerPromotionOrder;
	for (int32 CareerIndex = 0; CareerIndex < Officers.Num(); ++CareerIndex)
	{
		if (Officers[CareerIndex].IsFree()) { CareerPromotionOrder.Add(CareerIndex); }
	}
	CareerPromotionOrder.Sort([&](int32 CareerA, int32 CareerB)
	{
		const float CareerScoreA = Officers[CareerA].Experience + Officers[CareerA].RankMonths * 0.1f;
		const float CareerScoreB = Officers[CareerB].Experience + Officers[CareerB].RankMonths * 0.1f;
		return CareerScoreA == CareerScoreB ? CareerA < CareerB : CareerScoreA > CareerScoreB;
	});
	for (int32 CareerIndex : CareerPromotionOrder) { PromoteOfficer(CareerIndex); }
	for (auto& CareerNation : ForeignOfficers)
	{
		for (FCampaign1851Officer& CareerOfficer : CareerNation.Value)
		{
			bool bCareerCommand = ForeignCommanderIds.FindRef(CareerNation.Key) == CareerOfficer.Id;
			bool bCareerField = false;
			for (const FCampaign1851NeighbourArmy& CareerArmy : NeighbourArmies)
			{
				if (CareerArmy.Nation != CareerNation.Key || CareerArmy.OfficerId != CareerOfficer.Id) { continue; }
				bCareerCommand = true;
				const FCampaign1851EnemyCorps* CareerCorps = EnemyCorps.FindByPredicate([&](const FCampaign1851EnemyCorps& CareerCandidate) { return CareerCandidate.Id == CareerArmy.CorpsId && CareerCandidate.Men > 0; });
				bCareerField |= CareerCorps && CareerCorps->Route.IsValidIndex(CareerCorps->Leg);
			}
			CareerTick(CareerOfficer, bCareerCommand, bCareerField);
		}
	}
	ReplaceForeignOfficers();
	for (auto& CareerNation : ForeignOfficers)
	{
		// Each vacant army command requires a general. Promote one step at a time,
		// only while a command is empty; score combines seniority and earned merit.
		bool bCareerVacancy = !ForeignCommander(CareerNation.Key);
		for (const FCampaign1851NeighbourArmy& CareerArmy : NeighbourArmies)
		{
			if (CareerArmy.Nation == CareerNation.Key && !ForeignCommander(CareerNation.Key, CareerArmy.OfficerId)) { bCareerVacancy = true; }
		}
		if (bCareerVacancy)
		{
			int32 CareerBest = INDEX_NONE;
			for (int32 CareerIndex = 0; CareerIndex < CareerNation.Value.Num(); ++CareerIndex)
			{
				const FCampaign1851Officer& CareerCandidate = CareerNation.Value[CareerIndex];
				const int32 CareerNext = CareerRankIndex(CareerCandidate) + 1;
				if (CareerCandidate.CareerStatus != 0 || CareerCandidate.Away != 0 || CareerCandidate.bGeneral || CareerNext > 4 || CareerCandidate.Experience < Campaign1851Army::RankExperience(CareerNext)
					|| (CareerCandidate.RankMonths < 24 && !(CareerCandidate.RankMonths >= 12 && CareerCandidate.BattlesServed >= 3))) { continue; }
				if (CareerBest == INDEX_NONE || CareerCandidate.Experience + CareerCandidate.RankMonths * 0.1f > CareerNation.Value[CareerBest].Experience + CareerNation.Value[CareerBest].RankMonths * 0.1f) { CareerBest = CareerIndex; }
			}
			if (CareerBest != INDEX_NONE)
			{
				FCampaign1851Officer& CareerChosen = CareerNation.Value[CareerBest];
				const int32 CareerNext = CareerRankIndex(CareerChosen) + 1;
				CareerChosen.Rank = CareerNationRanks(CareerNation.Key)[CareerNext];
				CareerChosen.bGeneral = CareerNext >= 4;
				CareerChosen.RankMonths = 0;
				LogOfficerCareer(CareerChosen, TEXT("AI-forfremmelse til ledig post"));
			}
		}
	}
	ReplaceForeignOfficers();
}
