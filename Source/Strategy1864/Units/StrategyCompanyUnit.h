#pragma once

#include "CoreMinimal.h"
#include "StrategyUnit.h"
#include "StrategyCompanyUnit.generated.h"

class UStrategyInfantryVisualComponent;

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyCompanyUnit : public AStrategyUnit
{
    GENERATED_BODY()

public:
    AStrategyCompanyUnit();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Company")
    int32 CompanyNumber = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Visual")
    TObjectPtr<UStrategyInfantryVisualComponent> InfantryVisualComponent;
};
