#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyBattleAtmosphere.generated.h"

class UExponentialHeightFogComponent;
class UPostProcessComponent;

/**
 * The battle's light and air (PROJECT 1864): a late-afternoon sun low in the south-west (warm, long shadows), the
 * sky light capturing the sky, a thin warm haze that softens the distance, and the grading (a little less
 * saturation, warm gain, gentle contrast, soft bloom, a slight vignette). It takes over the map's sun and sky
 * light, so every battle map gets the same look without editing the maps.
 * -Strategy1864Sun=<elevation degrees>, -Strategy1864SunYaw=<degrees>, -Strategy1864Haze=<density> for tests.
 */
UCLASS()
class STRATEGY1864_API AStrategyBattleAtmosphere : public AActor
{
    GENERATED_BODY()

public:
    AStrategyBattleAtmosphere();

    /** Set up the light and air in the actor's world (spawned once by the battle scenario). */
    void Apply();

    /** The sun's elevation and its compass direction (degrees; yaw 0 = the light travels north). */
    UPROPERTY(EditAnywhere, Category="Strategy|Atmosphere")
    float SunElevation = 24.0f;

    UPROPERTY(EditAnywhere, Category="Strategy|Atmosphere")
    float SunYaw = 35.0f;

    UPROPERTY(EditAnywhere, Category="Strategy|Atmosphere")
    float HazeDensity = 0.012f;

private:
    UPROPERTY()
    TObjectPtr<UExponentialHeightFogComponent> Fog;

    UPROPERTY()
    TObjectPtr<UPostProcessComponent> Grading;
};
