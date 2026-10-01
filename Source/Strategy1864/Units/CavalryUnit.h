#pragma once

#include "CoreMinimal.h"
#include "StrategyUnit.h"
#include "../Formations/StrategyFormationTypes.h"
#include "CavalryUnit.generated.h"

class USkeletalMeshComponent;
class UStrategyCavalryChargeComponent;
class UStrategyDragoonComponent;
class UStrategyCavalryScreenAIComponent;
class UStrategyHorseAnimationStateComponent;
class UStrategyMountedAnimationSyncComponent;

UCLASS(Blueprintable)
class STRATEGY1864_API ACavalryUnit : public AStrategyUnit
{
    GENERATED_BODY()

public:
    ACavalryUnit();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Cavalry")
    TObjectPtr<USkeletalMeshComponent> HorseMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Cavalry")
    TObjectPtr<USkeletalMeshComponent> RiderMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Cavalry")
    TObjectPtr<UStrategyCavalryChargeComponent> ChargeComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Cavalry")
    TObjectPtr<UStrategyDragoonComponent> DragoonComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Cavalry")
    TObjectPtr<UStrategyCavalryScreenAIComponent> ScreenAIComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Cavalry")
    TObjectPtr<UStrategyHorseAnimationStateComponent> HorseAnimationStateComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Cavalry")
    TObjectPtr<UStrategyMountedAnimationSyncComponent> MountedAnimationSyncComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry")
    FName RiderSocketName = TEXT("RiderSocket");

    UFUNCTION(BlueprintCallable, Category="Strategy|Cavalry")
    void SetDefileMode(bool bEnable);

private:
    EStrategyFormationType PreDefileFormation = EStrategyFormationType::CavalryLine;
};
