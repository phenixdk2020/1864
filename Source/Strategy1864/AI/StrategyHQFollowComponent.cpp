#include "StrategyHQFollowComponent.h"

#include "../Command/StrategyCommandComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyHQUnit.h"
#include "../Units/StrategyUnit.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Visual/StrategyInfantryVisualComponent.h"

namespace
{
    struct FHQDrawnFootprint { FTransform Frame; FBox Bounds; };

    void GatherHQDrawnFootprints(AStrategyUnit* HQSubordinate, TArray<FHQDrawnFootprint>& HQFootprints,
        TSet<AStrategyUnit*>& HQVisited)
    {
        if (!IsValid(HQSubordinate) || HQVisited.Contains(HQSubordinate)) return;
        HQVisited.Add(HQSubordinate);
        if (const UStrategyInfantryVisualComponent* HQVisual = HQSubordinate->FindComponentByClass<UStrategyInfantryVisualComponent>())
        {
            FHQDrawnFootprint HQFootprint;
            if (HQVisual->GetDrawnFormation(HQFootprint.Frame, HQFootprint.Bounds))
            {
                HQFootprints.Add(HQFootprint);
                return;
            }
        }
        if (const AStrategyHQUnit* HQChild = Cast<AStrategyHQUnit>(HQSubordinate))
        {
            if (HQChild->CommandComponent)
                for (AStrategyUnit* HQDescendant : HQChild->CommandComponent->CurrentSubordinates)
                    GatherHQDrawnFootprints(HQDescendant, HQFootprints, HQVisited);
            return;
        }
        // No drawn infantry (cavalry or asset-free tests): retain the actor fallback.
        HQFootprints.Add({HQSubordinate->GetActorTransform(), FBox(FVector(-100.f), FVector(100.f))});
    }

    TArray<FHQDrawnFootprint> GetHQDrawnFootprints(const AStrategyHQUnit* HQCommander)
    {
        TArray<FHQDrawnFootprint> HQFootprints;
        TSet<AStrategyUnit*> HQVisited;
        HQVisited.Add(const_cast<AStrategyHQUnit*>(HQCommander));
        if (HQCommander && HQCommander->CommandComponent)
            for (AStrategyUnit* HQSubordinate : HQCommander->CommandComponent->CurrentSubordinates)
                GatherHQDrawnFootprints(HQSubordinate, HQFootprints, HQVisited);
        return HQFootprints;
    }

    // 15 m clear space plus 5 m for the whole mounted staff, even while its pivot turns.
    constexpr float HQStaffClearanceCm = 2000.f;

    bool HQSegmentClear(const FVector& HQStart, const FVector& HQEnd, const TArray<FHQDrawnFootprint>& HQFootprints)
    {
        for (const FHQDrawnFootprint& HQFootprint : HQFootprints)
        {
            const FBox HQBox = HQFootprint.Bounds.ExpandBy(HQStaffClearanceCm);
            const FVector HQA = HQFootprint.Frame.InverseTransformPosition(HQStart);
            const FVector HQB = HQFootprint.Frame.InverseTransformPosition(HQEnd);
            double HQEnter = 0., HQExit = 1.;
            bool bHQIntersects = true;
            for (int32 HQAxis = 0; HQAxis < 2; ++HQAxis)
            {
                const double HQTravel = HQB[HQAxis] - HQA[HQAxis];
                if (FMath::Abs(HQTravel) < KINDA_SMALL_NUMBER)
                {
                    if (HQA[HQAxis] < HQBox.Min[HQAxis] || HQA[HQAxis] > HQBox.Max[HQAxis]) bHQIntersects = false;
                    continue;
                }
                const double HQT0 = (HQBox.Min[HQAxis] - HQA[HQAxis]) / HQTravel;
                const double HQT1 = (HQBox.Max[HQAxis] - HQA[HQAxis]) / HQTravel;
                HQEnter = FMath::Max(HQEnter, FMath::Min(HQT0, HQT1));
                HQExit = FMath::Min(HQExit, FMath::Max(HQT0, HQT1));
            }
            if (bHQIntersects && HQEnter <= HQExit) return false;
        }
        return true;
    }

