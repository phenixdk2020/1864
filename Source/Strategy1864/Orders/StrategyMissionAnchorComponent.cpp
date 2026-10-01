#include "StrategyMissionAnchorComponent.h"

#include "StrategyOrderComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Units/StrategyUnit.h"

UStrategyMissionAnchorComponent::UStrategyMissionAnchorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyMissionAnchorComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());

    if (OwnerUnit && OwnerUnit->OrderComponent)
    {
        OwnerUnit->OrderComponent->OnOrderChanged.AddDynamic(
            this,
            &UStrategyMissionAnchorComponent::HandleOrderChanged);
    }
}

void UStrategyMissionAnchorComponent::HandleOrderChanged(const FStrategyOrder& NewOrder)
{
    if (NewOrder.Type == EStrategyOrderType::AttackHere ||
        NewOrder.Type == EStrategyOrderType::DefendHere)
    {
        MissionAnchor = NewOrder.TargetLocation;
        MissionFacingYaw = NewOrder.FacingYaw;
        bHasMissionFacing = NewOrder.bHasFacing;
        AnchoredMissionType = NewOrder.Type;
        return;
    }

    AnchoredMissionType = EStrategyOrderType::None;
}

void UStrategyMissionAnchorComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit ||
        AnchoredMissionType != EStrategyOrderType::DefendHere ||
        !OwnerUnit->OrderComponent ||
        !OwnerUnit->OrderComponent->HasStandingIntent() ||
        OwnerUnit->OrderComponent->IsPhysicallyExecuting() ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed ||
        (OwnerUnit->CommandComponent &&
         OwnerUnit->CommandComponent->CurrentSubordinates.Num() > 0))
    {
        return;
    }

    const float DistanceCm = FVector::Dist2D(
        OwnerUnit->GetActorLocation(),
        MissionAnchor);

    const float FacingError =
        bHasMissionFacing
        ? FMath::Abs(FMath::FindDeltaAngleDegrees(
            OwnerUnit->GetActorRotation().Yaw,
            MissionFacingYaw))
        : 0.0f;

    if (DistanceCm <= DefendReassertToleranceCm && FacingError <= 2.0f)
    {
        return;
    }

    // Same standing mission is physically reasserted without changing authority.
    OwnerUnit->OrderComponent->BeginExecution();

    if (OwnerUnit->MovementExecutor)
    {
        OwnerUnit->MovementExecutor->RestartCurrentOrderExecution();
    }
}
