#include "InfantryCompany.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SoldierMeshBuilder.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Default soldier material: vertex colour + livery slots from per-instance custom data.
	const TCHAR* LiveryMaterialPath = TEXT("/Game/Materials/M_Soldier_Livery.M_Soldier_Livery");

	// A fallen soldier lies face down; lift him by half his body depth.
	constexpr float FallenLift = 14.f;

	// Per-instance custom data layout, see Docs/AssetSpec_Infantry.md.
	constexpr int32 NumLiveryFloats = 13;

	// QA value: company column frontage. Not yet fixed in the design manual.
	constexpr int32 ColumnFiles = 8;

	const FLinearColor SelectionYellow(1.f, 0.78f, 0.1f);
	const FLinearColor DannebrogRed(0.6f, 0.02f, 0.03f);
	const FLinearColor RegimentalNavy(0.02f, 0.04f, 0.16f);
	const FLinearColor PoleWood(0.2f, 0.12f, 0.05f);

	// Flat colour material with a "Color" vector parameter; used for placeholders and flags.
	const TCHAR* FlatColourPath = TEXT("/Game/Materials/M_FlatColor.M_FlatColor");

	void SetFlatColour(UMeshComponent* Mesh, const FLinearColor& Colour)
	{
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, FlatColourPath);
		if (!Mesh || !Parent)
		{
			return;
		}
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Parent, Mesh);
		Mid->SetVectorParameterValue(TEXT("Color"), Colour);
		Mesh->SetMaterial(0, Mid);
	}
}

AInfantryCompany::AInfantryCompany()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Soldiers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Soldiers"));
	Soldiers->SetupAttachment(Root);
	Soldiers->NumCustomDataFloats = NumLiveryFloats;
	Soldiers->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Soldiers->SetCollisionResponseToAllChannels(ECR_Ignore);
	Soldiers->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	Fallen = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Fallen"));
	Fallen->SetupAttachment(Root);
	Fallen->NumCustomDataFloats = NumLiveryFloats;
	Fallen->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SelectionFootprint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionFootprint"));
	SelectionFootprint->SetupAttachment(Root);
	SelectionFootprint->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionFootprint->SetCastShadow(false);
	SelectionFootprint->SetVisibility(false);

	NationalColourPole = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NationalColourPole"));
	NationalColourPole->SetupAttachment(Root);
	NationalColour = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NationalColour"));
	NationalColour->SetupAttachment(Root);

	RegimentalColourPole = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RegimentalColourPole"));
	RegimentalColourPole->SetupAttachment(Root);
	RegimentalColour = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RegimentalColour"));
	RegimentalColour->SetupAttachment(Root);

	for (UStaticMeshComponent* Part : { NationalColourPole.Get(), NationalColour.Get(), RegimentalColourPole.Get(), RegimentalColour.Get() })
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));

	Soldiers->SetStaticMesh(CylinderMesh.Object);
	Fallen->SetStaticMesh(CylinderMesh.Object);
	SelectionFootprint->SetStaticMesh(PlaneMesh.Object);
	NationalColourPole->SetStaticMesh(CylinderMesh.Object);
	RegimentalColourPole->SetStaticMesh(CylinderMesh.Object);
	NationalColour->SetStaticMesh(CubeMesh.Object);
	RegimentalColour->SetStaticMesh(CubeMesh.Object);
}

void AInfantryCompany::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildFormation();
}

FVector2D AInfantryCompany::GetFootprintSize() const
{
	const int32 Count = FMath::Max(CurrentStrength, 1);
	switch (Formation)
	{
	case EInfantryFormation::Column:
	{
		const int32 Rows = FMath::DivideAndRoundUp(Count, ColumnFiles);
		return FVector2D(Rows * RankSpacing, ColumnFiles * FileSpacing);
	}
	case EInfantryFormation::Square:
	{
		const int32 FilesPerFace = FMath::DivideAndRoundUp(FMath::DivideAndRoundUp(Count, 4), Ranks);
		const float Side = FilesPerFace * FileSpacing;
		return FVector2D(Side, Side);
	}
	default:
	{
		const int32 Files = FMath::DivideAndRoundUp(Count, Ranks);
		return FVector2D(Ranks * RankSpacing, Files * FileSpacing);
	}
	}
}

