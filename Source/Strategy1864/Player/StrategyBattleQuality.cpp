#include "StrategyBattleQuality.h"
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
        Scalability::SetQualityLevels(Quality);
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
