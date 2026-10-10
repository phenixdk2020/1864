#include "StrategyBattlePerformance.h"
#include "../Visual/StrategyInfantryVisualComponent.h"
#include "../Visual/StrategyCavalryVisualComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Units/CavalryUnit.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RenderTimer.h"
#include "RenderingThread.h"
#include "DynamicRHI.h"
#include "RHIStats.h"
#include <atomic>

namespace
{
    TAutoConsoleVariable<int32> BattlePerfRenderer(TEXT("Strategy1864.Perf.Renderer"), 1, TEXT("Battle renderer preset overrides; 0 restores scalability policy."));
    TAutoConsoleVariable<int32> BattlePerfFigures(TEXT("Strategy1864.Perf.Figures"), 1, TEXT("Near animated infantry budget."));
    TAutoConsoleVariable<int32> BattlePerfNearCap(TEXT("Strategy1864.Perf.NearCap"), 400, TEXT("Animated infantry figures per world; 0 is unlimited."));
    TAutoConsoleVariable<float> BattlePerfNearCm(TEXT("Strategy1864.Perf.NearCm"), 4000.f, TEXT("Near infantry distance in cm; hysteresis is 20 percent."));
    TAutoConsoleVariable<int32> BattlePerfVegetation(TEXT("Strategy1864.Perf.Vegetation"), 1, TEXT("Vegetation culling and low preset grass density; rebuild field to change."));
    TAutoConsoleVariable<int32> BattlePerfEffects(TEXT("Strategy1864.Perf.Effects"), 1, TEXT("Visual smoke/blast budget and distance fade."));
    TAutoConsoleVariable<int32> BattlePerfEffectCap(TEXT("Strategy1864.Perf.EffectCap"), 60, TEXT("Combined active visual smoke/blast actors per world; 0 is unlimited."));
    TAutoConsoleVariable<int32> BattlePerfWarmup(TEXT("Strategy1864.Perf.Warmup"), 1, TEXT("Prepare VAT when figures are created rather than at first LOD transition."));
    TAutoConsoleVariable<int32> BattlePerfMarkers(TEXT("Strategy1864.Perf.Markers"), 1, TEXT("Skip distant world marker drawing."));
    TAutoConsoleVariable<int32> BattlePerfHitches(TEXT("Strategy1864.Perf.Hitches"), 1, TEXT("Cache visual unit enumeration for 0.5 seconds and reuse VAT scratch arrays."));
    TAutoConsoleVariable<float> BattlePerfSkyCaptureSeconds(TEXT("Strategy1864.Perf.SkyCaptureSeconds"), 10.f, TEXT("Minimum real seconds between battle skylight captures; Hitches=0 disables throttling."));
    TArray<TWeakObjectPtr<UStrategyInfantryVisualComponent>> BattlePerfVisuals;
    TSet<TWeakObjectPtr<UStrategyInfantryVisualComponent>> BattlePerfNear;
    TArray<TWeakObjectPtr<AActor>> BattlePerfEffectsActive;
    TWeakObjectPtr<UWorld> BattlePerfWorld;
    double BattlePerfNextBudget = 0;
    double BattlePerfNextLog = 0;
    double BattlePerfPreviousFrame = 0;
    double BattlePerfWorst = 0;
    std::atomic<uint32> BattlePerfDrawCycles{0}, BattlePerfGpuCycles{0};
    std::atomic<int32> BattlePerfDrawCalls{0}, BattlePerfTris{0};
}

bool Strategy1864Performance::Enabled(const TCHAR* Name)
{
    const IConsoleVariable* BattlePerfVar = IConsoleManager::Get().FindConsoleVariable(Name);
    return BattlePerfVar && BattlePerfVar->GetInt() != 0;
}

const TArray<TWeakObjectPtr<AStrategyUnit>>& Strategy1864Performance::VisualUnits(UWorld* World)
{
    struct FBattleVisualUnitCache
    {
        TArray<TWeakObjectPtr<AStrategyUnit>> Units;
        double NextRefresh = 0;
    };
    static TMap<TWeakObjectPtr<UWorld>, FBattleVisualUnitCache> BattleUnitCaches;
    for (auto BattleCacheIt = BattleUnitCaches.CreateIterator(); BattleCacheIt; ++BattleCacheIt)
        if (!BattleCacheIt.Key().IsValid()) BattleCacheIt.RemoveCurrent();
    FBattleVisualUnitCache& BattleCache = BattleUnitCaches.FindOrAdd(World);
    const double BattleNow = FPlatformTime::Seconds();
    if (!BattlePerfHitches.GetValueOnGameThread() || BattleNow >= BattleCache.NextRefresh)
    {
        BattleCache.NextRefresh = BattleNow + 0.5;
        BattleCache.Units.Reset();
        for (TActorIterator<AStrategyUnit> BattleUnitIt(World); BattleUnitIt; ++BattleUnitIt)
            if (IsValid(*BattleUnitIt)) BattleCache.Units.Add(*BattleUnitIt);
    }
    return BattleCache.Units;
}

