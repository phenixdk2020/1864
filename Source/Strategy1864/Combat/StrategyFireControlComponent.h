#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyFireControlTypes.h"
#include "StrategyFireControlComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyFireControlComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyFireControlComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire")
    EStrategyFirePolicy FirePolicy = EStrategyFirePolicy::Long;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire")
    float CloseRangeCm = 4000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire")
    float MediumRangeCm = 7000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire")
    float LongRangeCm = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire")
    float FireConeHalfAngleDegrees = 35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire")
    bool bRequireCurrentContact = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    bool bDrawQARangeCones = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    bool bShowPrussianQARangesFromStartup = true;

    UFUNCTION(BlueprintCallable, Category="Strategy|Fire")
    void SetFirePolicy(EStrategyFirePolicy NewPolicy);

    UFUNCTION(BlueprintPure, Category="Strategy|Fire")
    float GetActiveRangeCm() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire")
    bool IsInsideFireCone(const AStrategyUnit* Target) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire")
    bool CanEngageTarget(const AStrategyUnit* Target) const;

private:
    void DrawQARangeCones() const;
    void DrawRangeArc(float RangeCm, bool bActive, const FColor& Color) const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
