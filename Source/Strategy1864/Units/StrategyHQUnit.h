#pragma once

#include "CoreMinimal.h"
#include "StrategyUnit.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyHQUnit.generated.h"

class UStrategyParentFormationPlannerComponent;
class UStrategyHQFollowComponent;
class UStrategyCommandZoneComponent;
class UStrategyCavalryTaskingComponent;

UENUM(BlueprintType)
enum class EStrategyHQLevel : uint8
{
    Battalion,
    Regiment,
    Brigade,
    Division
};

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyHQUnit : public AStrategyUnit
{
    GENERATED_BODY()

public:
    AStrategyHQUnit();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|HQ")
    EStrategyHQLevel HQLevel = EStrategyHQLevel::Battalion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|HQ")
    float CommandInnerRadius = 32000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|HQ")
    float CommandOuterRadius = 45000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|HQ")
    TObjectPtr<UStrategyParentFormationPlannerComponent> FormationPlanner;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|HQ")
    TObjectPtr<UStrategyHQFollowComponent> HQFollowComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|HQ")
    TObjectPtr<UStrategyCommandZoneComponent> CommandZoneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|HQ")
    TObjectPtr<UStrategyCavalryTaskingComponent> CavalryTaskingComponent;

    UFUNCTION(BlueprintCallable, Category="Strategy|HQ")
    void ApplyHQLevelDefaults();

private:
    UFUNCTION()
    void HandleHQOrderChanged(const FStrategyOrder& NewOrder);
};
