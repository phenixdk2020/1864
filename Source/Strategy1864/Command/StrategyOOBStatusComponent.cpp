#include "StrategyOOBStatusComponent.h"

#include "StrategyCommandComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"

UStrategyOOBStatusComponent::UStrategyOOBStatusComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FStrategyOOBAggregateStatus UStrategyOOBStatusComponent::CalculateAggregateStatus() const
{
    FStrategyOOBAggregateStatus Result;
    TSet<const AStrategyUnit*> Visited;

    const AStrategyUnit* OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (OwnerUnit)
    {
        AccumulateRecursive(OwnerUnit, Visited, Result);
    }

    return Result;
}

void UStrategyOOBStatusComponent::AccumulateRecursive(
    const AStrategyUnit* Unit,
    TSet<const AStrategyUnit*>& Visited,
    FStrategyOOBAggregateStatus& InOutStatus) const
{
    if (!IsValid(Unit) || Visited.Contains(Unit))
    {
        return;
    }

    Visited.Add(Unit);
    ++InOutStatus.UnitCount;
    InOutStatus.InitialStrength += FMath::Max(0, Unit->InitialStrength);
    InOutStatus.CurrentStrength += FMath::Max(0, Unit->CurrentStrength);

    if (Unit->UnitState == EStrategyUnitState::Routed)
    {
        ++InOutStatus.RoutedCount;
    }

    if (Unit->UnitState == EStrategyUnitState::Destroyed)
    {
        ++InOutStatus.DestroyedCount;
    }

    if (Unit->OrderComponent && Unit->OrderComponent->IsPhysicallyExecuting())
    {
        ++InOutStatus.ExecutingCount;
    }

    if (!Unit->CommandComponent)
    {
        return;
    }

    for (const AStrategyUnit* Child : Unit->CommandComponent->CurrentSubordinates)
    {
        AccumulateRecursive(Child, Visited, InOutStatus);
    }
}