void Strategy1864Performance::RegisterFigures(UStrategyInfantryVisualComponent* Visual)
{
    BattlePerfVisuals.AddUnique(Visual);
}

bool Strategy1864Performance::AllowNear(UStrategyInfantryVisualComponent* Visual)
{
    if (!BattlePerfFigures.GetValueOnGameThread() || BattlePerfNearCap.GetValueOnGameThread() <= 0) return true;
    UWorld* BattleWorld = Visual->GetWorld();
    APlayerController* BattlePC = BattleWorld ? BattleWorld->GetFirstPlayerController() : nullptr;
    if (!BattlePC || !BattlePC->PlayerCameraManager) return true;
    const double BattleNow = FPlatformTime::Seconds();
    if (BattlePerfWorld.Get() != BattleWorld || BattleNow >= BattlePerfNextBudget)
    {
        BattlePerfWorld = BattleWorld;
        BattlePerfNextBudget = BattleNow + 0.5;
        BattlePerfVisuals.RemoveAll([](const auto& Entry) { return !Entry.IsValid() || !IsValid(Entry->GetOwner()); });
        const FVector BattleCamera = BattlePC->PlayerCameraManager->GetCameraLocation();
        BattlePerfVisuals.Sort([BattleCamera](const auto& A, const auto& B)
        {
            const double BattleADistance = FVector::DistSquared(BattleCamera, A->GetOwner()->GetActorLocation()) * (A->IsCrowdMode() ? 1.1 : 1.0);
            const double BattleBDistance = FVector::DistSquared(BattleCamera, B->GetOwner()->GetActorLocation()) * (B->IsCrowdMode() ? 1.1 : 1.0);
            return BattleADistance < BattleBDistance;
        });
        BattlePerfNear.Reset();
        int32 BattleRemaining = BattlePerfNearCap.GetValueOnGameThread();
        for (const auto& Entry : BattlePerfVisuals)
        {
            if (Entry->GetWorld() != BattleWorld || !Entry->bEnabled) continue;
            const int32 BattleCount = Entry->GetRenderedSoldierCount() + Entry->GetCorpseCount();
            if (BattleCount <= 0 || BattleCount > BattleRemaining) continue;
            BattlePerfNear.Add(Entry);
            BattleRemaining -= BattleCount;
        }
    }
    return BattlePerfNear.Contains(Visual);
}

bool Strategy1864Performance::CanSpawnEffect(UWorld* World)
{
    if (!BattlePerfEffects.GetValueOnGameThread()) return true;
    BattlePerfEffectsActive.RemoveAll([](const auto& Entry) { return !Entry.IsValid() || Entry->IsActorBeingDestroyed(); });
    const int32 BattleCap = BattlePerfEffectCap.GetValueOnGameThread();
    int32 BattleActive = 0;
    for (const auto& Entry : BattlePerfEffectsActive) if (Entry->GetWorld() == World) ++BattleActive;
    if (BattleCap > 0 && BattleActive >= BattleCap) return false;
    return true;
}

bool Strategy1864Performance::AdmitEffect(AActor* Effect)
{
    if (!CanSpawnEffect(Effect->GetWorld())) return false;
    BattlePerfEffectsActive.Add(Effect);
    return true;
}

float Strategy1864Performance::EffectFade(const AActor* Effect)
{
    if (!BattlePerfEffects.GetValueOnGameThread()) return 1.f;
    const APlayerController* BattlePC = Effect->GetWorld()->GetFirstPlayerController();
    if (!BattlePC || !BattlePC->PlayerCameraManager) return 1.f;
    const float BattleDistance = FVector::Distance(Effect->GetActorLocation(), BattlePC->PlayerCameraManager->GetCameraLocation());
    return FMath::Clamp((30000.f - BattleDistance) / 10000.f, 0.f, 1.f);
}

