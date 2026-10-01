#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyOrderTypes.h"
#include "StrategyOrderComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStrategyOrderChanged, const FStrategyOrder&, NewOrder);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStrategyExecutionStateChanged, EStrategyOrderExecutionState, OldState, EStrategyOrderExecutionState, NewState);

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyOrderComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyOrderComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintAssignable, Category="Strategy|Orders")
    FStrategyOrderChanged OnOrderChanged;

    UPROPERTY(BlueprintAssignable, Category="Strategy|Orders")
    FStrategyExecutionStateChanged OnExecutionStateChanged;

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    bool SetOrder(const FStrategyOrder& NewOrder);

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    bool QueueDelayedOrder(const FStrategyOrder& NewOrder, float DelaySeconds);

    UFUNCTION(BlueprintPure, Category="Strategy|Orders")
    bool HasDelayedOrder() const { return bHasDelayedOrder; }

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    void ClearOrder();

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    void BeginExecution();

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    void CompleteExecution();

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    void FailExecution();

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    void MarkPendingTarget();

    UFUNCTION(BlueprintPure, Category="Strategy|Orders")
    FStrategyOrder GetCurrentOrder() const { return CurrentOrder; }

    UFUNCTION(BlueprintPure, Category="Strategy|Orders")
    EStrategyOrderExecutionState GetExecutionState() const { return ExecutionState; }

    UFUNCTION(BlueprintPure, Category="Strategy|Orders")
    bool IsPhysicallyExecuting() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Orders")
    bool HasStandingIntent() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Orders")
    EStrategyCommandVisualState GetCommandVisualState() const;

private:
    bool CanReplaceCurrentOrder(const FStrategyOrder& NewOrder) const;
    void SetExecutionState(EStrategyOrderExecutionState NewState);

    UPROPERTY(VisibleInstanceOnly, Category="Strategy|Orders")
    FStrategyOrder CurrentOrder;

    UPROPERTY(VisibleInstanceOnly, Category="Strategy|Orders")
    EStrategyOrderExecutionState ExecutionState = EStrategyOrderExecutionState::Idle;

    UPROPERTY(VisibleInstanceOnly, Category="Strategy|Orders")
    int32 NextOrderSerial = 1;

    UPROPERTY(VisibleInstanceOnly, Category="Strategy|Orders")
    FStrategyOrder DelayedOrder;

    UPROPERTY(VisibleInstanceOnly, Category="Strategy|Orders")
    float DelayedOrderRemainingSeconds = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="Strategy|Orders")
    bool bHasDelayedOrder = false;
};
