// Regiments on the 1851 campaign map: ACampaign1851Map's army (see Campaign1851Army.h).

#include "Campaign1851Army.h"

#include "Campaign1851Map.h"
#include "Algo/Reverse.h"
#include "Campaign1851Scenery.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const TCHAR* ArmyMaterialPath = TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery");
	/** Regiments are drawn this much larger than the town pieces, so a formation reads beside a town. */
	constexpr float FormationScale = 4.f;
	/** Camera distance (km) below which the formations are shown (the counters show at every distance). */
	constexpr float FormationMaxDistanceKm = 60.f;
}

namespace Campaign1851Army
{
	ECampaign1851Arm ParseArm(const FString& Text)
	{
		if (Text == TEXT("guard")) return ECampaign1851Arm::Guard;
		if (Text == TEXT("jager")) return ECampaign1851Arm::Jager;
		if (Text == TEXT("cavalry")) return ECampaign1851Arm::Cavalry;
		if (Text == TEXT("artillery")) return ECampaign1851Arm::Artillery;
		if (Text == TEXT("horseartillery")) return ECampaign1851Arm::HorseArtillery;
		return ECampaign1851Arm::Infantry;
	}

	const TCHAR* ArmName(ECampaign1851Arm Arm)
	{
		switch (Arm)
		{
		case ECampaign1851Arm::Guard: return TEXT("Garde");
		case ECampaign1851Arm::Jager: return TEXT("Jægere");
		case ECampaign1851Arm::Cavalry: return TEXT("Kavaleri");
		case ECampaign1851Arm::Artillery: return TEXT("Artilleri");
		case ECampaign1851Arm::HorseArtillery: return TEXT("Ridende artilleri");
		default: return TEXT("Linjeinfanteri");
		}
	}

	float MarchKmPerDay(ECampaign1851Arm Arm)
	{
		// Foot 20 km a day on a dirt road; horse 30; foot artillery (guns and limbers, gunners walking) 18;
		// horse artillery, all mounted with six-horse teams, 28.
		switch (Arm)
		{
		case ECampaign1851Arm::Cavalry: return 30.f;
		case ECampaign1851Arm::Artillery: return 18.f;
		case ECampaign1851Arm::HorseArtillery: return 28.f;
		default: return Campaign1851Network::MarchKmPerDayRoad;
		}
	}

	const TCHAR* StatName(ECampaign1851OfficerStat Stat)
	{
		static const TCHAR* Names[] = { TEXT("Føring"), TEXT("Inspiration"), TEXT("Initiativ"), TEXT("Taktik"), TEXT("Stab"), TEXT("Disciplin"), TEXT("Aggressivitet"), TEXT("Nerve"), TEXT("Politisk vægt"), TEXT("Forsigtighed") };
		return Names[FMath::Clamp(int32(Stat), 0, int32(ECampaign1851OfficerStat::Count) - 1)];
	}

	const TCHAR* StatShort(ECampaign1851OfficerStat Stat)
	{
		static const TCHAR* Names[] = { TEXT("Før"), TEXT("Insp"), TEXT("Init"), TEXT("Takt"), TEXT("Stab"), TEXT("Disc"), TEXT("Aggr"), TEXT("Nerve"), TEXT("Pol"), TEXT("Fors") };
		return Names[FMath::Clamp(int32(Stat), 0, int32(ECampaign1851OfficerStat::Count) - 1)];
	}

	const TCHAR* ExperienceName(float Experience)
	{
		return Experience < 20.f ? TEXT("Rekrutter") : Experience < 40.f ? TEXT("Øvede") : Experience < 60.f ? TEXT("Erfarne") : Experience < 80.f ? TEXT("Veteraner") : TEXT("Elite");
	}

	int32 Stars(float Experience)
	{
		return FMath::Clamp(FMath::FloorToInt(Experience / 20.f), 0, 5);
	}

	const TArray<FString>& Ranks()
	{
		static const TArray<FString> Names = { TEXT("Kaptajn"), TEXT("Major"), TEXT("Oberstløjtnant"), TEXT("Oberst"), TEXT("Generalmajor"), TEXT("Generalløjtnant"), TEXT("General") };
		return Names;
	}

	int32 RankIndex(const FString& Rank)
	{
		const int32 Index = Ranks().IndexOfByKey(Rank);
		return Index == INDEX_NONE ? 1 : Index;
	}

	int32 RankPay(int32 Rank)
	{
		static const int32 Pay[] = { 500, 600, 800, 1000, 3000, 4000, 5000 };
		return Pay[FMath::Clamp(Rank, 0, 6)];
	}

	float RankExperience(int32 Rank)
	{
		static const float Xp[] = { 0.f, 30.f, 40.f, 50.f, 60.f, 70.f, 80.f };
		return Xp[FMath::Clamp(Rank, 0, 6)];
	}

	int32 TrainsNeeded(const FCampaign1851Regiment& R)
	{
		switch (R.Arm)
		{
		case ECampaign1851Arm::Cavalry: return FMath::Max(1, FMath::DivideAndRoundUp(R.Horses, 250));
		case ECampaign1851Arm::Artillery:
		case ECampaign1851Arm::HorseArtillery: return 1;
		default: return FMath::Max(1, FMath::DivideAndRoundUp(R.Men, 800));
		}
	}

	const TCHAR* EchelonName(ECampaign1851Echelon Echelon)
	{
		switch (Echelon)
		{
		case ECampaign1851Echelon::Army: return TEXT("Hær");
		case ECampaign1851Echelon::Division: return TEXT("Division");
		case ECampaign1851Echelon::Regiment: return TEXT("Regiment");
		case ECampaign1851Echelon::Detachment: return TEXT("Afdeling");
		default: return TEXT("Brigade");
		}
	}

	const TCHAR* FormationRole(ECampaign1851Echelon Echelon)
	{
		switch (Echelon)
		{
		case ECampaign1851Echelon::Army: return TEXT("Øverstkommanderende");
		case ECampaign1851Echelon::Division: return TEXT("Divisionschef");
		case ECampaign1851Echelon::Regiment: return TEXT("Regimentschef");
		case ECampaign1851Echelon::Detachment: return TEXT("Afdelingschef");
		default: return TEXT("Brigadechef");
		}
	}

	const TCHAR* StaffPostName(ECampaign1851Echelon Echelon, int32 Post)
	{
		if (Post == 1)
		{
			return TEXT("Næstkommanderende");
		}
		if (Post == 2)
		{
			return Echelon == ECampaign1851Echelon::Division || Echelon == ECampaign1851Echelon::Army ? TEXT("Stabschef") : TEXT("Adjudant");
		}
		return FormationRole(Echelon);
	}

	const TCHAR* UnitRole(ECampaign1851Arm Arm)
	{
		switch (Arm)
		{
		case ECampaign1851Arm::Cavalry: return TEXT("Kavaleriofficer");
		case ECampaign1851Arm::Artillery:
		case ECampaign1851Arm::HorseArtillery: return TEXT("Batterichef");
		default: return TEXT("Bataljonschef");
		}
	}

	const TCHAR* ArmMark(ECampaign1851Arm Arm)
	{
		switch (Arm)
		{
		case ECampaign1851Arm::Cavalry: return TEXT("CAV");
		case ECampaign1851Arm::Artillery:
		case ECampaign1851Arm::HorseArtillery: return TEXT("ART");
		default: return TEXT("II");
		}
	}

	int32 CompaniesFor(ECampaign1851Arm Arm)
	{
		return Arm == ECampaign1851Arm::Infantry || Arm == ECampaign1851Arm::Jager || Arm == ECampaign1851Arm::Guard ? 4 : 0;
	}

	const TCHAR* EchelonMark(ECampaign1851Echelon Echelon)
	{
		switch (Echelon)
		{
		case ECampaign1851Echelon::Army: return TEXT("XXXX");
		case ECampaign1851Echelon::Division: return TEXT("XX");
		case ECampaign1851Echelon::Regiment: return TEXT("III");
		case ECampaign1851Echelon::Detachment: return TEXT("II");
		default: return TEXT("X");
		}
	}

	const TCHAR* RouteModeName(ECampaign1851RouteMode Mode)
	{
		return Mode == ECampaign1851RouteMode::RoadsOnly ? TEXT("kun veje") : Mode == ECampaign1851RouteMode::Direct ? TEXT("lige linje") : TEXT("veje og tog");
	}

	const TCHAR* SkillName(ECampaign1851Skill Skill)
	{
		static const TCHAR* Names[] = { TEXT("Ladegreb"), TEXT("Skydning"), TEXT("Eksercits"), TEXT("Feltøvelse"), TEXT("Udholdenhed"), TEXT("Bajonet/storm") };
		return Names[FMath::Clamp(int32(Skill), 0, int32(ECampaign1851Skill::Count) - 1)];
	}

	const TCHAR* ProgramName(ECampaign1851Program Program)
	{
		static const TCHAR* Names[] = { TEXT("Hvile"), TEXT("Eksercits"), TEXT("Skydeøvelser"), TEXT("Feltøvelser"), TEXT("Marchøvelser"), TEXT("Bajonet og storm"), TEXT("Blandet") };
		return Names[FMath::Clamp(int32(Program), 0, int32(ECampaign1851Program::Count) - 1)];
	}

	float ProgramWeight(ECampaign1851Program Program, ECampaign1851Skill Skill)
	{
		using S = ECampaign1851Skill;
		switch (Program)
		{
		case ECampaign1851Program::Drill: return Skill == S::Drill ? 1.f : Skill == S::Loading ? 0.5f : 0.f;
		case ECampaign1851Program::LiveFire: return Skill == S::Marksmanship ? 1.f : Skill == S::Loading ? 0.6f : 0.f;
		case ECampaign1851Program::Field: return Skill == S::Fieldcraft ? 1.f : Skill == S::Endurance ? 0.4f : Skill == S::Drill ? 0.3f : 0.f;
		case ECampaign1851Program::March: return Skill == S::Endurance ? 1.f : Skill == S::Fieldcraft ? 0.2f : 0.f;
		case ECampaign1851Program::Assault: return Skill == S::Assault ? 1.f : Skill == S::Drill ? 0.3f : Skill == S::Endurance ? 0.2f : 0.f;
		case ECampaign1851Program::Mixed: return 0.35f;
		default: return 0.f;
		}
	}

	float DaysForTen(ECampaign1851Program Program, ECampaign1851Skill Skill, float Current, int32 Leadership)
	{
		// Same rule as the daily training in AdvanceArmy, stepped a day at a time.
		const float Weight = ProgramWeight(Program, Skill);
		const float Cap = 60.f + 4.f * Leadership;
		if (Weight <= 0.f || Current + 10.f > Cap)
		{
			return 0.f;
		}
		float V = Current, Days = 0.f;
		while (V < Current + 10.f && Days < 5000.f)
		{
			V += 0.12f * Weight * (0.5f + Leadership / 10.f) * FMath::Max(0.1f, 1.f - V / 100.f);
			Days += 1.f;
		}
		return Days;
	}

	int32 ProgramCostPerMonth(ECampaign1851Program Program)
	{
		// Powder and ball are the dear part; field days wear boots, horses and kit.
		static const int32 Cost[] = { 0, 40, 300, 120, 60, 60, 150 };
		return Cost[FMath::Clamp(int32(Program), 0, int32(ECampaign1851Program::Count) - 1)];
	}

	int32 OfficerRating(const FCampaign1851Officer& O)
	{
		using S = ECampaign1851OfficerStat;
		const float Weights[int32(S::Count)] = { 1.5f, 1.0f, 1.0f, 1.5f, 1.0f, 0.8f, 0.0f, 1.0f, O.bGeneral ? 0.3f : 0.f, 0.6f };
		float Sum = 0.f, Weight = 0.f;
		for (int32 s = 0; s < int32(S::Count); ++s)
		{
			Sum += Weights[s] * O.Stats[s];
			Weight += Weights[s];
		}
		// Aggression: neither timid nor reckless.
		const float Agg = 10.f - 1.6f * FMath::Abs(O.Stat(S::Aggression) - 6.f);
		Sum += 0.6f * Agg;
		Weight += 0.6f;
		const float Mean = Sum / FMath::Max(Weight, 0.01f);   // 1-10
		return FMath::Clamp(FMath::RoundToInt((Mean - 1.f) / 9.f * 90.f + O.Experience * 0.1f), 0, 100);
	}

	FBattleFactors BattleFactors(const FCampaign1851Regiment& R)
	{
		auto Map = [](float Skill, float Low, float High) { return FMath::Lerp(Low, High, FMath::Clamp(Skill / 100.f, 0.f, 1.f)); };
		FBattleFactors F;
		F.ReloadTime = Map(R.Skill(ECampaign1851Skill::Loading), 1.3f, 0.85f);
		F.Accuracy = Map(R.Skill(ECampaign1851Skill::Marksmanship), 0.7f, 1.25f);
		F.DeploySpeed = Map(R.Skill(ECampaign1851Skill::Drill), 0.75f, 1.2f);
		F.Skirmish = Map(R.Skill(ECampaign1851Skill::Fieldcraft), 0.7f, 1.2f);
		F.FatigueRate = Map(R.Skill(ECampaign1851Skill::Endurance), 1.3f, 0.75f);
		F.Assault = Map(R.Skill(ECampaign1851Skill::Assault), 0.75f, 1.25f);
		F.Morale = R.Morale;
		F.Cohesion = R.Cohesion / 100.f;
		F.Experience = R.Experience / 100.f;
		return F;
	}

