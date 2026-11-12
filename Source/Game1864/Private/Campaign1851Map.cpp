#include "Campaign1851Map.h"
#include "Campaign1851DecisionLog.h"

#include "Campaign1851Buildings.h"
#include "Campaign1851ConstructionSite.h"
#include "Campaign1851Scenery.h"
#include "Engine/World.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Campaign1851Json.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshAttributes.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"

namespace
{
	constexpr int32 GridX = 252;
	constexpr int32 GridY = 684; // doubled north extent retains Danish mesh density

	/** Raw map data (height PNG + JSON) lives outside Content so the editor does not offer to import it. */
	FString DataPath(const TCHAR* File)
	{
		return FPaths::ProjectDir() / TEXT("Data/Campaign1851") / File;
	}

	const TCHAR* FlatColourPath = TEXT("/Game/Materials/M_FlatColor.M_FlatColor");
#ifndef CAMPAIGN1851_SRGB   // one definition per unity blob
#define CAMPAIGN1851_SRGB
	FLinearColor Srgb(uint8 R, uint8 G, uint8 B) { return FLinearColor::FromSRGBColor(FColor(R, G, B)); }
#endif

	const TCHAR* ConstructionMaterialPath = TEXT("/Game/Campaign1851/M_Campaign1851Construction.M_Campaign1851Construction");
	const TCHAR* SceneryMaterialPath = TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery");
}

ACampaign1851Map::ACampaign1851Map()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Terrain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Terrain"));
	Terrain->SetupAttachment(Root);
	Terrain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Terrain->SetCastShadow(false);

	Backdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(Root);
	Backdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Backdrop->SetCastShadow(false);

	WorldSheet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldSheet"));
	WorldSheet->SetupAttachment(Root);
	WorldSheet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorldSheet->SetCastShadow(false);

	CityMarkers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CityMarkers"));
	CityMarkers->SetupAttachment(Root);
	CityMarkers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CityMarkers->SetCastShadow(false);

	ForeignMarkers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ForeignMarkers"));
	ForeignMarkers->SetupAttachment(Root);
	ForeignMarkers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ForeignMarkers->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Campaign1851/M_Campaign1851Map.M_Campaign1851Map"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BornholmTex(TEXT("/Game/Campaign1851/Map/Bornholm1851_Color.Bornholm1851_Color"));
	CityMarkers->SetStaticMesh(Sphere.Object);
	ForeignMarkers->SetStaticMesh(Sphere.Object);
	Backdrop->SetStaticMesh(Plane.Object);
	WorldSheet->SetStaticMesh(Plane.Object);
	if (Material.Succeeded())
	{
		MapMaterial = Material.Object;
	}
	if (BornholmTex.Succeeded())
	{
		BornholmTexture = BornholmTex.Object;
	}
}

void ACampaign1851Map::BeginPlay()
{
	Super::BeginPlay();
	if (!LoadData() || !LoadHeight())
	{
		UE_LOG(LogTemp, Error, TEXT("CAMPAIGN-1851|could not load map data from %s"), *DataPath(TEXT("")));
		return;
	}
	BuildTerrain();
	BuildMarkers();
	SeasonCollection = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Campaign1851/MPC_Campaign1851Season.MPC_Campaign1851Season"));
	if (!LoadAmtIds())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|no Denmark1851_Amter.png; amter cannot be picked on the map"));
	}
	const bool bFeatures = LoadFeatures();
	LoadHydro();
	BuildHydroMeshes();
	BuildBorderMeshes();
	BuildNeighbourMeshes();
	if (bFeatures)
	{
		// The bridges first, so the towns keep their houses off them (houses stood on Knippelsbro, 2026-11-11);
		// ResetNetwork finds the same bridges again.
		DetectBridges();
		BuildScenery();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|no Denmark1851_Features.png; the close zoom has no 3D scenery"));
	}
	bReady = true;
	UpdateSeason();
	Campaign1851Buildings::Load();
	ResetEconomy();
	ResetNetwork();
	LoadNations();
	const bool bArmyDataLoaded = LoadArmy(); // scenario stores must exist before ResetResources in ResetWorld
	ResetWorld(1851, 0.f);
	if (bArmyDataLoaded)
	{
		LoadOfficers();
		ResetArmy();
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|ready|cities=%d|labels=%d|size=%.0fx%.0f km"), Cities.Num(), Labels.Num(), SizeKm.X, SizeKm.Y);
}

void ACampaign1851Map::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bUseMapTiles && bReady) { UpdateMapTiles(); }
	// Plots only change with population and building projects (which update at once), so once a second is enough.
	const double TownNow = FPlatformTime::Seconds();
	if (TownNow >= TownPlotsNextUpdateSeconds)
	{
		TownPlotsNextUpdateSeconds = TownNow + 1.0;
		TownUpdatePlots();
	}
}

void ACampaign1851Map::DebugScaleTownPopulation(float TownFactor)
{
	for (FCampaign1851City& TownGrowthCity : Cities)
		if (!TownGrowthCity.bForeign) TownGrowthCity.Population = FMath::RoundToInt(TownGrowthCity.Population * TownFactor);
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|town-growth-test|factor=%.2f"), TownFactor);
	TownUpdatePlots();
}

// Reconstruct occupancy from population and saved site coordinates, never from event order.
void ACampaign1851Map::TownUpdatePlots()
{
	TownHouseReservations = TownBaseReservations;
	for (const auto& TownPair : TownEdgePlots)
	{
		const int32 TownCity = TownPair.Key;
		if (!Cities.IsValidIndex(TownCity)) continue;
		const FTownPopulationLayout* TownPopulation = TownPopulationLayouts.Find(TownCity);
		int32 TownRemaining = TownPopulation ? TownPopulation->TownHouseCount(Cities[TownCity].Population) : FMath::Max(18, 18 + Cities[TownCity].Population / 1000);
		int32 TownSites = 0, TownLastOrdinal = -2;
		bool TownWanted = false;
		TArray<FVector2D>& TownFree = TownReservedPlots.FindOrAdd(TownCity);
		const TArray<FVector2D> TownPrevious = TownFree;
		TownFree.Reset();
		for (FTownVisualPlot& TownPlot : TownVisualPlots)
		{
			if (TownPlot.TownCity != TownCity) continue;
			if (TownLastOrdinal != TownPlot.TownOrdinal)
			{
				TownLastOrdinal = TownPlot.TownOrdinal;
				bool TownBlocked = false;
				for (const ACampaign1851ConstructionSite* TownSite : Projects)
					if (TownSite && FVector2D::Distance(TownPlot.TownKm, TownSite->PlotKm) < TownPlot.TownRadius + TownSite->PlotRadiusKm) TownBlocked = true;
				TownWanted = TownPlot.TownOrdinal < 0 || (!TownBlocked && TownRemaining > 0);
				if (TownWanted && TownPlot.TownOrdinal >= 0) --TownRemaining;
			}
			if (TownWanted)
			{
				TownHouseReservations.Add({ TownPlot.TownKm, TownPlot.TownRadius });
			}
			if (TownWanted == TownPlot.TownActive) continue;
			TownPlot.TownActive = TownWanted;
			if (TownWanted) TownPlot.TownComponent->AddInstance(TownPlot.TownTransform);
			else TownPlot.TownComponent->ClearInstances();
		}
		for (int32 TownSlot = 0; TownSlot < TownPair.Value.Num(); ++TownSlot)
		{
			const FVector2D TownKm = TownPair.Value[TownSlot];
			bool TownOccupied = false;
			for (const auto& TownHouse : TownHouseReservations)
				if (FVector2D::Distance(TownKm, TownHouse.Key) < 0.3 + TownHouse.Value) TownOccupied = true;
			for (const ACampaign1851ConstructionSite* TownSite : Projects)
				if (TownSite && FVector2D::Distance(TownKm, TownSite->PlotKm) < 0.3 + TownSite->PlotRadiusKm) TownOccupied = true;
			const bool TownReserved = !TownOccupied && TownFree.Num() < (TownPopulation ? TownPopulation->TownReservedCount : 4);
			if (TownReserved) TownFree.Add(TownKm);
			if (const auto* TownStreets = TownEdgeStreets.Find(TownCity))
				if (TownStreets->IsValidIndex(TownSlot) && (*TownStreets)[TownSlot])
					(*TownStreets)[TownSlot]->SetVisibility(bSceneryVisible && (TownOccupied || TownReserved));
		}
		for (const ACampaign1851ConstructionSite* TownSite : Projects)
			if (TownSite && !TownSite->IsGarrison() && TownSite->GetCityIndex() == TownCity && TownSite->GetModule(0).Rule == ECampaign1851PlotRule::InTown) ++TownSites;
		if (TownPrevious != TownFree || TownLoggedEdgeSites.FindOrAdd(TownCity) != TownSites)
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|scenery|town %s reserved-plots=%d edge-sites=%d pop=%d"), *Cities[TownCity].Name, TownFree.Num(), TownSites, Cities[TownCity].Population);
		TownLoggedEdgeSites.FindOrAdd(TownCity) = TownSites;
	}
}

// ------------------------------------------------------------------ data

bool ACampaign1851Map::LoadData()
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *DataPath(TEXT("Denmark1851_Map.json"))))
	{
		return false;
	}
	TSharedPtr<FJsonObject> Json;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		return false;
	}

	FCampaign1851Projection Projection;
	const TSharedPtr<FJsonObject>* Proj = nullptr;
	if (!Json->TryGetObjectField(TEXT("projection"), Proj) || (*Proj)->GetStringField(TEXT("type")) != TEXT("laea"))
	{
		UE_LOG(LogTemp, Error, TEXT("CAMPAIGN-1851|map data has no LAEA projection; regenerate it with tools/map1851/build_map.py"));
		return false;
	}
	Projection.Lat0 = (*Proj)->GetNumberField(TEXT("lat0"));
	Projection.Lon0 = (*Proj)->GetNumberField(TEXT("lon0"));
	Projection.RadiusKm = (*Proj)->GetNumberField(TEXT("radiusKm"));

	auto ReadExtent = [&Projection](const TSharedPtr<FJsonObject>& Obj, FCampaign1851Extent& Out)
	{
		Out.XMin = Obj->GetNumberField(TEXT("xMin"));
		Out.XMax = Obj->GetNumberField(TEXT("xMax"));
		Out.YMin = Obj->GetNumberField(TEXT("yMin"));
		Out.YMax = Obj->GetNumberField(TEXT("yMax"));
		Out.Projection = Projection;
	};
	ReadExtent(Json->GetObjectField(TEXT("extentKm")), Extent);
	MapTextureYScale = Json->HasField(TEXT("northExtension")) ? 2.f : 1.f;
	if (const TSharedPtr<FJsonObject>* BornholmObj = nullptr; Json->TryGetObjectField(TEXT("bornholmKm"), BornholmObj))
	{
		ReadExtent(*BornholmObj, Bornholm);   // old data: Bornholm as an inset
	}
	if (const TSharedPtr<FJsonObject>* WorldObj = nullptr; Json->TryGetObjectField(TEXT("worldKm"), WorldObj))
	{
		ReadExtent(*WorldObj, WorldExtent);
		bHasWorld = true;
	}
	SizeKm = FVector2D(Extent.XMax - Extent.XMin, Extent.YMax - Extent.YMin);
	Json->TryGetNumberField(TEXT("detailTileKm"), DetailTileKm);
	LoadMapTiles();

	// Population overlays retain map indices and geography for save compatibility.
	if (ActiveScenario().Id == TEXT("1825"))
	{
		FString PopulationText;
		TSharedPtr<FJsonObject> PopulationJson;
		if (!FFileHelper::LoadFileToString(PopulationText, *DataPath(TEXT("Population_1825.json")))
			|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(PopulationText), PopulationJson) || !PopulationJson.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("CAMPAIGN-1825|mangler gyldige befolkningsdata"));
			return false;
		}
		for (const TCHAR* ArrayName : { TEXT("cities"), TEXT("foreignCities"), TEXT("amter") })
		{
			const bool bAmt = FString(ArrayName) == TEXT("amter");
			const TArray<TSharedPtr<FJsonValue>>* PopulationRows = nullptr;
			if (!PopulationJson->TryGetArrayField(ArrayName, PopulationRows)) { return false; }
			for (const TSharedPtr<FJsonValue>& MapValue : Json->GetArrayField(ArrayName))
			{
				const TSharedPtr<FJsonObject> MapRow = MapValue->AsObject();
				const TSharedPtr<FJsonValue>* Found = PopulationRows->FindByPredicate([&](const TSharedPtr<FJsonValue>& Entry)
				{
					const TSharedPtr<FJsonObject> ImportPopulationEntry = Campaign1851Json::Row(Entry);
					if (bAmt)
					{
						double ImportPopulationId = 0;
						return ImportPopulationEntry->TryGetNumberField(TEXT("id"), ImportPopulationId)
							&& ImportPopulationId == MapRow->GetNumberField(TEXT("id"));
					}
					FString ImportPopulationName;
					return ImportPopulationEntry->TryGetStringField(TEXT("name"), ImportPopulationName)
						&& ImportPopulationName == MapRow->GetStringField(TEXT("name"));
				});
				if (!Found) { UE_LOG(LogTemp, Error, TEXT("CAMPAIGN-1825|ufuldstændige befolkningsdata")); return false; }
				for (const TCHAR* Field : { TEXT("pop"), TEXT("population"), TEXT("urban"), TEXT("rural") })
				{
					double PopulationValue = 0;
					if ((*Found)->AsObject()->TryGetNumberField(Field, PopulationValue)) { MapRow->SetNumberField(Field, PopulationValue); }
				}
			}
		}
	}

	auto ReadCities = [this](const TArray<TSharedPtr<FJsonValue>>& Array, bool bForeign)
	{
		for (const TSharedPtr<FJsonValue>& Value : Array)
		{
			const TSharedPtr<FJsonObject> O = Value->AsObject();
			FCampaign1851City C;
			C.Name = O->GetStringField(TEXT("name"));
			C.Lat = O->GetNumberField(TEXT("lat"));
			C.Lon = O->GetNumberField(TEXT("lon"));
			C.Population = FMath::RoundToInt(float(O->GetNumberField(TEXT("pop"))));
			O->TryGetStringField(TEXT("region"), C.Region);
			O->TryGetBoolField(TEXT("capital"), C.bCapital);
			O->TryGetBoolField(TEXT("bornholm"), C.bBornholm);
			O->TryGetNumberField(TEXT("amt"), C.AmtId);
			C.bForeign = bForeign;
			Cities.Add(C);
		}
	};
	ReadCities(Json->GetArrayField(TEXT("cities")), false);
	ReadCities(Json->GetArrayField(TEXT("foreignCities")), true);
	LoadNeighbours(*Json);

	const TArray<TSharedPtr<FJsonValue>>* RoadArray = nullptr;
	if (Json->TryGetArrayField(TEXT("roads"), RoadArray))
	{
		for (const TSharedPtr<FJsonValue>& Road : *RoadArray)
		{
			FString Kind;
			Road->AsObject()->TryGetStringField(TEXT("kind"), Kind);
			TArray<FVector2D>& Line = (Kind == TEXT("ferry") ? FerryLines : RoadLines).AddDefaulted_GetRef();
			for (const TSharedPtr<FJsonValue>& Point : Road->AsObject()->GetArrayField(TEXT("km")))
			{
				const TArray<TSharedPtr<FJsonValue>>& XY = Point->AsArray();
				Line.Add(FVector2D(XY[0]->AsNumber(), XY[1]->AsNumber()));
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* AmtArray = nullptr;
	if (Json->TryGetArrayField(TEXT("amter"), AmtArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *AmtArray)
		{
			const TSharedPtr<FJsonObject> O = Value->AsObject();
			FCampaign1851Amt& A = Amter.AddDefaulted_GetRef();
			A.Id = int32(O->GetNumberField(TEXT("id")));
			A.Name = O->GetStringField(TEXT("name"));
			A.Seat = O->GetStringField(TEXT("seat"));
			A.Region = O->GetStringField(TEXT("region"));
			O->TryGetNumberField(TEXT("population"), A.Population);
			O->TryGetNumberField(TEXT("urban"), A.Urban);
			O->TryGetNumberField(TEXT("rural"), A.Rural);
			double Area = 0.0;
			O->TryGetNumberField(TEXT("areaKm2"), Area);
			A.AreaKm2 = float(Area);
			O->TryGetStringArrayField(TEXT("towns"), A.Towns);
		}
	}

	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("labels")))
	{
		const TSharedPtr<FJsonObject> O = Value->AsObject();
		FCampaign1851Label L;
		L.Text = O->GetStringField(TEXT("text"));
		L.Lat = O->GetNumberField(TEXT("lat"));
		L.Lon = O->GetNumberField(TEXT("lon"));
		L.Kind = O->GetStringField(TEXT("kind"));
		Labels.Add(L);
	}
	if (!LoadNetwork(*Json))
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|map data has no road links; regenerate it with tools/map1851/build_map.py"));
	}
	return true;
}

bool ACampaign1851Map::LoadHeight()
{
	TArray<uint8> Png;
	if (!FFileHelper::LoadFileToArray(Png, *DataPath(TEXT("Denmark1851_Height.png"))))
	{
		return false;
	}
	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
	TArray<uint8> Raw;
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Png.GetData(), Png.Num()) || !Wrapper->GetRaw(ERGBFormat::Gray, 16, Raw))
	{
		return false;
	}
	HeightW = Wrapper->GetWidth();
	HeightH = Wrapper->GetHeight();
	Height.SetNumUninitialized(HeightW * HeightH);
	FMemory::Memcpy(Height.GetData(), Raw.GetData(), Height.Num() * sizeof(uint16));
	return true;
}

float ACampaign1851Map::SampleHeight01(const FVector2D& Uv) const
{
    if (bUseMapTiles) { return TileHeightAtKm(FVector2D(Extent.XMin + Uv.X * SizeKm.X, Extent.YMin + Uv.Y * SizeKm.Y)) / (HeightRangeKm * float(KmToUnits)); }
	if (Height.Num() == 0 || Uv.X < 0.0 || Uv.X > 1.0 || Uv.Y < 0.0 || Uv.Y > 1.0)
	{
		return 0.f;
	}
	// Image row 0 is north; uv.Y = 0 is south.
	const float Fx = float(Uv.X) * (HeightW - 1);
	const float Fy = float(1.0 - Uv.Y) * (HeightH - 1);
	const int32 X0 = FMath::Clamp(FMath::FloorToInt(Fx), 0, HeightW - 2);
	const int32 Y0 = FMath::Clamp(FMath::FloorToInt(Fy), 0, HeightH - 2);
	const float Ax = Fx - X0, Ay = Fy - Y0;
	auto H = [this](int32 X, int32 Y) { return Height[Y * HeightW + X] / 65535.f; };
	return FMath::Lerp(FMath::Lerp(H(X0, Y0), H(X0 + 1, Y0), Ax), FMath::Lerp(H(X0, Y0 + 1), H(X0 + 1, Y0 + 1), Ax), Ay);
}

FVector ACampaign1851Map::Project(double Lat, double Lon) const
{
	const FVector2D Uv = Extent.ToUv(Lat, Lon);
	const double X = (Uv.X - 0.5) * SizeKm.X * KmToUnits;
	const double Y = -(Uv.Y - 0.5) * SizeKm.Y * KmToUnits;
	const double Z = bUseMapTiles ? TerrainZ(Uv) : SampleHeight01(Uv) * HeightRangeKm * KmToUnits;
	return GetActorTransform().TransformPosition(FVector(X, Y, Z));
}

float ACampaign1851Map::SizeClass(int32 Population)
{
	if (Population > 50000) return 1.9f;
	if (Population > 20000) return 1.5f;
	if (Population > 10000) return 1.25f;
	if (Population > 5000) return 1.0f;
	return 0.75f;
}

// ------------------------------------------------------------------ terrain

void ACampaign1851Map::BuildTerrain()
{
    if (bUseMapTiles)
    {
        // Overview is immediately usable; meshes are built one at a time after BeginPlay.
        const FVector2D TileCentre = GetNavigationCentre();
        WorldSheet->SetRelativeLocation(FVector(TileCentre.X, TileCentre.Y, -20.0));
        const FVector2D TileSize = GetNavigationSizeKm();
        WorldSheet->SetRelativeScale3D(FVector(TileSize.X * KmToUnits / 100.0, TileSize.Y * KmToUnits / 100.0, 1.0));
        WorldSheet->SetMaterial(0, TileOverviewMaterial);
        Terrain->SetVisibility(false);
        Backdrop->SetVisibility(false);
        return;
    }
	FMeshDescription Mesh;
	FStaticMeshAttributes Attributes(Mesh);
	Attributes.Register();
	Attributes.GetVertexInstanceUVs().SetNumChannels(1);
	const FPolygonGroupID Group = Mesh.CreatePolygonGroup();
	const FPolygonGroupID NorthLandGroup = Mesh.CreatePolygonGroup();
	const FPolygonGroupID NorthSeaGroup = Mesh.CreatePolygonGroup();
	Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Map");
	Attributes.GetPolygonGroupMaterialSlotNames()[NorthLandGroup] = TEXT("NorthLand");
	Attributes.GetPolygonGroupMaterialSlotNames()[NorthSeaGroup] = TEXT("NorthSea");

	TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> Signs = Attributes.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();

	const int32 VX = GridX + 1, VY = GridY + 1;
	GridZ.SetNumUninitialized(VX * VY);
	TArray<FVertexInstanceID> Instances;
	Instances.Reserve(VX * VY);
	for (int32 j = 0; j < VY; ++j)
	{
		for (int32 i = 0; i < VX; ++i)
		{
			const FVector2D Uv(double(i) / GridX, double(j) / GridY);
			const FVertexID V = Mesh.CreateVertex();
			GridZ[j * VX + i] = SampleHeight01(Uv) * HeightRangeKm * float(KmToUnits);
			Positions[V] = FVector3f(
				float((Uv.X - 0.5) * SizeKm.X * KmToUnits),
				float(-(Uv.Y - 0.5) * SizeKm.Y * KmToUnits),
				GridZ[j * VX + i]);
			const FVertexInstanceID I = Mesh.CreateVertexInstance(V);
			Normals[I] = FVector3f::UpVector;
			Tangents[I] = FVector3f::ForwardVector;
			Signs[I] = 1.f;
			UVs.Set(I, 0, FVector2f(float(Uv.X), float(1.0 - Uv.Y * MapTextureYScale)));  // texture row 0 = north
			Instances.Add(I);
		}
	}

	auto Tri = [&](int32 A, int32 B, int32 C)
	{
		// Unreal's front face normal is (P2 - P0) x (P1 - P0); the terrain must face +Z.
		const FVector3f P0 = Positions[Mesh.GetVertexInstanceVertex(Instances[A])];
		const FVector3f P1 = Positions[Mesh.GetVertexInstanceVertex(Instances[B])];
		const FVector3f P2 = Positions[Mesh.GetVertexInstanceVertex(Instances[C])];
		if (FVector3f::CrossProduct(P2 - P0, P1 - P0).Z < 0.f)
		{
			Swap(B, C);
		}
		const FVertexInstanceID Corners[3] = { Instances[A], Instances[B], Instances[C] };
		const FVector3f NorthTriangleCentre = (P0 + P1 + P2) / 3.f;
		const FVector2D NorthTriangleUv(NorthTriangleCentre.X / (SizeKm.X * KmToUnits) + 0.5, 0.5 - NorthTriangleCentre.Y / (SizeKm.Y * KmToUnits));
		const bool bNorthTriangle = MapTextureYScale > 1.f && NorthTriangleUv.Y > 1.0 / MapTextureYScale;
		Mesh.CreateTriangle(bNorthTriangle ? (SampleHeight01(NorthTriangleUv) >= 0.002f ? NorthLandGroup : NorthSeaGroup) : Group, Corners);
	};
	for (int32 j = 0; j < GridY; ++j)
	{
		for (int32 i = 0; i < GridX; ++i)
		{
			const int32 A = j * VX + i;
			Tri(A, A + 1, A + VX);
			Tri(A + 1, A + VX + 1, A + VX);
		}
	}

	UStaticMesh* Static = NewObject<UStaticMesh>(this, TEXT("SM_Campaign1851_Terrain"), RF_Transient);
	Static->GetStaticMaterials().Add(FStaticMaterial(MapMaterial, TEXT("Map")));
	// Original Danish textures keep their geographic UVs. Northern land/sea use flat colours;
	// no Content asset import or editor start is needed for the extension.
	UMaterialInstanceDynamic* NorthLandMaterial = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, FlatColourPath), this);
	UMaterialInstanceDynamic* NorthSeaMaterial = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, FlatColourPath), this);
	if (NorthLandMaterial) { NorthLandMaterial->SetVectorParameterValue(TEXT("Color"), Srgb(117, 125, 82)); }
	if (NorthSeaMaterial) { NorthSeaMaterial->SetVectorParameterValue(TEXT("Color"), Srgb(20, 38, 51)); }
	Static->GetStaticMaterials().Add(FStaticMaterial(NorthLandMaterial ? static_cast<UMaterialInterface*>(NorthLandMaterial) : static_cast<UMaterialInterface*>(MapMaterial), TEXT("NorthLand")));
	Static->GetStaticMaterials().Add(FStaticMaterial(NorthSeaMaterial ? static_cast<UMaterialInterface*>(NorthSeaMaterial) : static_cast<UMaterialInterface*>(MapMaterial), TEXT("NorthSea")));
	Static->bSupportRayTracing = false;
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;
	Params.bMarkPackageDirty = false;
	Params.bCommitMeshDescription = false;
	Static->BuildFromMeshDescriptions({ &Mesh }, Params);
	Terrain->SetStaticMesh(Static);
	Terrain->SetMaterial(0, MapMaterial);

	// Backdrop: the map sheet fades into this colour at its edges.
	Backdrop->SetRelativeLocation(FVector(0.0, 0.0, -50.0));
	Backdrop->SetRelativeScale3D(FVector(SizeKm.X * KmToUnits * 0.4, SizeKm.Y * KmToUnits * 0.4, 1.0));
	if (UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Campaign1851/M_Campaign1851Backdrop.M_Campaign1851Backdrop")))
	{
		Backdrop->SetMaterial(0, Parent);
	}
	if (bHasWorld)
	{
		// The coarse sheet of the world under the detailed map: its own place and size (km, same frame as the map).
		const double WorldW = WorldExtent.XMax - WorldExtent.XMin, WorldH = WorldExtent.YMax - WorldExtent.YMin;
		const double Dx = (WorldExtent.XMin + WorldExtent.XMax) * 0.5 - (Extent.XMin + Extent.XMax) * 0.5;
		const double Dy = (WorldExtent.YMin + WorldExtent.YMax) * 0.5 - (Extent.YMin + Extent.YMax) * 0.5;
		WorldSheet->SetRelativeLocation(FVector(Dx * KmToUnits, -Dy * KmToUnits, -20.0));
		WorldSheet->SetRelativeScale3D(FVector(WorldW * KmToUnits / 100.0, WorldH * KmToUnits / 100.0, 1.0));
		if (UMaterialInterface* WorldMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Campaign1851/M_Campaign1851World.M_Campaign1851World")))
		{
			WorldSheet->SetMaterial(0, WorldMat);
		}
	}
	else
	{
		WorldSheet->SetVisibility(false);
	}
}

// ------------------------------------------------------------------ markers

void ACampaign1851Map::BuildMarkers()
{
	auto Paint = [](UInstancedStaticMeshComponent* Ism, const FLinearColor& Colour)
	{
		if (UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, FlatColourPath))
		{
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Parent, Ism);
			Mid->SetVectorParameterValue(TEXT("Color"), Colour);
			Mid->SetScalarParameterValue(TEXT("Roughness"), 0.35f);
			Ism->SetMaterial(0, Mid);
		}
	};
	Paint(CityMarkers, FLinearColor(0.55f, 0.03f, 0.02f));
	Paint(ForeignMarkers, FLinearColor(0.16f, 0.15f, 0.13f));

	for (FCampaign1851City& City : Cities)
	{
		if (City.bBornholm)
		{
			continue;
		}
		City.World = Project(City.Lat, City.Lon);
		(City.bForeign ? ForeignMarkers : CityMarkers)->AddInstance(FTransform(City.World), true);
	}
	for (FCampaign1851Label& Label : Labels)
	{
		Label.World = Project(Label.Lat, Label.Lon);
	}
}

