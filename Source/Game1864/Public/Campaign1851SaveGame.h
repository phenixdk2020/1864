#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Campaign1851SaveGame.generated.h"

/** A line in the state's account book. */
USTRUCT()
struct FCampaign1851Transaction
{
	GENERATED_BODY()

	UPROPERTY() FDateTime Date;
	UPROPERTY() double Amount = 0.0;   // rigsdaler; negative = spent
	UPROPERTY() FString Text;
};

/** One town's garrison complex: days built per module (-1 = not started) and the module under way. */
USTRUCT()
struct FCampaign1851ProjectSave
{
	GENERATED_BODY()

	/** Town name rather than index, so saves survive changes to the town list. */
	UPROPERTY() FString City;
	UPROPERTY() TArray<float> ModuleDays;
	UPROPERTY() int32 ActiveModule = INDEX_NONE;
	/** "Garrison" (or empty in old saves) or a town building key; its plot and heading. */
	UPROPERTY() FString Kind;
	UPROPERTY() FVector2D PlotKm = FVector2D::ZeroVector;
	UPROPERTY() float Yaw = 0.f;
	/** v12: raised by private investors (no wages or upkeep from the state). */
	UPROPERTY() bool bPrivate = false;
	/** v18: standing in 1851 already. */
	UPROPERTY() bool bHistoric = false;
	/** v16: being pulled down. */
	UPROPERTY() bool bDemolishing = false;
	UPROPERTY() float DemolishDays = 0.f;
	UPROPERTY() float DemolishWages = 0.f;
	UPROPERTY() float DemolishDone = 0.f;
	UPROPERTY() TArray<float> DemolishFrom;
};

/** A field fortification (v13). */
USTRUCT()
struct FCampaign1851FortSave
{
	GENERATED_BODY()

	UPROPERTY() int32 Id = 0;
	UPROPERTY() FString Name;
	UPROPERTY() FVector2D Km = FVector2D::ZeroVector;
	UPROPERTY() float Yaw = 0.f;
	UPROPERTY() bool bLarge = false;
	UPROPERTY() int32 Guns = 0;
	UPROPERTY() int32 Defence = 1;
	UPROPERTY() int32 Garrison = 0;
	UPROPERTY() bool bBuilt = false;
	UPROPERTY() uint8 Work = 0;
	UPROPERTY() float DaysBuilt = 0.f;
	UPROPERTY() float WorkDays = 1.f;
	UPROPERTY() int32 WorkCost = 0;
	/** v14: the companies in it ("B6:2:190") and its trenches. */
	UPROPERTY() TArray<FString> Companies;
	UPROPERTY() bool bTrenches = false;
	UPROPERTY() double Invested = 0.0;
};

/** A nation's control and (without a map) its abstract state (v12). */
USTRUCT()
struct FCampaign1851NationSave
{
	GENERATED_BODY()

	UPROPERTY() FString Id;
	UPROPERTY() bool bPlayer = false;
	UPROPERTY() TArray<uint8> Modes;
	UPROPERTY() double Reserve = 0.0;
	UPROPERTY() double Population = 0.0;
	UPROPERTY() double Treasury = 0.0;
	UPROPERTY() double RailKm = 0.0;
	UPROPERTY() double ArmyMen = 0.0;
	UPROPERTY() double Industry = 1.0;
};

/** A decision or recommendation of a ministry (v12). */
USTRUCT()
struct FCampaign1851DecisionSave
{
	GENERATED_BODY()

	UPROPERTY() double Day = 0.0;
	UPROPERTY() int32 Nation = 0;
	UPROPERTY() uint8 Portfolio = 0;
	UPROPERTY() uint8 Kind = 0;
	UPROPERTY() FString Action;
	UPROPERTY() FString Reasons;
	UPROPERTY() double Cost = 0.0;
	UPROPERTY() bool bDone = false;
	UPROPERTY() bool bAdvice = false;
	UPROPERTY() int32 A = -1;
	UPROPERTY() int32 B = 0;
	UPROPERTY() FString Key;
};

