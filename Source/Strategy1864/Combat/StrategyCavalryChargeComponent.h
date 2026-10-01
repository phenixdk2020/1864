#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyCavalryChargeComponent.generated.h"

class ACavalryUnit;
class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyCavalryChargeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyCavalryChargeComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry Charge")
    float ChargeSpeedCmPerSecond = 1100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry Charge")
    float ContactRadiusCm = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry Charge")
    int32 ChargeImpactCasualties = 2;

    UFUNCTION(BlueprintPure, Category="Strategy|Cavalry Charge")
    bool IsChargeActive() const { return bChargeActive; }

private:
    UFUNCTION()
    void HandleOrderChanged(const FStrategyOrder& NewOrder);

    void BeginCharge();
    void EndCharge();
    AStrategyUnit* DetectEnemyContact(
        const FVector& From,
        const FVector& To,
        FVector& OutContactLocation) const;

    UPROPERTY()
    TObjectPtr<ACavalryUnit> OwnerCavalry;

    FVector PreviousLocation = FVector::ZeroVector;
    float PreviousMoveSpeed = 0.0f;
    bool bChargeActive = false;
};
