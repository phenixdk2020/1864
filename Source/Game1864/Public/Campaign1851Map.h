#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Campaign1851SaveGame.h"
#include "Campaign1851Network.h"
#include "Campaign1851Army.h"
#include "Campaign1851Nation.h"
#include "Campaign1851Fort.h"
#include "Campaign1851Map.generated.h"

class UStaticMeshComponent;
class FJsonObject;
class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;
class ACampaign1851ConstructionSite;
class UMaterialParameterCollection;
class UStaticMesh;
class FJsonObject;

/** A line of the state's monthly budget (the treasury window). */
struct FCampaign1851BudgetLine
{
	FString Text;
	double PerMonth = 0.0;   // rigsdaler; income positive, spending negative
};

/** Region codes used by the 1851 data: K Kingdom, S Schleswig, H Holstein/Lauenburg. */
/** A depot's room and what it holds: rations (a man a day), fodder (a horse a day), ammunition (battalion loads). */
struct FCampaign1851DepotCapacity
{
	float Food = 0.f;
	float Fodder = 0.f;
	float Ammo = 0.f;
};
struct FCampaign1851DepotStock
{
	float Food = 0.f;
	float Fodder = 0.f;
	float Ammo = 0.f;
};

/** A supply column on the roads: from a depot to a unit or a fort, and home again. */
enum class ESupplyColumnState : uint8 { Outbound, Returning };
struct FCampaign1851SupplyColumn
{
	int32 Id = 0;
	int32 Depot = INDEX_NONE;     // town it loads at and returns to
	bool bFort = false;
	int32 Target = INDEX_NONE;    // regiment index, or fort id
	float Food = 0.f, Fodder = 0.f, Ammo = 0.f;
	ESupplyColumnState State = ESupplyColumnState::Outbound;
	FVector2D Km = FVector2D::ZeroVector;
	TArray<FCampaign1851Leg> Route;
	int32 Leg = 0;
	float LegElapsed = 0.f;
};

/** An enemy corps on the map (war): its strength and the towns it marches on in turn. */
struct FCampaign1851EnemyCorps
{
	int32 Id = 0;
	FString Name;
	FString Nation;          // "PR", "AT", "DE" (the Confederation)
	int32 Men = 0;
	int32 Guns = 0;
	FVector2D Km = FVector2D::ZeroVector;
	int32 Town = INDEX_NONE; // where it stands (INDEX_NONE on the march)
	TArray<int32> Objectives;
	TArray<FCampaign1851Leg> Route;
	int32 Leg = 0;
	float LegElapsed = 0.f;
	bool bEngaged = false;   // in contact with Danish troops: a battle is at hand
	double RestUntil = 0.0;  // after a battle: waits (campaign day) before marching on
	int32 StartMen = 0;      // strength when it took the field (reinforcements up to half again)
	// What the Danes know of it (the fog of war): seen now, or last seen or reported.
	bool bSeen = true;
	FVector2D SeenKm = FVector2D::ZeroVector;
	double SeenDay = -1.0;
	int32 SeenMen = 0;       // the estimate (cavalry counts well, towns by rumour)
	FVector2D ReportKm = FVector2D::ZeroVector;
	double ReportDay = -1.0;
	double ReportArrive = -1.0;   // a report on its way (courier or telegraph)
	int32 ReportMen = 0;
	// The enemy's own thinking.
	double NextThink = 0.0;
	bool bWaitingNoted = false;
	double CrossingReadyDay = -1.0;   // boats gathered for a narrow sound (the Danish fleet holds the sea)
	TArray<int32> Barred;             // towns it cannot reach while the Danish fleet holds the sea
	int32 SiegeTown = INDEX_NONE;     // the position it besieges (or marches to besiege)
	bool bSieging = false;            // dug in before it: no battle until it storms
	double SiegeStart = 0.0;
};

/** A class of warship (Campaign1851Navy.cpp). */
struct FCampaign1851ShipClass
{
	const TCHAR* Name;
	float Strength;         // in the balance at sea (a ship of the line 10)
	int32 Year;             // can be ordered from (0: not built any more)
	double Cost;
	int32 Months;
	double UpkeepPerYear;
};

struct FCampaign1851Ship
{
	FString Name;
	int32 Class = 0;
	int32 Built = 1851;
	double ReadyDay = 0.0;  // on the stocks until then
};

namespace Campaign1851Navy
{
	const TArray<FCampaign1851ShipClass>& Classes();
}

/** A battle at hand: the Danish units and forts within reach of an enemy corps. */
struct FCampaign1851Battle
{
	int32 Id = 0;
	double Day = 0.0;
	FVector2D Km = FVector2D::ZeroVector;
	int32 Town = INDEX_NONE;
	int32 CorpsId = 0;
	TArray<int32> Regiments;
	TArray<int32> Forts;     // ids
	bool bWaiting = false;   // sent to the 3D battle; waiting for its result file
};

/** What a battle cost and decided (from the 3D battle, the automatic resolution, or a retreat). */
struct FCampaign1851BattleOutcome
{
	bool bDanishWin = false;
	bool bDraw = false;
	bool bRetreat = false;
	bool bFromBattle3D = false;
	TMap<int32, int32> UnitLosses;   // regiment index -> men lost
	TMap<int32, int32> UnitKills;    // regiment index -> enemy men it put out of the fight
	TMap<int32, float> UnitAmmo;     // regiment index -> share of the load used
	TMap<int32, float> FortLossShare;
	TArray<int32> CapturedForts;
	int32 EnemyLosses = 0;
};

/** A historical event on the road to 1864, as this campaign has it (date and weight varied, or skipped). */
struct FPlannedEvent
{
	FString Id;
	FString Text;
	double Day = 0.0;
	float Tension = 0.f;
	bool bSkip = false;
};

/** A bridge on a road link, or a sound where a pontoon bridge can be laid (Campaign1851Bridges.cpp). */
enum class EBridgeState : uint8 { Intact, Blown, Building, Site };
enum class EBridgeAction : uint8 { Blow, Rebuild, Build };

struct FCampaign1851Bridge
{
	int32 Id = 0;
	FString Name;
	int32 Link = INDEX_NONE;
	FVector2D Km = FVector2D::ZeroVector;
	float LengthM = 0.f;
	EBridgeState State = EBridgeState::Intact;
	float DaysLeft = 0.f;
	float FerryKm = 0.f;     // > 0: a pontoon bridge over this ferry's sound
	FVector2D EndA = FVector2D::ZeroVector, EndB = FVector2D::ZeroVector;   // the banks it joins (km)
	TArray<int32> Links;     // every road link over it (several roads may share one bridge)
};

/** Raw materials in the state's stores (Campaign1851Resources.cpp). */
enum class ECampaign1851Raw : uint8 { Iron, Coal, Timber, Powder, Cloth, Leather, Count };

namespace Campaign1851Resources
{
	struct FRawInfo
	{
		const TCHAR* Name;
		const TCHAR* Unit;
		double Price;      // bought abroad (rd. a unit)
		float Start;       // in store in 1851
		float Country;     // what the country gives a month of itself
		bool bImported;    // mostly from abroad
	};
	const FRawInfo& Info(ECampaign1851Raw R);
	/** A type of unit that can be raised. */
	struct FUnitType
	{
		const TCHAR* Name;
		const TCHAR* NameSuffix;
		const TCHAR* IdPrefix;
		ECampaign1851Arm Arm;
		int32 Men, Rifles, Guns, Horses, Uniforms, Leather;
		double CostFactor;
		int32 Mortars = 0;
		int32 Wagons = 0;
	};
	constexpr int32 UnitTypes = 6;
	constexpr double MortarPrice = 900.0;   // bought abroad
	constexpr double WagonPrice = 60.0;     // bought in the country
	const FUnitType& Type(int32 T);
	int32 RaiseParts(int32 T, int32 Size);
	FUnitType SizedType(int32 T, int32 Size);
	FString RaiseSizeName(int32 T, int32 Size);
}

/** The battlefield generator (Campaign1851Battlefield.cpp). */
enum class EBattlefieldCell : uint8 { Field, Sea, Meadow, Wood, Town, Water };

/** Field boundaries (Campaign1851Hydro.cpp): knicks (hedges on banks), stone or earth dikes, marsh ditches. */
enum class EHedgeKind : uint8 { Knick, Dike, Ditch };

/** A river or canal (Data/Campaign1851/Hydro1851.json), projected km from the source to the sea. */
struct FCampaign1851River
{
	FString Name;
	int32 Class = 1;           // 0 brook, 1 river, 2 large river, 3 the Elbe
	bool bCanal = false;
	float WidthM = 20.f;
	TArray<FVector2D> Km;
};

/** A lake: a wobbled ellipse. */
struct FCampaign1851Lake
{
	FString Name;
	FVector2D CentreKm = FVector2D::ZeroVector;
	float A = 1.f, B = 1.f, RotDeg = 0.f;   // semi-axes km; the long axis from east towards north
	uint32 Seed = 0;
	TArray<FVector2D> Km;                   // the shore
};

struct FCampaign1851BattleRiver
{
	FString Name;
	float WidthM = 10.f;
	TArray<FVector2D> M;
};

struct FCampaign1851BattleHedge
{
	EHedgeKind Kind = EHedgeKind::Knick;
	TArray<FVector2D> M;
};

struct FCampaign1851BattleCrossing
{
	FString Kind;     // bridge (a lane over a river) or ford (a track through a brook)
	FString River;
	FVector2D M = FVector2D::ZeroVector;
};

struct FCampaign1851BattleBuilding
{
	FVector2D M = FVector2D::ZeroVector;   // metres from the south-west corner (x east, y north)
	FVector2D Size = FVector2D(10.0, 10.0);
	float Yaw = 0.f;
	FString Kind;                          // house, farm, garrison, or a civil building's key
};

struct FCampaign1851BattleTown
{
	FString Name;
	FVector2D M = FVector2D::ZeroVector;
	float RadiusM = 0.f;
};

struct FCampaign1851Battlefield
{
	FString Name;
	FString Place;
	FVector2D CentreKm = FVector2D::ZeroVector;
	double Lat = 0.0, Lon = 0.0;
	float SizeKm = 8.f;
	double Day = 0.0;
	bool bSnow = false;
	TArray<float> HeightM;      // grid 256 x 256, row 0 at the south
	TArray<uint8> Kind;         // EBattlefieldCell
	TArray<uint8> Wood;         // 0-100
	TArray<TArray<FVector2D>> Roads, Lanes, Tracks, Chaussees, Rails;
	TArray<FCampaign1851BattleBuilding> Buildings;
	TArray<FCampaign1851BattleTown> Towns;
	TArray<struct FCampaign1851Fort> Forts;
	TArray<FVector2D> FortM;
	TArray<FVector2D> Villages;
	TArray<FVector2D> FarmM;
	TArray<struct FCampaign1851Bridge> Bridges;
	TArray<FVector2D> BridgeM;
	TArray<FCampaign1851BattleRiver> Rivers;
	TArray<TArray<FVector2D>> Lakes;
	TArray<FCampaign1851BattleHedge> Hedges;
	TArray<FCampaign1851BattleCrossing> Crossings;
	int32 Farms = 0;
	TArray<FColor> Pixels;      // the picture, 512 x 512, row 0 at the north
	TArray<FColor> GroundPixels;   // the same before the ways, buildings and boundaries are drawn (the 3D ground)
	bool IsValid() const { return HeightM.Num() > 0; }
};

/** A line of the final score (Campaign1851Endgame.cpp). */
struct FCampaign1851ScoreLine
{
	FString Text;
	float Points = 0.f;
};

/** Export goods (Campaign1851Economy.cpp). */
enum class ECampaign1851Good : uint8 { Grain, Cattle, Butter, Count };

namespace Campaign1851Economy
{
	const TCHAR* GoodName(ECampaign1851Good G);
}

/** The country's state at a month's end (the statistics). */
struct FCampaign1851Record
{
	double Day = 0.0;
	double Population = 0.0;
	double Treasury = 0.0;
	double ArmyMen = 0.0;
	double RailKm = 0.0;
	float Tension = 0.f;
	float Mood = 0.f;
	float Grain = 1.f;
	double Debt = 0.0;
};

