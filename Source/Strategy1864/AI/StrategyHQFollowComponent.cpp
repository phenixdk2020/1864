#include "StrategyHQFollowComponent.h"

#include "../Command/StrategyCommandComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyHQUnit.h"
#include "../Units/StrategyUnit.h"

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
            RearOffsetCm = 6500.0f;
            LateralOffsetCm = 0.0f;
            FollowSpeedCmPerSecond = 650.0f;
            break;

        case EStrategyHQLevel::Regiment:
            RearOffsetCm = 9000.0f;
            LateralOffsetCm = 0.0f;
            FollowSpeedCmPerSecond = 630.0f;
            break;

        case EStrategyHQLevel::Brigade:
            RearOffsetCm = 12000.0f;
            LateralOffsetCm = 6500.0f;
            FollowSpeedCmPerSecond = 620.0f;
            break;

        case EStrategyHQLevel::Division:
            RearOffsetCm = 14500.0f;
            LateralOffsetCm = -7500.0f;
            FollowSpeedCmPerSecond = 590.0f;
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

    const FVector Desired = CalculateDesiredHQPosition();
    const FVector Current = OwnerHQ->GetActorLocation();
    FVector Delta = Desired - Current;
    Delta.Z = 0.0f;

    const float Distance = Delta.Size();
    if (Distance <= SettleToleranceCm)
    {
        return;
    }

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
