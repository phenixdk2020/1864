#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyBattleAtmosphere.generated.h"

class UExponentialHeightFogComponent;
class UPostProcessComponent;
class ADirectionalLight;
class ASkyLight;

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

    /** The day of the year and the latitude: where the sun stands at an hour (Denmark 55.7 N). */
    UPROPERTY(EditAnywhere, Category="Strategy|Atmosphere")
    float DayOfYear = 182.0f;

    UPROPERTY(EditAnywhere, Category="Strategy|Atmosphere")
    float LatitudeDeg = 55.7f;

    /** The light of an hour of the day (0-24): the sun's height and bearing from the date and place, dusk and night
     *  with a pale moon, the sky, the haze and the exposure following. */
    void UpdateForHour(float Hour);

private:
    TWeakObjectPtr<ADirectionalLight> SunActor;
    TWeakObjectPtr<ASkyLight> SkyActor;
    float LastHour = -100.0f;
    double NextBattleSkyCapture = 0.0;

    UPROPERTY()
    TObjectPtr<UExponentialHeightFogComponent> Fog;

    UPROPERTY()
    TObjectPtr<UPostProcessComponent> Grading;
};