/** The currents of opinion (Campaign1851Politics.cpp). */
enum class ECampaign1851Current : uint8 { Helstat, Ejder, Scandinavian };

namespace Campaign1851Politics
{
	const TCHAR* CurrentName(ECampaign1851Current C);
	const TCHAR* CurrentEffect(ECampaign1851Current C);
}

/** A minister (Campaign1851Ministers.cpp): qualities 1-10. */
struct FCampaign1851Minister
{
	FString Name;
	ECampaign1851Current Line = ECampaign1851Current::Helstat;
	uint8 Skill = 5;     // how well the ministry chooses and how far its money goes
	uint8 Thrift = 5;    // how much it holds back
	uint8 Caution = 5;   // how bold its foreign, naval and financial steps are
	double Since = 0.0;
};

/** A peace the Danes may offer (Campaign1851Diplomacy.cpp). */
struct FCampaign1851PeaceOffer
{
	FString Name;
	TArray<int32> Ceded;   // town indices
	bool bAccepted = false;
	FString Why;
};

/** The day's weather (Campaign1851Weather.cpp). */
enum class ECampaign1851Weather : uint8 { Clear, Rain, Snow, Frost, Thaw, Storm };

namespace Campaign1851Weather
{
	const TCHAR* Name(ECampaign1851Weather W);
}

/** A research project of the war ministry (Campaign1851Research.cpp). */
struct FCampaign1851ResearchTopic
{
	const TCHAR* Id;
	const TCHAR* Name;
	const TCHAR* Effect;
	int32 Year;            // open from this year
	double CostPerMonth;
	int32 Months;
	const TCHAR* Needs;    // another topic first (or null)
	int32 Branch;          // the column of the research tree (Campaign1851Research::BranchName)
};

namespace Campaign1851Research
{
	/** A civil subject (farming, trades, roads, railways, the telegraph) against a military one. */
	bool IsCivil(int32 Topic);
	const TArray<FCampaign1851ResearchTopic>& Topics();
	int32 FindTopic(const FString& Id);
	constexpr int32 Branches = 8;
	/** The level in the research tree (0 = I). */
	int32 Tier(int32 Topic);
	const TCHAR* Roman(int32 Tier);
	const TCHAR* BranchName(int32 Branch);
	/** Doctrine levels: 0 strategic, 1 operational, 2 tactical. */
	int32 DoctrineChoices(int32 Level);
	const TCHAR* LevelName(int32 Level);
	const TCHAR* DoctrineName(int32 Level, int32 Choice);
	const TCHAR* DoctrineEffect(int32 Level, int32 Choice);
}

/** A nation's figures for the council window (from the map for Denmark, the abstract model for the others). */
struct FCampaign1851NationFigures
{
	double Population = 0.0;
	double YearlyBudget = 0.0;
	double ArmyMen = 0.0;
	double RailKm = 0.0;
	float Growth = 0.f;   // % a year
};

struct FCampaign1851City
{
	FString Name;
	double Lat = 0.0;
	double Lon = 0.0;
	int32 Population = 0;
	FString Region;
	bool bCapital = false;
	bool bBornholm = false;
	bool bForeign = false;
	FVector World = FVector::ZeroVector;
	/** Amt id (FCampaign1851Amt::Id); 0 for foreign towns. */
	int32 AmtId = 0;
	/** The nation holding the town in war ("PR", "AT"); empty when it is the kingdom's own. */
	FString Occupier;
	/** Ceded at a peace: it has left the monarchy (bForeign is then true). */
	bool bCeded = false;
	/** Days Danish troops have held an occupied town free of enemy corps (liberated at 2). */
	float LiberationDays = 0.f;

	/** Military building plot beside a main road at the edge of town (towns of 2,500+). */
	bool bHasPlot = false;
	FVector2D PlotKm = FVector2D::ZeroVector;
	float PlotYaw = 0.f;   // world yaw of the site; its +Y (parade ground, gate) faces the road
};

/**
 * An amt c. 1851 (county; Schleswig/Holstein amter and landscapes): the administrative unit
 * that holds population and pays taxes (design manual 4.2: regions as administrative containers).
 * Population is an estimate: town figures plus the region's rural population spread by area.
 */
struct FCampaign1851Amt
{
	int32 Id = 0;
	FString Name;
	FString Seat;
	FString Region;   // K, S, H
	int32 Population = 0;
	int32 Urban = 0;
	int32 Rural = 0;
	float AreaKm2 = 0.f;
	TArray<FString> Towns;
};

struct FCampaign1851Label
{
	FString Text;
	double Lat = 0.0;
	double Lon = 0.0;
	FString Kind;   // sea, strait, land, duchy, foreign
	FVector World = FVector::ZeroVector;
};

/**
 * Lambert Azimuthal Equal-Area on a sphere (as EPSG:3035 "ETRS89-LAEA Europe", centre 52N 10E).
 * Must match tools/map1851/build_map.py, which paints the map in the same projection.
 */
struct FCampaign1851Projection
{
	double Lat0 = 52.0, Lon0 = 10.0, RadiusKm = 6371.0088;

	/** Geographic degrees -> projected km (x east, y north). */
	FVector2D Forward(double Lat, double Lon) const
	{
		const double Lam = FMath::DegreesToRadians(Lon - Lon0);
		const double Phi = FMath::DegreesToRadians(Lat);
		const double Phi0 = FMath::DegreesToRadians(Lat0);
		const double K = FMath::Sqrt(2.0 / (1.0 + FMath::Sin(Phi0) * FMath::Sin(Phi) + FMath::Cos(Phi0) * FMath::Cos(Phi) * FMath::Cos(Lam)));
		return FVector2D(RadiusKm * K * FMath::Cos(Phi) * FMath::Sin(Lam),
			RadiusKm * K * (FMath::Cos(Phi0) * FMath::Sin(Phi) - FMath::Sin(Phi0) * FMath::Cos(Phi) * FMath::Cos(Lam)));
	}

	/** Projected km -> geographic degrees (latitude X, longitude Y): the inverse Lambert azimuthal equal-area. */
	FVector2D Inverse(const FVector2D& P) const
	{
		const double Phi0 = FMath::DegreesToRadians(Lat0);
		const double Rho = P.Size();
		if (Rho < 1e-9)
		{
			return FVector2D(Lat0, Lon0);
		}
		const double C = 2.0 * FMath::Asin(FMath::Clamp(Rho / (2.0 * RadiusKm), -1.0, 1.0));
		const double Lat = FMath::Asin(FMath::Cos(C) * FMath::Sin(Phi0) + P.Y * FMath::Sin(C) * FMath::Cos(Phi0) / Rho);
		const double Lon = FMath::DegreesToRadians(Lon0) + FMath::Atan2(P.X * FMath::Sin(C), Rho * FMath::Cos(Phi0) * FMath::Cos(C) - P.Y * FMath::Sin(Phi0) * FMath::Sin(C));
		return FVector2D(FMath::RadiansToDegrees(Lat), FMath::RadiansToDegrees(Lon));
	}
};

/** A projected rectangle (km) covered by a map texture. */
struct FCampaign1851Extent
{
	double XMin = 0.0, XMax = 1.0, YMin = 0.0, YMax = 1.0;
	FCampaign1851Projection Projection;

	/** 0..1 across the rectangle; U east, V north. */
	FVector2D ToUv(double Lat, double Lon) const
	{
		const FVector2D P = Projection.Forward(Lat, Lon);
		return FVector2D((P.X - XMin) / (XMax - XMin), (P.Y - YMin) / (YMax - YMin));
	}
};

/**
 * The painted campaign map of the Danish monarchy, 1851 (port of Strategy-Campaign5 v00.00.14).
 *
 * Map art comes from tools/map1851/build_map.py: the colour/detail textures are imported assets,
 * the height map and city/label data are read from <Project>/Data/Campaign1851 at load time.
 * Scale: 1 km = KmToUnits Unreal units; the map is centred on the actor. +X east, +Y south
 * (Unreal is left-handed, so north is -Y), +Z up.
 */
UCLASS()
class GAME1864_API ACampaign1851Map : public AActor
{
	GENERATED_BODY()

public:
	ACampaign1851Map();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Unreal units per kilometre. */
	static constexpr double KmToUnits = 100.0;

	/** Scenery pieces are drawn this much larger than modelled, board-game style. */
	static constexpr float PieceScale = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Map")
	TObjectPtr<UMaterialInterface> MapMaterial;

	UPROPERTY(EditAnywhere, Category = "Map")
	TObjectPtr<UTexture2D> BornholmTexture;

	/** Vertical exaggeration: the full height range in km (real Danish relief is ~0.17 km). */
	UPROPERTY(EditAnywhere, Category = "Map")
	float HeightRangeKm = 3.f;

	/** Camera distance (km) below which the main roads are shown. */
	UPROPERTY(EditAnywhere, Category = "Map")
	float RoadsMaxDistanceKm = 250.f;

	/** Camera distance (km) below which railways show as track rather than as the black-and-white map symbol. */
	UPROPERTY(EditAnywhere, Category = "Map")
	float RailTrackMaxDistanceKm = 20.f;

	/** Camera distance (km) below which the monarchy's red town dots are hidden (the 3D towns show instead). */
	UPROPERTY(EditAnywhere, Category = "Map")
	float CityDotsMinDistanceKm = 70.f;

	/** Camera distance (km) below which the 3D towns, farms, woods and village lanes are shown. */
	UPROPERTY(EditAnywhere, Category = "Map")
	float SceneryMaxDistanceKm = 100.f;

	/** World position of a coordinate, on the terrain surface. */
	FVector Project(double Lat, double Lon) const;

	const TArray<FCampaign1851City>& GetCities() const { return Cities; }
	const TArray<FCampaign1851Label>& GetLabels() const { return Labels; }
	const TArray<FCampaign1851Amt>& GetAmter() const { return Amter; }
	const FCampaign1851Amt* FindAmt(int32 Id) const { return Amter.FindByPredicate([Id](const FCampaign1851Amt& A) { return A.Id == Id; }); }
	/** Amt under a world position on the main map (0 = sea or foreign). */
	int32 AmtAtWorld(const FVector& World) const;
	/** Lights up an amt on the map (0 = none). */
	void SetHighlightedAmt(int32 Id);

	/** Taxes per head and year that go to construction (the state's development share). */
	static double EconomyValue(const TCHAR* Key, double Value1851);
	static double RuralTaxRate() { return EconomyValue(TEXT("ruralTax"), RuralTaxPerHead); }
	static double UrbanTaxRate() { return EconomyValue(TEXT("urbanTax"), UrbanTaxPerHead); }
	static constexpr double RuralTaxPerHead = 0.2;
	static constexpr double UrbanTaxPerHead = 0.45;
	static double AmtYearlyTax(const FCampaign1851Amt& Amt) { return Amt.Rural * RuralTaxRate() + Amt.Urban * UrbanTaxRate(); }
	/** The trades researched (the Næringsliv branch): the farms' and the towns' taxes, and the works' output. */
	double RuralTaxFactor() const;
	double UrbanTaxFactor() const;
	float WorksOutputFactor() const;
	/** Yearly tax income of a region (K, S, H) or of the whole monarchy (empty). */
	double YearlyTax(const FString& Region = FString()) const;
	static FString RegionName(const FString& Code);
	const FCampaign1851Extent& GetBornholmExtent() const { return Bornholm; }
	FVector2D GetSizeKm() const { return SizeKm; }
	bool IsReady() const { return bReady; }

	// ---- Campaign clock: days since 1 July 1851 00:00, running minute by minute (a 24-hour clock as in Hearts of Iron IV),
	// and the game speed (0 = paused, 1..5).

