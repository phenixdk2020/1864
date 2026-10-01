#include "StrategyArtilleryRepairComponent.h"

#include "StrategyArtilleryBatteryUnit.h"
#include "StrategyArtilleryDeploymentComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"

UStrategyArtilleryRepairComponent::UStrategyArtilleryRepairComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyArtilleryRepairComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
}

bool UStrategyArtilleryRepairComponent::BeginRepairDisabledGun()
{
    if (!OwnerBattery ||
        OwnerBattery->OwnershipState !=
            EStrategyArtilleryOwnershipState::Operational ||
        OwnerBattery->DisabledGunCount <= 0 ||
        OwnerBattery->CrewStrength < MinimumCrewForRepair ||
        bRepairing ||
        (OwnerBattery->MovementExecutor &&
         OwnerBattery->MovementExecutor->HasMovementGoal()) ||
        OwnerBattery->UnitState == EStrategyUnitState::UnderFire ||
        OwnerBattery->UnitState == EStrategyUnitState::Engaged ||
        OwnerBattery->UnitState == EStrategyUnitState::Routed ||
        OwnerBattery->UnitState == EStrategyUnitState::Abandoned ||
        OwnerBattery->UnitState == EStrategyUnitState::Destroyed)
    {
        return false;
    }

    if (OwnerBattery->DeploymentComponent &&
        !OwnerBattery->DeploymentComponent->IsDeployed() &&
        OwnerBattery->DeploymentComponent->MobilityState !=
            EStrategyArtilleryMobilityState::Disabled)
    {
        return false;
    }

    bRepairing = true;
    RepairRemainingSeconds =
        FMath::Max(0.1f, RepairSecondsPerGun);

    OwnerBattery->SetUnitState(EStrategyUnitState::Reforming);
    SetComponentTickEnabled(true);
    return true;
}

void UStrategyArtilleryRepairComponent::CancelRepair()
{
    bRepairing = false;
    RepairRemainingSeconds = 0.0f;
    SetComponentTickEnabled(false);

    if (OwnerBattery &&
        OwnerBattery->UnitState == EStrategyUnitState::Reforming)
    {
        OwnerBattery->SetUnitState(
            OwnerBattery->GetOperationalGunCount() > 0
            ? EStrategyUnitState::Ready
            : EStrategyUnitState::Disabled);
    }
}

void UStrategyArtilleryRepairComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bRepairing || !OwnerBattery)
    {
        SetComponentTickEnabled(false);
        return;
    }

    if (OwnerBattery->UnitState == EStrategyUnitState::UnderFire ||
        OwnerBattery->UnitState == EStrategyUnitState::Engaged ||
        OwnerBattery->UnitState == EStrategyUnitState::Routed ||
        OwnerBattery->UnitState == EStrategyUnitState::Abandoned ||
        OwnerBattery->UnitState == EStrategyUnitState::Destroyed ||
        (OwnerBattery->MovementExecutor &&
         OwnerBattery->MovementExecutor->HasMovementGoal()))
    {
        CancelRepair();
        return;
    }

    RepairRemainingSeconds =
        FMath::Max(
            0.0f,
            RepairRemainingSeconds - DeltaTime);

    if (RepairRemainingSeconds <= 0.0f)
    {
        CompleteRepair();
    }
}

void UStrategyArtilleryRepairComponent::CompleteRepair()
{
    if (!OwnerBattery || OwnerBattery->DisabledGunCount <= 0)
    {
        CancelRepair();
        return;
    }

    --OwnerBattery->DisabledGunCount;

    OwnerBattery->Fatigue =
        FMath::Clamp(
            OwnerBattery->Fatigue + RepairFatigueCost,
            0.0f,
            100.0f);

    if (OwnerBattery->DeploymentComponent &&
        OwnerBattery->DeploymentComponent->MobilityState ==
            EStrategyArtilleryMobilityState::Disabled)
    {
        OwnerBattery->DeploymentComponent->MobilityState =
            EStrategyArtilleryMobilityState::Deployed;
    }

    bRepairing = false;
    RepairRemainingSeconds = 0.0f;

    OwnerBattery->SetUnitState(
        OwnerBattery->GetOperationalGunCount() > 0
        ? EStrategyUnitState::Ready
        : EStrategyUnitState::Disabled);

    OwnerBattery->RefreshDebugLabel();
    SetComponentTickEnabled(false);
}
