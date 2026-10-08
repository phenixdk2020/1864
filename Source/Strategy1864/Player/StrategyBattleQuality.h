#pragma once
#include "CoreMinimal.h"
class UWorld;

// Battle-only policy; unique namespace also keeps unity builds collision-free.
namespace Strategy1864BattleQuality
{
    int32 GetPreset();
    int32 GetFigureDivisor();
    void ApplyPreset(UWorld* World, int32 Preset, bool bSave);
}