	float ColumnPace(const TArray<const FCampaign1851Regiment*>& Column, FString* OutWhy)
	{
		// Each unit's own pace: its arm, and how fit it is (endurance). The slowest sets the column's.
		float Pace = 1000.f;
		int32 Men = 0;
		const FCampaign1851Regiment* Slowest = nullptr;
		for (const FCampaign1851Regiment* R : Column)
		{
			Men += R->Men;
			// Mortars are heavy: slow on their wagons, very slow without enough of them.
			const float Mortar = R->Mortars > 0 ? (R->Wagons >= R->Mortars * 2 ? 0.8f : 0.5f) : 1.f;
			const float Own = MarchKmPerDay(R->Arm) * EndurancePaceFactor(R->Skill(ECampaign1851Skill::Endurance)) * Mortar;
			if (Own < Pace)
			{
				Pace = Own;
				Slowest = R;
			}
		}
		if (!Slowest)
		{
			return MarchKmPerDay(ECampaign1851Arm::Infantry);
		}
		const float Length = FMath::Clamp(1.f - 0.04f * (Men / 1000.f - 2.f), 0.75f, 1.f);
		if (OutWhy)
		{
			const float Fit = EndurancePaceFactor(Slowest->Skill(ECampaign1851Skill::Endurance));
			*OutWhy = Column.Num() > 1 ? FString::Printf(TEXT("%s bestemmer tempoet"), *Slowest->Name) : FString();
			if (FMath::Abs(Fit - 1.f) > 0.005f)
			{
				*OutWhy += FString::Printf(TEXT("%sudholdenhed %.0f: %+d %%"), OutWhy->IsEmpty() ? TEXT("") : TEXT("  ·  "),
					Slowest->Skill(ECampaign1851Skill::Endurance), FMath::RoundToInt((Fit - 1.f) * 100.f));
			}
			if (Length < 0.995f)
			{
				*OutWhy += FString::Printf(TEXT("%slang kolonne (%d mand): -%d %%"), OutWhy->IsEmpty() ? TEXT("") : TEXT("  ·  "),
					Men, FMath::RoundToInt((1.f - Length) * 100.f));
			}
		}
		return Pace * Length;
	}
}

// ------------------------------------------------------------------ data

bool ACampaign1851Map::LoadArmy()
{
	FString Text;
	TSharedPtr<FJsonObject> Json;
	const FString ArmyDataFile = ActiveScenario().Id == TEXT("1825") ? TEXT("Data/Campaign1851/Army_1825.json") : TEXT("Data/Campaign1851/Army1851.json");
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / ArmyDataFile))
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|army|cannot load %s"), *ArmyDataFile);
		return false;
	}
	ArmyScenarioEquipment.Reset();
	ArmyScenarioMagazines.Reset();
	const TSharedPtr<FJsonObject>* ArmyEquipmentObject = nullptr;
	if (Json->TryGetObjectField(TEXT("equipment"), ArmyEquipmentObject))
	{
		ArmyScenarioEquipment = *ArmyEquipmentObject;
	}
	const TArray<TSharedPtr<FJsonValue>>* ArmyMagazineArray = nullptr;
	if (Json->TryGetArrayField(TEXT("magazines"), ArmyMagazineArray))
	{
		for (const TSharedPtr<FJsonValue>& V : *ArmyMagazineArray)
		{
			const TSharedPtr<FJsonObject> M = V->AsObject();
			FCampaign1851DepotCapacity Cap;
			Cap.Food = float(M->GetNumberField(TEXT("food")));
			Cap.Fodder = float(M->GetNumberField(TEXT("fodder")));
			Cap.Ammo = float(M->GetNumberField(TEXT("ammo")));
			ArmyScenarioMagazines.Add(M->GetStringField(TEXT("town")), Cap);
		}
	}
	ArmyAtStart.Reset();
	FString Nation = TEXT("DK");
	Json->TryGetStringField(TEXT("nation"), Nation);
	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("regiments")))
	{
		const TSharedPtr<FJsonObject> O = Value->AsObject();
		FCampaign1851Regiment R;
		R.Id = O->GetStringField(TEXT("id"));
		R.Name = O->GetStringField(TEXT("name"));
		R.Nation = Nation;
		R.Arm = Campaign1851Army::ParseArm(O->GetStringField(TEXT("arm")));
		R.Home = FindCity(O->GetStringField(TEXT("home")));
		if (R.Home == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|army|%s: unknown town %s"), *R.Name, *O->GetStringField(TEXT("home")));
			continue;
		}
		R.Men = R.MaxMen = int32(O->GetNumberField(TEXT("men")));
		O->TryGetNumberField(TEXT("guns"), R.Guns);
		R.Present = ArmyPeacePresent(R.Arm);
		O->TryGetNumberField(TEXT("companies"), R.SavedCompanies);
		O->TryGetNumberField(TEXT("horses"), R.Horses);
		R.MaxHorses = R.Horses;
		O->TryGetNumberField(TEXT("guns"), R.Guns);
		R.Town = R.Home;
		// The army of 1851 has just come out of a war: seasoned, drilled; arms have their strengths.
		double Xp = 55.0;
		O->TryGetNumberField(TEXT("experience"), Xp);
		R.Experience = float(Xp);
		const TSharedPtr<FJsonObject>* SkillObj = nullptr;
		if (O->TryGetObjectField(TEXT("skills"), SkillObj))
		{
			static const TCHAR* Keys[] = { TEXT("loading"), TEXT("marksmanship"), TEXT("drill"), TEXT("fieldcraft"), TEXT("endurance"), TEXT("assault") };
			for (int32 s = 0; s < int32(ECampaign1851Skill::Count); ++s)
			{
				double V = R.Skills[s];
				(*SkillObj)->TryGetNumberField(Keys[s], V);
				R.Skills[s] = float(V);
			}
		}
		ArmyAtStart.Add(MoveTemp(R));
	}
	// General commands: the towns of each; a regiment belongs to the command of its garrison.
	CommandsAtStart.Reset();
	CommandGeneralIds.Reset();
	const TArray<TSharedPtr<FJsonValue>>* CommandArray = nullptr;
	if (Json->TryGetArrayField(TEXT("commands"), CommandArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *CommandArray)
		{
			const TSharedPtr<FJsonObject> O = Value->AsObject();
			FCampaign1851Command C;
			C.Id = O->GetStringField(TEXT("id"));
			C.Name = O->GetStringField(TEXT("name"));
			O->TryGetStringField(TEXT("area"), C.Area);
			C.HQ = FindCity(O->GetStringField(TEXT("hq")));
			TArray<FString> Towns;
			O->TryGetStringArrayField(TEXT("towns"), Towns);
			for (const FString& T : Towns)
			{
				const int32 Index = FindCity(T);
				if (Index != INDEX_NONE)
				{
					C.Towns.Add(Index);
				}
			}
			FString GeneralId;
			O->TryGetStringField(TEXT("general"), GeneralId);
			CommandGeneralIds.Add(GeneralId);
			CommandsAtStart.Add(C);
		}
	}
	for (FCampaign1851Regiment& R : ArmyAtStart)
	{
		R.Command = CommandsAtStart.IndexOfByPredicate([&R](const FCampaign1851Command& C) { return C.Towns.Contains(R.Home); });
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|army|%d regiments, %d general commands"), ArmyAtStart.Num(), CommandsAtStart.Num());
	return ArmyAtStart.Num() > 0;
}

void ACampaign1851Map::ResetArmy()
{
	// Pieces of battalions raised in the last campaign go with them.
	for (int32 i = ArmyAtStart.Num(); i < RegimentPieces.Num(); ++i)
	{
		if (RegimentPieces[i])
		{
			RegimentPieces[i]->DestroyComponent();
		}
	}
	for (int32 c = ArmyAtStart.Num() * 3; c < RegimentCars.Num(); ++c)
	{
		if (RegimentCars[c])
		{
			RegimentCars[c]->DestroyComponent();
		}
	}
	RegimentPieces.SetNum(FMath::Min(RegimentPieces.Num(), ArmyAtStart.Num()));
	RegimentCars.SetNum(FMath::Min(RegimentCars.Num(), ArmyAtStart.Num() * 3));
	Regiments = ArmyAtStart;
	Commands = CommandsAtStart;
	// The regiments' experience in this campaign: the 1851 figures, varied by the historical deviation.
	FRandomStream Rng(Seed + 7);
	for (FCampaign1851Regiment& R : Regiments)
	{
		R.Experience = FMath::Clamp(R.Experience + 15.f * Deviation * Rng.FRandRange(-1.f, 1.f), 5.f, 95.f);
	}
	Formations.Reset();
	NextFormationId = 1;
	ResetOfficers();
	ResetTroopTrains();
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		PlaceInTown(i);
	}
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		UpdateRegimentPiece(i);
	}
}

double ACampaign1851Map::ArmyEquipmentNumber(const TCHAR* Key, double Fallback) const
{
	double Value = Fallback;
	if (ArmyScenarioEquipment.IsValid()) { ArmyScenarioEquipment->TryGetNumberField(Key, Value); }
	return Value;
}

float ACampaign1851Map::ArmyPeacePresent(ECampaign1851Arm Arm) const
{
	if (ActiveScenario().Id != TEXT("1825")) { return Campaign1851Mobilisation::PeacePresent; }
	if (Arm == ECampaign1851Arm::Guard) { return 1.f; }
	if (Arm == ECampaign1851Arm::Cavalry) { return 0.6f; }
	if (Arm == ECampaign1851Arm::Artillery || Arm == ECampaign1851Arm::HorseArtillery) { return 0.65f; }
	return 0.35f;
}

FString ACampaign1851Map::ArmyWeaponText(ECampaign1851Arm Arm) const
{
	if (!ArmyScenarioEquipment.IsValid()) { return FString(); }
	const bool bGuns = Arm == ECampaign1851Arm::Artillery || Arm == ECampaign1851Arm::HorseArtillery;
	if (bGuns && HasResearch(TEXT("riflegun"))) { return TEXT("Riflet forladeskyts (udforsket)"); }
	if (!bGuns && Arm != ECampaign1851Arm::Cavalry && HasResearch(TEXT("breech"))) { return TEXT("Bagladegevær (udforsket)"); }
	FString Value;
	ArmyScenarioEquipment->TryGetStringField(bGuns ? TEXT("artilleryWeapon") : Arm == ECampaign1851Arm::Cavalry ? TEXT("cavalryWeapon") : TEXT("infantryWeapon"), Value);
	return Value;
}

FString ACampaign1851Map::ArmyUniformText(ECampaign1851Arm Arm) const
{
	TArray<FString> Values;
	if (ArmyScenarioEquipment.IsValid()) { ArmyScenarioEquipment->TryGetStringArrayField(TEXT("uniforms"), Values); }
	return Values.IsValidIndex(int32(Arm)) ? Values[int32(Arm)] : FString();
}

Campaign1851Army::FBattleFactors ACampaign1851Map::ArmyBattleFactors(const FCampaign1851Regiment& R) const
{
	Campaign1851Army::FBattleFactors F = Campaign1851Army::BattleFactors(R);
	if (R.Arm == ECampaign1851Arm::Infantry || R.Arm == ECampaign1851Arm::Guard || R.Arm == ECampaign1851Arm::Jager)
	{
		// A researched breechloader replaces the flintlock; do not multiply its bonus by the obsolete weapon.
		if (!HasResearch(TEXT("breech")))
		{
			F.ReloadTime *= float(ArmyEquipmentNumber(TEXT("infantryReload"), 1.0));
			F.Accuracy *= float(ArmyEquipmentNumber(TEXT("infantryAccuracy"), 1.0));
		}
	}
	return F;
}

// ------------------------------------------------------------------ queries

TArray<int32> ACampaign1851Map::RegimentsIn(int32 CityIndex) const
{
	TArray<int32> Out;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		if (Regiments[i].Town == CityIndex && !Regiments[i].IsMarching())
		{
			Out.Add(i);
		}
	}
	return Out;
}

TArray<FVector2D> ACampaign1851Map::LegLine(const FCampaign1851Leg& Leg) const
{
	if (Leg.bOffRoad || !Links.IsValidIndex(Leg.Link))
	{
		return { Leg.FromKm, Leg.ToKm };
	}
	const FCampaign1851Link& L = Links[Leg.Link];
	TArray<FVector2D> Line = Leg.bRail ? L.RailPath : L.Km;
	if (L.A != Leg.From)
	{
		Algo::Reverse(Line);
	}
	if (Leg.LineTo >= 0.f)
	{
		// Only the first part of the road: up to where the column turns off into the fields.
		TArray<FVector2D> Part = { Line[0] };
		double At = 0.0;
		for (int32 i = 0; i + 1 < Line.Num(); ++i)
		{
			const double Len = FVector2D::Distance(Line[i], Line[i + 1]);
			if (At + Len >= Leg.LineTo)
			{
				Part.Add(FMath::Lerp(Line[i], Line[i + 1], Len > 0.0 ? (Leg.LineTo - At) / Len : 0.0));
				break;
			}
			Part.Add(Line[i + 1]);
			At += Len;
		}
		return Part;
	}
	return Line;
}

FVector2D ACampaign1851Map::TownKm(int32 CityIndex) const
{
	return Cities.IsValidIndex(CityIndex) ? Extent.Projection.Forward(Cities[CityIndex].Lat, Cities[CityIndex].Lon) : FVector2D::ZeroVector;
}

FVector2D ACampaign1851Map::KmAtWorld(const FVector& World) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(World);
	return FVector2D(L.X / KmToUnits + SizeKm.X * 0.5 + Extent.XMin, -L.Y / KmToUnits + SizeKm.Y * 0.5 + Extent.YMin);
}

bool ACampaign1851Map::IsDryLine(const FVector2D& A, const FVector2D& B) const
{
	// Across the monarchy's land; a brook or a pond (up to ~300 m of it) does not stop a column.
	const double Length = FVector2D::Distance(A, B);
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(Length / 0.1));
	int32 Wet = 0;
	for (int32 s = 0; s <= Steps; ++s)
	{
		Wet += IsMonarchyLand(FMath::Lerp(A, B, double(s) / Steps)) ? 0 : 1;
	}
	return Wet * 0.1 <= 0.3;
}

