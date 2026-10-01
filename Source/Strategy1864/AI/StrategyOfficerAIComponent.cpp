#include "StrategyOfficerAIComponent.h"

#include "../Command/StrategyCommandComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"
#include "StrategyCommandDelayComponent.h"
#include "StrategyAITelemetryComponent.h"

UStrategyOfficerAIComponent::UStrategyOfficerAIComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyOfficerAIComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategyOfficerAIComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit || !OwnerUnit->bOfficerAIEnabled)
    {
        return;
    }

    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator < EvaluationIntervalSeconds)
    {
        return;
    }

    EvaluationAccumulator = 0.0f;
    EvaluateInheritedMission();
}

void UStrategyOfficerAIComponent::SetAIEnabled(
    bool bEnabled,
    bool bCascadeToSubordinates)
{
    if (!OwnerUnit)
    {
        OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    }

    if (!OwnerUnit)
    {
        return;
    }

    OwnerUnit->bOfficerAIEnabled = bEnabled;

    if (!bCascadeToSubordinates || !OwnerUnit->CommandComponent)
    {
        return;
    }

    for (AStrategyUnit* Subordinate : OwnerUnit->CommandComponent->CurrentSubordinates)
    {
        if (!IsValid(Subordinate))
        {
            continue;
        }

        Subordinate->bOfficerAIEnabled = bEnabled;

        if (Subordinate->OfficerAIComponent)
        {
            Subordinate->OfficerAIComponent->SetAIEnabled(bEnabled, true);
        }
    }
}

bool UStrategyOfficerAIComponent::IsAIEnabled() const
{
    return OwnerUnit && OwnerUnit->bOfficerAIEnabled;
}

void UStrategyOfficerAIComponent::EvaluateInheritedMission()
{
    if (!OwnerUnit ||
        !OwnerUnit->CommandComponent ||
        !OwnerUnit->OrderComponent)
    {
        return;
    }

    AStrategyUnit* Parent = OwnerUnit->CommandComponent->CurrentCommandParent;
    if (!IsValid(Parent) || !Parent->OrderComponent)
    {
        return;
    }

    const FStrategyOrder ParentOrder = Parent->OrderComponent->GetCurrentOrder();
    if (!ParentOrder.IsValidOrder() ||
        ParentOrder.OrderSerial == LastInheritedParentOrderSerial ||
        !CanAcceptInheritedMission())
    {
        return;
    }

    // Attack/Defend are normally assigned by the higher formation planner.
    // Do not create a second slot writer for those mission types.
    if (ParentOrder.Type == EStrategyOrderType::AttackHere ||
        ParentOrder.Type == EStrategyOrderType::DefendHere)
    {
        LastInheritedParentOrderSerial = ParentOrder.OrderSerial;
        return;
    }

    FStrategyOrder InheritedOrder = ParentOrder;
    InheritedOrder.Authority = EStrategyOrderAuthority::InheritedAI;
    InheritedOrder.OrderSerial = 0;

    const float Delay =
        OwnerUnit->CommandDelayComponent
        ? OwnerUnit->CommandDelayComponent->CalculateDelayFromCurrentParent()
        : 0.0f;

    if (OwnerUnit->OrderComponent->QueueDelayedOrder(InheritedOrder, Delay))
    {
        LastInheritedParentOrderSerial = ParentOrder.OrderSerial;

        if (OwnerUnit->AITelemetryComponent)
        {
            OwnerUnit->AITelemetryComponent->SetDecision(
                TEXT("Inherited mission"),
                FString::Printf(
                    TEXT("Parent order queued with %.2fs command delay"),
                    Delay));
        }
    }
}

bool UStrategyOfficerAIComponent::CanAcceptInheritedMission() const
{
    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        return false;
    }

    const FStrategyOrder CurrentOrder = OwnerUnit->OrderComponent->GetCurrentOrder();

    if (!CurrentOrder.IsValidOrder())
    {
        return true;
    }

    if (CurrentOrder.Authority == EStrategyOrderAuthority::DirectPlayer)
    {
        return false;
    }

    return !OwnerUnit->OrderComponent->IsPhysicallyExecuting() ||
        CurrentOrder.Authority == EStrategyOrderAuthority::InheritedAI;
}