	/** Moves the clock on by the real time passed, in whole minutes; drives the projects and the season. */
	void AdvanceTime(float DeltaSeconds);
	void SetSpeed(int32 InSpeed) { const int32 New = FMath::Clamp(InSpeed, 0, NumSpeeds() - 1); if (New != Speed) { MinuteCarry = 0.0; NextDayAt = 0.0; } Speed = New; }
	int32 GetSpeed() const { return Speed; }
	static int32 NumSpeeds() { return 7; }
	/** Campaign hours per real second at a speed step (0, 0.25, 1, 3, 8, 24, 96); speed 6 jumps a whole day at a time. */
	static float HoursPerSecondAt(int32 InSpeed);
	static const TCHAR* SpeedLabel(int32 InSpeed);
	/** "14:37" */
	static FString FormatClock(const FDateTime& Date) { return FString::Printf(TEXT("%02d:%02d"), Date.GetHour(), Date.GetMinute()); }
	double GetCampaignDays() const { return CampaignDays; }
	void SetCampaignDays(double Days) { CampaignDays = FMath::Max(FMath::RoundToDouble(Days * 1440.0) / 1440.0, 0.0); MinuteCarry = 0.0; UpdateSeason(); }
	/** The scenarios the campaign can start in: 1825 (the default, a peaceful beginning that leads to the war of 1848) and 1851
	 *  (just after the three years' war). The choice is kept in the settings and takes effect when the map is loaded. */
	struct FScenario
	{
		FString Id;
		FString Name;
		FString Text;
		int32 Year = 1851;
		float PopulationFactor = 1.f;   // the towns' and amter's people against the 1851 figures
		float ArmyFactor = 1.f;         // the peacetime army against the 1851 one
		float ExperienceFactor = 1.f;   // the army's seasoning (1851 has just fought a war)
		float StartTension = 25.f;
	};
	static const TArray<FScenario>& Scenarios();
	static int32 ScenarioIndex();
	static const FScenario& ActiveScenario() { return Scenarios()[ScenarioIndex()]; }
	static void SetScenarioIndex(int32 Index);
	static FDateTime StartDate() { return FDateTime(ActiveScenario().Year, 7, 1); }
	FDateTime GetDate() const { return StartDate() + FTimespan::FromDays(CampaignDays); }
	/** "1. juli 1851", or "1. jul. 1851" with bShort. */
	static FString FormatDate(const FDateTime& Date, bool bShort = false);
	/** "Vinter", "Forår", "Sommer" or "Efterår". */
	FString GetSeasonName() const;

	// ---- Treasury (design manual 20.2: money / state credit; backlog B-346 budget with a transaction log).

	/** Cash at the start of a campaign; income comes from the amter's taxes (YearlyTax). */
	// TEST (2026-09-29): 5,000,000 while the army and buildings are being tried out; the campaign value is 150,000.
	static constexpr double StartingTreasury = 5000000.0;
	double GetTreasury() const { return Treasury; }
	const TArray<FCampaign1851Transaction>& GetLedger() const { return Ledger; }
	/** This month's budget as it stands: taxes by region, then every kind of spending (construction at today's pace). */
	TArray<FCampaign1851BudgetLine> MonthlyBudget() const;
	/** Upkeep of finished buildings per month. */
	double GetMonthlyUpkeep() const;
	/** Books money in or out (negative = spent) with a reason. */
	void AddTransaction(double Amount, const FString& Text);
	/** A new campaign's cash and an empty account book. */
	void ResetEconomy();
	void RestoreEconomy(double InTreasury, const TArray<FCampaign1851Transaction>& InLedger);
	/** True if the down payment on a building of this price can be paid now. */
	bool CanAfford(int32 CostRd) const;

	/**
	 * Starts (or returns the running) barracks project in a town; null if the town has no plot
	 * or the treasury cannot pay the down payment. bCharge = false when restoring a save.
	 */
	ACampaign1851ConstructionSite* StartProject(int32 CityIndex, bool bCharge = true);
	ACampaign1851ConstructionSite* FindProject(int32 CityIndex) const;

	// ---- Town buildings (arsenal, lazaret, coastal battery, ...), each on its own plot.

	/** The town's site for a building, if it has been started. */
	ACampaign1851ConstructionSite* FindBuilding(int32 CityIndex, const FString& Key) const;
	/**
	 * Why a town cannot have a building (not counting money): "kræver 10.000 indb.", "fra 1854", ...
	 * Empty when it can; "-" when the building does not belong to the town at all (the list hides it).
	 */
	FString BuildingBlockReason(int32 CityIndex, const FString& Key) const;
	/**
	 * Starts a town building: finds a plot by the building's rule (or uses ForcedKm/ForcedYaw from a
	 * save), clears houses and trees there, pays the down payment when bCharge. Null with OutReason on failure.
	 */
	ACampaign1851ConstructionSite* StartBuilding(int32 CityIndex, const FString& Key, bool bCharge = true, const FVector2D* ForcedKm = nullptr, float ForcedYaw = 0.f, FString* OutReason = nullptr);
	bool IsCoastalTown(int32 CityIndex) const;
	/** Town radius (km) as the scenery lays the town out. */
	static float TownRadiusKm(int32 Population) { return FMath::Clamp(0.75f * FMath::Sqrt(Population / 1000.f), 0.9f, 6.5f); }
	/** Removes every building project (before loading a save). */
	void ClearProjects();
	/** Recreates a saved project (garrison or town building); returns false if the town is unknown or has no plot. */
	bool RestoreProject(const FString& CityName, const TArray<float>& ModuleDays, int32 ActiveModule, const FString& Kind, const FVector2D& PlotKm, float Yaw);
	int32 FindCity(const FString& Name) const;

	/** Starts a garrison module (1..) at a town whose barracks is finished; false if it cannot start. */
	bool StartModule(int32 CityIndex, int32 Module);
	const TArray<TObjectPtr<ACampaign1851ConstructionSite>>& GetProjects() const { return Projects; }
	/** World position of a town's building plot (on the terrain). */
	FVector PlotWorld(int32 CityIndex) const;

	// ---- Manpower: each amt's reserve, replacements and new battalions (Campaign1851Manpower.cpp).

	/** The amt (index) a town lies in. */
	int32 AmtIndexOfTown(int32 CityIndex) const;
	/** Trained men in an amt's reserve, the fit men (ceiling) and the yearly class. */
	float GetManpower(int32 AmtIndex) const { return AmtManpower.IsValidIndex(AmtIndex) ? AmtManpower[AmtIndex] : 0.f; }
	float ManpowerCap(int32 AmtIndex) const;
	float YearlyClass(int32 AmtIndex) const;
	/** Why no battalion can be raised in a town (empty if it can). */
	FString RaiseBlockReason(int32 CityIndex) const;
	/** Raises a battalion of recruits at the town's barracks from its amt's reserve; its index or INDEX_NONE. */
	int32 RaiseBattalion(int32 CityIndex, FString* OutReason = nullptr);
	bool DeployRaisedUnit(int32 RegimentIndex);
	/** Upkeep a month of the battalions raised since 1851. */
	double RaisedUpkeepPerMonth() const;
	/** A battalion's strength to fill up to (less its companies in forts). */
	int32 BattalionTarget(int32 Regiment) const;
	const TArray<float>& GetAmtManpower() const { return AmtManpower; }
	void SetAmtManpower(const TArray<float>& In) { if (In.Num() == AmtManpower.Num()) { AmtManpower = In; } }

	// ---- Supply (Campaign1851Supply.cpp).

	/** Room of a town's depot (garrison depot, grain store, arsenal); zero where there is none. */
	FCampaign1851DepotCapacity DepotCapacity(int32 CityIndex) const;
	FCampaign1851DepotStock DepotStock(int32 CityIndex) const;
	/** The depot (town) that feeds a point (within a day's march, with something in it), or INDEX_NONE. */
	int32 DepotFor(const FVector2D& Km) const;
	/** What filling the depots costs a month at the current rate. */
	double StockingCostPerMonth() const;
	void ResetSupply();
	TArray<FString> SaveSupply() const;
	void RestoreSupply(const TArray<FString>& Lines);
	/** Writes Saved/Battle/Units.json for the 3D battles. */
	void ExportUnits() const;
	/** Sends a supply column from the nearest depot with stock to a unit (regiment index) or a fort (id). */
	bool SendSupplyColumn(bool bFort, int32 Target, FString* OutReason = nullptr);
	const TArray<FCampaign1851SupplyColumn>& GetSupplyColumns() const { return SupplyColumns; }
	int32 GetSupplyColumnCount() const { return SupplyColumnCount; }
	int32 FreeSupplyColumns() const;
	/** Buys a new column (wagons and horses); false if the treasury cannot. */
	bool BuySupplyColumn();
	TArray<FString> SaveSupplyColumns() const;
	void RestoreSupplyColumns(const TArray<FString>& Lines);

	// ---- Foreign affairs (Campaign1851Diplomacy.cpp).

	enum class EDiplomacyAction : uint8 { Envoy, Trade, Alliance, Guarantee };
	void ResetDiplomacy();
	/** Why an action cannot be taken now (empty: it can; "-": it does not apply to this nation). */
	FString DiplomacyBlockReason(int32 NationIndex, EDiplomacyAction Action) const;
	bool DoDiplomacy(int32 NationIndex, EDiplomacyAction Action, FString* OutReason = nullptr);
	/** Factor on rises of the tension (each guarantor takes a fifth off). */
	float GuaranteeDamping() const;
	/** The Sound Dues (or their redemption) and the trade treaties, rd. a year. */
	double ForeignIncomePerYear() const;
	int32 TradeTreaties() const;
	bool IsSoundDuesAbolished() const { return bSoundDuesAbolished; }
	bool HasPeaceConference() const { return PeaceTalksDay >= 0.0; }
	/** -1 .. 1: how the war stands (losses traded, towns held, the blockade). */
	float WarScore() const;
	/** The peaces on offer and whether the enemy would take them. */
	TArray<FCampaign1851PeaceOffer> PeaceOffers() const;
	/** Offers a peace; false (with the reason) if the enemy refuses. */
	bool MakePeace(int32 Offer, FString* OutReason = nullptr);
	double AllianceCostNow() const { return AllianceCost * (Government == ECampaign1851Current::Scandinavian ? 0.5 : 1.0); }

	// ---- Raw materials, equipment and new units (Campaign1851Resources.cpp).

	void ResetResources();
	float GetRaw(ECampaign1851Raw R) const { return RawStock[int32(R)]; }
	int32 GetMortarStock() const { return MortarStock; }
	int32 GetWagonStock() const { return WagonStock; }
	/** Buys mortars (abroad) or wagons (in the country) for the store. */
	bool BuyKit(bool bMortars, int32 Count, FString* OutReason = nullptr);
	double RawPrice(ECampaign1851Raw R) const;
	bool CanImport(FString* OutReason = nullptr) const;
	bool BuyRaw(ECampaign1851Raw R, float Amount, FString* OutReason = nullptr);
	/** A month's making and use of each raw material (the use at the arms works' full output). */
	void RawFlow(float OutMade[int32(ECampaign1851Raw::Count)], float OutUsed[int32(ECampaign1851Raw::Count)]) const;
	bool RaiseTownOk(int32 Town) const;
	TArray<int32> RaiseTowns() const;
	FString UnitBlockReason(int32 Type, int32 Town, int32 Size = 2) const;
	double UnitCost(int32 Type, int32 Size = 2) const;
	/** Raises a unit of a type at a garrison town, under a general command, with a training programme. */
	int32 RaiseUnit(int32 Type, int32 Town, int32 Command, ECampaign1851Program Program, FString* OutReason = nullptr, int32 Size = 2);
	TArray<FString> SaveResources() const;
	void RestoreResources(const TArray<FString>& Lines);

	// ---- Bridges (Campaign1851Bridges.cpp).

	const TArray<FCampaign1851Bridge>& GetBridges() const { return Bridges; }

	// ---- rivers, lakes, field boundaries (Campaign1851Hydro.cpp)
	const TArray<FCampaign1851River>& GetRivers() const { return Rivers; }
	const TArray<FCampaign1851Lake>& GetLakes() const { return Lakes; }
	static float RiverWidthM(int32 Class);
	static float RiverDrawnKm(int32 Class);
	bool IsLake(const FVector2D& Km, float MarginKm = 0.f, int32* OutLake = nullptr) const;
	/** Distance to the nearest river's middle within about a km (1e9 when none). */
	double RiverDistanceKm(const FVector2D& Km, int32* OutRiver = nullptr) const;
	/** On a lake or a river as drawn, give or take the margin. */
	bool IsFreshWater(const FVector2D& Km, float MarginKm = 0.f) const;
	EHedgeKind HedgeKindAt(const FVector2D& Km) const;