void ACampaign1851Map::UpdateMarkers(float CameraDistanceKm)
{
	if (!bReady)
	{
		return;
	}
	const bool bShowScenery = CameraDistanceKm < SceneryMaxDistanceKm;
	if (bShowScenery != bSceneryVisible)
	{
		bSceneryVisible = bShowScenery;
		for (UHierarchicalInstancedStaticMeshComponent* Part : Scenery)
		{
			Part->SetVisibility(bShowScenery);
		}
		for (ACampaign1851ConstructionSite* Site : Projects)
		{
			if (Site)
			{
				Site->SetActorHiddenInGame(!bShowScenery);
			}
		}
		for (UStaticMeshComponent* Part : FortParts)
		{
			if (Part)
			{
				Part->SetVisibility(bShowScenery);
			}
		}
	}
	if (Lanes)
	{
		Lanes->SetVisibility(bShowScenery);
	}
	if (TownMinorStreetMesh)
	{
		TownMinorStreetMesh->SetVisibility(bShowScenery);
	}
	UpdateHydroVisibility(CameraDistanceKm);
	const bool bShowRoads = CameraDistanceKm < RoadsMaxDistanceKm;
	if (Roads && bShowRoads != bRoadsVisible)
	{
		bRoadsVisible = bShowRoads;
		Roads->SetVisibility(bShowRoads);
		if (Ferries)
		{
			Ferries->SetVisibility(bShowRoads);
		}
	}
	LastCameraDistanceKm = CameraDistanceKm;
	for (int32 i = 0; i < RegimentPieces.Num(); ++i)
	{
		if (RegimentPieces[i])
		{
			// A regiment riding in another's train keeps its piece hidden (UpdateRegimentPiece decides).
			const bool bPassenger = Regiments.IsValidIndex(i) && Regiments[i].Group != 0 && Regiments[i].IsMarching() && Regiments[i].Route[Regiments[i].Leg].bRail
				&& Regiments.ContainsByPredicate([&](const FCampaign1851Regiment& O) { return &O < &Regiments[i] && O.Group == Regiments[i].Group && O.IsMarching() && O.Route[O.Leg].bRail; });
			RegimentPieces[i]->SetVisibility(CameraDistanceKm < 60.f && !bPassenger);
		}
	}
	for (int32 t = 0; t < TroopTrainPieces.Num(); ++t)
	{
		if (TroopTrainPieces[t])
		{
			const int32 Train = t / TrainVehicles;
			TroopTrainPieces[t]->SetVisibility(bSceneryVisible && TroopTrainList.IsValidIndex(Train) && !TroopTrainList[Train].bBoarded);
		}
	}
	for (int32 c = 0; c < RegimentCars.Num(); ++c)
	{
		// A regiment's carriages show only while it rides the train.
		const int32 i = c / 3;
		if (RegimentCars[c])
		{
			RegimentCars[c]->SetVisibility(CameraDistanceKm < 60.f && Regiments.IsValidIndex(i) && Regiments[i].IsMarching() && Regiments[i].Route[Regiments[i].Leg].bRail);
		}
	}
	if (SeasonCollection)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, SeasonCollection, TEXT("ViewKm"), CameraDistanceKm);
	}
	UpdateNetworkVisibility();
	// Close in, the 3D town speaks for itself: the monarchy's dots go (foreign towns have no 3D town).
	CityMarkers->SetVisibility(CameraDistanceKm > CityDotsMinDistanceKm);
	// Close in, the dot floats above the roofs and church towers so the town never hides it.
	const float Lift = FMath::GetMappedRangeValueClamped(FVector2D(40.f, 120.f), FVector2D(24.f, 0.f), CameraDistanceKm);

	// Diameter in km: ~9 px for a 5-10k town on a 1080p screen, growing a little when zoomed in.
	const float Growth = FMath::Lerp(1.8f, 1.f, FMath::GetMappedRangeValueClamped(FVector2D(25.f, 400.f), FVector2D(0.f, 1.f), CameraDistanceKm));
	int32 CityIndex = 0, ForeignIndex = 0;
	for (const FCampaign1851City& City : Cities)
	{
		if (City.bBornholm)
		{
			continue;
		}
		const float DiameterUnits = 0.0042f * CameraDistanceKm * SizeClass(City.Population) * Growth * float(KmToUnits);
		const FTransform Xf(FQuat::Identity, City.World + FVector(0.0, 0.0, DiameterUnits * 0.35 + Lift), FVector(DiameterUnits / 100.f));
		if (City.bForeign)
		{
			ForeignMarkers->UpdateInstanceTransform(ForeignIndex++, Xf, true, false, true);
		}
		else
		{
			CityMarkers->UpdateInstanceTransform(CityIndex++, Xf, true, false, true);
		}
	}
	CityMarkers->MarkRenderStateDirty();
	ForeignMarkers->MarkRenderStateDirty();
}

// ------------------------------------------------------------------ 3D scenery (close zoom)

bool ACampaign1851Map::LoadFeatures()
{
	TArray<uint8> Png;
	if (!FFileHelper::LoadFileToArray(Png, *DataPath(TEXT("Denmark1851_Features.png"))))
	{
		return false;
	}
	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
	TArray<uint8> Raw;
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Png.GetData(), Png.Num()) || !Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
	{
		return false;
	}
	FeaturesW = Wrapper->GetWidth();
	FeaturesH = Wrapper->GetHeight();
	Features.SetNumUninitialized(FeaturesW * FeaturesH);
	FMemory::Memcpy(Features.GetData(), Raw.GetData(), Features.Num() * sizeof(FColor));
	return true;
}

FVector2D ACampaign1851Map::UvFromKm(const FVector2D& Km) const
{
	return FVector2D((Km.X - Extent.XMin) / SizeKm.X, (Km.Y - Extent.YMin) / SizeKm.Y);
}

float ACampaign1851Map::TerrainZ(const FVector2D& Uv) const
{
    if (bUseMapTiles) { return TileHeightAtKm(FVector2D(Extent.XMin + Uv.X * SizeKm.X, Extent.YMin + Uv.Y * SizeKm.Y)); }
	const int32 VX = GridX + 1;
	const double Gx = FMath::Clamp(Uv.X, 0.0, 1.0) * GridX, Gy = FMath::Clamp(Uv.Y, 0.0, 1.0) * GridY;
	const int32 I = FMath::Min(int32(Gx), GridX - 1), J = FMath::Min(int32(Gy), GridY - 1);
	const float Fx = float(Gx - I), Fy = float(Gy - J);
	const float H00 = GridZ[J * VX + I], H10 = GridZ[J * VX + I + 1], H01 = GridZ[(J + 1) * VX + I], H11 = GridZ[(J + 1) * VX + I + 1];
	// Same split as BuildTerrain: (i,j)(i+1,j)(i,j+1) and (i+1,j)(i+1,j+1)(i,j+1).
	return Fx + Fy <= 1.f
		? H00 + Fx * (H10 - H00) + Fy * (H01 - H00)
		: H11 + (1.f - Fx) * (H01 - H11) + (1.f - Fy) * (H10 - H11);
}

FVector ACampaign1851Map::LocalAtKm(const FVector2D& Km) const
{
	const FVector2D Uv = UvFromKm(Km);
	return FVector((Uv.X - 0.5) * SizeKm.X * KmToUnits, -(Uv.Y - 0.5) * SizeKm.Y * KmToUnits, TerrainZ(Uv));
}

bool ACampaign1851Map::IsMonarchyLand(const FVector2D& Km) const
{
	const FVector2D Uv = UvFromKm(Km);
	if (Uv.X < 0.0 || Uv.X >= 1.0 || Uv.Y < 0.0 || Uv.Y >= 1.0)
	{
		return false;
	}
	if (NeighbourOwners.Num() > 0)
	{
		const FVector2D NeighbourLandUv = OwnerUvFromKm(Km);
		const int32 NeighbourLandX = FMath::Clamp(int32(NeighbourLandUv.X * NeighbourOwnerW), 0, NeighbourOwnerW - 1);
		const int32 NeighbourLandY = FMath::Clamp(int32((1.0 - NeighbourLandUv.Y) * NeighbourOwnerH), 0, NeighbourOwnerH - 1);
		const FString* NeighbourLandId = NeighbourOwnerIds.Find(NeighbourOwners[NeighbourLandY * NeighbourOwnerW + NeighbourLandX]);
		if (NeighbourLandId && *NeighbourLandId != TEXT("DK")) { return false; }
	}
	if (Features.Num() == 0) { return false; }
	const int32 X = int32(Uv.X * FeaturesW), Y = int32((1.0 - Uv.Y) * FeaturesH);  // row 0 = north
	return Features[FMath::Clamp(Y, 0, FeaturesH - 1) * FeaturesW + FMath::Clamp(X, 0, FeaturesW - 1)].R > 127;
}

float ACampaign1851Map::Woodland(const FVector2D& Km) const
{
    if (bUseMapTiles && !IsMonarchyLand(Km)) { return TileForestAtKm(Km); }
	const FVector2D Uv = UvFromKm(Km);
	if (Uv.X < 0.0 || Uv.X >= 1.0 || Uv.Y < 0.0 || Uv.Y >= 1.0)
	{
		return bUseMapTiles ? TileForestAtKm(Km) : 0.f;
	}
	if (Features.Num() == 0) { return 0.f; }
	const int32 X = int32(Uv.X * FeaturesW), Y = int32((1.0 - Uv.Y) * FeaturesH);
	return Features[FMath::Clamp(Y, 0, FeaturesH - 1) * FeaturesW + FMath::Clamp(X, 0, FeaturesW - 1)].G / 255.f;
}

