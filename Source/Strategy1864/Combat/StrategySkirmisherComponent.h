#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategySkirmisherComponent.generated.h"

class AStrategyUnit;

UENUM(BlueprintType)
enum class EStrategySkirmisherRole : uint8
{
    Screen,
    Recon,
    Harass,
    CoverAdvance,
    CoverRetreat
};

UENUM(BlueprintType)
enum class EStrategySkirmisherState : uint8
{
    Attached,
    Deploying,
    Deployed,
    Recalling
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategySkirmisherComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategySkirmisherComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Skirmishers")
    EStrategySkirmisherState State = EStrategySkirmisherState::Attached;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Skirmishers")
    EStrategySkirmisherRole Role = EStrategySkirmisherRole::Screen;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Skirmishers", meta=(ClampMin="0.05", ClampMax="0.40"))
    float DefaultDeploymentFraction = 0.20f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Skirmishers")
    int32 DetachedStrength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Skirmishers")
    float DeploySeconds = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Skirmishers")
    float RecallSeconds = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Skirmishers")
    float ScreenDistanceCm = 3000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Skirmishers")
    FVector ScreenAnchor = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable, Category="Strategy|Skirmishers")
    bool DeploySkirmishers(
        EStrategySkirmisherRole NewRole,
        float Fraction = -1.0f);

    UFUNCTION(BlueprintCallable, Category="Strategy|Skirmishers")
    bool RecallSkirmishers();

    UFUNCTION(BlueprintPure, Category="Strategy|Skirmishers")
    bool IsDeployed() const
    {
        return State == EStrategySkirmisherState::Deployed;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Skirmishers")
    float GetVolleyDensityMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Skirmishers")
    float GetIncomingHitMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Skirmishers")
    float GetAwarenessRangeMultiplier() const;

private:
    void CompleteDeploy();
    void CompleteRecall();
    void UpdateScreenAnchor();

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float TransitionRemainingSeconds = 0.0f;
    float ActiveFraction = 0.0f;
};
