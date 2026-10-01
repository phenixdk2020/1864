#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyOfficerAIComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyOfficerAIComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyOfficerAIComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|AI")
    float EvaluationIntervalSeconds = 0.35f;

    UFUNCTION(BlueprintCallable, Category="Strategy|AI")
    void SetAIEnabled(bool bEnabled, bool bCascadeToSubordinates = true);

    UFUNCTION(BlueprintPure, Category="Strategy|AI")
    bool IsAIEnabled() const;

private:
    void EvaluateInheritedMission();
    bool CanAcceptInheritedMission() const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float EvaluationAccumulator = 0.0f;
    int32 LastInheritedParentOrderSerial = 0;
};
