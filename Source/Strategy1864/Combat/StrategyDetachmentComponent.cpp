#include "StrategyDetachmentComponent.h"
#include "../Units/StrategyUnit.h"
#include "StrategyCombatComponent.h"

UStrategyDetachmentComponent::UStrategyDetachmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyDetachmentComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

FName UStrategyDetachmentComponent::CreateDetachment(
    EStrategyDetachmentType Type,
    int32 RequestedStrength,
    int32 RequestedAmmoRounds,
    const FVector& TaskAnchor)
{
    if (!OwnerUnit ||
        Type == EStrategyDetachmentType::None ||
        RequestedStrength <= 0 ||
        GetActiveDetachmentCount() >= FMath::Max(1, MaximumConcurrentDetachments))
    {
        return NAME_None;
    }

    ReconcileInvalidDetachments();

    const int32 MaxDetached =
        FMath::Max(
            1,
            FMath::FloorToInt(
                static_cast<float>(OwnerUnit->CurrentStrength) *
                FMath::Clamp(MaximumDetachedFraction, 0.05f, 0.90f)));

    const int32 Capacity =
        FMath::Max(0, MaxDetached - GetDetachedStrength());

    const int32 Strength =
        FMath::Min3(
            RequestedStrength,
            Capacity,
            FMath::Max(0, GetAvailableParentStrength() - 1));

    if (Strength <= 0)
    {
        return NAME_None;
    }

    int32 Ammo = FMath::Max(0, RequestedAmmoRounds);
    if (OwnerUnit->CombatComponent)
    {
        Ammo = FMath::Min(Ammo, OwnerUnit->CombatComponent->AmmunitionRounds);
        OwnerUnit->CombatComponent->AmmunitionRounds -= Ammo;
        OwnerUnit->CombatComponent->bOutOfAmmo =
            OwnerUnit->CombatComponent->AmmunitionRounds <= 0;
    }

    FStrategyDetachmentRecord Record;
    Record.DetachmentId = FName(
        *FString::Printf(
            TEXT("%s-DET-%03d"),
            *OwnerUnit->StableUnitId.ToString(),
            ++SerialCounter));
    Record.Type = Type;
    Record.OrganicParentId = OwnerUnit->StableUnitId;
    Record.AssignedStrength = Strength;
    Record.CurrentStrength = Strength;
    Record.AssignedAmmoRounds = Ammo;
    Record.RemainingAmmoRounds = Ammo;
    Record.TaskAnchor = TaskAnchor;
    Record.Cohesion = OwnerUnit->Cohesion;
    Record.bActive = true;

    Detachments.Add(Record);
    return Record.DetachmentId;
}

bool UStrategyDetachmentComponent::RecallDetachment(FName DetachmentId)
{
    FStrategyDetachmentRecord* Record = FindMutable(DetachmentId);
    if (!Record || !Record->bActive)
    {
        return false;
    }

    if (OwnerUnit && OwnerUnit->CombatComponent && Record->RemainingAmmoRounds > 0)
    {
        OwnerUnit->CombatComponent->ResupplyAmmunition(Record->RemainingAmmoRounds);
    }

    if (OwnerUnit)
    {
        OwnerUnit->Cohesion =
            FMath::Clamp(
                FMath::Min(OwnerUnit->Cohesion, Record->Cohesion) - 0.5f,
                0.0f,
                100.0f);
    }

    Record->RemainingAmmoRounds = 0;
    Record->CurrentStrength = 0;
    Record->bActive = false;
    return true;
}

int32 UStrategyDetachmentComponent::ApplyDetachmentLoss(
    FName DetachmentId,
    int32 RequestedLoss)
{
    FStrategyDetachmentRecord* Record = FindMutable(DetachmentId);
    if (!OwnerUnit || !Record || !Record->bActive || RequestedLoss <= 0)
    {
        return 0;
    }

    const int32 Applied = FMath::Min(RequestedLoss, Record->CurrentStrength);
    Record->CurrentStrength -= Applied;
    OwnerUnit->ApplyStrengthLoss(Applied);

    if (Record->CurrentStrength <= 0)
    {
        Record->CurrentStrength = 0;
        Record->bActive = false;
    }

    return Applied;
}

int32 UStrategyDetachmentComponent::ConsumeDetachmentAmmo(
    FName DetachmentId,
    int32 RequestedRounds)
{
    FStrategyDetachmentRecord* Record = FindMutable(DetachmentId);
    if (!Record || !Record->bActive || RequestedRounds <= 0)
    {
        return 0;
    }

    const int32 Consumed = FMath::Min(RequestedRounds, Record->RemainingAmmoRounds);
    Record->RemainingAmmoRounds -= Consumed;
    return Consumed;
}

bool UStrategyDetachmentComponent::SetDetachmentAnchor(
    FName DetachmentId,
    const FVector& NewAnchor)
{
    FStrategyDetachmentRecord* Record = FindMutable(DetachmentId);
    if (!Record || !Record->bActive)
    {
        return false;
    }

    Record->TaskAnchor = NewAnchor;
    return true;
}

int32 UStrategyDetachmentComponent::GetDetachedStrength() const
{
    int32 Total = 0;
    for (const FStrategyDetachmentRecord& Record : Detachments)
    {
        if (Record.bActive)
        {
            Total += FMath::Max(0, Record.CurrentStrength);
        }
    }
    return Total;
}

int32 UStrategyDetachmentComponent::GetAvailableParentStrength() const
{
    return OwnerUnit
        ? FMath::Max(0, OwnerUnit->CurrentStrength - GetDetachedStrength())
        : 0;
}

int32 UStrategyDetachmentComponent::GetActiveDetachmentCount() const
{
    int32 Count = 0;
    for (const FStrategyDetachmentRecord& Record : Detachments)
    {
        Count += Record.bActive ? 1 : 0;
    }
    return Count;
}

bool UStrategyDetachmentComponent::HasActiveType(
    EStrategyDetachmentType Type) const
{
    for (const FStrategyDetachmentRecord& Record : Detachments)
    {
        if (Record.bActive && Record.Type == Type)
        {
            return true;
        }
    }
    return false;
}

void UStrategyDetachmentComponent::ReconcileInvalidDetachments()
{
    if (!OwnerUnit)
    {
        return;
    }

    int32 RemainingBudget = FMath::Max(0, OwnerUnit->CurrentStrength - 1);

    for (FStrategyDetachmentRecord& Record : Detachments)
    {
        if (!Record.bActive)
        {
            continue;
        }

        Record.CurrentStrength =
            FMath::Clamp(Record.CurrentStrength, 0, RemainingBudget);

        RemainingBudget -= Record.CurrentStrength;

        if (Record.CurrentStrength <= 0)
        {
            Record.bActive = false;
            Record.RemainingAmmoRounds = 0;
        }
    }
}

FStrategyDetachmentRecord* UStrategyDetachmentComponent::FindMutable(
    FName DetachmentId)
{
    return Detachments.FindByPredicate(
        [DetachmentId](const FStrategyDetachmentRecord& Record)
        {
            return Record.DetachmentId == DetachmentId;
        });
}

const FStrategyDetachmentRecord* UStrategyDetachmentComponent::FindConst(
    FName DetachmentId) const
{
    return Detachments.FindByPredicate(
        [DetachmentId](const FStrategyDetachmentRecord& Record)
        {
            return Record.DetachmentId == DetachmentId;
        });
}