	// ---- the battlefield in 3D (Campaign1851BattleView.cpp)
	bool EnterBattleView();
	void LeaveBattleView();
	bool IsBattleView() const { return bBattleView; }
	FVector BattleViewOrigin() const;
	FVector2D BattleViewHalfExtent() const;
	float BattleViewGroundZ() const;
	int32 BridgeIndex(int32 Id) const;
	FString BridgeBlockReason(int32 Id, EBridgeAction Action) const;
	bool BridgeAction(int32 Id, EBridgeAction Action, FString* OutReason = nullptr);
	TArray<FString> SaveBridges() const;
	void RestoreBridges(const TArray<FString>& Lines);

	// ---- The battlefield generator (Campaign1851Battlefield.cpp).

	/** Builds the ground of a square of SizeKm around a point and writes Saved/Battle/Battlefield_<Name>.json and .png. */
	bool GenerateBattlefield(FVector2D CentreKm, float SizeKm, FString Name);
	const FCampaign1851Battlefield& GetBattlefield() const { return Battlefield; }
	UTexture2D* GetBattlefieldTexture() const { return BattlefieldTexture; }
	int32 GetBattlefieldVersion() const { return BattlefieldVersion; }
	FString BattlefieldFile() const;
	float BattlefieldSizeKm = 8.f;

	// ---- The ministers (Campaign1851Ministers.cpp).

	const FCampaign1851Minister& GetMinister(ECampaign1851Portfolio P) const { return Ministers[int32(P)]; }
	bool DismissMinister(int32 Portfolio);
	/** The others who could take the post (the pool of the portfolio, the present minister left out). */
	TArray<FCampaign1851Minister> MinisterCandidates(ECampaign1851Portfolio P) const;
	bool AppointMinister(int32 Portfolio, int32 Candidate);
	void SetAllDelegation(ECampaign1851Delegation Mode);
	float MinisterBudgetFactor(ECampaign1851Portfolio P) const;

	// ---- The choice of nation and the end (Campaign1851Endgame.cpp).

	/** The nation of the next new game ("DK" on the map, "SE" on the abstract model). */
	FString NewGameNation = TEXT("DK");
	void ApplyNewGameNation();
	/** The nation the player governs (Denmark unless another was chosen). */
	int32 GetPlayedNation() const;
	void AdjustNationWeight(int32 Portfolio, float Delta);
	TArray<FCampaign1851ScoreLine> FinalScore() const;
	static FString FinalGrade(float Total);
	/** True once when the campaign has ended (1 January 1867, or the peace after a war). */
	bool TakeEndPending();

	// ---- The historical works and sieges (Campaign1851Siege.cpp).

	void ResetFortProgrammes();
	double ProgrammeCost(int32 Index) const;
	bool BuildProgramme(int32 Index);
	bool HasFortsNear(int32 Town) const;

	// ---- Trade goods, loans and the record (Campaign1851Economy.cpp).

	float PriceIndex(ECampaign1851Good G) const;
	double ExportValuePerYear(ECampaign1851Good G) const;
	double ExportDutyPerYear() const;
	double GetDebt() const { return Debt; }
	float GetDebtRate() const { return DebtRate; }
	/** The rate a new loan would cost now. */
	float CreditRate() const;
	/** Three years of current tax, export duty and foreign income, in either scenario. */
	double LoanLimit() const;
	FString LoanBlockReason(double Amount) const;
	bool TakeLoan(double Amount, FString* OutReason = nullptr);
	bool RepayLoan(double Amount);
	const TArray<FCampaign1851Record>& GetHistory() const { return History; }
	TArray<FString> SaveEconomy() const;
	void RestoreEconomy(const TArray<FString>& Lines);

	// ---- Government and opinion (Campaign1851Politics.cpp).

	void ResetPolitics();
	float GetSupport(ECampaign1851Current C) const { return Support[int32(C)]; }
	float GetMood() const { return Mood; }
	int32 GetDanishWarLosses() const { return DanishWarLosses; }
	int32 GetEnemyWarLosses() const { return EnemyWarLosses; }
	const FString& GetPrimeMinister() const { return PrimeMinister; }
	ECampaign1851Current GetGovernment() const { return Government; }
	double GetGovernmentSince() const { return GovernmentSince; }
	float TaxMoodFactor() const;
	float CallInMoodFactor() const;
	float GovernmentTensionFactor() const;
	ECampaign1851Current LeadingCurrent() const;
	TArray<FString> SavePolitics() const;
	void RestorePolitics(const TArray<FString>& Lines);
	TArray<FString> SaveDiplomacy() const;
	void RestoreDiplomacy(const TArray<FString>& Lines);
	static constexpr double EnvoyCost = 5000.0;
	static constexpr double TreatyCost = 10000.0;
	static constexpr double AllianceCost = 20000.0;
	static constexpr double GuaranteeCost = 15000.0;

	/** New Year: the officers age, retire or die; the officer school's class joins (Campaign1851Career.cpp). */
	void YearlyOfficers();

	// ---- Sickness, the wounded and prisoners (Campaign1851Health.cpp).

	int32 SickTotal() const;
	int32 GetDanesCaptured() const { return DanesCaptured; }
	int32 GetEnemyCaptured() const { return EnemyCaptured; }
	/** The whole war: enemy men killed, wounded and taken (prisoners exchanged still count), and the booty. */
	int32 GetEnemyKilled() const { return EnemyKilled; }
	int32 GetEnemyWounded() const { return EnemyWounded; }
	int32 GetEnemyCapturedTotal() const { return EnemyCapturedTotal; }
	int32 GetCapturedRifles() const { return CapturedRifles; }
	int32 GetCapturedGuns() const { return CapturedGuns; }
	int32 GetCapturedHorses() const { return CapturedHorses; }
	int32 GetCapturedWagons() const { return CapturedWagons; }
	int32 GetCapturedColours() const { return CapturedColours; }
	void ExchangePrisoners();

	// ---- Weather (Campaign1851Weather.cpp): follows from the seed, day by day.

	float TemperatureOn(int32 Day) const;
	ECampaign1851Weather WeatherOn(int32 Day) const;
	ECampaign1851Weather GetWeather() const;
	float GetTemperature() const;
	/** A hard frost: the narrow waters (Slien, Alssund) bear. */
	bool IsIceWinter() const;
	FString GetSeasonAndWeather() const;
	/** How fast a leg is covered today (1 normal; mud, snow and thaw slow the roads, a storm the ferries). */
	float LegPace(const FCampaign1851Leg& Leg) const;

	// ---- The navy (Campaign1851Navy.cpp).

	void ResetNavy();
	const TArray<FCampaign1851Ship>& GetShips() const { return Ships; }
	float DanishSeaStrength() const;
	float EnemySeaStrength() const;
	bool HasSeaControl() const;
	double NavyUpkeepPerYear() const;
	FString ShipBlockReason(int32 Class) const;
	bool OrderShip(int32 Class, FString* OutReason = nullptr);
	bool IsBlockade() const { return bBlockade; }
	void SetBlockade(bool bOn);
	/** 0: the corps may cross, 1: it must gather boats first (a narrow sound), 2: the fleet bars the way. */
	int32 WaterCrossing(const FCampaign1851EnemyCorps& C, const TArray<FCampaign1851Leg>& Legs, FString& OutFerry) const;
	TArray<FString> SaveNavy() const;
	void RestoreNavy(const TArray<FString>& Lines);

	// ---- Research and doctrine (Campaign1851Research.cpp).

	void ResetResearch();
	bool HasResearch(const TCHAR* Id) const;
	/** A pontoon bridge's cost and days (the Pontonnerkorps cheapens and hastens them). */
	double PontoonCost() const;
	float PontoonDays() const;
	/** The battle's command zones (Stabsskolen, Generalstaben). */
	float CommandReachFactor() const;
	/** battleRules and aiDefaults for the 3D battle (Docs/BattleLink1851.md). */
	void WriteBattleRulesJson(const TSharedRef<FJsonObject>& Doc) const;
	/** An officer for the battle files (name, rank, experience, the ten stats); vacant when none. */
	TSharedRef<FJsonObject> OfficerJson(int32 Officer) const;
	/** A unit as the 3D battle builds it: companies, squadrons or the battery, with their officers. */
	TSharedRef<FJsonObject> BattleOrganisationJson(int32 Regiment) const;
	/** Why a topic cannot be started (empty: it can). */
	FString ResearchBlockReason(int32 Topic) const;
	/** The year a subject opens in the scenario (0: open from the start). */
	int32 ResearchOpenYear(int32 Topic) const;
	bool StartResearch(int32 Topic, FString* OutReason = nullptr);
	/** Research runs on two tracks, a military and a civil one (a project at a time in each). */
	int32 GetResearching(bool bCivil = false) const { return bCivil ? ResearchingCivil : Researching; }
	int32 GetResearchMonths(bool bCivil = false) const { return bCivil ? ResearchMonthsCivil : ResearchMonths; }
	int32 GetDoctrine(int32 Level) const { return Level >= 0 && Level < 3 ? Doctrine[Level] : 0; }
	bool SetDoctrine(int32 Level, int32 Choice, FString* OutReason = nullptr);
	bool IsDoctrineChanging() const { return CampaignDays < DoctrineSettledDay; }
	double GetDoctrineSettledDay() const { return DoctrineSettledDay; }
	/** Effects in the battles, forts, supply and mobilisation. */
	float DanishQualityFactor(const FCampaign1851Battle& B) const;
	float DanishLossFactor() const;
	float FortCoverBonus() const;
	float DanishGunFactor() const;
	float InfantryFactor() const;
	double ArmyEquipmentNumber(const TCHAR* Key, double Fallback) const;
	float ArmyPeacePresent(ECampaign1851Arm Arm) const;
	FString ArmyWeaponText(ECampaign1851Arm Arm) const;
	FString ArmyUniformText(ECampaign1851Arm Arm) const;
	Campaign1851Army::FBattleFactors ArmyBattleFactors(const FCampaign1851Regiment& R) const;
	float FoodCap() const;
	float CallInFactor() const;
	/** "doctrine" and "research" for Units.json and the battle request. */
	void WriteDoctrineJson(const TSharedRef<FJsonObject>& Doc) const;
	TArray<FString> SaveResearch() const;
	void RestoreResearch(const TArray<FString>& Lines);
	/** For tests: a topic done at once. */
	void GrantResearch(const FString& Id) { if (Campaign1851Research::FindTopic(Id) != INDEX_NONE) { Researched.Add(Id); } }
	static constexpr double DoctrineChangeDays = 60.0;
	static constexpr double DoctrineChangeCost = 5000.0;

	// ---- War and peace (Campaign1851War.cpp).

	float GetTension() const { return Tension; }
	bool IsAtWar() const { return bAtWar; }
	const TArray<FCampaign1851EnemyCorps>& GetEnemyCorps() const { return EnemyCorps; }
	const TArray<FPlannedEvent>& GetEventPlan() const { return EventPlan; }
	void ResetWar();
	/** For tests: the federal execution and the declaration of war now. */
	void ForceWar() { SpawnCorps(TEXT("Forbundskorpset (Sachsen, Hannover)"), TEXT("DE"), 0.09f, TEXT("Altona"), { TEXT("Rendsborg") }, 0.f); Tension = 80.f; DeclareWar(); }
	TArray<FString> SaveWar() const;
	void RestoreWar(const TArray<FString>& Lines);
	/** Battles at hand (the game pauses for the player's choice). */
	const TArray<FCampaign1851Battle>& GetBattles() const { return Battles; }
	/** Writes Saved/Battle/BattleRequest_N.json for the 3D battle game and waits for BattleResult_N.json. */
	bool FightBattleIn3D(int32 BattleId);
	void AutoResolveBattle(int32 BattleId);
	void RetreatFromBattle(int32 BattleId);
	/** The Danes' chance of winning (0-1) by strength, quality, cover and supply. */
	float BattleOdds(const FCampaign1851Battle& B) const;
	void BattleStrengths(const FCampaign1851Battle& B, float& OutDanish, float& OutEnemy) const;
	int32 CorpsIndexOf(const FCampaign1851Battle& B) const;
	/** Reads result files of battles sent to 3D. */
	void PollBattleResults();
	/** True if an amt's seat is held by the enemy (its taxes are lost). */
	bool IsAmtOccupied(const FCampaign1851Amt& A) const;
	/** Danish soldiers needed at an occupied town (within 3 km, two days, no enemy corps within 10 km) to free it. */
	static constexpr int32 LiberationMen = 300;

