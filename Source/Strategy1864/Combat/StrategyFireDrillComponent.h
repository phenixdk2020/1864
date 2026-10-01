#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyStanceComponent.h"
#include "StrategyFireDrillComponent.generated.h"

UENUM(BlueprintType)
enum class EStrategyLoadingMethod : uint8
{
    MuzzleLoader,
    BreechLoader
};

UENUM(BlueprintType)
enum class EStrategyFireDrillMode : uint8
{
    FrontRank,
    Volley,
    Independent,

    // Legacy serialized values retained for compatibility with older QA data.
    AlternatingSections UMETA(Hidden),
    KneelingFrontRank UMETA(Hidden),

    TwoRankFire,
    FireByRank
};

UENUM(BlueprintType)
enum class EStrategyFireDrillResearchLevel : uint8
{
    FrontRankFire,
    TwoRankFire,
    FireByRank,
    ControlledVolley,
    IndependentFire,
    AdvancedFireDrill
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyFireDrillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyFireDrillComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire Drill")
    EStrategyLoadingMethod LoadingMethod = EStrategyLoadingMethod::MuzzleLoader;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Strategy|Fire Drill")
    EStrategyFireDrillResearchLevel ResearchLevel =
        EStrategyFireDrillResearchLevel::FrontRankFire;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Strategy|Fire Drill")
    EStrategyFireDrillMode DrillMode = EStrategyFireDrillMode::FrontRank;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire Drill", meta=(ClampMin="0.0", ClampMax="100.0"))
    float DrillTraining = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire Drill", meta=(ClampMin="0.0", ClampMax="100.0"))
    float FireDiscipline = 50.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Fire Drill")
    int32 ActiveFireByRankIndex = 0;

    UFUNCTION(BlueprintCallable, Category="Strategy|Fire Drill")
    bool SetDrillMode(EStrategyFireDrillMode NewMode);

    UFUNCTION(BlueprintCallable, Category="Strategy|Fire Drill")
    void SetResearchLevel(EStrategyFireDrillResearchLevel NewLevel);

    UFUNCTION(BlueprintCallable, Category="Strategy|Fire Drill")
    void SetLoadingMethod(EStrategyLoadingMethod NewMethod);

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    bool IsDrillModeUnlocked(EStrategyFireDrillMode Mode) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    EStrategyFireDrillResearchLevel GetRequiredResearchLevel(
        EStrategyFireDrillMode Mode) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    EStrategyFireDrillMode GetHighestUnlockedDrillMode() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    bool CanUseAutomaticDrillSelection() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Fire Drill")
    bool SelectAutomaticDrillMode(
        bool bPreferShockVolley,
        bool bPreferContinuousFire);

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    bool IsFormationSlotEligibleToFire(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    float GetEligibleFiringFraction(EStrategyStance Stance) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    float GetReloadMultiplier(EStrategyStance Stance) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    float GetVolleyCoordinationMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    float GetIndependentFireCadenceMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    bool SupportsProneReload() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    bool SupportsKneelingFrontRank() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    bool IsAlternatingFire() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    int32 GetActiveFireByRankIndex() const
    {
        return ActiveFireByRankIndex;
    }

    UFUNCTION(BlueprintCallable, Category="Strategy|Fire Drill")
    void AdvanceFireByRankCycle();

    UFUNCTION(BlueprintCallable, Category="Strategy|Fire Drill")
    void ResetFireByRankCycle();

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Drill")
    float GetDoctrineReadiness() const;

private:
    static EStrategyFireDrillMode NormalizeLegacyMode(
        EStrategyFireDrillMode Mode);

    int32 GetConfiguredRankCount() const;
};
