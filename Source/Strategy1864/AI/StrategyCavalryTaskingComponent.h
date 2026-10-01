#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyCavalryTaskingComponent.generated.h"

class AStrategyUnit;
class ACavalryUnit;

USTRUCT()
struct FStrategyTemporaryCavalryAttachment
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<ACavalryUnit> Cavalry;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> PreviousCommandParent;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> AssignedMajor;
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyCavalryTaskingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyCavalryTaskingComponent();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry Tasking")
    float ReserveRearOffsetCm = 15000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry Tasking")
    float ReserveLateralOffsetCm = 4500.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Cavalry Tasking")
    void ReleaseTemporaryAttachments(bool bIssueReserveMove = true);

private:
    UFUNCTION()
    void HandleOwnerOrderChanged(const FStrategyOrder& NewOrder);

    UFUNCTION()
    void HandleOwnerExecutionStateChanged(
        EStrategyOrderExecutionState OldState,
        EStrategyOrderExecutionState NewState);

    void AssignForAttack(const FStrategyOrder& AttackOrder);
    void AssignDefensiveReserve(const FStrategyOrder& DefendOrder);
    TArray<ACavalryUnit*> GetAvailableDirectCavalry() const;
    TArray<AStrategyUnit*> GetCommandedMajorsRecursive() const;
    void CollectMajorsRecursive(AStrategyUnit* Parent, TArray<AStrategyUnit*>& OutMajors) const;
    bool CavalryHasProtectedDirectOrder(const ACavalryUnit* Cavalry) const;
    void AttachCavalryToMajor(
        ACavalryUnit* Cavalry,
        AStrategyUnit* Major,
        const FStrategyOrder& ParentOrder,
        float LateralSign);
    FVector CalculateReservePosition(
        const AStrategyUnit* Anchor,
        float FacingYaw,
        float LateralSign) const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    UPROPERTY()
    TArray<FStrategyTemporaryCavalryAttachment> ActiveAttachments;
};