/** A link of the road network with work done on it: paved, railway built, or a project under way. */
USTRUCT()
struct FCampaign1851LinkSave
{
	GENERATED_BODY()

	/** The link's towns by name. */
	UPROPERTY() FString A;
	UPROPERTY() FString B;
	UPROPERTY() bool bChaussee = false;
	UPROPERTY() bool bRailway = false;
	/** ECampaign1851LinkWork of the project under way (0 = none), its days done, and what it costs. */
	UPROPERTY() uint8 Work = 0;
	UPROPERTY() float DaysBuilt = 0.f;
};

/** A regiment's state: where it stands or which stretch it is on, where it is going, its strength. */
USTRUCT()
struct FCampaign1851RegimentSave
{
	GENERATED_BODY()
	/** v29: per-unit names, palette and paid weapon conversion. */
	UPROPERTY() FString CustomName;
	UPROPERTY() FString OriginalName;
	UPROPERTY() TArray<int32> UniformPalette;
	UPROPERTY() int32 WeaponLevel = -1;
	UPROPERTY() int32 PendingWeaponLevel = -1;
	UPROPERTY() float WeaponConversionDays = 0.f;

	UPROPERTY() FString Id;
	/** The town it stands in, or the town the current stretch started from. */
	UPROPERTY() FString Town;
	/** Marching: the end of the current stretch, the days spent on it and the final destination. */
	UPROPERTY() FString LegTo;
	UPROPERTY() float LegElapsed = 0.f;
	UPROPERTY() FString Destination;
	/** v8: its position, and a march's goal in the field and kind of route. */
	UPROPERTY() FVector2D Km = FVector2D::ZeroVector;
	UPROPERTY() FVector2D DestinationKm = FVector2D::ZeroVector;
	UPROPERTY() bool bMarching = false;
	UPROPERTY() uint8 Mode = 0;
	UPROPERTY() int32 Men = 0;
	UPROPERTY() float Morale = 0.8f;
	/** Pace (km/day) and column of the march under way. */
	UPROPERTY() float Pace = 0.f;
	UPROPERTY() int32 Group = 0;
	/** Unit qualities (v7). */
	UPROPERTY() float Experience = 0.f;
	UPROPERTY() TArray<float> Skills;
	UPROPERTY() TArray<float> FireDrills;
	/** Service record: day|place|result|killed|wounded|captured|enemy|3d per battle. */
	UPROPERTY() TArray<FString> Service;
	UPROPERTY() uint8 Program = 1;
	UPROPERTY() float Cohesion = 0.f;
	/** v15: raised during the campaign, with its definition. */
	UPROPERTY() bool bRaised = false;
	/** v28: initial garrison training; older saves retain deployed units. */
	UPROPERTY() bool bTraining = false;
	UPROPERTY() float RaisingProgress = 0.f;
	UPROPERTY() int32 RaisingType = 0;
	UPROPERTY() bool bDetached = false;
	UPROPERTY() FString Name;
	UPROPERTY() uint8 Arm = 0;
	UPROPERTY() FString Home;
	UPROPERTY() int32 MaxMen = 0;
	/** v17: what a split or a move of companies changed: the number of companies (-1: as at the start), horses, guns, nation. */
	UPROPERTY() int32 Companies = -1;
	UPROPERTY() int32 Horses = -1;
	UPROPERTY() int32 MaxHorses = -1;
	UPROPERTY() int32 Guns = -1;
	UPROPERTY() FString Nation;
	UPROPERTY() TArray<float> CompanyWeight;
	UPROPERTY() TArray<float> SectionGuns;
	UPROPERTY() TArray<float> SectionHorses;
	UPROPERTY() TArray<float> SectionMaxHorses;
};

/** A formation of the field army (v11): its place in the tree, its commander and its regiments. */
USTRUCT()
struct FCampaign1851FormationSave
{
	GENERATED_BODY()