TArray<FTransform> AInfantryCompany::ComputeSlots(int32 Count) const
{
	// Local space: +X is the company's front, +Y its right. Line and Square are centred on the
	// root; Column extends backwards from the head of the column.
	TArray<FTransform> Slots;
	Slots.Reserve(Count);

	auto AddSlot = [&Slots](float X, float Y, float Yaw)
	{
		Slots.Add(FTransform(FRotator(0.f, Yaw, 0.f), FVector(X, Y, 0.f)));
	};

	switch (Formation)
	{
	case EInfantryFormation::Column:
	{
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Row = i / ColumnFiles;
			const int32 File = i % ColumnFiles;
			AddSlot(-Row * RankSpacing, (File - (ColumnFiles - 1) * 0.5f) * FileSpacing, 0.f);
		}
		break;
	}
	case EInfantryFormation::Square:
	{
		// Four faces, each Ranks deep, facing outwards.
		const int32 PerFace = FMath::DivideAndRoundUp(Count, 4);
		const int32 FilesPerFace = FMath::DivideAndRoundUp(PerFace, Ranks);
		const float Half = FilesPerFace * FileSpacing * 0.5f;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Face = i / PerFace;
			const int32 InFace = i % PerFace;
			const int32 Rank = InFace / FilesPerFace;
			const int32 File = InFace % FilesPerFace;
			const float Along = (File - (FilesPerFace - 1) * 0.5f) * FileSpacing;
			const float Out = Half - Rank * RankSpacing;
			const float FaceYaw = Face * 90.f;
			const FVector Local = FRotator(0.f, FaceYaw, 0.f).RotateVector(FVector(Out, Along, 0.f));
			AddSlot(Local.X, Local.Y, FaceYaw);
		}
		break;
	}
	default:
	{
		const int32 Files = FMath::DivideAndRoundUp(Count, Ranks);
		const float DepthCentre = (Ranks - 1) * RankSpacing * 0.5f;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Rank = i / Files;
			const int32 File = i % Files;
			AddSlot(DepthCentre - Rank * RankSpacing, (File - (Files - 1) * 0.5f) * FileSpacing, 0.f);
		}
		break;
	}
	}

	FRandomStream Noise(GetUniqueID());
	for (FTransform& Slot : Slots)
	{
		Slot.AddToTranslation(FVector(Noise.FRandRange(-1.f, 1.f), Noise.FRandRange(-1.f, 1.f), 0.f) * PositionJitter);
		Slot.ConcatenateRotation(FRotator(0.f, Noise.FRandRange(-4.f, 4.f), 0.f).Quaternion());
		Slot.SetScale3D(FVector(Noise.FRandRange(0.95f, 1.05f)));
	}
	return Slots;
}

FVector AInfantryCompany::ProjectToGround(const FVector& WorldPoint) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return WorldPoint;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InfantryGround), false, this);
	FHitResult Hit;
	const FVector Start = WorldPoint + FVector(0.f, 0.f, 5000.f);
	const FVector End = WorldPoint - FVector(0.f, 0.f, 20000.f);
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		return Hit.ImpactPoint;
	}
	return WorldPoint;
}

void AInfantryCompany::RebuildFormation()
{
	CurrentStrength = FMath::Clamp(CurrentStrength, 0, FullStrength);

	// Without an authored mesh, use the procedural crowd soldier.
	UStaticMesh* Mesh = SoldierMesh ? SoldierMesh.Get() : Game1864::SoldierMesh::GetInfantry(ResolveSoldierMaterial());
	Soldiers->SetStaticMesh(Mesh);
	Fallen->SetStaticMesh(Mesh);

	Soldiers->ClearInstances();
	const FTransform ActorXf = GetActorTransform();
	const TArray<FTransform> Slots = ComputeSlots(CurrentStrength);

	TArray<FTransform> Instances;
	Instances.Reserve(Slots.Num());
	for (const FTransform& Slot : Slots)
	{
		// Conform every soldier to the terrain under his own slot.
		const FVector Ground = ProjectToGround(ActorXf.TransformPosition(Slot.GetLocation()));
		FTransform Instance = Slot;
		Instance.SetLocation(ActorXf.InverseTransformPosition(Ground));
		Instances.Add(Instance);
	}
	Soldiers->AddInstances(Instances, false, false);

	ApplyLivery();
	PlaceColours();
	UpdateFootprint();
}