    FVector HQClearWaypoint(const FVector& HQCurrent, const FVector& HQDesired,
        const TArray<FHQDrawnFootprint>& HQFootprints)
    {
        // A moving/wheeling company may envelop a stationary HQ. Walk out; never teleport it.
        for (const FHQDrawnFootprint& HQFootprint : HQFootprints)
        {
            const FBox HQBox = HQFootprint.Bounds.ExpandBy(HQStaffClearanceCm);
            FVector HQLocal = HQFootprint.Frame.InverseTransformPosition(HQCurrent);
            if (HQLocal.X >= HQBox.Min.X && HQLocal.X <= HQBox.Max.X &&
                HQLocal.Y >= HQBox.Min.Y && HQLocal.Y <= HQBox.Max.Y)
            {
                // Use the nearest edge to clear an encroaching formation quickly.
                const double HQDistances[] = {HQLocal.X - HQBox.Min.X, HQBox.Max.X - HQLocal.X,
                    HQLocal.Y - HQBox.Min.Y, HQBox.Max.Y - HQLocal.Y};
                int32 HQEdge = 0;
                for (int32 HQIndex = 1; HQIndex < 4; ++HQIndex)
                    if (HQDistances[HQIndex] < HQDistances[HQEdge]) HQEdge = HQIndex;
                if (HQEdge == 0) HQLocal.X = HQBox.Min.X - 10.f;
                if (HQEdge == 1) HQLocal.X = HQBox.Max.X + 10.f;
                if (HQEdge == 2) HQLocal.Y = HQBox.Min.Y - 10.f;
                if (HQEdge == 3) HQLocal.Y = HQBox.Max.Y + 10.f;
                FVector HQEscape = HQFootprint.Frame.TransformPosition(HQLocal);
                HQEscape.Z = HQCurrent.Z;
                return HQEscape;
            }
        }
        if (HQSegmentClear(HQCurrent, HQDesired, HQFootprints)) return HQDesired;
        // Visibility graph round expanded rectangles: handles several adjacent companies.
        TArray<FVector> HQNodes {HQCurrent, HQDesired};
        for (const FHQDrawnFootprint& HQFootprint : HQFootprints)
        {
            const FBox HQBox = HQFootprint.Bounds.ExpandBy(HQStaffClearanceCm + 10.f);
            for (int32 HQCorner = 0; HQCorner < 4; ++HQCorner)
            {
                FVector HQPoint = HQFootprint.Frame.TransformPosition(FVector(
                    (HQCorner & 1) ? HQBox.Max.X : HQBox.Min.X,
                    (HQCorner & 2) ? HQBox.Max.Y : HQBox.Min.Y, 0.f));
                HQPoint.Z = HQCurrent.Z;
                HQNodes.Add(HQPoint);
            }
        }
        TArray<double> HQCosts;
        TArray<int32> HQParents;
        TArray<bool> HQDone;
        HQCosts.Init(TNumericLimits<double>::Max(), HQNodes.Num());
        HQParents.Init(INDEX_NONE, HQNodes.Num());
        HQDone.Init(false, HQNodes.Num());
        HQCosts[0] = 0.;
        for (int32 HQIteration = 0; HQIteration < HQNodes.Num(); ++HQIteration)
        {
            int32 HQBest = INDEX_NONE;
            for (int32 HQIndex = 0; HQIndex < HQNodes.Num(); ++HQIndex)
                if (!HQDone[HQIndex] && (HQBest == INDEX_NONE || HQCosts[HQIndex] < HQCosts[HQBest])) HQBest = HQIndex;
            if (HQBest == INDEX_NONE || HQCosts[HQBest] == TNumericLimits<double>::Max()) break;
            if (HQBest == 1) break;
            HQDone[HQBest] = true;
            for (int32 HQNext = 1; HQNext < HQNodes.Num(); ++HQNext)
            {
                if (HQDone[HQNext] || !HQSegmentClear(HQNodes[HQBest], HQNodes[HQNext], HQFootprints)) continue;
                const double HQCost = HQCosts[HQBest] + FVector::Dist2D(HQNodes[HQBest], HQNodes[HQNext]);
                if (HQCost < HQCosts[HQNext]) { HQCosts[HQNext] = HQCost; HQParents[HQNext] = HQBest; }
            }
        }
        if (HQParents[1] == INDEX_NONE) return HQCurrent;
        int32 HQFirst = 1;
        while (HQParents[HQFirst] != 0) HQFirst = HQParents[HQFirst];
        return HQNodes[HQFirst];
    }
}

UStrategyHQFollowComponent::UStrategyHQFollowComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Read the companies after their pre-physics visual settling update.
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UStrategyHQFollowComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerHQ = Cast<AStrategyHQUnit>(GetOwner());
    ApplyLevelDefaults();
}

void UStrategyHQFollowComponent::ApplyLevelDefaults()
{
    if (!OwnerHQ)
    {
        OwnerHQ = Cast<AStrategyHQUnit>(GetOwner());
    }

    if (!OwnerHQ)
    {
        return;
    }

    switch (OwnerHQ->HQLevel)
    {
        case EStrategyHQLevel::Battalion:
            if (RearOffsetCm < 0.0f) { RearOffsetCm = 6000.0f; }
            LateralOffsetCm = 0.0f;
            if (FollowSpeedCmPerSecond < 0.0f) { FollowSpeedCmPerSecond = 650.0f; }
            break;

        case EStrategyHQLevel::Regiment:
            if (RearOffsetCm < 0.0f) { RearOffsetCm = 12000.0f; }
            LateralOffsetCm = 0.0f;
            if (FollowSpeedCmPerSecond < 0.0f) { FollowSpeedCmPerSecond = 630.0f; }
            break;

        case EStrategyHQLevel::Brigade:
            if (RearOffsetCm < 0.0f) { RearOffsetCm = 25000.0f; }
            LateralOffsetCm = 6500.0f;
            if (FollowSpeedCmPerSecond < 0.0f) { FollowSpeedCmPerSecond = 620.0f; }
            break;

        case EStrategyHQLevel::Division:
            if (RearOffsetCm < 0.0f) { RearOffsetCm = 40000.0f; }
            LateralOffsetCm = -7500.0f;
            if (FollowSpeedCmPerSecond < 0.0f) { FollowSpeedCmPerSecond = 590.0f; }
            break;

        default:
            break;
    }
}

void UStrategyHQFollowComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEnableFollow ||
        !OwnerHQ ||
        !OwnerHQ->CommandComponent ||
        OwnerHQ->CommandComponent->CurrentSubordinates.Num() == 0)
    {
        return;
    }

    // A direct MOVE belongs to the player. Background HQ follow must not fight it.
    if (OwnerHQ->OrderComponent)
    {
        const FStrategyOrder CurrentOrder = OwnerHQ->OrderComponent->GetCurrentOrder();
        if (CurrentOrder.Authority == EStrategyOrderAuthority::DirectPlayer &&
            CurrentOrder.Type == EStrategyOrderType::Move &&
            OwnerHQ->OrderComponent->IsPhysicallyExecuting())
        {
            return;
        }
    }

    FollowRetrySeconds = FMath::Max(0.0f, FollowRetrySeconds - DeltaTime);
    const FVector Desired = CalculateDesiredHQPosition();
    const FVector Current = OwnerHQ->GetActorLocation();
    const TArray<FHQDrawnFootprint> HQFootprints = GetHQDrawnFootprints(OwnerHQ);
    const FVector HQWaypoint = HQClearWaypoint(Current, Desired, HQFootprints);
    FVector Delta = HQWaypoint - Current;
    Delta.Z = 0.0f;

    const float Distance = Delta.Size();
    if (Distance <= KINDA_SMALL_NUMBER ||
        (Distance <= SettleToleranceCm && HQWaypoint.Equals(Desired, 0.1f)))
    {
        return;
    }

    // The HQ follows its companies directly. (Moving it through its order channel replaced the commander's own order, an
    // attack ordered by the player, with a follow move, so the companies lost their mission.)
    const FVector Step =
        Delta.GetSafeNormal() *
        FMath::Min(Distance, FollowSpeedCmPerSecond * DeltaTime);

    OwnerHQ->SetActorLocation(Current + Step);
}

FVector UStrategyHQFollowComponent::CalculateDesiredHQPosition() const
{
    if (!OwnerHQ || !OwnerHQ->CommandComponent)
    {
        return OwnerHQ ? OwnerHQ->GetActorLocation() : FVector::ZeroVector;
    }

    const TArray<FHQDrawnFootprint> HQFootprints = GetHQDrawnFootprints(OwnerHQ);
    if (HQFootprints.IsEmpty()) return OwnerHQ->GetActorLocation();
    FVector Centroid = FVector::ZeroVector;
    for (const FHQDrawnFootprint& HQFootprint : HQFootprints) Centroid += HQFootprint.Frame.GetLocation();
    Centroid /= static_cast<float>(HQFootprints.Num());

    float FacingYaw = OwnerHQ->GetActorRotation().Yaw;
    if (OwnerHQ->OrderComponent)
    {
        const FStrategyOrder CurrentOrder = OwnerHQ->OrderComponent->GetCurrentOrder();
        if (CurrentOrder.bHasFacing)
        {
            FacingYaw = CurrentOrder.FacingYaw;
        }
    }

    const FRotator FacingRotation(0.0f, FacingYaw, 0.0f);
    const FVector Forward = FacingRotation.Vector();
    const FVector Right = FRotationMatrix(FacingRotation).GetScaledAxis(EAxis::Y);

    // Use the rearmost drawn corner along the commander's facing, rather than actor centres.
    double HQRearProjection = TNumericLimits<double>::Max();
    for (const FHQDrawnFootprint& HQFootprint : HQFootprints)
    {
        for (int32 HQCorner = 0; HQCorner < 4; ++HQCorner)
        {
            const FVector HQPoint = HQFootprint.Frame.TransformPosition(FVector(
                (HQCorner & 1) ? HQFootprint.Bounds.Max.X : HQFootprint.Bounds.Min.X,
                (HQCorner & 2) ? HQFootprint.Bounds.Max.Y : HQFootprint.Bounds.Min.Y, 0.f));
            HQRearProjection = FMath::Min(HQRearProjection, FVector::DotProduct(HQPoint, Forward));
        }
    }
    const double HQMargin = FMath::Max(double(RearOffsetCm), double(HQStaffClearanceCm)) + SettleToleranceCm;
    return Centroid + Forward * (HQRearProjection - HQMargin - FVector::DotProduct(Centroid, Forward))
        + Right * LateralOffsetCm;
}
