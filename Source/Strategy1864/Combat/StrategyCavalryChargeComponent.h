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
    float ImpactLossPerRider = 0.18f; // Tunable balance estimate, not a verified historical rate.
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float FlankShockFactor = 1.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float FlankImpactMultiplier = 1.8f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float RearImpactMultiplier = 2.5f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float ColumnImpactMultiplier = 1.4f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float BrokenSquareImpactMultiplier = 1.8f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float SquareRepelDistanceCm = 4000.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float SteadySquareMorale = 50.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float SteadySquareCohesion = 45.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float MomentumBuildDistanceCm = 12000.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float ImpactMoraleShock = 45.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float ImpactCohesionShock = 55.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Cavalry Charge")
    float RepulseMoraleLoss = 18.0f;
    UPROPERTY(VisibleAnywhere, Category="Strategy|Cavalry Charge")
    float ChargeConfidence = 0.0f;
    UPROPERTY(VisibleAnywhere, Category="Strategy|Cavalry Charge")
    float ChargeMomentum = 0.0f;
    UPROPERTY(VisibleAnywhere, Category="Strategy|Cavalry Charge")
    bool bLastChargeRepulsed = false;

    bool IsSteadySquare(const AStrategyUnit* Target) const;
    void ResolveImpact(AStrategyUnit* Target);
    void RepelCharge(AStrategyUnit* Target);

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