int32 ACampaign1851Map::NearestTownFrom(const FVector2D& Km) const
{
	TArray<TPair<double, int32>> Near;
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		if (!Cities[c].bForeign && !Cities[c].bBornholm)
		{
			Near.Add({ FVector2D::Distance(Km, TownKm(c)), c });
		}
	}
	Near.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
	for (int32 n = 0; n < Near.Num() && n < 6; ++n)
	{
		if (Near[n].Key < 60.0 && IsDryLine(Km, TownKm(Near[n].Value)))
		{
			return Near[n].Value;
		}
	}
	return INDEX_NONE;
}

FString ACampaign1851Map::DescribePlace(int32 CityIndex, const FVector2D& Km) const
{
	if (Cities.IsValidIndex(CityIndex))
	{
		return Cities[CityIndex].Name;
	}
	int32 Best = INDEX_NONE;
	double BestKm = 1e9;
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		const double D = FVector2D::Distance(Km, TownKm(c));
		if (!Cities[c].bForeign && D < BestKm)
		{
			BestKm = D;
			Best = c;
		}
	}
	return Best == INDEX_NONE ? FString(TEXT("i terrænet")) : FString::Printf(TEXT("terrænet %.0f km fra %s"), BestKm, *Cities[Best].Name);
}

void ACampaign1851Map::TravelTimes(int32 From, float Pace, bool bRail, TArray<float>& OutDays, TArray<FCampaign1851Leg>& OutVia) const
{
	// Dijkstra over the towns; a leg's cost is its time. Boarding a train costs a day of loading.
	auto LegFor = [&](int32 Link, int32 At, bool bArrivedByRail)
	{
		const FCampaign1851Link& L = Links[Link];
		FCampaign1851Leg Leg;
		Leg.Link = Link;
		Leg.From = At;
		Leg.To = L.A == At ? L.B : L.A;
		Leg.FromKm = TownKm(Leg.From);
		Leg.ToKm = TownKm(Leg.To);
		Leg.bRail = bRail && L.bRailway;
		Leg.Days = Leg.bRail
			? L.RailKm / Campaign1851Network::TrainKmPerDay + (bArrivedByRail ? 0.f : Campaign1851Network::TrainLoadingDays)
			: L.RoadKm / (Pace * (L.bChaussee ? Campaign1851Network::MarchKmPerDayChaussee / Campaign1851Network::MarchKmPerDayRoad : 1.f))
				+ (L.HasFerry() ? Campaign1851Network::FerryDays : 0.f);
		return Leg;
	};
	OutDays.Init(TNumericLimits<float>::Max(), Cities.Num());
	OutVia.Reset();
	OutVia.SetNum(Cities.Num());
	TArray<bool> Done;
	Done.Init(false, Cities.Num());
	if (!Cities.IsValidIndex(From))
	{
		return;
	}
	OutDays[From] = 0.f;
	for (;;)
	{
		int32 At = INDEX_NONE;
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			if (!Done[c] && OutDays[c] < TNumericLimits<float>::Max() && (At == INDEX_NONE || OutDays[c] < OutDays[At]))
			{
				At = c;
			}
		}
		if (At == INDEX_NONE)
		{
			break;
		}
		Done[At] = true;
		const bool bByRail = At != From && OutVia[At].bRail;
		for (int32 Link = 0; Link < Links.Num(); ++Link)
		{
			if ((Links[Link].A != At && Links[Link].B != At) || Links[Link].bBlocked)
			{
				continue;
			}
			const FCampaign1851Leg Leg = LegFor(Link, At, bByRail);
			if (OutDays[At] + Leg.Days < OutDays[Leg.To])
			{
				OutDays[Leg.To] = OutDays[At] + Leg.Days;
				OutVia[Leg.To] = Leg;
			}
		}
	}
}

bool ACampaign1851Map::FindRoute(int32 From, int32 To, float Pace, TArray<FCampaign1851Leg>& OutLegs, bool bRail) const
{
	OutLegs.Reset();
	if (!Cities.IsValidIndex(From) || !Cities.IsValidIndex(To) || From == To)
	{
		return false;
	}
	TArray<float> Days;
	TArray<FCampaign1851Leg> Via;
	TravelTimes(From, Pace, bRail, Days, Via);
	if (Days[To] == TNumericLimits<float>::Max())
	{
		return false;
	}
	for (int32 At = To; At != From; At = Via[At].From)
	{
		OutLegs.Insert(Via[At], 0);
	}
	return true;
}

bool ACampaign1851Map::PlanMarch(int32 FromTown, const FVector2D& FromKm, int32 ToTown, const FVector2D& ToKm, float Pace, ECampaign1851RouteMode Mode,
	TArray<FCampaign1851Leg>& OutLegs, FString* OutReason) const
{
	OutLegs.Reset();
	const FVector2D StartKm = Cities.IsValidIndex(FromTown) ? TownKm(FromTown) : FromKm;
	const FVector2D EndKm = Cities.IsValidIndex(ToTown) ? TownKm(ToTown) : ToKm;
	const float FieldPace = Pace * Campaign1851Army::OffRoadPaceFactor;
	auto Across = [&](int32 A, const FVector2D& AKm, int32 B, const FVector2D& BKm)
	{
		FCampaign1851Leg Leg;
		Leg.bOffRoad = true;
		Leg.From = A;
		Leg.To = B;
		Leg.FromKm = AKm;
		Leg.ToKm = BKm;
		Leg.Days = float(FVector2D::Distance(AKm, BKm)) / FieldPace;
		return Leg;
	};
	if ((FromTown != INDEX_NONE && FromTown == ToTown) || FVector2D::Distance(StartKm, EndKm) < 0.05)
	{
		if (OutReason) { *OutReason = TEXT("Den er der allerede"); }
		return false;
	}
	const bool bDirectDry = IsDryLine(StartKm, EndKm);
	if (Mode == ECampaign1851RouteMode::Direct)
	{
		if (!bDirectDry)
		{
			if (OutReason) { *OutReason = TEXT("Der er vand i vejen: vælg veje"); }
			return false;
		}
		OutLegs.Add(Across(FromTown, StartKm, ToTown, EndKm));
		return true;
	}

	// By road: every way of joining the roads (a town near the start) and of leaving them (a town, or
	// any point along a road) is weighed, and the fastest whole march wins, the fields included.
	const bool bRail = Mode == ECampaign1851RouteMode::RoadsAndRail;
	struct FEntry { int32 Town; float Days; };
	TArray<FEntry> Entries;
	if (Cities.IsValidIndex(FromTown))
	{
		Entries.Add({ FromTown, 0.f });
	}
	else
	{
		TArray<TPair<double, int32>> Near;
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			if (!Cities[c].bForeign && !Cities[c].bBornholm && FVector2D::Distance(StartKm, TownKm(c)) < 40.0)
			{
				Near.Add({ FVector2D::Distance(StartKm, TownKm(c)), c });
			}
		}
		Near.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
		for (int32 n = 0; n < Near.Num() && Entries.Num() < 4; ++n)
		{
			if (IsDryLine(StartKm, TownKm(Near[n].Value)))
			{
				Entries.Add({ Near[n].Value, float(Near[n].Key) / FieldPace });
			}
		}
	}

	struct FPlan
	{
		float Days = TNumericLimits<float>::Max();
		int32 Entry = INDEX_NONE;     // town the roads start from
		int32 Exit = INDEX_NONE;      // town the roads reach (the partial link starts there)
		int32 Link = INDEX_NONE;      // leaving part-way along this link, at Along km from Exit
		float Along = 0.f;
		FVector2D LeaveKm = FVector2D::ZeroVector;
		bool bDirect = false;
	};
	FPlan Best;
	if (bDirectDry)
	{
		Best.Days = float(FVector2D::Distance(StartKm, EndKm)) / FieldPace;
		Best.bDirect = true;
	}
	TArray<float> BestDays;
	TArray<FCampaign1851Leg> BestVia;
	for (const FEntry& E : Entries)
	{
		TArray<float> Days;
		TArray<FCampaign1851Leg> Via;
		TravelTimes(E.Town, Pace, bRail, Days, Via);
		// Candidates, cheapest first; the (slower) dry-land test only for those that could win.
		TArray<FPlan> Candidates;
		if (Cities.IsValidIndex(ToTown))
		{
			if (Days[ToTown] < TNumericLimits<float>::Max())
			{
				FPlan P;
				P.Days = E.Days + Days[ToTown];
				P.Entry = E.Town;
				P.Exit = ToTown;
				Candidates.Add(P);
			}
		}
		else
		{
			for (int32 c = 0; c < Cities.Num(); ++c)
			{
				const double Off = FVector2D::Distance(TownKm(c), EndKm);
				if (Days[c] < TNumericLimits<float>::Max() && Off < 40.0)
				{
					FPlan P;
					P.Days = E.Days + Days[c] + float(Off) / FieldPace;
					P.Entry = E.Town;
					P.Exit = c;
					P.LeaveKm = TownKm(c);
					Candidates.Add(P);
				}
			}
			// Leaving a road part-way: sample every road link each kilometre from both ends.
			for (int32 k = 0; k < Links.Num(); ++k)
			{
				const FCampaign1851Link& L = Links[k];
				if (L.HasFerry() || L.bBlocked)
				{
					continue;
				}
				const float RoadPace = Pace * (L.bChaussee ? Campaign1851Network::MarchKmPerDayChaussee / Campaign1851Network::MarchKmPerDayRoad : 1.f);
				const double Length = LineLength(L.Km);
				for (double d = 1.0; d < Length; d += 1.0)
				{
					const FVector2D Point = AlongLine(L.Km, d);
					const double Off = FVector2D::Distance(Point, EndKm);
					if (Off > 40.0)
					{
						continue;
					}
					for (int32 Side = 0; Side < 2; ++Side)
					{
						const int32 Town = Side == 0 ? L.A : L.B;
						const double Along = Side == 0 ? d : Length - d;
						if (Days[Town] == TNumericLimits<float>::Max())
						{
							continue;
						}
						FPlan P;
						P.Days = E.Days + Days[Town] + float(Along) / RoadPace + float(Off) / FieldPace;
						P.Entry = E.Town;
						P.Exit = Town;
						P.Link = k;
						P.Along = float(Along);
						P.LeaveKm = Point;
						Candidates.Add(P);
					}
				}
			}
		}
		Candidates.Sort([](const FPlan& A, const FPlan& B) { return A.Days < B.Days; });
		for (const FPlan& P : Candidates)
		{
			if (P.Days >= Best.Days)
			{
				break;
			}
			if (Cities.IsValidIndex(ToTown) || IsDryLine(P.LeaveKm, EndKm))
			{
				Best = P;
				BestDays = Days;
				BestVia = Via;
				break;
			}
		}
	}
	if (Best.Days == TNumericLimits<float>::Max())
	{
		if (OutReason) { *OutReason = TEXT("Ingen vej derhen"); }
		return false;
	}
	if (Best.bDirect)
	{
		OutLegs.Add(Across(FromTown, StartKm, ToTown, EndKm));
		return true;
	}
	// Build it: fields to the entry town, roads to the exit town, part of a road, fields to the goal.
	if (!Cities.IsValidIndex(FromTown))
	{
		OutLegs.Add(Across(INDEX_NONE, StartKm, Best.Entry, TownKm(Best.Entry)));
	}
	TArray<FCampaign1851Leg> RoadLegs;
	for (int32 At = Best.Exit; At != Best.Entry; At = BestVia[At].From)
	{
		RoadLegs.Insert(BestVia[At], 0);
	}
	OutLegs.Append(RoadLegs);
	if (Links.IsValidIndex(Best.Link))
	{
		const FCampaign1851Link& L = Links[Best.Link];
		FCampaign1851Leg Part;
		Part.Link = Best.Link;
		Part.From = Best.Exit;
		Part.To = INDEX_NONE;
		Part.FromKm = TownKm(Best.Exit);
		Part.ToKm = Best.LeaveKm;
		Part.LineTo = Best.Along;
		Part.Days = Best.Along / (Pace * (L.bChaussee ? Campaign1851Network::MarchKmPerDayChaussee / Campaign1851Network::MarchKmPerDayRoad : 1.f));
		OutLegs.Add(Part);
		OutLegs.Add(Across(INDEX_NONE, Best.LeaveKm, INDEX_NONE, EndKm));
	}
	else if (!Cities.IsValidIndex(ToTown))
	{
		OutLegs.Add(Across(Best.Exit, TownKm(Best.Exit), INDEX_NONE, EndKm));
	}
	return OutLegs.Num() > 0;
}

FVector2D ACampaign1851Map::ShownKm(int32 Regiment) const
{
	// Units standing together are drawn side by side across their heading (first in the middle), so the
	// miniatures and their labels do not sit on top of each other.
	const FCampaign1851Regiment& R = Regiments[Regiment];
	if (R.IsMarching() && R.Route[R.Leg].bRail)
	{
		return R.Km;
	}
	int32 Slot = 0;
	for (int32 j = 0; j < Regiment; ++j)
	{
		const FCampaign1851Regiment& O = Regiments[j];
		if (O.Men > 0 && !(O.IsMarching() && O.Route[O.Leg].bRail) && FVector2D::Distance(O.Km, R.Km) < 0.35)
		{
			++Slot;
		}
	}
	if (Slot == 0)
	{
		return R.Km;
	}
	const FVector2D Heading = R.Heading.IsNearlyZero() ? FVector2D(1.0, 0.0) : R.Heading.GetSafeNormal();
	const FVector2D Side(-Heading.Y, Heading.X);
	const double Step = 0.62 * ((Slot + 1) / 2) * (Slot % 2 == 1 ? 1.0 : -1.0);
	return R.Km + Side * Step;
}