	// ---- Peace footing and mobilisation (Campaign1851Mobilisation.cpp).

	ECampaign1851Footing GetFooting() const { return Footing; }
	void SetFooting(ECampaign1851Footing F) { Footing = F; }
	/** Calls the men on leave in (pays the order); false with the reason. */
	bool Mobilise(FString* OutReason = nullptr);
	/** Back to peace footing: the men go home over a few days. */
	void Demobilise();
	/** Pay and keep a month of the men beyond the peace strength. */
	double MobilisedPayPerMonth() const;

	// ---- Materiel: rifles, guns and horses (Campaign1851Materiel.cpp).

	int32 GetRifles() const { return Rifles; }
	int32 GetHorseStock() const { return Horses; }
	void SetMateriel(int32 InRifles, int32 InHorses) { Rifles = FMath::Max(0, InRifles); Horses = FMath::Max(0, InHorses); }
	/** The stores as in 1851 (rifles, field guns, remounts). */
	void ResetMateriel();
	/** Rifles from the store, the rest bought abroad (booked); returns the import cost. */
	double TakeRifles(int32 Wanted, const FString& For);
	/** Horses from the store, the rest bought in the amter (booked). */
	int32 TakeHorses(int32 Wanted, const FString& For);

	// ---- Pulling down and salvage (Campaign1851Salvage.cpp).

	/** Pulls a building or a whole garrison down (private ones against compensation). */
	bool DemolishSite(ACampaign1851ConstructionSite* Site, FString* OutReason = nullptr);
	/** "materialer ca. 6.000 rd." (and the owner's compensation for private buildings). */
	FString DemolishText(const ACampaign1851ConstructionSite* Site) const;
	/** Slights a fort: its companies go home, its guns to the store, materials to the nearest town. */
	bool DemolishFort(int32 Id, FString* OutReason = nullptr);
	/** Fortress guns in the state's store (for any fort). */
	int32 GetGunStock() const { return GunStock; }

	/** A foot battery made a horse battery (every gunner mounted, keeps up with the cavalry): 6 guns instead of 8
	 *  (two back to the store), 180 men, 230 horses (the extra from the stock) and the money. */
	static constexpr double HorseBatteryCost = 12000.0;
	static constexpr int32 HorseBatteryHorses = 230;
	bool CanUpgradeToHorseBattery(int32 RegimentIndex, FString* OutWhy = nullptr) const;
	bool UpgradeToHorseBattery(int32 RegimentIndex, FString* OutWhy = nullptr);

	/** Split a unit in two: half its companies (with their captains and men) become a unit of their own where it
	 *  stands (a half battalion, under no chief until one is appointed). The new unit's index, or INDEX_NONE. */
	int32 SplitRegiment(int32 RegimentIndex, FString* OutWhy = nullptr, int32 Moved = 0);
	/** One company (or, for cavalry, one squadron) with its captain and men becomes a unit of its own where the unit stands. */
	int32 SplitOffCompany(int32 RegimentIndex, int32 Company, FString* OutWhy = nullptr);
	/** Companies of a battalion, or squadrons of cavalry (140 men each); 0 for the rest. */
	int32 SubUnitCount(int32 RegimentIndex) const;
	/** Exact artillery-section share: Resource 0 = guns, 1 = horses, 2 = horse establishment. */
	int32 SectionResource(int32 Regiment, int32 Section, int32 Resource) const;
	bool TransferSectionGuns(int32 FromReg, int32 From, int32 ToReg, int32 To, int32 Count, FString* OutWhy = nullptr);
	/** Taking part in a battle that is waiting or being fought: it cannot be split, joined or have its companies moved. */
	bool IsInBattle(int32 RegimentIndex) const;
	/** A free id for a half of this unit (the old id with a letter, then with two). */
	FString FreeSplitId(const FString& OldId) const;
	/** The id of the unit the halves came from (a letter suffix stripped from a split-off half). */
	static FString SplitBase(const FCampaign1851Regiment& R);
	int32 SubUnitMen(int32 RegimentIndex, int32 Index) const;
	/** Two halves of one unit (one split off the other, or both off the same). */
	bool IsSplitPair(int32 A, int32 B) const;
	/** A half of this unit standing with it (to join again), INDEX_NONE if none. */
	int32 MergePartner(int32 RegimentIndex) const;
	bool CanMerge(int32 Keep, int32 Absorb, FString* OutWhy = nullptr) const;
	/** Joins Absorb into Keep (companies, men, horses, guns; the skills by men); Absorb is removed. The new index
	 *  of Keep, or INDEX_NONE. */
	int32 MergeRegiments(int32 Keep, int32 Absorb, FString* OutWhy = nullptr);
	/** A company (with its captain and men) goes over to another unit of the same kind standing with it. */
	bool MoveCompany(int32 From, int32 Company, int32 To, FString* OutWhy = nullptr, int32* OutTo = nullptr);
	/** Takes a unit out of the army; every index to the units after it moves down by one. */
	void RemoveRegimentAt(int32 Index);

	/** Attack a seen enemy corps within EngageKm of the given units: the battle is offered at once (these units
	 *  fight, wherever they stand in that reach). The nearest such corps and its distance, or INDEX_NONE. */
	static constexpr double EngageKm = 15.0;
	int32 EngageableCorps(const TArray<int32>& Units, double* OutKm = nullptr) const;
	bool EngageCorps(int32 CorpsIndex, const TArray<int32>& Units, FString* OutWhy = nullptr);
	void SetGunStock(int32 N) { GunStock = FMath::Max(0, N); }
	/** Materials stored in a town (rigsdaler). */
	double GetMaterialsIn(int32 CityIndex) const { return MaterialsIn(CityIndex); }
	/** Materials stored within reach (km) of a point, in rigsdaler. */
	double MaterialsNear(const FVector2D& Km) const;
	const TArray<FVector>& GetMaterialLots() const { return MaterialLots; }
	void SetMaterialLots(const TArray<FVector>& In) { MaterialLots = In; }
	static constexpr double MaterialReachKm = 30.0;

	// ---- Field fortifications, skanser (Campaign1851Forts.cpp).

	const TArray<FCampaign1851Fort>& GetForts() const { return Forts; }
	int32 FortIndex(int32 Id) const;
	/** The nearest monarchy town as the crow flies. */
	int32 NearestTown(const FVector2D& Km) const;
	/** Why no fort can be raised there (empty if it can): not on the monarchy's land, in a town, too close to another, no money. */
	FString FortBlockReason(const FVector2D& Km, bool bLarge) const;
	/** Starts a fort (pays the down payment); returns its id, or INDEX_NONE with the reason. */
	int32 StartFort(const FVector2D& Km, bool bLarge, float Yaw, FString* OutReason = nullptr);
	/** Two more guns, or the next level of defence (ECampaign1851FortWork::Guns / Defence). */
	bool UpgradeFort(int32 Id, ECampaign1851FortWork Work, FString* OutReason = nullptr);
	/** Turns a fort's front (degrees). */
	void TurnFort(int32 Id, float DeltaYaw);
	/** Sends a company of a battalion near the fort into it (its men leave the battalion). */
	bool AddFortCompany(int32 FortId, int32 Regiment, int32 Company, FString* OutReason = nullptr);
	/** Takes a company in a fort back to its battalion (which must be near). */
	bool ReturnFortCompany(int32 FortId, int32 Entry, FString* OutReason = nullptr);
	/** Companies of battalions standing near a fort that could hold it (regiment * 10 + company). */
	TArray<int32> FortCandidates(int32 FortId) const;
	/** Men in the fort (up to its room) and in the reserve behind it. */
	void FortMen(const FCampaign1851Fort& F, int32& OutInside, int32& OutReserve) const;
	/** Other forts joined to this one by trenches (ids). */
	TArray<int32> TrenchLinks(int32 FortId) const;
	/** For tests: every fort finished, fully armed and strengthened. */
	void CompleteForts();
	FVector FortWorld(int32 Index) const;
	double FortUpkeepPerYear() const;
	void ResetForts();
	TArray<FCampaign1851FortSave> SaveForts() const;
	void RestoreForts(const TArray<FCampaign1851FortSave>& Saves);
	/** Writes Saved/Battle/Fortifications.json for the 3D battles. */
	void ExportForts() const;

	// ---- Nations, growth and the national AI (Campaign1851Nations.cpp).

	const TArray<FCampaign1851Nation>& GetNations() const { return Nations; }
	int32 GetPlayerNation() const { return PlayerNation; }
	/** How the player runs a portfolio of his own nation (MANUEL, RÅDGIVER, AUTO). */
	void SetDelegation(ECampaign1851Portfolio P, ECampaign1851Delegation Mode) { if (Nations.IsValidIndex(PlayerNation)) { Nations[PlayerNation].Modes[int32(P)] = Mode; } }
	/** Guardrail: cash the ministries must leave in the treasury. */
	void SetReserve(double Rd) { if (Nations.IsValidIndex(PlayerNation)) { Nations[PlayerNation].Reserve = FMath::Max(0.0, Rd); } }
	const TArray<FCampaign1851Decision>& GetDecisions() const { return Decisions; }
	/** Carries out a ministry's recommendation; false if it can no longer be done. */
	bool ExecuteDecision(int32 Index);
	int32 GetSeed() const { return Seed; }
	float GetDeviation() const { return Deviation; }
	/** The historical deviation for the next new game (0 .. 0.5). */
	float NewGameDeviation = 0.2f;
	/**
	 * A new world from a seed: towns and amter as in 1851, each amt's growth, the nations' priorities and the
	 * opening of the railways under construction varied by up to the deviation. Same seed, same world.
	 */
	void ResetWorld(int32 InSeed, float InDeviation);
	/** Growth a year (%) of a town and of an amt's countryside, with stations, chausséer and civil buildings. */
	float UrbanGrowthRate(int32 CityIndex) const;
	float RuralGrowthRate(int32 AmtIndex) const;
	/** Yearly income from finished civil buildings (trade tax, customs, postage). */
	double CivilIncomePerYear() const;
	/** The civil buildings' yield to the state a year: fees and excise, the tax of their workers, customs and export duties. */
	double CivilFeesPerYear() const;
	double CivilJobTaxPerYear() const;
	double CivilTradePerYear() const;
	int32 CivilJobs() const;
	FCampaign1851NationFigures NationFigures(int32 NationIndex) const;
	void SaveWorld(UCampaign1851SaveGame* Save) const;
	/** The civil buildings the towns already have in 1851 (town halls, schools, merchants, breweries, the industry of the big towns). */
	void SeedHistoricBuildings();
	void RestoreWorld(const UCampaign1851SaveGame* Save);

	// ---- Roads and railways (design manual 20.16.1; Campaign1851Network.cpp).

	const TArray<FCampaign1851Link>& GetLinks() const { return Links; }
	const TArray<FCampaign1851Railway>& GetRailways() const { return Railways; }
	/** Links from a town, nearest neighbour first. */
	TArray<int32> LinksOf(int32 CityIndex) const;
	/** Why a link cannot get this work ("færgeoverfart", "allerede chaussé", ...); empty if it can (money aside). */
	FString LinkBlockReason(int32 Link, ECampaign1851LinkWork Work) const;
	int32 LinkWorkCost(int32 Link, ECampaign1851LinkWork Work) const;
	float LinkWorkDays(int32 Link, ECampaign1851LinkWork Work) const;
	/** Starts paving or a railway on a link and pays the down payment (bCharge); false with OutReason if it cannot. */
	bool StartLinkWork(int32 Link, ECampaign1851LinkWork Work, bool bCharge = true, FString* OutReason = nullptr);
	/** True if an open railway stops in the town. */
	bool HasStation(int32 CityIndex) const;
	/** Days from one end of a link to the other by the best way open now (march, or train). */
	float LinkTravelDays(int32 Link) const;
	/** "68 km landevej · 3½ dagsmarch", "jernbane 70 km · 1 dag med tog", ... */
	FString LinkTravelText(int32 Link) const;
	/** Other town of a link. */
	int32 LinkOther(int32 Link, int32 CityIndex) const { return Links.IsValidIndex(Link) ? (Links[Link].A == CityIndex ? Links[Link].B : Links[Link].A) : INDEX_NONE; }
	/** Back to the network of 1851 (new game, or before loading). */
	void ResetNetwork();
	/** Built roads and railways and the projects under way, for saving; restored after ResetNetwork. */
	TArray<FCampaign1851LinkSave> SaveNetwork() const;
	int32 RestoreNetwork(const TArray<FCampaign1851LinkSave>& Saves);
	/** Messages for the player ("Jernbanen ... er åbnet"); the controller shows them. */
	/** The news since the last call (also kept in the newspaper's log). */
	TArray<FString> TakeNews();
	const TArray<TPair<double, FString>>& GetNewsLog() const { return NewsLog; }
	/** World position of a projected-km point on the terrain. */
	FVector WorldAtKm(const FVector2D& Km) const;
	/** Point at a distance along a polyline (km), with the heading there. */
	static FVector2D AlongLine(const TArray<FVector2D>& Line, double Distance, FVector2D* OutDirection = nullptr);
	static double LineLength(const TArray<FVector2D>& Line);

