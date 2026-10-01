#include "StrategySupplyWagonUnit.h"

#include "StrategySupplyCargoComponent.h"
#include "StrategySupplyCaptureComponent.h"
#include "StrategySupplyComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"

AStrategySupplyWagonUnit::AStrategySupplyWagonUnit()
{
    Echelon = EStrategyEchelon::Supply;

    CargoComponent =
        CreateDefaultSubobject<UStrategySupplyCargoComponent>(
            TEXT("SupplyCargoComponent"));

    SupplyCaptureComponent =
        CreateDefaultSubobject<UStrategySupplyCaptureComponent>(
            TEXT("SupplyCaptureComponent"));
}

void AStrategySupplyWagonUnit::BeginPlay()
{
    Super::BeginPlay();

    Echelon = EStrategyEchelon::Supply;

    DriverStrength = FMath::Max(0, DriverStrength);
    HorseStrength = FMath::Max(0, HorseStrength);

    InitialStrength = FMath::Max(1, DriverStrength);
    CurrentStrength = DriverStrength;

    if (MovementExecutor)
    {
        MovementExecutor->MoveSpeedCmPerSecond = 450.0f;
    }

    if (FireControlComponent)
    {
        FireControlComponent->SetFirePolicy(EStrategyFirePolicy::Hold);
    }

    if (CombatComponent)
    {
        CombatComponent->AmmunitionRounds = 0;
        CombatComponent->MaxAmmunitionRounds = 0;
    }

    if (SupplyComponent)
    {
        SupplyComponent->bActsAsSupplySource = true;
        SupplyComponent->StoredAmmunitionRounds = 0;
        SupplyComponent->MaxStoredAmmunitionRounds = 0;
        SupplyComponent->ResupplyRadiusCm = 2500.0f;
        SupplyComponent->TransferRoundsPerSecond = 90.0f;
    }

    RefreshDebugLabel();
}

float AStrategySupplyWagonUnit::GetMobilityFactor() const
{
    const float DriverFactor =
        DriversRequiredForFullMobility > 0
        ? FMath::Clamp(
            static_cast<float>(DriverStrength) /
            static_cast<float>(DriversRequiredForFullMobility),
            0.0f,
            1.0f)
        : 1.0f;

    const float HorseFactor =
        HorsesRequiredForFullMobility > 0
        ? FMath::Clamp(
            static_cast<float>(HorseStrength) /
            static_cast<float>(HorsesRequiredForFullMobility),
            0.0f,
            1.0f)
        : 1.0f;

    const float ConditionFactor =
        FMath::Clamp(WagonCondition / 100.0f, 0.0f, 1.0f);

    return FMath::Min3(
        DriverFactor,
        HorseFactor,
        ConditionFactor);
}

bool AStrategySupplyWagonUnit::CanMoveSupplyWagon() const
{
    return OwnershipState == EStrategySupplyOwnershipState::Operational &&
        GetMobilityFactor() > 0.05f &&
        UnitState != EStrategyUnitState::Abandoned &&
        UnitState != EStrategyUnitState::Destroyed;
}

void AStrategySupplyWagonUnit::ApplySupplyDamage(
    int32 DriverLoss,
    int32 HorseLoss,
    float WagonDamage,
    float CargoLossFraction)
{
    DriverStrength =
        FMath::Max(
            0,
            DriverStrength - FMath::Max(0, DriverLoss));

    HorseStrength =
        FMath::Max(
            0,
            HorseStrength - FMath::Max(0, HorseLoss));

    WagonCondition =
        FMath::Clamp(
            WagonCondition - FMath::Max(0.0f, WagonDamage),
            0.0f,
            100.0f);

    if (CargoComponent && CargoLossFraction > 0.0f)
    {
        CargoComponent->ApplyCargoLossFraction(CargoLossFraction);
    }

    CurrentStrength = DriverStrength;

    if (WagonCondition <= 0.0f)
    {
        OwnershipState = EStrategySupplyOwnershipState::Destroyed;
        SetUnitState(EStrategyUnitState::Destroyed);

        if (CargoComponent)
        {
            CargoComponent->ApplyCargoLossFraction(0.75f);
        }
    }
    else if (DriverStrength <= 0)
    {
        AbandonSupplyWagon();
    }

    RefreshDebugLabel();
}

bool AStrategySupplyWagonUnit::AbandonSupplyWagon()
{
    if (OwnershipState == EStrategySupplyOwnershipState::Destroyed ||
        OwnershipState == EStrategySupplyOwnershipState::Abandoned)
    {
        return false;
    }

    OwnershipState = EStrategySupplyOwnershipState::Abandoned;
    CurrentStrength = 0;

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
