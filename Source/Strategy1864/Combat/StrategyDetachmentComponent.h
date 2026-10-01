#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyDetachmentComponent.generated.h"

class AStrategyUnit;

UENUM(BlueprintType)
enum class EStrategyDetachmentType : uint8
{
    None,
    Skirmisher,
    Marksman,
    PioneerWorkingParty,
    AmmunitionParty,
    StretcherParty,
    PicketScout
};

USTRUCT(BlueprintType)
struct FStrategyDetachmentRecord
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FName DetachmentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EStrategyDetachmentType Type = EStrategyDetachmentType::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FName OrganicParentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AssignedStrength = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 CurrentStrength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AssignedAmmoRounds = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 RemainingAmmoRounds = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Cohesion = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TaskAnchor = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bActive = false;
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyDetachmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyDetachmentComponent();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Detachments")
    int32 MaximumConcurrentDetachments = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Detachments")
    float MaximumDetachedFraction = 0.45f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Detachments")
    TArray<FStrategyDetachmentRecord> Detachments;

    UFUNCTION(BlueprintCallable, Category="Strategy|Detachments")
    FName CreateDetachment(
        EStrategyDetachmentType Type,
        int32 RequestedStrength,
        int32 RequestedAmmoRounds,
        const FVector& TaskAnchor);

    UFUNCTION(BlueprintCallable, Category="Strategy|Detachments")
    bool RecallDetachment(FName DetachmentId);

    UFUNCTION(BlueprintCallable, Category="Strategy|Detachments")
    int32 ApplyDetachmentLoss(FName DetachmentId, int32 RequestedLoss);

    UFUNCTION(BlueprintCallable, Category="Strategy|Detachments")
    int32 ConsumeDetachmentAmmo(FName DetachmentId, int32 RequestedRounds);

    UFUNCTION(BlueprintCallable, Category="Strategy|Detachments")
    bool SetDetachmentAnchor(FName DetachmentId, const FVector& NewAnchor);

    UFUNCTION(BlueprintPure, Category="Strategy|Detachments")
    int32 GetDetachedStrength() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Detachments")
    int32 GetAvailableParentStrength() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Detachments")
    int32 GetActiveDetachmentCount() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Detachments")
    bool HasActiveType(EStrategyDetachmentType Type) const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Detachments")
    void ReconcileInvalidDetachments();

private:
    FStrategyDetachmentRecord* FindMutable(FName DetachmentId);
    const FStrategyDetachmentRecord* FindConst(FName DetachmentId) const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    int32 SerialCounter = 0;
};
