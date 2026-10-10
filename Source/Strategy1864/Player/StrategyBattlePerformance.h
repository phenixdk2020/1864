#pragma once
#include "CoreMinimal.h"
class UStrategyInfantryVisualComponent;
class APlayerController;
class AActor;
class AStrategyUnit;
class UWorld;

namespace Strategy1864Performance
{
    void RegisterFigures(UStrategyInfantryVisualComponent* Visual);
    bool AllowNear(UStrategyInfantryVisualComponent* Visual);
    void Tick(APlayerController* Controller, float DeltaTime);
    bool AdmitEffect(AActor* Effect);
    bool CanSpawnEffect(UWorld* World);
    float EffectFade(const AActor* Effect);
    bool Enabled(const TCHAR* Name);
    const TArray<TWeakObjectPtr<AStrategyUnit>>& VisualUnits(UWorld* World);
}
