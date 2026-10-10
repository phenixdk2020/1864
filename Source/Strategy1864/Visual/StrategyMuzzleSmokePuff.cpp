#include "StrategyMuzzleSmokePuff.h"
#include "../Player/StrategyBattlePerformance.h"
#include "../Player/StrategyBattleQuality.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr int32 SmokeBalls = 4;
}

AStrategyMuzzleSmokePuff::AStrategyMuzzleSmokePuff()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BattleSphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BattleLitAsset(TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BattleUnlitAsset(TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
    BattleSmokeSphere = BattleSphereAsset.Object;
    BattleSmokeLit = BattleLitAsset.Object;
    BattleSmokeUnlit = BattleUnlitAsset.Object;
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void AStrategyMuzzleSmokePuff::BeginPlay()
{
    Super::BeginPlay();
    if (!Strategy1864Performance::AdmitEffect(this)) { Destroy(); return; }
    Balls.Reserve(SmokeBalls); Materials.Reserve(SmokeBalls); Offsets.Reserve(SmokeBalls);
    // Engine content only: the basic sphere and the simple translucent material (its "Color" alpha is the opacity).
    UStaticMesh* Sphere = BattleSmokeSphere;
    UMaterialInterface* Translucent = Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Effects")) && Strategy1864BattleQuality::GetPreset() < 2 && BattleSmokeUnlit ? BattleSmokeUnlit.Get() : BattleSmokeLit.Get();
    if (!Sphere)
    {
        Destroy();
        return;
    }
    const int32 BattleSmokeBallCount = FMath::Min(SmokeBalls, 2 + Strategy1864BattleQuality::GetPreset());
    for (int32 b = 0; b < BattleSmokeBallCount; ++b)
    {
        UStaticMeshComponent* Ball = NewObject<UStaticMeshComponent>(this);
        Ball->SetupAttachment(Root);
        Ball->SetStaticMesh(Sphere);
        Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Ball->SetCastShadow(false);
        Ball->SetCullDistance(30000.0f);
        Ball->bVisibleInRayTracing = false;
        Ball->RegisterComponent();
        UMaterialInstanceDynamic* Mid = Translucent ? UMaterialInstanceDynamic::Create(Translucent, this) : nullptr;
        if (Mid)
        {
            Ball->SetMaterial(0, Mid);
        }
        Balls.Add(Ball);
        Materials.Add(Mid);
        Offsets.Add(FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-0.4f, 0.8f)));
    }
    LifetimeSeconds *= FMath::FRandRange(0.8f, 1.25f);
}

void AStrategyMuzzleSmokePuff::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Age += DeltaTime;
    const float T = FMath::Clamp(Age / FMath::Max(LifetimeSeconds, 0.1f), 0.0f, 1.0f);
    if (T >= 1.0f)
    {
        Destroy();
        return;
    }
    // The push of the shot dies away; the drift and rise go on.
    const float Push = FMath::Exp(-2.5f * Age);
    AddActorWorldOffset(FVector(Drift.X * (0.25f + Push), Drift.Y * (0.25f + Push), Drift.Z) * DeltaTime);
    const float Diameter = FMath::Lerp(StartDiameterCm, EndDiameterCm, FMath::Sqrt(T));
    const float Opacity = Strategy1864Performance::EffectFade(this) * StartOpacity * FMath::Pow(1.0f - T, 1.6f);
    for (int32 b = 0; b < Balls.Num(); ++b)
    {
        if (!Balls[b])
        {
            continue;
        }
        const float Size = Diameter * (0.7f + 0.15f * b) / 100.0f;
        Balls[b]->SetRelativeScale3D(FVector(Size, Size, Size * 0.85f));
        Balls[b]->SetRelativeLocation(Offsets[b] * Diameter * 0.35f);
        if (Materials.IsValidIndex(b) && Materials[b])
        {
            Materials[b]->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.82f, 0.81f, 0.78f, Opacity));
        }
    }
}
