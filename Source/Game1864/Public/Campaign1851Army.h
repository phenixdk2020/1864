#pragma once

#include "CoreMinimal.h"

/**
 * Regiments on the 1851 campaign map (design manual 7: the unit model; its simulation numbers are
 * authoritative, the counters and miniatures only show them). They stand in their garrison towns and
 * march along the road network (Campaign1851Network): on foot at the road or chaussée pace, by train
 * where a railway is open, across the ferries.
 */

/** Arm of service; decides the counter symbol and the miniature. */
enum class ECampaign1851Arm : uint8
{
	Infantry,
	Guard,
	Jager,
	Cavalry,
	Artillery,        // foot artillery: horse-drawn guns, the gunners walk
	HorseArtillery    // horse artillery: every gunner mounted, keeps up with the cavalry
};

/**
 * A troop train (locomotive and some twenty carriages: a battalion, 250 horses or a battery). It stands at
 * a station, runs empty to a column's boarding station, carries the column and stays where it set it down.
 */
struct FCampaign1851TroopTrain
{
	int32 Id = 0;
	int32 Station = INDEX_NONE;     // where it stands (INDEX_NONE while running)
	int32 Lead = INDEX_NONE;        // lead regiment of the column it serves (INDEX_NONE = free)
	int32 Board = INDEX_NONE;       // where the column boards
	int32 Release = INDEX_NONE;     // where the column leaves the train (the train stays there)
	bool bBoarded = false;
	TArray<struct FCampaign1851Leg> Path;   // the empty run to Board
	int32 PathLeg = 0;
	float PathElapsed = 0.f;
	FVector2D Km = FVector2D::ZeroVector;

	bool IsFree() const { return Lead == INDEX_NONE; }
	bool IsRunningEmpty() const { return PathLeg < Path.Num(); }
};

/** An officer's qualities (design manual 8), each 1-10. Aggression runs from cautious (1) to bold (10). */
enum class ECampaign1851OfficerStat : uint8
{
	Leadership,    // Føring: morale, recovery, getting subordinates to act
	Inspiration,   // Inspiration: lifting and steadying the troops' will to fight
	Initiative,    // Initiativ: acting within the intent without new orders
	Tactical,      // Taktik: flanks, ground, reserves, timing
	Staff,         // Stab: orders, reports, coordination; the pace of a column
	Discipline,    // Disciplin: how closely orders are followed
	Aggression,    // Aggressivitet: attack and pursuit versus caution
	Composure,     // Nerve: keeping the overview under pressure
	Political,     // Politisk vægt: appointments, dismissals, prestige
	Caution,       // Forsigtighed: scouting, covering the flanks, not walking into a trap
	Count
};

/**
 * A regimental chief or a general. Chiefs command one regiment; a general is attached to a regiment
 * as its headquarters and commands the stack or column that regiment is in.
 */
struct FCampaign1851Officer
{
	FString Id;
	FString Name;
	FString Rank;
	int32 Born = 1800;
	bool bGeneral = false;
	uint8 Stats[int32(ECampaign1851OfficerStat::Count)] = { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };
	/** 0-100; grows with days in the field (and later with battles). */
	float Experience = 0.f;
	/** The regiment commanded (chief) or accompanied (general); INDEX_NONE = unassigned, in the pool. */
	int32 Regiment = INDEX_NONE;
	/** Hired during the campaign (not part of the 1851 officer corps). */
	bool bRecruited = false;
	/** A general commanding a general command (FCampaign1851Command index), INDEX_NONE otherwise. */
	int32 Command = INDEX_NONE;
	/** The field formation he commands (FCampaign1851Formation::Id), 0 = none. */
	int32 Formation = 0;
	/** A company chief (kaptajn): the regiment (battalion) index and the company in it, INDEX_NONE otherwise. */
	int32 CaptainOf = INDEX_NONE;
	int32 Company = INDEX_NONE;
	/** A staff post at a formation's headquarters (FCampaign1851Formation::Id, 0 = none): 1 deputy, 2 chief of staff. */
	int32 StaffOf = 0;
	int32 StaffPost = 0;
	bool IsFree() const { return Regiment == INDEX_NONE && Command == INDEX_NONE && Formation == 0 && CaptainOf == INDEX_NONE && StaffOf == 0; }

	int32 Stat(ECampaign1851OfficerStat S) const { return Stats[int32(S)]; }
};

/**
 * What a unit has trained for (design manual 20.9: training, readiness and combat experience are
 * separate). 0-100 each; the 3D battles read them (Campaign1851Army::BattleFactors).
 */
