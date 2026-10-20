#include "StrategyCompanyUnit.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Visual/StrategyInfantryVisualComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"

AStrategyCompanyUnit::AStrategyCompanyUnit()
{
    Echelon = EStrategyEchelon::Company;
    if (FormationComponent)
    {
        FormationComponent->RankCount = 3;
        FormationComponent->ColumnWidth = 3;
    }
    // Quick march: 5.4 km/h. Other unit types retain their own movement speeds.
    if (MovementExecutor) MovementExecutor->MoveSpeedCmPerSecond = 150.0f;

    InfantryVisualComponent =
        CreateDefaultSubobject<UStrategyInfantryVisualComponent>(
            TEXT("InfantryVisualComponent"));
}