void ACampaign1851Map::BuildScenery()
{
	using Campaign1851Scenery::EPiece;
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, SceneryMaterialPath);
	if (!Material)
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|missing %s; run Tools/Campaign/setup_campaign1851.py"), SceneryMaterialPath);
		return;
	}

	constexpr int32 PieceCount = int32(EPiece::Count);

	TArray<FTransform> Items[PieceCount];
	FRandomStream Rng(1851);
	auto Place = [&](EPiece Piece, const FVector2D& Km, float Yaw, float Scale, float ScaleZ = 1.f)
	{
		Items[int32(Piece)].Emplace(FRotator(0.f, Yaw, 0.f), LocalAtKm(Km) - FVector(0.0, 0.0, 0.3), FVector(Scale, Scale, Scale * ScaleZ) * PieceScale);
	};

	// The farm type of the region (user 2026-10-23, approved images): chosen from the place itself, never with a roll of the main
	// random stream, so the rest of the scenery stays where it was. Sources: Docs/Bygninger-DK-Research-2026-10-21.md, section 1.
	auto FarmPieceAt = [this](const FVector2D& FarmKm)
	{
		const FVector2D FarmLatLon = Extent.Projection.Inverse(FarmKm);
		const double FarmLat = FarmLatLon.X, FarmLon = FarmLatLon.Y;
		// A stable number 0..1 for the spot (a few farms of another type in every region).
		const float FarmMix = FMath::Frac(FMath::Sin(float(FarmKm.X) * 12.9898f + float(FarmKm.Y) * 78.233f) * 43758.547f);
		if (HedgeKindAt(FarmKm) == EHedgeKind::Ditch) return FarmLat < 55.0 ? EPiece::FarmHaubarg : EPiece::FarmLanghus;   // the marsh
		if (FarmLat < 54.45) return FarmMix < 0.8f ? EPiece::FarmNiedersachsen : EPiece::FarmTwo;                          // Holstein, Lauenburg
		if (FarmLat > 56.6 && FarmLon < 10.7) return FarmMix < 0.6f ? EPiece::FarmParallel : EPiece::FarmTwo;              // north Jutland
		if (FarmLon < 9.3) return FarmMix < 0.7f ? EPiece::FarmLanghus : EPiece::FarmTwo;                                  // west and south Jutland
		return FarmMix < 0.35f ? EPiece::Farm : FarmMix < 0.7f ? EPiece::FarmThree : EPiece::FarmTwo;                       // the islands, east Jutland, east Schleswig
	};

	// Spatial hash so buildings keep their distance (radii in km).
	struct FFootprint { FVector2D Km; float Radius; };
	TMap<FIntPoint, TArray<FFootprint>> Taken;
	constexpr double Cell = 0.6;
	auto CellOf = [](const FVector2D& Km) { return FIntPoint(FMath::FloorToInt(Km.X / Cell), FMath::FloorToInt(Km.Y / Cell)); };
	auto IsFree = [&](const FVector2D& Km, float Radius)
	{
		Radius *= PieceScale;
		const FIntPoint C = CellOf(Km);
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			for (int32 dx = -1; dx <= 1; ++dx)
			{
				if (const TArray<FFootprint>* List = Taken.Find(C + FIntPoint(dx, dy)))
				{
					for (const FFootprint& F : *List)
					{
						if (FVector2D::DistSquared(F.Km, Km) < FMath::Square(F.Radius + Radius))
						{
							return false;
						}
					}
				}
			}
		}
		return true;
	};
	auto Take = [&](const FVector2D& Km, float Radius) { Taken.FindOrAdd(CellOf(Km)).Add({ Km, Radius * PieceScale }); };

	// ---- Main roads: keep buildings and woods off them (sampled every 100 m).
	auto Resample = [](const TArray<FVector2D>& Line, double StepKm)
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i + 1 < Line.Num(); ++i)
		{
			const double Len = FVector2D::Distance(Line[i], Line[i + 1]);
			const int32 Steps = FMath::Max(1, FMath::CeilToInt(Len / StepKm));
			for (int32 s = 0; s < Steps; ++s)
			{
				Out.Add(FMath::Lerp(Line[i], Line[i + 1], double(s) / Steps));
			}
		}
		if (Line.Num() > 0)
		{
			Out.Add(Line.Last());
		}
		return Out;
	};
	TArray<FVector2D>& RoadSamples = RoadSampleKm;
	RoadSamples.Reset();
	for (const TArray<FVector2D>& Line : RoadLines)
	{
		for (const FVector2D& P : Resample(Line, 0.1))
		{
			Take(P, 0.026f);
			RoadSamples.Add(P);
		}
	}
	// Railways of 1851 likewise (lines the player builds clear their way when work starts).
	for (const FCampaign1851Railway& Railway : Railways)
	{
		for (const FVector2D& P : Resample(Railway.Km, 0.1))
		{
			Take(P, 0.03f);
			RoadSamples.Add(P);
		}
	}

	// ---- Towns: whitewashed and yellow houses with red roofs along a street grid, churches in the core.
	TownStreetLinesKm.Reset();
	struct FTown { FVector2D Km; float RadiusKm; };
	TArray<FTown> Towns;
	TArray<FTown> TownMinorClearings;
	// The 1825 town style (Allan 2026-11-07, after comparing with other games, "tættere huse + træer", "uden stræder", "ja, gør
	// det"): B towns close round the market without drawn ring streets, and a garden tree by the houses in classes A, B and C;
	// the A towns draw only their main streets. -CampaignTownStyleOld gives the layout of 2026-11-05. 1851 is not affected.
	const bool TownStyle2 = !FParse::Param(FCommandLine::Get(), TEXT("CampaignTownStyleOld"));
	TArray<FTransform> TownGardenTrees;
	// Conservative XY circles cover every fitted house mesh, including random scale and roof overhang.
	float TownBuildingRadius = 0.1f;
	float TownChurchRadius = 0.3f;
	float TownMeshRadii[5] = {};
	FVector2D TownMeshMinKm[5], TownMeshMaxKm[5];
	bool TownMeshImported[5] = {};
	// Class C, 1825 only: exaggerated map symbols, sized by fitted XY bounds.
	// PieceScale / KmToUnits = 25 m/model unit: beech crowns 154-267 m,
	// fitted windmill 227 x 316 m (before 0.9-1.1 variation). A house reads ~130 m across.
	struct FTownLayoutClass
	{
		int32 TownMaxPopulation, TownHouses, TownRows;
		float TownHouseRadiusKm, TownChurchRadiusKm, TownPitchKm, TownStreetWidthKm;
		float TownHalfLengthKm, TownRowOffsetKm, TownSquareXKm, TownSquareHalfWidthKm, TownChurchOffsetKm, TownClearingRadiusKm;
	};
	const FTownLayoutClass TownMinorClass = { 5000, 20, 4, 0.080f, 0.120f, 0.162f, 0.024f, 0.43f, 0.095f, 0.420f, 0.050f, 0.280f, 0.70f };
	const FTownLayoutClass TownRegionalClass = { 8000, 48, 6, 0.080f, 0.144f, 0.162f, 0.024f, 0.43f, 0.095f, 0.420f, 0.050f, 0.280f, 0.70f };
	const FTownPopulationLayout TownRegionalPopulation = { 5000, 30, 150, 6 };
	TArray<TArray<FVector2D>> TownRenderedRoads = RoadLines;
	TArray<TArray<FVector2D>> TownMinorStreets;
	const EPiece TownHousePieces[] = { EPiece::TownHouse, EPiece::TownHouseOchre, EPiece::TownHouseTimber, EPiece::MerchantHouse, EPiece::Church };
	const TCHAR* TownHouseModels[] = { TEXT("SM_TownHouse_White"), TEXT("SM_TownHouse_Ochre"), TEXT("SM_TownHouse_Timber"), TEXT("SM_MerchantHouse"), TEXT("SM_Church") };
	for (int32 TownModelIndex = 0; TownModelIndex < 5; ++TownModelIndex)
	{
		UStaticMesh* TownProcedural = Campaign1851Scenery::Build(TownHousePieces[TownModelIndex], Material);
		if (!TownProcedural) continue;
		FBox TownBox = TownProcedural->GetBoundingBox();
		if (!FParse::Param(FCommandLine::Get(), TEXT("Campaign1851OldScenery")))
			if (UStaticMesh* TownModel = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Campaign1851/Scenery/DK/%s.%s"), TownHouseModels[TownModelIndex], TownHouseModels[TownModelIndex])))
			{
				TownMeshImported[TownModelIndex] = true;
				const FBox TownModelBox = TownModel->GetBoundingBox();
				const double TownFit = TownBox.Max.Z / FMath::Max(1.0, TownModelBox.Max.Z);
				TownBox = FBox(TownModelBox.Min * TownFit, TownModelBox.Max * TownFit);
			}
		TownMeshMinKm[TownModelIndex] = FVector2D(TownBox.Min.X, TownBox.Min.Y) * (PieceScale / KmToUnits);
		TownMeshMaxKm[TownModelIndex] = FVector2D(TownBox.Max.X, TownBox.Max.Y) * (PieceScale / KmToUnits);
		const double TownX = FMath::Max(FMath::Abs(TownBox.Min.X), FMath::Abs(TownBox.Max.X));
		const double TownY = FMath::Max(FMath::Abs(TownBox.Min.Y), FMath::Abs(TownBox.Max.Y));
		const float TownMeshRadius = float(FMath::Sqrt(TownX * TownX + TownY * TownY) * PieceScale / KmToUnits);
		TownMeshRadii[TownModelIndex] = FMath::Max(TownMeshRadius, 0.0001f);
		if (TownModelIndex == 4) TownChurchRadius = FMath::Max(TownChurchRadius, TownMeshRadius * 1.25f);
		else TownBuildingRadius = FMath::Max(TownBuildingRadius, TownMeshRadius * 1.05f);
	}
	const auto TownRoadTaken = Taken;
	auto TownBuild = [&](bool TownLegacy)
	{
		for (FCampaign1851City& City : Cities)
		{
			if (City.bForeign || City.bBornholm)
			{
				continue;
			}
			const FVector2D Centre = Extent.Projection.Forward(City.Lat, City.Lon);
			const float Pop = float(City.Population);
			const float Radius = FMath::Clamp(0.75f * FMath::Sqrt(Pop / 1000.f), 0.9f, 6.5f);
			Towns.Add({ Centre, Radius });

			// Military plot: beside a main road just outside the town, on the flattest ground available.
			if (TownLegacy && Pop >= 2500.f)
			{
				float BestScore = TNumericLimits<float>::Max();
				for (int32 s = 1; s + 1 < RoadSamples.Num(); s += 2)
				{
					const double D = FVector2D::Distance(RoadSamples[s], Centre);
					const FVector2D Along = (RoadSamples[s + 1] - RoadSamples[s - 1]).GetSafeNormal();
					if (D < Radius + 0.3 || D > Radius + 1.3 || Along.IsNearlyZero())
					{
						continue;
					}
					for (float Side : { -1.f, 1.f })
					{
						const FVector2D Normal = FVector2D(-Along.Y, Along.X) * Side;
						const FVector2D P = RoadSamples[s] + Normal * 0.6;   // the parade ground reaches back towards the road
						bool bOk = IsMonarchyLand(P) && IsFree(P, 0.2f);
						float ZMin = TNumericLimits<float>::Max(), ZMax = -ZMin;
						for (float A : { -0.42f, 0.42f })
						{
							for (float B : { -0.3f, 0.45f })
							{
								const FVector2D Q = P + Along * A - Normal * B;
								bOk &= IsMonarchyLand(Q);
								const float Z = TerrainZ(UvFromKm(Q));
								ZMin = FMath::Min(ZMin, Z);
								ZMax = FMath::Max(ZMax, Z);
							}
						}
						const float Score = (ZMax - ZMin) + 4.f * float(FMath::Abs(D - Radius - 0.6));
						if (bOk && Score < BestScore)
						{
							BestScore = Score;
							City.bHasPlot = true;
							City.PlotKm = P;
							// Local +Y (the gate) must point back at the road; world Y is south.
							const FVector2D Facing = -Normal;
							City.PlotYaw = FMath::RadiansToDegrees(FMath::Atan2(-Facing.X, -Facing.Y));
						}
					}
				}
				// No road site (Rønne has few roads; Altona's own ground lies outside the land mask, 2026-11-05): the flattest site
				// on a ring round the town, with the gate towards the town.
				for (float GarrisonRing = Radius + 0.6f; !City.bHasPlot && GarrisonRing <= Radius + 6.f; GarrisonRing += 0.3f)
				{
					for (int32 GarrisonStep = 0; GarrisonStep < 24; ++GarrisonStep)
					{
						const float GarrisonAngle = GarrisonStep * UE_TWO_PI / 24.f;
						const FVector2D GarrisonNormal(FMath::Cos(GarrisonAngle), FMath::Sin(GarrisonAngle));
						const FVector2D GarrisonAlong(-GarrisonNormal.Y, GarrisonNormal.X);
						const FVector2D P = Centre + GarrisonNormal * GarrisonRing;
						bool bOk = IsMonarchyLand(P) && IsFree(P, 0.2f);
						float ZMin = TNumericLimits<float>::Max(), ZMax = -ZMin;
						for (float A : { -0.42f, 0.42f })
							for (float B : { -0.3f, 0.45f })
							{
								const FVector2D Q = P + GarrisonAlong * A - GarrisonNormal * B;
								bOk &= IsMonarchyLand(Q);
								const float Z = TerrainZ(UvFromKm(Q));
								ZMin = FMath::Min(ZMin, Z);
								ZMax = FMath::Max(ZMax, Z);
							}
						const float Score = ZMax - ZMin;
						if (bOk && Score < BestScore)
						{
							BestScore = Score;
							City.bHasPlot = true;
							City.PlotKm = P;
							const FVector2D Facing = -GarrisonNormal;
							City.PlotYaw = FMath::RadiansToDegrees(FMath::Atan2(-Facing.X, -Facing.Y));
						}
					}
				}
				if (City.bHasPlot)
				{
					Take(City.PlotKm, 0.2f);   // the whole garrison complex: barracks, flanking modules, infirmary
					const FVector2D Gate = FVector2D(-FMath::Sin(FMath::DegreesToRadians(City.PlotYaw)), -FMath::Cos(FMath::DegreesToRadians(City.PlotYaw)));
					Take(City.PlotKm + Gate * 0.22, 0.09f);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|no garrison plot found for %s"), *City.Name);
				}
			}

			if (!TownLegacy && City.bHasPlot)
			{
				Take(City.PlotKm, 0.2f);
				const FVector2D TownGate(-FMath::Sin(FMath::DegreesToRadians(City.PlotYaw)), -FMath::Cos(FMath::DegreesToRadians(City.PlotYaw)));
				Take(City.PlotKm + TownGate * 0.22, 0.09f);
			}

			if (!TownLegacy && ActiveScenario().Year == 1825 && !City.bCapital && Pop < TownMinorClass.TownMaxPopulation)
			{
				const FTownLayoutClass& TownClass = TownMinorClass;
				// Follow the nearest incoming road instead of choosing an unrelated random grid.
				FVector2D TownAxis(1.0, 0.0);
				double TownNearest = TNumericLimits<double>::Max();
				for (const TArray<FVector2D>& TownRoad : RoadLines)
					for (int32 TownI = 1; TownI < TownRoad.Num(); ++TownI)
					{
						const double TownDistance = FVector2D::DistSquared(TownRoad[TownI], Centre);
						if (TownDistance < TownNearest && !TownRoad[TownI].Equals(TownRoad[TownI - 1]))
						{
							TownNearest = TownDistance;
							TownAxis = (TownRoad[TownI] - TownRoad[TownI - 1]).GetSafeNormal();
						}
					}
				FVector2D TownNormal(-TownAxis.Y, TownAxis.X);
				const FVector2D TownRoadAxis = TownAxis;
				float TownLayoutYawOffset = 0;
				// Plan in street coordinates. Bounds include the imported mesh's final -90 degree fit.
				// Preserve the previous uniform scale; only orientation and spacing change.
				struct FTownPlot
				{
					int32 Model;
					FVector2D Mid, Half, Pivot;
					float Facing, Scale;
				};
				TArray<FTownPlot> TownPlan;
				const float TownYaw = -FMath::RadiansToDegrees(FMath::Atan2(TownAxis.Y, TownAxis.X));
				auto TownMakePlot = [&](int32 TownModelIndex, const FVector2D& TownMid, bool TownFlip)
				{
					const float TownScale = (TownModelIndex == 4 ? TownClass.TownChurchRadiusKm : TownClass.TownHouseRadiusKm) / TownMeshRadii[TownModelIndex];
					const FVector2D TownSize = (TownMeshMaxKm[TownModelIndex] - TownMeshMinKm[TownModelIndex]) * TownScale;
					const FVector2D TownMeshMid = (TownMeshMaxKm[TownModelIndex] + TownMeshMinKm[TownModelIndex]) * (TownScale * 0.5);
					const bool TownLongX = TownSize.X >= TownSize.Y;
					const float TownTurn = (TownLongX ? 0.f : 90.f) + (TownFlip ? 180.f : 0.f);
					const double TownCos = FMath::Cos(FMath::DegreesToRadians(TownTurn));
					const double TownSin = FMath::Sin(FMath::DegreesToRadians(TownTurn));
					const FVector2D TownPivotOffset(TownCos * TownMeshMid.X - TownSin * TownMeshMid.Y, -TownSin * TownMeshMid.X - TownCos * TownMeshMid.Y);
					return FTownPlot{ TownModelIndex, TownMid, (TownLongX ? TownSize : FVector2D(TownSize.Y, TownSize.X)) * 0.5,
						TownMid - TownPivotOffset, TownYaw + TownTurn + (TownMeshImported[TownModelIndex] ? 90.f : 0.f), TownScale };
				};
				// Church and square are reserved before either continuous street front.
				const FTownPlot TownChurchShape = TownMakePlot(4, FVector2D::ZeroVector, false);
				double TownRowLength = 0;
				const int32 TownColumns = TownClass.TownHouses / TownClass.TownRows;
				for (int32 TownColumn = 0; TownColumn < TownColumns; ++TownColumn)
				{
					const FTownPlot TownShape = TownMakePlot(TownColumn % 3, FVector2D::ZeroVector, false);
					TownRowLength += TownShape.Half.X * 2;
				}
				const double TownSquareHalf = FMath::Max(0.045, TownChurchShape.Half.X);
				const double TownSquareX = TownRowLength * 0.5 + TownSquareHalf + TownClass.TownStreetWidthKm * 0.5;
				TownPlan.Add(TownMakePlot(4, FVector2D(TownSquareX, TownSquareHalf + TownClass.TownStreetWidthKm * 0.5 + TownChurchShape.Half.Y), false));
				for (int32 TownRow = 0; TownRow < TownClass.TownRows; ++TownRow)
				{
					double TownCursor = -TownRowLength * 0.5;
					for (int32 TownColumn = 0; TownColumn < TownColumns; ++TownColumn)
					{
						FTownPlot TownPlot = TownMakePlot(TownColumn % 3, FVector2D::ZeroVector, TownRow % 2 == 0);
						const double TownSide = TownRow % 2 == 0 ? -1.0 : 1.0;
						TownPlot = TownMakePlot(TownColumn % 3, FVector2D(TownCursor + TownPlot.Half.X,
							TownSide * (TownClass.TownStreetWidthKm * 0.5 + TownPlot.Half.Y) + (TownRow / 2) * 0.40), TownRow % 2 == 0);
						TownPlan.Add(TownPlot);
						TownCursor += TownPlot.Half.X * 2; // Adjacent mesh bounds touch; no circular safety gap.
					}
				}
				FVector2D TownOrigin = Centre;
				auto TownPosition = [&](const FVector2D& TownLocal) { return TownOrigin + TownAxis * TownLocal.X + TownNormal * TownLocal.Y; };
				auto TownLocal = [&](const FVector2D& TownPoint) { const FVector2D TownD = TownPoint - TownOrigin; return FVector2D(FVector2D::DotProduct(TownD, TownAxis), FVector2D::DotProduct(TownD, TownNormal)); };
				// Segment against an expanded rectangular footprint (street coordinates), including endpoints.
				auto TownSegmentClear = [&](const FTownPlot& TownPlot, const FVector2D& TownA, const FVector2D& TownB, double TownWidth)
				{
					const FVector2D TownStart = TownLocal(TownA) - TownPlot.Mid;
					const FVector2D TownDelta = TownLocal(TownB) - TownLocal(TownA);
					double TownEnter = 0, TownExit = 1;
					for (int32 TownDim = 0; TownDim < 2; ++TownDim)
					{
						const double TownLimit = TownPlot.Half[TownDim] + TownWidth;
						if (FMath::Abs(TownDelta[TownDim]) < 1.e-10)
						{
							if (FMath::Abs(TownStart[TownDim]) > TownLimit) return true;
						}
						else
						{
							const double TownT0 = (-TownLimit - TownStart[TownDim]) / TownDelta[TownDim];
							const double TownT1 = (TownLimit - TownStart[TownDim]) / TownDelta[TownDim];
							TownEnter = FMath::Max(TownEnter, FMath::Min(TownT0, TownT1));
							TownExit = FMath::Min(TownExit, FMath::Max(TownT0, TownT1));
							if (TownEnter > TownExit) return true;
						}
					}
					return false;
				};
				const double TownHalfLength = TownSquareX + TownSquareHalf;
				const double TownClearing = FMath::Sqrt(TownHalfLength * TownHalfLength + FMath::Square(0.65 + TownChurchShape.Half.Y * 2)) + 0.13;
				auto TownPlotClear = [&](const FTownPlot& TownPlot)
				{
					// Sample the entire footprint on a <=20 m grid, rather than its enclosing circle.
					const int32 TownNX = FMath::Max(1, FMath::CeilToInt(TownPlot.Half.X * 100));
					const int32 TownNY = FMath::Max(1, FMath::CeilToInt(TownPlot.Half.Y * 100));
					for (int32 TownX = 0; TownX <= TownNX; ++TownX)
						for (int32 TownY = 0; TownY <= TownNY; ++TownY)
						{
							const FVector2D TownTest = TownPosition(TownPlot.Mid + FVector2D(TownPlot.Half.X * (2.0 * TownX / TownNX - 1), TownPlot.Half.Y * (2.0 * TownY / TownNY - 1)));
							if (!IsMonarchyLand(TownTest) || IsFreshWater(TownTest, 0.003f)) return false;
						}
					// Main roads are clipped/rerouted outside the complete footprint plus 130 m clearance.
					for (const TArray<FVector2D>& TownLane : LaneLinesKm)
						for (int32 TownI = 1; TownI < TownLane.Num(); ++TownI)
							if (!TownSegmentClear(TownPlot, TownLane[TownI - 1], TownLane[TownI], 0.0375)) return false;
					for (const FCampaign1851Railway& TownRail : Railways)
						for (int32 TownI = 1; TownI < TownRail.Km.Num(); ++TownI)
							if (!TownSegmentClear(TownPlot, TownRail.Km[TownI - 1], TownRail.Km[TownI], 0.1)) return false;
					// The square itself may hold a bridge end (the bridge in the middle of Randers and København, 2026-11-11).
					if (TownPlot.Model >= 0)
						for (const FCampaign1851Bridge& TownBridge : Bridges)
							if (!TownSegmentClear(TownPlot, TownBridge.EndA, TownBridge.EndB, 0.125)) return false;
					if (City.bHasPlot && !TownSegmentClear(TownPlot, City.PlotKm, City.PlotKm, 0.25)) return false;
					// Saved sites mask occupancy later; they must not change the candidate sequence.
					return true;
				};
				const FTownPlot TownSquare = { -1, FVector2D(TownSquareX, 0), FVector2D(TownSquareHalf, TownSquareHalf), FVector2D::ZeroVector, 0, 1 };
				bool TownFound = false;
				TArray<FTownPlot> TownBestPlan;
				FVector2D TownBestOrigin = Centre, TownBestAxis = TownAxis;
				float TownBestYawOffset = 0;
				// Nearest complete inland site, at most 1 km from the unchanged town marker.
				for (int32 TownRing = 0; TownRing <= 20 && !TownFound; ++TownRing)
					for (int32 TownDirection = 0; TownDirection < (TownRing == 0 ? 1 : TownRing * 8) && !TownFound; ++TownDirection)
					{
						const double TownAngle = UE_TWO_PI * TownDirection / FMath::Max(1, TownRing * 8);
						TownOrigin = Centre + FVector2D(FMath::Cos(TownAngle), FMath::Sin(TownAngle)) * (TownRing * 0.05);
						// Prefer the road alignment; try the opposite end and perpendicular streets if coast blocks it.
						for (float TownTurn : { 0.f, 180.f, 90.f, 270.f })
						{
							const double TownCos = FMath::Cos(FMath::DegreesToRadians(TownTurn));
							const double TownSin = FMath::Sin(FMath::DegreesToRadians(TownTurn));
							TownAxis = FVector2D(TownRoadAxis.X * TownCos - TownRoadAxis.Y * TownSin, TownRoadAxis.X * TownSin + TownRoadAxis.Y * TownCos);
							TownNormal = FVector2D(-TownAxis.Y, TownAxis.X);
							TownLayoutYawOffset = -TownTurn;
							if (!TownPlotClear(TownPlan[0]) || !TownPlotClear(TownSquare)) continue;
							TArray<FTownPlot> TownCandidatePlan;
							TownCandidatePlan.Add(TownPlan[0]);
							for (int32 TownI = 1; TownI < TownPlan.Num(); ++TownI)
								if (TownPlotClear(TownPlan[TownI])) TownCandidatePlan.Add(TownPlan[TownI]);
							TownFound = TownCandidatePlan.Num() == TownPlan.Num();
							if (TownCandidatePlan.Num() > TownBestPlan.Num())
							{
								TownBestPlan = MoveTemp(TownCandidatePlan);
								TownBestOrigin = TownOrigin;
								TownBestAxis = TownAxis;
								TownBestYawOffset = TownLayoutYawOffset;
							}
							if (TownFound) break;
						}
					}
				if (!TownFound)
				{
					// An obstructed outer row must never cost the town its valid church and square.
					if (TownBestPlan.IsEmpty())
					{
						UE_LOG(LogTemp, Error, TEXT("CAMPAIGN-1851|scenery|town %s houses=0 church=0 plinths=0 reserved-plots=0 edge-sites=0 pop=%d street-points=0 overlaps=0 extent-m=0 no land site for church/square within 1 km"), *City.Name, City.Population);
						continue;
					}
					TownPlan = MoveTemp(TownBestPlan);
					TownOrigin = TownBestOrigin;
					TownAxis = TownBestAxis;
					TownNormal = FVector2D(-TownAxis.Y, TownAxis.X);
					TownLayoutYawOffset = TownBestYawOffset;
					UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|scenery|town %s church/square retained; only %d/%d house plots clear within 1 km"), *City.Name, TownPlan.Num() - 1, TownClass.TownHouses);
				}
				TownMinorClearings.Add({ TownOrigin, float(TownClearing) });
				TArray<TArray<FVector2D>> TownConnections;
				TArray<TArray<FVector2D>> TownClippedRoads;
				auto TownConnect = [&](const FVector2D& TownPoint)
				{
					const FVector2D TownEntry = TownLocal(TownPoint);
					const double TownGateX = TownEntry.X < 0 ? -TownRowLength * 0.5 - 0.03 : TownHalfLength + 0.03;
					TownConnections.Add({ TownPoint, TownPosition(FVector2D(TownGateX, TownEntry.Y)), TownPosition(FVector2D(TownGateX, 0)), TownPosition(FVector2D(TownEntry.X < 0 ? -TownRowLength * 0.5 : TownHalfLength, 0)) });
				};
				for (const TArray<FVector2D>& TownRoad : TownRenderedRoads)
				{
					// Faster start (2026-11-05): a road that never comes near this town is kept as it is instead of being
					// resampled every 20 m for every small town (the ribbons resample all roads every 200 m when drawn).
					bool TownNear = false;
					for (int32 TownI = 1; TownI < TownRoad.Num() && !TownNear; ++TownI)
					{
						const FVector2D TownDelta = TownRoad[TownI] - TownRoad[TownI - 1];
						const double TownLengthSq = TownDelta.SizeSquared();
						const double TownT = TownLengthSq > 0.0 ? FMath::Clamp(FVector2D::DotProduct(TownOrigin - TownRoad[TownI - 1], TownDelta) / TownLengthSq, 0.0, 1.0) : 0.0;
						TownNear = FVector2D::Distance(TownOrigin, TownRoad[TownI - 1] + TownDelta * TownT) < TownClearing + 0.1;
					}
					if (!TownNear) { if (TownRoad.Num() > 1) TownClippedRoads.Add(TownRoad); continue; }
					TArray<FVector2D> TownRun;
					bool TownWasInside = false;
					for (const FVector2D& TownPoint : Resample(TownRoad, 0.02))
					{
						const bool TownInside = FVector2D::Distance(TownPoint, TownOrigin) < TownClearing;
						if (!TownInside)
						{
							if (TownWasInside) TownConnect(TownPoint);
							TownRun.Add(TownPoint);
						}
						else if (TownRun.Num() > 0)
						{
							TownConnect(TownRun.Last());
							if (TownRun.Num() > 1) TownClippedRoads.Add(TownRun);
							TownRun.Reset();
						}
						TownWasInside = TownInside;
					}
					if (TownRun.Num() > 1) TownClippedRoads.Add(TownRun);
				}
				TownRenderedRoads = MoveTemp(TownClippedRoads);
				TownConnections.Add({ TownPosition(FVector2D(-TownRowLength * 0.5, 0)), TownPosition(FVector2D(TownHalfLength, 0)) });
				TownConnections.Add({ TownPosition(FVector2D(-TownRowLength * 0.5, 0.40)), TownPosition(FVector2D(TownHalfLength + 0.03, 0.40)), TownPosition(FVector2D(TownHalfLength + 0.03, 0)) });
				// A closed square is always emitted with the church, before the houses.
				TownConnections.Add({ TownPosition(TownSquare.Mid + FVector2D(-TownSquareHalf, -TownSquareHalf)),
					TownPosition(TownSquare.Mid + FVector2D(TownSquareHalf, -TownSquareHalf)),
					TownPosition(TownSquare.Mid + FVector2D(TownSquareHalf, TownSquareHalf)),
					TownPosition(TownSquare.Mid + FVector2D(-TownSquareHalf, TownSquareHalf)),
					TownPosition(TownSquare.Mid + FVector2D(-TownSquareHalf, -TownSquareHalf)) });
				const int32 TownCityIndex = int32(&City - Cities.GetData());
				// Shared edge sequence: street ends first, then additional parallel street fronts.
				TArray<FVector2D>& TownEdges = TownEdgePlots.FindOrAdd(TownCityIndex);
				for (int32 TownRing = 0; TownRing < 32; ++TownRing)
					for (int32 TownSide : { -1, 1 })
					{
						const double TownEnd = TownSide < 0 ? -TownRowLength * 0.5 : TownHalfLength;
						const FVector2D TownMid(TownEnd + TownSide * (0.5 + (TownRing / 4) * 0.7), (TownRing % 4) * 0.7);
						const FTownPlot TownReserve = { -1, TownMid, FVector2D(0.3, 0.3), FVector2D::ZeroVector, 0, 1 };
						if (!TownPlotClear(TownReserve)) continue;
						TownEdges.Add(TownPosition(TownMid));
						TownMinorClearings.Add({ TownPosition(TownMid), 0.45f });
						double TownCursor = TownMid.X - 0.25;
						for (int32 TownHouse = 0; TownHouse < 4; ++TownHouse)
						{
							const FTownPlot TownShape = TownMakePlot(TownHouse % 3, FVector2D::ZeroVector, false);
							const FTownPlot TownGrowth = TownMakePlot(TownHouse % 3, FVector2D(TownCursor + TownShape.Half.X, TownMid.Y + TownShape.Half.Y), false);
							if (TownPlotClear(TownGrowth)) TownPlan.Add(TownGrowth);
							TownCursor += TownShape.Half.X * 2;
						}
						const TArray<TArray<FVector2D>> TownStreet = { { TownPosition(FVector2D(TownEnd, 0)), TownPosition(FVector2D(TownEnd, TownMid.Y - TownClass.TownStreetWidthKm * 0.5)), TownPosition(FVector2D(TownMid.X + TownSide * 0.35, TownMid.Y - TownClass.TownStreetWidthKm * 0.5)) } };
						UStaticMeshComponent* TownStreetMesh = BuildRibbons(TownStreet, TownClass.TownStreetWidthKm, Srgb(160, 126, 88), *FString::Printf(TEXT("TownEdgeStreet_%d_%d"), TownCityIndex, TownEdgeStreets.FindOrAdd(TownCityIndex).Num()), Material);
						if (TownStreetMesh) TownStreetMesh->SetVisibility(false);
						TownEdgeStreets.FindOrAdd(TownCityIndex).Add(TownStreetMesh);
					}

				// Unreal world distances are centimetres; KmToUnits is only the compressed XY map scale.
				const double TownWorldZScale = FMath::Max(1.e-6, double(FMath::Abs(GetActorScale3D().Z)));
				const double TownSlopeThreshold = 50.0 / TownWorldZScale; // 0.5 m in world space.
				const double TownStoneOverlap = 1.0 / TownWorldZScale; // 1 cm at either end.
				int32 TownPlinthCount = 0;
				int32 TownVisibleHouses = 0;
				UStaticMesh* TownCube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
				UMaterialInstanceDynamic* TownStone = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, FlatColourPath), this);
				if (TownStone) TownStone->SetVectorParameterValue(TEXT("Color"), Srgb(205, 203, 195));
				for (int32 TownIndex = 0; TownIndex < TownPlan.Num(); ++TownIndex)
				{
					const FTownPlot& TownPlot = TownPlan[TownIndex];
					TownHouseReservations.Add({ TownPosition(TownPlot.Mid), TownPlot.Half.Size() });
					double TownLow = TNumericLimits<double>::Max(), TownHigh = -TownLow;
					for (int32 TownX : { -1, 1 }) for (int32 TownY : { -1, 1 })
					{
						const double TownZ = LocalAtKm(TownPosition(TownPlot.Mid + FVector2D(TownX * TownPlot.Half.X, TownY * TownPlot.Half.Y))).Z;
						TownLow = FMath::Min(TownLow, TownZ);
						TownHigh = FMath::Max(TownHigh, TownZ);
					}
					const int32 TownOrdinal = TownIndex - 1;
					const bool TownActive = TownOrdinal < FMath::Max(18, 18 + City.Population / 1000);
					if (TownIndex > 0 && TownActive) ++TownVisibleHouses;
					auto TownAddVisual = [&](UStaticMesh* TownMesh, UMaterialInterface* TownMaterial, const FTransform& TownTransform)
					{
						if (!TownMesh) return;
						UHierarchicalInstancedStaticMeshComponent* TownComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
						TownComponent->SetupAttachment(RootComponent);
						TownComponent->SetStaticMesh(TownMesh);
						if (TownMaterial) TownComponent->SetMaterial(0, TownMaterial);
						TownComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						TownComponent->SetVisibility(false);
						TownComponent->RegisterComponent();
						if (TownActive) TownComponent->AddInstance(TownTransform);
						Scenery.Add(TownComponent);
						TownVisualPlots.Add({ TownCityIndex, TownOrdinal, TownPosition(TownPlot.Mid), TownPlot.Half.Size(), TownActive, TownComponent, TownTransform });
					};
					UStaticMesh* TownMesh = Campaign1851Scenery::Build(TownHousePieces[TownPlot.Model], Material);
					double TownFit = 1;
					float TownFacing = TownPlot.Facing + TownLayoutYawOffset;
					if (TownMeshImported[TownPlot.Model])
					{
						UStaticMesh* TownImported = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Campaign1851/Scenery/DK/%s.%s"), TownHouseModels[TownPlot.Model], TownHouseModels[TownPlot.Model]));
						TownFit = TownMesh->GetBoundingBox().Max.Z / FMath::Max(1.0, TownImported->GetBoundingBox().Max.Z);
						TownMesh = TownImported;
						TownFacing -= 90.f;
					}
					FVector TownBase = LocalAtKm(TownPosition(TownPlot.Pivot));
					TownBase.Z = TownHigh - TownMesh->GetBoundingBox().Min.Z * TownFit * TownPlot.Scale * PieceScale;
					TownAddVisual(TownMesh, nullptr, FTransform(FRotator(0, TownFacing, 0), TownBase, FVector(TownFit * TownPlot.Scale * PieceScale)));
					if (TownStyle2 && TownIndex > 0 && TownActive)
					{
						// A garden tree by the house (class C), where its trunk touches no house and no street.
						const double TownTreeX = TownPlot.Half.X + 0.02, TownTreeY = TownPlot.Half.Y + 0.02;
						for (const FVector2D& TownTreeOffset : { FVector2D(0, TownTreeY), FVector2D(0, -TownTreeY), FVector2D(TownTreeX, TownTreeY), FVector2D(-TownTreeX, -TownTreeY) })
						{
							const FTownPlot TownTreePlot = { 0, TownPlot.Mid + TownTreeOffset, FVector2D(0.012, 0.012), TownPlot.Mid + TownTreeOffset, 0.f, 1.f };
							bool TownTreeClear = TownPlotClear(TownTreePlot);
							for (const FTownPlot& TownOther : TownPlan)
								if (FMath::Abs(TownOther.Mid.X - TownTreePlot.Mid.X) < TownOther.Half.X + 0.012 && FMath::Abs(TownOther.Mid.Y - TownTreePlot.Mid.Y) < TownOther.Half.Y + 0.012) TownTreeClear = false;
							for (const TArray<FVector2D>& TownStreet : TownConnections)
								for (int32 TownI = 1; TownTreeClear && TownI < TownStreet.Num(); ++TownI)
									if (!TownSegmentClear(TownTreePlot, TownStreet[TownI - 1], TownStreet[TownI], TownClass.TownStreetWidthKm * 0.5)) TownTreeClear = false;
							if (!TownTreeClear) continue;
							TownGardenTrees.Emplace(FRotator(0.f, float(TownIndex * 37 % 360), 0.f), LocalAtKm(TownPosition(TownTreePlot.Mid)) - FVector(0.0, 0.0, 0.3), FVector(0.45f + 0.06f * (TownIndex % 3)) * PieceScale);
							break;
						}
					}
					if (TownCube && TownHigh - TownLow >= TownSlopeThreshold)
					{
						FVector TownPlinthBase = LocalAtKm(TownPosition(TownPlot.Mid));
						TownPlinthBase.Z = (TownHigh + TownLow) * 0.5;
						TownAddVisual(TownCube, TownStone, FTransform(FRotator(0, TownYaw + TownLayoutYawOffset, 0), TownPlinthBase,
							FVector(TownPlot.Half.X * 2 * KmToUnits / 100, TownPlot.Half.Y * 2 * KmToUnits / 100, (TownHigh - TownLow + 2 * TownStoneOverlap) / 100)));
						if (TownActive) ++TownPlinthCount;
					}
				}
				TArray<FVector2D>& TownReserved = TownReservedPlots.FindOrAdd(TownCityIndex);
				TownUpdatePlots();
				TownMinorStreets.Append(TownConnections);
				int32 TownStreetPoints = 0;
				for (const TArray<FVector2D>& TownLine : TownConnections) TownStreetPoints += TownLine.Num();
				// Diameter in geographic metres, from occupied footprints only (not the growth bank).
				TArray<FVector2D> TownBuiltCorners;
				for (int32 TownIndex = 0; TownIndex < TownPlan.Num(); ++TownIndex)
				{
					bool TownOccupied = false;
					for (const FTownVisualPlot& TownVisual : TownVisualPlots)
						if (TownVisual.TownCity == TownCityIndex && TownVisual.TownOrdinal == TownIndex - 1 && TownVisual.TownActive) { TownOccupied = true; break; }
					if (!TownOccupied) continue;
					const FTownPlot& TownPlot = TownPlan[TownIndex];
					for (int32 TownX : { -1, 1 }) for (int32 TownY : { -1, 1 })
						TownBuiltCorners.Add(TownPosition(TownPlot.Mid + FVector2D(TownX * TownPlot.Half.X, TownY * TownPlot.Half.Y)));
				}
				int32 TownPlacedSites = 0;
				for (const ACampaign1851ConstructionSite* TownSite : Projects)
					if (TownSite && !TownSite->IsGarrison() && TownSite->GetCityIndex() == TownCityIndex && TownSite->GetModule(0).Rule == ECampaign1851PlotRule::InTown)
					{
						++TownPlacedSites;
						for (int32 TownX : { -1, 1 }) for (int32 TownY : { -1, 1 })
							TownBuiltCorners.Add(TownSite->PlotKm + FVector2D(TownX * TownSite->PlotRadiusKm, TownY * TownSite->PlotRadiusKm));
					}
				double TownExtent = 0;
				for (const FVector2D& TownA : TownBuiltCorners)
					for (const FVector2D& TownB : TownBuiltCorners)
						TownExtent = FMath::Max(TownExtent, FVector2D::Distance(TownA, TownB));
				UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|scenery|town %s houses=%d church=1 plinths=%d reserved-plots=%d edge-sites=%d pop=%d street-points=%d overlaps=0 extent-m=%.0f shift-m=%.0f"), *City.Name, TownVisibleHouses, TownPlinthCount, TownReserved.Num(), TownPlacedSites, City.Population, TownStreetPoints, TownExtent * 1000.0, FVector2D::Distance(TownOrigin, Centre) * 1000.0);
				continue;
			}

			// Class B: roads converge on the market; fixed, town-index-seeded parcels grow outwards.
			// Class A and København get the same close layout as class B in the 1825 town style (Allan 2026-11-08, "ja, gør det").
			if (!TownLegacy && ActiveScenario().Year == 1825 && ((!City.bCapital && Pop >= TownMinorClass.TownMaxPopulation && Pop < TownRegionalClass.TownMaxPopulation)
				|| (TownStyle2 && (City.bCapital || Pop >= TownRegionalClass.TownMaxPopulation))))
			{
				const FTownLayoutClass& TownClass = TownRegionalClass;
				const int32 TownCityIndex = int32(&City - Cities.GetData());
				FRandomStream TownLayoutRandom(1825 + TownCityIndex * 7919);
				// København: 200 houses and one more per 10,000 people over 5,000 (about 210), not one per 150 (over 700).
				const FTownPopulationLayout TownBlockPopulation = City.bCapital ? FTownPopulationLayout{ 5000, 200, 10000, 6 } : TownRegionalPopulation;
				const bool TownBlockLarge = City.bCapital || Pop >= TownRegionalClass.TownMaxPopulation;
				TownPopulationLayouts.Add(TownCityIndex, TownBlockPopulation);
				FVector2D TownOrigin = Centre;
				auto TownPosition = [&](const FVector2D& TownPoint) { return TownOrigin + TownPoint; };
				struct FTownPlot
				{
					int32 TownModel;
					FVector2D TownMid, TownHalf, TownPivot;
					float TownFacing, TownScale;
					double TownAngle;
				};
				TArray<FTownPlot> TownPlan;
				auto TownRotate = [](const FVector2D& TownPoint, double TownAngle)
				{
					return FVector2D(TownPoint.X * FMath::Cos(TownAngle) - TownPoint.Y * FMath::Sin(TownAngle),
						TownPoint.X * FMath::Sin(TownAngle) + TownPoint.Y * FMath::Cos(TownAngle));
				};
				auto TownCorner = [&](const FTownPlot& TownPlot, const FVector2D& TownOffset)
				{
					return TownPlot.TownMid + TownRotate(TownOffset, TownPlot.TownAngle);
				};
				auto TownMakePlot = [&](int32 TownModel, const FVector2D& TownMid, double TownAngle, bool TownFlip)
				{
					const float TownScale = (TownModel == 4 ? TownClass.TownChurchRadiusKm : TownClass.TownHouseRadiusKm * (TownModel == 3 ? 1.2f : 1.f)) / TownMeshRadii[TownModel];
					const FVector2D TownSize = (TownMeshMaxKm[TownModel] - TownMeshMinKm[TownModel]) * TownScale;
					const bool TownLongX = TownSize.X >= TownSize.Y;
					const double TownTurn = (TownLongX ? 0.0 : UE_PI * 0.5) + (TownFlip ? UE_PI : 0.0);
					const FVector2D TownMeshMid = (TownMeshMaxKm[TownModel] + TownMeshMinKm[TownModel]) * (TownScale * 0.5);
					// Mesh coordinates use world Y; geographic Y runs in the opposite direction.
					const FVector2D TownOffset = TownRotate(FVector2D(TownMeshMid.X, -TownMeshMid.Y), TownAngle - TownTurn);
					return FTownPlot{ TownModel, TownMid, (TownLongX ? TownSize : FVector2D(TownSize.Y, TownSize.X)) * 0.5,
						TownMid - TownOffset, float(FMath::RadiansToDegrees(TownTurn - TownAngle)) + (TownMeshImported[TownModel] ? 90.f : 0.f), TownScale, TownAngle };
				};
				auto TownSegmentClear = [&](const FTownPlot& TownPlot, const FVector2D& TownA, const FVector2D& TownB, double TownWidth)
				{
					const FVector2D TownStart = TownRotate(TownA - TownPosition(TownPlot.TownMid), -TownPlot.TownAngle);
					const FVector2D TownDelta = TownRotate(TownB - TownA, -TownPlot.TownAngle);
					double TownEnter = 0, TownExit = 1;
					for (int32 TownDim = 0; TownDim < 2; ++TownDim)
					{
						const double TownLimit = TownPlot.TownHalf[TownDim] + TownWidth;
						if (FMath::Abs(TownDelta[TownDim]) < 1.e-10)
						{
							if (FMath::Abs(TownStart[TownDim]) > TownLimit) return true;
						}
						else
						{
							const double TownT0 = (-TownLimit - TownStart[TownDim]) / TownDelta[TownDim];
							const double TownT1 = (TownLimit - TownStart[TownDim]) / TownDelta[TownDim];
							TownEnter = FMath::Max(TownEnter, FMath::Min(TownT0, TownT1));
							TownExit = FMath::Min(TownExit, FMath::Max(TownT0, TownT1));
							if (TownEnter > TownExit) return true;
						}
					}
					return false;
				};
				// Resolve foreign owners once; owner 0 is sea, and missing/out-of-grid data is not foreign land.
				bool TownForeignOwners[256] = {};
				if (TownStyle2)
					for (const auto& TownForeignOwnerPair : NeighbourOwnerIds)
						TownForeignOwners[TownForeignOwnerPair.Key] = TownForeignOwnerPair.Key != 0 && !TownForeignOwnerPair.Value.IsEmpty() && TownForeignOwnerPair.Value != TEXT("DK");
				auto TownForeignNear = [&](const FVector2D& TownKmPoint, double TownMargin)
				{
					if (!TownStyle2 || NeighbourOwners.Num() == 0 || NeighbourOwnerW <= 0 || NeighbourOwnerH <= 0) return false;
					for (double TownForeignRadius : { 0.0, TownMargin * 0.5, TownMargin })
						for (int32 TownForeignDirection = 0; TownForeignDirection < (TownForeignRadius > 0 ? 12 : 1); ++TownForeignDirection)
						{
							const double TownForeignAngle = TownForeignDirection * UE_TWO_PI / 12;
							const FVector2D TownForeignPoint = TownKmPoint + FVector2D(FMath::Cos(TownForeignAngle), FMath::Sin(TownForeignAngle)) * TownForeignRadius;
							const FVector2D TownForeignUv = OwnerUvFromKm(TownForeignPoint);
							if (TownForeignUv.X < 0 || TownForeignUv.X >= 1 || TownForeignUv.Y <= 0 || TownForeignUv.Y > 1) continue;
							const int32 TownForeignX = FMath::Clamp(int32(TownForeignUv.X * NeighbourOwnerW), 0, NeighbourOwnerW - 1);
							const int32 TownForeignY = FMath::Clamp(int32((1.0 - TownForeignUv.Y) * NeighbourOwnerH), 0, NeighbourOwnerH - 1);
							const int32 TownForeignCell = TownForeignY * NeighbourOwnerW + TownForeignX;
							if (NeighbourOwners.IsValidIndex(TownForeignCell) && TownForeignOwners[NeighbourOwners[TownForeignCell]] && !IsMonarchyLand(TownForeignPoint)) return true;
						}
					return false;
				};
				auto TownLandClear = [&](const FTownPlot& TownPlot, bool TownHouseBorder = false)
				{
					if (TownHouseBorder && TownStyle2 && TownForeignNear(TownPosition(TownPlot.TownMid), 0.15 + TownPlot.TownHalf.Size())) return false;
					const int32 TownNX = FMath::Max(1, FMath::CeilToInt(TownPlot.TownHalf.X * 100));
					const int32 TownNY = FMath::Max(1, FMath::CeilToInt(TownPlot.TownHalf.Y * 100));
					for (int32 TownX = 0; TownX <= TownNX; ++TownX)
						for (int32 TownY = 0; TownY <= TownNY; ++TownY)
						{
							const FVector2D TownTest = TownPosition(TownCorner(TownPlot, FVector2D(TownPlot.TownHalf.X * (2.0 * TownX / TownNX - 1), TownPlot.TownHalf.Y * (2.0 * TownY / TownNY - 1))));
							if (!IsMonarchyLand(TownTest) || IsFreshWater(TownTest, 0.003f)) return false;
						}
					for (const TArray<FVector2D>& TownLane : LaneLinesKm)
						for (int32 TownI = 1; TownI < TownLane.Num(); ++TownI)
							if (!TownSegmentClear(TownPlot, TownLane[TownI - 1], TownLane[TownI], 0.0375)) return false;
					for (const FCampaign1851Railway& TownRail : Railways)
						for (int32 TownI = 1; TownI < TownRail.Km.Num(); ++TownI)
							if (!TownSegmentClear(TownPlot, TownRail.Km[TownI - 1], TownRail.Km[TownI], 0.1)) return false;
					// The square itself may hold a bridge end (the bridge in the middle of Randers and København, 2026-11-11).
					if (TownPlot.TownModel >= 0)
						for (const FCampaign1851Bridge& TownBridge : Bridges)
							if (!TownSegmentClear(TownPlot, TownBridge.EndA, TownBridge.EndB, 0.125)) return false;
					return !City.bHasPlot || TownSegmentClear(TownPlot, City.PlotKm, City.PlotKm, 0.25);
				};
				// Keep the square near the marker; only its land footprint may move it (<=300 m).
				FTownPlot TownSquare = { -1, FVector2D::ZeroVector, FVector2D(0.14, 0.14), FVector2D::ZeroVector, 0, 1, 0 };
				bool TownFound = false;
				for (int32 TownRing = 0; TownRing <= 6 && !TownFound; ++TownRing)
					for (int32 TownDirection = 0; TownDirection < (TownRing ? 24 : 1) && !TownFound; ++TownDirection)
					{
						const double TownAngle = UE_TWO_PI * TownDirection / 24;
						TownOrigin = Centre + FVector2D(FMath::Cos(TownAngle), FMath::Sin(TownAngle)) * (TownRing * 0.05);
						TownFound = TownLandClear(TownSquare);
					}
				if (!TownFound) { UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|town %s no land for central square"), *City.Name); continue; }
				TArray<TArray<FVector2D>> TownConnections;
				TArray<TArray<FVector2D>> TownMainRoads;
				auto TownStreetClear = [&](const TArray<FVector2D>& TownLine, double TownHalfWidth = 0.018)
				{
					for (const FVector2D& TownSample : Resample(TownLine, 0.01))
						for (double TownX : { -TownHalfWidth, 0.0, TownHalfWidth })
							for (double TownY : { -TownHalfWidth, 0.0, TownHalfWidth })
								if (!IsMonarchyLand(TownSample + FVector2D(TownX, TownY)) || IsFreshWater(TownSample + FVector2D(TownX, TownY), 0.003f)) return false;
					return true;
				};
				auto TownCurve = [&](const FVector2D& TownA, const FVector2D& TownB, double TownBend)
				{
					TArray<FVector2D> TownLine;
					const FVector2D TownD = TownB - TownA;
					const FVector2D TownN = FVector2D(-TownD.Y, TownD.X).GetSafeNormal();
					for (int32 TownStep = 0; TownStep <= 24; ++TownStep)
					{
						const double TownT = TownStep / 24.0;
						TownLine.Add(TownA + TownD * TownT + TownN * (TownBend * FMath::Sin(UE_PI * TownT)));
					}
					return TownLine;
				};
				auto TownAddMain = [&](const TArray<FVector2D>& TownLine)
				{
					if (FVector2D::Distance(TownLine[0], TownLine.Last()) < 0.2) return;
					for (const TArray<FVector2D>& TownExisting : TownMainRoads)
						if (FVector2D::Distance(TownExisting.Last(), TownLine.Last()) < 0.15) return;
					TownMainRoads.Add(TownLine);
				};
				// Replace each in-town road run only when both approaches reach the square on land.
				// Failed river/coast approaches retain their original road/bridge, and remain exclusion zones.
				const TArray<TArray<FVector2D>> TownOriginalRoads = TownRenderedRoads;
				TArray<TArray<FVector2D>> TownClippedRoads;
				for (const TArray<FVector2D>& TownRoad : TownRenderedRoads)
				{
					const TArray<FVector2D> TownSamples = Resample(TownRoad, 0.025);
					int32 TownFirst = INDEX_NONE, TownLast = INDEX_NONE;
					for (int32 TownI = 0; TownI < TownSamples.Num(); ++TownI)
						if (FVector2D::Distance(TownSamples[TownI], TownOrigin) < 1.25) { if (TownFirst == INDEX_NONE) TownFirst = TownI; TownLast = TownI; }
					if (TownFirst == INDEX_NONE) { TownClippedRoads.Add(TownRoad); continue; }
					TownFirst = FMath::Max(0, TownFirst - 1); TownLast = FMath::Min(TownSamples.Num() - 1, TownLast + 1);
					TArray<FVector2D> TownApproachA = TownCurve(TownOrigin, TownSamples[TownFirst], 0.045);
					TArray<FVector2D> TownApproachB = TownCurve(TownOrigin, TownSamples[TownLast], -0.035);
					if (!TownStreetClear(TownApproachA, 0.065) || !TownStreetClear(TownApproachB, 0.065))
					{
						TownClippedRoads.Add(TownRoad);
						// Keep real river crossings; connect the nearest accessible bank to the market.
						int32 TownNearest = TownFirst;
						for (int32 TownI = TownFirst; TownI <= TownLast; ++TownI)
							if (FVector2D::DistSquared(TownSamples[TownI], TownOrigin) < FVector2D::DistSquared(TownSamples[TownNearest], TownOrigin)) TownNearest = TownI;
						TArray<FVector2D> TownLink = TownCurve(TownOrigin, TownSamples[TownNearest], 0.025);
						if (TownStreetClear(TownLink, 0.065)) { TownClippedRoads.Add(TownLink); TownAddMain(TownLink); }
						continue;
					}
					TownAddMain(TownApproachA); TownAddMain(TownApproachB);
					TownClippedRoads.Add(TownApproachA); TownClippedRoads.Add(TownApproachB);
					TArray<FVector2D> TownRun;
					for (int32 TownI = 0; TownI <= TownFirst; ++TownI) TownRun.Add(TownSamples[TownI]);
					if (TownRun.Num() > 1) TownClippedRoads.Add(TownRun);
					TownRun.Reset();
					for (int32 TownI = TownLast; TownI < TownSamples.Num(); ++TownI) TownRun.Add(TownSamples[TownI]);
					if (TownRun.Num() > 1) TownClippedRoads.Add(TownRun);
				}
				TownRenderedRoads = MoveTemp(TownClippedRoads);
				// Faster start (2026-11-05): every plot was tested against every road on the map. Only roads within 12 km of the
				// market can touch a plot (the growth bank reaches about 8 km out), so the others are left out of the test.
				TArray<TArray<FVector2D>> TownNearRoads;
				for (const TArray<FVector2D>& TownRoad : TownRenderedRoads)
					for (int32 TownI = 1; TownI < TownRoad.Num(); ++TownI)
					{
						const FVector2D TownDelta = TownRoad[TownI] - TownRoad[TownI - 1];
						const double TownLengthSq = TownDelta.SizeSquared();
						const double TownT = TownLengthSq > 0.0 ? FMath::Clamp(FVector2D::DotProduct(TownOrigin - TownRoad[TownI - 1], TownDelta) / TownLengthSq, 0.0, 1.0) : 0.0;
						if (FVector2D::DistSquared(TownOrigin, TownRoad[TownI - 1] + TownDelta * TownT) < 144.0) { TownNearRoads.Add(TownRoad); break; }
					}
				auto TownRoadClear = [&](const FTownPlot& TownPlot)
				{
					for (const TArray<FVector2D>& TownRoad : TownNearRoads)
						for (int32 TownI = 1; TownI < TownRoad.Num(); ++TownI)
							if (!TownSegmentClear(TownPlot, TownRoad[TownI - 1], TownRoad[TownI], 0.065)) return false;
					for (const TArray<FVector2D>& TownRoad : TownConnections)
						for (int32 TownI = 1; TownI < TownRoad.Num(); ++TownI)
							if (!TownSegmentClear(TownPlot, TownRoad[TownI - 1], TownRoad[TownI], TownClass.TownStreetWidthKm * 0.5)) return false;
					return true;
				};
				auto TownPlotsSeparate = [&](const FTownPlot& TownA, const FTownPlot& TownB)
				{
					// Separating axes of both oriented rectangles, with a small gable gap.
					for (double TownAngle : { TownA.TownAngle, TownB.TownAngle })
						for (double TownTurn : { 0.0, UE_PI * 0.5 })
						{
							const FVector2D TownAxis(FMath::Cos(TownAngle + TownTurn), FMath::Sin(TownAngle + TownTurn));
							const FVector2D TownAU = TownRotate(TownAxis, -TownA.TownAngle), TownBU = TownRotate(TownAxis, -TownB.TownAngle);
							const double TownSpan = FMath::Abs(TownAU.X) * TownA.TownHalf.X + FMath::Abs(TownAU.Y) * TownA.TownHalf.Y + FMath::Abs(TownBU.X) * TownB.TownHalf.X + FMath::Abs(TownBU.Y) * TownB.TownHalf.Y;
							if (FMath::Abs(FVector2D::DotProduct(TownA.TownMid - TownB.TownMid, TownAxis)) >= TownSpan + 0.003) return true;
						}
					return false;
				};
				// Choose a church on a free sector of the central square, not at a town gate.
				// The large towns (København's many roads, 2026-11-08) may set the church further out from the square.
				for (double TownChurchOut = 0.0; TownChurchOut <= (TownBlockLarge ? 0.6 : 0.0) + 0.001 && TownPlan.IsEmpty(); TownChurchOut += 0.05)
				for (int32 TownTry = 0; TownTry < 48 && TownPlan.IsEmpty(); ++TownTry)
				{
					const double TownAngle = TownTry * UE_TWO_PI / 48;
					const FTownPlot TownChurchShape = TownMakePlot(4, FVector2D::ZeroVector, TownAngle, false);
					const FVector2D TownMid = TownRotate(FVector2D(0, 0.14 * (FMath::Abs(FMath::Cos(TownAngle)) + FMath::Abs(FMath::Sin(TownAngle))) + TownChurchShape.TownHalf.Y + 0.015 + TownChurchOut), TownAngle);
					const FTownPlot TownChurch = TownMakePlot(4, TownMid, TownAngle, false);
					if (TownLandClear(TownChurch) && TownRoadClear(TownChurch)) TownPlan.Add(TownChurch);
				}
				if (TownPlan.IsEmpty()) { TownRenderedRoads = TownOriginalRoads; UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|town %s no road-free church at square"), *City.Name); continue; }
				auto TownStreetAvoidsChurch = [&](const TArray<FVector2D>& TownLine)
				{
					for (int32 TownI = 1; TownI < TownLine.Num(); ++TownI)
						if (!TownSegmentClear(TownPlan[0], TownLine[TownI - 1], TownLine[TownI], TownClass.TownStreetWidthKm * 0.5)) return false;
					return true;
				};
				// Additional irregular spokes provide inland frontage in coastal towns, never a grid.
				const double TownPhase = TownLayoutRandom.FRandRange(0.f, float(UE_TWO_PI));
				for (int32 TownSpoke = 0; TownSpoke < 7; ++TownSpoke)
				{
					const double TownAngle = TownPhase + TownSpoke * UE_TWO_PI / 7 + TownLayoutRandom.FRandRange(-0.13f, 0.13f);
					const FVector2D TownEnd = TownOrigin + FVector2D(FMath::Cos(TownAngle), FMath::Sin(TownAngle)) * TownLayoutRandom.FRandRange(1.05f, 1.55f);
					TArray<FVector2D> TownLine = TownCurve(TownOrigin, TownEnd, TownLayoutRandom.FRandRange(-0.09f, 0.09f));
					if (TownStreetClear(TownLine) && TownStreetAvoidsChurch(TownLine)) TownConnections.Add(TownLine);
				}
				TArray<TArray<FVector2D>> TownFronts = TownMainRoads;
				TownFronts.Append(TownConnections);
				const int32 TownTrunks = TownFronts.Num();
				for (int32 TownBranch = 0; TownBranch < TownTrunks; ++TownBranch)
				{
					const TArray<FVector2D> TownTrunk = TownFronts[TownBranch];
					const int32 TownFork = TownLayoutRandom.RandRange(8, 15);
					const FVector2D TownD = (TownTrunk[TownFork + 1] - TownTrunk[TownFork]).GetSafeNormal();
					const double TownTurn = (TownBranch % 2 ? -1 : 1) * TownLayoutRandom.FRandRange(0.75f, 1.3f);
					const FVector2D TownEnd = TownTrunk[TownFork] + TownRotate(TownD, TownTurn) * TownLayoutRandom.FRandRange(0.32f, 0.65f);
					TArray<FVector2D> TownLine = TownCurve(TownTrunk[TownFork], TownEnd, 0.035);
					if (TownStreetClear(TownLine) && TownStreetAvoidsChurch(TownLine)) { TownConnections.Add(TownLine); TownFronts.Add(TownLine); }
					// Short cross-lanes join neighbouring spokes where land allows it.
					if (TownBranch > 0 && TownBranch % 2 == 0)
					{
						TownLine = TownCurve(TownFronts[TownBranch - 1][10], TownFronts[TownBranch][8], -0.025);
						if (TownStreetClear(TownLine) && TownStreetAvoidsChurch(TownLine)) TownConnections.Add(TownLine);
					}
				}
				// Odense compact (2026-11-05): the spokes alone gave too few plots near the market, so the last houses went to the
				// growth bank up to 5 km out. Ring streets round the market (in 30° pieces; a piece over water is left out) give
				// frontage in the core. Added after the spokes' branches, so those keep their order; no random draws.
				// -CampaignTownStyle2 (Allan 2026-11-07, "tættere huse + træer"): five rings close round the market, so the houses
				// stand in rows round the square instead of strung out.
				TArray<double> TownRingRadii = TownStyle2 ? TArray<double>{ 0.26, 0.40, 0.54, 0.68, 0.82 } : TArray<double>{ 0.45, 0.75, 1.05 };
				// Further rings for the large towns and for growth: houses are taken nearest first, so a B town only reaches them as it
				// grows (Odense at 1.6 times its people spread 3 km along the roads, 2026-11-12).
				if (TownStyle2) for (double TownMoreRing = 0.96; TownMoreRing <= 1.8; TownMoreRing += 0.14) TownRingRadii.Add(TownMoreRing);   // up to about 200 houses
				for (const double TownRingRadius : TownRingRadii)
					for (int32 TownPiece = 0; TownPiece < 12; ++TownPiece)
					{
						TArray<FVector2D> TownLine;
						for (int32 TownStep = 0; TownStep <= 24; ++TownStep)
						{
							const double TownAngle = TownPhase + (TownPiece + TownStep / 24.0) * UE_TWO_PI / 12;
							TownLine.Add(TownOrigin + FVector2D(FMath::Cos(TownAngle), FMath::Sin(TownAngle)) * TownRingRadius);
						}
						// With -CampaignTownStyle2 the rings only line up the houses: they are not drawn and not kept free (2026-11-07).
						if (TownStreetClear(TownLine) && TownStreetAvoidsChurch(TownLine)) { if (!TownStyle2) TownConnections.Add(TownLine); TownFronts.Add(TownLine); }
					}
				TArray<FTownPlot> TownCandidates;
				for (int32 TownFront = 0; TownFront < TownFronts.Num(); ++TownFront)
				{
					const TArray<FVector2D>& TownLine = TownFronts[TownFront];
					const bool TownMain = TownFront < TownMainRoads.Num();
					double TownAlong = 0;
					for (int32 TownI = 1; TownI < TownLine.Num(); ++TownI)
					{
						const FVector2D TownD = TownLine[TownI] - TownLine[TownI - 1];
						const double TownLength = TownD.Size();
						for (double TownS = TownAlong; TownS < TownLength; TownS += 0.045)
							for (int32 TownSide : { -1, 1 })
							{
								const FVector2D TownPoint = TownLine[TownI - 1] + TownD * (TownS / FMath::Max(0.000001, TownLength));
								const double TownDistance = FVector2D::Distance(TownPoint, TownOrigin);
								if (TownDistance < 0.16 || (TownDistance > 0.75 && TownLayoutRandom.FRand() < 0.24f)) continue;
								const int32 TownModel = TownDistance < 0.38 ? 3 : TownLayoutRandom.RandRange(0, 2);
								const double TownAngle = FMath::Atan2(TownD.Y, TownD.X) + TownLayoutRandom.FRandRange(-0.025f, 0.025f);
								const FTownPlot TownShape = TownMakePlot(TownModel, FVector2D::ZeroVector, TownAngle, TownSide < 0);
								const FVector2D TownMid = TownPoint - TownOrigin + TownRotate(FVector2D(0, TownSide * (TownShape.TownHalf.Y + (TownMain ? 0.065 : TownClass.TownStreetWidthKm * 0.5) + TownLayoutRandom.FRandRange(0.006f, 0.018f))), TownAngle);
								const FTownPlot TownPlot = TownMakePlot(TownModel, TownMid, TownAngle, TownSide < 0);
								if (TownLandClear(TownPlot, true) && TownRoadClear(TownPlot) && TownPlotsSeparate(TownPlot, TownSquare)) TownCandidates.Add(TownPlot);
							}
						TownAlong = FMath::Fmod(TownAlong - TownLength, 0.045);
						if (TownAlong < 0) TownAlong += 0.045;
					}
				}
				// Stable insertion sort: equal-distance candidates retain generation order on every load.
				for (int32 TownI = 1; TownI < TownCandidates.Num(); ++TownI)
				{
					const FTownPlot TownCandidate = TownCandidates[TownI];
					int32 TownJ = TownI;
					while (TownJ > 0 && TownCandidates[TownJ - 1].TownMid.SizeSquared() > TownCandidate.TownMid.SizeSquared()) { TownCandidates[TownJ] = TownCandidates[TownJ - 1]; --TownJ; }
					TownCandidates[TownJ] = TownCandidate;
				}
				for (const FTownPlot& TownCandidate : TownCandidates)
				{
					bool TownClear = true;
					for (const FTownPlot& TownExisting : TownPlan) if (!TownPlotsSeparate(TownCandidate, TownExisting)) { TownClear = false; break; }
					if (TownClear) TownPlan.Add(TownCandidate);
				}
				// A terrain-validated bank along outward continuations, not parallel rows beside town.
				TArray<TArray<FVector2D>> TownGrowthRoads;
				TArray<FVector2D>& TownEdges = TownEdgePlots.FindOrAdd(TownCityIndex);
				for (int32 TownRing = 0; TownRing < 8; ++TownRing)
					for (int32 TownFront = 0; TownFront < TownTrunks; ++TownFront)
					{
						const TArray<FVector2D>& TownTrunk = TownFronts[TownFront];
						const FVector2D TownD = (TownTrunk.Last() - TownTrunk[TownTrunk.Num() - 2]).GetSafeNormal();
						const FVector2D TownEnd = TownTrunk.Last() + TownD * (0.65 + TownRing * 0.75);
						const FVector2D TownMid = TownEnd - TownOrigin + FVector2D(-TownD.Y, TownD.X) * (0.35 * (TownFront % 2 ? -1 : 1));
						const FTownPlot TownReserve = { -1, TownMid, FVector2D(0.3, 0.3), FVector2D::ZeroVector, 0, 1, 0 };
						const TArray<FVector2D> TownStreet = TownCurve(TownTrunk.Last(), TownEnd + TownD * 0.35, 0.025);
						if (!TownLandClear(TownReserve) || !TownRoadClear(TownReserve) || !TownStreetClear(TownStreet)) continue;
						bool TownClear = true;
						for (const FTownPlot& TownExisting : TownPlan)
							if (FVector2D::Distance(TownExisting.TownMid, TownMid) < 0.3 + TownExisting.TownHalf.Size()) TownClear = false;
						for (const FVector2D& TownExisting : TownEdges) if (FVector2D::Distance(TownExisting, TownPosition(TownMid)) < 0.65) TownClear = false;
						for (const FTownPlot& TownExisting : TownPlan)
							for (int32 TownI = 1; TownI < TownStreet.Num(); ++TownI)
								if (!TownSegmentClear(TownExisting, TownStreet[TownI - 1], TownStreet[TownI], TownClass.TownStreetWidthKm * 0.5)) TownClear = false;
						for (const TArray<FVector2D>& TownRoad : TownGrowthRoads)
							for (int32 TownI = 1; TownI < TownRoad.Num(); ++TownI)
								if (!TownSegmentClear(TownReserve, TownRoad[TownI - 1], TownRoad[TownI], TownClass.TownStreetWidthKm * 0.5)) TownClear = false;
						for (const FVector2D& TownExisting : TownEdges)
						{
							const FTownPlot TownOther = { -1, TownExisting - TownOrigin, FVector2D(0.3, 0.3), FVector2D::ZeroVector, 0, 1, 0 };
							for (int32 TownI = 1; TownI < TownStreet.Num(); ++TownI)
								if (!TownSegmentClear(TownOther, TownStreet[TownI - 1], TownStreet[TownI], TownClass.TownStreetWidthKm * 0.5)) TownClear = false;
						}
						if (!TownClear) continue;
						TownGrowthRoads.Add(TownStreet);
						const double TownAngle = FMath::Atan2(TownD.Y, TownD.X);
						for (int32 TownHouse = 0; TownHouse < 4; ++TownHouse)
						{
							const FTownPlot TownGrowth = TownMakePlot((TownFront + TownHouse) % 3, TownMid + TownD * (-0.22 + TownHouse * 0.145), TownAngle, TownFront % 2 == 0);
							bool TownGrowthClear = TownLandClear(TownGrowth, true) && TownRoadClear(TownGrowth);
							for (const TArray<FVector2D>& TownRoad : TownGrowthRoads)
								for (int32 TownI = 1; TownI < TownRoad.Num(); ++TownI)
									if (!TownSegmentClear(TownGrowth, TownRoad[TownI - 1], TownRoad[TownI], TownClass.TownStreetWidthKm * 0.5)) TownGrowthClear = false;
							for (const FTownPlot& TownExisting : TownPlan) if (!TownPlotsSeparate(TownGrowth, TownExisting)) TownGrowthClear = false;
							if (TownGrowthClear) TownPlan.Add(TownGrowth);
						}
						TownEdges.Add(TownPosition(TownMid));
						TownMinorClearings.Add({ TownPosition(TownMid), 0.45f });
						UStaticMeshComponent* TownStreetMesh = BuildRibbons({ TownStreet }, TownClass.TownStreetWidthKm, Srgb(160, 126, 88), *FString::Printf(TEXT("TownEdgeStreet_%d_%d"), TownCityIndex, TownEdgeStreets.FindOrAdd(TownCityIndex).Num()), Material);
						if (TownStreetMesh) TownStreetMesh->SetVisibility(false);
						TownEdgeStreets.FindOrAdd(TownCityIndex).Add(TownStreetMesh);
					}
				// Every 1825 town of this layout, not only the large ones: Odense and Rendsborg had 2 of 6 (Allan 2026-11-11).
				if (TownStyle2)
				{
					// A continuation can cross the dense core or the sea even when an inland parcel is free.
					// Keep the original bank/order; top up genuinely free slots without a connecting street or random draws.
					auto TownReserveUnoccupied = [&](const FVector2D& TownReserveKm)
					{
						for (const FTownPlot& TownReserveHouse : TownPlan)
							if (FVector2D::Distance(TownPosition(TownReserveHouse.TownMid), TownReserveKm) < 0.3 + TownReserveHouse.TownHalf.Size()) return false;
						for (const auto& TownReserveOtherHouse : TownHouseReservations)
							if (FVector2D::Distance(TownReserveOtherHouse.Key, TownReserveKm) < 0.3 + TownReserveOtherHouse.Value) return false;
						for (const ACampaign1851ConstructionSite* TownReserveProject : Projects)
							if (TownReserveProject && FVector2D::Distance(TownReserveProject->PlotKm, TownReserveKm) < 0.3 + TownReserveProject->PlotRadiusKm) return false;
						return true;
					};
					int32 TownReserveFreeCount = 0;
					int32 TownReserveMisses[4] = {};   // occupied, spacing, road/land, growth street
					for (const FVector2D& TownReserveExisting : TownEdges)
						if (TownReserveUnoccupied(TownReserveExisting)) ++TownReserveFreeCount;
					// Stay inside the 12 km road cache, including the reserve footprint.
					for (double TownFallbackRadius = 2.5; TownFallbackRadius <= 10.5 && TownReserveFreeCount < TownBlockPopulation.TownReservedCount; TownFallbackRadius += 0.5)
						for (int32 TownFallbackDirection = 0; TownFallbackDirection < 48 && TownReserveFreeCount < TownBlockPopulation.TownReservedCount; ++TownFallbackDirection)
						{
							const double TownFallbackAngle = TownFallbackDirection * UE_TWO_PI / 48;
							const FVector2D TownFallbackMid = FVector2D(FMath::Cos(TownFallbackAngle), FMath::Sin(TownFallbackAngle)) * TownFallbackRadius;
							const FVector2D TownFallbackKm = TownPosition(TownFallbackMid);
							if (!TownReserveUnoccupied(TownFallbackKm)) { ++TownReserveMisses[0]; continue; }
							bool TownFallbackClear = true;
							for (const FVector2D& TownFallbackExisting : TownEdges)
								if (FVector2D::Distance(TownFallbackExisting, TownFallbackKm) < 0.65) TownFallbackClear = false;
							if (!TownFallbackClear) { ++TownReserveMisses[1]; continue; }
							const FTownPlot TownFallbackReserve = { -1, TownFallbackMid, FVector2D(0.3, 0.3), FVector2D::ZeroVector, 0, 1, 0 };
							if (!TownRoadClear(TownFallbackReserve) || !TownLandClear(TownFallbackReserve)) { ++TownReserveMisses[2]; continue; }
							for (const TArray<FVector2D>& TownFallbackRoad : TownGrowthRoads)
								for (int32 TownFallbackSegment = 1; TownFallbackSegment < TownFallbackRoad.Num(); ++TownFallbackSegment)
									if (!TownSegmentClear(TownFallbackReserve, TownFallbackRoad[TownFallbackSegment - 1], TownFallbackRoad[TownFallbackSegment], TownClass.TownStreetWidthKm * 0.5)) TownFallbackClear = false;
							if (!TownFallbackClear) { ++TownReserveMisses[3]; continue; }
							TownEdges.Add(TownFallbackKm);
							TownMinorClearings.Add({ TownFallbackKm, 0.45f });
							TownEdgeStreets.FindOrAdd(TownCityIndex).Add(nullptr);
							++TownReserveFreeCount;
						}
					if (TownReserveFreeCount < TownBlockPopulation.TownReservedCount)
						UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|town %s free plots %d/%d; misses occupied=%d spacing=%d road-or-land=%d growth=%d"), *City.Name,
							TownReserveFreeCount, TownBlockPopulation.TownReservedCount, TownReserveMisses[0], TownReserveMisses[1], TownReserveMisses[2], TownReserveMisses[3]);
				}
				if (TownPlan.Num() - 1 < TownBlockPopulation.TownHouseCount(City.Population))
					UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|town %s only %d/%d house candidates on land"), *City.Name, TownPlan.Num() - 1, TownBlockPopulation.TownHouseCount(City.Population));
				for (const FTownPlot& TownPlot : TownPlan) TownMinorClearings.Add({ TownPosition(TownPlot.TownMid), float(TownPlot.TownHalf.Size() + 0.05) });
				// Trace the square's perimeter, leaving all approach junctions open.
				TownConnections.Add({ TownPosition(FVector2D(-0.12, -0.12)), TownPosition(FVector2D(0.12, -0.12)), TownPosition(FVector2D(0.12, 0.12)), TownPosition(FVector2D(-0.12, 0.12)), TownPosition(FVector2D(-0.12, -0.12)) });
				// Unreal world distances are centimetres; KmToUnits is only the compressed XY map scale.
				const double TownWorldZScale = FMath::Max(1.e-6, double(FMath::Abs(GetActorScale3D().Z)));
				const double TownSlopeThreshold = 50.0 / TownWorldZScale; // 0.5 m in world space.
				const double TownStoneOverlap = 1.0 / TownWorldZScale; // 1 cm at either end.
				int32 TownPlinthCount = 0;
				int32 TownVisibleHouses = 0;
				UStaticMesh* TownCube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
				UMaterialInstanceDynamic* TownStone = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, FlatColourPath), this);
				if (TownStone) TownStone->SetVectorParameterValue(TEXT("Color"), Srgb(205, 203, 195));
				UStaticMesh* TownGardenMesh = TownStyle2 ? Campaign1851Scenery::Build(EPiece::Broadleaf, Material) : nullptr;
				for (int32 TownIndex = 0; TownIndex < TownPlan.Num(); ++TownIndex)
				{
					const FTownPlot& TownPlot = TownPlan[TownIndex];
					TownHouseReservations.Add({ TownPosition(TownPlot.TownMid), TownPlot.TownHalf.Size() });
					double TownLow = TNumericLimits<double>::Max(), TownHigh = -TownLow;
					for (int32 TownX : { -1, 1 }) for (int32 TownY : { -1, 1 })
					{
						const double TownZ = LocalAtKm(TownPosition(TownCorner(TownPlot, FVector2D(TownX * TownPlot.TownHalf.X, TownY * TownPlot.TownHalf.Y)))).Z;
						TownLow = FMath::Min(TownLow, TownZ);
						TownHigh = FMath::Max(TownHigh, TownZ);
					}
					const int32 TownOrdinal = TownIndex - 1;
					const bool TownActive = TownOrdinal < TownBlockPopulation.TownHouseCount(City.Population);
					if (TownIndex > 0 && TownActive) ++TownVisibleHouses;
					auto TownAddVisual = [&](UStaticMesh* TownMesh, UMaterialInterface* TownMaterial, const FTransform& TownTransform)
					{
						if (!TownMesh) return;
						UHierarchicalInstancedStaticMeshComponent* TownComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
						TownComponent->SetupAttachment(RootComponent);
						TownComponent->SetStaticMesh(TownMesh);
						if (TownMaterial) TownComponent->SetMaterial(0, TownMaterial);
						TownComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						TownComponent->SetVisibility(false);
						TownComponent->RegisterComponent();
						if (TownActive) TownComponent->AddInstance(TownTransform);
						Scenery.Add(TownComponent);
						TownVisualPlots.Add({ TownCityIndex, TownOrdinal, TownPosition(TownPlot.TownMid), TownPlot.TownHalf.Size(), TownActive, TownComponent, TownTransform });
					};
					UStaticMesh* TownMesh = Campaign1851Scenery::Build(TownHousePieces[TownPlot.TownModel], Material);
					double TownFit = 1;
					float TownFacing = TownPlot.TownFacing;
					if (TownMeshImported[TownPlot.TownModel])
					{
						UStaticMesh* TownImported = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Campaign1851/Scenery/DK/%s.%s"), TownHouseModels[TownPlot.TownModel], TownHouseModels[TownPlot.TownModel]));
						TownFit = TownMesh->GetBoundingBox().Max.Z / FMath::Max(1.0, TownImported->GetBoundingBox().Max.Z);
						TownMesh = TownImported;
						TownFacing -= 90.f;
					}
					FVector TownBase = LocalAtKm(TownPosition(TownPlot.TownPivot));
					TownBase.Z = TownHigh - TownMesh->GetBoundingBox().Min.Z * TownFit * TownPlot.TownScale * PieceScale;
					TownAddVisual(TownMesh, nullptr, FTransform(FRotator(0, TownFacing, 0), TownBase, FVector(TownFit * TownPlot.TownScale * PieceScale)));
					if (TownStyle2 && TownIndex > 0)
					{
						// A garden tree behind (or else beside) every house, where it touches no house, street or road.
						const double TownTreeX = TownPlot.TownHalf.X + 0.02, TownTreeY = TownPlot.TownHalf.Y + 0.02;
						const FVector2D TownTreeOffsets[] = { FVector2D(0, TownTreeY), FVector2D(0, -TownTreeY), FVector2D(TownTreeX, 0), FVector2D(-TownTreeX, 0),
							FVector2D(TownTreeX, TownTreeY), FVector2D(-TownTreeX, TownTreeY), FVector2D(TownTreeX, -TownTreeY), FVector2D(-TownTreeX, -TownTreeY) };
						for (const FVector2D& TownTreeOffset : TownTreeOffsets)
						{
							// The trunk's footprint only (the crown may reach over a roof or a garden, 2026-11-07).
							const FTownPlot TownTreePlot = { -1, TownCorner(TownPlot, TownTreeOffset), FVector2D(0.012, 0.012), FVector2D::ZeroVector, 0, 1, 0 };
							bool TownTreeClear = !TownForeignNear(TownPosition(TownTreePlot.TownMid), 0.15 + TownTreePlot.TownHalf.Size())
								&& TownRoadClear(TownTreePlot) && IsMonarchyLand(TownPosition(TownTreePlot.TownMid)) && !IsFreshWater(TownPosition(TownTreePlot.TownMid), 0.02f);
							for (const FTownPlot& TownOther : TownPlan) if (TownTreeClear && !TownPlotsSeparate(TownTreePlot, TownOther)) TownTreeClear = false;
							if (!TownTreeClear) continue;
							// Share the house ordinal, so saved population and later growth toggle the tree with its house/plinth.
							TownAddVisual(TownGardenMesh, nullptr, FTransform(FRotator(0.f, float(TownIndex * 37 % 360), 0.f), LocalAtKm(TownPosition(TownTreePlot.TownMid)) - FVector(0.0, 0.0, 0.3), FVector(0.45f + 0.06f * (TownIndex % 3)) * PieceScale));
							break;
						}
					}
					if (TownCube && TownHigh - TownLow >= TownSlopeThreshold)
					{
						FVector TownPlinthBase = LocalAtKm(TownPosition(TownPlot.TownMid));
						TownPlinthBase.Z = (TownHigh + TownLow) * 0.5;
						TownAddVisual(TownCube, TownStone, FTransform(FRotator(0, float(-FMath::RadiansToDegrees(TownPlot.TownAngle)), 0), TownPlinthBase,
							FVector(TownPlot.TownHalf.X * 2 * KmToUnits / 100, TownPlot.TownHalf.Y * 2 * KmToUnits / 100, (TownHigh - TownLow + 2 * TownStoneOverlap) / 100)));
						if (TownActive) ++TownPlinthCount;
					}
				}
				TArray<FVector2D>& TownReserved = TownReservedPlots.FindOrAdd(TownCityIndex);
				TownUpdatePlots();
				TownMinorStreets.Append(TownConnections);
				int32 TownStreetPoints = 0;
				for (const TArray<FVector2D>& TownLine : TownConnections) TownStreetPoints += TownLine.Num();
				// Diameter in geographic metres, from occupied footprints only (not the growth bank).
				TArray<FVector2D> TownBuiltCorners;
				for (int32 TownIndex = 0; TownIndex < TownPlan.Num(); ++TownIndex)
				{
					bool TownOccupied = false;
					for (const FTownVisualPlot& TownVisual : TownVisualPlots)
						if (TownVisual.TownCity == TownCityIndex && TownVisual.TownOrdinal == TownIndex - 1 && TownVisual.TownActive) { TownOccupied = true; break; }
					if (!TownOccupied) continue;
					const FTownPlot& TownPlot = TownPlan[TownIndex];
					for (int32 TownX : { -1, 1 }) for (int32 TownY : { -1, 1 })
						TownBuiltCorners.Add(TownPosition(TownCorner(TownPlot, FVector2D(TownX * TownPlot.TownHalf.X, TownY * TownPlot.TownHalf.Y))));
				}
				int32 TownPlacedSites = 0;
				for (const ACampaign1851ConstructionSite* TownSite : Projects)
					if (TownSite && !TownSite->IsGarrison() && TownSite->GetCityIndex() == TownCityIndex && TownSite->GetModule(0).Rule == ECampaign1851PlotRule::InTown)
					{
						++TownPlacedSites;
						for (int32 TownX : { -1, 1 }) for (int32 TownY : { -1, 1 })
							TownBuiltCorners.Add(TownSite->PlotKm + FVector2D(TownX * TownSite->PlotRadiusKm, TownY * TownSite->PlotRadiusKm));
					}
				double TownExtent = 0;
				for (const FVector2D& TownA : TownBuiltCorners)
					for (const FVector2D& TownB : TownBuiltCorners)
						TownExtent = FMath::Max(TownExtent, FVector2D::Distance(TownA, TownB));
				UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|scenery|town %s houses=%d church=1 plinths=%d reserved-plots=%d edge-sites=%d pop=%d street-points=%d overlaps=0 extent-m=%.0f shift-m=%.0f"), *City.Name, TownVisibleHouses, TownPlinthCount, TownReserved.Num(), TownPlacedSites, City.Population, TownStreetPoints, TownExtent * 1000.0, FVector2D::Distance(TownOrigin, Centre) * 1000.0);
				continue;
			}

			// The town is laid out along streets (user 2026-10-22, after the reference town image): a main street through the centre,
			// a market square by the church, a cross street from 1,500 people and a grid of blocks from 8,000. The houses stand along
			// the streets with their fronts to them, close in the core and looser on the edge; merchants' houses ring the square and
			// line the quay of a harbour town. Each town has its own random stream (from its name), so it looks the same every time.
			FRandomStream TownRng(int32(GetTypeHash(City.Name)));
			const float Street = TownRng.FRandRange(0.f, 90.f);
			const FVector2D TownU(FMath::Cos(FMath::DegreesToRadians(Street)), FMath::Sin(FMath::DegreesToRadians(Street)));
			const FVector2D TownV(-TownU.Y, TownU.X);
			auto TownAt = [&](float U, float V) { return Centre + TownU * U + TownV * V; };
			// Harbour side: the direction from the centre in which the water is nearest (none for an inland town).
			FVector2D TownSea = FVector2D::ZeroVector;
			for (int32 TownDir = 0; TownDir < 16 && TownSea.IsZero(); ++TownDir)
			{
				const float TownA = UE_TWO_PI * TownDir / 16.f;
				const FVector2D TownProbe(FMath::Cos(TownA), FMath::Sin(TownA));
				for (float TownD = 0.3f; TownD <= Radius; TownD += 0.3f)
					if (!IsMonarchyLand(Centre + TownProbe * TownD)) { if (TownD < Radius * 0.8f) TownSea = TownProbe; break; }
			}
			// Streets in the town's own frame (U along the main street, V across), as segments.
			struct FTownStreet { FVector2D A, B; bool bMain; };
			TArray<FTownStreet> TownStreets;
			// Class A and København in 1825 (2026-11-05): the same house and church size as classes B and C (they were 2.5 times
			// larger), a street grid in every A town, more houses than a B town and København inside its 1825 extent (about
			// 2.4 km from the square, not 6 km). The 1851 scenario keeps its layout.
			const bool TownClassA = !TownLegacy && ActiveScenario().Year == 1825;
			const float TownLength = TownClassA && City.bCapital ? FMath::Min(Radius * 0.95f, 2.4f)
				: TownLegacy || City.bCapital ? Radius * 0.95f : FMath::Min(Radius * 0.95f, Pop < 1500.f ? 0.85f : Pop < 8000.f ? 1.2f : 1.8f);
			const float TownBend = TownRng.FRandRange(-0.18f, 0.18f) * (TownClassA ? TownLength * 0.15f : Radius);   // the main street bends a little, as old streets do
			TownStreets.Add({ FVector2D(-TownLength, TownBend), FVector2D(0.f, 0.f), true });
			TownStreets.Add({ FVector2D(0.f, 0.f), FVector2D(TownLength, -TownBend), true });
			if (Pop >= 1500.f) TownStreets.Add({ FVector2D(0.f, -TownLength * 0.7f), FVector2D(0.f, TownLength * 0.7f), true });
			// A grid of blocks from 3,000 people (the houses are large symbols on the map: below that a few streets carry them all);
			// a small town of 1,500-3,000 gets one back street on each side of the main street.
			if (Pop >= 1500.f && (TownLegacy || City.bCapital || TownClassA))
			{
				const float TownBlock = 0.6f;
				const int32 TownBlocks = Pop >= 3000.f ? 99 : 1;
				for (int32 TownK = 1; TownK <= TownBlocks && TownK * TownBlock < TownLength * 0.8f; ++TownK)
				{
					for (const float TownSide : { -1.f, 1.f })
					{
						const float TownOff = TownSide * TownK * TownBlock;
						const float TownHalf = FMath::Sqrt(FMath::Max(0.f, FMath::Square(TownLength * 0.85f) - TownOff * TownOff));
						TownStreets.Add({ FVector2D(-TownHalf, TownOff), FVector2D(TownHalf, TownOff), false });   // parallel to the main street
						if (Pop >= 3000.f) TownStreets.Add({ FVector2D(TownOff, -TownHalf), FVector2D(TownOff, TownHalf), false });   // across it
					}
				}
			}

			const float TownHouseRadius = TownClassA ? 0.09f : TownBuildingRadius;
			const float TownRowOffset = TownClassA ? TownHouseRadius + 0.045f : FMath::Max(0.17f, TownHouseRadius + 0.045f);
			const float TownHouseStep = TownLegacy ? 0.2f : TownClassA ? TownHouseRadius * 2.f + 0.015f : FMath::Max(0.22f, TownHouseRadius * 2.f + 0.015f);
			const float TownChurchSize = TownRegionalClass.TownChurchRadiusKm * (City.bCapital ? 1.5f : 1.3f);   // footprint radius in km (class A)
			const float TownChurchFoot = TownClassA ? TownChurchSize * 1.05f : TownChurchRadius;
			const float TownChurchScale = TownClassA ? TownChurchSize / TownMeshRadii[4] : City.bCapital ? 1.25f : 1.05f;
			auto TownRoadFree = [&](const FVector2D& TownKm, float TownRadius, const TArray<FTownStreet>& TownLines,
				const FVector2D& TownAxisU, const FVector2D& TownAxisV, const FVector2D& TownCentre)
			{
				auto TownSegmentFree = [&](const FVector2D& TownA, const FVector2D& TownB, float TownHalfWidth)
				{
					const FVector2D TownDelta = TownB - TownA;
					const double TownLengthSq = TownDelta.SizeSquared();
					const double TownT = TownLengthSq > 0.0 ? FMath::Clamp(FVector2D::DotProduct(TownKm - TownA, TownDelta) / TownLengthSq, 0.0, 1.0) : 0.0;
					return FVector2D::DistSquared(TownKm, TownA + TownDelta * TownT) >= FMath::Square(TownRadius + TownHalfWidth);
				};
				for (const TArray<FVector2D>& TownRoad : RoadLines)
					for (int32 TownI = 1; TownI < TownRoad.Num(); ++TownI)
						if (!TownSegmentFree(TownRoad[TownI - 1], TownRoad[TownI], 0.085f)) return false;
				for (const FCampaign1851Railway& TownRail : Railways)
					for (int32 TownI = 1; TownI < TownRail.Km.Num(); ++TownI)
						if (!TownSegmentFree(TownRail.Km[TownI - 1], TownRail.Km[TownI], 0.1f)) return false;
				for (const FCampaign1851Bridge& TownBridge : Bridges)
					if (!TownSegmentFree(TownBridge.EndA, TownBridge.EndB, 0.125f)) return false;
				for (const FTownStreet& TownLine : TownLines)
					if (!TownSegmentFree(TownCentre + TownAxisU * TownLine.A.X + TownAxisV * TownLine.A.Y,
						TownCentre + TownAxisU * TownLine.B.X + TownAxisV * TownLine.B.Y, 0.0375f)) return false;
				return true;
			};
			// The square: kept free in the middle; the first church on its north side.
			const float TownSquare = Pop >= 3000.f ? 0.32f : 0.22f;
			// København (2026-11-05): the grid's 0.6 km blocks leave no gap for the church's footprint (radius >= 0.3 km), so the
			// church never stood. Its site is chosen against the main streets only, before the street points are taken, and the
			// grid streets are cut round it.
			bool TownCapitalChurchFound = false;
			FVector2D TownCapitalChurchKm = FVector2D::ZeroVector;
			if (TownClassA && City.bCapital && Pop >= 1500.f)
			{
				TArray<FTownStreet> TownMainStreets;
				for (const FTownStreet& TownStreet : TownStreets) if (TownStreet.bMain) TownMainStreets.Add(TownStreet);
				for (float TownRing = TownSquare + TownChurchFoot + 0.02f; TownRing <= Radius * 0.7f && !TownCapitalChurchFound; TownRing += 0.05f)
					for (int32 TownStep = 0; TownStep < 32 && !TownCapitalChurchFound; ++TownStep)
					{
						const float TownAngle = TownStep * UE_TWO_PI / 32.f;
						const FVector2D TownLocal(TownRing * FMath::Cos(TownAngle), TownRing * FMath::Sin(TownAngle));
						const FVector2D TownChurchKm = TownAt(TownLocal.X, TownLocal.Y);
						if (!IsMonarchyLand(TownChurchKm) || !IsFree(TownChurchKm, TownChurchFoot / PieceScale) || IsFreshWater(TownChurchKm, TownChurchFoot)
							|| !TownRoadFree(TownChurchKm, TownChurchFoot + 0.01f, TownMainStreets, TownU, TownV, Centre)) continue;
						TownCapitalChurchFound = true;
						TownCapitalChurchKm = TownChurchKm;
						// The grid streets run along U or V in the town frame (A before B): cut the stretch under the church.
						const float TownCut = TownChurchFoot + 0.05f;
						TArray<FTownStreet> TownCutStreets;
						for (const FTownStreet& TownStreet : TownStreets)
						{
							const bool TownAlongU = FMath::IsNearlyEqual(TownStreet.A.Y, TownStreet.B.Y);
							const float TownOffset = TownAlongU ? TownStreet.A.Y - TownLocal.Y : TownStreet.A.X - TownLocal.X;
							if (TownStreet.bMain || FMath::Abs(TownOffset) >= TownCut) { TownCutStreets.Add(TownStreet); continue; }
							const float TownHalfGap = FMath::Sqrt(TownCut * TownCut - TownOffset * TownOffset);
							const float TownMidS = TownAlongU ? TownLocal.X : TownLocal.Y;
							const float TownStartS = TownAlongU ? TownStreet.A.X : TownStreet.A.Y;
							const float TownEndS = TownAlongU ? TownStreet.B.X : TownStreet.B.Y;
							auto TownPiece = [&](float TownFrom, float TownTo)
							{
								if (TownTo - TownFrom < 0.05f) return;
								TownCutStreets.Add({ TownAlongU ? FVector2D(TownFrom, TownStreet.A.Y) : FVector2D(TownStreet.A.X, TownFrom),
									TownAlongU ? FVector2D(TownTo, TownStreet.A.Y) : FVector2D(TownStreet.A.X, TownTo), false });
							};
							TownPiece(TownStartS, FMath::Min(TownEndS, TownMidS - TownHalfGap));
							TownPiece(FMath::Max(TownStartS, TownMidS + TownHalfGap), TownEndS);
						}
						TownStreets = MoveTemp(TownCutStreets);
					}
				if (!TownCapitalChurchFound)
					UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|scenery|town %s no church site clear of the main streets"), *City.Name);
			}
			int32 TownStreetPoints = 0;
			if (!TownLegacy)
			{
				for (const FTownStreet& TownStreet : TownStreets)
				{
					TArray<FVector2D> TownEndpoints;
					TownEndpoints.Add(TownAt(TownStreet.A.X, TownStreet.A.Y));
					TownEndpoints.Add(TownAt(TownStreet.B.X, TownStreet.B.Y));
					for (const FVector2D& TownPoint : Resample(TownEndpoints, 0.025))
					{
						Take(TownPoint, 0.04f / PieceScale);
						++TownStreetPoints;
					}
				}
			}
			Take(Centre, TownLegacy ? TownSquare : TownSquare / PieceScale);
			TArray<FFootprint> TownBuildingPlots;
			int32 TownChurchesPlaced = 0;
			const int32 Churches = TownLegacy ? FMath::Clamp(1 + int32(Pop / 12000.f), 1, 6) : Pop < 1500.f ? 0 : 1;
			if (TownCapitalChurchFound)
			{
				Place(EPiece::Church, TownCapitalChurchKm, Street, TownChurchScale);
				Take(TownCapitalChurchKm, TownChurchFoot / PieceScale);
				TownBuildingPlots.Add({ TownCapitalChurchKm, TownChurchFoot });
				TownBaseReservations.Add({ TownCapitalChurchKm, TownChurchFoot });
				++TownChurchesPlaced;
			}
			for (int32 c = TownChurchesPlaced, Tries = 0; c < Churches && Tries < 200; ++Tries)
			{
				const FVector2D Km = !TownLegacy ? TownAt(
					(TownSquare + TownChurchFoot + 0.02f + (Tries / 16) * 0.04f) * FMath::Cos(Tries * UE_TWO_PI / 16.f),
					(TownSquare + TownChurchFoot + 0.02f + (Tries / 16) * 0.04f) * FMath::Sin(Tries * UE_TWO_PI / 16.f))
					: c == 0 ? TownAt(0.f, TownSquare + 0.14f + Tries * 0.02f)
					: TownAt(TownRng.FRandRange(-0.7f, 0.7f) * TownLength, (TownRng.FRand() < 0.5f ? -1.f : 1.f) * (0.3f + 0.6f * TownRng.FRand()) * TownLength * 0.5f);
				if (IsMonarchyLand(Km) && IsFree(Km, TownLegacy ? 0.12f : TownChurchFoot / PieceScale) && !(TownClassA && IsFreshWater(Km, TownChurchFoot))
					&& (TownLegacy || (FVector2D::Distance(Km, Centre) <= Radius && TownRoadFree(Km, TownChurchFoot, TownStreets, TownU, TownV, Centre))))
				{
					Place(EPiece::Church, Km, Street + (TownRng.FRand() < 0.5f ? 0.f : 180.f), TownChurchScale);
					Take(Km, TownLegacy ? 0.12f : TownChurchFoot / PieceScale);
					if (!TownLegacy) TownBuildingPlots.Add({ Km, TownChurchFoot });
					TownBaseReservations.Add({ Km, TownChurchFoot });
					++TownChurchesPlaced;
					++c;
				}
			}
			// A dense street grid (København) can leave no gap on the ring round the square: search the whole core, nearest first.
			if (!TownLegacy && Churches > 0 && TownChurchesPlaced == 0)
			{
				for (float TownRing = TownSquare; TownRing <= Radius * 0.7f && TownChurchesPlaced == 0; TownRing += 0.05f)
					for (int32 TownStep = 0; TownStep < 32 && TownChurchesPlaced == 0; ++TownStep)
					{
						const float TownAngle = TownStep * UE_TWO_PI / 32.f;
						const FVector2D Km = TownAt(TownRing * FMath::Cos(TownAngle), TownRing * FMath::Sin(TownAngle));
						if (!IsMonarchyLand(Km) || !IsFree(Km, TownChurchFoot / PieceScale) || (TownClassA && IsFreshWater(Km, TownChurchFoot)) || !TownRoadFree(Km, TownChurchFoot, TownStreets, TownU, TownV, Centre)) continue;
						Place(EPiece::Church, Km, Street, TownChurchScale);
						Take(Km, TownChurchFoot / PieceScale);
						TownBuildingPlots.Add({ Km, TownChurchFoot });
						TownBaseReservations.Add({ Km, TownChurchFoot });
						++TownChurchesPlaced;
					}
				if (TownChurchesPlaced == 0)
					UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|scenery|town %s no free site for its church"), *City.Name);
			}
			// Class A: more houses than the largest B town (B: 30 + 1 per 150 people over 5,000), at most 200.
			const int32 Houses = TownLegacy || City.bCapital ? FMath::Clamp(int32(Pop / 22.f), 45, 2600)
				: TownClassA ? FMath::Clamp(30 + int32(Pop - 5000.f) / 150, 50, 200) : Pop < 1500.f ? 3 : Pop < 8000.f ? 6 : 14;
			// Class A houses get the B size; the random draw stays in the same argument, so 1851 keeps its stream.
			auto TownHouseScale = [&](EPiece TownScalePiece, float TownScaleRandom)
			{
				if (!TownClassA) return TownScaleRandom;
				const int32 TownScaleModel = TownScalePiece == EPiece::MerchantHouse ? 3 : TownScalePiece == EPiece::TownHouseTimber ? 2 : TownScalePiece == EPiece::TownHouseOchre ? 1 : 0;
				return TownRegionalClass.TownHouseRadiusKm * (TownScaleModel == 3 ? 1.2f : 1.f) / TownMeshRadii[TownScaleModel] * (0.9f + 0.1f * TownScaleRandom);
			};
			int32 TownPlaced = 0;
			auto TownHouse = [&](const FVector2D& Km, float FrontYaw, bool bSquareFront)
			{
				if (TownPlaced >= Houses || !IsMonarchyLand(Km) || !IsFree(Km, TownLegacy ? 0.04f : TownHouseRadius / PieceScale)) return;
				if (TownClassA && IsFreshWater(Km, TownHouseRadius)) return;   // the Elbe at Altona, rivers through the A towns
				const float D = float(FVector2D::Distance(Km, Centre));
				const float Core = FMath::Clamp(1.f - D / (TownClassA ? TownLength : Radius), 0.f, 1.f);
				// Fewer houses towards the edge (gardens and gaps), none beyond the town.
				if (D > Radius || (TownLegacy && TownRng.FRand() > 0.35f + 0.65f * Core)) return;
				if (!TownLegacy)
				{
					if (!TownRoadFree(Km, TownHouseRadius, TownStreets, TownU, TownV, Centre)) return;
					for (const FFootprint& TownPlot : TownBuildingPlots)
						if (FVector2D::DistSquared(Km, TownPlot.Km) < FMath::Square(TownHouseRadius + TownPlot.Radius)) return;
				}
				const bool bQuay = !TownSea.IsZero() && !IsMonarchyLand(Km + TownSea * 0.25f);
				const float Kind = TownRng.FRand();
				const EPiece Piece = (bSquareFront || bQuay) && Kind < 0.7f ? EPiece::MerchantHouse
					: Core > 0.55f && Kind < 0.3f ? EPiece::MerchantHouse
					: Kind < 0.55f ? EPiece::TownHouse : Kind < 0.75f ? EPiece::TownHouseOchre : EPiece::TownHouseTimber;
				Place(Piece, Km, FrontYaw + TownRng.FRandRange(-3.f, 3.f), TownHouseScale(Piece, TownRng.FRandRange(0.85f, 1.05f)),
					Piece == EPiece::MerchantHouse ? TownRng.FRandRange(0.9f, 1.05f) : TownRng.FRandRange(0.85f, 1.05f) + 0.25f * Core);
				Take(Km, TownLegacy ? 0.04f : TownHouseRadius / PieceScale);
				if (!TownLegacy) TownBuildingPlots.Add({ Km, TownHouseRadius });
				TownBaseReservations.Add({ Km, TownHouseRadius });   // later building projects must not land on this house
				++TownPlaced;
			};
			// Round the square first, then along every street, front row before back row.
			for (int32 TownEdge = 0; TownEdge < 4; ++TownEdge)
				for (float TownS = -TownSquare; TownS <= TownSquare + 0.01f; TownS += TownHouseStep)
				{
					const float TownR = TownSquare + (TownLegacy ? 0.12f : TownRowOffset);
					const FVector2D TownLocal = TownEdge == 0 ? FVector2D(TownS, TownR) : TownEdge == 1 ? FVector2D(TownS, -TownR) : TownEdge == 2 ? FVector2D(TownR, TownS) : FVector2D(-TownR, TownS);
					TownHouse(TownAt(TownLocal.X, TownLocal.Y), Street + TownEdge * 90.f, true);
				}
			TArray<TArray<FVector2D>> TownStreetLines;
			for (const float TownRow : { TownLegacy ? 0.12f : TownRowOffset, 0.33f })
				for (const FTownStreet& TownStreet : TownStreets)
				{
					const FVector2D TownDir = (TownStreet.B - TownStreet.A).GetSafeNormal();
					const FVector2D TownN(-TownDir.Y, TownDir.X);
					const float TownLen = float(FVector2D::Distance(TownStreet.A, TownStreet.B));
					const float TownYaw = Street + FMath::RadiansToDegrees(FMath::Atan2(TownDir.Y, TownDir.X));
					for (float TownS = 0.1f; TownS < TownLen; TownS += TownHouseStep)
						for (const float TownSide : { -1.f, 1.f })
						{
							const FVector2D TownLocal = TownStreet.A + TownDir * TownS + TownN * (TownSide * TownRow);
							if (!TownLegacy && TownRow == 0.33f) continue; // Back plots stay open as courtyards and gardens.
							if (TownLegacy && TownRow > 0.2f && (!TownStreet.bMain || TownLocal.Size() > TownLength * 0.55f)) continue;   // a back row only in the core of the main streets
							TownHouse(TownAt(TownLocal.X, TownLocal.Y), TownYaw + (TownSide > 0.f ? 90.f : -90.f), false);
						}
					if ((TownLegacy ? TownRow < 0.2f : TownRow != 0.33f) && !(TownStyle2 && TownClassA && !TownStreet.bMain))
					{
						TArray<FVector2D> TownLine;
						for (float TownS = 0.f; TownS <= TownLen + 0.01f; TownS += 0.2f)
						{
							const FVector2D TownLocal = TownStreet.A + TownDir * FMath::Min(TownS, TownLen);
							const FVector2D TownKm = TownAt(TownLocal.X, TownLocal.Y);
							if (FVector2D::Distance(TownKm, Centre) <= Radius && IsMonarchyLand(TownKm) && !(TownClassA && IsFreshWater(TownKm))) TownLine.Add(TownKm);
							else if (TownLine.Num() >= 2) { TownStreetLines.Add(TownLine); TownLine.Reset(); }
							else TownLine.Reset();
						}
						if (TownLine.Num() >= 2) TownStreetLines.Add(TownLine);
					}
				}
			TownStreetLinesKm.Append(TownStreetLines);
			if (TownStyle2 && TownClassA)
			{
				// A garden tree behind or beside each house (class A): the trunk must be free of houses and streets (Taken).
				for (int32 TownTreeHouse = 0; TownTreeHouse < TownBuildingPlots.Num(); ++TownTreeHouse)
				{
					const FFootprint TownTreeOwner = TownBuildingPlots[TownTreeHouse];
					if (TownTreeOwner.Radius > TownHouseRadius + 0.001f) continue;   // the church keeps its own ground
					for (int32 TownTreeStep = 0; TownTreeStep < 8; ++TownTreeStep)
					{
						const float TownTreeAngle = (TownTreeStep + TownTreeHouse) * UE_TWO_PI / 8.f;
						const FVector2D TownTreeKm = TownTreeOwner.Km + FVector2D(FMath::Cos(TownTreeAngle), FMath::Sin(TownTreeAngle)) * (TownTreeOwner.Radius + 0.02f);
						if (!IsMonarchyLand(TownTreeKm) || IsFreshWater(TownTreeKm, 0.02f) || !IsFree(TownTreeKm, 0.012f / PieceScale)) continue;
						Take(TownTreeKm, 0.012f / PieceScale);
						TownGardenTrees.Emplace(FRotator(0.f, float(TownTreeHouse * 37 % 360), 0.f), LocalAtKm(TownTreeKm) - FVector(0.0, 0.0, 0.3), FVector(0.45f + 0.06f * (TownTreeHouse % 3)) * PieceScale);
						break;
					}
				}
			}
			if (!TownLegacy) UE_LOG(LogTemp, Verbose, TEXT("CAMPAIGN-1851|town|%s|pop=%.0f|houses=%d/%d|streets=%d|harbour=%d"), *City.Name, Pop, TownPlaced, Houses, TownStreets.Num(), TownSea.IsZero() ? 0 : 1);
			if (!TownLegacy)
			{
				int32 TownOverlaps = 0;
				for (int32 TownI = 0; TownI < TownBuildingPlots.Num(); ++TownI)
				{
					const FFootprint& TownPlot = TownBuildingPlots[TownI];
					if (!TownRoadFree(TownPlot.Km, TownPlot.Radius, TownStreets, TownU, TownV, Centre)) ++TownOverlaps;
					for (int32 TownJ = 0; TownJ < TownI; ++TownJ)
						if (FVector2D::DistSquared(TownPlot.Km, TownBuildingPlots[TownJ].Km) < FMath::Square(TownPlot.Radius + TownBuildingPlots[TownJ].Radius)) ++TownOverlaps;
				}
				double TownExtent = 0;
				for (const FFootprint& TownA : TownBuildingPlots)
					for (const FFootprint& TownB : TownBuildingPlots)
						TownExtent = FMath::Max(TownExtent, FVector2D::Distance(TownA.Km, TownB.Km) + TownA.Radius + TownB.Radius);
				UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|scenery|town %s houses=%d church=%d street-points=%d overlaps=%d extent-m=%.0f"), *City.Name, TownPlaced, TownChurchesPlaced > 0 ? 1 : 0, TownStreetPoints, TownOverlaps, TownExtent * 1000.0);
			}
			// Windmills on the town's edge, where the wind comes off the fields.
			const int32 Mills = TownLegacy ? FMath::Clamp(1 + int32(Pop / 6000.f), 1, 4) : 0;
			for (int32 m = 0, Tries = 0; m < Mills && Tries < 60; ++Tries)
			{
				const float A = TownRng.FRandRange(0.f, UE_TWO_PI);
				const FVector2D Km = Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * (Radius + TownRng.FRandRange(0.3f, 1.2f));
				if (IsMonarchyLand(Km) && IsFree(Km, 0.06f))
				{
					Place(EPiece::Windmill, Km, TownRng.FRandRange(-30.f, 30.f), TownRng.FRandRange(0.9f, 1.1f));   // sails to the west wind
					Take(Km, 0.06f);
					++m;
				}
			}
		}
	};
	// Retain the old countryside exclusion map exactly: IsFree branches there consume the shared RNG.
	TownBuild(true);
	const auto TownCountrysideTaken = Taken;
	const auto TownCountrysideMills = Items[int32(EPiece::Windmill)];
	Taken = TownRoadTaken;
	for (int32 TownPieceIndex = 0; TownPieceIndex < PieceCount; ++TownPieceIndex) Items[TownPieceIndex].Reset();
	Towns.Reset();
	TownStreetLinesKm.Reset();
	// The first pass only shapes the countryside: its houses are no reservations (2026-11-11: København's 6.5 km of
	// first-pass houses left none of its free building plots free).
	TownBaseReservations.Reset();
	TownBuild(false);
	Items[int32(EPiece::Windmill)] = TownCountrysideMills;
	Taken = TownCountrysideTaken;

	auto InTown = [&Towns](const FVector2D& Km, float Margin)
	{
		for (const FTown& T : Towns)
		{
			if (FVector2D::DistSquared(T.Km, Km) < FMath::Square(T.RadiusKm + Margin))
			{
				return true;
			}
		}
		return false;
	};

	// ---- Countryside: villages with a church, four-winged farms and cottages on a jittered grid.
	enum class ESite : uint8 { Village, Farm, Cottage };
	struct FSite { FVector2D Km; ESite Kind; };
	TArray<FSite> Sites;
	constexpr double FarmCellKm = 2.0;
	for (double Y = Extent.YMin; Y < Extent.YMax; Y += FarmCellKm)
	{
		for (double X = Extent.XMin; X < Extent.XMax; X += FarmCellKm)
		{
			const FVector2D Km(X + Rng.FRand() * FarmCellKm, Y + Rng.FRand() * FarmCellKm);
			if (!IsMonarchyLand(Km) || Woodland(Km) > 0.3f || InTown(Km, 0.5f) || IsFreshWater(Km, 0.3f))
			{
				continue;
			}
			const float Roll = Rng.FRand();
			if (Roll < 0.72f)
			{
				Sites.Add({ Km, Roll < 0.06f ? ESite::Village : Roll < 0.55f ? ESite::Farm : ESite::Cottage });
			}
		}
	}

	CountrySites.Reset();   // kept for the battlefield generator (x, y km, kind)
	for (const FSite& S : Sites)
	{
		CountrySites.Add(FVector(S.Km.X, S.Km.Y, float(S.Kind)));
	}
	// Village lanes: each village to the nearest main road and to its nearest neighbour village,
	// gently winding, and only over land.
	TArray<TArray<FVector2D>> LaneLines;
	auto AddLane = [&](const FVector2D& A, const FVector2D& B)
	{
		const FVector2D D = B - A;
		const double Len = D.Size();
		if (Len < 0.3)
		{
			return;
		}
		const FVector2D Normal = FVector2D(-D.Y, D.X) / Len;
		const float Bend = Rng.FRandRange(-0.08f, 0.08f) * float(Len), Wiggle = Rng.FRandRange(-0.03f, 0.03f) * float(Len);
		TArray<FVector2D> Line;
		const int32 Steps = FMath::Max(2, FMath::CeilToInt(Len / 0.2));
		for (int32 s = 0; s <= Steps; ++s)
		{
			const double T = double(s) / Steps;
			const FVector2D P = A + D * T + Normal * (Bend * FMath::Sin(T * UE_PI) + Wiggle * FMath::Sin(T * 3.0 * UE_PI));
			if (s > 0 && s < Steps && !IsMonarchyLand(P))
			{
				return;
			}
			Line.Add(P);
		}
		for (const FTown& TownMinorClearing : TownMinorClearings)
			for (int32 TownLaneIndex = 1; TownLaneIndex < Line.Num(); ++TownLaneIndex)
			{
				const FVector2D TownLaneDelta = Line[TownLaneIndex] - Line[TownLaneIndex - 1];
				const double TownLaneT = TownLaneDelta.SizeSquared() > 0 ? FMath::Clamp(FVector2D::DotProduct(TownMinorClearing.Km - Line[TownLaneIndex - 1], TownLaneDelta) / TownLaneDelta.SizeSquared(), 0.0, 1.0) : 0.0;
				if (FVector2D::Distance(TownMinorClearing.Km, Line[TownLaneIndex - 1] + TownLaneDelta * TownLaneT) < TownMinorClearing.RadiusKm + 0.0375) return;
			}
		LaneLines.Add(MoveTemp(Line));
	};
	TArray<FVector2D> Villages;
	for (const FSite& Site : Sites)
	{
		if (Site.Kind == ESite::Village)
		{
			Villages.Add(Site.Km);
		}
	}
	TSet<TPair<int32, int32>> Linked;
	for (int32 v = 0; v < Villages.Num(); ++v)
	{
		double Best = 8.0 * 8.0;
		const FVector2D* Nearest = nullptr;
		for (const FVector2D& P : RoadSamples)
		{
			const double D2 = FVector2D::DistSquared(P, Villages[v]);
			if (D2 < Best)
			{
				Best = D2;
				Nearest = &P;
			}
		}
		if (Nearest)
		{
			AddLane(Villages[v], *Nearest);
		}
		int32 Other = INDEX_NONE;
		Best = 6.0 * 6.0;
		for (int32 w = 0; w < Villages.Num(); ++w)
		{
			const double D2 = FVector2D::DistSquared(Villages[w], Villages[v]);
			if (w != v && D2 < Best)
			{
				Best = D2;
				Other = w;
			}
		}
		if (Other != INDEX_NONE && !Linked.Contains(TPair<int32, int32>(FMath::Min(v, Other), FMath::Max(v, Other))))
		{
			Linked.Add(TPair<int32, int32>(FMath::Min(v, Other), FMath::Max(v, Other)));
			AddLane(Villages[v], Villages[Other]);
		}
	}
	for (const TArray<FVector2D>& Line : LaneLines)
	{
		for (const FVector2D& P : Resample(Line, 0.1))
		{
			Take(P, 0.016f);
		}
	}

	for (const FSite& Site : Sites)
	{
		const FVector2D& Km = Site.Km;
		if (Site.Kind == ESite::Village)
		{
			if (IsFree(Km, 0.12f))
			{
				Place(EPiece::Church, Km, Rng.FRandRange(0.f, 360.f), 0.85f);
				Take(Km, 0.12f);
			}
			// Many villages had their mill.
			if (Rng.FRand() < 0.35f)
			{
				const float A = Rng.FRandRange(0.f, UE_TWO_PI);
				const FVector2D P = Km + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rng.FRandRange(0.5f, 1.1f);
				if (IsMonarchyLand(P) && IsFree(P, 0.06f))
				{
					Place(EPiece::Windmill, P, Rng.FRandRange(-30.f, 30.f), Rng.FRandRange(0.8f, 1.f));
					Take(P, 0.06f);
				}
			}
			const int32 Count = Rng.RandRange(5, 11);
			for (int32 i = 0, Tries = 0; i < Count && Tries < 60; ++Tries)
			{
				const float A = Rng.FRandRange(0.f, UE_TWO_PI), D = Rng.FRandRange(0.35f, 1.3f);
				const FVector2D P = Km + FVector2D(FMath::Cos(A), FMath::Sin(A)) * D;
				const bool bFarm = Rng.FRand() < 0.25f;
				const float R = bFarm ? 0.1f : 0.035f;
				if (IsMonarchyLand(P) && IsFree(P, R))
				{
					Place(bFarm ? FarmPieceAt(P) : EPiece::Cottage, P, Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.8f, 1.1f));
					Take(P, R);
					++i;
				}
			}
		}
		else if (Site.Kind == ESite::Farm && IsFree(Km, 0.1f))
		{
			Place(FarmPieceAt(Km), Km, Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.8f, 1.05f));
			Take(Km, 0.1f);
			// Haystacks in the fields round the farm.
			for (int32 k = Rng.RandRange(0, 3); k > 0; --k)
			{
				const float A = Rng.FRandRange(0.f, UE_TWO_PI);
				const FVector2D P = Km + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rng.FRandRange(0.14f, 0.3f);
				if (IsMonarchyLand(P) && IsFree(P, 0.015f))
				{
					Place(EPiece::Haystack, P, 0.f, Rng.FRandRange(0.8f, 1.2f));
					Take(P, 0.015f);
				}
			}
		}
		else if (Site.Kind == ESite::Cottage && IsFree(Km, 0.035f))
		{
			Place(EPiece::Cottage, Km, Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.85f, 1.1f));
			Take(Km, 0.035f);
		}
	}

	// ---- Trees along the main roads outside the towns (user 2026-10-22, the reference town image: rows of trees by the roads).
	// In stretches: a row of trees on one side for a few hundred metres, then open road. Their own random stream.
	{
		FRandomStream RoadTreeRng(1849);
		int32 RoadTrees = 0;
		for (int32 RoadTreeIndex = 1; RoadTreeIndex + 1 < RoadSamples.Num(); ++RoadTreeIndex)
		{
			const FVector2D RoadTreeAt = RoadSamples[RoadTreeIndex];
			const FVector2D RoadTreeAlong = (RoadSamples[RoadTreeIndex + 1] - RoadSamples[RoadTreeIndex - 1]).GetSafeNormal();
			// A stretch is planted where a slow noise along the road is high (about a third of the road).
			const float RoadTreeStretch = FMath::PerlinNoise2D(RoadTreeAt * 1.7);
			if (RoadTreeAlong.IsNearlyZero() || RoadTreeStretch < 0.12f || RoadTreeRng.FRand() < 0.2f) continue;
			const float RoadTreeSide = FMath::PerlinNoise2D(RoadTreeAt * 0.6 + FVector2D(31.0, 7.0)) > 0.f ? 1.f : -1.f;
			// Just clear of the road's own reserved strip (its samples hold 0.065 km, the tree 0.05 km).
			const FVector2D RoadTreeKm = RoadTreeAt + FVector2D(-RoadTreeAlong.Y, RoadTreeAlong.X) * (RoadTreeSide * 0.14f);
			if (!IsMonarchyLand(RoadTreeKm) || InTown(RoadTreeKm, 0.3f) || Woodland(RoadTreeKm) > 0.3f || IsFreshWater(RoadTreeKm, 0.1f) || !IsFree(RoadTreeKm, 0.02f))
				continue;
			Place(RoadTreeRng.FRand() < 0.35f ? EPiece::Oak : EPiece::Broadleaf, RoadTreeKm, RoadTreeRng.FRandRange(0.f, 360.f), RoadTreeRng.FRandRange(0.75f, 1.0f));
			Take(RoadTreeKm, 0.02f);
			++RoadTrees;
		}
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|scenery|road trees=%d"), RoadTrees);
	}

	// ---- Field boundaries round the farms and villages (Campaign1851Hydro.cpp): a grid of fields about each,
	// its sides in segments with gaps for the gates; knicks in the duchies and east Jutland, stone and earth
	// dikes on the islands and the heath, ditches in the marsh. Their own random stream, so the rest of the
	// scenery stays as it was.
	{
		FRandomStream HedgeRng(1864);
		int32 Segments = 0;
		for (const FSite& S : Sites)
		{
			if (S.Kind == ESite::Cottage)
			{
				continue;
			}
			const float FieldYaw = HedgeRng.FRandRange(0.f, 90.f);
			const FVector2D Ax(FMath::Cos(FMath::DegreesToRadians(FieldYaw)), FMath::Sin(FMath::DegreesToRadians(FieldYaw)));
			const FVector2D Ay(-Ax.Y, Ax.X);
			const float FieldKm = HedgeRng.FRandRange(0.26f, 0.4f);
			const int32 N = S.Kind == ESite::Village ? 3 : 2;
			const FVector2D Corner = S.Km - (Ax + Ay) * (FieldKm * N * 0.5f) + Ax * HedgeRng.FRandRange(-0.08f, 0.08f) + Ay * HedgeRng.FRandRange(-0.08f, 0.08f);
			const EHedgeKind HedgeKind = HedgeKindAt(S.Km);
			const EPiece Piece = HedgeKind == EHedgeKind::Knick ? EPiece::Knick : HedgeKind == EHedgeKind::Dike ? EPiece::StoneDike : EPiece::Ditch;
			for (int32 Dir = 0; Dir < 2; ++Dir)
			{
				const FVector2D U = Dir == 0 ? Ax : Ay, V = Dir == 0 ? Ay : Ax;
				for (int32 Line = 0; Line <= N; ++Line)
				{
					for (int32 Half = 0; Half < N * 2; ++Half)
					{
						if (HedgeRng.FRand() < 0.22f)
						{
							continue;   // a gap or a gate
						}
						const FVector2D A = Corner + V * (Line * FieldKm) + U * (Half * FieldKm * 0.5f);
						const FVector2D Mid = A + U * (FieldKm * 0.25f);
						if (!IsMonarchyLand(A) || !IsMonarchyLand(A + U * (FieldKm * 0.5f)) || InTown(Mid, 0.1f) || Woodland(Mid) > 0.5f
							|| !IsFree(Mid, 0.012f) || IsFreshWater(Mid, 0.04f))
						{
							continue;
						}
						// Map-local Y runs south, so the heading's Y flips.
						const float LocalYaw = FMath::RadiansToDegrees(FMath::Atan2(-U.Y, U.X));
						Items[int32(Piece)].Emplace(FRotator(0.f, LocalYaw, 0.f), LocalAtKm(Mid) - FVector(0.0, 0.0, 0.2),
							FVector(FieldKm * 0.5f * float(KmToUnits) / 4.f, PieceScale, PieceScale));
						++Segments;
					}
				}
			}
		}
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|scenery|field boundaries=%d"), Segments);
	}

	// ---- Trees: woods where the painted map has woodland, plus scattered field trees. Mostly beech;
	// spruce becomes common towards the west Jutland heath plantations.
	const double PixelKmX = SizeKm.X / FeaturesW, PixelKmY = SizeKm.Y / FeaturesH;
	for (int32 Py = 0; Py < FeaturesH; ++Py)
	{
		for (int32 Px = 0; Px < FeaturesW; ++Px)
		{
			const FColor F = Features[Py * FeaturesW + Px];
			if (F.R < 128)
			{
				continue;
			}
			const float Wood = F.G / 255.f;
			// Outside the painted woods, trees gather in copses and groves (noise ~1-2 km across) instead of an even scatter.
			const float Copse = FMath::Clamp(FMath::PerlinNoise2D(FVector2D(Px, Py) * 0.19) * 2.6f - 0.35f, 0.f, 1.f);
			float Expected = Wood > 0.08f ? 2.2f * Wood : 0.02f + 1.4f * Copse;
			const FVector2D Corner(Extent.XMin + Px * PixelKmX, Extent.YMax - (Py + 1) * PixelKmY);
			const float Westness = FMath::Clamp(float(-40.0 - Corner.X) / 80.f, 0.f, 1.f);
			while (Expected > 0.f)
			{
				if (Expected < 1.f && Rng.FRand() > Expected)
				{
					break;
				}
				Expected -= 1.f;
				const FVector2D Km = Corner + FVector2D(Rng.FRand() * PixelKmX, Rng.FRand() * PixelKmY);
				if (InTown(Km, 0.1f) || !IsFree(Km, 0.015f) || IsFreshWater(Km, 0.04f))
				{
					continue;
				}
				const bool bConifer = Rng.FRand() < 0.12f + 0.5f * Westness;
				// Beech the common broadleaf; oak a third of the rest (more of it out in the fields and hedges than deep in the woods).
				const EPiece Tree = bConifer ? EPiece::Conifer : Rng.FRand() < (Wood > 0.08f ? 0.2f : 0.45f) ? EPiece::Oak : EPiece::Broadleaf;
				Place(Tree, Km, Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.75f, 1.3f), Rng.FRandRange(0.85f, 1.15f));
			}
		}
	}

	// ---- One instanced component per piece.
	int32 Total = 0;
	for (int32 p = 0; p < PieceCount; ++p)
	{
		const EPiece Piece = EPiece(p);
		UHierarchicalInstancedStaticMeshComponent* Part = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, *FString::Printf(TEXT("Scenery_%s"), Campaign1851Scenery::Name(Piece)));
		Part->SetupAttachment(Root);
		UStaticMesh* ProceduralPiece = Campaign1851Scenery::Build(Piece, Material);
		// The modelled Danish buildings (2026-10-22, SourceAssets/Campaign1851/Scenery/DK, imported by Content/Python/import_scenery_dk.py)
		// replace the procedural boxes where they exist: same height as the piece they replace, facade (+X in the model) turned to
		// the piece's street side (-Y). -Campaign1851OldScenery keeps the procedural pieces.
		const TCHAR* SceneryModelName = nullptr;
		switch (Piece)
		{
		case EPiece::TownHouse: SceneryModelName = TEXT("SM_TownHouse_White"); break;
		case EPiece::TownHouseOchre: SceneryModelName = TEXT("SM_TownHouse_Ochre"); break;
		case EPiece::TownHouseTimber: SceneryModelName = TEXT("SM_TownHouse_Timber"); break;
		case EPiece::MerchantHouse: SceneryModelName = TEXT("SM_MerchantHouse"); break;
		case EPiece::Cottage: SceneryModelName = TEXT("SM_Cottage"); break;
		case EPiece::Farm: SceneryModelName = TEXT("SM_Farm"); break;
		case EPiece::FarmThree: SceneryModelName = TEXT("SM_Farm_Trelaenget"); break;
		case EPiece::FarmTwo: SceneryModelName = TEXT("SM_Farm_Tolaenget"); break;
		case EPiece::FarmParallel: SceneryModelName = TEXT("SM_Farm_Parallel"); break;
		case EPiece::FarmLanghus: SceneryModelName = TEXT("SM_Farm_VestjyskLanghus"); break;
		case EPiece::FarmHaubarg: SceneryModelName = TEXT("SM_Farm_Haubarg"); break;
		case EPiece::FarmNiedersachsen: SceneryModelName = TEXT("SM_Farm_Niedersachsen"); break;
		case EPiece::Church: SceneryModelName = TEXT("SM_Church"); break;
		case EPiece::Windmill: SceneryModelName = TEXT("SM_Windmill"); break;
		default: break;
		}
		UStaticMesh* SceneryModel = SceneryModelName && !FParse::Param(FCommandLine::Get(), TEXT("Campaign1851OldScenery"))
			? LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Campaign1851/Scenery/DK/%s.%s"), SceneryModelName, SceneryModelName)) : nullptr;
		// Final bounds-aware exclusion catches village outbuildings and tree crowns as well
		// as site centres, without changing countryside placement or its random stream.
		const bool TownClearPiece = Piece == EPiece::Cottage || Piece == EPiece::Broadleaf || Piece == EPiece::Oak || Piece == EPiece::Conifer
			|| Piece == EPiece::Farm || Piece == EPiece::FarmThree || Piece == EPiece::FarmTwo || Piece == EPiece::FarmParallel
			|| Piece == EPiece::FarmLanghus || Piece == EPiece::FarmHaubarg || Piece == EPiece::FarmNiedersachsen;
		if (TownClearPiece && ProceduralPiece && TownMinorClearings.Num() > 0)
		{
			const FBox TownSceneryBox = SceneryModel ? SceneryModel->GetBoundingBox() : ProceduralPiece->GetBoundingBox();
			const double TownSceneryFit = SceneryModel ? ProceduralPiece->GetBoundingBox().Max.Z / FMath::Max(1.0, TownSceneryBox.Max.Z) : 1.0;
			const double TownSceneryX = FMath::Max(FMath::Abs(TownSceneryBox.Min.X), FMath::Abs(TownSceneryBox.Max.X));
			const double TownSceneryY = FMath::Max(FMath::Abs(TownSceneryBox.Min.Y), FMath::Abs(TownSceneryBox.Max.Y));
			Items[p].RemoveAll([&](const FTransform& TownSceneryItem)
			{
				const FVector TownSceneryLocation = TownSceneryItem.GetLocation();
				const double TownSceneryRadius = FMath::Sqrt(TownSceneryX * TownSceneryX + TownSceneryY * TownSceneryY) * TownSceneryFit * TownSceneryItem.GetScale3D().X;
				for (const FTown& TownMinorClearing : TownMinorClearings)
				{
					const FVector TownClearingLocation = LocalAtKm(TownMinorClearing.Km);
					if (FVector2D::Distance(FVector2D(TownSceneryLocation.X, TownSceneryLocation.Y), FVector2D(TownClearingLocation.X, TownClearingLocation.Y)) < TownMinorClearing.RadiusKm * KmToUnits + TownSceneryRadius) return true;
				}
				return false;
			});
		}
		// -CampaignTownStyle2: the garden trees come after the town clearing, which would otherwise remove them.
		if (Piece == EPiece::Broadleaf) Items[p].Append(TownGardenTrees);
		// Other town classes use the same level foundations, sampled at actual fitted mesh corners.
		const bool TownFoundationPiece = Piece == EPiece::TownHouse || Piece == EPiece::TownHouseOchre || Piece == EPiece::TownHouseTimber || Piece == EPiece::MerchantHouse || Piece == EPiece::Church;
		if (TownFoundationPiece && ProceduralPiece)
		{
			UStaticMesh* TownFoundationMesh = SceneryModel ? SceneryModel : ProceduralPiece;
			const FBox TownBounds = TownFoundationMesh->GetBoundingBox();
			const double TownFit = SceneryModel ? ProceduralPiece->GetBoundingBox().Max.Z / FMath::Max(1.0, TownBounds.Max.Z) : 1.0;
			UStaticMesh* TownCube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			UHierarchicalInstancedStaticMeshComponent* TownFoundations = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			TownFoundations->SetupAttachment(RootComponent);
			TownFoundations->SetStaticMesh(TownCube);
			UMaterialInstanceDynamic* TownStone = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, FlatColourPath), this);
			if (TownStone) { TownStone->SetVectorParameterValue(TEXT("Color"), Srgb(110, 110, 105)); TownFoundations->SetMaterial(0, TownStone); }
			TownFoundations->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			TownFoundations->SetVisibility(false);
			TownFoundations->RegisterComponent();
			Scenery.Add(TownFoundations);
			for (FTransform& TownItem : Items[p])
			{
				const FTransform TownFitted = FTransform(FRotator(0, SceneryModel ? -90 : 0, 0), FVector::ZeroVector, FVector(TownFit)) * TownItem;
				double TownLow = TNumericLimits<double>::Max(), TownHigh = -TownLow;
				for (double TownX : { TownBounds.Min.X, TownBounds.Max.X }) for (double TownY : { TownBounds.Min.Y, TownBounds.Max.Y })
				{
					const FVector TownCorner = TownFitted.TransformPosition(FVector(TownX, TownY, 0));
					const FVector2D TownKm(Extent.XMin + SizeKm.X * 0.5 + TownCorner.X / KmToUnits, Extent.YMin + SizeKm.Y * 0.5 - TownCorner.Y / KmToUnits);
					const double TownZ = LocalAtKm(TownKm).Z;
					TownLow = FMath::Min(TownLow, TownZ); TownHigh = FMath::Max(TownHigh, TownZ);
				}
				FVector TownBase = TownItem.GetLocation();
				TownBase.Z = TownHigh - TownBounds.Min.Z * TownFitted.GetScale3D().Z;
				TownItem.SetLocation(TownBase);
				// 0.5 m in world space, as the B and C towns (2026-11-05; KmToUnits is the compressed XY scale, so every house got a plinth).
				if (TownCube && TownHigh - TownLow >= 50.0 / FMath::Max(1.e-6, double(FMath::Abs(GetActorScale3D().Z))))
				{
					FVector TownMid = TownFitted.TransformPosition(TownBounds.GetCenter());
					TownMid.Z = (TownHigh + TownLow) * 0.5;
					TownFoundations->AddInstance(FTransform(TownFitted.GetRotation(), TownMid, FVector((TownBounds.Max.X - TownBounds.Min.X) * TownFitted.GetScale3D().X / 100, (TownBounds.Max.Y - TownBounds.Min.Y) * TownFitted.GetScale3D().Y / 100, (TownHigh - TownLow) / 100)));
				}
			}
		}
		if (SceneryModel && ProceduralPiece)
		{
			const FBox SceneryOldBox = ProceduralPiece->GetBoundingBox();
			const FBox SceneryNewBox = SceneryModel->GetBoundingBox();
			const double SceneryFit = SceneryOldBox.Max.Z / FMath::Max(1.0, SceneryNewBox.Max.Z);
			const FTransform SceneryLocalFit(FRotator(0.f, -90.f, 0.f), FVector::ZeroVector, FVector(SceneryFit));
			// The old piece's drop shadow, under the model (the models carry no baked ground shadow).
			if (UStaticMesh* SceneryShadow = Campaign1851Scenery::BuildShadow(Piece, Material))
			{
				UHierarchicalInstancedStaticMeshComponent* ShadowPart = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, *FString::Printf(TEXT("Scenery_%s_Shadow"), Campaign1851Scenery::Name(Piece)));
				ShadowPart->SetupAttachment(Root);
				ShadowPart->SetStaticMesh(SceneryShadow);
				ShadowPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				ShadowPart->SetCastShadow(false);
				ShadowPart->SetVisibility(false);
				ShadowPart->RegisterComponent();
				ShadowPart->AddInstances(Items[p], false, false);
				Scenery.Add(ShadowPart);
			}
			for (FTransform& SceneryItem : Items[p]) SceneryItem = SceneryLocalFit * SceneryItem;
			Part->SetStaticMesh(SceneryModel);
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|scenery model|%s fit=%.5f"), SceneryModelName, SceneryFit);
		}
		else
		{
			Part->SetStaticMesh(ProceduralPiece);
		}
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetVisibility(false);
		Part->RegisterComponent();
		Part->AddInstances(Items[p], false, false);
		Scenery.Add(Part);
		Total += Items[p].Num();
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|scenery|%s=%d"), Campaign1851Scenery::Name(Piece), Items[p].Num());
	}
	bSceneryVisible = false;
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|scenery|total=%d"), Total);

	// ---- Road ribbons (the unlit scenery material dims non-instanced meshes to 0.84, hence the bright dirt).
	TArray<TArray<FVector2D>> Dense;
	for (const TArray<FVector2D>& Line : TownRenderedRoads)
	{
		Dense.Add(Resample(Line, 0.2));
	}
	Roads = BuildRibbons(Dense, 0.13f, Srgb(176, 136, 92), TEXT("Roads"), Material);
	Dense.Reset();
	for (const TArray<FVector2D>& Line : LaneLines)
	{
		Dense.Add(Resample(Line, 0.2));
	}
	// The town streets (not lanes for the battlefield generator) are drawn with the lanes.
	Dense.Append(TownStreetLinesKm);
	Lanes = BuildRibbons(Dense, 0.075f, Srgb(160, 126, 88), TEXT("Lanes"), Material);
	if (!TownMinorStreets.IsEmpty())
	{
		TownMinorStreetMesh = BuildRibbons(TownMinorStreets, TownMinorClass.TownStreetWidthKm, Srgb(160, 126, 88), TEXT("MinorTownStreets"), Material);
		TownMinorStreetMesh->SetVisibility(bSceneryVisible);
	}
	// Ferries: pale dashes across the water, 350 m on / 250 m off.
	Dense.Reset();
	for (const TArray<FVector2D>& Line : FerryLines)
	{
		const TArray<FVector2D> Points = Resample(Line, 0.05);
		TArray<FVector2D> Dash;
		double Along = 0.0;
		for (int32 i = 0; i < Points.Num(); ++i)
		{
			if (i > 0)
			{
				Along += FVector2D::Distance(Points[i - 1], Points[i]);
			}
			if (FMath::Fmod(Along, 0.6) < 0.35)
			{
				Dash.Add(Points[i]);
			}
			else if (Dash.Num() > 0)
			{
				Dense.Add(MoveTemp(Dash));
				Dash.Reset();
			}
		}
		if (Dash.Num() > 1)
		{
			Dense.Add(MoveTemp(Dash));
		}
	}
	Ferries = BuildRibbons(Dense, 0.07f, Srgb(236, 226, 196), TEXT("Ferries"), Material);
	bRoadsVisible = false;
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|roads|main=%d|ferries=%d|lanes=%d"), RoadLines.Num(), FerryLines.Num(), LaneLines.Num());
	LaneLinesKm = LaneLines;   // kept for the battlefield generator
}

