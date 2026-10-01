#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyMountedAnimationSyncComponent.generated.h"

class ACavalryUnit;
class UStrategyHorseAnimationStateComponent;
class UStrategyHumanAnimationStateComponent;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyMountedAnimationSyncComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyMountedAnimationSyncComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    void SyncRiderToHorse();

    UPROPERTY()
    TObjectPtr<ACavalryUnit> OwnerCavalry;

    UPROPERTY()
    TObjectPtr<UStrategyHorseAnimationStateComponent> HorseState;

    UPROPERTY()
    TObjectPtr<UStrategyHumanAnimationStateComponent> HumanState;
};