void Strategy1864Performance::Tick(APlayerController* Controller, float DeltaTime)
{
    static const bool BattleDebug = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugGpu"));
    if (!Controller->IsLocalController()) return;
    const double BattleNow = FPlatformTime::Seconds();
    static double BattleConfigLogAt = 0;
    static FString BattleLastConfig;
    static TWeakObjectPtr<UWorld> BattleLogWorld;
    if (BattleLogWorld.Get() != Controller->GetWorld())
    {
        BattleLogWorld = Controller->GetWorld();
        BattlePerfPreviousFrame = 0;
        BattlePerfWorst = 0;
        BattlePerfNextLog = BattleNow + 1.0;
        BattleConfigLogAt = 0;
        BattleLastConfig.Reset();
    }
    if (BattleNow >= BattleConfigLogAt)
    {
        BattleConfigLogAt = BattleNow + 0.5;
        const FString BattleConfig = FString::Printf(TEXT("renderer=%d figures=%d near=%.0f cm cap=%d vegetation=%d effects=%d cap=%d warmup=%d hitches=%d sky=%.1f s markers=%d debugGpu=%d"),
            BattlePerfRenderer.GetValueOnGameThread(), BattlePerfFigures.GetValueOnGameThread(), BattlePerfNearCm.GetValueOnGameThread(), BattlePerfNearCap.GetValueOnGameThread(),
            BattlePerfVegetation.GetValueOnGameThread(), BattlePerfEffects.GetValueOnGameThread(), BattlePerfEffectCap.GetValueOnGameThread(), BattlePerfWarmup.GetValueOnGameThread(),
            BattlePerfHitches.GetValueOnGameThread(), BattlePerfSkyCaptureSeconds.GetValueOnGameThread(), BattlePerfMarkers.GetValueOnGameThread(), BattleDebug);
        if (BattleConfig != BattleLastConfig)
        {
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-PERF: %s"), *BattleConfig);
            BattleLastConfig = BattleConfig;
        }
    }
    if (!BattleDebug) return;
    const double BattleFrameMs = BattlePerfPreviousFrame > 0 ? (BattleNow - BattlePerfPreviousFrame) * 1000.0 : DeltaTime * 1000.0;
    BattlePerfPreviousFrame = BattleNow;
    BattlePerfWorst = FMath::Max(BattlePerfWorst, BattleFrameMs);
    ENQUEUE_RENDER_COMMAND(Strategy1864SampleGpu)([](FRHICommandListImmediate&)
    {
        BattlePerfDrawCycles.store(GRenderThreadTime);
        BattlePerfGpuCycles.store(RHIGetGPUFrameCycles());
        BattlePerfDrawCalls.store(GNumDrawCallsRHI[0]);
        BattlePerfTris.store(GNumPrimitivesDrawnRHI[0]);
    });
    if (BattleNow < BattlePerfNextLog) return;
    BattlePerfNextLog = BattleNow + 1.0;
    int32 BattleFigures = 0;
    for (const auto& Entry : BattlePerfVisuals)
        if (Entry.IsValid() && Entry->GetWorld() == Controller->GetWorld() && Entry->bEnabled)
            BattleFigures += Entry->GetRenderedSoldierCount() + Entry->GetCorpseCount();
    for (const auto& BattleUnit : VisualUnits(Controller->GetWorld()))
        if (BattleUnit.IsValid())
            if (const ACavalryUnit* BattleCavalry = Cast<ACavalryUnit>(BattleUnit.Get()))
                if (const UStrategyCavalryVisualComponent* BattleCavalryVisual = BattleCavalry->FindComponentByClass<UStrategyCavalryVisualComponent>())
                    BattleFigures += BattleCavalryVisual->GetRenderedHorsemanCount();
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-GPU: frame %.2f ms, game %.2f ms, draw %.2f ms, gpu %.2f ms, figures %d, draw calls %d, tris %d, worst frame %.2f ms"),
        BattleFrameMs, FPlatformTime::ToMilliseconds(GGameThreadTime), FPlatformTime::ToMilliseconds(BattlePerfDrawCycles.load()),
        FPlatformTime::ToMilliseconds(BattlePerfGpuCycles.load()), BattleFigures, BattlePerfDrawCalls.load(), BattlePerfTris.load(), BattlePerfWorst);
    BattlePerfWorst = 0;
}
