#include "StrategyCompanyUnit.h"
#include "../Visual/StrategyInfantryVisualComponent.h"

AStrategyCompanyUnit::AStrategyCompanyUnit()
{
    Echelon = EStrategyEchelon::Company;

    InfantryVisualComponent =
        CreateDefaultSubobject<UStrategyInfantryVisualComponent>(
            TEXT("InfantryVisualComponent"));
}
