#include "StrategyBattleAtmosphere.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AStrategyBattleAtmosphere::AStrategyBattleAtmosphere()
{
    PrimaryActorTick.bCanEverTick = false;
    Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Haze"));
    SetRootComponent(Fog);
    Grading = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Grading"));
    Grading->SetupAttachment(Fog);
    Grading->bUnbound = true;
    Grading->Priority = 1.0f;
}

void AStrategyBattleAtmosphere::Apply()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Sun="), SunElevation);
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864SunYaw="), SunYaw);
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Haze="), HazeDensity);

    // The sun: low and warm, from behind the Danish left (the light travels north-east), long soft shadows.
    for (TActorIterator<ADirectionalLight> It(World); It; ++It)
    {
        UDirectionalLightComponent* Sun = Cast<UDirectionalLightComponent>(It->GetLightComponent());
        if (!Sun)
        {
            continue;
        }
        Sun->SetMobility(EComponentMobility::Movable);
        It->SetActorRotation(FRotator(-SunElevation, SunYaw, 0.0f));
        Sun->SetIntensity(10.0f);
        Sun->bUseTemperature = true;
        Sun->SetTemperature(5400.0f);
        Sun->SetLightColor(FLinearColor(1.0f, 0.96f, 0.9f));
        Sun->SetAtmosphereSunLight(true);
        Sun->SetCastShadows(true);
        Sun->SetDynamicShadowDistanceMovableLight(60000.0f);
        Sun->SetDynamicShadowCascades(4);
        Sun->ContactShadowLength = 0.03f;
        Sun->SetLightSourceAngle(1.2f);   // soft edges
        Sun->MarkRenderStateDirty();
        break;   // one sun
    }
    for (TActorIterator<ASkyLight> It(World); It; ++It)
    {
        if (USkyLightComponent* Sky = It->GetLightComponent())
        {
            Sky->SetMobility(EComponentMobility::Movable);
            Sky->bRealTimeCapture = true;
            Sky->SetIntensity(1.0f);
            Sky->SetLowerHemisphereColor(FLinearColor(0.07f, 0.08f, 0.05f));
            Sky->bLowerHemisphereIsBlack = false;
            Sky->RecaptureSky();
        }
        break;
    }

    // The haze: thin near the ground, warm where the sun is, so the far woods go soft and blue-grey.
    Fog->SetFogDensity(HazeDensity);
    Fog->SetFogHeightFalloff(0.12f);
    Fog->SetStartDistance(4000.0f);
    Fog->SetFogInscatteringColor(FLinearColor(0.42f, 0.50f, 0.62f));
    Fog->SetDirectionalInscatteringExponent(6.0f);
    Fog->SetDirectionalInscatteringStartDistance(8000.0f);
    Fog->SetDirectionalInscatteringColor(FLinearColor(0.62f, 0.48f, 0.30f));
    Fog->SetFogMaxOpacity(0.85f);
    Fog->SetVolumetricFog(false);

    // The grading: the reference's warm, slightly muted afternoon.
    FPostProcessSettings& P = Grading->Settings;
    P.bOverride_ColorSaturation = true;
    P.ColorSaturation = FVector4(0.92f, 0.92f, 0.92f, 1.0f);
    P.bOverride_ColorContrast = true;
    P.ColorContrast = FVector4(1.06f, 1.06f, 1.06f, 1.0f);
    P.bOverride_ColorGain = true;
    P.ColorGain = FVector4(1.03f, 1.0f, 0.95f, 1.0f);
    P.bOverride_ColorGammaShadows = true;
    P.ColorGammaShadows = FVector4(0.98f, 1.0f, 1.04f, 1.0f);
    P.bOverride_BloomIntensity = true;
    P.BloomIntensity = 0.35f;
    P.bOverride_VignetteIntensity = true;
    P.VignetteIntensity = 0.35f;
    P.bOverride_AutoExposureBias = true;
    float Exposure = -0.4f;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Exposure="), Exposure);
    P.AutoExposureBias = Exposure;
    P.bOverride_AmbientOcclusionIntensity = true;
    P.AmbientOcclusionIntensity = 0.6f;
    P.bOverride_FilmGrainIntensity = true;
    P.FilmGrainIntensity = 0.0f;
    P.bOverride_SceneFringeIntensity = true;
    P.SceneFringeIntensity = 0.0f;
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ATMOSPHERE: sun %.0f deg at yaw %.0f, haze %.3f"), SunElevation, SunYaw, HazeDensity);
}
