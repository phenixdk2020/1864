#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyHQFollowComponent.generated.h"

class AStrategyHQUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyHQFollowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyHQFollowComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|HQ Follow")
    bool bEnableFollow = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|HQ Follow")
    float FollowSpeedCmPerSecond = 620.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|HQ Follow")
    float SettleToleranceCm = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|HQ Follow")
    float RearOffsetCm = 6500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|HQ Follow")
    float LateralOffsetCm = 0.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|HQ Follow")
    FVector CalculateDesiredHQPosition() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|HQ Follow")
    void ApplyLevelDefaults();

private:
    UPROPERTY()
    TObjectPtr<AStrategyHQUnit> OwnerHQ;
};