enum class ECampaign1851Skill : uint8
{
	Loading,       // Ladegreb og ilddisciplin: reload time, orderly volleys, ammunition spent
	Marksmanship,  // Skydning: accuracy and range judgement (gunnery for the artillery)
	Drill,         // Eksercits: formations, turning and deploying, keeping the ranks
	Fieldcraft,    // Feltøvelse: cover, skirmishing, use of ground
	Endurance,     // Udholdenhed: march pace, fatigue, recovery (and the horses' condition for mounted arms)
	Assault,       // Bajonet og storm: the charge and close combat
	Count
};

/** A regiment's training programme in garrison: what it trains, and what it costs a month. */
enum class ECampaign1851Program : uint8
{
	Rest,          // Hvile: nothing trained, nothing spent; skills slowly fall
	Drill,         // Eksercits
	LiveFire,      // Skydeøvelser (powder and ball)
	Field,         // Feltøvelser
	March,         // Marchøvelser (long marches in full kit; riding for mounted arms)
	Assault,       // Bajonet og storm
	Mixed,         // Blandet: a little of everything
	Count
};

/** How a march order goes: by road and railway, by road only, or straight across country. */
enum class ECampaign1851RouteMode : uint8
{
	RoadsAndRail,   // fastest: roads, chausséer, ferries and open railways
	RoadsOnly,      // on foot along the roads as far as they go (no trains)
	Direct          // a straight line across the fields, slower than the road
};

/** Levels of the field army (design manual: Army -> Division -> Brigade -> Regiment; levels optional). */
enum class ECampaign1851Echelon : uint8
{
	Army,
	Division,
	Brigade,
	Regiment,     // two battalions under an oberstløjtnant
	Detachment
};

/**
 * A formation of the field army that the player puts together (the operational order of battle):
 * a name, a level, a parent formation (0 = directly under the field army) and a commander.
 * Regiments belong to at most one formation; those in none stay in garrison under their general command.
 */
struct FCampaign1851Formation
{
	int32 Id = 0;
	FString Name;
	ECampaign1851Echelon Echelon = ECampaign1851Echelon::Brigade;
	int32 Parent = 0;
	int32 Commander = INDEX_NONE;   // officer index
	/** The headquarters staff: the deputy takes over (acting) when the chief is missing; the chief of staff's
	 *  staff work sets the pace of the formation's columns. Officer indices. */
	int32 Deputy = INDEX_NONE;
	int32 StaffChief = INDEX_NONE;
};

/**
 * A general command (generalkommando) of the peacetime army: a region's towns, its headquarters and its
 * commanding general. Regiments belong to the command of their garrison (the order of battle, OOB).
 */
struct FCampaign1851Command
{
	FString Id;
	FString Name;
	FString Area;
	int32 HQ = INDEX_NONE;
	TArray<int32> Towns;
	int32 General = INDEX_NONE;   // officer index
};

/**
 * One stretch of a march: a link walked (or ridden by train) from one town to the next, or a stretch
 * across country (bOffRoad) between two points; From/To are INDEX_NONE for a point that is no town.
 */
struct FCampaign1851Leg
{
	int32 Link = INDEX_NONE;
	int32 From = INDEX_NONE;
	int32 To = INDEX_NONE;
	FVector2D FromKm = FVector2D::ZeroVector;
	FVector2D ToKm = FVector2D::ZeroVector;
	bool bRail = false;
	bool bOffRoad = false;
	/** Waiting at a station for the trains (From = To, no distance). */
	bool bWait = false;
	/** Part of a link only (km along it from its From end); a negative LineTo means the whole link. */
	float LineTo = -1.f;
	float Days = 0.f;
};

/** A column's march worked out before it is ordered (for the order dialog and for the order itself). */
struct FCampaign1851MarchPlan
{
	bool bOk = false;
	ECampaign1851RouteMode Mode = ECampaign1851RouteMode::RoadsAndRail;   // may fall back to roads (no trains)
	float Pace = 20.f;
	TArray<FCampaign1851Leg> Route;
	float Days = 0.f;          // to the goal, from now
	float WaitDays = 0.f;      // waiting for trains at the station
	int32 TrainsNeeded = 0;
	TArray<int32> Trains;      // the troop trains it takes (indices)
	TArray<TArray<FCampaign1851Leg>> TrainPaths;   // their empty runs to the boarding station
	int32 Board = INDEX_NONE;
	int32 Release = INDEX_NONE;
	FString TrainSource;       // "2 tog fra Roskilde, 1 tog holder i København"
	FString Note;              // why it cannot, or why it marches instead
};

