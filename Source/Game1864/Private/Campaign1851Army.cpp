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
		static const TCHAR* Names[] = { TEXT("Føring"), TEXT("Inspiration"), TEXT("Initiativ"), TEXT("Taktik"), TEXT("Stab"), TEXT("Disciplin"), TEXT("Aggressivitet"), TEXT("Nerve"), TEXT("Politisk vægt") };
		return Names[FMath::Clamp(int32(Stat), 0, int32(ECampaign1851OfficerStat::Count) - 1)];
	}

	const TCHAR* StatShort(ECampaign1851OfficerStat Stat)
	{
		static const TCHAR* Names[] = { TEXT("Før"), TEXT("Insp"), TEXT("Init"), TEXT("Takt"), TEXT("Stab"), TEXT("Disc"), TEXT("Aggr"), TEXT("Nerve"), TEXT("Pol") };
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
			const float Own = MarchKmPerDay(R->Arm) * EndurancePaceFactor(R->Skill(ECampaign1851Skill::Endurance));
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
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Campaign1851/Army1851.json")))
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|army|no Data/Campaign1851/Army1851.json"));
		return false;
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
		O->TryGetNumberField(TEXT("horses"), R.Horses);
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
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|army|%d regiments"), ArmyAtStart.Num());
	return ArmyAtStart.Num() > 0;
}

void ACampaign1851Map::ResetArmy()
{
	Regiments = ArmyAtStart;
	ResetOfficers();
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		PlaceInTown(i);
	}
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		UpdateRegimentPiece(i);
	}
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

bool ACampaign1851Map::FindRoute(int32 From, int32 To, float Pace, TArray<FCampaign1851Leg>& OutLegs, bool bRail) const
{
	OutLegs.Reset();
	if (!Cities.IsValidIndex(From) || !Cities.IsValidIndex(To) || From == To)
	{
		return false;
	}
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
	TArray<float> Best;
	Best.Init(TNumericLimits<float>::Max(), Cities.Num());
	TArray<FCampaign1851Leg> Via;
	Via.SetNum(Cities.Num());
	TArray<bool> Done;
	Done.Init(false, Cities.Num());
	Best[From] = 0.f;
	for (;;)
	{
		int32 At = INDEX_NONE;
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			if (!Done[c] && Best[c] < TNumericLimits<float>::Max() && (At == INDEX_NONE || Best[c] < Best[At]))
			{
				At = c;
			}
		}
		if (At == INDEX_NONE || At == To)
		{
			break;
		}
		Done[At] = true;
		const bool bByRail = At != From && Via[At].bRail;
		for (int32 Link = 0; Link < Links.Num(); ++Link)
		{
			if (Links[Link].A != At && Links[Link].B != At)
			{
				continue;
			}
			const FCampaign1851Leg Leg = LegFor(Link, At, bByRail);
			if (Best[At] + Leg.Days < Best[Leg.To])
			{
				Best[Leg.To] = Best[At] + Leg.Days;
				Via[Leg.To] = Leg;
			}
		}
	}
	if (Best[To] == TNumericLimits<float>::Max())
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
	auto Across = [&](int32 A, const FVector2D& AKm, int32 B, const FVector2D& BKm)
	{
		FCampaign1851Leg Leg;
		Leg.bOffRoad = true;
		Leg.From = A;
		Leg.To = B;
		Leg.FromKm = AKm;
		Leg.ToKm = BKm;
		Leg.Days = float(FVector2D::Distance(AKm, BKm)) / (Pace * Campaign1851Army::OffRoadPaceFactor);
		return Leg;
	};
	if ((FromTown != INDEX_NONE && FromTown == ToTown) || FVector2D::Distance(StartKm, EndKm) < 0.05)
	{
		if (OutReason) { *OutReason = TEXT("Den er der allerede"); }
		return false;
	}
	if (Mode == ECampaign1851RouteMode::Direct)
	{
		if (!IsDryLine(StartKm, EndKm))
		{
			if (OutReason) { *OutReason = TEXT("Der er vand i vejen: vælg veje"); }
			return false;
		}
		OutLegs.Add(Across(FromTown, StartKm, ToTown, EndKm));
		return true;
	}
	// By road: across the fields to the nearest town if it starts or ends out there, the roads between.
	const int32 EnterTown = Cities.IsValidIndex(FromTown) ? FromTown : NearestTownFrom(StartKm);
	const int32 LeaveTown = Cities.IsValidIndex(ToTown) ? ToTown : NearestTownFrom(EndKm);
	if (EnterTown == INDEX_NONE || LeaveTown == INDEX_NONE)
	{
		if (OutReason) { *OutReason = TEXT("Ingen vej derhen"); }
		return false;
	}
	if (EnterTown == LeaveTown && !Cities.IsValidIndex(FromTown) && !Cities.IsValidIndex(ToTown) && IsDryLine(StartKm, EndKm))
	{
		OutLegs.Add(Across(INDEX_NONE, StartKm, INDEX_NONE, EndKm));   // both near the same town: straight there
		return true;
	}
	if (!Cities.IsValidIndex(FromTown))
	{
		OutLegs.Add(Across(INDEX_NONE, StartKm, EnterTown, TownKm(EnterTown)));
	}
	TArray<FCampaign1851Leg> RoadLegs;
	if (EnterTown != LeaveTown && !FindRoute(EnterTown, LeaveTown, Pace, RoadLegs, Mode == ECampaign1851RouteMode::RoadsAndRail))
	{
		if (OutReason) { *OutReason = FString::Printf(TEXT("Ingen vej til %s"), *Cities[LeaveTown].Name); }
		OutLegs.Reset();
		return false;
	}
	OutLegs.Append(RoadLegs);
	if (!Cities.IsValidIndex(ToTown))
	{
		OutLegs.Add(Across(LeaveTown, TownKm(LeaveTown), INDEX_NONE, EndKm));
	}
	return OutLegs.Num() > 0;
}

