#include "StrategyBattleQuality.h"
#include "Scalability.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "../Units/StrategyCompanyUnit.h"
#include "../Visual/StrategyInfantryVisualComponent.h"

namespace Strategy1864BattleQuality
{
    int32 GetPreset()
    {
        int32 Preset = 2;   // HØJ until the player chooses otherwise
        if (GConfig) { GConfig->GetInt(TEXT("/Script/Strategy1864.Settings"), TEXT("BattleQuality"), Preset, GGameUserSettingsIni); }
        return FMath::Clamp(Preset, 0, 2);
    }

    int32 GetFigureDivisor()
    {
        const int32 Divisors[] = { 5, 2, 1 };
        int32 Divisor = Divisors[GetPreset()];
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
        const int32 Divisors[] = { 5, 2, 1 };
        const int32 Divisor = bSave ? Divisors[Preset] : GetFigureDivisor();
        if (World)
        {
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
