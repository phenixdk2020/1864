#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyFortificationAssaultComponent.generated.h"

class AStrategyDefensivePosition;
class AStrategyUnit;

UENUM(BlueprintType)
enum class EStrategyAssaultPhase : uint8
{
    Idle,
    Preparing,
    Approaching,
    Breaching,
    AssaultReady,
    Failed
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyFortificationAssaultComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyFortificationAssaultComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Assault")
    int32 Ladders = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Assault")
    int32 Planks = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Assault")
    int32 Axes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Assault")
    int32 Crowbars = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Assault")
    int32 ExplosiveCharges = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Assault")
    int32 WorkingPartyStrength = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Assault")
    float BasePreparationSeconds = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Assault")
    float BaseBreachSeconds = 25.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Assault")
    EStrategyAssaultPhase AssaultPhase = EStrategyAssaultPhase::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Assault")
    float RemainingSeconds = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Assault")
    TObjectPtr<AStrategyDefensivePosition> TargetPosition;

    UFUNCTION(BlueprintCallable, Category="Strategy|Assault")
    bool PrepareAssault(AStrategyDefensivePosition* Position);

    UFUNCTION(BlueprintCallable, Category="Strategy|Assault")
    bool BeginBreach();

    UFUNCTION(BlueprintCallable, Category="Strategy|Assault")
    void CancelAssault();

    UFUNCTION(BlueprintPure, Category="Strategy|Assault")
    float GetEquipmentFactor() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Assault")
    float GetExposureMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Assault")
    float GetObstacleCrossingMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Assault")
    bool HasLadderCapability() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Assault")
    bool HasGapCrossingCapability() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Assault")
    bool HasBreachCapability() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Assault")
    bool ConsumeAssaultEquipment();

private:
    void CompleteCurrentPhase();

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
