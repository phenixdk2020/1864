#include "StrategyArtilleryProjectilePresentation.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"

AStrategyArtilleryProjectilePresentation::AStrategyArtilleryProjectilePresentation()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot =
        CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SetActorEnableCollision(false);
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

    if (Spec.Style == EStrategyProjectilePresentationStyle::Canister)
    {
        bImpacted = true;
        SetActorLocation(Spec.LaunchLocation);
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
        DrawCanisterPresentation();

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

        if (Alpha >= 1.0f)
        {
            bImpacted = true;
            SetActorLocation(ImpactLocation);
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

    DrawPresentationDebug();
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
