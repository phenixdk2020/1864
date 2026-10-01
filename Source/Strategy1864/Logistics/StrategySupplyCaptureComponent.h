#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategySupplyCaptureComponent.generated.h"

class AStrategySupplyWagonUnit;
class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategySupplyCaptureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategySupplyCaptureComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply|Capture")
    float CaptureRadiusCm = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply|Capture")
    float CaptureHoldSeconds = 6.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Supply|Capture")
    float CaptureProgressSeconds = 0.0f;

private:
    AStrategyUnit* FindCapturingEnemy() const;
    bool HasFriendlyProtection() const;
    void CompleteCapture(AStrategyUnit* Captor);

    UPROPERTY()
    TObjectPtr<AStrategySupplyWagonUnit> OwnerWagon;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> ActiveCaptor;
};
