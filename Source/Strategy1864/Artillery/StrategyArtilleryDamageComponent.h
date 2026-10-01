#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyArtilleryDamageComponent.generated.h"

class AStrategyArtilleryBatteryUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryDamageComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryDamageComponent();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    void ApplyIncomingHits(int32 Hits, bool bExplosive);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    void SetDeterministicRandomSeed(int32 Seed)
    {
        RandomStream.Initialize(Seed);
    }

private:
    UPROPERTY()
    TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;

    FRandomStream RandomStream;
};
