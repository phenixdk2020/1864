#include "StrategyBattleQuality.h"
#include "UObject/UObjectGlobals.h"
#include "../Visual/StrategyCourierRider.h"
#include "../Visual/StrategyBattleBlast.h"
#include "../Visual/StrategyMuzzleSmokePuff.h"
#include "StrategyBattlePerformance.h"
#include "HAL/IConsoleManager.h"
#include "Scalability.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "Components/LightComponent.h"
#include "UObject/UObjectIterator.h"
#include "../Units/StrategyCompanyUnit.h"
#include "../Visual/StrategyInfantryVisualComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Visual/StrategyCavalryVisualComponent.h"

namespace Strategy1864BattleQuality
{
    bool GetShadowsOn()
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864NoShadows"))) { return false; }
        int32 On = 1;
        if (GConfig) { GConfig->GetInt(TEXT("/Script/Strategy1864.Settings"), TEXT("Shadows"), On, GGameUserSettingsIni); }
        return On != 0;
    }

    void ApplyShadows(UWorld* World)
    {
        if (!World) { return; }
        // The shadow casting each light had before the player switched the shadows off, so that they come back as they were.
        static TMap<TWeakObjectPtr<ULightComponent>, bool> Originals;
        const bool bOn = GetShadowsOn();
        for (TObjectIterator<ULightComponent> It; It; ++It)
        {
            ULightComponent* Light = *It;
            if (!IsValid(Light) || Light->GetWorld() != World) { continue; }
            const TWeakObjectPtr<ULightComponent> Key(Light);
            if (!Originals.Contains(Key)) { Originals.Add(Key, Light->CastShadows); }
            const bool bWanted = bOn ? Originals[Key] : false;
            if (Light->CastShadows != bWanted) { Light->SetCastShadows(bWanted); }
        }
    }

    void SetShadowsOn(UWorld* World, bool bOn, bool bSave)
    {
        if (bSave && GConfig)
        {
            GConfig->SetInt(TEXT("/Script/Strategy1864.Settings"), TEXT("Shadows"), bOn ? 1 : 0, GGameUserSettingsIni);
            GConfig->Flush(false, GGameUserSettingsIni);
        }
        ApplyShadows(World);
    }

    int32 GetPreset()
    {
        int32 Preset = 1;   // MIDDEL until the player chooses otherwise
        if (GConfig) { GConfig->GetInt(TEXT("/Script/Strategy1864.Settings"), TEXT("BattleQuality"), Preset, GGameUserSettingsIni); }
        return FMath::Clamp(Preset, 0, 2);
    }

    void SetFigureDivisor(int32 Divisor)
    {
        if (GConfig)
        {
            GConfig->SetInt(TEXT("/Script/Strategy1864.Settings"), TEXT("FigureDivisor"), FMath::Max(1, Divisor), GGameUserSettingsIni);
            GConfig->Flush(false, GGameUserSettingsIni);
        }
    }

    int32 GetFigureDivisor()
    {
        // One figure for each man unless the player chose otherwise (it is a setting of its own, not a part of the quality preset).
        int32 Divisor = 1;
        if (GConfig) { GConfig->GetInt(TEXT("/Script/Strategy1864.Settings"), TEXT("FigureDivisor"), Divisor, GGameUserSettingsIni); }
        // Explicit launch options win at startup; HUD choices can change the current battle.
        FParse::Value(FCommandLine::Get(), TEXT("Strategy1864FieldLOD="), Divisor);
        return FMath::Max(1, Divisor);
    }

    void ApplyPreset(UWorld* World, int32 Preset, bool bSave)
    {
        Preset = FMath::Clamp(Preset, 0, 2);
        Scalability::FQualityLevels Quality;
        // UE medium/high/epic groups; keep textures and readable unit silhouettes.
        Quality.SetFromSingleQualityLevel(Preset + 1);
        const float Resolution[] = { 70.0f, 85.0f, 100.0f };
        Quality.ResolutionQuality = Resolution[Preset];
        Scalability::SetQualityLevels(Quality, true);
        // CDOs hold smoke assets strongly, before the first combat volley.
        GetDefault<AStrategyBattleBlast>();
        GetDefault<AStrategyMuzzleSmokePuff>();
        GetDefault<AStrategyCourierRider>();
        // Same priority as scalability: switching back to HOEJ restores its original UE group values.
        {
            struct FBattlePresetCVar { const TCHAR* Name; float Low; float Medium; };
            const FBattlePresetCVar BattlePresetCVars[] = {
                { TEXT("r.Lumen.DiffuseIndirect.Allow"), 0.f, 1.f },
                { TEXT("r.Lumen.Reflections.Allow"), 0.f, 1.f },
                { TEXT("r.SSR.Quality"), 0.f, 2.f },
                { TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"), 32.f, 32.f },
                { TEXT("r.Lumen.Reflections.DownsampleFactor"), 2.f, 2.f },
                { TEXT("r.Shadow.MaxResolution"), 512.f, 1024.f },
                { TEXT("r.Shadow.CSM.MaxCascades"), 1.f, 4.f },
                { TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"), 1.f, 0.f },
                { TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectionalMoving"), 1.f, 0.f }
            };
            for (const FBattlePresetCVar& BattleSetting : BattlePresetCVars)
            {
                if (IConsoleVariable* BattleVar = IConsoleManager::Get().FindConsoleVariable(BattleSetting.Name))
                {
                    if (Preset < 2 && Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Renderer")) &&
                        (BattleVar->GetFlags() & (ECVF_ReadOnly | ECVF_Cheat)) == 0)
                        BattleVar->Set(Preset == 0 ? BattleSetting.Low : BattleSetting.Medium, ECVF_SetByScalability);
                    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-PERF: preset %d, %s=%s"), Preset, BattleSetting.Name, *BattleVar->GetString());
                }
                else UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-PERF: missing CVar %s"), BattleSetting.Name);
            }
        }
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-PERF: preset %d applied (HOEJ uses existing epic groups); renderer=%d, figures=%d, vegetation=%d, effects=%d, warmup=%d, markers=%d"),
            Preset, Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Renderer")), Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Figures")),
            Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Vegetation")), Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Effects")),
            Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Warmup")), Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Markers")));
        ApplyShadows(World);
        const int32 Divisor = GetFigureDivisor();
        if (World)
        {
            for (TActorIterator<ACavalryUnit> CavalryIt(World); CavalryIt; ++CavalryIt)
            {
                if (IsValid(*CavalryIt))
                    if (UStrategyCavalryVisualComponent* CavalryVisual = CavalryIt->FindComponentByClass<UStrategyCavalryVisualComponent>())
                        CavalryVisual->MenPerHorseman = Divisor;
            }
            for (TActorIterator<AStrategyCompanyUnit> It(World); It; ++It)
            {
                if (IsValid(*It) && It->InfantryVisualComponent) { It->InfantryVisualComponent->SetVisualScaleDivisor(Divisor); }
            }
        }
        if (bSave && GConfig)
        {
            GConfig->SetInt(TEXT("/Script/Strategy1864.Settings"), TEXT("BattleQuality"), Preset, GGameUserSettingsIni);
            GConfig->Flush(false, GGameUserSettingsIni);
        }
    }
}
