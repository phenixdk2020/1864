#include "StrategyHQFollowComponent.h"

#include "../Command/StrategyCommandComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyHQUnit.h"
#include "../Units/StrategyUnit.h"
#include "../Movement/StrategyMovementExecutorComponent.h"

UStrategyHQFollowComponent::UStrategyHQFollowComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
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
            if (RearOffsetCm < 0.0f) { RearOffsetCm = 6500.0f; }
            LateralOffsetCm = 0.0f;
            if (FollowSpeedCmPerSecond < 0.0f) { FollowSpeedCmPerSecond = 650.0f; }
            break;

        case EStrategyHQLevel::Regiment:
            if (RearOffsetCm < 0.0f) { RearOffsetCm = 9000.0f; }
            LateralOffsetCm = 0.0f;
            if (FollowSpeedCmPerSecond < 0.0f) { FollowSpeedCmPerSecond = 630.0f; }
            break;

        case EStrategyHQLevel::Brigade:
            if (RearOffsetCm < 0.0f) { RearOffsetCm = 12000.0f; }
            LateralOffsetCm = 6500.0f;
            if (FollowSpeedCmPerSecond < 0.0f) { FollowSpeedCmPerSecond = 620.0f; }
            break;

        case EStrategyHQLevel::Division:
            if (RearOffsetCm < 0.0f) { RearOffsetCm = 14500.0f; }
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
    FVector Delta = Desired - Current;
    Delta.Z = 0.0f;

    const float Distance = Delta.Size();
    if (Distance <= SettleToleranceCm)
    {
        return;
    }

    // Let the movement executor own navigation, bridge queues and terrain movement.
    // Finish the current follow route before requesting a new one; do not reset it each frame.
    if (OwnerHQ->MovementExecutor && OwnerHQ->OrderComponent &&
        !OwnerHQ->MovementExecutor->HasMovementGoal() && FollowRetrySeconds <= 0.0f)
    {
        FollowRetrySeconds = 0.25f;
        FStrategyOrder HQFollowOrder;
        HQFollowOrder.Type = EStrategyOrderType::Move;
        HQFollowOrder.Authority = EStrategyOrderAuthority::InheritedAI;
        HQFollowOrder.TargetLocation = Desired;
        if (OwnerHQ->OrderComponent->SetOrder(HQFollowOrder))
        {
            OwnerHQ->MovementExecutor->MoveSpeedCmPerSecond = FollowSpeedCmPerSecond;
        }
    }
}

FVector UStrategyHQFollowComponent::CalculateDesiredHQPosition() const
{
    if (!OwnerHQ || !OwnerHQ->CommandComponent)
    {
        return OwnerHQ ? OwnerHQ->GetActorLocation() : FVector::ZeroVector;
    }

    FVector Centroid = FVector::ZeroVector;
    int32 ValidCount = 0;

    for (AStrategyUnit* Subordinate : OwnerHQ->CommandComponent->CurrentSubordinates)
    {
        if (IsValid(Subordinate))
        {
            Centroid += Subordinate->GetActorLocation();
            ++ValidCount;
        }
    }

    if (ValidCount == 0)
    {
        return OwnerHQ->GetActorLocation();
    }

    Centroid /= static_cast<float>(ValidCount);

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

    return Centroid - Forward * RearOffsetCm + Right * LateralOffsetCm;
}
