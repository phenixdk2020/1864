#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyFormationTypes.h"
#include "StrategyFormationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FStrategyFormationChanged,
    EStrategyFormationType,
    OldFormation,
    EStrategyFormationType,
    NewFormation);

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyFormationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyFormationComponent();

    UPROPERTY(BlueprintAssignable, Category="Strategy|Formation")
    FStrategyFormationChanged OnFormationChanged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    EStrategyFormationType CurrentFormation = EStrategyFormationType::Line;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    int32 RankCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float SoldierLateralSpacingCm = 75.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float SoldierRankSpacingCm = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    int32 ColumnWidth = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    float FullCompanyTargetFrontageCm = 4800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    float FullCompanyFrontageToleranceCm = 500.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Formation")
    void SetFormation(EStrategyFormationType NewFormation);

    UFUNCTION(BlueprintPure, Category="Strategy|Formation")
    TArray<FStrategyFormationSlot> GenerateSoldierSlots(
        const FVector& FormationCenter,
        float FacingYaw,
        int32 Strength) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Formation")
    float EstimateFrontageCm(int32 Strength) const;

    UFUNCTION(BlueprintPure, Category="Strategy|QA")
    bool IsFullCompanyFrontageWithinBaseline(int32 Strength = 190) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Presentation")
    bool UsesSquareVisualOwnership() const
    {
        return CurrentFormation == EStrategyFormationType::Square;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Presentation")
    bool UsesLinearFormationVisuals() const
    {
        return CurrentFormation != EStrategyFormationType::Square;
    }
};
