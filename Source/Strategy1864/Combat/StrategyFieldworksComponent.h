#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Engineering/StrategyDefensivePosition.h"
#include "StrategyFieldworksComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyFieldworksComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyFieldworksComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    EStrategyDefensivePositionType RequestedPositionType =
        EStrategyDefensivePositionType::Breastwork;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float BuildSeconds = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    bool bEngineerUnit = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float EngineerBuildTimeMultiplier = 0.55f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Fieldworks")
    bool bBuilding = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Fieldworks")
    float BuildRemainingSeconds = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    bool BeginHastyFieldworks();

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    void CancelFieldworks();

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    bool HasCompletedFieldworks() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    AStrategyDefensivePosition* GetCompletedPosition() const
    {
        return CompletedPosition;
    }

private:
    void CompleteFieldworks();

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    UPROPERTY()
    TObjectPtr<AStrategyDefensivePosition> CompletedPosition;
};
