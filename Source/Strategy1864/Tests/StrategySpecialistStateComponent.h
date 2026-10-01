#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Combat/StrategyDetachmentComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Combat/StrategyFireDrillComponent.h"
#include "../Engineering/StrategyWorkingPartyComponent.h"
#include "StrategySpecialistStateComponent.generated.h"

class AStrategyUnit;

USTRUCT(BlueprintType)
struct FStrategySpecialistStateSnapshot
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FName UnitId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 UnitStrength = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FStrategyDetachmentRecord> Detachments;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 NCOStrength = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float NCOQuality = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bOfficerAvailable = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EStrategyStance Stance = EStrategyStance::Standing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EStrategyLoadingMethod LoadingMethod = EStrategyLoadingMethod::MuzzleLoader;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EStrategyFireDrillResearchLevel FireDrillResearchLevel =
        EStrategyFireDrillResearchLevel::FrontRankFire;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EStrategyFireDrillMode DrillMode = EStrategyFireDrillMode::FrontRank;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 WorkingPartyWorkers = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EStrategyWorkingPartyTask WorkingPartyTask =
        EStrategyWorkingPartyTask::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 CarriedAmmunition = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 WoundedWaiting = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 WoundedCollected = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Cohesion = 100.0f;
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategySpecialistStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategySpecialistStateComponent();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Specialist State")
    FStrategySpecialistStateSnapshot LastSnapshot;

    UFUNCTION(BlueprintCallable, Category="Strategy|Specialist State")
    FStrategySpecialistStateSnapshot CaptureSnapshot();

    UFUNCTION(BlueprintCallable, Category="Strategy|Specialist State")
    bool RestoreSnapshot(const FStrategySpecialistStateSnapshot& Snapshot);

    UFUNCTION(BlueprintCallable, Category="Strategy|Specialist State")
    bool RestoreLastSnapshot();

    UFUNCTION(BlueprintPure, Category="Strategy|Specialist State")
    bool ValidateCurrentState(FString& FailureReason) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Specialist State")
    int32 BuildDeterministicDigest() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Specialist State")
    void ResetSpecialistState();

    UFUNCTION(BlueprintPure, Category="Strategy|Specialist State")
    bool HasDetachedStrengthLeak() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Specialist State")
    bool HasInvalidNCOState() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Specialist State")
    bool HasInvalidWorkingPartyState() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Specialist State")
    FString BuildQAStatusLine() const;

private:
    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
