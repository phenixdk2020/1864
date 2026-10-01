#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyArtilleryCaptureComponent.generated.h"

class AStrategyArtilleryBatteryUnit;
class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryCaptureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryCaptureComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Capture")
    float CaptureRadiusCm = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Capture")
    float CaptureHoldSeconds = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Capture")
    int32 MinimumQualifiedCrewForReuse = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Capture")
    float ReusePreparationSeconds = 20.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery|Capture")
    float CaptureProgressSeconds = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery|Capture")
    float ReuseRemainingSeconds = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Capture")
    bool AttemptReuse(
        int32 AvailableQualifiedCrew,
        bool bHasCompatibleAmmunition);

private:
    AStrategyUnit* FindCapturingEnemy() const;
    bool HasFriendlyProtection() const;
    void CompleteCapture(AStrategyUnit* Captor);

    UPROPERTY()
    TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> ActiveCaptor;

    bool bReusePending = false;
};
