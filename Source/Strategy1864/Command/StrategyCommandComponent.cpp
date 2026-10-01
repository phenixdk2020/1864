#include "StrategyCommandComponent.h"

#include "../Units/StrategyUnit.h"

UStrategyCommandComponent::UStrategyCommandComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyCommandComponent::SetOrganicParent(AStrategyUnit* NewParent)
{
    AStrategyUnit* OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (!OwnerUnit ||
        NewParent == OwnerUnit ||
        OrganicParent == NewParent ||
        WouldCreateCommandCycle(NewParent))
    {
        return;
    }

    AStrategyUnit* OldOrganicParent = OrganicParent;
    const bool bFollowingOrganic =
        !CurrentCommandParent ||
        CurrentCommandParent == OldOrganicParent;

    if (OldOrganicParent && OldOrganicParent->CommandComponent)
    {
        OldOrganicParent->CommandComponent->RemoveOrganicSubordinate(OwnerUnit);
    }

    OrganicParent = NewParent;

    if (OrganicParent && OrganicParent->CommandComponent)
    {
        OrganicParent->CommandComponent->AddOrganicSubordinate(OwnerUnit);
    }

    if (bFollowingOrganic)
    {
        SetCurrentCommandParent(OrganicParent);
    }
}

void UStrategyCommandComponent::SetCurrentCommandParent(AStrategyUnit* NewParent)
{
    AStrategyUnit* OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (!OwnerUnit ||
        NewParent == OwnerUnit ||
        CurrentCommandParent == NewParent ||
        WouldCreateCommandCycle(NewParent))
    {
        return;
    }

    AStrategyUnit* OldParent = CurrentCommandParent;
    RemoveFromParentCurrentList(OldParent, OwnerUnit);

    CurrentCommandParent = NewParent;
    AddToParentCurrentList(CurrentCommandParent, OwnerUnit);

    OnCurrentCommandParentChanged.Broadcast(OldParent, CurrentCommandParent);
}

void UStrategyCommandComponent::RestoreOrganicCommandParent()
{
    SetCurrentCommandParent(OrganicParent);
}

void UStrategyCommandComponent::AddOrganicSubordinate(AStrategyUnit* Unit)
{
    if (IsValid(Unit) && Unit != GetOwner())
    {
        OrganicSubordinates.AddUnique(Unit);
    }
}

void UStrategyCommandComponent::RemoveOrganicSubordinate(AStrategyUnit* Unit)
{
    OrganicSubordinates.Remove(Unit);
}

void UStrategyCommandComponent::AddCurrentSubordinate(AStrategyUnit* Unit)
{
    if (IsValid(Unit) && Unit != GetOwner())
    {
        CurrentSubordinates.AddUnique(Unit);
    }
}

void UStrategyCommandComponent::RemoveCurrentSubordinate(AStrategyUnit* Unit)
{
    CurrentSubordinates.Remove(Unit);
}

void UStrategyCommandComponent::RemoveFromParentCurrentList(AStrategyUnit* Parent, AStrategyUnit* Child)
{
    if (IsValid(Parent) && Parent->CommandComponent)
    {
        Parent->CommandComponent->RemoveCurrentSubordinate(Child);
    }
}

void UStrategyCommandComponent::AddToParentCurrentList(AStrategyUnit* Parent, AStrategyUnit* Child)
{
    if (IsValid(Parent) && Parent->CommandComponent)
    {
        Parent->CommandComponent->AddCurrentSubordinate(Child);
    }
}


bool UStrategyCommandComponent::WouldCreateCommandCycle(AStrategyUnit* NewParent) const
{
    const AStrategyUnit* OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (!OwnerUnit || !IsValid(NewParent))
    {
        return false;
    }

    TSet<const AStrategyUnit*> Visited;
    const AStrategyUnit* Cursor = NewParent;

    while (IsValid(Cursor))
    {
        if (Cursor == OwnerUnit)
        {
            return true;
        }

        if (Visited.Contains(Cursor))
        {
            return true;
        }

        Visited.Add(Cursor);

        if (!Cursor->CommandComponent)
        {
            break;
        }

        Cursor = Cursor->CommandComponent->CurrentCommandParent;
    }

    return false;
}