UStaticMeshComponent* ACampaign1851Map::BuildRibbons(const TArray<TArray<FVector2D>>& Lines, float WidthKm, const FLinearColor& Colour, const TCHAR* Name, UMaterialInterface* Material)
{
	// Map-local units on the terrain surface, lifted a little so the ribbon never dips under it.
	TArray<TArray<FVector>> Local;
	for (const TArray<FVector2D>& Line : Lines)
	{
		TArray<FVector>& Out = Local.AddDefaulted_GetRef();
		for (const FVector2D& P : Line)
		{
			Out.Add(LocalAtKm(P) + FVector(0.0, 0.0, 1.2));
		}
	}
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, Name);
	Mesh->SetupAttachment(Root);
	Mesh->SetStaticMesh(Campaign1851Scenery::BuildRibbons(Local, WidthKm * 0.5f * float(KmToUnits), Colour, Material, *FString::Printf(TEXT("SM_Campaign1851_%s"), Name)));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetVisibility(false);
	Mesh->RegisterComponent();
	return Mesh;
}

// ------------------------------------------------------------------ building projects

float ACampaign1851Map::TerrainWorldZ(const FVector& World) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(World);
	const FVector2D Uv(Local.X / (SizeKm.X * KmToUnits) + 0.5, -Local.Y / (SizeKm.Y * KmToUnits) + 0.5);
	return float(GetActorTransform().TransformPosition(FVector(Local.X, Local.Y, TerrainZ(Uv))).Z);
}

