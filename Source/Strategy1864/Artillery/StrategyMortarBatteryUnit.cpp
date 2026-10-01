#include "StrategyMortarBatteryUnit.h"
#include "StrategyMortarDeploymentComponent.h"
#include "StrategyMortarFireComponent.h"

AStrategyMortarBatteryUnit::AStrategyMortarBatteryUnit()
{
    MortarDeploymentComponent =
        CreateDefaultSubobject<UStrategyMortarDeploymentComponent>(
            TEXT("MortarDeploymentComponent"));

    MortarFireComponent =
        CreateDefaultSubobject<UStrategyMortarFireComponent>(
            TEXT("MortarFireComponent"));

    DisplayName = FText::FromString(TEXT("Mortar Battery"));
}

bool AStrategyMortarBatteryUnit::IsMortarCombatReady() const
{
    return IsCombatEffective() &&
        MortarDeploymentComponent &&
        MortarDeploymentComponent->CanFire() &&
        MortarFireComponent &&
        MortarFireComponent->AmmunitionBombs > 0 &&
        GetOperationalMortarCount() > 0;
}

int32 AStrategyMortarBatteryUnit::GetOperationalMortarCount() const
{
    if (MortarCrewStrength <= 0 || MortarPieceCount <= 0)
    {
        return 0;
    }

    const int32 CrewLimited = FMath::Max(1, MortarCrewStrength / 8);
    return FMath::Min(MortarPieceCount, CrewLimited);
}