struct FCampaign1851Regiment
{
	FString Id;
	FString Name;
	FString Nation = TEXT("DK");
	ECampaign1851Arm Arm = ECampaign1851Arm::Infantry;

	int32 Men = 0;
	int32 MaxMen = 0;
	int32 Horses = 0;
	int32 Guns = 0;
	float Morale = 0.8f;
	/** 0-100: field and battle experience (the army of 1851 are veterans of 1848-50). */
	float Experience = 55.f;
	/** Trained skills (ECampaign1851Skill), 0-100, kept up in garrison by the training programme. */
	float Skills[int32(ECampaign1851Skill::Count)] = { 60.f, 50.f, 60.f, 55.f, 55.f, 50.f };
	ECampaign1851Program Program = ECampaign1851Program::Drill;
	float Skill(ECampaign1851Skill S) const { return Skills[int32(S)]; }
	/** Mean of the trained skills. */
	float MeanSkill() const
	{
		float Sum = 0.f;
		for (float V : Skills) { Sum += V; }
		return Sum / float(int32(ECampaign1851Skill::Count));
	}
	/** 0-100: how well the unit holds together (falls with long marches, rises at rest). */
	float Cohesion = 70.f;
	/** The general command it belongs to (order of battle). */
	int32 Command = INDEX_NONE;
	/** Its formation in the field army (FCampaign1851Formation::Id), 0 = in garrison. */
	int32 Formation = 0;
	/** The chiefs (officer index) of its companies; empty for units without companies (cavalry, batteries). */
	TArray<int32> Captains;
	/** Per company: the fort (id) it holds, 0 = with the battalion. Its men are then not in Men. */
	TArray<int32> CompanyFort;
	/** Officer indices: the regiment's chief, and a general whose headquarters marches with it. */
	int32 Chief = INDEX_NONE;
	int32 General = INDEX_NONE;

	int32 Home = INDEX_NONE;    // garrison town
	int32 Town = INDEX_NONE;    // where it stands; INDEX_NONE while marching

	/** March orders: the legs to go, the one under way and the time spent on it (days). */
	TArray<FCampaign1851Leg> Route;
	int32 Leg = 0;
	float LegElapsed = 0.f;
	/** Road pace of the column it marches with (km/day; the slowest arm and the column's length). */
	float PaceKmPerDay = 20.f;
	/** Regiments ordered together share a group and march as one column (0 = alone). */
	int32 Group = 0;
	/** Where the current order found it (to go back to when the order is cancelled). */
	int32 OriginTown = INDEX_NONE;
	FVector2D OriginKm = FVector2D::ZeroVector;
	/** How the current march was ordered (for re-planning after a load). */
	ECampaign1851RouteMode Mode = ECampaign1851RouteMode::RoadsAndRail;
	FVector2D Km = FVector2D::ZeroVector;   // position on the map now (projected km)
	FVector2D Heading = FVector2D(1.0, 0.0);

	bool IsMarching() const { return Route.IsValidIndex(Leg); }
	/** Standing somewhere that is not a town (after a march across country). */
	bool IsInField() const { return !IsMarching() && Town == INDEX_NONE; }
	/** Where the march ends: a town (or INDEX_NONE for a point in the field, see DestinationKm). */
	int32 Destination() const { return Route.Num() > 0 ? Route.Last().To : Town; }
	FVector2D DestinationKm() const { return Route.Num() > 0 ? Route.Last().ToKm : Km; }
	/** Days left to the destination. */
	float DaysLeft() const
	{
		float Days = 0.f;
		for (int32 l = Leg; l < Route.Num(); ++l)
		{
			Days += Route[l].Days - (l == Leg ? LegElapsed : 0.f);
		}
		return FMath::Max(Days, 0.f);
	}
};

