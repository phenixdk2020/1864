#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyDragoonComponent.generated.h"

class ACavalryUnit;

UENUM(BlueprintType)
enum class EStrategyCavalryRole : uint8
{
    Cavalry,
    Dragoon
};

UENUM(BlueprintType)
enum class EStrategyMountedState : uint8
{
    Mounted,
    Dismounted,
    ReturningToMounts
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyDragoonComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyDragoonComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Dragoon")
    EStrategyCavalryRole Role = EStrategyCavalryRole::Cavalry;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Dragoon")
    EStrategyMountedState MountedState = EStrategyMountedState::Mounted;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Dragoon")
    FVector HorseParkAnchor = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Dragoon")
    int32 DismountedCombatStrength = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Dragoon")
    int32 HorseHolderStrength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Dragoon")
    float CombatGroupFraction = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Dragoon")
    float MountedMoveSpeedCmPerSecond = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Dragoon")
    float DismountedMoveSpeedCmPerSecond = 450.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Dragoon")
    float RemountToleranceCm = 100.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Dragoon")
    bool DismountAtCurrentPosition();

    UFUNCTION(BlueprintCallable, Category="Strategy|Dragoon")
    bool RequestRemount();

private:
    void CompleteRemount();

    UPROPERTY()
    TObjectPtr<ACavalryUnit> OwnerCavalry;
};