FVector ACampaign1851Map::RegimentWorld(int32 Regiment) const
{
	return Regiments.IsValidIndex(Regiment) ? WorldAtKm(ShownKm(Regiment)) : FVector::ZeroVector;
}

// ------------------------------------------------------------------ orders

bool ACampaign1851Map::OrderMarch(const TArray<int32>& Column, int32 CityIndex, FString* OutReason)
{
	return OrderMarchTo(Column, CityIndex, TownKm(CityIndex), ECampaign1851RouteMode::RoadsAndRail, OutReason);
}

bool ACampaign1851Map::OrderMarchTo(const TArray<int32>& Column, int32 CityIndex, const FVector2D& TargetKm, ECampaign1851RouteMode Mode, FString* OutReason)
{
	for (int32 Index : Column)
	{
		if (Regiments.IsValidIndex(Index) && Regiments[Index].bTraining)
		{
			if (OutReason) { *OutReason = TEXT("Enheden træner i garnison. Vælg INDSÆT TIDLIGT først."); }
			return false;
		}
	}
	const FCampaign1851MarchPlan Plan = PlanColumn(Column, CityIndex, TargetKm, Mode);
	if (!Plan.bOk)
	{
		if (OutReason) { *OutReason = Plan.Note; }
		return false;
	}
	static int32 NextGroup = 1;
	const int32 Group = Column.Num() > 1 ? NextGroup++ : 0;
	const FCampaign1851Regiment& Lead = Regiments[Column[0]];
	const bool bLeadMarching = Lead.IsMarching();
	const FVector2D LeadStart = bLeadMarching ? Lead.Route[Lead.Leg].ToKm : Lead.Km;
	int32 Ordered = 0;
	for (int32 i : Column)
	{
		if (!Regiments.IsValidIndex(i))
		{
			continue;
		}
		FCampaign1851Regiment& R = Regiments[i];
		const bool bMarching = R.IsMarching();
		const int32 StartTown = bMarching ? R.Route[R.Leg].To : R.Town;
		const FVector2D StartKm = bMarching ? R.Route[R.Leg].ToKm : R.Km;
		// Regiments starting where the lead does share its route (and its trains); any others march on their own.
		TArray<FCampaign1851Leg> Legs;
		const bool bWithLead = bMarching == bLeadMarching && (StartTown == (bLeadMarching ? Lead.Route[Lead.Leg].To : Lead.Town)) && (StartTown != INDEX_NONE || FVector2D::Distance(StartKm, LeadStart) < 0.3);
		const bool bThere = bMarching && ((CityIndex != INDEX_NONE && StartTown == CityIndex) || (CityIndex == INDEX_NONE && FVector2D::Distance(StartKm, TargetKm) < 0.05));
		if (bWithLead)
		{
			Legs = Plan.Route;
		}
		else if (!bThere && !PlanMarch(StartTown, StartKm, CityIndex, TargetKm, Plan.Pace, ECampaign1851RouteMode::RoadsOnly, Legs, OutReason))
		{
			continue;
		}
		R.PaceKmPerDay = Plan.Pace;
		R.Group = Group;
		R.Mode = Plan.Mode;
		if (bMarching)
		{
			TArray<FCampaign1851Leg> Route = { R.Route[R.Leg] };
			Route.Append(Legs);
			R.Route = MoveTemp(Route);
			R.Leg = 0;
		}
		else
		{
			R.OriginTown = R.Town;
			R.OriginKm = Cities.IsValidIndex(R.Town) ? TownKm(R.Town) : R.Km;
			R.Route = MoveTemp(Legs);
			R.Leg = 0;
			R.LegElapsed = 0.f;
			R.Town = INDEX_NONE;
			R.Km = R.Route[0].FromKm;   // from the town itself (not the camp), or from where it stands
		}
		++Ordered;
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|army|%s marches to %s (%s)|%d legs|%.1f days|pace %.1f km/day"), *R.Name, *DescribePlace(CityIndex, TargetKm),
			Campaign1851Army::RouteModeName(Plan.Mode), R.Route.Num(), R.DaysLeft(), Plan.Pace);
		UpdateRegimentPiece(i);
	}
	// The trains set off towards the boarding station.
	for (int32 n = 0; n < Plan.Trains.Num(); ++n)
	{
		FCampaign1851TroopTrain& T = TroopTrainList[Plan.Trains[n]];
		T.Lead = Column[0];
		T.Board = Plan.Board;
		T.Release = Plan.Release;
		T.bBoarded = false;
		T.Path = Plan.TrainPaths[n];
		T.PathLeg = 0;
		T.PathElapsed = 0.f;
		if (T.Path.Num() > 0)
		{
			T.Station = INDEX_NONE;
		}
	}
	OrderNote = Plan.Note.IsEmpty() && Plan.Trains.Num() > 0
		? FString::Printf(TEXT("%s%s"), *Plan.TrainSource, Plan.WaitDays > 0.01f ? *FString::Printf(TEXT("  ·  kolonnen venter %s på togene"), *FormatDuration(Plan.WaitDays)) : TEXT(""))
		: Plan.Note;
	UpdateTroopTrainPieces();
	return Ordered > 0;
}

// ------------------------------------------------------------------ troop trains

void ACampaign1851Map::RailTimes(int32 From, TArray<float>& OutDays, TArray<FCampaign1851Leg>& OutVia) const
{
	// Dijkstra over the open railways only (an empty train needs no loading).
	OutDays.Init(TNumericLimits<float>::Max(), Cities.Num());
	OutVia.Reset();
	OutVia.SetNum(Cities.Num());
	if (!Cities.IsValidIndex(From))
	{
		return;
	}
	TArray<bool> Done;
	Done.Init(false, Cities.Num());
	OutDays[From] = 0.f;
	for (;;)
	{
		int32 At = INDEX_NONE;
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			if (!Done[c] && OutDays[c] < TNumericLimits<float>::Max() && (At == INDEX_NONE || OutDays[c] < OutDays[At]))
			{
				At = c;
			}
		}
		if (At == INDEX_NONE)
		{
			break;
		}
		Done[At] = true;
		for (int32 k = 0; k < Links.Num(); ++k)
		{
			const FCampaign1851Link& L = Links[k];
			if (!L.bRailway || L.bBlocked || (L.A != At && L.B != At))
			{
				continue;
			}
			FCampaign1851Leg Leg;
			Leg.Link = k;
			Leg.From = At;
			Leg.To = L.A == At ? L.B : L.A;
			Leg.FromKm = TownKm(Leg.From);
			Leg.ToKm = TownKm(Leg.To);
			Leg.bRail = true;
			Leg.Days = L.RailKm / Campaign1851Network::TrainKmPerDay;
			if (OutDays[At] + Leg.Days < OutDays[Leg.To])
			{
				OutDays[Leg.To] = OutDays[At] + Leg.Days;
				OutVia[Leg.To] = Leg;
			}
		}
	}
}

FString ACampaign1851Map::FormatDuration(float Days)
{
	const int32 Hours = FMath::Max(0, FMath::RoundToInt(Days * 24.f));
	return Hours >= 24 ? FString::Printf(TEXT("%d d. %d t."), Hours / 24, Hours % 24) : FString::Printf(TEXT("%d t."), Hours);
}

FCampaign1851MarchPlan ACampaign1851Map::PlanColumn(const TArray<int32>& Column, int32 CityIndex, const FVector2D& TargetKm, ECampaign1851RouteMode Mode) const
{
	FCampaign1851MarchPlan Plan;
	Plan.Mode = Mode;
	TArray<const FCampaign1851Regiment*> Members;
	for (int32 i : Column)
	{
		if (Regiments.IsValidIndex(i))
		{
			if (Regiments[i].bTraining)
			{
				Plan.Note = TEXT("Enheden træner i garnison. Vælg INDSÆT TIDLIGT først.");
				return Plan;
			}
			Members.Add(&Regiments[i]);
			Plan.TrainsNeeded += Campaign1851Army::TrainsNeeded(Regiments[i]);
		}
	}
	if (Members.Num() == 0)
	{
		return Plan;
	}
	if (Cities.IsValidIndex(CityIndex) && Cities[CityIndex].bForeign)
	{
		Plan.Note = TEXT("Hæren går ikke over grænsen i fredstid");
		return Plan;
	}
	if (!Cities.IsValidIndex(CityIndex) && !IsMonarchyLand(TargetKm))
	{
		Plan.Note = TEXT("Kun på monarkiets land");
		return Plan;
	}
	Plan.Pace = Campaign1851Army::ColumnPace(Members);
	if (const int32 Staff = ColumnStaff(Column))
	{
		Plan.Pace *= Campaign1851Army::StaffPaceFactor(Staff);
	}
	// From where the lead regiment will be: its town or point, or the end of the stretch it is on.
	const FCampaign1851Regiment& Lead = *Members[0];
	const bool bMarching = Lead.IsMarching();
	const int32 StartTown = bMarching ? Lead.Route[Lead.Leg].To : Lead.Town;
	const FVector2D StartKm = bMarching ? Lead.Route[Lead.Leg].ToKm : Lead.Km;
	const float Offset = bMarching ? FMath::Max(0.f, Lead.Route[Lead.Leg].Days - Lead.LegElapsed) : 0.f;
	// A column already on its way to the goal (its current stretch ends there) just finishes that stretch.
	const bool bAtGoal = bMarching && ((CityIndex != INDEX_NONE && StartTown == CityIndex) || (CityIndex == INDEX_NONE && FVector2D::Distance(StartKm, TargetKm) < 0.05));
	FString Why;
	if (!bAtGoal && !PlanMarch(StartTown, StartKm, CityIndex, TargetKm, Plan.Pace, Mode, Plan.Route, &Why))
	{
		Plan.Note = Why;
		return Plan;
	}
	auto Replan = [&](const FString& Note)
	{
		Plan.Mode = ECampaign1851RouteMode::RoadsOnly;
		Plan.Note = Note;
		Plan.Route.Reset();
		PlanMarch(StartTown, StartKm, CityIndex, TargetKm, Plan.Pace, Plan.Mode, Plan.Route);
	};
	const int32 First = Plan.Route.IndexOfByPredicate([](const FCampaign1851Leg& L) { return L.bRail; });
	if (First != INDEX_NONE)
	{
		int32 Last = First;
		while (Plan.Route.IsValidIndex(Last + 1) && Plan.Route[Last + 1].bRail)
		{
			++Last;
		}
		bool bSecondStretch = false;
		for (int32 l = Last + 1; l < Plan.Route.Num(); ++l)
		{
			bSecondStretch |= Plan.Route[l].bRail;
		}
		const int32 Board = Plan.Route[First].From;
		// The nearest free trains that can reach the boarding station on the rails.
		TArray<float> Days;
		TArray<FCampaign1851Leg> Via;
		RailTimes(Board, Days, Via);
		TArray<TPair<float, int32>> Candidates;
		for (int32 t = 0; t < TroopTrainList.Num(); ++t)
		{
			const FCampaign1851TroopTrain& T = TroopTrainList[t];
			if (T.IsFree() && Days.IsValidIndex(T.Station) && Days[T.Station] < TNumericLimits<float>::Max())
			{
				Candidates.Add({ Days[T.Station], t });
			}
		}
		Candidates.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key < B.Key; });
		if (bSecondStretch)
		{
			Replan(TEXT("Togrejsen skifter bane undervejs: kolonnen marcherer"));
		}
		else if (Candidates.Num() < Plan.TrainsNeeded)
		{
			Replan(FString::Printf(TEXT("Kun %d ledige tog på hele jernbanenettet, der når %s (skal bruge %d): kolonnen marcherer"), Candidates.Num(), *Cities[Board].Name, Plan.TrainsNeeded));
		}
		else
		{
			// Wait at the station for the trains still on their way (the loading day is in the first rail stretch).
			float ColumnThere = Offset;
			for (int32 l = 0; l < First; ++l)
			{
				ColumnThere += Plan.Route[l].Days;
			}
			float TrainsThere = 0.f;
			TMap<int32, int32> FromStation;
			for (int32 n = 0; n < Plan.TrainsNeeded; ++n)
			{
				const int32 t = Candidates[n].Value;
				Plan.Trains.Add(t);
				TrainsThere = FMath::Max(TrainsThere, Candidates[n].Key);
				++FromStation.FindOrAdd(TroopTrainList[t].Station);
				TArray<FCampaign1851Leg> Path;
				for (int32 At = TroopTrainList[t].Station; At != Board && Via[At].From != INDEX_NONE; )
				{
					// Via points back towards the boarding station: walk from the train's station along it.
					FCampaign1851Leg Leg = Via[At];
					Swap(Leg.From, Leg.To);
					Swap(Leg.FromKm, Leg.ToKm);
					Path.Add(Leg);
					At = Leg.To;
				}
				Plan.TrainPaths.Add(Path);
			}
			Plan.Board = Board;
			Plan.Release = Plan.Route[Last].To;
			Plan.WaitDays = FMath::Max(0.f, TrainsThere - ColumnThere);
			if (Plan.WaitDays > 0.01f)
			{
				FCampaign1851Leg Wait;
				Wait.bWait = true;
				Wait.From = Wait.To = Board;
				Wait.FromKm = Wait.ToKm = TownKm(Board);
				Wait.Days = Plan.WaitDays;
				Plan.Route.Insert(Wait, First);
			}
			for (const TPair<int32, int32>& S : FromStation)
			{
				Plan.TrainSource += FString::Printf(TEXT("%s%d tog %s %s"), Plan.TrainSource.IsEmpty() ? TEXT("") : TEXT(", "), S.Value,
					S.Key == Board ? TEXT("holder i") : TEXT("fra"), Cities.IsValidIndex(S.Key) ? *Cities[S.Key].Name : TEXT("?"));
			}
		}
	}
	Plan.Days = Offset;
	for (const FCampaign1851Leg& L : Plan.Route)
	{
		Plan.Days += L.Days;
	}
	Plan.bOk = Plan.Route.Num() > 0 || bAtGoal;
	return Plan;
}

