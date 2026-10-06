#include "StrategyArtilleryProjectilePresentation.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "../Visual/StrategyBattleBlast.h"
#include "UObject/ConstructorHelpers.h"

AStrategyArtilleryProjectilePresentation::AStrategyArtilleryProjectilePresentation()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot =
        CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
    BallMesh->SetupAttachment(SceneRoot);
    BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BallMesh->SetCastShadow(false);
    BallMesh->bVisibleInRayTracing = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        BallMesh->SetStaticMesh(Sphere.Object);
    }
    // Larger than a real 12-pounder ball (12 cm): it must be seen from the commander's height.
    BallMesh->SetRelativeScale3D(FVector(0.28f));

    SetActorEnableCollision(false);
}

void AStrategyArtilleryProjectilePresentation::StrikeAt(const FVector& At, bool bBounce)
{
    UWorld* World = GetWorld();
    const FVector Dir = (ImpactLocation - Spec.LaunchLocation).GetSafeNormal2D();
    if (bBounce)
    {
        AStrategyBattleBlast::Spawn(World, EStrategyBlastKind::GroundImpact, At, Dir, 0.65f);
        return;
    }
    switch (Spec.Style)
    {
    case EStrategyProjectilePresentationStyle::Shell:
        AStrategyBattleBlast::Spawn(World, EStrategyBlastKind::ShellBurst, At, Dir, bFromMortar ? 1.4f : 1.0f);
        break;
    case EStrategyProjectilePresentationStyle::Shrapnel:
        AStrategyBattleBlast::Spawn(World, EStrategyBlastKind::AirBurst, At, Dir, 1.0f);
        break;
    default:
        AStrategyBattleBlast::Spawn(World, bFromMortar ? EStrategyBlastKind::ShellBurst : EStrategyBlastKind::GroundImpact, At, Dir, 1.0f);
        break;
    }
}

void AStrategyArtilleryProjectilePresentation::InitializePresentation(
    const FStrategyArtilleryProjectileSpec& InSpec,
    const TArray<FVector>& InTrajectoryPoints,
    const FVector& InFinalPoint,
    bool bInDrawTrajectory)
{
    Spec = InSpec;
    TrajectoryPoints = InTrajectoryPoints;
    ImpactLocation = InFinalPoint;
    bDrawTrajectory = bInDrawTrajectory;
    ElapsedSeconds = 0.0f;
    PostImpactElapsedSeconds = 0.0f;
    bImpacted = false;

    SetActorLocation(Spec.LaunchLocation);

    if (BallMesh)
    {
        // A dark ball: the simple translucent material, full opacity.
        if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent")))
        {
            UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, this);
            Mid->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.03f, 0.03f, 1.0f));
            BallMesh->SetMaterial(0, Mid);
        }
        BallMesh->SetRelativeScale3D(FVector(bFromMortar ? 0.4f : 0.28f));
    }

    if (Spec.Style == EStrategyProjectilePresentationStyle::Canister)
    {
        bImpacted = true;
        SetActorLocation(Spec.LaunchLocation);
        if (BallMesh) { BallMesh->SetVisibility(false); }
        AStrategyBattleBlast::Spawn(GetWorld(), EStrategyBlastKind::Canister, Spec.LaunchLocation, Spec.PrimaryImpactLocation - Spec.LaunchLocation, 1.0f,
            FVector::Dist2D(Spec.LaunchLocation, Spec.PrimaryImpactLocation));
    }
}

FVector AStrategyArtilleryProjectilePresentation::EvaluateTrajectory(
    float Alpha) const
{
    if (TrajectoryPoints.Num() == 0)
    {
        return Spec.PrimaryImpactLocation;
    }

    if (TrajectoryPoints.Num() == 1)
    {
        return TrajectoryPoints[0];
    }

    const float Clamped = FMath::Clamp(Alpha, 0.0f, 1.0f);
    const float Scaled =
        Clamped * static_cast<float>(TrajectoryPoints.Num() - 1);

    const int32 IndexA =
        FMath::Clamp(
            FMath::FloorToInt(Scaled),
            0,
            TrajectoryPoints.Num() - 1);

    const int32 IndexB =
        FMath::Min(IndexA + 1, TrajectoryPoints.Num() - 1);

    const float LocalAlpha = Scaled - static_cast<float>(IndexA);

    return FMath::Lerp(
        TrajectoryPoints[IndexA],
        TrajectoryPoints[IndexB],
        LocalAlpha);
}

