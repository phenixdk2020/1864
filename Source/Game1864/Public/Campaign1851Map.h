#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Campaign1851SaveGame.h"
#include "Campaign1851Network.h"
#include "Campaign1851Army.h"
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
	/** Makes a general the commanding general of a general command (the previous one goes to the pool). */
	bool AssignCommandGeneral(int32 Officer, int32 Command);
	/** Why an officer cannot be promoted now (empty if he can): top rank, too little experience. */
	FString PromotionBlock(int32 Officer) const;
	/** Promotes an officer one rank; a colonel made generalmajor leaves his regiment for the pool of generals. */
	bool PromoteOfficer(int32 Officer);

	// ---- Troop trains (rolling stock, backlog B-380): the state's trains, those under way, those on order.
	static constexpr int32 TroopTrainCost = 30000;
	static constexpr float TroopTrainDeliveryDays = 120.f;
	int32 GetTroopTrains() const { return TroopTrains; }
	/** Trains not carrying (or returning from) a column now. */
	int32 FreeTroopTrains() const;
	/** Busy trains: how many, and the day each group is back (campaign days). */
	const TArray<FVector2D>& GetTrainBookings() const { return TrainBookings; }
	/** Ordered trains: how many, and the day they arrive (campaign days). */
	const TArray<FVector2D>& GetTrainOrders() const { return TrainOrders; }
	/** Loaded troop trains (a save). */
	void RestoreTrains(int32 Count, const TArray<FVector2D>& Bookings, const TArray<FVector2D>& Orders) { TroopTrains = Count; TrainBookings = Bookings; TrainOrders = Orders; }
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
	TArray<FString> CommandGeneralIds;
	int32 TroopTrains = 4;
	TArray<FVector2D> TrainBookings;   // X count, Y campaign day free again
	TArray<FVector2D> TrainOrders;     // X count, Y campaign day of delivery
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