	UPROPERTY() int32 Id = 0;
	UPROPERTY() FString Name;
	UPROPERTY() uint8 Echelon = 2;
	UPROPERTY() int32 Parent = 0;
	UPROPERTY() FString Commander;          // officer id
	UPROPERTY() FString Deputy;             // officer id (headquarters staff)
	UPROPERTY() FString StaffChief;         // officer id
	UPROPERTY() TArray<FString> Regiments;  // regiment ids directly in it
};

/** A troop train (v10): where it stands, and the column it serves. */
USTRUCT()
struct FCampaign1851TrainSave
{
	GENERATED_BODY()

	UPROPERTY() int32 Id = 0;
	UPROPERTY() FString Station;
	UPROPERTY() FString Lead;       // regiment id
	UPROPERTY() FString Board;
	UPROPERTY() FString Release;
	UPROPERTY() bool bBoarded = false;
	UPROPERTY() FString TransferTo;   // moved to another railway (empty: not)
	UPROPERTY() float TransferDays = 0.f;
};

/** An officer (v7): who he is, his qualities and experience, and the regiment he serves with (empty = pool). */
USTRUCT()
struct FCampaign1851OfficerSave
{
	GENERATED_BODY()

	UPROPERTY() FString Id;
	UPROPERTY() FString Name;
	UPROPERTY() FString Rank;
	UPROPERTY() int32 Born = 1800;
	UPROPERTY() bool bGeneral = false;
	UPROPERTY() bool bRecruited = false;
	UPROPERTY() TArray<uint8> Stats;
	UPROPERTY() float Experience = 0.f;
	UPROPERTY() FString Regiment;
	/** v9: the general command he leads (id), if any. */
	UPROPERTY() FString Command;
	/** v11: the battalion (id) and company he leads as captain, if any. */
	UPROPERTY() FString CaptainOf;
	UPROPERTY() int32 Company = INDEX_NONE;
	/** v18: wounded (1) or a prisoner (2), and until when (ISO date). */
	UPROPERTY() uint8 Away = 0;
	UPROPERTY() FString AwayUntil;
	UPROPERTY() TArray<FString> Career;
};

/**
 * A saved 1851 campaign (Saved/SaveGames/<slot>.sav). Holds the construction projects, the
 * camera view and the selected town. Bump SaveVersion when the layout changes and migrate in
 * ACampaign1851PlayerController::LoadFromSlot.
 */