void ACampaign1851Map::AdvanceTroopTrains(float DeltaDays)
{
	for (int32 t = 0; t < TroopTrainList.Num(); ++t)
	{
		FCampaign1851TroopTrain& T = TroopTrainList[t];
		// Being moved to another railway: there when the days are up.
		if (T.TransferTo != INDEX_NONE)
		{
			T.TransferDays -= DeltaDays;
			if (T.TransferDays <= 0.f)
			{
				T.Station = T.TransferTo;
				T.TransferTo = INDEX_NONE;
				T.TransferDays = 0.f;
				News.Add(FString::Printf(TEXT("Tog %d er fremme i %s"), T.Id, Cities.IsValidIndex(T.Station) ? *Cities[T.Station].Name : TEXT("?")));
			}
			continue;
		}
		if (T.IsRunningEmpty())
		{
			T.PathElapsed += DeltaDays;
			while (T.IsRunningEmpty() && T.PathElapsed >= T.Path[T.PathLeg].Days)
			{
				T.PathElapsed -= T.Path[T.PathLeg].Days;
				++T.PathLeg;
			}
			if (!T.IsRunningEmpty())
			{
				T.Station = T.Board;
				T.Path.Reset();
				T.PathLeg = 0;
				T.PathElapsed = 0.f;
			}
		}
		if (T.IsFree())
		{
			continue;
		}
		// With its column on the rails; free again (at the end station) once the column has left the train,
		// or where it stands if the column's orders no longer take the train.
		const FCampaign1851Regiment* L = Regiments.IsValidIndex(T.Lead) ? &Regiments[T.Lead] : nullptr;
		const bool bOnRail = L && L->IsMarching() && L->Route[L->Leg].bRail;
		bool bRailAhead = false;
		for (int32 l = L && L->IsMarching() ? L->Leg : 0; L && l < L->Route.Num(); ++l)
		{
			bRailAhead |= L->Route[l].bRail;
		}
		if (bOnRail)
		{
			T.bBoarded = true;
			T.Station = INDEX_NONE;
			T.Km = L->Km;
		}
		else if (!bRailAhead && !T.IsRunningEmpty())
		{
			T.Station = T.bBoarded ? T.Release : (Cities.IsValidIndex(T.Station) ? T.Station : T.Board);
			T.Lead = INDEX_NONE;
			T.bBoarded = false;
		}
	}
	for (int32 o = TrainOrders.Num() - 1; o >= 0; --o)
	{
		if (CampaignDays >= TrainOrders[o].Y)
		{
			// Delivered by ship to the station it was ordered for.
			FCampaign1851TroopTrain T;
			T.Id = NextTrainId++;
			const int32 To = int32(TrainOrders[o].X) - 1;
			T.Station = Cities.IsValidIndex(To) ? To : FindCity(TEXT("København"));
			TroopTrainList.Add(T);
			News.Add(FString::Printf(TEXT("Et nyt troppetog er leveret i %s (i alt %d)"), Cities.IsValidIndex(T.Station) ? *Cities[T.Station].Name : TEXT("?"), TroopTrainList.Num()));
			TrainOrders.RemoveAt(o);
		}
	}
	UpdateTroopTrainPieces();
}

void ACampaign1851Map::ResetTroopTrains()
{
	// 1851: two trains on the Copenhagen line, two on the Holstein line.
	TroopTrainList.Reset();
	TrainOrders.Reset();
	NextTrainId = 1;
	for (const TCHAR* Station : { TEXT("København"), TEXT("København"), TEXT("Altona"), TEXT("Altona") })
	{
		FCampaign1851TroopTrain T;
		T.Id = NextTrainId++;
		T.Station = FindCity(Station);
		TroopTrainList.Add(T);
	}
	UpdateTroopTrainPieces();
}

int32 ACampaign1851Map::FreeTroopTrains() const
{
	int32 Free = 0;
	for (const FCampaign1851TroopTrain& T : TroopTrainList)
	{
		Free += T.IsFree() ? 1 : 0;
	}
	return Free;
}

FString ACampaign1851Map::DescribeTrain(int32 Train) const
{
	if (!TroopTrainList.IsValidIndex(Train))
	{
		return FString();
	}
	const FCampaign1851TroopTrain& T = TroopTrainList[Train];
	if (T.TransferTo != INDEX_NONE)
	{
		return FString::Printf(TEXT("skibes til %s  ·  fremme ca. %s"), Cities.IsValidIndex(T.TransferTo) ? *Cities[T.TransferTo].Name : TEXT("?"),
			*FormatDate(GetDate() + FTimespan::FromDays(T.TransferDays), true));
	}
	const FString Column = Regiments.IsValidIndex(T.Lead) ? Regiments[T.Lead].Name + (Regiments[T.Lead].Group ? TEXT(" m.fl.") : TEXT("")) : FString();
	if (T.IsRunningEmpty())
	{
		float Left = -T.PathElapsed;
		for (int32 l = T.PathLeg; l < T.Path.Num(); ++l)
		{
			Left += T.Path[l].Days;
		}
		const FDateTime There = GetDate() + FTimespan::FromDays(Left);
		return FString::Printf(TEXT("kører tomt til %s efter %s  ·  fremme %s %s"), Cities.IsValidIndex(T.Board) ? *Cities[T.Board].Name : TEXT("?"), *Column,
			*FormatClock(There), *FormatDate(There, true));
	}
	if (T.bBoarded)
	{
		return FString::Printf(TEXT("kører %s til %s"), *Column, Cities.IsValidIndex(T.Release) ? *Cities[T.Release].Name : TEXT("?"));
	}
	if (!T.IsFree())
	{
		return FString::Printf(TEXT("venter i %s på %s"), Cities.IsValidIndex(T.Station) ? *Cities[T.Station].Name : TEXT("?"), *Column);
	}
	return FString::Printf(TEXT("holder ledigt i %s"), Cities.IsValidIndex(T.Station) ? *Cities[T.Station].Name : TEXT("?"));
}

void ACampaign1851Map::UpdateTroopTrainPieces()
{
	EnsureTrainParts();
	if (TrainParts.Num() < 3 || GridZ.Num() == 0)
	{
		return;
	}
	for (int32 c = TroopTrainList.Num() * TrainVehicles; c < TroopTrainPieces.Num(); ++c)
	{
		if (TroopTrainPieces[c])
		{
			TroopTrainPieces[c]->DestroyComponent();
		}
	}
	TroopTrainPieces.SetNum(TroopTrainList.Num() * TrainVehicles);
	TMap<int32, int32> Parked;   // trains standing at each station, to set them one behind the other
	for (int32 t = 0; t < TroopTrainList.Num(); ++t)
	{
		const FCampaign1851TroopTrain& T = TroopTrainList[t];
		TArray<UStaticMeshComponent*> Parts;
		for (int32 v = 0; v < TrainVehicles; ++v)
		{
			TObjectPtr<UStaticMeshComponent>& P = TroopTrainPieces[t * TrainVehicles + v];
			if (!P)
			{
				P = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("TroopTrain")));
				P->SetupAttachment(Root);
				P->SetStaticMesh(TrainParts[v == 0 ? 0 : v == 1 ? 1 : 2]);
				P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				P->SetCastShadow(false);
				P->SetWorldScale3D(FVector(PieceScale * 1.6f));
				P->RegisterComponent();
			}
			Parts.Add(P);
		}
		// With its column it is drawn as the column's train; otherwise on its own.
		const bool bShow = !T.bBoarded && T.TransferTo == INDEX_NONE && bSceneryVisible;
		for (UStaticMeshComponent* P : Parts)
		{
			P->SetVisibility(bShow);
		}
		if (!bShow)
		{
			continue;
		}
		if (T.IsRunningEmpty())
		{
			const FCampaign1851Leg& Leg = T.Path[T.PathLeg];
			const TArray<FVector2D> Line = LegLine(Leg);
			PlaceTrain(Parts, Line, LineLength(Line) * FMath::Clamp(T.PathElapsed / FMath::Max(Leg.Days, 0.001f), 0.f, 1.f), 1.f, PieceScale * 1.6f);
			continue;
		}
		// Parked: on a line through the station, just out from the town centre, one behind the other.
		const int32 Slot = Parked.FindOrAdd(T.Station)++;
		for (const FCampaign1851Railway& R : Railways)
		{
			if (R.IsOpen(GetDate()) && R.Towns.Contains(T.Station))
			{
				const FVector2D Centre = TownKm(T.Station);
				const double Length = LineLength(R.Km);
				double Best = 0.0, BestDist = 1e9;
				for (double At = 0.0; At <= Length; At += 0.1)
				{
					const double D = FVector2D::Distance(AlongLine(R.Km, At), Centre);
					if (D < BestDist)
					{
						BestDist = D;
						Best = At;
					}
				}
				const float Dir = Best < Length * 0.5 ? 1.f : -1.f;
				PlaceTrain(Parts, R.Km, FMath::Clamp(Best + Dir * (0.9 + Slot * 0.5), 0.0, Length), Dir, PieceScale * 1.6f);
				break;
			}
		}
	}
}

void ACampaign1851Map::StopRegiment(int32 Regiment)
{
	if (!Regiments.IsValidIndex(Regiment) || !Regiments[Regiment].IsMarching())
	{
		return;
	}
	// Halt on the spot: the column stands in the field where it was (Km is kept up to date by the march).
	FCampaign1851Regiment& R = Regiments[Regiment];
	R.Route.Reset();
	R.Leg = 0;
	R.LegElapsed = 0.f;
	R.Group = 0;
	R.Town = INDEX_NONE;
	UpdateRegimentPiece(Regiment);
}

void ACampaign1851Map::CancelOrder(int32 Regiment)
{
	if (!Regiments.IsValidIndex(Regiment) || !Regiments[Regiment].IsMarching())
	{
		return;
	}
	FCampaign1851Regiment& R = Regiments[Regiment];
	const int32 OriginTown = R.OriginTown;
	const FVector2D OriginKm = R.OriginKm;
	const ECampaign1851RouteMode Mode = R.Mode;
	StopRegiment(Regiment);
	if (!OriginKm.IsZero() && FVector2D::Distance(OriginKm, R.Km) > 0.05)
	{
		OrderMarchTo({ Regiment }, OriginTown, OriginKm, Mode);
	}
	else if (Cities.IsValidIndex(OriginTown))
	{
		R.Town = OriginTown;
		PlaceInTown(Regiment);
		UpdateRegimentPiece(Regiment);
	}
}

void ACampaign1851Map::HaltRegiment(int32 Regiment)
{
	if (Regiments.IsValidIndex(Regiment) && Regiments[Regiment].IsMarching())
	{
		FCampaign1851Regiment& R = Regiments[Regiment];
		R.Route.SetNum(R.Leg + 1);
	}
}

// ------------------------------------------------------------------ time

