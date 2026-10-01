#include "StrategyParentExecutionComponent.h"

#include "../Command/StrategyCommandComponent.h"
#include "StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"

UStrategyParentExecutionComponent::UStrategyParentExecutionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyParentExecutionComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        SetComponentTickEnabled(false);
        return;
    }

    OwnerUnit->OrderComponent->OnOrderChanged.AddDynamic(
        this,
        &UStrategyParentExecutionComponent::HandleOrderChanged);
}

void UStrategyParentExecutionComponent::HandleOrderChanged(const FStrategyOrder& NewOrder)
{
    if (!OwnerUnit || !OwnerUnit->CommandComponent)
    {
        return;
    }

    if (!NewOrder.IsValidOrder() ||
        OwnerUnit->CommandComponent->CurrentSubordinates.Num() == 0)
    {
        SetComponentTickEnabled(false);
        return;
    }

    EvaluationAccumulator = 0.0f;
    SetComponentTickEnabled(true);
}

void UStrategyParentExecutionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit || !OwnerUnit->OrderComponent || !OwnerUnit->CommandComponent)
    {
        SetComponentTickEnabled(false);
        return;
    }

    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator < EvaluationIntervalSeconds)
    {
        return;
    }

    EvaluationAccumulator = 0.0f;

    if (HasExecutingSubordinates())
    {
        if (OwnerUnit->OrderComponent->GetExecutionState() != EStrategyOrderExecutionState::Executing)
        {
            OwnerUnit->OrderComponent->BeginExecution();
        }
        return;
    }

    if (OwnerUnit->OrderComponent->IsPhysicallyExecuting())
    {
        OwnerUnit->OrderComponent->CompleteExecution();
    }

    SetComponentTickEnabled(false);
}

bool UStrategyParentExecutionComponent::HasExecutingSubordinates() const
{
    if (!OwnerUnit || !OwnerUnit->CommandComponent)
    {
        return false;
    }

    for (AStrategyUnit* Subordinate : OwnerUnit->CommandComponent->CurrentSubordinates)
    {
        if (!IsValid(Subordinate) || !Subordinate->OrderComponent)
        {
            continue;
        }

        if (Subordinate->OrderComponent->IsPhysicallyExecuting())
        {
            return true;
        }
    }

    return false;
}
