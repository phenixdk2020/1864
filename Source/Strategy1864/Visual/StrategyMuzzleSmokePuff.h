#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyMuzzleSmokePuff.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * A puff of black-powder smoke from one musket: a few soft balls that swell, drift with the shot and the
 * wind, rise a little and thin out over some seconds. Presentation only (the smoke's effect on sight is
 * AStrategySmokeField).
 */
UCLASS()
class STRATEGY1864_API AStrategyMuzzleSmokePuff : public AActor
{
    GENERATED_BODY()

public:
    AStrategyMuzzleSmokePuff();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** Drift in cm per second (the shot's push, the wind, a little rise). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Smoke")
    FVector Drift = FVector(60.0f, 0.0f, 18.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Smoke")
    float LifetimeSeconds = 9.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Smoke")
    float StartDiameterCm = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Smoke")
    float EndDiameterCm = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Smoke")
    float StartOpacity = 0.55f;

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> Balls;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

    TArray<FVector> Offsets;
    float Age = 0.0f;
};
