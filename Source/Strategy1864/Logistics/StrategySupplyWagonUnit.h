#pragma once

#include "CoreMinimal.h"
#include "../Units/StrategyUnit.h"
#include "StrategySupplyTypes.h"
#include "StrategySupplyWagonUnit.generated.h"

class UStrategySupplyCargoComponent;
class UStrategySupplyCaptureComponent;

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategySupplyWagonUnit : public AStrategyUnit
{
    GENERATED_BODY()

public:
    AStrategySupplyWagonUnit();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply")
    int32 WagonCount = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply")
    int32 DriverStrength = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply")
    int32 HorseStrength = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply")
    int32 DriversRequiredForFullMobility = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply")
    int32 HorsesRequiredForFullMobility = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply", meta=(ClampMin="0.0", ClampMax="100.0"))
    float WagonCondition = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Supply")
    EStrategySupplyOwnershipState OwnershipState =
        EStrategySupplyOwnershipState::Operational;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Supply")
    TObjectPtr<UStrategySupplyCargoComponent> CargoComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Supply")
    TObjectPtr<UStrategySupplyCaptureComponent> SupplyCaptureComponent;

    UFUNCTION(BlueprintPure, Category="Strategy|Supply")
    float GetMobilityFactor() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Supply")
    bool CanMoveSupplyWagon() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Supply")
    void ApplySupplyDamage(
        int32 DriverLoss,
        int32 HorseLoss,
        float WagonDamage,
        float CargoLossFraction);

    UFUNCTION(BlueprintCallable, Category="Strategy|Supply")
    bool AbandonSupplyWagon();
};