	// ---- Regiments (design manual 7; Campaign1851Army.cpp).

	const TArray<FCampaign1851Regiment>& GetRegiments() const { return Regiments; }
	int32 FindRegiment(const FString& Id) const { return Regiments.IndexOfByPredicate([&Id](const FCampaign1851Regiment& R) { return R.Id == Id; }); }
	/** Regiments standing in a town. */
	TArray<int32> RegimentsIn(int32 CityIndex) const;
	/** Fastest times (days) from a town to every town, and the leg that reaches each (Dijkstra over the links). */
	void TravelTimes(int32 From, float Pace, bool bRail, TArray<float>& OutDays, TArray<FCampaign1851Leg>& OutVia) const;
	/** Fastest way between two towns now for a column of this road pace (km/day), by road, chaussée, ferry and (bRail) railway. */
	bool FindRoute(int32 From, int32 To, float Pace, TArray<FCampaign1851Leg>& OutLegs, bool bRail = true) const;
	/**
	 * A march from a town or a point to a town or a point: straight across country (Direct), or across the
	 * fields to the nearest town and then by road (and rail) to the town nearest the goal.
	 */
	bool PlanMarch(int32 FromTown, const FVector2D& FromKm, int32 ToTown, const FVector2D& ToKm, float Pace, ECampaign1851RouteMode Mode,
		TArray<FCampaign1851Leg>& OutLegs, FString* OutReason = nullptr) const;
	/** Sends regiments as one column to a town (CityIndex) or a point in the field (TargetKm, CityIndex = INDEX_NONE). */
	bool OrderMarchTo(const TArray<int32>& Column, int32 CityIndex, const FVector2D& TargetKm, ECampaign1851RouteMode Mode, FString* OutReason = nullptr);
	FVector2D TownKm(int32 CityIndex) const;
	/** Projected km of a world position (the inverse of WorldAtKm, ignoring height). */
	FVector2D KmAtWorld(const FVector& World) const;
	/** True if a straight march between two points stays on the monarchy's land. */
	bool IsDryLine(const FVector2D& A, const FVector2D& B) const;
	/** The nearest monarchy town reachable across the fields from a point (INDEX_NONE if none within 60 km). */
	int32 NearestTownFrom(const FVector2D& Km) const;
	/** "Aalborg", or "terrænet 4 km fra Aalborg". */
	FString DescribePlace(int32 CityIndex, const FVector2D& Km) const;
	/**
	 * Sends regiments to a town as one column, at the pace of its slowest arm (Campaign1851Army::ColumnPace);
	 * any on the march finish their current stretch first.
	 */
	bool OrderMarch(const TArray<int32>& Column, int32 CityIndex, FString* OutReason = nullptr);
	/** Sets a regiment's training programme in garrison. */
	void SetProgram(int32 Regiment, ECampaign1851Program Program) { if (Regiments.IsValidIndex(Regiment)) { Regiments[Regiment].Program = Program; } }
	/** Halts a regiment at the end of the stretch it is on. */
	void HaltRegiment(int32 Regiment);
	/** The ground a leg covers, from its From town to its To town (projected km). */
	TArray<FVector2D> LegLine(const FCampaign1851Leg& Leg) const;
	/** Where a regiment is on the map now. */
	FVector RegimentWorld(int32 Regiment) const;
	/** Where a unit is drawn: its place, moved aside when others stand there too. */
	FVector2D ShownKm(int32 Regiment) const;
	// ---- Officers (design manual 8; Campaign1851Officers.cpp).

	const TArray<FCampaign1851Officer>& GetOfficers() const { return Officers; }
	/** Unassigned officers (bGenerals: generals, else regimental officers). */
	TArray<int32> OfficerPool(bool bGenerals) const;
	/** Makes an officer the chief of a regiment, or (a general) attaches him to it; whoever held the post goes to the pool. */
	bool AssignOfficer(int32 Officer, int32 Regiment);
	/** A headquarters staff post (1 deputy, 2 chief of staff; 0 = the chief himself). */
	bool AssignFormationStaff(int32 Officer, int32 Formation, int32 Post);
	/** The officer leaves the staff post he holds, if any. */
	void LeaveStaffPost(int32 Officer);
	/** Who leads the formation: its chief, or (acting) its deputy when the chief is missing. */
	int32 ActingCommander(int32 Formation, bool* bOutActing = nullptr) const;
	/** The best staff stat over a column: its general or a chief of staff above it (0 = none). */
	int32 ColumnStaff(const TArray<int32>& Column) const;
	/** The battalion's senior captain (most experienced): second in command after the major. */
	int32 SeniorCaptain(int32 Regiment) const;
	/** "Divisionschef, 1. Division", "Kompagnichef, 3. Kompagni (6. Bataillon)", "ledig". */
	FString OfficerRole(int32 Officer) const;
	/** A 3D battle's officers: those of ours who were wounded or taken (out of their posts until they recover or are exchanged),
	 *  and the enemy officers we took. */
	void ApplyOfficerCasualties(const TArray<TSharedPtr<FJsonValue>>& Ours, const TArray<TSharedPtr<FJsonValue>>& Theirs);
	void DailyOfficers();
	/** What the enemy asks to let a captured officer go (by his rank), and paying it (he returns at once). */
	int32 RansomCost(int32 Officer) const;
	bool RansomOfficer(int32 Officer, FString* OutWhy = nullptr);
	int32 EnemyOfficersHeld = 0;
	/** A company's number, counted through its regiment (the 2nd battalion has companies 5-8). */
	int32 CompanyNumber(int32 Regiment, int32 Company) const;
	/** A company's men (the battalion's strength spread over its companies). */
	int32 CompanyMen(int32 Regiment, int32 Company) const;
	/** The companies' strengths fixed as they are now (before something changes the battalion's men or companies). */
	void FreezeCompanyStrength(int32 Regiment);
	/** Two companies of the same battalion share their men evenly (the stronger gives to the weaker). */
	/** Count men from one company to another of the same battalion (at most what the first has and the second has room for). */
	bool TransferCompanyMen(int32 FromRegiment, int32 From, int32 ToRegiment, int32 To, int32 Count, FString* OutWhy = nullptr);
	/** The room a company has for men (the battalion's establishment per company). */
	int32 CompanyCapacity(int32 Regiment) const;
	bool BalanceCompanies(int32 Regiment, int32 A, int32 B, FString* OutWhy = nullptr);
	/** All the battalion's companies in the field get the same number of men. */
	bool EqualizeCompanies(int32 Regiment, FString* OutWhy = nullptr);
	/** Hires a new officer or general into the pool (pays the cost); INDEX_NONE if the treasury cannot. */
	int32 RecruitOfficer(bool bGeneral, const TCHAR* RecruitmentRank = nullptr);
	/** The general of a stack or column: the first general attached to one of its regiments. */
	const FCampaign1851Officer* ColumnGeneral(const TArray<int32>& Column) const;
	int32 OfficerCost(bool bGeneral) const { return bGeneral ? GeneralRecruitCost : OfficerRecruitCost; }
	/** Stops a regiment where it is now (in the field if it was between towns). */
	void StopRegiment(int32 Regiment);
	/** Cancels a march: the regiment goes back to where the order found it. */
	void CancelOrder(int32 Regiment);
	const TArray<FCampaign1851Command>& GetCommands() const { return Commands; }

	// ---- The field army: formations the player puts together (the tree in KAMPORDEN, drag and drop).
	const TArray<FCampaign1851Formation>& GetFormations() const { return Formations; }
	int32 FormationIndex(int32 Id) const;
	/** A new, numbered formation under Parent (0 = the field army itself); returns its id. */
	int32 CreateFormation(ECampaign1851Echelon Echelon, int32 Parent);
	bool CanInsertFormationHQ(int32 Parent, ECampaign1851Echelon Echelon) const;
	int32 InsertFormationHQ(int32 Parent, ECampaign1851Echelon Echelon);
	bool CanReturnToGarrison(int32 Regiment, FString* OutReason = nullptr) const;
	const TCHAR* FormationPostRank(int32 Formation, int32 Post) const;
	bool CanAssignFormationPost(int32 Officer, int32 Formation, int32 Post) const;
	/** Dissolves a formation: its units and sub-formations go up a level. */
	void DissolveFormation(int32 Id);
	/** A formation and all under it dissolved, its units back in garrison; returns the number of units. */
	int32 ReturnFormationToGarrison(int32 Id);
	/** True if formation Id is Ancestor or lies under it. */
	bool IsInside(int32 Id, int32 Ancestor) const;
	/** Puts a formation under another (0 = the field army); refused if it would go under itself. */
	bool MoveFormation(int32 Id, int32 NewParent);
	/** Puts a regiment in a formation (0 = back to its garrison). */
	bool MoveRegimentToFormation(int32 Regiment, int32 Formation);
	/**
	 * A general command's garrison units into the field army at once: under the army a division (the foot in
	 * brigades of four, cavalry and batteries under the division, the command's general at its head), under a
	 * division a brigade, deeper all into the formation itself. Returns the new formation's id (0 if nothing).
	 */
	int32 FormFromCommand(int32 Command, int32 Parent);
	/** Every regiment in a formation and its sub-formations. */
	TArray<int32> FormationRegiments(int32 Id) const;
	/** Makes an officer the commander of a formation (he leaves any other post). */
	bool AssignFormationCommander(int32 Officer, int32 Formation);
	/** Releases a formation's commander to the officer pool, keeping the HQ and its units. */
	bool RemoveFormationCommander(int32 Formation);
	/** For testing: a field army of two divisions and a reserve (brigades, commanders) from the garrisons. */
	void BuildTestFieldArmy();
	TArray<FCampaign1851FormationSave> SaveFormations() const;
	void RestoreFormations(const TArray<FCampaign1851FormationSave>& Saves);
	/** Makes a general the commanding general of a general command (the previous one goes to the pool). */
	bool AssignCommandGeneral(int32 Officer, int32 Command);
	/** Why an officer cannot be promoted now (empty if he can): top rank, too little experience. */
	FString PromotionBlock(int32 Officer) const;
	/** Promotes an officer one rank; a colonel made generalmajor leaves his regiment for the pool of generals. */
	bool PromoteOfficer(int32 Officer);