FVector ACampaign1851Map::PlotWorld(int32 CityIndex) const
{
	return Cities.IsValidIndex(CityIndex) && Cities[CityIndex].bHasPlot
		? GetActorTransform().TransformPosition(LocalAtKm(Cities[CityIndex].PlotKm))
		: FVector::ZeroVector;
}

ACampaign1851ConstructionSite* ACampaign1851Map::FindProject(int32 CityIndex) const
{
	for (ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && Site->IsGarrison() && Site->GetCityIndex() == CityIndex)
		{
			return Site;
		}
	}
	return nullptr;
}

ACampaign1851ConstructionSite* ACampaign1851Map::FindBuilding(int32 CityIndex, const FString& Key) const
{
	for (ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && !Site->IsGarrison() && Site->GetCityIndex() == CityIndex && Site->GetKind() == Key)
		{
			return Site;
		}
	}
	return nullptr;
}

ACampaign1851ConstructionSite* ACampaign1851Map::SpawnSite(int32 CityIndex, const FVector2D& Km, float Yaw, const TArray<FCampaign1851SiteModule>& Modules, bool bGarrison, const FVector2D& GateLocal)
{
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, ConstructionMaterialPath);
	if (!Material || !Cities.IsValidIndex(CityIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|no construction material or town %d"), CityIndex);
		return nullptr;
	}
	// Buildings stand on the lowest ground under the plot, so on a slope they sink into the hill
	// instead of floating. Garrisons and earthworks (which spread wide on the exaggerated relief)
	// keep the height at their centre.
	FVector Base = GetActorTransform().TransformPosition(LocalAtKm(Km));
	const float Reach = (bGarrison || !Modules[0].bScaffold) ? 0.f : Modules[0].PlotRadiusKm * 0.75f;
	for (int32 a = 0; Reach > 0.f && a < 8; ++a)
	{
		const float A = a * UE_TWO_PI / 8.f;
		Base.Z = FMath::Min(Base.Z, GetActorTransform().TransformPosition(LocalAtKm(Km + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Reach)).Z);
	}
	const FTransform Xf(FRotator(0.f, Yaw, 0.f), Base - FVector(0.0, 0.0, 0.6), FVector(PieceScale));

	// The wagon runs from the town centre to the site's gate, on the terrain.
	TArray<FVector> Path;
	const FVector From = Cities[CityIndex].World, To = Xf.TransformPosition(FVector(GateLocal.X, GateLocal.Y, 0.0));
	const int32 Steps = FMath::Max(2, FMath::CeilToInt(FVector::Dist2D(From, To) / 10.0));
	for (int32 s = 0; s <= Steps; ++s)
	{
		FVector P = FMath::Lerp(From, To, double(s) / Steps);
		P.Z = TerrainWorldZ(P) + 0.3;
		Path.Add(P);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACampaign1851ConstructionSite* Site = GetWorld()->SpawnActor<ACampaign1851ConstructionSite>(ACampaign1851ConstructionSite::StaticClass(), Xf, Params);
	Site->SetActorScale3D(FVector(PieceScale));
	Site->Setup(CityIndex, Path, Material, Modules, bGarrison);
	Site->PlotKm = Km;
	Site->PlotRadiusKm = Modules[0].PlotRadiusKm;
	Site->SetActorHiddenInGame(!bSceneryVisible);
	Projects.Add(Site);
	return Site;
}

ACampaign1851ConstructionSite* ACampaign1851Map::StartProject(int32 CityIndex, bool bCharge)
{
	if (ACampaign1851ConstructionSite* Existing = FindProject(CityIndex))
	{
		return Existing;
	}
	const TArray<FCampaign1851SiteModule>& Garrison = ACampaign1851ConstructionSite::GarrisonModules();
	if (!Cities.IsValidIndex(CityIndex) || !Cities[CityIndex].bHasPlot || (bCharge && (!BarracksBlockReason(CityIndex).IsEmpty() || !CanAfford(Garrison[0].Cost()))))
	{
		return nullptr;
	}
	const FCampaign1851City& City = Cities[CityIndex];
	ACampaign1851ConstructionSite* Site = SpawnSite(CityIndex, City.PlotKm, City.PlotYaw, Garrison, true,
		FVector2D(Campaign1851Scenery::BarracksLength * 0.5 + 2.5, 11.0));
	if (Site && bCharge)
	{
		AddTransaction(-Garrison[0].Cost() * Campaign1851Buildings::DownPayment, FString::Printf(TEXT("Materialer bestilt: %s, infanterikaserne"), *City.Name));
		UseMaterials(TownKm(CityIndex), Garrison[0].Cost(), FString::Printf(TEXT("%s, infanterikaserne"), *City.Name));
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|project|%s|barracks started"), *City.Name);
	const auto LogActionResult = Site;
	if (LogActionResult && CampaignLogActor == TEXT("player")) { LogCampaignDecision(CampaignLogNation(), TEXT("build"), TEXT("StartProject"), TEXT("Ordre gennemført via kortets funktion"), FString(), { { TEXT("CityIndex"), double(CityIndex) }, { TEXT("bCharge"), double(bCharge) } }, {  }); }
	return LogActionResult;
}

bool ACampaign1851Map::StartModule(int32 CityIndex, int32 Module)
{
	ACampaign1851ConstructionSite* Site = FindProject(CityIndex);
	if (!Site || !Site->CanStartModule(Module) || !CanAfford(Site->ModuleCost(Module)))
	{
		return false;
	}
	Site->StartModule(Module);
	AddTransaction(-Site->ModuleCost(Module) * Campaign1851Buildings::DownPayment,
		FString::Printf(TEXT("Materialer bestilt: %s, %s"), *Cities[CityIndex].Name, *Site->ModuleName(Module).ToLower()));
	UseMaterials(TownKm(CityIndex), Site->ModuleCost(Module), FString::Printf(TEXT("%s, %s"), *Cities[CityIndex].Name, *Site->ModuleName(Module).ToLower()));
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|project|%s|%s started"), *Cities[CityIndex].Name, *Site->ModuleName(Module));
	const auto LogActionResult = true;
	if (LogActionResult && CampaignLogActor == TEXT("player")) { LogCampaignDecision(CampaignLogNation(), TEXT("build"), TEXT("StartModule"), TEXT("Ordre gennemført via kortets funktion"), FString(), { { TEXT("CityIndex"), double(CityIndex) }, { TEXT("Module"), double(Module) } }, {  }); }
	return LogActionResult;
}

// ------------------------------------------------------------------ town buildings

namespace
{
	/** Forts go only where the war was fought over them (1848-50, 1864). */
	struct FStrategicPoint { const TCHAR* Town; double Lat, Lon; };
	const FStrategicPoint StrategicPoints[] = {
		{ TEXT("Slesvig"), 54.48, 9.52 },       // Dannevirke
		{ TEXT("Sønderborg"), 54.905, 9.735 },  // Dybbøl
		{ TEXT("Fredericia"), 55.568, 9.735 },  // Fredericia's ramparts
	};

	const FStrategicPoint* StrategicPointFor(const FString& Town)
	{
		for (const FStrategicPoint& P : StrategicPoints)
		{
			if (Town == P.Town)
			{
				return &P;
			}
		}
		return nullptr;
	}

	/** Local +Y (the building's front) pointing along a direction in projected km (world Y is south). */
	float YawFacing(const FVector2D& Dir) { return FMath::RadiansToDegrees(FMath::Atan2(-Dir.X, -Dir.Y)); }
}

bool ACampaign1851Map::IsSea(const FVector2D& Km) const
{
    if (bFullNeighbourGrid && !IsMonarchyLand(Km))
    {
        const FVector2D TileSeaUv = OwnerUvFromKm(Km);
        if (TileSeaUv.X < 0 || TileSeaUv.X >= 1 || TileSeaUv.Y <= 0 || TileSeaUv.Y > 1) { return true; }
        const int32 TileSeaX = FMath::Clamp(int32(TileSeaUv.X * NeighbourOwnerW), 0, NeighbourOwnerW - 1);
        const int32 TileSeaY = FMath::Clamp(int32((1.0 - TileSeaUv.Y) * NeighbourOwnerH), 0, NeighbourOwnerH - 1);
        return NeighbourOwners[TileSeaY * NeighbourOwnerW + TileSeaX] == 0;
    }
	return !IsMonarchyLand(Km) && SampleHeight01(UvFromKm(Km)) < 0.002f;
}

bool ACampaign1851Map::IsCoastalTown(int32 CityIndex) const
{
	if (const bool* Known = CoastalTowns.Find(CityIndex))
	{
		return *Known;
	}
	const FCampaign1851City& City = Cities[CityIndex];
	const FVector2D Centre = Extent.Projection.Forward(City.Lat, City.Lon);
	const float R = TownRadiusKm(City.Population);
	bool bCoast = false;
	for (float D = 0.3f; D <= R + 2.5f && !bCoast; D += 0.25f)
	{
		for (int32 a = 0; a < 24 && !bCoast; ++a)
		{
			const float A = a * UE_TWO_PI / 24.f;
			bCoast = IsSea(Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * D);
		}
	}
	CoastalTowns.Add(CityIndex, bCoast);
	return bCoast;
}

FString ACampaign1851Map::BuildingBlockReason(int32 CityIndex, const FString& Key) const
{
	const FCampaign1851SiteModule* Def = ACampaign1851ConstructionSite::FindTownBuilding(Key);
	if (!Def || !Cities.IsValidIndex(CityIndex) || Cities[CityIndex].bForeign || Cities[CityIndex].bBornholm)
	{
		return TEXT("-");
	}
	const FCampaign1851City& City = Cities[CityIndex];
	if (Def->Rule == ECampaign1851PlotRule::Strategic && !StrategicPointFor(City.Name))
	{
		return TEXT("-");
	}
	if (FindBuilding(CityIndex, Key))
	{
		return FString();
	}
	if (City.Population < Def->MinPopulation)
	{
		return FString::Printf(TEXT("kræver over %d.%03d indb."), Def->MinPopulation / 1000, Def->MinPopulation % 1000);
	}
	if (Def->bNeedsCoast && !IsCoastalTown(CityIndex))
	{
		return TEXT("kræver kyst og havn");
	}
	if (Def->FromYear > 0 && GetDate().GetYear() < Def->FromYear)
	{
		return FString::Printf(TEXT("kan bygges fra %d"), Def->FromYear);
	}
	return FString();
}

bool ACampaign1851Map::PlotFits(const FVector2D& Km, float RadiusKm) const
{
	for (const TPair<FVector2D, double>& TownReservation : TownHouseReservations)
		if (FVector2D::Distance(Km, TownReservation.Key) < RadiusKm + TownReservation.Value) return false;
	if (!IsMonarchyLand(Km))
	{
		return false;
	}
	for (float X : { -0.8f, 0.8f })
	{
		for (float Y : { -0.8f, 0.8f })
		{
			if (!IsMonarchyLand(Km + FVector2D(X, Y) * RadiusKm))
			{
				return false;
			}
		}
	}
	for (const FVector2D& P : RoadSampleKm)
	{
		if (FVector2D::DistSquared(P, Km) < FMath::Square(RadiusKm * 0.8f + 0.05f))
		{
			return false;
		}
	}
	for (const FCampaign1851City& C : Cities)
	{
		if (C.bHasPlot && FVector2D::DistSquared(C.PlotKm, Km) < FMath::Square(RadiusKm + 0.45f))
		{
			return false;
		}
	}
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && !Site->IsGarrison() && FVector2D::DistSquared(Site->PlotKm, Km) < FMath::Square(RadiusKm + Site->PlotRadiusKm))
		{
			return false;
		}
	}
	return true;
}

