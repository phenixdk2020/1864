#include "Campaign1851Map.h"

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
	bReady = true;
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
			C.bForeign = bForeign;
			Cities.Add(C);
		}
	};
	ReadCities(Json->GetArrayField(TEXT("cities")), false);
	ReadCities(Json->GetArrayField(TEXT("foreignCities")), true);

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
	TArray<FVertexInstanceID> Instances;
	Instances.Reserve(VX * VY);
	for (int32 j = 0; j < VY; ++j)
	{
		for (int32 i = 0; i < VX; ++i)
		{
			const FVector2D Uv(double(i) / GridX, double(j) / GridY);
			const FVertexID V = Mesh.CreateVertex();
			Positions[V] = FVector3f(
				float((Uv.X - 0.5) * SizeKm.X * KmToUnits),
				float(-(Uv.Y - 0.5) * SizeKm.Y * KmToUnits),
				SampleHeight01(Uv) * HeightRangeKm * float(KmToUnits));
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
		const FTransform Xf(FQuat::Identity, City.World + FVector(0.0, 0.0, DiameterUnits * 0.35), FVector(DiameterUnits / 100.f));
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
