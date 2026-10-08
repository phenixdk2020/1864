#include "StrategyArtilleryVisualComponent.h"

#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyMortarBatteryUnit.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
    // The models as imported (Content/Python/import_units_1864.py): about 2 m long. The gun's muzzle points
    // along +Y, the mortar's along -X; a field gun with its trail is some 4.5 m.
    constexpr float CannonScale = 2.2f;
    constexpr float CannonYaw = -90.0f;
    constexpr float MortarScale = 1.1f;
    constexpr float MortarYaw = 180.0f;
}

UStrategyArtilleryVisualComponent::UStrategyArtilleryVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.f;
}

void UStrategyArtilleryVisualComponent::BeginPlay()
{
    Super::BeginPlay();
    AStrategyArtilleryBatteryUnit* Battery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
    bMortar = Cast<AStrategyMortarBatteryUnit>(Battery) != nullptr;
    if (Battery && Battery->MovementExecutor) AddTickPrerequisiteComponent(Battery->MovementExecutor);
    GunMesh = LoadObject<UStaticMesh>(nullptr, bMortar ? TEXT("/Game/Units/Items/SM_Mortar_1864.SM_Mortar_1864") : TEXT("/Game/Units/Items/SM_Cannon_1864.SM_Cannon_1864"));
    if (!Battery || !GunMesh)
    {
        SetComponentTickEnabled(false);
        return;
    }
    Guns = NewObject<UInstancedStaticMeshComponent>(Battery, TEXT("BatteryGuns"));
    Guns->SetStaticMesh(GunMesh);
    Guns->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Guns->SetCastShadow(true);
    Guns->bVisibleInRayTracing = false;
    Guns->SetupAttachment(Battery->GetRootComponent());
    Guns->RegisterComponent();
    if (Battery->QAPlaceholderMesh)
    {
        Battery->QAPlaceholderMesh->SetVisibility(false, true);
    }
    Rebuild();
    if (Battery) ArtilleryVisualPath.Reset(Battery->GetActorTransform());
}

void UStrategyArtilleryVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    Rebuild();
    if (!Guns || !GetOwner() || DeltaTime <= 0.f) return;
    ArtilleryVisualPath.Advance(GetOwner()->GetActorTransform(), DeltaTime, false, 160.f);
    Guns->SetWorldLocation(ArtilleryVisualPath.Center);
    Guns->SetWorldRotation(FRotator(0.f, ArtilleryVisualPath.Facing, 0.f));
    for (int32 GunIndex = 0; GunIndex < GunGoals.Num(); ++GunIndex)
    {
        FTransform GunShown;
        if (!Guns->GetInstanceTransform(GunIndex, GunShown, false)) continue;
        FVector GunPosition = GunShown.GetLocation();
        FStrategyVisualFormationPath::SmoothTravel(GunPosition, GunVelocities[GunIndex], GunGoals[GunIndex].GetLocation(), DeltaTime, 420.f);
        FRotator GunRotation = GunShown.Rotator();
        GunRotation.Yaw = FStrategyVisualFormationPath::SmoothFacing(GunRotation.Yaw, GunTurnVelocities[GunIndex], GunGoals[GunIndex].Rotator().Yaw, DeltaTime, 120.f);
        const float GunAlpha = 1.f - FMath::Exp(-12.f * DeltaTime);
        GunRotation.Roll = FMath::Lerp(GunRotation.Roll, GunGoals[GunIndex].Rotator().Roll, GunAlpha);
        GunShown.SetLocation(GunPosition);
        GunShown.SetRotation(GunRotation.Quaternion());
        GunShown.SetScale3D(FMath::Lerp(GunShown.GetScale3D(), GunGoals[GunIndex].GetScale3D(), GunAlpha));
        Guns->UpdateInstanceTransform(GunIndex, GunShown, false, false, false);
    }
    Guns->MarkRenderStateDirty();
    ArtilleryVisualPath.PreviousUnit = GetOwner()->GetActorTransform();
}

void UStrategyArtilleryVisualComponent::Rebuild()
{
    const AStrategyArtilleryBatteryUnit* Battery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
    if (!Battery || !Guns)
    {
        return;
    }
    // The QA box comes back when the unit refreshes its label: keep it hidden while the guns stand there.
    if (Battery->QAPlaceholderMesh && Battery->QAPlaceholderMesh->IsVisible())
    {
        Battery->QAPlaceholderMesh->SetVisibility(false, true);
    }
    const AStrategyMortarBatteryUnit* Mortars = Cast<AStrategyMortarBatteryUnit>(Battery);
    const int32 Total = Mortars ? FMath::Max(1, Mortars->MortarPieceCount) : FMath::Max(1, Battery->GunCount);
    const int32 Destroyed = Mortars ? 0 : FMath::Clamp(Battery->DestroyedGunCount, 0, Total);
    const int32 Working = Mortars ? FMath::Clamp(Mortars->GetOperationalMortarCount(), 0, Total) : FMath::Clamp(Battery->GetOperationalGunCount(), 0, Total);
    const bool bLimbered = Battery->CanNormalMove();
    if (Total == BuiltTotal && Working == BuiltWorking && Destroyed == BuiltDestroyed && bLimbered == bBuiltLimbered)
    {
        return;
    }
    BuiltTotal = Total;
    BuiltWorking = Working;
    BuiltDestroyed = Destroyed;
    bBuiltLimbered = bLimbered;
    // The QA box is the unit's own scale; the guns stand at world size.
    const FVector OwnerScale = Battery->GetActorScale3D();
    const FVector Unscale(1.0f / FMath::Max(0.01f, OwnerScale.X), 1.0f / FMath::Max(0.01f, OwnerScale.Y), 1.0f / FMath::Max(0.01f, OwnerScale.Z));
    const float Scale = bMortar ? MortarScale : CannonScale;
    const float BaseYaw = bMortar ? MortarYaw : CannonYaw;
    TArray<FTransform> Instances;
    for (int32 g = 0; g < Total; ++g)
    {
        const bool bDestroyed = g >= Total - Destroyed;
        const bool bDisabled = !bDestroyed && g >= Working;
        FVector Place;
        float Yaw = BaseYaw;
        if (bLimbered && !bDestroyed)
        {
            // On the march: one behind the other, the muzzle to the rear (the limber pulls by the trail).
            Place = FVector(-g * ColumnIntervalCm, 0.0f, 0.0f);
            Yaw += 180.0f;
        }
        else
        {
            Place = FVector(0.0f, (g - (Total - 1) * 0.5f) * LineIntervalCm, 0.0f);
        }
        FRotator Rotation(0.0f, Yaw + (bDisabled ? 14.0f : 0.0f), bDestroyed ? 38.0f : 0.0f);
        Instances.Emplace(Rotation, Place * Unscale, Unscale * Scale);
    }
    GunGoals = Instances;
    GunVelocities.SetNumZeroed(Instances.Num());
    GunTurnVelocities.SetNumZeroed(Instances.Num());
    if (Guns->GetInstanceCount() != Instances.Num())
    {
        Guns->ClearInstances();
        Guns->AddInstances(Instances, false, false);
    }
}