namespace Campaign1851Army
{
	ECampaign1851Arm ParseArm(const FString& Text);
	/** "Linjeinfanteri", "Garde", "Jægere", "Kavaleri", "Artilleri", "Ridende artilleri". */
	const TCHAR* ArmName(ECampaign1851Arm Arm);
	/** Marching pace of the arm on a road (km/day); a chaussée adds a third. */
	float MarchKmPerDay(ECampaign1851Arm Arm);
	/**
	 * Pace of a column (km/day on a road): the slowest arm in it, and a long column is slower still,
	 * 4 % per thousand men beyond two thousand, down to three quarters. OutWhy explains it.
	 */
	float ColumnPace(const TArray<const FCampaign1851Regiment*>& Column, FString* OutWhy = nullptr);
	/** "Føring", "Inspiration", ... and the short form for tables ("Før"). */
	const TCHAR* StatName(ECampaign1851OfficerStat Stat);
	const TCHAR* StatShort(ECampaign1851OfficerStat Stat);
	/** "Rekrutter", "Øvede", "Erfarne", "Veteraner", "Elite" for 0-100. */
	const TCHAR* ExperienceName(float Experience);
	/** 0-5 stars for an officer's experience. */
	int32 Stars(float Experience);
	const TCHAR* SkillName(ECampaign1851Skill Skill);
	const TCHAR* ProgramName(ECampaign1851Program Program);
	/** What a programme trains (weight per skill, 0-1). */
	float ProgramWeight(ECampaign1851Program Program, ECampaign1851Skill Skill);
	/** Days a programme needs to raise a skill by 10 from Current under a chief of this Leadership (0 = it does not, or the ceiling stops it). */
	float DaysForTen(ECampaign1851Program Program, ECampaign1851Skill Skill, float Current, int32 Leadership);
	/** Cost per month of a programme for a unit of 760 men (scaled by strength). */
	int32 ProgramCostPerMonth(ECampaign1851Program Program);
	/** March pace factor of a unit's endurance: 20 = -10 %, 55 = 1.0, 90 = +8 %. */
	inline float EndurancePaceFactor(float Endurance) { return 0.88f + 0.0022f * Endurance; }
	/** What the 3D battles take from a unit's training (multipliers on the base values). */
	struct FBattleFactors
	{
		float ReloadTime = 1.f;     // 1.3 raw .. 0.85 well trained
		float Accuracy = 1.f;       // 0.7 .. 1.25
		float DeploySpeed = 1.f;    // 0.75 .. 1.2
		float Skirmish = 1.f;       // 0.7 .. 1.2
		float FatigueRate = 1.f;    // 1.3 .. 0.75 (lower = tires more slowly)
		float Assault = 1.f;        // 0.75 .. 1.25
		float Morale = 0.8f;        // current morale 0-1
		float Cohesion = 0.7f;      // current cohesion 0-1
		float Experience = 0.55f;   // 0-1
	};
	FBattleFactors BattleFactors(const FCampaign1851Regiment& Regiment);
	/** Officer ranks, lowest first: Kaptajn ... General; generals from Generalmajor (index 4). */
	const TArray<FString>& Ranks();
	int32 RankIndex(const FString& Rank);
	constexpr int32 FirstGeneralRank = 4;
	/** Pay per year of a rank (rigsdaler). */
	int32 RankPay(int32 Rank);
	/** Experience needed to be promoted to a rank. */
	float RankExperience(int32 Rank);
	/** Troop trains a regiment fills: a battalion per 800 men, cavalry per 250 horses, a battery each. */
	int32 TrainsNeeded(const FCampaign1851Regiment& Regiment);
	/** Across country a column goes at this share of its road pace (hedges, ditches, ploughed fields). */
	constexpr float OffRoadPaceFactor = 0.6f;
	const TCHAR* RouteModeName(ECampaign1851RouteMode Mode);
	/** "Hær", "Division", "Brigade", "Afdeling"; and the map symbol's size mark (XXXX, XX, X, II). */
	const TCHAR* EchelonName(ECampaign1851Echelon Echelon);
	const TCHAR* EchelonMark(ECampaign1851Echelon Echelon);
	/** The chief's function of a formation: "Divisionschef", "Brigadechef", "Regimentschef". */
	const TCHAR* FormationRole(ECampaign1851Echelon Echelon);
	/** A headquarters staff post: 0 chief (FormationRole), 1 "Næstkommanderende", 2 "Stabschef" / "Adjudant". */
	const TCHAR* StaffPostName(ECampaign1851Echelon Echelon, int32 Post);
	/** The chief's function of a unit: "Bataljonschef", "Kavaleriofficer", "Batterichef". */
	const TCHAR* UnitRole(ECampaign1851Arm Arm);
	/** The map symbol of a unit: II (battalion), CAV, ART. */
	const TCHAR* ArmMark(ECampaign1851Arm Arm);
	/** Companies of a unit: four for a battalion (infantry, jægere, the Guard), none for the others. */
	int32 CompaniesFor(ECampaign1851Arm Arm);
	/** A general's effect on the pace of his column: Stab 5 = 1.0, each point ±1 %. */
	inline float StaffPaceFactor(int32 Staff) { return 0.95f + 0.01f * Staff; }
}
