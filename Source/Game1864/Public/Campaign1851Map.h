#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Campaign1851Map.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;
class ACampaign1851ConstructionSite;

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

	/** Military building plot beside a main road at the edge of town (towns of 2,500+). */
	bool bHasPlot = false;
	FVector2D PlotKm = FVector2D::ZeroVector;
	float PlotYaw = 0.f;   // world yaw of the site; its +Y (parade ground, gate) faces the road
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
	const FCampaign1851Extent& GetBornholmExtent() const { return Bornholm; }
	FVector2D GetSizeKm() const { return SizeKm; }
	bool IsReady() const { return bReady; }

	/** Starts (or returns the running) barracks project in a town; null if the town has no plot. */
	ACampaign1851ConstructionSite* StartProject(int32 CityIndex);
	ACampaign1851ConstructionSite* FindProject(int32 CityIndex) const;
	/** Removes every building project (before loading a save). */
	void ClearProjects();
	/** Recreates a saved garrison complex; returns false if the town is unknown or has no plot. */
	bool RestoreProject(const FString& CityName, const TArray<float>& ModuleDays, int32 ActiveModule);
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
	TArray<TArray<FVector2D>> RoadLines;   // projected km
	TArray<TArray<FVector2D>> FerryLines;  // projected km, landing to landing

	TArray<uint16> Height;
	int32 HeightW = 0, HeightH = 0;
	TArray<float> GridZ;               // terrain mesh vertex heights (units)
	TArray<FColor> Features;           // R = monarchy land, G = woodland
	int32 FeaturesW = 0, FeaturesH = 0;
	bool bSceneryVisible = false;
	bool bRoadsVisible = false;
	bool bReady = false;
};