UCLASS()
class GAME1864_API UCampaign1851SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Version history reconstructed from the field comments (no layout change here).
	 * 1: projects/view; 2: date/speed; 3: treasury/ledger; 4: town buildings;
	 * 5: roads/railways; 6: regiments; 7: officers/unit qualities; 8: field marches;
	 * 9: general commands; 10: troop trains; 11: field formations/company captains;
	 * 12: campaign seed/deviation, populations, nations, decisions/private projects;
	 * 13: field forts; 14: fort companies/trenches; 15: raised units/amt manpower;
	 * 16: demolition, gun/material stores; 17: supply and split/company equipment;
	 * 18: historical projects and wounded/captured officers; 19: supply columns;
	 * 20: rifle/horse stocks; 21: footing/present strength; 22: war state;
	 * 23: diplomacy/research/doctrine; 24: navy; 25: politics; 26: economy;
	 * 27: bridges; 28: initial garrison training; 29: event files (fired and blocked events);
	 * 30: the player's settings (graphics, figures, shadows, couriers, camera speed, ...).
	 * 31: civil research project id and elapsed months in Research.
	 * 32: after-action reports, prisoner holders, extended service entries and officer careers.
	 */
	static constexpr int32 CurrentVersion = 32;

    // v32: exact report, prisoner pools, extended service entries and officer careers; old saves default to empty.
    UPROPERTY() FString LastAfterActionReport;
    UPROPERTY() TMap<FString, int32> PrisonersByNation;
	UPROPERTY() int32 SaveVersion = CurrentVersion;
	/** The player's settings at the time of the save (the battle's settings section of GameUserSettings: graphics preset, figure scale, shadows,
	 *  couriers, enemy range, camera speed, ...). They come back when the game is loaded. */
	UPROPERTY() TMap<FString, FString> Settings;
	/** The scenario it was played in (an index into ACampaign1851Map::Scenarios; saves from before the scenarios are 1851 = 1). */
	UPROPERTY() int32 Scenario = 1;
	UPROPERTY() FDateTime SavedAt;
	/** One line for the load menu, e.g. "Aalborg: kaserne, stalde". */
	UPROPERTY() FString Summary;

	UPROPERTY() FVector CameraTarget = FVector::ZeroVector;
	UPROPERTY() float CameraDistanceKm = 960.f;
	UPROPERTY() float CameraYaw = 0.f;
	UPROPERTY() FString SelectedCity;

	UPROPERTY() TArray<FCampaign1851ProjectSave> Projects;

	/** Days since 1 July 1851, and the game speed step. */
	UPROPERTY() double CampaignDays = 0.0;
	UPROPERTY() int32 Speed = 1;

	/** The state's cash (rigsdaler) and its recent account book. */
	UPROPERTY() double Treasury = 0.0;
	UPROPERTY() TArray<FCampaign1851Transaction> Ledger;

	/** Chausséer and railways built by the player, and link projects under way. */
	UPROPERTY() TArray<FCampaign1851LinkSave> Links;

	/** The regiments (missing ones stay in their garrisons). */
	UPROPERTY() TArray<FCampaign1851RegimentSave> Regiments;

	/** The officer corps (v7). */
	UPROPERTY() TArray<FCampaign1851OfficerSave> Officers;

	/** Troop trains (v10) and trains on order (count, day of delivery). */
	UPROPERTY() TArray<FCampaign1851TrainSave> Trains;

	/** The field army's formations (v11). */
	UPROPERTY() TArray<FCampaign1851FormationSave> Formations;
	UPROPERTY() TArray<FVector2D> TrainOrders;

	/** The world (v12): campaign seed and historical deviation, grown towns and amter, nations, decisions. */
	UPROPERTY() int32 Seed = 1851;
	UPROPERTY() float Deviation = 0.f;
	UPROPERTY() TArray<int32> CityPopulation;
	UPROPERTY() TArray<int32> AmtUrban;
	UPROPERTY() TArray<int32> AmtRural;
	UPROPERTY() TArray<FCampaign1851NationSave> Nations;
	UPROPERTY() TArray<FCampaign1851DecisionSave> Decisions;

	/** Supply (v17): depots, units and forts, one line each. */
	UPROPERTY() TArray<FString> Supply;
	/** Supply columns (v19): how many, and the loads of those under way. */
	UPROPERTY() TArray<FString> SupplyColumns;
	/** Materiel stores (v20): rifles, horses (the guns are GunStock). */
	UPROPERTY() int32 Rifles = 0;
	UPROPERTY() int32 HorseStock = 0;
	/** Footing (v21) and each unit's men present (in Supply lines as "present|id|share"). */
	UPROPERTY() uint8 Footing = 0;
	/** War and peace (v22; v29 adds blocked events and event-format migration). */
	UPROPERTY() TArray<FString> War;
	/** Foreign affairs (v23): relations, treaties, the Sound Dues, ceded towns. */
	UPROPERTY() TArray<FString> Diplomacy;
	/** Research and doctrine (v23). */
	UPROPERTY() TArray<FString> Research;
	/** The navy (v24): ships, blockade. */
	UPROPERTY() TArray<FString> Navy;
	/** Government and opinion (v25). */
	UPROPERTY() TArray<FString> Politics;
	/** Loans, the monthly record and the newspaper's log (v26). */
	UPROPERTY() TArray<FString> Economy;
	/** Bridges blown, rebuilt or laid (v27). */
	UPROPERTY() TArray<FString> Bridges;

	/** The state's store of fortress guns and the materials stores (town, rd., day) (v16). */
	UPROPERTY() int32 GunStock = 0;
	UPROPERTY() TArray<FVector> MaterialLots;

	/** Each amt's reserve of trained men (v15). */
	UPROPERTY() TArray<float> AmtManpower;

	/** Field fortifications (v13). */
	UPROPERTY() TArray<FCampaign1851FortSave> Forts;
};
