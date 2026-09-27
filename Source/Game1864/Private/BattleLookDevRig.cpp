#include "BattleLookDevRig.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"

ABattleLookDevRig::ABattleLookDevRig()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Sun->bUseTemperature = true;
	Sun->SetLightSourceAngle(1.2f);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetIntensity(1.f);

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(Root);
	HeightFog->SetFogDensity(0.012f);
	HeightFog->SetFogHeightFalloff(0.15f);
	HeightFog->SetVolumetricFog(true);

	Grade = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Grade"));
	Grade->SetupAttachment(Root);
	Grade->bUnbound = true;
}

void ABattleLookDevRig::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Directional lights shine along +X; pitch down by the elevation.
	Sun->SetWorldRotation(FRotator(-SunElevation, SunAzimuth, 0.f));
	Sun->SetIntensity(SunIntensity);
	Sun->SetTemperature(SunTemperature);

	FPostProcessSettings& S = Grade->Settings;

	// Fixed exposure so the grade reads the same in every shot.
	S.bOverride_AutoExposureMethod = true;
	S.AutoExposureMethod = AEM_Manual;
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = ExposureBias;

	S.bOverride_WhiteTemp = true;
	S.WhiteTemp = 6500.f - Warmth * 6000.f;

	S.bOverride_ColorSaturation = true;
	S.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.f);
	S.bOverride_ColorContrast = true;
	S.ColorContrast = FVector4(Contrast, Contrast, Contrast, 1.f);

	// Warm highlights, slightly cool shadows.
	S.bOverride_ColorGainHighlights = true;
	S.ColorGainHighlights = FVector4(1.04f, 1.f, 0.93f, 1.f);
	S.bOverride_ColorGainShadows = true;
	S.ColorGainShadows = FVector4(0.96f, 0.98f, 1.03f, 1.f);

	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.45f;

	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = VignetteIntensity;
}
