#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyWorkingPartyComponent.generated.h"

class AStrategyUnit;
class AStrategyDefensivePosition;

UENUM(BlueprintType)
enum class EStrategyWorkingPartyTask : uint8
{
    None,
    CarryAmmunition,
    StretcherCollection,
    DigFieldworks,
    BreachObstacle,
    RepairPosition
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyWorkingPartyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyWorkingPartyComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Working Party")
    int32 AvailableWorkers = 24;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Working Party")
    int32 AssignedWorkers = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Working Party")
    EStrategyWorkingPartyTask ActiveTask = EStrategyWorkingPartyTask::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Working Party")
    int32 CarriedAmmunitionRounds = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Working Party")
    int32 WoundedWaitingCollection = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Working Party")
    int32 WoundedCollected = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Working Party")
    float TaskProgress = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Working Party")
    TObjectPtr<AStrategyUnit> TargetUnit;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Working Party")
    TObjectPtr<AStrategyDefensivePosition> TargetPosition;

    UFUNCTION(BlueprintCallable, Category="Strategy|Working Party")
    bool AssignTask(EStrategyWorkingPartyTask Task, int32 WorkerCount);

    UFUNCTION(BlueprintCallable, Category="Strategy|Working Party")
    void CancelTask();

    UFUNCTION(BlueprintCallable, Category="Strategy|Working Party")
    void SetTargetUnit(AStrategyUnit* NewTarget);

    UFUNCTION(BlueprintCallable, Category="Strategy|Working Party")
    void SetTargetPosition(AStrategyDefensivePosition* NewTarget);

    UFUNCTION(BlueprintCallable, Category="Strategy|Working Party")
    int32 LoadAmmunition(int32 Rounds);

    UFUNCTION(BlueprintCallable, Category="Strategy|Working Party")
    int32 DeliverAmmunition();

    UFUNCTION(BlueprintCallable, Category="Strategy|Working Party")
    void AddWoundedForCollection(int32 Wounded);

    UFUNCTION(BlueprintCallable, Category="Strategy|Working Party")
    int32 CollectWounded(int32 Requested);

    UFUNCTION(BlueprintPure, Category="Strategy|Working Party")
    float GetWorkRate() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Working Party")
    bool HasActiveTask() const;

private:
    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
