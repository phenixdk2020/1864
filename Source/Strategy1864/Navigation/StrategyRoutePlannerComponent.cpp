#include "StrategyRoutePlannerComponent.h"

#include "StrategyRiverBarrier.h"
#include "StrategyNavigationObstacle.h"
#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"

UStrategyRoutePlannerComponent::UStrategyRoutePlannerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

TArray<FVector> UStrategyRoutePlannerComponent::BuildRoute(
    const FVector& StartLocation,
    const FVector& EndLocation) const
{
    return BuildRoutePlan(StartLocation, EndLocation).Points;
}

FStrategyRoutePlan UStrategyRoutePlannerComponent::BuildRoutePlan(
    const FVector& StartLocation,
    const FVector& EndLocation) const
{
    FStrategyRoutePlan Plan;

    AStrategyRiverBarrier* River = FindRelevantRiverBarrier(
        StartLocation,
        EndLocation);

    if (!River)
    {
        AppendNavSegment(StartLocation, EndLocation, Plan.Points);
        ApplyStaticObstacleDetours(StartLocation, EndLocation, Plan.Points);
        ApplyTacticalTerrainElevation(Plan.Points);

        if (!ValidateSlopeProfile(StartLocation, Plan.Points, Plan.FailureReason))
        {
            Plan.bValid = false;
        }

        return Plan;
    }

    if (River->IsInsideRiver(StartLocation))
    {
        Plan.bValid = false;
        Plan.FailureReason = TEXT("Start location is inside hard-blocked water.");
        return Plan;
    }

    if (River->IsInsideRiver(EndLocation))
    {
        Plan.bValid = false;
        Plan.FailureReason = TEXT("Destination is inside hard-blocked water.");
        return Plan;
    }

    if (River->RequiresBankChange(StartLocation, EndLocation))
    {
        const int32 StartSide = River->GetBankSide(StartLocation);
        const int32 EndSide = River->GetBankSide(EndLocation);

        const FVector NearApproach = River->GetBridgeApproachForSide(StartSide);
        const FVector FarApproach = River->GetBridgeApproachForSide(EndSide);
        const FVector FarExit = River->GetBridgeExitForSide(EndSide);

        AppendNavSegment(StartLocation, NearApproach, Plan.Points);
        Plan.BridgeEnterPointIndex = Plan.Points.Num() - 1;

        Plan.Points.Add(FarApproach);
        Plan.Points.Add(FarExit);
        Plan.BridgeExitPointIndex = Plan.Points.Num() - 1;
        Plan.bUsesBridge = true;
        Plan.BridgeBarrier = River;

        AppendNavSegment(FarExit, EndLocation, Plan.Points);

        // Preserve explicit bridge enter/exit indices. The pre/post bridge
        // segments already use NavMesh; extra static detours are skipped here.
        ApplyTacticalTerrainElevation(Plan.Points);

        if (!ValidateSlopeProfile(StartLocation, Plan.Points, Plan.FailureReason))
        {
            Plan.bValid = false;
        }

        return Plan;
    }

    if (River->SameBankChordIntersectsRiver(StartLocation, EndLocation))
    {
        const int32 Side = River->GetBankSide(StartLocation);
        const FVector BankWaypoint = River->GetBridgeExitForSide(Side);

        AppendNavSegment(StartLocation, BankWaypoint, Plan.Points);
        AppendNavSegment(BankWaypoint, EndLocation, Plan.Points);

        Plan.bSameBankDetour = true;
        ApplyStaticObstacleDetours(StartLocation, EndLocation, Plan.Points);
        ApplyTacticalTerrainElevation(Plan.Points);

        if (!ValidateSlopeProfile(StartLocation, Plan.Points, Plan.FailureReason))
        {
            Plan.bValid = false;
        }

        return Plan;
    }

    AppendNavSegment(StartLocation, EndLocation, Plan.Points);
    ApplyStaticObstacleDetours(StartLocation, EndLocation, Plan.Points);
    ApplyTacticalTerrainElevation(Plan.Points);

    if (!ValidateSlopeProfile(StartLocation, Plan.Points, Plan.FailureReason))
    {
        Plan.bValid = false;
    }

    return Plan;
}

AStrategyRiverBarrier* UStrategyRoutePlannerComponent::FindRelevantRiverBarrier(
    const FVector& StartLocation,
    const FVector& EndLocation) const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AStrategyRiverBarrier* BestBarrier = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();
    const FVector Midpoint = (StartLocation + EndLocation) * 0.5f;

    for (TActorIterator<AStrategyRiverBarrier> It(GetWorld()); It; ++It)
    {
        AStrategyRiverBarrier* Barrier = *It;
        if (!IsValid(Barrier))
        {
            continue;
        }

        if (!Barrier->IsInsideRiver(StartLocation) &&
            !Barrier->IsInsideRiver(EndLocation) &&
            !Barrier->RequiresBankChange(StartLocation, EndLocation) &&
            !Barrier->SameBankChordIntersectsRiver(StartLocation, EndLocation))
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared2D(
            Midpoint,
            Barrier->GetActorLocation());

        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            BestBarrier = Barrier;
        }
    }

    return BestBarrier;
}

