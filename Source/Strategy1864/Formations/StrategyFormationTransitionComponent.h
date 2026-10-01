#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyFormationTypes.h"
#include "StrategyFormationTransitionComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyFormationTransitionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyFormationTransitionComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float BaseReformSeconds = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float SecondsPer100Men = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Formation")
    float ReformRemainingSeconds = 0.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Formation")
    bool IsReforming() const { return ReformRemainingSeconds > 0.0f; }

private:
    UFUNCTION()
    void HandleFormationChanged(
        EStrategyFormationType OldFormation,
        EStrategyFormationType NewFormation);

    void CompleteReform();

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