	// ---- Troop trains (rolling stock, backlog B-380): the state's trains, those under way, those on order.
	static constexpr int32 TroopTrainCost = 30000;
	static constexpr float TroopTrainDeliveryDays = 120.f;
	int32 GetTroopTrains() const { return TroopTrainList.Num(); }
	const TArray<FCampaign1851TroopTrain>& GetTroopTrainList() const { return TroopTrainList; }
	/** Trains not serving a column now. */
	int32 FreeTroopTrains() const;
	/** "holder ledigt i Roskilde", "kører tomt til ...", "kører 9. Bataillon til ...". */
	FString DescribeTrain(int32 Train) const;
	/** Works out a column's march (route, trains, waiting, time) without ordering it. */
	FCampaign1851MarchPlan PlanColumn(const TArray<int32>& Column, int32 CityIndex, const FVector2D& TargetKm, ECampaign1851RouteMode Mode) const;
	/** "4 d. 6 t.", "9 t." */
	static FString FormatDuration(float Days);
	/** Ordered trains: how many, and the day they arrive (campaign days). */
	const TArray<FVector2D>& GetTrainOrders() const { return TrainOrders; }
	/** Saved troop trains; restored after the regiments. */
	TArray<FCampaign1851TrainSave> SaveTrains() const;
	void RestoreTrains(const TArray<FCampaign1851TrainSave>& Saves, const TArray<FVector2D>& Orders);
	/** The trains of 1851 (new game, or before loading). */
	void ResetTroopTrains();
	/** Orders a troop train (locomotive and carriages) from abroad; false if the treasury cannot pay. */
	/** A train set from England, delivered to the station given (a rail network's depot; none: Copenhagen). */
	bool OrderTroopTrain(int32 Station = INDEX_NONE);
	/** The railways that hang together, each with its stations and the depot where new trains are landed. */
	struct FRailNet { FString Name; TArray<int32> Stations; int32 Depot = INDEX_NONE; };
	TArray<FRailNet> RailNets() const;
	/** Move a free train to another network's depot (by ship or cart): cost and days. */
	static constexpr double TrainTransferCost = 6000.0;
	float TrainTransferDays(int32 Train, int32 Station) const;
	bool TransferTrain(int32 Train, int32 Station, FString* OutReason = nullptr);
	/** A note from the last march order for the player (e.g. not enough trains, so the column marches). */
	FString TakeOrderNote() { FString Note = OrderNote; OrderNote.Reset(); return Note; }

	/** Sends an officer in the pool home (no more pay); false if he holds a post. */
	bool DismissOfficer(int32 Officer);
	/** Officers' pay per month (all officers, assigned or in the pool). */
	double OfficerPayPerMonth() const;
	TArray<FCampaign1851OfficerSave> SaveOfficers() const;
	void RestoreOfficers(const TArray<FCampaign1851OfficerSave>& Saves);

	/** The army of 1851 in its garrisons (new game, or before loading). */
	void ResetArmy();
	TArray<FCampaign1851RegimentSave> SaveArmy() const;
	int32 RestoreArmy(const TArray<FCampaign1851RegimentSave>& Saves);

	/** Screen-size scaling for the city markers; called by the player controller each frame. */
	void UpdateMarkers(float CameraDistanceKm);
	/** The camera's distance (km) at the last marker update. */
	float GetCameraDistanceKm() const { return LastCameraDistanceKm; }
	/** Closer than this (km) the regiments are picked by their miniatures, not by NATO counters. */
	static constexpr float MiniatureViewKm = 15.f;

	/** Population size class used by markers and the legend. */
	static float SizeClass(int32 Population);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Terrain;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> CityMarkers;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> ForeignMarkers;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Backdrop;

	/** The coarse painted sheet of the world under the detailed map (one plane, the same projection). */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> WorldSheet;

	/** Main roads between the towns (routed by build_map.py). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Roads;

	/** Ferry crossings (dashed over the water), shown with the main roads. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Ferries;

	/** Village lanes, generated with the scenery. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Lanes;

	/** Rivers by class and the lakes (Campaign1851Hydro.cpp). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> RiverMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACampaign1851ConstructionSite>> Projects;

	/** One instanced component per Campaign1851Scenery::EPiece, created at BeginPlay. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Scenery;

private:
	bool LoadData();
	bool LoadHeight();
	float SampleHeight01(const FVector2D& Uv) const;
	void BuildTerrain();
	void BuildMarkers();
	bool LoadFeatures();
	bool LoadHydro();
	void BuildHydroMeshes();
	/** The monarchy's land border as a red band (Campaign1851Hydro.cpp). */
	void BuildBorderMeshes();
	void UpdateHydroVisibility(float CameraDistanceKm);
	void BuildBattleView();
	bool bBattleView = false;
	int32 BattleViewVersion = -1;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> BattleViewParts;
	TArray<FCampaign1851River> Rivers;
	TArray<FCampaign1851Lake> Lakes;
	TMap<FIntPoint, TArray<FIntPoint>> RiverCells;   // 1 km cell -> (river, segment)
	void BuildScenery();
	/** Drapes polylines (projected km) on the terrain as a ribbon mesh. */
	UStaticMeshComponent* BuildRibbons(const TArray<TArray<FVector2D>>& Lines, float WidthKm, const FLinearColor& Colour, const TCHAR* Name, UMaterialInterface* Material);

	/** Map-local position of a projected-km point, on the terrain mesh surface. */
	FVector LocalAtKm(const FVector2D& Km) const;
	FVector2D UvFromKm(const FVector2D& Km) const;
	/** Terrain height (units) under a world position. */
	float TerrainWorldZ(const FVector& World) const;
	/** Terrain height (units) exactly as the triangulated terrain mesh has it. */
	float TerrainZ(const FVector2D& Uv) const;
	bool IsMonarchyLand(const FVector2D& Km) const;
	/** Woodland density 0..1. */
	float Woodland(const FVector2D& Km) const;

	FCampaign1851Extent Extent;
	FCampaign1851Extent Bornholm;
	/** The coarse world sheet under the detailed map (same projection): the backdrop plane covers it. */
	FCampaign1851Extent WorldExtent;
	bool bHasWorld = false;
	FVector2D SizeKm = FVector2D(1.0, 1.0);
	float DetailTileKm = 2.5f;
	TArray<FCampaign1851City> Cities;
	TArray<FCampaign1851Label> Labels;
	TArray<FCampaign1851Amt> Amter;
	TArray<uint8> AmtIds;   // Denmark1851_Amter.png: amt id per pixel, row 0 = north
	int32 AmtIdsW = 0, AmtIdsH = 0;
	bool LoadAmtIds();
	TArray<TArray<FVector2D>> RoadLines;   // projected km
	TArray<TArray<FVector2D>> FerryLines;  // projected km, landing to landing
	TArray<FVector2D> RoadSampleKm;        // main roads every 100 m, to keep plots off them
	mutable TMap<int32, bool> CoastalTowns;

	/** Spawns a site at a plot; GateLocal is where the supply wagon stops (piece units). */
	ACampaign1851ConstructionSite* SpawnSite(int32 CityIndex, const FVector2D& Km, float Yaw, const TArray<struct FCampaign1851SiteModule>& Modules, bool bGarrison, const FVector2D& GateLocal);
	bool FindBuildingPlot(int32 CityIndex, const struct FCampaign1851SiteModule& Def, FVector2D& OutKm, float& OutYaw) const;
	bool PlotFits(const FVector2D& Km, float RadiusKm) const;
	bool IsSea(const FVector2D& Km) const;
	/** Removes scenery instances (houses, farms, trees) on a plot. */
	void ClearScenery(const FVector2D& Km, float RadiusKm);

	TArray<uint16> Height;
	int32 HeightW = 0, HeightH = 0;
	TArray<float> GridZ;               // terrain mesh vertex heights (units)
	TArray<FColor> Features;           // R = monarchy land, G = woodland
	int32 FeaturesW = 0, FeaturesH = 0;
	bool bSceneryVisible = false;
	bool bRoadsVisible = false;

	// ---- Officers (Campaign1851Officers.cpp)
	TArray<FCampaign1851Command> Commands;
	TArray<FCampaign1851Command> CommandsAtStart;
	TArray<FCampaign1851Formation> Formations;
	int32 NextFormationId = 1;
	TArray<FString> CommandGeneralIds;
	TArray<FCampaign1851TroopTrain> TroopTrainList;
	int32 NextTrainId = 1;
	TArray<FVector2D> TrainOrders;     // X count, Y campaign day of delivery
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> TroopTrainPieces;   // four per train
	/** Days by rail alone from a town to every town (and the leg reaching each). */
	void RailTimes(int32 From, TArray<float>& OutDays, TArray<FCampaign1851Leg>& OutVia) const;
	void AdvanceTroopTrains(float DeltaDays);
	void UpdateTroopTrainPieces();
	FString OrderNote;
	bool LoadOfficers();
	/** The officer corps of 1851: the generals, and a chief for every regiment plus a few in reserve (fixed seed). */
	void ResetOfficers();
	FCampaign1851Officer MakeOfficer(FRandomStream& Rng, bool bGeneral, const FString& Rank) const;
	TArray<FCampaign1851Officer> Officers;
	TArray<FCampaign1851Officer> GeneralsAtStart;
	TSharedPtr<FJsonObject> OfficerScenarioData;
	TMap<FString, FString> OfficerStartCommands;
	TArray<FString> FirstNames, Surnames;
	int32 OfficerPay = 600, GeneralPay = 3000, OfficerRecruitCost = 1500, GeneralRecruitCost = 6000;
	int32 NextOfficerNumber = 1;

	// ---- Army (Campaign1851Army.cpp)
	bool LoadArmy();
	void AdvanceArmy(float DeltaDays, float DeltaSeconds);
	/** Position of a regiment standing in a town: around the edge of it, each in its own place. */
	void PlaceInTown(int32 Regiment);
	void UpdateRegimentPiece(int32 Regiment);
	TArray<FCampaign1851Regiment> Regiments;
	TArray<FCampaign1851Regiment> ArmyAtStart;
	/** Immutable scenario equipment; mutable stocks are saved by the existing save records. */
	TSharedPtr<FJsonObject> ArmyScenarioEquipment;
	TMap<FString, FCampaign1851DepotCapacity> ArmyScenarioMagazines;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RegimentPieces;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMesh>> ArmyMeshes;   // per ECampaign1851Arm

	// ---- Network (Campaign1851Network.cpp)
	bool LoadNetwork(const FJsonObject& Json);
	/** Daily work on the link projects, historical lines opening, trains and work gangs moving. */
	void AdvanceNetwork(float DeltaDays, float DeltaSeconds);
	/** Railways, chausséer and works as ribbons; rebuilt when they change. */
	void RebuildNetworkMeshes();
	void UpdateNetworkVisibility();
	/** Marks the links an open railway serves, and reports lines opening (bAnnounce). */
	void UpdateOpenRailways(bool bAnnounce);
	void FinishLinkWork(int32 Link);
	/** Removes scenery instances within RadiusKm of a line (one pass over the instances). */
	void ClearSceneryAlong(const TArray<FVector2D>& Line, float RadiusKm);
	UStaticMeshComponent* MakeRibbon(const TArray<TArray<FVector2D>>& Lines, float WidthKm, const FLinearColor& Colour, const TCHAR* Name, float Lift);
	double NetworkUpkeepPerYear() const;
	TArray<FCampaign1851Link> Links;
	TArray<FCampaign1851Railway> Railways;
	int32 HistoricRailways = 0;   // Railways[0..HistoricRailways) come from the map data
	TArray<FString> News;

	// The supply layer (Campaign1851Supply.cpp).
	void AdvanceSupply(float DeltaDays);
	void MonthlySupply();
	int32 TownForPurchase(const FVector2D& Km) const;
	double AmmoLoadPrice() const;
	TMap<int32, FCampaign1851DepotStock> Depots;
	bool bHungerNews = false;
	void AdvanceSupplyColumns(float DeltaDays);
	void UpdateSupplyColumnPieces();
	bool PlanColumnRoute(FCampaign1851SupplyColumn& C, const FVector2D& To, int32 ToTown);
	FVector2D SupplyTargetKm(const FCampaign1851SupplyColumn& C) const;
	TArray<FCampaign1851SupplyColumn> SupplyColumns;
	int32 SupplyColumnCount = Campaign1851Supply::ColumnsAtStart;
	int32 NextSupplyColumnId = 1;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> SupplyColumnPieces;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> ColumnMesh;