bool ACampaign1851Map::FindBuildingPlot(int32 CityIndex, const FCampaign1851SiteModule& Def, FVector2D& OutKm, float& OutYaw) const
{
	const FCampaign1851City& City = Cities[CityIndex];
	const FVector2D Centre = Extent.Projection.Forward(City.Lat, City.Lon);
	const float R = TownRadiusKm(City.Population);
	if (Def.Rule == ECampaign1851PlotRule::Strategic)
	{
		const FStrategicPoint* Point = StrategicPointFor(City.Name);
		OutKm = Point ? Extent.Projection.Forward(Point->Lat, Point->Lon) : Centre;
		OutYaw = 0.f;   // the front faces south, where the enemy came from
		return Point != nullptr;
	}
	if (Def.Rule == ECampaign1851PlotRule::InTown && Def.PlotRadiusKm <= 0.3f)
		if (const TArray<FVector2D>* TownReserved = TownReservedPlots.Find(CityIndex))
			for (const FVector2D& TownKm : *TownReserved)
				if (PlotFits(TownKm, Def.PlotRadiusKm))
				{
					OutKm = TownKm;
					OutYaw = YawFacing((Centre - TownKm).GetSafeNormal());
					return true;
				}
	float MinD = R + 0.2f, MaxD = R + 1.2f;
	switch (Def.Rule)
	{
	case ECampaign1851PlotRule::InTown:
		MinD = R + Def.PlotRadiusKm;
		for (const FTownVisualPlot& TownHouse : TownVisualPlots)
			if (TownHouse.TownCity == CityIndex && TownHouse.TownActive)
				MinD = FMath::Max(MinD, float(FVector2D::Distance(Centre, TownHouse.TownKm) + TownHouse.TownRadius + Def.PlotRadiusKm));
		MaxD = MinD + 3.f; break;
	case ECampaign1851PlotRule::Outside: MinD = R + 1.2f; MaxD = R + 3.f; break;
	case ECampaign1851PlotRule::Shore:   MinD = 0.2f * R; MaxD = R + 3.f; break;
	default: break;
	}
	// Prefer street fronts on the roads leaving town, beyond the occupied envelope.
	if (Def.Rule == ECampaign1851PlotRule::InTown)
		for (const TArray<FVector2D>& TownRoad : RoadLines)
			for (int32 TownI = 1; TownI < TownRoad.Num(); ++TownI)
			{
				const FVector2D TownDirection = (TownRoad[TownI] - TownRoad[TownI - 1]).GetSafeNormal();
				const FVector2D TownNormal(-TownDirection.Y, TownDirection.X);
				for (int32 TownSide : { -1, 1 })
				{
					const FVector2D TownKm = TownRoad[TownI] + TownNormal * (TownSide * (Def.PlotRadiusKm + 0.08f));
					const double TownDistance = FVector2D::Distance(Centre, TownKm);
					if (TownDistance < MinD || TownDistance > MaxD || !PlotFits(TownKm, Def.PlotRadiusKm)) continue;
					OutKm = TownKm;
					OutYaw = YawFacing(-TownNormal * TownSide);
					return true;
				}
			}
	// Nearest fitting plot, searched ring by ring; the start angle varies by town.
	const float Offset = float(CityIndex) * 0.61f;
	for (float D = MinD; D <= MaxD; D += 0.1f)
	{
		for (int32 a = 0; a < 36; ++a)
		{
			const float A = Offset + a * UE_TWO_PI / 36.f;
			const FVector2D Dir(FMath::Cos(A), FMath::Sin(A));
			const FVector2D P = Centre + Dir * D;
			if (!PlotFits(P, Def.PlotRadiusKm))
			{
				continue;
			}
			if (Def.Rule == ECampaign1851PlotRule::Shore)
			{
				if (!IsSea(P + Dir * (Def.PlotRadiusKm + 0.25f)))
				{
					continue;
				}
				OutYaw = YawFacing(Dir);    // facing the water
			}
			else
			{
				OutYaw = YawFacing(-Dir);   // facing the town
			}
			OutKm = P;
			return true;
		}
	}
	return false;
}

