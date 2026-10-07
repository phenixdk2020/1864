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
        SunActor = *It;
        break;   // one sun
    }
    for (TActorIterator<ASkyLight> It(World); It; ++It)
    {
        if (USkyLightComponent* Sky = It->GetLightComponent())
        {
            Sky->SetMobility(EComponentMobility::Movable);
            Sky->bRealTimeCapture = true;
            Sky->SetIntensity(2.2f);
            Sky->SetLowerHemisphereColor(FLinearColor(0.07f, 0.08f, 0.05f));
            Sky->bLowerHemisphereIsBlack = false;
            Sky->RecaptureSky();
        }
        SkyActor = *It;
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

void AStrategyBattleAtmosphere::UpdateForHour(float Hour)
{
    if (FMath::Abs(Hour - LastHour) < 0.02f || !SunActor.IsValid())
    {
        return;
    }
    LastHour = Hour;
    // The sun from the date and the place: declination by the day of the year, the hour angle from the hour.
    const float Declination = FMath::DegreesToRadians(23.44f * FMath::Sin(2.0f * PI * (284.0f + DayOfYear) / 365.0f));
    const float Phi = FMath::DegreesToRadians(LatitudeDeg);
    const float HourAngle = FMath::DegreesToRadians((Hour - 12.0f) * 15.0f);
    const float SinEl = FMath::Sin(Phi) * FMath::Sin(Declination) + FMath::Cos(Phi) * FMath::Cos(Declination) * FMath::Cos(HourAngle);
    const float Elevation = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(SinEl, -1.0f, 1.0f)));
    const float CosEl = FMath::Max(0.01f, FMath::Cos(FMath::DegreesToRadians(Elevation)));
    float Azimuth = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp((FMath::Sin(Declination) - SinEl * FMath::Sin(Phi)) / (CosEl * FMath::Cos(Phi)), -1.0f, 1.0f)));
    if (HourAngle > 0.0f) { Azimuth = 360.0f - Azimuth; }
    // 0 at night, 1 in daylight; dusk and dawn between (the sun from 6 degrees below the horizon to 10 above).
    const float Day = FMath::SmoothStep(-6.0f, 10.0f, Elevation);
    const float Low = 1.0f - FMath::SmoothStep(4.0f, 30.0f, Elevation);   // a low sun is red

    if (ADirectionalLight* Light = SunActor.Get())
    {
        if (UDirectionalLightComponent* Sun = Cast<UDirectionalLightComponent>(Light->GetLightComponent()))
        {
            const bool bSun = Elevation > -3.0f;
            // The sun, or at night a pale moon high in the south (the light travels north).
            const float Pitch = bSun ? -FMath::Max(Elevation, 1.5f) : -38.0f;
            const float Yaw = bSun ? Azimuth + 180.0f : 0.0f;
            Light->SetActorRotation(FRotator(Pitch, Yaw, 0.0f));
            Sun->SetIntensity(FMath::Lerp(0.35f, 10.0f * FMath::Lerp(1.0f, 0.55f, Low), Day));
            Sun->bUseTemperature = true;
            Sun->SetTemperature(FMath::Lerp(bSun ? 2600.0f : 9500.0f, 5400.0f, bSun ? 1.0f - Low : 0.0f));
            Sun->MarkRenderStateDirty();
        }
    }
    if (ASkyLight* SkyActorPtr = SkyActor.Get())
    {
        if (USkyLightComponent* Sky = SkyActorPtr->GetLightComponent())
        {
            Sky->SetIntensity(FMath::Lerp(0.35f, 2.2f, Day));
            Sky->RecaptureSky();
        }
    }
    Fog->SetFogInscatteringColor(FMath::Lerp(FLinearColor(0.03f, 0.04f, 0.08f), FLinearColor(0.42f, 0.50f, 0.62f), Day));
    Fog->SetDirectionalInscatteringColor(FMath::Lerp(FLinearColor(0.02f, 0.03f, 0.06f), FLinearColor(0.62f, 0.48f, 0.30f), Day));
    FPostProcessSettings& P = Grading->Settings;
    P.bOverride_AutoExposureBias = true;
    P.AutoExposureBias = FMath::Lerp(0.1f, -0.4f, Day);
}
