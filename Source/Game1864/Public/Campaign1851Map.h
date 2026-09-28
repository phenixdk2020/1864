#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Campaign1851SaveGame.h"
#include "Campaign1851Map.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;
class ACampaign1851ConstructionSite;
class UMaterialParameterCollection;

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

	// ---- Campaign calendar: days since 1 July 1851 and the game speed (0 = paused).

	/** Moves the calendar on by the real time passed; drives the building projects and the season. */
	void AdvanceTime(float DeltaSeconds);
	void SetSpeed(int32 InSpeed) { Speed = FMath::Clamp(InSpeed, 0, NumSpeeds() - 1); }
	int32 GetSpeed() const { return Speed; }
	static int32 NumSpeeds() { return 4; }
	/** Campaign days per real second at a speed step (0, 1, 3, 10). */
	static float DaysPerSecondAt(int32 InSpeed);
	static const TCHAR* SpeedLabel(int32 InSpeed);
	double GetCampaignDays() const { return CampaignDays; }
	void SetCampaignDays(double Days) { CampaignDays = FMath::Max(Days, 0.0); UpdateSeason(); }
	static FDateTime StartDate() { return FDateTime(1851, 7, 1); }
	FDateTime GetDate() const { return StartDate() + FTimespan::FromDays(CampaignDays); }
	/** "1. juli 1851", or "1. jul. 1851" with bShort. */
	static FString FormatDate(const FDateTime& Date, bool bShort = false);
	/** "Vinter", "Forår", "Sommer" or "Efterår". */
	FString GetSeasonName() const;

	// ---- Treasury (design manual 20.2: money / state credit; backlog B-346 budget with a transaction log).

	/** Cash at the start of a campaign; income comes from the amter's taxes (YearlyTax). */
	static constexpr double StartingTreasury = 150000.0;
	double GetTreasury() const { return Treasury; }
	const TArray<FCampaign1851Transaction>& GetLedger() const { return Ledger; }
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

	/** Season weights for the map materials (MPC_Campaign1851Season): Snow, Bare, Autumn, Spring. */
	void UpdateSeason();
	double CampaignDays = 0.0;
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
