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
    UPROPERTY(EditAnywhere, Category="Strategy|Square")
    float SquareFaceFireShare = 0.25f;

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
    FString GetActiveRangeLabel() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire")
    bool IsInsideFireCone(const AStrategyUnit* Target) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire")
    bool CanEngageTarget(const AStrategyUnit* Target) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire")
    void GetFireFront(FVector& Left, FVector& Right, int32 FaceIndex = 0) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire")
    bool IsLocationInsideFireField(FVector Location, float RangeCm) const;

    /** Can a man standing at From (facing Forward) bring his musket to bear on any part of the target
     *  formation: ahead of him, inside the range, within the half angle of the fire cone from his place. */
    bool CanPointBearOn(const FVector& From, const FVector& Forward, const AStrategyUnit* Target, float RangeCm) const;

    /** The share of the company's men who can bear on the target (by angle and range from each man's slot);
     *  1 for a square (its faces have their own share). */
    float GetBearingFraction(const AStrategyUnit* Target, int32* OutBearing = nullptr, int32* OutTotal = nullptr) const;

    /** Points across the target's formation (its front, centre and rear corners), for the checks above. */
    TArray<FVector> GetTargetSamplePoints(const AStrategyUnit* Target) const;

private:
    void DrawQARangeCones() const;
    void DrawRangeArc(float InnerRangeCm, float RangeCm, bool bActive, const FColor& Color, const TCHAR* Label) const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