void AInfantryCompany::ApplyLivery()
{
	FRandomStream Variation(GetUniqueID() * 31);
	auto WriteLivery = [this, &Variation](UInstancedStaticMeshComponent* Ism)
	{
		const TArray<FLinearColor> Colours = { Livery.Coat, Livery.Trousers, Livery.Facings, Livery.Piping };
		for (int32 Index = 0; Index < Ism->GetInstanceCount(); ++Index)
		{
			TArray<float> Data;
			Data.Reserve(NumLiveryFloats);
			for (const FLinearColor& C : Colours)
			{
				Data.Append({ C.R, C.G, C.B });
			}
			Data.Add(Variation.FRand()); // Slot 12: face/kit variant + wear.
			Ism->SetCustomData(Index, Data, false);
		}
		Ism->MarkRenderStateDirty();
	};
	WriteLivery(Soldiers);
	WriteLivery(Fallen);

	// An authored soldier mesh keeps its own (textured) materials unless a livery material is set explicitly.
	if (SoldierMesh && !SoldierMaterial)
	{
		Soldiers->EmptyOverrideMaterials();
		Fallen->EmptyOverrideMaterials();
		return;
	}

	UMaterialInterface* Material = ResolveSoldierMaterial();
	Soldiers->SetMaterial(0, Material);
	Fallen->SetMaterial(0, Material);
}

UMaterialInterface* AInfantryCompany::ResolveSoldierMaterial() const
{
	return SoldierMaterial ? SoldierMaterial.Get() : LoadObject<UMaterialInterface>(nullptr, LiveryMaterialPath);
}

void AInfantryCompany::PlaceColours()
{
	// The colour party stands centred behind the rear rank, as in the reference.
	const float RearX = -GetFootprintSize().X * 0.5f - 250.f;
	const float PoleHeight = 420.f;
	const FVector PoleScale(0.05f, 0.05f, PoleHeight / 100.f);
	const FVector FlagScale(0.02f, 1.6f, 1.1f);

	auto PlacePair = [&](UStaticMeshComponent* Pole, UStaticMeshComponent* Flag, float Y, const FLinearColor& FlagColour)
	{
		const FVector Base = GetActorTransform().InverseTransformPosition(
			ProjectToGround(GetActorTransform().TransformPosition(FVector(RearX, Y, 0.f))));
		Pole->SetRelativeLocation(Base + FVector(0.f, 0.f, PoleHeight * 0.5f));
		Pole->SetRelativeScale3D(PoleScale);
		Pole->SetVisibility(bShowColours);
		Flag->SetVisibility(bShowColours);

		// Flag (160 x 110 cm) flies from the top of the pole.
		Flag->SetRelativeScale3D(FlagScale);
		Flag->SetRelativeLocation(Base + FVector(0.f, FlagScale.Y * 50.f, PoleHeight - FlagScale.Z * 50.f - 10.f));

		SetFlatColour(Pole, PoleWood);
		SetFlatColour(Flag, FlagColour);
	};

	PlacePair(NationalColourPole, NationalColour, -150.f, DannebrogRed);
	PlacePair(RegimentalColourPole, RegimentalColour, 150.f, RegimentalNavy);
}

void AInfantryCompany::UpdateFootprint()
{
	const FVector2D Size = GetFootprintSize();
	const float Margin = 150.f;
	const float CentreX = Formation == EInfantryFormation::Column ? -Size.X * 0.5f : 0.f;

	const FVector Ground = ProjectToGround(GetActorTransform().TransformPosition(FVector(CentreX, 0.f, 0.f)));
	SelectionFootprint->SetRelativeLocation(GetActorTransform().InverseTransformPosition(Ground) + FVector(0.f, 0.f, 3.f));
	SelectionFootprint->SetRelativeScale3D(FVector((Size.X + Margin) / 100.f, (Size.Y + Margin) / 100.f, 1.f));

	SetFlatColour(SelectionFootprint, SelectionYellow);
	SelectionFootprint->SetVisibility(bSelected);
}

void AInfantryCompany::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	SelectionFootprint->SetVisibility(bSelected);
}

void AInfantryCompany::ApplyCasualties(int32 Count)
{
	FRandomStream Noise(GetUniqueID() + FallenCount * 7919);
	Count = FMath::Min(Count, Soldiers->GetInstanceCount());
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Last = Soldiers->GetInstanceCount() - 1;
		FTransform Standing;
		Soldiers->GetInstanceTransform(Last, Standing, false);
		Soldiers->RemoveInstance(Last);

		// Fall forward or sideways, a little out of the slot.
		FTransform Lying = Standing;
		const FVector Offset(Noise.FRandRange(40.f, 160.f), Noise.FRandRange(-60.f, 60.f), 0.f);
		const FVector Ground = ProjectToGround(GetActorTransform().TransformPosition(Standing.GetLocation() + Offset));
		Lying.SetLocation(GetActorTransform().InverseTransformPosition(Ground) + FVector(0.f, 0.f, FallenLift));
		Lying.SetRotation(FRotator(-90.f, Noise.FRandRange(-180.f, 180.f), 0.f).Quaternion());
		Fallen->AddInstance(Lying, false);
	}

	FallenCount += Count;
	CurrentStrength = Soldiers->GetInstanceCount();
	ApplyLivery();
}
