#include "Campaign1851Map.h"

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
#include "Engine/StaticMesh.h"
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
	constexpr int32 GridY = 342;

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
	PrimaryActorTick.bCanEverTick = false;

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
	if (bFeatures)
	{
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
	ResetWorld(1851, 0.f);
	if (LoadArmy())
	{
		LoadOfficers();
		ResetArmy();
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|ready|cities=%d|labels=%d|size=%.0fx%.0f km"), Cities.Num(), Labels.Num(), SizeKm.X, SizeKm.Y);
}

void ACampaign1851Map::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
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
	ReadExtent(Json->GetObjectField(TEXT("bornholmKm")), Bornholm);
	SizeKm = FVector2D(Extent.XMax - Extent.XMin, Extent.YMax - Extent.YMin);
	Json->TryGetNumberField(TEXT("detailTileKm"), DetailTileKm);

	auto ReadCities = [this](const TArray<TSharedPtr<FJsonValue>>& Array, bool bForeign)
	{
		for (const TSharedPtr<FJsonValue>& Value : Array)
		{
			const TSharedPtr<FJsonObject> O = Value->AsObject();
			FCampaign1851City C;
			C.Name = O->GetStringField(TEXT("name"));
			C.Lat = O->GetNumberField(TEXT("lat"));
			C.Lon = O->GetNumberField(TEXT("lon"));
			C.Population = int32(O->GetNumberField(TEXT("pop")));
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
	const double Z = SampleHeight01(Uv) * HeightRangeKm * KmToUnits;
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
	FMeshDescription Mesh;
	FStaticMeshAttributes Attributes(Mesh);
	Attributes.Register();
	Attributes.GetVertexInstanceUVs().SetNumChannels(1);
	const FPolygonGroupID Group = Mesh.CreatePolygonGroup();

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
			UVs.Set(I, 0, FVector2f(float(Uv.X), float(1.0 - Uv.Y)));  // texture row 0 = north
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
		Mesh.CreateTriangle(Group, Corners);
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
	const int32 X = int32(Uv.X * FeaturesW), Y = int32((1.0 - Uv.Y) * FeaturesH);  // row 0 = north
	return Features[FMath::Clamp(Y, 0, FeaturesH - 1) * FeaturesW + FMath::Clamp(X, 0, FeaturesW - 1)].R > 127;
}

float ACampaign1851Map::Woodland(const FVector2D& Km) const
{
	const FVector2D Uv = UvFromKm(Km);
	if (Uv.X < 0.0 || Uv.X >= 1.0 || Uv.Y < 0.0 || Uv.Y >= 1.0)
	{
		return 0.f;
	}
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
	struct FTown { FVector2D Km; float RadiusKm; };
	TArray<FTown> Towns;
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
		if (Pop >= 2500.f)
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

		const float Street = Rng.FRandRange(0.f, 90.f);
		const int32 Churches = FMath::Clamp(1 + int32(Pop / 12000.f), 1, 6);
		for (int32 c = 0, Tries = 0; c < Churches && Tries < 400; ++Tries)
		{
			const float D = (c == 0 ? 0.15f + Tries * 0.02f : Radius * 0.6f * FMath::Sqrt(Rng.FRand()));
			const float A = Rng.FRandRange(0.f, UE_TWO_PI);
			const FVector2D Km = Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * D;
			if (IsMonarchyLand(Km) && IsFree(Km, 0.12f))
			{
				Place(EPiece::Church, Km, Street + (Rng.FRand() < 0.5f ? 0.f : 180.f), City.bCapital ? 1.25f : 1.05f);
				Take(Km, 0.12f);
				++c;
			}
		}

		const int32 Houses = FMath::Clamp(int32(Pop / 22.f), 45, 2600);
		for (int32 h = 0, Tries = 0; h < Houses && Tries < Houses * 8; ++Tries)
		{
			const float D = Radius * Rng.FRand();  // uniform radius = dense core, thinning suburbs
			const float A = Rng.FRandRange(0.f, UE_TWO_PI);
			const FVector2D Km = Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * D;
			if (!IsMonarchyLand(Km) || !IsFree(Km, 0.04f))
			{
				continue;
			}
			const float Core = 1.f - D / Radius;  // taller houses in the centre
			// Merchants' brick houses crowd the core; plastered and half-timbered houses everywhere else.
			const float Kind = Rng.FRand();
			const EPiece Piece = Core > 0.55f && Kind < 0.35f ? EPiece::MerchantHouse
				: Kind < 0.55f ? EPiece::TownHouse : Kind < 0.75f ? EPiece::TownHouseOchre : EPiece::TownHouseTimber;
			// Plain houses grow a little taller towards the core; the brick merchants' houses are tall already.
			Place(Piece, Km, Street + (Rng.FRand() < 0.5f ? 0.f : 90.f) + Rng.FRandRange(-5.f, 5.f),
				Rng.FRandRange(0.8f, 1.1f), Piece == EPiece::MerchantHouse ? Rng.FRandRange(0.9f, 1.05f) : Rng.FRandRange(0.85f, 1.05f) + 0.25f * Core);
			Take(Km, 0.04f);
			++h;
		}
		// Windmills on the town's edge, where the wind comes off the fields.
		const int32 Mills = FMath::Clamp(1 + int32(Pop / 6000.f), 1, 4);
		for (int32 m = 0, Tries = 0; m < Mills && Tries < 60; ++Tries)
		{
			const float A = Rng.FRandRange(0.f, UE_TWO_PI);
			const FVector2D Km = Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * (Radius + Rng.FRandRange(0.3f, 1.2f));
			if (IsMonarchyLand(Km) && IsFree(Km, 0.06f))
			{
				Place(EPiece::Windmill, Km, Rng.FRandRange(-30.f, 30.f), Rng.FRandRange(0.9f, 1.1f));   // sails to the west wind
				Take(Km, 0.06f);
				++m;
			}
		}
	}
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
					Place(bFarm ? EPiece::Farm : EPiece::Cottage, P, Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.8f, 1.1f));
					Take(P, R);
					++i;
				}
			}
		}
		else if (Site.Kind == ESite::Farm && IsFree(Km, 0.1f))
		{
			Place(EPiece::Farm, Km, Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.8f, 1.05f));
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
		Part->SetStaticMesh(Campaign1851Scenery::Build(Piece, Material));
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
	for (const TArray<FVector2D>& Line : RoadLines)
	{
		Dense.Add(Resample(Line, 0.2));
	}
	Roads = BuildRibbons(Dense, 0.13f, Srgb(176, 136, 92), TEXT("Roads"), Material);
	Dense.Reset();
	for (const TArray<FVector2D>& Line : LaneLines)
	{
		Dense.Add(Resample(Line, 0.2));
	}
	Lanes = BuildRibbons(Dense, 0.075f, Srgb(160, 126, 88), TEXT("Lanes"), Material);
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
	if (!Cities.IsValidIndex(CityIndex) || !Cities[CityIndex].bHasPlot || (bCharge && !CanAfford(Garrison[0].Cost())))
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
	return Site;
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
	return true;
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
	float MinD = R + 0.2f, MaxD = R + 1.2f;
	switch (Def.Rule)
	{
	case ECampaign1851PlotRule::InTown:  MinD = 0.25f * R; MaxD = 0.85f * R; break;
	case ECampaign1851PlotRule::Outside: MinD = R + 1.2f; MaxD = R + 3.f; break;
	case ECampaign1851PlotRule::Shore:   MinD = 0.2f * R; MaxD = R + 3.f; break;
	default: break;
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
	ClearScenery(Km, Def->PlotRadiusKm);
	ACampaign1851ConstructionSite* Site = SpawnSite(CityIndex, Km, Yaw, { *Def }, false, FVector2D(Def->Length * 0.5 + 2.5, Def->Width * 0.5 + 2.5));
	if (!Site)
	{
		return Fail(TEXT("Byggepladsen kunne ikke oprettes"));
	}
	if (bCharge)
	{
		AddTransaction(-Def->Cost() * Campaign1851Buildings::DownPayment, FString::Printf(TEXT("Materialer bestilt: %s, %s"), *Cities[CityIndex].Name, *Def->Name.ToLower()));
		UseMaterials(Km, Def->Cost(), FString::Printf(TEXT("%s, %s"), *Cities[CityIndex].Name, *Def->Name.ToLower()));
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|project|%s|%s started at %.1f,%.1f km"), *Cities[CityIndex].Name, *Def->Name, Km.X, Km.Y);
	return Site;
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
	if (!bReady)
	{
		return;
	}
	const int32 MonthBefore = GetDate().GetMonth();
	// The clock ticks in whole minutes; the work of those minutes is done at once.
	double Minutes = 0.0;
	if (Speed >= 6)
	{
		// Speed 6: exactly one day per beat, in an even rhythm (1, 2, 3 ...). A slow frame never jumps two
		// days at once: time that piled up is dropped rather than caught up.
		MinuteCarry += DeltaSeconds;
		if (MinuteCarry >= DaySeconds)
		{
			Minutes = 1440.0;
			MinuteCarry = FMath::Min(MinuteCarry - DaySeconds, double(DaySeconds) * 0.5);
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
	if (CivilIncomePerYear() > 0.5)
	{
		Lines.Insert({ TEXT("Erhverv, told og post"), CivilIncomePerYear() / 12.0 }, 3);
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
	return Total + (NetworkUpkeepPerYear() + FortUpkeepPerYear()) / 12.0;
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
	if (CivilIncomePerYear() > 0.5)
	{
		AddTransaction(CivilIncomePerYear() / 12.0, TEXT("Erhverv, told og post"));
	}
	// The world moves on: towns grow, investors build, the ministries (and the other nations) decide.
	GrowMonth();
	MonthlyMateriel();
	MonthlyFooting();
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
		Total += (Region.IsEmpty() || A.Region == Region) && !IsAmtOccupied(A) ? AmtYearlyTax(A) : 0.0;
	}
	Total *= TaxMoodFactor();   // a discontented country pays reluctantly
	return Footing == ECampaign1851Footing::Peace ? Total : Total * Campaign1851Mobilisation::WarTaxFactor;
}

FString ACampaign1851Map::RegionName(const FString& Code)
{
	return Code == TEXT("K") ? TEXT("Kongeriget") : Code == TEXT("S") ? TEXT("Slesvig") : Code == TEXT("H") ? TEXT("Holsten og Lauenborg") : TEXT("Udland");
}
