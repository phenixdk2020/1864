#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyArtilleryRepairComponent.generated.h"

class AStrategyArtilleryBatteryUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryRepairComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryRepairComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Repair")
    float RepairSecondsPerGun = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Repair")
    int32 MinimumCrewForRepair = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Repair")
    float RepairFatigueCost = 4.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery|Repair")
    bool bRepairing = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery|Repair")
    float RepairRemainingSeconds = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Repair")
    bool BeginRepairDisabledGun();

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Repair")
    void CancelRepair();

private:
    void CompleteRepair();

    UPROPERTY()
    TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;
};
