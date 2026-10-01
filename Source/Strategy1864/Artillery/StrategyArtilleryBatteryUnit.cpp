#include "StrategyArtilleryBatteryUnit.h"

#include "StrategyArtilleryDeploymentComponent.h"
#include "StrategyArtilleryAmmunitionComponent.h"
#include "StrategyArtilleryFireMissionComponent.h"
#include "StrategyArtilleryDamageComponent.h"
#include "StrategyArtilleryCaptureComponent.h"
#include "StrategyArtilleryTraverseComponent.h"
#include "StrategyArtilleryRepairComponent.h"
#include "StrategyArtilleryPositioningComponent.h"
#include "StrategyArtilleryProjectilePresentationComponent.h"
#include "StrategyArtilleryCrewAnimationComponent.h"
#include "../Visual/StrategyUniformAppearanceComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyContactComponent.h"

AStrategyArtilleryBatteryUnit::AStrategyArtilleryBatteryUnit()
{
    Echelon = EStrategyEchelon::Artillery;

    DeploymentComponent =
        CreateDefaultSubobject<UStrategyArtilleryDeploymentComponent>(
            TEXT("ArtilleryDeploymentComponent"));

    ArtilleryAmmunitionComponent =
        CreateDefaultSubobject<UStrategyArtilleryAmmunitionComponent>(
            TEXT("ArtilleryAmmunitionComponent"));

    ArtilleryFireMissionComponent =
        CreateDefaultSubobject<UStrategyArtilleryFireMissionComponent>(
            TEXT("ArtilleryFireMissionComponent"));

    ArtilleryDamageComponent =
        CreateDefaultSubobject<UStrategyArtilleryDamageComponent>(
            TEXT("ArtilleryDamageComponent"));

    ArtilleryCaptureComponent =
        CreateDefaultSubobject<UStrategyArtilleryCaptureComponent>(
            TEXT("ArtilleryCaptureComponent"));

    ArtilleryTraverseComponent =
        CreateDefaultSubobject<UStrategyArtilleryTraverseComponent>(
            TEXT("ArtilleryTraverseComponent"));

    ArtilleryRepairComponent =
        CreateDefaultSubobject<UStrategyArtilleryRepairComponent>(
            TEXT("ArtilleryRepairComponent"));

    ArtilleryPositioningComponent =
        CreateDefaultSubobject<UStrategyArtilleryPositioningComponent>(
            TEXT("ArtilleryPositioningComponent"));

    ProjectilePresentationComponent =
        CreateDefaultSubobject<UStrategyArtilleryProjectilePresentationComponent>(
            TEXT("ProjectilePresentationComponent"));

    CrewAnimationComponent =
        CreateDefaultSubobject<UStrategyArtilleryCrewAnimationComponent>(
            TEXT("CrewAnimationComponent"));
}

void AStrategyArtilleryBatteryUnit::BeginPlay()
{
    Super::BeginPlay();

    Echelon = EStrategyEchelon::Artillery;

    GunCount = FMath::Max(1, GunCount);
    CrewStrength = FMath::Max(0, CrewStrength);
    DriverStrength = FMath::Max(0, DriverStrength);
    HorseStrength = FMath::Max(0, HorseStrength);

    InitialStrength = FMath::Max(1, CrewStrength + DriverStrength);
    CurrentStrength = InitialStrength;

    MaximumFireRangeCm = GunProfile.MaximumRangeCm;

    if (ContactComponent)
    {
        ContactComponent->MaximumAwarenessRangeCm =
            GunProfile.MaximumRangeCm;
    }

    if (MovementExecutor)
    {
        MovementExecutor->MoveSpeedCmPerSecond = 550.0f;
    }

    if (FireControlComponent)
    {
        FireControlComponent->CloseRangeCm = 40000.0f;
        FireControlComponent->MediumRangeCm = 100000.0f;
        FireControlComponent->LongRangeCm = GunProfile.MaximumRangeCm;
        FireControlComponent->SetFirePolicy(EStrategyFirePolicy::Hold);
    }

    if (CombatComponent)
    {
        // Infantry combat loop is not authoritative for artillery ammunition/fire.
        CombatComponent->AmmunitionRounds = 0;
        CombatComponent->MaxAmmunitionRounds = 0;
    }

    if (UniformAppearanceComponent)
    {
        UniformAppearanceComponent->VisualProfile.UniformPresetId =
            TEXT("QA_DK_ARTILLERY");
    }

    RefreshDebugLabel();
}

int32 AStrategyArtilleryBatteryUnit::GetPhysicallyAvailableGunCount() const
{
    return FMath::Max(
        0,
        GunCount - DisabledGunCount - DestroyedGunCount);
}

int32 AStrategyArtilleryBatteryUnit::GetCrewLimitedGunCount() const
{
    const int32 CrewPerGun = FMath::Max(1, GunProfile.CrewRequiredPerGun);
    return FMath::Clamp(CrewStrength / CrewPerGun, 0, GunCount);
}