void ACampaign1851Map::ClearScenery(const FVector2D& Km, float RadiusKm)
{
	const FVector Centre = LocalAtKm(Km);
	const double Radius2 = FMath::Square(RadiusKm * KmToUnits * 1.1);
	for (UHierarchicalInstancedStaticMeshComponent* Part : Scenery)
	{
		TArray<int32> Remove;
		for (int32 i = 0; i < Part->GetInstanceCount(); ++i)
		{
			FTransform T;
			if (Part->GetInstanceTransform(i, T, false) && FVector::DistSquared2D(T.GetLocation(), Centre) < Radius2)
			{
				Remove.Add(i);
			}
		}
		if (Remove.Num() > 0)
		{
			Part->RemoveInstances(Remove);
		}
	}
}

ACampaign1851ConstructionSite* ACampaign1851Map::StartBuilding(int32 CityIndex, const FString& Key, bool bCharge, const FVector2D* ForcedKm, float ForcedYaw, FString* OutReason)
{
	auto Fail = [OutReason](const FString& Why) -> ACampaign1851ConstructionSite*
	{
		if (OutReason)
		{
			*OutReason = Why;
		}
		return nullptr;
	};
	const FCampaign1851SiteModule* Def = ACampaign1851ConstructionSite::FindTownBuilding(Key);
	if (!Def || !Cities.IsValidIndex(CityIndex))
	{
		return Fail(TEXT("Ukendt bygning"));
	}
	if (ACampaign1851ConstructionSite* Existing = FindBuilding(CityIndex, Key))
	{
		return Existing;
	}
	if (bCharge)
	{
		const FString Why = BuildingBlockReason(CityIndex, Key);
		if (!Why.IsEmpty())
		{
			return Fail(Why == TEXT("-") ? FString(TEXT("Kan ikke bygges her")) : Why);
		}
		if (!CanAfford(Def->Cost()))
		{
			return Fail(TEXT("Ikke råd til materialerne endnu"));
		}
	}
	TownUpdatePlots();
	FVector2D Km;
	float Yaw = ForcedYaw;
	if (ForcedKm)
	{
		Km = *ForcedKm;
	}
	else if (!FindBuildingPlot(CityIndex, *Def, Km, Yaw))
	{
		return Fail(TEXT("Ingen ledig byggegrund"));
	}
	if (Def->Rule != ECampaign1851PlotRule::InTown) ClearScenery(Km, Def->PlotRadiusKm);
	ACampaign1851ConstructionSite* Site = SpawnSite(CityIndex, Km, Yaw, { *Def }, false, FVector2D(Def->Length * 0.5 + 2.5, Def->Width * 0.5 + 2.5));
	if (!Site)
	{
		return Fail(TEXT("Byggepladsen kunne ikke oprettes"));
	}
	TownUpdatePlots();
	if (bCharge)
	{
		AddTransaction(-Def->Cost() * Campaign1851Buildings::DownPayment, FString::Printf(TEXT("Materialer bestilt: %s, %s"), *Cities[CityIndex].Name, *Def->Name.ToLower()));
		UseMaterials(Km, Def->Cost(), FString::Printf(TEXT("%s, %s"), *Cities[CityIndex].Name, *Def->Name.ToLower()));
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|project|%s|%s started at %.1f,%.1f km"), *Cities[CityIndex].Name, *Def->Name, Km.X, Km.Y);
	const auto LogActionResult = Site;
	if (LogActionResult && CampaignLogActor == TEXT("player")) { LogCampaignDecision(CampaignLogNation(), TEXT("build"), TEXT("StartBuilding"), TEXT("Ordre gennemført via kortets funktion"), FString(), { { TEXT("CityIndex"), double(CityIndex) }, { TEXT("bCharge"), double(bCharge) }, { TEXT("ForcedYaw"), double(ForcedYaw) } }, { { TEXT("Key"), Key } }); }
	return LogActionResult;
}

int32 ACampaign1851Map::FindCity(const FString& Name) const
{
	return Cities.IndexOfByPredicate([&Name](const FCampaign1851City& C) { return C.Name.Equals(Name, ESearchCase::IgnoreCase); });
}

void ACampaign1851Map::ClearProjects()
{
	for (ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site)
		{
			Site->Destroy();
		}
	}
	Projects.Reset();
}

