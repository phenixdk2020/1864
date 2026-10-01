#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyHumanVisualTypes.h"
#include "StrategyHumanAnimationStateComponent.generated.h"

class AStrategyUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FStrategyHumanAnimationActionChanged,
    EStrategyHumanAnimationAction,
    PreviousAction,
    EStrategyHumanAnimationAction,
    NewAction);

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyHumanAnimationStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyHumanAnimationStateComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintAssignable, Category="Strategy|Visual|Animation")
    FStrategyHumanAnimationActionChanged OnAnimationActionChanged;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Visual|Animation")
    EStrategyHumanAnimationAction CurrentAction =
        EStrategyHumanAnimationAction::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Visual|Animation")
    EStrategyHumanAnimationAction BaseLocomotionAction =
        EStrategyHumanAnimationAction::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Visual|Animation")
    bool bMounted = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Visual|Animation")
    bool bWeaponReady = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Visual|Animation")
    float ActionRemainingSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    float RunSpeedThresholdCmPerSecond = 650.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Animation")
    void RequestAction(
        EStrategyHumanAnimationAction Action,
        float DurationSeconds = 0.0f);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Animation")
    void ClearAction();

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Animation")
    void SetMounted(bool bNewMounted);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Animation")
    void SetWeaponReady(bool bNewReady)
    {
        bWeaponReady = bNewReady;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Animation")
    bool IsTransientActionActive() const
    {
        return ActionRemainingSeconds > 0.0f;
    }

private:
    void RefreshBaseLocomotion();
    void SetCurrentActionInternal(
        EStrategyHumanAnimationAction NewAction);

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    FVector PreviousLocation = FVector::ZeroVector;
    float LastMeasuredSpeedCmPerSecond = 0.0f;
};
