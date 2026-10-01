#include "StrategyUniformAppearanceComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UStrategyUniformAppearanceComponent::UStrategyUniformAppearanceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyUniformAppearanceComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyAppearance();
}

FStrategyUniformColors
UStrategyUniformAppearanceComponent::GetResolvedColors() const
{
    FStrategyUniformColors Result = BasePreset.Colors;

    if (BasePreset.bLockHistoricalPalette)
    {
        return Result;
    }

    if (Overrides.bOverrideCoat)
    {
        Result.Coat = Overrides.Coat;
    }

    if (Overrides.bOverrideTrousers)
    {
        Result.Trousers = Overrides.Trousers;
    }

    if (Overrides.bOverrideFacings)
    {
        Result.Facings = Overrides.Facings;
    }

    if (Overrides.bOverrideHeadgearDetail)
    {
        Result.HeadgearDetail = Overrides.HeadgearDetail;
    }

    if (Overrides.bOverrideLeather)
    {
        Result.Leather = Overrides.Leather;
    }

    if (Overrides.bOverrideAccent)
    {
        Result.Accent = Overrides.Accent;
    }

    if (Overrides.bOverrideMetal)
    {
        Result.Metal = Overrides.Metal;
    }

    return Result;
}

void UStrategyUniformAppearanceComponent::SetPreset(
    const FStrategyUniformPreset& NewPreset,
    bool bApplyNow)
{
    BasePreset = NewPreset;
    VisualProfile.UniformPresetId = BasePreset.PresetId;

    if (bApplyNow)
    {
        ApplyAppearance();
    }
}

void UStrategyUniformAppearanceComponent::SetOverrides(
    const FStrategyUniformOverrides& NewOverrides,
    bool bApplyNow)
{
    Overrides = NewOverrides;

    if (bApplyNow)
    {
        ApplyAppearance();
    }
}

void UStrategyUniformAppearanceComponent::ClearOverrides(
    bool bApplyNow)
{
    Overrides = FStrategyUniformOverrides();

    if (bApplyNow)
    {
        ApplyAppearance();
    }
}

bool UStrategyUniformAppearanceComponent::ShouldTargetMesh(
    const USkeletalMeshComponent* Mesh) const
{
    if (!Mesh)
    {
        return false;
    }

    if (TargetMeshComponentNames.Num() == 0)
    {
        return true;
    }

    return TargetMeshComponentNames.Contains(Mesh->GetFName());
}

void UStrategyUniformAppearanceComponent::ApplyColorsToMesh(
    USkeletalMeshComponent* Mesh,
    const FStrategyUniformColors& Colors)
{
    if (!Mesh)
    {
        return;
    }

    const int32 MaterialCount = Mesh->GetNumMaterials();

    for (int32 MaterialIndex = 0;
         MaterialIndex < MaterialCount;
         ++MaterialIndex)
    {
        UMaterialInstanceDynamic* MID =
            Mesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex);

        if (!MID)
        {
            continue;
        }

        MID->SetVectorParameterValue(CoatParameter, Colors.Coat);
        MID->SetVectorParameterValue(TrousersParameter, Colors.Trousers);
        MID->SetVectorParameterValue(FacingsParameter, Colors.Facings);
        MID->SetVectorParameterValue(
            HeadgearParameter,
            Colors.HeadgearDetail);
        MID->SetVectorParameterValue(LeatherParameter, Colors.Leather);
        MID->SetVectorParameterValue(AccentParameter, Colors.Accent);
        MID->SetVectorParameterValue(MetalParameter, Colors.Metal);
    }
}

void UStrategyUniformAppearanceComponent::ApplyAppearance()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    const FStrategyUniformColors Colors = GetResolvedColors();

    TArray<USkeletalMeshComponent*> Meshes;
    OwnerActor->GetComponents<USkeletalMeshComponent>(Meshes);

    for (USkeletalMeshComponent* Mesh : Meshes)
    {
        if (ShouldTargetMesh(Mesh))
        {
            ApplyColorsToMesh(Mesh, Colors);
        }
    }
}