bool ACampaign1851Map::RestoreProject(const FString& CityName, const TArray<float>& ModuleDays, int32 ActiveModule, const FString& Kind, const FVector2D& PlotKm, float Yaw)
{
	const int32 CityIndex = FindCity(CityName);
	ACampaign1851ConstructionSite* Site = nullptr;
	if (CityIndex != INDEX_NONE)
	{
		Site = (Kind.IsEmpty() || Kind == TEXT("Garrison")) ? StartProject(CityIndex, false) : StartBuilding(CityIndex, Kind, false, &PlotKm, Yaw);
	}
	if (!Site)
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|save|cannot restore '%s' in '%s'"), *Kind, *CityName);
		return false;
	}
	Site->RestoreState(ModuleDays, ActiveModule);
	return true;
}

// ------------------------------------------------------------------ calendar and seasons

namespace
{
	/** Speed 1: a quarter of an hour a second (a day in 96 s); speed 3: a day in 8 s; speed 5: a day a second. */
	const float SpeedHours[] = { 0.f, 0.25f, 1.f, 3.f, 8.f, 24.f, 96.f };
	const TCHAR* SpeedLabels[] = { TEXT("PAUSE"), TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6") };
	const TCHAR* MonthNames[] = { TEXT("januar"), TEXT("februar"), TEXT("marts"), TEXT("april"), TEXT("maj"), TEXT("juni"),
		TEXT("juli"), TEXT("august"), TEXT("september"), TEXT("oktober"), TEXT("november"), TEXT("december") };
	const TCHAR* MonthShort[] = { TEXT("jan."), TEXT("feb."), TEXT("mar."), TEXT("apr."), TEXT("maj"), TEXT("jun."),
		TEXT("jul."), TEXT("aug."), TEXT("sep."), TEXT("okt."), TEXT("nov."), TEXT("dec.") };

	float Ramp(float X, float A, float B) { return FMath::Clamp((X - A) / (B - A), 0.f, 1.f); }

	/** 0..1 over the year: rises Rise0..Rise1, falls Fall0..Fall1 (day of year; past 365 wraps into the next year). */
	float SeasonWindow(float Day, float Rise0, float Rise1, float Fall0, float Fall1)
	{
		auto W = [&](float X) { return FMath::Min(Ramp(X, Rise0, Rise1), 1.f - Ramp(X, Fall0, Fall1)); };
		return FMath::Max(W(Day), W(Day + 365.f));
	}
}

float ACampaign1851Map::HoursPerSecondAt(int32 InSpeed) { return SpeedHours[FMath::Clamp(InSpeed, 0, NumSpeeds() - 1)]; }
const TCHAR* ACampaign1851Map::SpeedLabel(int32 InSpeed) { return SpeedLabels[FMath::Clamp(InSpeed, 0, NumSpeeds() - 1)]; }

FString ACampaign1851Map::FormatDate(const FDateTime& Date, bool bShort)
{
	const int32 M = FMath::Clamp(Date.GetMonth(), 1, 12) - 1;
	return FString::Printf(TEXT("%d. %s %d"), Date.GetDay(), bShort ? MonthShort[M] : MonthNames[M], Date.GetYear());
}

FString ACampaign1851Map::GetSeasonName() const
{
	const int32 M = GetDate().GetMonth();
	return M == 12 || M <= 2 ? TEXT("Vinter") : M <= 5 ? TEXT("Forår") : M <= 8 ? TEXT("Sommer") : TEXT("Efterår");
}

void ACampaign1851Map::AdvanceTime(float DeltaSeconds)
{
	FCampaign1851LogActorScope LogSimulationScope(this, TEXT("ai"));
	if (!bReady)
	{
		return;
	}
	const int32 MonthBefore = GetDate().GetMonth();
	// The clock ticks in whole minutes; the work of those minutes is done at once.
	double Minutes = 0.0;
	if (Speed >= 6)
	{
		// Speed 6: exactly one day per beat, in an even rhythm (1, 2, 3 ...). The beat is kept on the wall
		// clock from the end of the last day's work, so a day that takes long to compute (marches, supply,
		// the war) delays the next one instead of letting it follow at once.
		if (FPlatformTime::Seconds() >= NextDayAt)
		{
			Minutes = 1440.0;
			bDayBeat = true;
		}
	}
	else
	{
		MinuteCarry += double(DeltaSeconds) * HoursPerSecondAt(Speed) * 60.0;
		Minutes = FMath::FloorToDouble(MinuteCarry);
		MinuteCarry -= Minutes;
	}
	const float DeltaDays = float(Minutes / 1440.0);
	CampaignDays = FMath::RoundToDouble((CampaignDays + Minutes / 1440.0) * 1440.0) / 1440.0;
	for (ACampaign1851ConstructionSite* Site : Projects)
	{
		if (!Site)
		{
			continue;
		}
		// Work done today: slower in frost, and only as far as the treasury can pay the wages.
		float Work = 0.f;
		const int32 Module = Site->GetActiveModule();
		if (Module != INDEX_NONE && DeltaDays > 0.f && Site->IsPrivate())
		{
			Work = DeltaDays * Campaign1851Buildings::WorkRate(Site->ModuleType(Module), GetDate());   // private money
		}
		else if (Module != INDEX_NONE && DeltaDays > 0.f)
		{
			Work = DeltaDays * Campaign1851Buildings::WorkRate(Site->ModuleType(Module), GetDate());
			const double PerDay = Site->ModuleCostPerDay(Module);
			const bool bShort = Work * PerDay > Treasury;
			if (bShort)
			{
				Work = PerDay > 0.0 ? float(Treasury / PerDay) : Work;
			}
			Site->SetStalled(bShort);
			const double Spend = Work * PerDay;
			Treasury -= Spend;
			MonthSpend.FindOrAdd(FString::Printf(TEXT("Byggeri: %s, %s"), *Cities[Site->GetCityIndex()].Name,
				*Site->ModuleName(Module).ToLower())) += Spend;
		}
		Site->Advance(Work, Speed > 0 ? DeltaSeconds : 0.f);
	}
	AdvanceNetwork(DeltaDays, Speed > 0 ? DeltaSeconds : 0.f);
	AdvanceForts(DeltaDays);
	AdvanceDemolitions(DeltaDays);
	AdvanceSupply(DeltaDays);
	AdvanceFooting(DeltaDays);
	AdvanceWar(DeltaDays);
	AdvanceSupplyColumns(DeltaDays);
	AdvanceArmy(DeltaDays, Speed > 0 ? DeltaSeconds : 0.f);
	if (GetDate().GetMonth() != MonthBefore)
	{
		CloseMonth();
	}
	// -CampaignAutoBuild: raise whole garrison complexes, module after module, paid like any order (demos, captures).
	static const bool bAutoBuild = FParse::Param(FCommandLine::Get(), TEXT("CampaignAutoBuild"));
	for (int32 i = 0; bAutoBuild && i < Projects.Num(); ++i)
	{
		for (int32 m = 1; Projects[i] && Projects[i]->IsGarrison() && Projects[i]->GetActiveModule() == INDEX_NONE && m < Projects[i]->NumModules(); ++m)
		{
			if (Projects[i]->CanStartModule(m) && StartModule(Projects[i]->GetCityIndex(), m))
			{
				break;
			}
		}
	}
	UpdateSeason();
	if (bDayBeat)
	{
		NextDayAt = FPlatformTime::Seconds() + DaySeconds;
		bDayBeat = false;
	}

	TickCampaignLog();
}

void ACampaign1851Map::UpdateSeason()
{
	if (!SeasonCollection || !GetWorld())
	{
		return;
	}
	// Danish seasons by day of year: snow Dec-Mar, bare broadleaves Nov-Apr, autumn colours
	// Sep-Nov, fresh spring green Apr-Jun. Summer is the painting as it is (harvest gold).
	const FDateTime Date = GetDate();
	const float Day = float(Date.GetDayOfYear()) + float(Date.GetTimeOfDay().GetTotalDays());
	const float Snow = 0.9f * SeasonWindow(Day, 335.f, 370.f, 416.f, 444.f);
	const float Bare = SeasonWindow(Day, 314.f, 335.f, 465.f, 486.f);
	const float Autumn = SeasonWindow(Day, 253.f, 288.f, 314.f, 339.f);
	const float Spring = SeasonWindow(Day, 100.f, 130.f, 150.f, 172.f);
	UKismetMaterialLibrary::SetScalarParameterValue(this, SeasonCollection, TEXT("Snow"), Snow);
	UKismetMaterialLibrary::SetScalarParameterValue(this, SeasonCollection, TEXT("Bare"), Bare);
	UKismetMaterialLibrary::SetScalarParameterValue(this, SeasonCollection, TEXT("Autumn"), Autumn);
	UKismetMaterialLibrary::SetScalarParameterValue(this, SeasonCollection, TEXT("Spring"), Spring);
}

// ------------------------------------------------------------------ treasury

TArray<FCampaign1851BudgetLine> ACampaign1851Map::MonthlyBudget() const
{
	TArray<FCampaign1851BudgetLine> Lines;
	for (const TCHAR* Region : { TEXT("K"), TEXT("S"), TEXT("H") })
	{
		Lines.Add({ FString::Printf(TEXT("Skatter: %s"), *RegionName(Region)), YearlyTax(Region) / 12.0 });
	}
	// Construction at today's pace (wages for 30 working days, slower in frost).
	const FDateTime Now = GetDate();
	double Building = 0.0, Works = 0.0;
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		const int32 Module = Site && !Site->IsPrivate() ? Site->GetActiveModule() : INDEX_NONE;
		if (Module != INDEX_NONE)
		{
			Building += Site->ModuleCostPerDay(Module) * 30.0 * Campaign1851Buildings::WorkRate(Site->ModuleType(Module), Now);
		}
	}
	for (const FCampaign1851Link& L : Links)
	{
		if (L.Work != ECampaign1851LinkWork::None)
		{
			Works += L.WorkCost * (1.0 - Campaign1851Buildings::DownPayment) / FMath::Max(L.WorkDays, 1.f) * 30.0 * Campaign1851Buildings::WorkRate(Campaign1851Network::WorkType(), Now);
		}
	}
	double Training = 0.0;
	for (const FCampaign1851Regiment& R : Regiments)
	{
		Training += R.IsMarching() ? 0.0 : Campaign1851Army::ProgramCostPerMonth(R.Program) * R.Men / 760.0;
	}
	const double RoadUpkeep = NetworkUpkeepPerYear() / 12.0;
	Lines.Add({ TEXT("Byggeri (dagløn)"), -Building });
	Lines.Add({ TEXT("Veje og jernbaner under anlæg"), -Works });
	Lines.Add({ TEXT("Drift af garnisoner og bygninger"), -(GetMonthlyUpkeep() - RoadUpkeep) });
	Lines.Add({ TEXT("Vedligehold af chausséer og jernbaner"), -RoadUpkeep });
	Lines.Add({ TEXT("Officerslønninger"), -OfficerPayPerMonth() });
	Lines.Add({ TEXT("Hærens øvelser"), -Training });
	// The civil buildings' yield, in three lines (fees, the workers' tax, customs and export).
	{
		int32 At = 3;
		if (CivilTradePerYear() > 0.5) { Lines.Insert({ TEXT("Told og eksport (havne, handel, industri)"), CivilTradePerYear() / 12.0 }, At++); }
		if (CivilJobTaxPerYear() > 0.5) { Lines.Insert({ FString::Printf(TEXT("Skat af %d arbejdspladser"), CivilJobs()), CivilJobTaxPerYear() / 12.0 }, At++); }
		if (CivilFeesPerYear() > 0.5) { Lines.Insert({ TEXT("Afgifter, post og gebyrer"), CivilFeesPerYear() / 12.0 }, At++); }
	}
	if (RaisedUpkeepPerMonth() > 0.5)
	{
		Lines.Add({ TEXT("Nye bataljoners underhold"), -RaisedUpkeepPerMonth() });
	}
	if (StockingCostPerMonth() > 0.5)
	{
		Lines.Add({ TEXT("Forråd til depoterne"), -StockingCostPerMonth() });
	}
	if (MobilisedPayPerMonth() > 0.5)
	{
		Lines.Add({ TEXT("De indkaldtes løn og underhold"), -MobilisedPayPerMonth() });
	}
	if (RailwayIncomePerYear() > 0.5) { Lines.Add({ TEXT("Jernbaner: passagerer og fragt"), RailwayIncomePerYear() / 12.0 }); }
	Lines.Add({ TEXT("Told på udførsel (korn, kvæg, smør)"), ExportDutyPerYear() / 12.0 });
	Lines.Add({ TEXT("Øresundstold og handelstraktater"), ForeignIncomePerYear() / 12.0 });
	Lines.Add({ TEXT("Flåden"), -NavyUpkeepPerYear() / 12.0 });
	if (Debt > 0.5)
	{
		Lines.Add({ TEXT("Renter af statsgælden"), -Debt * DebtRate / 12.0 });
	}
	return Lines;
}

double ACampaign1851Map::GetMonthlyUpkeep() const
{
	double Total = 0.0;
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		Total += Site && !Site->IsPrivate() && !Site->IsHistoric() ? Site->GetYearlyUpkeep() / 12.0 : 0.0;
	}
	return Total + (NetworkUpkeepPerYear() + FortUpkeepPerYear()) / 12.0 + BilletUpkeepPerMonth();
}

void ACampaign1851Map::AddTransaction(double Amount, const FString& Text)
{
	Treasury += Amount;
	Ledger.Add({ GetDate(), Amount, Text });
	if (Ledger.Num() > 200)
	{
		Ledger.RemoveAt(0, Ledger.Num() - 200);
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|treasury|%s|%+.0f|%s|balance %.0f"), *FormatDate(GetDate(), true), Amount, *Text, Treasury);
}

void ACampaign1851Map::ResetEconomy()
{
	Treasury = 0.0;
	Ledger.Reset();
	MonthSpend.Reset();
	AddTransaction(StartingTreasury, TEXT("Kassebeholdning ved felttogets start"));
}

void ACampaign1851Map::RestoreEconomy(double InTreasury, const TArray<FCampaign1851Transaction>& InLedger)
{
	Treasury = InTreasury;
	Ledger = InLedger;
	MonthSpend.Reset();
}

bool ACampaign1851Map::CanAfford(int32 CostRd) const
{
	return Treasury >= CostRd * Campaign1851Buildings::DownPayment;
}

void ACampaign1851Map::CloseMonth()
{
	RefillMinistryBudgets();
	// Wages leave the treasury day by day; the account book gets one line per project and month.
	for (const TPair<FString, double>& Spend : MonthSpend)
	{
		if (Spend.Value >= 1.0)
		{
			Treasury += Spend.Value;   // already paid: undo here so booking the line does not charge twice
			AddTransaction(-Spend.Value, Spend.Key);
		}
	}
	MonthSpend.Reset();
	const double RoadUpkeep = NetworkUpkeepPerYear() / 12.0;
	const double Upkeep = GetMonthlyUpkeep() - RoadUpkeep;
	if (Upkeep >= 1.0)
	{
		AddTransaction(-Upkeep, TEXT("Drift af garnisoner og bygninger"));
	}
	double TrainingCost = 0.0;
	for (const FCampaign1851Regiment& R : Regiments)
	{
		TrainingCost += R.IsMarching() ? 0.0 : Campaign1851Army::ProgramCostPerMonth(R.Program) * R.Men / 760.0;
	}
	if (TrainingCost >= 1.0)
	{
		AddTransaction(-TrainingCost, TEXT("Hærens øvelser"));
	}
	const double OfficerSalaries = OfficerPayPerMonth();
	if (OfficerSalaries >= 1.0)
	{
		AddTransaction(-OfficerSalaries, TEXT("Officerslønninger"));
	}
	if (RoadUpkeep >= 1.0)
	{
		AddTransaction(-RoadUpkeep, TEXT("Vedligehold af chausséer og jernbaner"));
	}
	for (const TCHAR* Region : { TEXT("K"), TEXT("S"), TEXT("H") })
	{
		AddTransaction(YearlyTax(Region) / 12.0, FString::Printf(TEXT("Skatter: %s"), *RegionName(Region)));
	}
	if (CivilTradePerYear() > 0.5) { AddTransaction(CivilTradePerYear() / 12.0, TEXT("Told og eksport")); }
	if (CivilJobTaxPerYear() > 0.5) { AddTransaction(CivilJobTaxPerYear() / 12.0, TEXT("Skat af arbejdspladser")); }
	if (CivilFeesPerYear() > 0.5) { AddTransaction(CivilFeesPerYear() / 12.0, TEXT("Afgifter, post og gebyrer")); }
	if (RailwayIncomePerYear() > 0.5) { AddTransaction(RailwayIncomePerYear() / 12.0, TEXT("Jernbaner: passagerer og fragt")); }
	// The world moves on: towns grow, investors build, the ministries (and the other nations) decide.
	GrowMonth();
	MonthlyMateriel();
	MonthlyFooting();
	MonthlyOfficerCareers();
	MonthlyWar();
	MonthlyDiplomacy();
	MonthlyResearch();
	MonthlyNavy();
	MonthlyPolitics();
	MonthlyEconomy();
	MonthlyFortProgrammes();
	CheckCampaignEnd();
	if (GetDate().GetMonth() == 1)
	{
		YearlyOfficers();
	}
	MonthlyBuildingMaterials();
	MonthlyManpower();
	MonthlyNeighbourDistricts();
	MonthlySalvage();
	MonthlySupply();
	PrivateInvestment();
	RunNationalAI();
}

// ------------------------------------------------------------------ amter

bool ACampaign1851Map::LoadAmtIds()
{
	TArray<uint8> Png;
	if (!FFileHelper::LoadFileToArray(Png, *DataPath(TEXT("Denmark1851_Amter.png"))))
	{
		return false;
	}
	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Png.GetData(), Png.Num()) || !Wrapper->GetRaw(ERGBFormat::Gray, 8, AmtIds))
	{
		return false;
	}
	AmtIdsW = Wrapper->GetWidth();
	AmtIdsH = Wrapper->GetHeight();
	return true;
}

int32 ACampaign1851Map::AmtAtWorld(const FVector& World) const
{
    // Amt ids remain Danish ids. Foreign districts use their own raster/legend,
    // never overload this value with 256 + a foreign district index.
    if (bUseMapTiles && !IsMonarchyLand(KmAtWorld(World))) { return 0; }
	if (AmtIds.Num() == 0)
	{
		return 0;
	}
	const FVector Local = GetActorTransform().InverseTransformPosition(World);
	const double U = Local.X / (SizeKm.X * KmToUnits) + 0.5, V = -Local.Y / (SizeKm.Y * KmToUnits) + 0.5;
	if (U < 0.0 || U >= 1.0 || V < 0.0 || V >= 1.0)
	{
		return 0;
	}
	const int32 X = FMath::Clamp(int32(U * AmtIdsW), 0, AmtIdsW - 1), Y = FMath::Clamp(int32((1.0 - V) * AmtIdsH), 0, AmtIdsH - 1);
	return AmtIds[Y * AmtIdsW + X];
}

void ACampaign1851Map::SetHighlightedAmt(int32 Id)
{
	if (SeasonCollection)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, SeasonCollection, TEXT("SelectedAmt"), float(Id > 0 ? Id : -1));
	}
}

double ACampaign1851Map::YearlyTax(const FString& Region) const
{
	double Total = 0.0;
	for (const FCampaign1851Amt& A : Amter)
	{
		Total += (Region.IsEmpty() || A.Region == Region) && !IsAmtOccupied(A) ? A.Rural * RuralTaxRate() * RuralTaxFactor() + A.Urban * UrbanTaxRate() * UrbanTaxFactor() : 0.0;
	}
	Total *= TaxMoodFactor();   // a discontented country pays reluctantly
	return Footing == ECampaign1851Footing::Peace ? Total : Total * Campaign1851Mobilisation::WarTaxFactor;
}

FString ACampaign1851Map::RegionName(const FString& Code)
{
	return Code == TEXT("K") ? TEXT("Kongeriget") : Code == TEXT("S") ? TEXT("Slesvig") : Code == TEXT("H") ? TEXT("Holsten og Lauenborg") : TEXT("Udland");
}