void ACampaign1851Map::AdvanceArmy(float DeltaDays, float DeltaSeconds)
{

	for (int32 i = 0; i < Regiments.Num() && DeltaDays > 0.f; ++i)
	{
		// The chief's hand: drill in garrison, morale towards what he can inspire, cohesion at rest.
		FCampaign1851Regiment& R = Regiments[i];
		const FCampaign1851Officer* Chief = Officers.IsValidIndex(R.Chief) ? &Officers[R.Chief] : nullptr;
		const float Lead = Chief ? Chief->Stat(ECampaign1851OfficerStat::Leadership) : 3.f;
		const float Insp = Chief ? Chief->Stat(ECampaign1851OfficerStat::Inspiration) : 3.f;
		const float Endurance = R.Skill(ECampaign1851Skill::Endurance);
		if (R.bTraining && !R.IsMarching() && R.Town == R.Home && !IsInBattle(i))
		{
			const float Gain = FMath::Min(1.f - R.RaisingProgress, DeltaDays * R.RaisingRate());
			R.RaisingProgress += Gain;
			for (float& Skill : R.Skills) { Skill = FMath::Min(60.f, Skill + Gain * 40.f); }
			R.Experience = FMath::Min(40.f, R.Experience + Gain * 20.f);
			R.Cohesion = FMath::Min(80.f, R.Cohesion + Gain * 60.f);
			R.Morale = FMath::Min(0.8f, R.Morale + Gain * 0.35f);
			if (R.RaisingProgress >= 1.f - KINDA_SMALL_NUMBER)
			{
				R.bTraining = false;
				R.RaisingProgress = 1.f;
				News.Add(R.Name + TEXT(" har afsluttet grunduddannelsen"));
			}
			continue; // Initial training has its own rates; no veteran stat floors.
		}
		if (R.bTraining) { continue; } // Pause initial training away from home or during battle.
		if (R.IsMarching())
		{
			// A fit unit and a good chief keep it together on the road; the march itself hardens it a little.
			R.Cohesion = FMath::Max(FMath::Min(40.f, R.Cohesion), R.Cohesion - DeltaDays * 0.4f * (1.2f - Lead / 12.f) * (1.3f - Endurance / 100.f));
			R.Experience = FMath::Min(70.f, R.Experience + DeltaDays * 0.02f);   // field service, up to seasoned
			float& Fit = R.Skills[int32(ECampaign1851Skill::Endurance)];
			Fit = FMath::Min(70.f, Fit + DeltaDays * 0.02f);
			float& Field = R.Skills[int32(ECampaign1851Skill::Fieldcraft)];
			Field = FMath::Min(65.f, Field + DeltaDays * 0.01f);
		}
		else
		{
			// Garrison: the programme trains, gains slow near the chief's ceiling; the rest slowly falls
			// (fitness fastest), down to a floor the old soldiers keep.
			const float Cap = 60.f + 4.f * Lead;
			for (int32 s = 0; s < int32(ECampaign1851Skill::Count); ++s)
			{
				const ECampaign1851Skill Skill = ECampaign1851Skill(s);
				float& V = R.Skills[s];
				const float Weight = Campaign1851Army::ProgramWeight(R.Program, Skill);
				if (Weight > 0.f && V < Cap)
				{
					V = FMath::Min(Cap, V + DeltaDays * 0.12f * Weight * (0.5f + Lead / 10.f) * FMath::Max(0.1f, 1.f - V / 100.f) * (0.4f + 0.6f * R.Present));
				}
				else if (Weight <= 0.f)
				{
					V = FMath::Max(FMath::Min(30.f, V), V - DeltaDays * (Skill == ECampaign1851Skill::Endurance ? 0.03f : 0.01f));
				}
			}
			R.Cohesion = FMath::Min(90.f, R.Cohesion + DeltaDays * 0.3f);
			// The fire methods the army has researched: drilled in (about forty days to the battle-ready 60
			// under a fair chief with eksercits, longer with skydeøvelser or blandet).
			static const TCHAR* DrillTopics[4] = { TEXT("tworank"), TEXT("firebyrank"), TEXT("volley"), TEXT("independent") };
			const float DrillWeight = R.Program == ECampaign1851Program::Drill ? 1.f : R.Program == ECampaign1851Program::LiveFire ? 0.8f
				: R.Program == ECampaign1851Program::Mixed ? 0.4f : 0.f;
			for (int32 d = 0; d < 4; ++d)
			{
				if (DrillWeight > 0.f && HasResearch(DrillTopics[d]) && R.FireDrills[d] < 100.f)
				{
					R.FireDrills[d] = FMath::Min(100.f, R.FireDrills[d] + DeltaDays * 1.5f * DrillWeight * (0.5f + Lead / 10.f) * (0.4f + 0.6f * R.Present));
				}
			}
		}
		// Well-trained soldiers trust themselves: up to +10 % on what the chief can inspire.
		const float MoraleTarget = 0.7f + 0.025f * Insp + 0.1f * (R.MeanSkill() - 50.f) / 50.f;
		R.Morale += (MoraleTarget - R.Morale) * FMath::Min(1.f, DeltaDays * 0.01f * (0.5f + Lead / 10.f));
		for (int32 o : { R.Chief, R.General })
		{
			if (Officers.IsValidIndex(o))
			{
				Officers[o].Experience = FMath::Min(100.f, Officers[o].Experience + DeltaDays * (R.IsMarching() ? 0.05f : 0.01f));
			}
		}
	}
	bool bAnyMoved = false;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		FCampaign1851Regiment& R = Regiments[i];
		if (!R.IsMarching())
		{
			continue;
		}
		R.LegElapsed += DeltaDays * LegPace(R.Route[R.Leg]);
		while (R.IsMarching() && R.LegElapsed >= R.Route[R.Leg].Days)
		{
			R.LegElapsed -= R.Route[R.Leg].Days;
			++R.Leg;
		}
		if (!R.IsMarching())
		{
			// Arrived: in a town it camps outside it; in the field it halts where it is, facing on.
			const FCampaign1851Leg Last = R.Route.Last();
			R.Town = Last.To;
			R.Route.Reset();
			R.Group = 0;
			R.Leg = 0;
			R.LegElapsed = 0.f;
			if (Cities.IsValidIndex(R.Town))
			{
				PlaceInTown(i);
			}
			else
			{
				R.Km = Last.ToKm;
				R.Heading = (Last.ToKm - Last.FromKm).GetSafeNormal();
			}
			News.Add(FString::Printf(TEXT("%s er ankommet til %s"), *R.Name, *DescribePlace(R.Town, R.Km)));
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|army|%s arrived in %s"), *R.Name, *DescribePlace(R.Town, R.Km));
		}
		else
		{
			const FCampaign1851Leg& Leg = R.Route[R.Leg];
			const TArray<FVector2D> Line = LegLine(Leg);
			const double Length = LineLength(Line);
			FVector2D Dir;
			R.Km = AlongLine(Line, Length * FMath::Clamp(R.LegElapsed / FMath::Max(Leg.Days, 0.001f), 0.f, 1.f), &Dir);
			R.Heading = Dir;
		}
		UpdateRegimentPiece(i);
		bAnyMoved = true;
	}
	// Those standing still move aside when others come to (or leave) their place.
	if (bAnyMoved)
	{
		for (int32 i = 0; i < Regiments.Num(); ++i)
		{
			if (!Regiments[i].IsMarching())
			{
				UpdateRegimentPiece(i);
			}
		}
	}
	AdvanceTroopTrains(DeltaDays);
}

void ACampaign1851Map::PlaceInTown(int32 Regiment)
{
	FCampaign1851Regiment& R = Regiments[Regiment];
	if (!Cities.IsValidIndex(R.Town))
	{
		return;
	}
	// Around the town on its fields, one place per regiment there, facing out.
	const TArray<int32> Here = RegimentsIn(R.Town);
	const int32 Slot = FMath::Max(0, Here.IndexOfByKey(Regiment));
	const FCampaign1851City& C = Cities[R.Town];
	const FVector2D Centre = Extent.Projection.Forward(C.Lat, C.Lon);
	// Out on the fields past the last houses; the camps go round the town on land, clear of the sea.
	const float Radius = TownRadiusKm(C.Population) * 1.1f + 0.7f;
	int32 Found = 0;
	for (int32 Step = 0; Step < 36; ++Step)
	{
		const float Angle = FMath::DegreesToRadians(200.f + Step * 30.f + (Step / 12) * 15.f);
		const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
		const FVector2D Spot = Centre + Dir * (Radius + (Step / 12) * 0.8f);
		if (!IsMonarchyLand(Spot) || !IsMonarchyLand(Spot + Dir * 0.4) || !IsMonarchyLand(Spot - Dir * 0.4))
		{
			continue;
		}
		if (Found++ == Slot)
		{
			R.Heading = Dir;
			R.Km = Spot;
			ClearScenery(Spot, 0.3f);
			return;
		}
	}
	R.Heading = FVector2D(-1.0, 0.0);
	R.Km = Centre + R.Heading * Radius;
}

void ACampaign1851Map::UpdateRegimentPiece(int32 Regiment)
{
	if (ArmyMeshes.Num() == 0)
	{
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, ArmyMaterialPath);
		if (!Material || GridZ.Num() == 0)
		{
			return;
		}
		using Campaign1851Scenery::ESitePiece;
		for (ESitePiece Piece : { ESitePiece::FormationInfantry, ESitePiece::FormationGuard, ESitePiece::FormationJager, ESitePiece::FormationCavalry, ESitePiece::FormationArtillery, ESitePiece::FormationHorseArtillery })
		{
			ArmyMeshes.Add(Campaign1851Scenery::BuildSitePiece(Piece, Material));
		}
	}
	RegimentPieces.SetNum(Regiments.Num());
	UStaticMeshComponent* Piece = RegimentPieces[Regiment];
	const FCampaign1851Regiment& R = Regiments[Regiment];
	if (!Piece)
	{
		Piece = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), *FString::Printf(TEXT("Regiment_%s"), *R.Id)));
		Piece->SetupAttachment(Root);
		Piece->SetStaticMesh(ArmyMeshes[FMath::Min(int32(R.Arm), ArmyMeshes.Num() - 1)]);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetCastShadow(false);
		Piece->SetWorldScale3D(FVector(PieceScale * FormationScale));
		Piece->RegisterComponent();
		RegimentPieces[Regiment] = Piece;
	}
	// By rail it is a train (engine and three carriages on the track); otherwise its formation.
	const bool bOnTrain = R.IsMarching() && R.Route[R.Leg].bRail;
	if (bOnTrain)
	{
		EnsureTrainParts();
	}
	bool bTrain = bOnTrain && TrainParts.Num() == 3;
	bool bPassenger = false;   // rides in the train of an earlier regiment of the same column
	if (bTrain && R.Group != 0)
	{
		for (int32 j = 0; j < Regiment; ++j)
		{
			if (Regiments[j].Group == R.Group && Regiments[j].IsMarching() && Regiments[j].Route[Regiments[j].Leg].bRail)
			{
				bPassenger = true;
				break;
			}
		}
	}
	UStaticMesh* Wanted = bTrain ? TrainParts[0].Get() : ArmyMeshes[FMath::Min(int32(R.Arm), ArmyMeshes.Num() - 1)].Get();
	const float TrainScale = PieceScale * 1.6f;
	if (Piece->GetStaticMesh() != Wanted)
	{
		Piece->SetStaticMesh(Wanted);
		Piece->SetWorldScale3D(FVector(bTrain ? TrainScale : PieceScale * FormationScale));
	}
	RegimentCars.SetNum(Regiments.Num() * 3);
	for (int32 c = 0; c < 3; ++c)
	{
		TObjectPtr<UStaticMeshComponent>& Car = RegimentCars[Regiment * 3 + c];
		if (bTrain && !Car)
		{
			Car = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), *FString::Printf(TEXT("RegimentCar_%s"), *R.Id)));
			Car->SetupAttachment(Root);
			Car->SetStaticMesh(TrainParts[c == 0 ? 1 : 2]);
			Car->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Car->SetCastShadow(false);
			Car->SetWorldScale3D(FVector(TrainScale));
			Car->RegisterComponent();
		}
		if (Car)
		{
			Car->SetVisibility(bTrain && !bPassenger && LastCameraDistanceKm < FormationMaxDistanceKm);
		}
	}
	if (bPassenger)
	{
		Piece->SetVisibility(false);
		return;
	}
	if (bTrain)
	{
		const FCampaign1851Leg& Leg = R.Route[R.Leg];
		const TArray<FVector2D> Line = LegLine(Leg);
		const double Front = LineLength(Line) * FMath::Clamp(R.LegElapsed / FMath::Max(Leg.Days, 0.001f), 0.f, 1.f);
		PlaceTrain({ Piece, RegimentCars[Regiment * 3].Get(), RegimentCars[Regiment * 3 + 1].Get(), RegimentCars[Regiment * 3 + 2].Get() }, Line, Front, 1.f, TrainScale);
		Piece->SetVisibility(LastCameraDistanceKm < FormationMaxDistanceKm);
		return;
	}
	// The formation faces +X: turn it to the heading (world Y is south).
	Piece->SetWorldLocationAndRotation(WorldAtKm(ShownKm(Regiment)) + FVector(0.0, 0.0, 0.5),
		FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(-R.Heading.Y, R.Heading.X)) + float(GetActorRotation().Yaw), 0.f));
	Piece->SetVisibility(LastCameraDistanceKm < FormationMaxDistanceKm);
}

// ------------------------------------------------------------------ save

// ------------------------------------------------------------------ field formations

int32 ACampaign1851Map::FormationIndex(int32 Id) const
{
	return Formations.IndexOfByPredicate([Id](const FCampaign1851Formation& F) { return F.Id == Id; });
}

int32 ACampaign1851Map::CreateFormation(ECampaign1851Echelon Echelon, int32 Parent)
{
	// Numbered per level: "1. Division", "3. Brigade".
	int32 Number = 1;
	for (const FCampaign1851Formation& F : Formations)
	{
		Number += F.Echelon == Echelon ? 1 : 0;
	}
	FCampaign1851Formation F;
	F.Id = NextFormationId++;
	F.Echelon = Echelon;
	F.Name = Echelon == ECampaign1851Echelon::Army ? FString(TEXT("Felthæren")) : FString::Printf(TEXT("%d. %s"), Number, Campaign1851Army::EchelonName(Echelon));
	F.Parent = FormationIndex(Parent) != INDEX_NONE ? Parent : 0;
	Formations.Add(F);
	return F.Id;
}

int32 ACampaign1851Map::FormFromCommand(int32 Command, int32 Parent)
{
	if (!Commands.IsValidIndex(Command))
	{
		return 0;
	}
	TArray<int32> Foot, Other;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		const FCampaign1851Regiment& R = Regiments[i];
		if (R.Command == Command && R.Formation == 0 && R.Men > 0)
		{
			(R.Arm == ECampaign1851Arm::Infantry || R.Arm == ECampaign1851Arm::Jager || R.Arm == ECampaign1851Arm::Guard ? Foot : Other).Add(i);
		}
	}
	if (Foot.Num() + Other.Num() == 0)
	{
		return 0;
	}
	const int32 ParentIndex = FormationIndex(Parent);
	const ECampaign1851Echelon Above = ParentIndex == INDEX_NONE ? ECampaign1851Echelon::Army : Formations[ParentIndex].Echelon;
	const ECampaign1851Echelon Echelon = Above == ECampaign1851Echelon::Army ? ECampaign1851Echelon::Division : Above == ECampaign1851Echelon::Division ? ECampaign1851Echelon::Brigade : ECampaign1851Echelon::Regiment;
	const int32 Id = CreateFormation(Echelon, ParentIndex == INDEX_NONE ? 0 : Parent);
	if (Echelon == ECampaign1851Echelon::Division && Foot.Num() > 4)
	{
		// The foot in brigades of up to four battalions.
		for (int32 b = 0; b < Foot.Num(); b += 4)
		{
			const int32 Brigade = CreateFormation(ECampaign1851Echelon::Brigade, Id);
			for (int32 k = b; k < FMath::Min(b + 4, Foot.Num()); ++k)
			{
				MoveRegimentToFormation(Foot[k], Brigade);
			}
		}
	}
	else
	{
		for (int32 i : Foot)
		{
			MoveRegimentToFormation(i, Id);
		}
	}
	for (int32 i : Other)
	{
		MoveRegimentToFormation(i, Id);
	}
	// The command's general leads it when he is free to.
	if (Officers.IsValidIndex(Commands[Command].General))
	{
		AssignFormationCommander(Commands[Command].General, Id);
	}
	return Id;
}

