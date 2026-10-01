#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyArtilleryTraverseComponent.generated.h"

class AStrategyArtilleryBatteryUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryTraverseComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryTraverseComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float TraverseSpeedDegreesPerSecond = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float FacingToleranceDegrees = 1.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    bool bTraversing = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    float DesiredFacingYaw = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    bool RequestTraverseToward(const FVector& WorldLocation);

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool IsLocationInsideTraverseArc(const FVector& WorldLocation) const;

private:
    UPROPERTY()
    TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;
};