int32 AStrategyArtilleryBatteryUnit::GetOperationalGunCount() const
{
    return FMath::Min(
        GetPhysicallyAvailableGunCount(),
        GetCrewLimitedGunCount());
}

float AStrategyArtilleryBatteryUnit::GetHorseMobilityFactor() const
{
    if (HorsesRequiredForFullMobility <= 0)
    {
        return 1.0f;
    }

    return FMath::Clamp(
        static_cast<float>(HorseStrength) /
        static_cast<float>(HorsesRequiredForFullMobility),
        0.0f,
        1.0f);
}

float AStrategyArtilleryBatteryUnit::GetTowedMobilityFactor() const
{
    const float DriverFactor =
        DriversRequiredForFullMobility > 0
        ? FMath::Clamp(
            static_cast<float>(DriverStrength) /
            static_cast<float>(DriversRequiredForFullMobility),
            0.0f,
            1.0f)
        : 1.0f;

    return FMath::Min(
        GetHorseMobilityFactor(),
        DriverFactor);
}

bool AStrategyArtilleryBatteryUnit::CanNormalMove() const
{
    return OwnershipState == EStrategyArtilleryOwnershipState::Operational &&
        DeploymentComponent &&
        DeploymentComponent->IsLimbered() &&
        GetTowedMobilityFactor() > 0.05f;
}

bool AStrategyArtilleryBatteryUnit::CanFireBattery() const
{
    return OwnershipState == EStrategyArtilleryOwnershipState::Operational &&
        DeploymentComponent &&
        DeploymentComponent->IsDeployed() &&
        GetOperationalGunCount() > 0;
}

bool AStrategyArtilleryBatteryUnit::AbandonBattery()
{
    if (OwnershipState != EStrategyArtilleryOwnershipState::Operational ||
        UnitState == EStrategyUnitState::Destroyed ||
        UnitState == EStrategyUnitState::Abandoned)
    {
        return false;
    }

    PersonnelEvacuatedOnAbandon =
        FMath::Max(0, CrewStrength + DriverStrength);

    CrewStrength = 0;
    DriverStrength = 0;
    CurrentStrength = 0;

    OwnershipState = EStrategyArtilleryOwnershipState::Abandoned;

    if (DeploymentComponent)
    {
        DeploymentComponent->MobilityState =
            EStrategyArtilleryMobilityState::Abandoned;
    }

    if (MovementExecutor)
    {
        MovementExecutor->StopMovement();
    }

    if (OrderComponent)
    {
        OrderComponent->ClearOrder();
    }

    SetUnitState(EStrategyUnitState::Abandoned);
    RefreshDebugLabel();
    return true;
}

void AStrategyArtilleryBatteryUnit::ApplyBatteryDamage(
    int32 PersonnelLoss,
    int32 HorseLoss,
    int32 GunDisabled,
    int32 GunDestroyed)
{
    const int32 AppliedPersonnel =
        FMath::Clamp(PersonnelLoss, 0, CrewStrength + DriverStrength);

    int32 RemainingPersonnelLoss = AppliedPersonnel;

    const int32 CrewLoss = FMath::Min(CrewStrength, RemainingPersonnelLoss);
    CrewStrength -= CrewLoss;
    RemainingPersonnelLoss -= CrewLoss;

    const int32 DriverLoss =
        FMath::Min(DriverStrength, RemainingPersonnelLoss);
    DriverStrength -= DriverLoss;

    HorseStrength =
        FMath::Max(0, HorseStrength - FMath::Max(0, HorseLoss));

    DestroyedGunCount =
        FMath::Clamp(
            DestroyedGunCount + FMath::Max(0, GunDestroyed),
            0,
            GunCount);

    const int32 MaxDisabled =
        FMath::Max(0, GunCount - DestroyedGunCount);

    DisabledGunCount =
        FMath::Clamp(
            DisabledGunCount + FMath::Max(0, GunDisabled),
            0,
            MaxDisabled);

    CurrentStrength =
        FMath::Max(0, CrewStrength + DriverStrength);

    if (CurrentStrength <= 0)
    {
        OwnershipState = EStrategyArtilleryOwnershipState::Abandoned;
        SetUnitState(EStrategyUnitState::Abandoned);

        if (DeploymentComponent)
        {
            DeploymentComponent->MobilityState =
                EStrategyArtilleryMobilityState::Abandoned;
        }
    }
    else if (GetOperationalGunCount() <= 0)
    {
        SetUnitState(EStrategyUnitState::Disabled);

        if (DeploymentComponent)
        {
            DeploymentComponent->MobilityState =
                EStrategyArtilleryMobilityState::Disabled;
        }
    }

    RefreshDebugLabel();
}