FVector ACampaign1851Map::RegimentWorld(int32 Regiment) const
{
	return Regiments.IsValidIndex(Regiment) ? WorldAtKm(Regiments[Regiment].Km) : FVector::ZeroVector;
}

// ------------------------------------------------------------------ orders

bool ACampaign1851Map::OrderMarch(const TArray<int32>& Column, int32 CityIndex, FString* OutReason)
{
	return OrderMarchTo(Column, CityIndex, TownKm(CityIndex), ECampaign1851RouteMode::RoadsAndRail, OutReason);
}

bool ACampaign1851Map::OrderMarchTo(const TArray<int32>& Column, int32 CityIndex, const FVector2D& TargetKm, ECampaign1851RouteMode Mode, FString* OutReason)
{
	TArray<const FCampaign1851Regiment*> Members;
	for (int32 i : Column)
	{
		if (Regiments.IsValidIndex(i))
		{
			Members.Add(&Regiments[i]);
		}
	}
	if (Members.Num() == 0)
	{
		return false;
	}
	if (Cities.IsValidIndex(CityIndex) && Cities[CityIndex].bForeign)
	{
		if (OutReason) { *OutReason = TEXT("Hæren går ikke over grænsen i fredstid"); }
		return false;
	}
	if (!Cities.IsValidIndex(CityIndex) && !IsMonarchyLand(TargetKm))
	{
		if (OutReason) { *OutReason = TEXT("Kun på monarkiets land"); }
		return false;
	}
	// The column marches at its slowest pace. Regiments already on the march finish the stretch
	// under way and go on from its end; the others set out together.
	float Pace = Campaign1851Army::ColumnPace(Members);
	if (const FCampaign1851Officer* General = ColumnGeneral(Column))
	{
		Pace *= Campaign1851Army::StaffPaceFactor(General->Stat(ECampaign1851OfficerStat::Staff));
	}
	static int32 NextGroup = 1;
	const int32 Group = Column.Num() > 1 ? NextGroup++ : 0;
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
		TArray<FCampaign1851Leg> Legs;
		if (!PlanMarch(StartTown, StartKm, CityIndex, TargetKm, Pace, Mode, Legs, OutReason))
		{
			continue;
		}
		R.PaceKmPerDay = Pace;
		R.Group = Group;
		R.Mode = Mode;
		if (bMarching)
		{
			TArray<FCampaign1851Leg> Route = { R.Route[R.Leg] };
			Route.Append(Legs);
			R.Route = MoveTemp(Route);
			R.Leg = 0;
		}
		else
		{
			R.Route = MoveTemp(Legs);
			R.Leg = 0;
			R.LegElapsed = 0.f;
			R.Town = INDEX_NONE;
			// It sets out from the town itself (not the camp), or from where it stands in the field.
			R.Km = R.Route[0].FromKm;
		}
		++Ordered;
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|army|%s marches to %s (%s)|%d legs|%.1f days|pace %.1f km/day"), *R.Name, *DescribePlace(CityIndex, TargetKm),
			Campaign1851Army::RouteModeName(Mode), R.Route.Num(), R.DaysLeft(), Pace);
		UpdateRegimentPiece(i);
	}
	return Ordered > 0;
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
		if (R.IsMarching())
		{
			// A fit unit and a good chief keep it together on the road; the march itself hardens it a little.
			R.Cohesion = FMath::Max(40.f, R.Cohesion - DeltaDays * 0.4f * (1.2f - Lead / 12.f) * (1.3f - Endurance / 100.f));
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
					V = FMath::Min(Cap, V + DeltaDays * 0.12f * Weight * (0.5f + Lead / 10.f) * FMath::Max(0.1f, 1.f - V / 100.f));
				}
				else if (Weight <= 0.f)
				{
					V = FMath::Max(30.f, V - DeltaDays * (Skill == ECampaign1851Skill::Endurance ? 0.03f : 0.01f));
				}
			}
			R.Cohesion = FMath::Min(90.f, R.Cohesion + DeltaDays * 0.3f);
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
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		FCampaign1851Regiment& R = Regiments[i];
		if (!R.IsMarching())
		{
			continue;
		}
		R.LegElapsed += DeltaDays;
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
	}
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
	// The formation faces +X: turn it to the heading (world Y is south).
	Piece->SetWorldLocationAndRotation(WorldAtKm(R.Km) + FVector(0.0, 0.0, 0.5),
		FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(-R.Heading.Y, R.Heading.X)) + float(GetActorRotation().Yaw), 0.f));
	Piece->SetVisibility(LastCameraDistanceKm < FormationMaxDistanceKm);
}

// ------------------------------------------------------------------ save

TArray<FCampaign1851RegimentSave> ACampaign1851Map::SaveArmy() const
{
	TArray<FCampaign1851RegimentSave> Out;
	for (const FCampaign1851Regiment& R : Regiments)
	{
		FCampaign1851RegimentSave& S = Out.AddDefaulted_GetRef();
		S.Id = R.Id;
		S.Men = R.Men;
		S.Morale = R.Morale;
		S.Pace = R.PaceKmPerDay;
		S.Group = R.Group;
		S.Experience = R.Experience;
		S.Skills = TArray<float>(R.Skills, int32(ECampaign1851Skill::Count));
		S.Program = uint8(R.Program);
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
	TArray<TPair<int32, int32>> Marches;   // regiment, save index
	for (int32 k = 0; k < Saves.Num(); ++k)
	{
		const FCampaign1851RegimentSave& S = Saves[k];
		const int32 i = FindRegiment(S.Id);
		if (i == INDEX_NONE)
		{
			continue;
		}
		FCampaign1851Regiment& R = Regiments[i];
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
