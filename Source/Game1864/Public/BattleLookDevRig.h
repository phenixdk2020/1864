#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleLookDevRig.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;

/**
 * One drop-in actor for the battle reference look: low warm late-afternoon sun, atmosphere,
 * light haze and a warm filmic grade. Drop it into an empty level with a landscape.
 */
UCLASS()
class GAME1864_API ABattleLookDevRig : public AActor
{
	GENERATED_BODY()

public:
	ABattleLookDevRig();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Sun elevation above the horizon, degrees. ~30 = late afternoon. */
	UPROPERTY(EditAnywhere, Category = "Sun", meta = (ClampMin = "2", ClampMax = "89"))
	float SunElevation = 32.f;

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (ClampMin = "0", ClampMax = "360"))
	float SunAzimuth = 55.f;

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (Units = "Lux"))
	float SunIntensity = 9.f;

	UPROPERTY(EditAnywhere, Category = "Sun", meta = (Units = "Kelvin"))
	float SunTemperature = 5300.f;

	/** Manual exposure bias (EV). Lower = darker. */
	UPROPERTY(EditAnywhere, Category = "Grade")
	float ExposureBias = 10.f;

	UPROPERTY(EditAnywhere, Category = "Grade")
	float Warmth = 0.02f;

	UPROPERTY(EditAnywhere, Category = "Grade")
	float Saturation = 1.08f;

	UPROPERTY(EditAnywhere, Category = "Grade")
	float Contrast = 1.06f;

	UPROPERTY(EditAnywhere, Category = "Grade")
	float VignetteIntensity = 0.35f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPostProcessComponent> Grade;
};
