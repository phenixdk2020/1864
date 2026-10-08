#pragma once
#include "CoreMinimal.h"
class UWorld;

// Battle-only policy; unique namespace also keeps unity builds collision-free.
namespace Strategy1864BattleQuality
{
    int32 GetPreset();
    int32 GetFigureDivisor();
    bool GetShadowsOn();                                      // all shadows on or off (saved; -Strategy1864NoShadows turns them off)
    void SetShadowsOn(UWorld* World, bool bOn, bool bSave);   // switches every light's shadow casting
    void ApplyShadows(UWorld* World);                         // re-applies the choice (lights added later are caught too)
    void SetFigureDivisor(int32 Divisor);   // how many men a figure stands for (saved); the quality preset does not change it
    void ApplyPreset(UWorld* World, int32 Preset, bool bSave);
}
