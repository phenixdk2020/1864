#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyFormationTypes.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyParentFormationPlannerComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyParentFormationPlannerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyParentFormationPlannerComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float CompanyNominalSpacingCm = 7200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float CompanyMinimumReservedSpacingCm = 6800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float MajorRearOffsetCm = 6500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float RegimentChildSpacingCm = 30000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float BrigadeChildSpacingCm = 60000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float DivisionChildSpacingCm = 90000.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Formation")
    TArray<FStrategyFormationSlot> GenerateCompanyLineSlots(
        const FVector& ObjectiveCenter,
        float FacingYaw,
        int32 CompanyCount) const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Formation")
    bool IssueCompanySlots(
        const FVector& ObjectiveCenter,
        float FacingYaw,
        bool bDefensiveMission,
        EStrategyOrderAuthority Authority);

    UFUNCTION(BlueprintCallable, Category="Strategy|Formation")
    bool IssueCompanySlotsForOrder(
        const FVector& ObjectiveCenter,
        float FacingYaw,
        EStrategyOrderType OrderType,
        EStrategyOrderAuthority Authority);

    UFUNCTION(BlueprintCallable, Category="Strategy|Formation")
    bool IssueDirectSubordinateSlots(
        const FVector& ObjectiveCenter,
        float FacingYaw,
        EStrategyOrderType OrderType,
        EStrategyOrderAuthority Authority,
        float SpacingCm);

private:
    TArray<AStrategyUnit*> GetCommandedCompanies() const;
    TArray<AStrategyUnit*> GetDirectCommandedSubordinates() const;
};