void UStrategyRoutePlannerComponent::AppendNavSegment(
    const FVector& SegmentStart,
    const FVector& SegmentEnd,
    TArray<FVector>& InOutPoints) const
{
    const float Step = FMath::Max(250.0f, TerrainSampleStepCm);

    auto AppendSampledStraight =
        [&InOutPoints, Step](const FVector& From, const FVector& To)
        {
            const float Distance = FVector::Dist2D(From, To);
            const int32 Steps =
                FMath::Max(
                    1,
                    FMath::CeilToInt(Distance / Step));

            for (int32 Index = 1; Index <= Steps; ++Index)
            {
                const float Alpha =
                    static_cast<float>(Index) /
                    static_cast<float>(Steps);

                InOutPoints.Add(
                    FMath::Lerp(From, To, Alpha));
            }
        };

    if (!bUseNavigationSystem || !GetWorld())
    {
        AppendSampledStraight(SegmentStart, SegmentEnd);
        return;
    }

    UNavigationPath* Path =
        UNavigationSystemV1::FindPathToLocationSynchronously(
            GetWorld(),
            SegmentStart,
            SegmentEnd,
            GetOwner());

    if (!Path || !Path->IsValid() || Path->PathPoints.Num() < 2)
    {
        AppendSampledStraight(SegmentStart, SegmentEnd);
        return;
    }

    FVector Previous = SegmentStart;

    for (int32 Index = 1; Index < Path->PathPoints.Num(); ++Index)
    {
        const FVector& Point = Path->PathPoints[Index];
        AppendSampledStraight(Previous, Point);
        Previous = Point;
    }
}


void UStrategyRoutePlannerComponent::ApplyStaticObstacleDetours(
    const FVector& StartLocation,
    const FVector& EndLocation,
    TArray<FVector>& InOutPoints) const
{
    if (!GetWorld() || InOutPoints.Num() == 0)
    {
        return;
    }

    TArray<AStrategyNavigationObstacle*> Obstacles;
    for (TActorIterator<AStrategyNavigationObstacle> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It))
        {
            Obstacles.Add(*It);
        }
    }

    if (Obstacles.Num() == 0)
    {
        return;
    }

    TArray<FVector> Rebuilt;
    FVector SegmentStart = StartLocation;

    for (const FVector& SegmentEnd : InOutPoints)
    {
        AStrategyNavigationObstacle* BlockingObstacle = nullptr;

        for (AStrategyNavigationObstacle* Obstacle : Obstacles)
        {
            if (Obstacle->IntersectsSegment2D(SegmentStart, SegmentEnd))
            {
                BlockingObstacle = Obstacle;
                break;
            }
        }

        if (BlockingObstacle)
        {
            Rebuilt.Add(
                BlockingObstacle->BuildDetourPoint(
                    SegmentStart,
                    SegmentEnd));
        }

        Rebuilt.Add(SegmentEnd);
        SegmentStart = SegmentEnd;
    }

    InOutPoints = MoveTemp(Rebuilt);
}

void UStrategyRoutePlannerComponent::ApplyTacticalTerrainElevation(
    TArray<FVector>& InOutPoints) const
{
    for (FVector& Point : InOutPoints)
    {
        Point =
            UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
                this,
                Point);
    }
}

bool UStrategyRoutePlannerComponent::ValidateSlopeProfile(
    const FVector& StartLocation,
    const TArray<FVector>& Points,
    FString& OutFailureReason) const
{
    FVector Previous =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            this,
            StartLocation);

    for (const FVector& Point : Points)
    {
        const FVector Delta = Point - Previous;
        const float Horizontal = FVector(Delta.X, Delta.Y, 0.0f).Size();

        if (Horizontal > KINDA_SMALL_NUMBER)
        {
            const float SlopeDegrees =
                FMath::RadiansToDegrees(
                    FMath::Atan2(
                        FMath::Abs(Delta.Z),
                        Horizontal));

            if (SlopeDegrees > MaxTraversableSlopeDegrees)
            {
                OutFailureReason =
                    FString::Printf(
                        TEXT("Route slope %.1f exceeds %.1f degrees."),
                        SlopeDegrees,
                        MaxTraversableSlopeDegrees);
                return false;
            }
        }

        Previous = Point;
    }

    return true;
}