void AStrategyArtilleryProjectilePresentation::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (Spec.Style == EStrategyProjectilePresentationStyle::Canister)
    {
        if (bDrawProjectilePoint)
        {
            DrawCanisterPresentation();
        }

        PostImpactElapsedSeconds += DeltaTime;
        if (PostImpactElapsedSeconds >= 0.45f)
        {
            Destroy();
        }
        return;
    }

    if (!bImpacted)
    {
        ElapsedSeconds += DeltaTime;

        const float FlightSeconds =
            FMath::Max(0.05f, Spec.FlightSeconds);

        const float Alpha =
            FMath::Clamp(
                ElapsedSeconds / FlightSeconds,
                0.0f,
                1.0f);

        SetActorLocation(EvaluateTrajectory(Alpha));

        // A round shot bounding over the field throws up the earth where it touches.
        if (Spec.Style == EStrategyProjectilePresentationStyle::RoundShot && TrajectoryPoints.Num() > 2)
        {
            const int32 Now = FMath::FloorToInt(Alpha * (TrajectoryPoints.Num() - 1));
            for (int32 i = FMath::Max(1, PassedPoint + 1); i <= Now && i < TrajectoryPoints.Num() - 1; ++i)
            {
                const FVector& P = TrajectoryPoints[i];
                const bool bLow = P.Z - UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, P) < 40.0f;
                const bool bDip = P.Z <= TrajectoryPoints[i - 1].Z && P.Z <= TrajectoryPoints[i + 1].Z;
                if (bLow && bDip)
                {
                    StrikeAt(P, true);
                }
            }
            PassedPoint = FMath::Max(PassedPoint, Now);
        }

        if (Alpha >= 1.0f)
        {
            bImpacted = true;
            SetActorLocation(ImpactLocation);
            if (BallMesh) { BallMesh->SetVisibility(Spec.Style == EStrategyProjectilePresentationStyle::RoundShot); }
            StrikeAt(ImpactLocation, false);
        }
    }
    else
    {
        PostImpactElapsedSeconds += DeltaTime;

        if (PostImpactElapsedSeconds >= PostImpactLifetimeSeconds)
        {
            Destroy();
            return;
        }
    }

    if (bDrawProjectilePoint || bDrawTrajectory)
    {
        DrawPresentationDebug();
    }
}

void AStrategyArtilleryProjectilePresentation::DrawPresentationDebug()
{
    if (!GetWorld())
    {
        return;
    }

    if (bDrawProjectilePoint && !bImpacted)
    {
        DrawDebugPoint(
            GetWorld(),
            GetActorLocation(),
            ProjectilePointSize,
            FColor::White,
            false,
            0.0f);
    }

    if (bImpacted)
    {
        DrawDebugPoint(
            GetWorld(),
            ImpactLocation,
            ProjectilePointSize * 1.5f,
            FColor::Red,
            false,
            0.0f);

        if (Spec.Style == EStrategyProjectilePresentationStyle::Shell)
        {
            DrawDebugSphere(
                GetWorld(),
                ImpactLocation,
                180.0f,
                12,
                FColor::Orange,
                false,
                0.0f,
                0,
                2.0f);
        }
        else if (Spec.Style == EStrategyProjectilePresentationStyle::Shrapnel)
        {
            FVector Forward =
                ImpactLocation - Spec.LaunchLocation;
            Forward.Z = 0.0f;
            Forward = Forward.GetSafeNormal();

            const FVector Right(-Forward.Y, Forward.X, 0.0f);

            for (int32 Index = -4; Index <= 4; ++Index)
            {
                FVector FragmentDirection =
                    (Forward +
                     Right * (static_cast<float>(Index) * 0.045f) +
                     FVector(0.0f, 0.0f, -0.10f))
                    .GetSafeNormal();

                DrawDebugLine(
                    GetWorld(),
                    ImpactLocation,
                    ImpactLocation + FragmentDirection * 900.0f,
                    FColor::Yellow,
                    false,
                    0.0f,
                    0,
                    1.0f);
            }
        }
    }

    if (!bDrawTrajectory || TrajectoryPoints.Num() < 2)
    {
        return;
    }

    for (int32 Index = 1; Index < TrajectoryPoints.Num(); ++Index)
    {
        DrawDebugLine(
            GetWorld(),
            TrajectoryPoints[Index - 1],
            TrajectoryPoints[Index],
            FColor::Cyan,
            false,
            0.0f,
            0,
            2.0f);
    }
}

void AStrategyArtilleryProjectilePresentation::DrawCanisterPresentation()
{
    if (!GetWorld())
    {
        return;
    }

    FVector Forward =
        Spec.PrimaryImpactLocation - Spec.LaunchLocation;
    Forward.Z = 0.0f;

    const float Range = Forward.Size();
    Forward = Forward.GetSafeNormal();

    if (Forward.IsNearlyZero())
    {
        return;
    }

    const FVector Right(-Forward.Y, Forward.X, 0.0f);

    const int32 RayCount = 11;

    for (int32 Index = 0; Index < RayCount; ++Index)
    {
        const float T =
            RayCount > 1
            ? static_cast<float>(Index) /
              static_cast<float>(RayCount - 1)
            : 0.5f;

        const float Lateral =
            FMath::Lerp(-0.18f, 0.18f, T);

        FVector Direction =
            (Forward + Right * Lateral).GetSafeNormal();

        const FVector End =
            Spec.LaunchLocation +
            Direction * Range;

        DrawDebugLine(
            GetWorld(),
            Spec.LaunchLocation,
            End,
            FColor(190, 190, 190),
            false,
            0.0f,
            0,
            1.0f);
    }
}