void ACampaign1851Map::DissolveFormation(int32 Id)
{
	const int32 Index = FormationIndex(Id);
	if (Index == INDEX_NONE)
	{
		return;
	}
	// Its units and sub-formations go up to its parent; its commander to the pool.
	const int32 Parent = Formations[Index].Parent;
	for (FCampaign1851Regiment& R : Regiments)
	{
		R.Formation = R.Formation == Id ? Parent : R.Formation;
	}
	for (FCampaign1851Formation& F : Formations)
	{
		F.Parent = F.Parent == Id ? Parent : F.Parent;
	}
	if (Officers.IsValidIndex(Formations[Index].Commander))
	{
		Officers[Formations[Index].Commander].Formation = 0;
	}
	for (const int32 Staff : { Formations[Index].Deputy, Formations[Index].StaffChief })
	{
		if (Officers.IsValidIndex(Staff))
		{
			Officers[Staff].StaffOf = Officers[Staff].StaffPost = 0;
		}
	}
	Formations.RemoveAt(Index);
}

bool ACampaign1851Map::IsInside(int32 Id, int32 Ancestor) const
{
	for (int32 At = Id, Guard = 0; At != 0 && Guard < 64; ++Guard)
	{
		if (At == Ancestor)
		{
			return true;
		}
		const int32 Index = FormationIndex(At);
		At = Index == INDEX_NONE ? 0 : Formations[Index].Parent;
	}
	return false;
}

bool ACampaign1851Map::MoveFormation(int32 Id, int32 NewParent)
{
	const int32 Index = FormationIndex(Id);
	if (Index == INDEX_NONE || (NewParent != 0 && (FormationIndex(NewParent) == INDEX_NONE || IsInside(NewParent, Id))))
	{
		return false;   // no loops: a formation cannot go under itself or its own sub-formations
	}
	Formations[Index].Parent = NewParent;
	return true;
}

int32 ACampaign1851Map::ReturnFormationToGarrison(int32 Id)
{
	if (FormationIndex(Id) == INDEX_NONE)
	{
		return 0;
	}
	const TArray<int32> Units = FormationRegiments(Id);
	for (int32 i : Units)
	{
		Regiments[i].Formation = 0;
	}
	// It and everything under it.
	TArray<int32> Ids;
	for (const FCampaign1851Formation& F : Formations)
	{
		if (IsInside(F.Id, Id))
		{
			Ids.Add(F.Id);
		}
	}
	for (int32 F : Ids)
	{
		DissolveFormation(F);
	}
	return Units.Num();
}

bool ACampaign1851Map::MoveRegimentToFormation(int32 Regiment, int32 Formation)
{
	if (!Regiments.IsValidIndex(Regiment) || (Formation != 0 && FormationIndex(Formation) == INDEX_NONE))
	{
		return false;
	}
	Regiments[Regiment].Formation = Formation;
	return true;
}

TArray<int32> ACampaign1851Map::FormationRegiments(int32 Id) const
{
	TArray<int32> Out;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		if (Regiments[i].Formation != 0 && IsInside(Regiments[i].Formation, Id))
		{
			Out.Add(i);
		}
	}
	return Out;
}

bool ACampaign1851Map::AssignFormationCommander(int32 Officer, int32 Formation)
{
	const int32 Index = FormationIndex(Formation);
	if (!Officers.IsValidIndex(Officer) || Index == INDEX_NONE)
	{
		return false;
	}
	FCampaign1851Officer& O = Officers[Officer];
	// He leaves any post he had; whoever led the formation goes to the pool.
	if (Officers.IsValidIndex(Formations[Index].Commander))
	{
		Officers[Formations[Index].Commander].Formation = 0;
	}
	if (Regiments.IsValidIndex(O.Regiment))
	{
		(O.bGeneral ? Regiments[O.Regiment].General : Regiments[O.Regiment].Chief) = INDEX_NONE;
		O.Regiment = INDEX_NONE;
	}
	if (Commands.IsValidIndex(O.Command))
	{
		Commands[O.Command].General = INDEX_NONE;
		O.Command = INDEX_NONE;
	}
	if (Regiments.IsValidIndex(O.CaptainOf) && Regiments[O.CaptainOf].Captains.IsValidIndex(O.Company))
	{
		Regiments[O.CaptainOf].Captains[O.Company] = INDEX_NONE;
	}
	O.CaptainOf = O.Company = INDEX_NONE;
	LeaveStaffPost(Officer);
	const int32 OldIndex = FormationIndex(O.Formation);
	if (OldIndex != INDEX_NONE)
	{
		Formations[OldIndex].Commander = INDEX_NONE;
	}
	O.Formation = Formation;
	Formations[Index].Commander = Officer;
	return true;
}

