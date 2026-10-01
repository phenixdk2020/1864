#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyMortarFireComponent.generated.h"

class AStrategyUnit;
class AStrategyDefensivePosition;
class UStrategyMortarDeploymentComponent;

UENUM(BlueprintType)
enum class EStrategyMortarFireMode : uint8
{
    Hold,
    UnitTarget,
    AreaTarget
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyMortarFireComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyMortarFireComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    int32 AmmunitionBombs = 48;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    int32 MaxAmmunitionBombs = 48;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float MinRangeCm = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float MaxRangeCm = 18000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float ReloadSeconds = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float DispersionRadiusCm = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float FortificationDamagePerBomb = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float InfantryHitChancePerBomb = 0.18f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mortar")
    EStrategyMortarFireMode FireMode = EStrategyMortarFireMode::Hold;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mortar")
    TObjectPtr<AStrategyUnit> UnitTarget;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mortar")
    TObjectPtr<AStrategyDefensivePosition> PositionTarget;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mortar")
    FVector AreaTarget = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mortar")
    float ReloadRemainingSeconds = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    bool SetUnitTarget(AStrategyUnit* NewTarget);

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    bool SetAreaTarget(const FVector& NewTarget);

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    bool SetFortificationTarget(AStrategyDefensivePosition* NewTarget);

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    void HoldFire();

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    bool FireOneBomb();

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    void ResupplyBombs(int32 Bombs);

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    bool IsTargetInRange(const FVector& TargetLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    FVector BuildHighArcApex(const FVector& TargetLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    float GetEstimatedTimeOfFlight(const FVector& TargetLocation) const;

private:
    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    UPROPERTY()
    TObjectPtr<UStrategyMortarDeploymentComponent> Deployment;

    FRandomStream RandomStream;
};
