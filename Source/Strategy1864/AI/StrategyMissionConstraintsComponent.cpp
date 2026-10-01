#include "StrategyMissionConstraintsComponent.h"

#include "../Combat/StrategyFireDisciplineComponent.h"
#include "../Units/StrategyUnit.h"

UStrategyMissionConstraintsComponent::UStrategyMissionConstraintsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyMissionConstraintsComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategyMissionConstraintsComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (OwnerUnit && OwnerUnit->FireDisciplineComponent)
    {
        OwnerUnit->FireDisciplineComponent->SetConserveAmmunition(
            bConserveAmmunition);
    }
}

void UStrategyMissionConstraintsComponent::SetMissionArea(
    const FVector& Center,
    float RadiusCm)
{
    MissionAreaCenter = Center;
    MissionAreaRadiusCm = FMath::Max(100.0f, RadiusCm);
    bDoNotLeaveArea = true;
}

FVector UStrategyMissionConstraintsComponent::ClampGoalToMissionArea(
    const FVector& RequestedGoal) const
{
    if (!bDoNotLeaveArea)
    {
        return RequestedGoal;
    }

    FVector Offset = RequestedGoal - MissionAreaCenter;
    Offset.Z = 0.0f;

    const float Distance = Offset.Size();
    if (Distance <= MissionAreaRadiusCm)
    {
        return RequestedGoal;
    }

    const FVector Flat =
        MissionAreaCenter +
        Offset.GetSafeNormal() * MissionAreaRadiusCm;

    return FVector(Flat.X, Flat.Y, RequestedGoal.Z);
}

bool UStrategyMissionConstraintsComponent::CanPursueTarget(
    const AStrategyUnit* Target) const
{
    if (!IsValid(Target))
    {
        return false;
    }

    if (bDoNotPursue &&
        Target->UnitState == EStrategyUnitState::Routed)
    {
        return false;
    }

    return true;
}