void ACampaign1851Map::BuildTestFieldArmy()
{
	auto General = [this](const TCHAR* Id) { return Officers.IndexOfByPredicate([Id](const FCampaign1851Officer& O) { return O.Id == Id; }); };
	auto Put = [this](int32 Formation, std::initializer_list<const TCHAR*> Ids)
	{
		for (const TCHAR* Id : Ids)
		{
			MoveRegimentToFormation(FindRegiment(Id), Formation);
		}
	};
	auto FreeOfficer = [this]()
	{
		for (int32 o = 0; o < Officers.Num(); ++o)
		{
			if (!Officers[o].bGeneral && Officers[o].IsFree())
			{
				return o;
			}
		}
		return int32(INDEX_NONE);
	};
	// New officers for the brigades and regiments (the peacetime corps has only a few in reserve).
	FRandomStream Rng(1864);
	auto Chief = [&](int32 Formation, const TCHAR* Rank)
	{
		int32 O = FreeOfficer();
		if (O == INDEX_NONE)
		{
			FCampaign1851Officer New = MakeOfficer(Rng, false, Rank);
			New.Id = FString::Printf(TEXT("R%d"), NextOfficerNumber++);
			O = Officers.Add(New);
		}
		Officers[O].Rank = Rank;
		AssignFormationCommander(O, Formation);
	};
	auto Regiment = [&](int32 Brigade, const TCHAR* A, const TCHAR* B)
	{
		const int32 R = CreateFormation(ECampaign1851Echelon::Regiment, Brigade);
		Put(R, { A, B });
		Chief(R, TEXT("Oberstløjtnant"));
		return R;
	};
	const int32 Div1 = CreateFormation(ECampaign1851Echelon::Division, 0);
	const int32 Div2 = CreateFormation(ECampaign1851Echelon::Division, 0);
	const int32 Reserve = CreateFormation(ECampaign1851Echelon::Division, 0);
	Formations[FormationIndex(Reserve)].Name = TEXT("Reserven");
	AssignFormationCommander(General(TEXT("G_MEZA")), Div1);
	AssignFormationCommander(General(TEXT("G_KROGH")), Div2);
	AssignFormationCommander(General(TEXT("G_BULOW")), Reserve);
	const int32 B1 = CreateFormation(ECampaign1851Echelon::Brigade, Div1), B2 = CreateFormation(ECampaign1851Echelon::Brigade, Div1);
	const int32 B3 = CreateFormation(ECampaign1851Echelon::Brigade, Div2), B4 = CreateFormation(ECampaign1851Echelon::Brigade, Div2);
	const int32 Guard = CreateFormation(ECampaign1851Echelon::Brigade, Reserve);
	Formations[FormationIndex(Guard)].Name = TEXT("Gardebrigaden");
	for (const int32 Brigade : { B1, B2, B3, B4, Guard })
	{
		Chief(Brigade, TEXT("Oberst"));
	}
	Regiment(B1, TEXT("B6"), TEXT("B7"));
	Regiment(B1, TEXT("B8"), TEXT("B9"));
	Regiment(B2, TEXT("B10"), TEXT("J1"));
	Put(Div1, { TEXT("D2"), TEXT("A3"), TEXT("RA2") });
	Regiment(B3, TEXT("B11"), TEXT("B12"));
	Regiment(B3, TEXT("B13"), TEXT("B14"));
	Regiment(B4, TEXT("J2"), TEXT("J3"));
	Put(Div2, { TEXT("D4"), TEXT("A4"), TEXT("A5") });
	const int32 GuardRegiment = Regiment(Guard, TEXT("LG"), TEXT("B1"));
	Formations[FormationIndex(GuardRegiment)].Name = TEXT("Garderegimentet");
	Regiment(Guard, TEXT("B2"), TEXT("B3"));
	Put(Reserve, { TEXT("GH"), TEXT("A1"), TEXT("A2"), TEXT("RA1") });
	// Every headquarters with a deputy and a chief of staff (adjutant below the divisions).
	for (int32 f = 0; f < Formations.Num(); ++f)
	{
		const bool bDivision = Formations[f].Echelon == ECampaign1851Echelon::Division;
		const bool bBrigade = Formations[f].Echelon == ECampaign1851Echelon::Brigade;
		const TCHAR* DeputyRank = bDivision ? TEXT("Oberst") : bBrigade ? TEXT("Oberstløjtnant") : TEXT("Major");
		const TCHAR* StaffRank = bDivision ? TEXT("Major") : TEXT("Kaptajn");
		for (int32 Post = 1; Post <= 2; ++Post)
		{
			FCampaign1851Officer New = MakeOfficer(Rng, false, Post == 1 ? DeputyRank : StaffRank);
			New.Id = FString::Printf(TEXT("R%d"), NextOfficerNumber++);
			AssignFormationStaff(Officers.Add(New), Formations[f].Id, Post);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|army|test field army: %d formations"), Formations.Num());
}

void ACampaign1851Map::LeaveStaffPost(int32 Officer)
{
	if (!Officers.IsValidIndex(Officer))
	{
		return;
	}
	FCampaign1851Officer& O = Officers[Officer];
	const int32 Index = FormationIndex(O.StaffOf);
	if (Index != INDEX_NONE)
	{
		int32& Post = O.StaffPost == 1 ? Formations[Index].Deputy : Formations[Index].StaffChief;
		Post = Post == Officer ? INDEX_NONE : Post;
	}
	O.StaffOf = O.StaffPost = 0;
}

bool ACampaign1851Map::AssignFormationStaff(int32 Officer, int32 Formation, int32 Post)
{
	if (Post == 0)
	{
		return AssignFormationCommander(Officer, Formation);
	}
	const int32 Index = FormationIndex(Formation);
	if (!Officers.IsValidIndex(Officer) || Index == INDEX_NONE || Post < 1 || Post > 2)
	{
		return false;
	}
	FCampaign1851Officer& O = Officers[Officer];
	// He leaves whatever post he had (as for a commander); whoever held this one goes to the pool.
	if (Regiments.IsValidIndex(O.Regiment))
	{
		(O.bGeneral ? Regiments[O.Regiment].General : Regiments[O.Regiment].Chief) = INDEX_NONE;
		O.Regiment = INDEX_NONE;
	}
	if (Commands.IsValidIndex(O.Command))
	{
		Commands[O.Command].General = INDEX_NONE;
		O.Command = INDEX_NONE;
	}
	const int32 OldIndex = FormationIndex(O.Formation);
	if (OldIndex != INDEX_NONE)
	{
		Formations[OldIndex].Commander = INDEX_NONE;
	}
	O.Formation = 0;
	if (Regiments.IsValidIndex(O.CaptainOf) && Regiments[O.CaptainOf].Captains.IsValidIndex(O.Company))
	{
		Regiments[O.CaptainOf].Captains[O.Company] = INDEX_NONE;
	}
	O.CaptainOf = O.Company = INDEX_NONE;
	LeaveStaffPost(Officer);
	int32& Slot = Post == 1 ? Formations[Index].Deputy : Formations[Index].StaffChief;
	if (Officers.IsValidIndex(Slot))
	{
		Officers[Slot].StaffOf = Officers[Slot].StaffPost = 0;
	}
	Slot = Officer;
	O.StaffOf = Formation;
	O.StaffPost = Post;
	return true;
}

int32 ACampaign1851Map::ActingCommander(int32 Formation, bool* bOutActing) const
{
	const int32 Index = FormationIndex(Formation);
	if (bOutActing)
	{
		*bOutActing = false;
	}
	if (Index == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	if (Officers.IsValidIndex(Formations[Index].Commander))
	{
		return Formations[Index].Commander;
	}
	if (Officers.IsValidIndex(Formations[Index].Deputy))
	{
		if (bOutActing)
		{
			*bOutActing = true;
		}
		return Formations[Index].Deputy;
	}
	return INDEX_NONE;
}

int32 ACampaign1851Map::ColumnStaff(const TArray<int32>& Column) const
{
	// The best staff work over the column: its general's, or a chief of staff of a formation it belongs to.
	int32 Best = 0;
	if (const FCampaign1851Officer* General = ColumnGeneral(Column))
	{
		Best = General->Stat(ECampaign1851OfficerStat::Staff);
	}
	for (int32 i : Column)
	{
		if (!Regiments.IsValidIndex(i))
		{
			continue;
		}
		for (int32 At = Regiments[i].Formation, Guard = 0; At != 0 && Guard < 16; ++Guard)
		{
			const int32 Index = FormationIndex(At);
			if (Index == INDEX_NONE)
			{
				break;
			}
			if (Officers.IsValidIndex(Formations[Index].StaffChief))
			{
				Best = FMath::Max(Best, int32(Officers[Formations[Index].StaffChief].Stat(ECampaign1851OfficerStat::Staff)));
			}
			At = Formations[Index].Parent;
		}
	}
	return Best;
}

int32 ACampaign1851Map::SeniorCaptain(int32 Regiment) const
{
	int32 Best = INDEX_NONE;
	if (Regiments.IsValidIndex(Regiment))
	{
		for (int32 O : Regiments[Regiment].Captains)
		{
			if (Officers.IsValidIndex(O) && (Best == INDEX_NONE || Officers[O].Experience > Officers[Best].Experience))
			{
				Best = O;
			}
		}
	}
	return Best;
}

TArray<FCampaign1851FormationSave> ACampaign1851Map::SaveFormations() const
{
	TArray<FCampaign1851FormationSave> Out;
	for (const FCampaign1851Formation& F : Formations)
	{
		FCampaign1851FormationSave& S = Out.AddDefaulted_GetRef();
		S.Id = F.Id;
		S.Name = F.Name;
		S.Echelon = uint8(F.Echelon);
		S.Parent = F.Parent;
		S.Commander = Officers.IsValidIndex(F.Commander) ? Officers[F.Commander].Id : FString();
		S.Deputy = Officers.IsValidIndex(F.Deputy) ? Officers[F.Deputy].Id : FString();
		S.StaffChief = Officers.IsValidIndex(F.StaffChief) ? Officers[F.StaffChief].Id : FString();
		for (const FCampaign1851Regiment& R : Regiments)
		{
			if (R.Formation == F.Id)
			{
				S.Regiments.Add(R.Id);
			}
		}
	}
	return Out;
}

void ACampaign1851Map::RestoreFormations(const TArray<FCampaign1851FormationSave>& Saves)
{
	Formations.Reset();
	NextFormationId = 1;
	for (FCampaign1851Regiment& R : Regiments)
	{
		R.Formation = 0;
	}
	for (const FCampaign1851FormationSave& S : Saves)
	{
		FCampaign1851Formation F;
		F.Id = S.Id;
		F.Name = S.Name;
		F.Echelon = ECampaign1851Echelon(FMath::Min<uint8>(S.Echelon, uint8(ECampaign1851Echelon::Detachment)));
		F.Parent = S.Parent;
		NextFormationId = FMath::Max(NextFormationId, S.Id + 1);
		Formations.Add(F);
		for (const FString& Id : S.Regiments)
		{
			const int32 i = FindRegiment(Id);
			if (i != INDEX_NONE)
			{
				Regiments[i].Formation = F.Id;
			}
		}
		const int32 O = Officers.IndexOfByPredicate([&S](const FCampaign1851Officer& X) { return X.Id == S.Commander; });
		if (O != INDEX_NONE)
		{
			AssignFormationCommander(O, F.Id);
		}
		const FString* Staff[] = { &S.Deputy, &S.StaffChief };
		for (int32 Post = 1; Post <= 2; ++Post)
		{
			const int32 X = Staff[Post - 1]->IsEmpty() ? INDEX_NONE : Officers.IndexOfByPredicate([&](const FCampaign1851Officer& Y) { return Y.Id == *Staff[Post - 1]; });
			if (X != INDEX_NONE)
			{
				AssignFormationStaff(X, F.Id, Post);
			}
		}
	}
}

TArray<FCampaign1851TrainSave> ACampaign1851Map::SaveTrains() const
{
	// A train running empty is saved at its boarding station (it is there on loading).
	TArray<FCampaign1851TrainSave> Out;
	for (const FCampaign1851TroopTrain& T : TroopTrainList)
	{
		FCampaign1851TrainSave& S = Out.AddDefaulted_GetRef();
		S.Id = T.Id;
		const int32 Station = T.IsRunningEmpty() ? T.Board : T.Station;
		S.Station = Cities.IsValidIndex(Station) ? Cities[Station].Name : FString();
		S.Lead = Regiments.IsValidIndex(T.Lead) ? Regiments[T.Lead].Id : FString();
		S.Board = Cities.IsValidIndex(T.Board) ? Cities[T.Board].Name : FString();
		S.Release = Cities.IsValidIndex(T.Release) ? Cities[T.Release].Name : FString();
		S.TransferTo = Cities.IsValidIndex(T.TransferTo) ? Cities[T.TransferTo].Name : FString();
		S.TransferDays = T.TransferDays;
		S.bBoarded = T.bBoarded;
	}
	return Out;
}

void ACampaign1851Map::RestoreTrains(const TArray<FCampaign1851TrainSave>& Saves, const TArray<FVector2D>& Orders)
{
	TroopTrainList.Reset();
	NextTrainId = 1;
	for (const FCampaign1851TrainSave& S : Saves)
	{
		FCampaign1851TroopTrain T;
		T.Id = S.Id;
		T.Station = FindCity(S.Station);
		T.Lead = FindRegiment(S.Lead);
		T.Board = FindCity(S.Board);
		T.Release = FindCity(S.Release);
		T.bBoarded = S.bBoarded;
		T.TransferTo = S.TransferTo.IsEmpty() ? INDEX_NONE : FindCity(S.TransferTo);
		T.TransferDays = T.TransferTo != INDEX_NONE ? S.TransferDays : 0.f;
		NextTrainId = FMath::Max(NextTrainId, S.Id + 1);
		TroopTrainList.Add(T);
	}
	TrainOrders = Orders;
	UpdateTroopTrainPieces();
}

TArray<FCampaign1851RegimentSave> ACampaign1851Map::SaveArmy() const
{
	TArray<FCampaign1851RegimentSave> Out;
	for (const FCampaign1851Regiment& R : Regiments)
	{
		FCampaign1851RegimentSave& S = Out.AddDefaulted_GetRef();
		S.Id = R.Id;
		S.Men = R.Men;
		S.bRaised = R.bRaised;
		S.bTraining = R.bTraining;
		S.RaisingProgress = R.RaisingProgress;
		S.RaisingType = R.RaisingType;
		S.bDetached = R.bDetached;
		S.MaxMen = R.MaxMen;
		S.Companies = R.Captains.Num();
		S.Horses = R.Horses;
		S.MaxHorses = R.MaxHorses;
		S.Guns = R.Guns;
		S.Nation = R.Nation;
		S.CompanyWeight = R.CompanyWeight;
		S.SectionMaxHorses = R.SectionMaxHorses;
		S.SectionHorses = R.SectionHorses;
		S.SectionGuns = R.SectionGuns;
		if (R.bRaised)
		{
			S.Name = R.Name;
			S.Arm = uint8(R.Arm);
			S.Home = Cities.IsValidIndex(R.Home) ? Cities[R.Home].Name : FString();
		}
		S.Morale = R.Morale;
		S.Pace = R.PaceKmPerDay;
		S.Group = R.Group;
		S.Experience = R.Experience;
		S.Skills = TArray<float>(R.Skills, int32(ECampaign1851Skill::Count));
		S.Program = uint8(R.Program);
		S.FireDrills = TArray<float>(R.FireDrills, 4);
		for (const FCampaign1851ServiceEntry& E : R.Service)
		{
			S.Service.Add(FString::Printf(TEXT("%.2f|%s|%d|%d|%d|%d|%d|%d"), E.Day, *E.Place, E.Result, E.Killed, E.Wounded, E.Captured, E.EnemyKilled, E.bFrom3D ? 1 : 0));
		}
		S.Cohesion = R.Cohesion;
		// Where it is (town, or a point), and where it is going: a march is planned again from here on loading.
		S.Town = !R.IsMarching() && Cities.IsValidIndex(R.Town) ? Cities[R.Town].Name : FString();
		S.Km = R.Km;
		if (R.IsMarching())
		{
			S.Destination = Cities.IsValidIndex(R.Destination()) ? Cities[R.Destination()].Name : FString();
			S.DestinationKm = R.DestinationKm();
			S.bMarching = true;
			S.Mode = uint8(R.Mode);
		}
	}
	return Out;
}

int32 ACampaign1851Map::RestoreArmy(const TArray<FCampaign1851RegimentSave>& Saves)
{
	int32 Restored = 0;
	// Old 1825 saves contain the scaled 1851 roster. Keep that saved army rather than adding a second one.
	const bool bLegacy1825Army = ActiveScenario().Id == TEXT("1825") && Saves.ContainsByPredicate([](const FCampaign1851RegimentSave& S)
	{
		return !S.bRaised && !S.Id.StartsWith(TEXT("1825_"));
	});
	if (bLegacy1825Army)
	{
		for (auto& Piece : RegimentPieces) { if (Piece) { Piece->DestroyComponent(); } }
		for (auto& Car : RegimentCars) { if (Car) { Car->DestroyComponent(); } }
		RegimentPieces.Reset();
		RegimentCars.Reset();
		Regiments.Reset();
	}
	TArray<TPair<int32, int32>> Marches;   // regiment, save index
	for (int32 k = 0; k < Saves.Num(); ++k)
	{
		const FCampaign1851RegimentSave& S = Saves[k];
		int32 i = FindRegiment(S.Id);
		if (i == INDEX_NONE && (S.bRaised || bLegacy1825Army) && FindCity(S.Home) != INDEX_NONE)
		{
			i = AddRaisedRegiment(S.Id, S.Name, ECampaign1851Arm(S.Arm), FindCity(S.Home), S.MaxMen);
			if (i != INDEX_NONE)
			{
				Regiments[i].bDetached = S.bDetached;
				if (bLegacy1825Army) { Regiments[i].bRaised = S.bRaised; }
			}
		}
		if (i == INDEX_NONE)
		{
			continue;
		}
		FCampaign1851Regiment& R = Regiments[i];
		// What a split or a move of companies changed (saves before this keep the start's figures).
		if (S.MaxMen > 0) { R.MaxMen = S.MaxMen; }
		if (S.Horses >= 0) { R.Horses = S.Horses; }
		if (S.MaxHorses >= 0) { R.MaxHorses = S.MaxHorses; }
		if (S.Guns >= 0) { R.Guns = S.Guns; }
		if (!S.Nation.IsEmpty()) { R.Nation = S.Nation; }
		R.bTraining = S.bTraining;
		R.RaisingProgress = FMath::Clamp(S.RaisingProgress, 0.f, 1.f);
		R.RaisingType = FMath::Clamp(S.RaisingType, 0, Campaign1851Resources::UnitTypes - 1);
		R.SavedCompanies = S.Companies;
		R.CompanyWeight = S.CompanyWeight;
		R.SectionMaxHorses = S.SectionMaxHorses;
		R.SectionHorses = S.SectionHorses;
		R.SectionGuns = S.SectionGuns;
		R.Men = FMath::Clamp(S.Men, 0, R.MaxMen);
		R.Morale = S.Morale;
		R.Route.Reset();
		R.Leg = 0;
		R.LegElapsed = 0.f;
		R.PaceKmPerDay = S.Pace > 0.f ? S.Pace : Campaign1851Army::MarchKmPerDay(R.Arm);
		R.Group = S.Group;
		if (S.Skills.Num() > 0)   // saves before v7 keep the 1851 values
		{
			R.Experience = S.Experience;
			R.Cohesion = S.Cohesion;
			for (int32 s = 0; s < int32(ECampaign1851Skill::Count) && s < S.Skills.Num(); ++s)
			{
				R.Skills[s] = S.Skills[s];
			}
			R.Program = ECampaign1851Program(FMath::Min<uint8>(S.Program, uint8(ECampaign1851Program::Count) - 1));
		}
		for (int32 d = 0; d < 4 && d < S.FireDrills.Num(); ++d)
		{
			R.FireDrills[d] = S.FireDrills[d];
		}
		R.Service.Reset();
		R.TotalKilled = R.TotalWounded = R.TotalCaptured = R.TotalEnemyKilled = 0;
		for (const FString& Line : S.Service)
		{
			TArray<FString> P;
			Line.ParseIntoArray(P, TEXT("|"), false);
			if (P.Num() >= 8)
			{
				FCampaign1851ServiceEntry E;
				E.Day = FCString::Atod(*P[0]);
				E.Place = P[1];
				E.Result = uint8(FCString::Atoi(*P[2]));
				E.Killed = FCString::Atoi(*P[3]);
				E.Wounded = FCString::Atoi(*P[4]);
				E.Captured = FCString::Atoi(*P[5]);
				E.EnemyKilled = FCString::Atoi(*P[6]);
				E.bFrom3D = P[7] == TEXT("1");
				R.Service.Add(E);
				R.TotalKilled += E.Killed;
				R.TotalWounded += E.Wounded;
				R.TotalCaptured += E.Captured;
				R.TotalEnemyKilled += E.EnemyKilled;
			}
		}
		// In a town, or out in the field (v8 saves keep the point; older ones knew only towns).
		R.Town = FindCity(S.Town);
		const bool bHasPoint = !S.Km.IsZero();
		if (R.Town == INDEX_NONE && !bHasPoint)
		{
			R.Town = R.Home;
		}
		if (bHasPoint && (R.Town == INDEX_NONE || S.bMarching))
		{
			R.Km = S.Km;
		}
		if (S.bMarching || !S.Destination.IsEmpty())
		{
			Marches.Add({ i, k });
		}
		++Restored;
	}
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		if (Regiments[i].Town != INDEX_NONE)
		{
			PlaceInTown(i);
		}
	}
	// Marches carry on from where the column was, by the same kind of route.
	for (const TPair<int32, int32>& M : Marches)
	{
		FCampaign1851Regiment& R = Regiments[M.Key];
		const FCampaign1851RegimentSave& S = Saves[M.Value];
		const int32 Destination = FindCity(S.Destination);
		const FVector2D Target = Destination != INDEX_NONE ? TownKm(Destination) : S.DestinationKm;
		const bool bFromPoint = !S.Km.IsZero() && S.bMarching;
		const int32 From = bFromPoint ? INDEX_NONE : R.Town;
		if (PlanMarch(From, R.Km, Destination, Target, R.PaceKmPerDay, ECampaign1851RouteMode(FMath::Min<uint8>(S.Mode, 2)), R.Route))
		{
			R.Town = INDEX_NONE;
			R.Km = R.Route[0].FromKm;
		}
	}
	AdvanceArmy(0.f, 0.f);
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		UpdateRegimentPiece(i);
	}
	return Restored;
}