	// The war layer.
	void DailyWar();
	void MonthlyWar();
	void AdvanceWar(float DeltaDays);
	void DeclareWar();
	// Reconnaissance and the enemy's decisions (Campaign1851Intel.cpp).
	void UpdateIntel();
	float EnemyEstimateOfDefence(int32 Town) const;
	bool ChooseCorpsObjective(int32 CorpsIndex);
	void EnemyReinforcements();
	void SpawnCorps(const FString& Name, const FString& NationId, float ShareOfArmy, const FString& From, const TArray<FString>& Objectives, float Delay);
	void CreateBattle(int32 CorpsIndex);
	void ApplyBattle(int32 BattleIndex, const FCampaign1851BattleOutcome& O);
	TArray<FCampaign1851Battle> Battles;
	int32 NextBattleId = 1;
	double LastBattlePoll = 0.0;
	float Tension = 25.f;
	bool bAtWar = false;
	double WarStartDay = 0.0;
	// Foreign affairs.
	void MonthlyDiplomacy();
	bool bSoundDuesAbolished = false;
	int32 RedemptionYearsLeft = 0;
	double PeaceTalksDay = -1.0;
	// Raw materials.
	float MonthlyRawMaterials();
	float RawStock[int32(ECampaign1851Raw::Count)] = {};
	bool bRawShortNoted = false;
	int32 MortarStock = 12;
	int32 WagonStock = 150;
	// Bridges.
	void DetectBridges();
	void ApplyBridge(const FCampaign1851Bridge& B);
	void DailyBridges();
	TArray<FCampaign1851Bridge> Bridges;
	TArray<float> LinkFerryKm0;
	/** The bridges as meshes on the map: a deck over the water with its railings (a blown one in two stumps). */
	void RebuildBridgeMeshes();
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> BridgeMeshes;
	// The battlefield.
	void RenderBattlefield();
	void WriteBattlefield() const;
	FCampaign1851Battlefield Battlefield;
	TArray<TArray<FVector2D>> LaneLinesKm;   // the country lanes (projected km)
	TArray<FVector> CountrySites;            // villages (0), farms (1), cottages (2) of the map's countryside
	float LastWarScore = 0.f;                // at the last peace
	UPROPERTY(Transient) TObjectPtr<UTexture2D> BattlefieldTexture;
	int32 BattlefieldVersion = 0;
	// Ministers.
	FCampaign1851Minister MakeMinister(ECampaign1851Portfolio P, ECampaign1851Current Line, const FString& Avoid) const;
	bool LoadScenarioMinisters();
	TArray<FCampaign1851Minister> ScenarioMinisterPools[int32(ECampaign1851Portfolio::Count)];
	void AppointCabinet(ECampaign1851Current Line);
	TArray<FCampaign1851Decision> MinisterOptions(ECampaign1851Portfolio P, double Budget) const;
	bool CarryOutMinister(const FCampaign1851Decision& D);
	TArray<FString> SaveMinisters() const;
public:
	double GetMinistryBudget(ECampaign1851Portfolio P) const { return MinistryBudget[int32(P)]; }
	double GetMinistryPot(ECampaign1851Portfolio P) const { return MinistryPot[int32(P)]; }
	/** One step up or down the ladder 0, 1.000, 2.000, 5.000 ... 100.000 rd. a month. */
	void StepMinistryBudget(ECampaign1851Portfolio P, int32 Dir);
	/** The month's allowance into each pot (at the month's close). */
	void RefillMinistryBudgets();
	/** An AUTO ministry of the player spends from its pot; false when the pot is short. */
	bool MinistryCanSpend(ECampaign1851Portfolio P, double Cost) const { return Cost <= MinistryPot[int32(P)] + 0.5; }
	void MinistrySpend(ECampaign1851Portfolio P, double Cost) { MinistryPot[int32(P)] = FMath::Max(0.0, MinistryPot[int32(P)] - Cost); }
private:
	void RestoreMinister(const TArray<FString>& P);
	FCampaign1851Minister Ministers[int32(ECampaign1851Portfolio::Count)];
	/** What each ministry may spend on AUTO: a monthly allowance (rd.) and the pot it fills (at most three months). */
	double MinistryBudget[int32(ECampaign1851Portfolio::Count)] = { 5000.0, 15000.0, 10000.0, 10000.0, 5000.0, 2000.0, 10000.0, 10000.0 };
	double MinistryPot[int32(ECampaign1851Portfolio::Count)] = { 5000.0, 15000.0, 10000.0, 10000.0, 5000.0, 2000.0, 10000.0, 10000.0 };
	// The end.
	void CheckCampaignEnd();
	int32 PlayedNation = 0;
	bool bEndPending = false;
	bool bEndShown = false;
	// Works and sieges.
	void MonthlyFortProgrammes();
	void DailySieges();
	TArray<FString> SaveFortProgrammes() const;
	void RestoreFortProgramme(int32 Index, int32 State, double Day);
	TArray<double> ProgrammeDay;
	TArray<int32> ProgrammeState;
	// Economy.
	void MonthlyEconomy();
	double Debt = 0.0;
	float DebtRate = 0.04f;
	TArray<FCampaign1851Record> History;
	TArray<TPair<double, FString>> NewsLog;
	// Politics.
	void MonthlyPolitics();
	void FormGovernment(const FString& Name, ECampaign1851Current Line, const FString& Why);
	void PoliticalShock(float MoodChange, float EjderChange);
	float Support[3] = { 45.f, 40.f, 15.f };
	float Mood = 60.f;
	FString PrimeMinister;
	ECampaign1851Current Government = ECampaign1851Current::Helstat;
	double GovernmentSince = 0.0;
	int32 NextCabinet = 0;
	int32 DanishWarLosses = 0;
	int32 EnemyWarLosses = 0;
	// Officers' careers (Campaign1851Career.cpp).
	void VacateOfficer(int32 Officer);
	// Health.
	void DailyHealth();
	/** A battle's losses of a unit: the wounded to its lazaret, the prisoners counted. */
	void SplitLosses(int32 RegimentIndex, int32 Lost, bool bDefeat, int32& OutPrisoners);
	int32 DanesCaptured = 0;
	int32 EnemyCaptured = 0;
	int32 EnemyKilled = 0, EnemyWounded = 0, EnemyCapturedTotal = 0;
	int32 CapturedRifles = 0, CapturedGuns = 0, CapturedHorses = 0, CapturedWagons = 0, CapturedColours = 0;
	// Weather.
	void DailyWeather();
	bool IceWinterOn(int32 Day) const;
	mutable int32 WeatherCacheDay = INT32_MIN;
	mutable int32 WeatherCacheSeed = 0;
	mutable int32 WeatherCacheStartYear = 0;
	mutable ECampaign1851Weather WeatherCache = ECampaign1851Weather::Clear;
	mutable float TemperatureCache = 0.f;
	mutable bool bIceCache = false;
	// The navy.
	void MonthlyNavy();
	TArray<FCampaign1851Ship> Ships;
	bool bBlockade = false;
	double AustrianSquadronDay = -1.0;
	// Research and doctrine.
	void MonthlyResearch();
	TSet<FString> Researched;
	int32 Researching = INDEX_NONE;
	int32 ResearchMonths = 0;
	bool bResearchStalled = false;
	int32 ResearchingCivil = INDEX_NONE;
	int32 ResearchMonthsCivil = 0;
	bool bResearchStalledCivil = false;
	int32 Doctrine[3] = { 0, 0, 1 };
	double DoctrineSettledDay = 0.0;
	TArray<FString> EventsFired;
	TArray<FPlannedEvent> EventPlan;
	TArray<FCampaign1851EnemyCorps> EnemyCorps;
	int32 NextCorpsId = 1;
	int32 LastWarDay = -1;

	// The footing layer.
	void AdvanceFooting(float DeltaDays);
	void MonthlyFooting();
	ECampaign1851Footing Footing = ECampaign1851Footing::Peace;

	// The materiel layer.
	void MonthlyMateriel();
	int32 Rifles = 0;
	int32 Horses = 0;

	// The salvage layer (Campaign1851Salvage.cpp).
	void AddMaterials(int32 CityIndex, double Rd, const FString& From, bool bNews = true);
	double MaterialsIn(int32 CityIndex) const;
	void MonthlyBuildingMaterials();
	static constexpr double MaterialStoreCap = 5000.0;
	void UseMaterials(const FVector2D& Km, double Cost, const FString& For);
	void MonthlySalvage();
	void AdvanceDemolitions(float DeltaDays);
	void FinishFortDemolition(int32 Index);
	int32 TakeGunsFromStock(int32 Wanted);
	int32 GunStock = 0;
	TArray<FVector> MaterialLots;   // town index, rd., campaign day stored

	// The manpower layer (Campaign1851Manpower.cpp).
	void ResetManpower();
	void MonthlyManpower();
	int32 AddRaisedRegiment(const FString& Id, const FString& Name, ECampaign1851Arm Arm, int32 Home, int32 MaxMen, bool bTraining = false);
	TArray<float> AmtManpower;

	// The fort layer (Campaign1851Forts.cpp).
	void AdvanceForts(float DeltaDays);
	void UpdateFortVisual(int32 Index);
	void UpdateTrenches();
	bool EnsureFortMeshes();
	FTransform FortTransform(const FCampaign1851Fort& F) const;
	TArray<FCampaign1851Fort> Forts;
	int32 NextFortId = 1;
	TArray<int32> FortPartOwner;   // fort id per part
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> FortParts;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMesh>> FortMeshes;

	// The world layer (Campaign1851Nations.cpp).
	bool LoadNations();
	void GrowMonth();
	void PrivateInvestment();
	void RunNationalAI();
	void RunAbstractNation(int32 NationIndex);
	TArray<FCampaign1851Decision> DecisionOptions(int32 NationIndex, ECampaign1851Portfolio P, double Budget, FRandomStream& Rng) const;
	bool CarryOut(const FCampaign1851Decision& D);
	void AddDecision(const FCampaign1851Decision& D);
	TArray<FCampaign1851Nation> NationsAtStart;
	TArray<FCampaign1851Nation> Nations;
	int32 PlayerNation = 0;
	int32 Seed = 1851;
	float Deviation = 0.f;
	TArray<FCampaign1851Decision> Decisions;
	TArray<float> AmtGrowthMul;
	TArray<int32> CityBasePopulation;
	TArray<FIntPoint> AmtBase;   // urban, rural in 1851
	TMap<FString, TPair<FDateTime, FDateTime>> RailwayBaseDates;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Chaussees;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RailBed;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RailTrack;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RailWorks;
	/** Close in: the track itself (gravel, sleepers, two rails) instead of the map symbol. */
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RailGravel;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RailSleepers;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> RailSteel;
	float LastCameraDistanceKm = 1000.f;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Trains;    // one per open railway (index = Railways)
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Stations;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Gangs;     // one per link (null when idle)
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> TrainMesh;
	/** Engine, brown and green carriage: a train is laid on the track vehicle by vehicle. */
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMesh>> TrainParts;
	/** Carriages behind a regiment's engine when it goes by rail (three per regiment). */
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RegimentCars;
	/** Vehicles per train: the engine and three carriages. */
	static constexpr int32 TrainVehicles = 4;
	void EnsureTrainParts();
	/**
	 * Lays a train on a line: Parts[0] the engine with its front FrontKm along the line, running towards
	 * Direction (+1: along the line, -1: back), the carriages behind it; each vehicle follows the curve.
	 */
	void PlaceTrain(const TArray<UStaticMeshComponent*>& Parts, const TArray<FVector2D>& Line, double FrontKm, float Direction, float Scale) const;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> GangMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> StationMesh;
	TArray<float> TrainAt;       // km along the line
	TArray<float> TrainDirection;
	TArray<float> TrainPause;
	float GangClock = 0.f;
	/** Progress (km) drawn at the last mesh rebuild, to rebuild only when the works have grown. */
	double DrawnWorkKm = -1.0;

	/** Season weights for the map materials (MPC_Campaign1851Season): Snow, Bare, Autumn, Spring. */
	void UpdateSeason();
	double CampaignDays = 0.0;
	double MinuteCarry = 0.0;   // part of a minute of real time not yet ticked
	double NextDayAt = 0.0;     // speed 6: wall-clock time (FPlatformTime) of the next day
	bool bDayBeat = false;
	/** Speed 6: real seconds per campaign day. */
	static constexpr float DaySeconds = 0.4f;
	int32 Speed = 1;

	/** Closes a month: grant in, upkeep and the month's construction wages out. */
	void CloseMonth();
	double Treasury = StartingTreasury;
	TArray<FCampaign1851Transaction> Ledger;
	/** Construction spending this month per "town, building", booked at month end. */
	TMap<FString, double> MonthSpend;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> SeasonCollection;
	bool bReady = false;
};
