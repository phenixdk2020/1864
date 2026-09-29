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
	static constexpr double RuralTaxPerHead = 0.2;
	static constexpr double UrbanTaxPerHead = 0.45;
	static double AmtYearlyTax(const FCampaign1851Amt& Amt) { return Amt.Rural * RuralTaxPerHead + Amt.Urban * UrbanTaxPerHead; }
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
	void SetSpeed(int32 InSpeed) { Speed = FMath::Clamp(InSpeed, 0, NumSpeeds() - 1); }
	int32 GetSpeed() const { return Speed; }
	static int32 NumSpeeds() { return 7; }
	/** Campaign hours per real second at a speed step (0, 0.25, 1, 3, 8, 24, 96); speed 6 jumps a whole day at a time. */
	static float HoursPerSecondAt(int32 InSpeed);
	static const TCHAR* SpeedLabel(int32 InSpeed);
	/** "14:37" */
	static FString FormatClock(const FDateTime& Date) { return FString::Printf(TEXT("%02d:%02d"), Date.GetHour(), Date.GetMinute()); }
	double GetCampaignDays() const { return CampaignDays; }
	void SetCampaignDays(double Days) { CampaignDays = FMath::Max(FMath::RoundToDouble(Days * 1440.0) / 1440.0, 0.0); MinuteCarry = 0.0; UpdateSeason(); }
	static FDateTime StartDate() { return FDateTime(1851, 7, 1); }
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
	/** Upkeep a month of the battalions raised since 1851. */
	double RaisedUpkeepPerMonth() const;
	/** A battalion's strength to fill up to (less its companies in forts). */
	int32 BattalionTarget(int32 Regiment) const;
	const TArray<float>& GetAmtManpower() const { return AmtManpower; }
	void SetAmtManpower(const TArray<float>& In) { if (In.Num() == AmtManpower.Num()) { AmtManpower = In; } }

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
	FCampaign1851NationFigures NationFigures(int32 NationIndex) const;
	void SaveWorld(UCampaign1851SaveGame* Save) const;
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
	TArray<FString> TakeNews() { TArray<FString> Out = MoveTemp(News); News.Reset(); return Out; }
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
	/** A company's number, counted through its regiment (the 2nd battalion has companies 5-8). */
	int32 CompanyNumber(int32 Regiment, int32 Company) const;
	/** A company's men (the battalion's strength spread over its companies). */
	int32 CompanyMen(int32 Regiment, int32 Company) const;
	/** Hires a new officer or general into the pool (pays the cost); INDEX_NONE if the treasury cannot. */
	int32 RecruitOfficer(bool bGeneral);
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
	/** Dissolves a formation: its units and sub-formations go up a level. */
	void DissolveFormation(int32 Id);
	/** True if formation Id is Ancestor or lies under it. */
	bool IsInside(int32 Id, int32 Ancestor) const;
	/** Puts a formation under another (0 = the field army); refused if it would go under itself. */
	bool MoveFormation(int32 Id, int32 NewParent);
	/** Puts a regiment in a formation (0 = back to its garrison). */
	bool MoveRegimentToFormation(int32 Regiment, int32 Formation);
	/** Every regiment in a formation and its sub-formations. */
	TArray<int32> FormationRegiments(int32 Id) const;
	/** Makes an officer the commander of a formation (he leaves any other post). */
	bool AssignFormationCommander(int32 Officer, int32 Formation);
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
	bool OrderTroopTrain();
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

	/** Main roads between the towns (routed by build_map.py). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Roads;

	/** Ferry crossings (dashed over the water), shown with the main roads. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Ferries;

	/** Village lanes, generated with the scenery. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Lanes;

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

	// The manpower layer (Campaign1851Manpower.cpp).
	void ResetManpower();
	void MonthlyManpower();
	int32 AddRaisedRegiment(const FString& Id, const FString& Name, ECampaign1851Arm Arm, int32 Home, int32 MaxMen);
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
