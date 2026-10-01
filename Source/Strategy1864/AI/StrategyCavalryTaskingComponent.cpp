#include "StrategyCavalryTaskingComponent.h"

#include "../Command/StrategyCommandComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyHQUnit.h"
#include "../Units/StrategyUnit.h"

UStrategyCavalryTaskingComponent::UStrategyCavalryTaskingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyCavalryTaskingComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        return;
    }

    OwnerUnit->OrderComponent->OnOrderChanged.AddDynamic(
        this,
        &UStrategyCavalryTaskingComponent::HandleOwnerOrderChanged);

    OwnerUnit->OrderComponent->OnExecutionStateChanged.AddDynamic(
        this,
        &UStrategyCavalryTaskingComponent::HandleOwnerExecutionStateChanged);
}

void UStrategyCavalryTaskingComponent::HandleOwnerOrderChanged(
    const FStrategyOrder& NewOrder)
{
    if (NewOrder.Type == EStrategyOrderType::AttackHere)
    {
        ReleaseTemporaryAttachments(false);
        AssignForAttack(NewOrder);
        return;
    }

    if (NewOrder.Type == EStrategyOrderType::DefendHere)
    {
        ReleaseTemporaryAttachments(false);
        AssignDefensiveReserve(NewOrder);
        return;
    }

    if (ActiveAttachments.Num() > 0)
    {
        ReleaseTemporaryAttachments(true);
    }
}

void UStrategyCavalryTaskingComponent::HandleOwnerExecutionStateChanged(
    EStrategyOrderExecutionState OldState,
    EStrategyOrderExecutionState NewState)
{
    if (ActiveAttachments.Num() == 0)
    {
        return;
    }

    if (NewState == EStrategyOrderExecutionState::Completed ||
        NewState == EStrategyOrderExecutionState::Failed ||
        NewState == EStrategyOrderExecutionState::Superseded)
    {
        ReleaseTemporaryAttachments(true);
    }
}

void UStrategyCavalryTaskingComponent::AssignForAttack(
    const FStrategyOrder& AttackOrder)
{
    if (!OwnerUnit)
    {
        return;
    }

    TArray<ACavalryUnit*> CavalryUnits = GetAvailableDirectCavalry();
    TArray<AStrategyUnit*> Majors = GetCommandedMajorsRecursive();

    if (CavalryUnits.Num() == 0 || Majors.Num() == 0)
    {
        return;
    }

    CavalryUnits.Sort([](const ACavalryUnit& A, const ACavalryUnit& B)
    {
        return A.StableUnitId.LexicalLess(B.StableUnitId);
    });

    Majors.Sort([](const AStrategyUnit& A, const AStrategyUnit& B)
    {
        return A.StableUnitId.LexicalLess(B.StableUnitId);
    });

    if (CavalryUnits.Num() >= 2 && Majors.Num() >= 2)
    {
        const float DirectCost =
            FVector::DistSquared2D(CavalryUnits[0]->GetActorLocation(), Majors[0]->GetActorLocation()) +
            FVector::DistSquared2D(CavalryUnits[1]->GetActorLocation(), Majors[1]->GetActorLocation());

        const float CrossCost =
            FVector::DistSquared2D(CavalryUnits[0]->GetActorLocation(), Majors[1]->GetActorLocation()) +
            FVector::DistSquared2D(CavalryUnits[1]->GetActorLocation(), Majors[0]->GetActorLocation());

        if (CrossCost < DirectCost)
        {
            Swap(Majors[0], Majors[1]);
        }
    }

    const int32 PairCount = FMath::Min(CavalryUnits.Num(), Majors.Num());

    for (int32 Index = 0; Index < PairCount; ++Index)
    {
        const float LateralSign = (Index % 2 == 0) ? -1.0f : 1.0f;
        AttachCavalryToMajor(
            CavalryUnits[Index],
            Majors[Index],
            AttackOrder,
            LateralSign);
    }
}

void UStrategyCavalryTaskingComponent::AssignDefensiveReserve(
    const FStrategyOrder& DefendOrder)
{
    TArray<ACavalryUnit*> CavalryUnits = GetAvailableDirectCavalry();
    TArray<AStrategyUnit*> Majors = GetCommandedMajorsRecursive();

    if (CavalryUnits.Num() == 0 || Majors.Num() == 0)
    {
        return;
    }

    CavalryUnits.Sort([](const ACavalryUnit& A, const ACavalryUnit& B)
    {
        return A.StableUnitId.LexicalLess(B.StableUnitId);
    });

    Majors.Sort([](const AStrategyUnit& A, const AStrategyUnit& B)
    {
        return A.StableUnitId.LexicalLess(B.StableUnitId);
    });

    if (CavalryUnits.Num() >= 2 && Majors.Num() >= 2)
    {
        const float DirectCost =
            FVector::DistSquared2D(CavalryUnits[0]->GetActorLocation(), Majors[0]->GetActorLocation()) +
            FVector::DistSquared2D(CavalryUnits[1]->GetActorLocation(), Majors[1]->GetActorLocation());

        const float CrossCost =
            FVector::DistSquared2D(CavalryUnits[0]->GetActorLocation(), Majors[1]->GetActorLocation()) +
            FVector::DistSquared2D(CavalryUnits[1]->GetActorLocation(), Majors[0]->GetActorLocation());

        if (CrossCost < DirectCost)
        {
            Swap(Majors[0], Majors[1]);
        }
    }

    const int32 PairCount = FMath::Min(CavalryUnits.Num(), Majors.Num());

    for (int32 Index = 0; Index < PairCount; ++Index)
    {
        ACavalryUnit* Cavalry = CavalryUnits[Index];
        AStrategyUnit* Major = Majors[Index];

        if (!IsValid(Cavalry) ||
            !IsValid(Major) ||
            !Cavalry->OrderComponent)
        {
            continue;
        }

        const float FacingYaw =
            DefendOrder.bHasFacing
            ? DefendOrder.FacingYaw
            : Major->GetActorRotation().Yaw;

        const float LateralSign = (Index % 2 == 0) ? -1.0f : 1.0f;

        FStrategyOrder ReserveOrder;
        ReserveOrder.Type = EStrategyOrderType::Move;
        ReserveOrder.TargetLocation =
            CalculateReservePosition(Major, FacingYaw, LateralSign);
        ReserveOrder.FacingYaw = FacingYaw;
        ReserveOrder.bHasFacing = true;
        ReserveOrder.Authority = EStrategyOrderAuthority::OfficerAI;

        Cavalry->OrderComponent->SetOrder(ReserveOrder);
    }
}

TArray<ACavalryUnit*> UStrategyCavalryTaskingComponent::GetAvailableDirectCavalry() const
{
    TArray<ACavalryUnit*> Result;

    if (!OwnerUnit || !OwnerUnit->CommandComponent)
    {
        return Result;
    }

    for (AStrategyUnit* Subordinate : OwnerUnit->CommandComponent->CurrentSubordinates)
    {
        ACavalryUnit* Cavalry = Cast<ACavalryUnit>(Subordinate);
        if (!IsValid(Cavalry) || CavalryHasProtectedDirectOrder(Cavalry))
        {
            continue;
        }

        Result.Add(Cavalry);
    }

    return Result;
}

TArray<AStrategyUnit*> UStrategyCavalryTaskingComponent::GetCommandedMajorsRecursive() const
{
    TArray<AStrategyUnit*> Result;
    CollectMajorsRecursive(OwnerUnit, Result);
    return Result;
}

void UStrategyCavalryTaskingComponent::CollectMajorsRecursive(
    AStrategyUnit* Parent,
    TArray<AStrategyUnit*>& OutMajors) const
{
    if (!IsValid(Parent) || !Parent->CommandComponent)
    {
        return;
    }

    for (AStrategyUnit* Subordinate : Parent->CommandComponent->CurrentSubordinates)
    {
        if (!IsValid(Subordinate))
        {
            continue;
        }

        const AStrategyHQUnit* HQ = Cast<AStrategyHQUnit>(Subordinate);
        if (HQ && HQ->HQLevel == EStrategyHQLevel::Battalion)
        {
            OutMajors.AddUnique(Subordinate);
            continue;
        }

        CollectMajorsRecursive(Subordinate, OutMajors);
    }
}

bool UStrategyCavalryTaskingComponent::CavalryHasProtectedDirectOrder(
    const ACavalryUnit* Cavalry) const
{
    if (!IsValid(Cavalry) || !Cavalry->OrderComponent)
    {
        return true;
    }

    const FStrategyOrder CurrentOrder = Cavalry->OrderComponent->GetCurrentOrder();
    return CurrentOrder.IsValidOrder() &&
        CurrentOrder.Authority == EStrategyOrderAuthority::DirectPlayer &&
        Cavalry->OrderComponent->IsPhysicallyExecuting();
}

void UStrategyCavalryTaskingComponent::AttachCavalryToMajor(
    ACavalryUnit* Cavalry,
    AStrategyUnit* Major,
    const FStrategyOrder& ParentOrder,
    float LateralSign)
{
    if (!IsValid(Cavalry) ||
        !IsValid(Major) ||
        !Cavalry->CommandComponent ||
        !Cavalry->OrderComponent)
    {
        return;
    }

    FStrategyTemporaryCavalryAttachment Attachment;
    Attachment.Cavalry = Cavalry;
    Attachment.PreviousCommandParent =
        Cavalry->CommandComponent->CurrentCommandParent;
    Attachment.AssignedMajor = Major;

    Cavalry->CommandComponent->SetCurrentCommandParent(Major);
    ActiveAttachments.Add(Attachment);

    const float FacingYaw =
        ParentOrder.bHasFacing
        ? ParentOrder.FacingYaw
        : Major->GetActorRotation().Yaw;

    FStrategyOrder SupportOrder;
    SupportOrder.Type = EStrategyOrderType::Move;
    SupportOrder.TargetLocation =
        CalculateReservePosition(Major, FacingYaw, LateralSign);
    SupportOrder.FacingYaw = FacingYaw;
    SupportOrder.bHasFacing = true;
    SupportOrder.Authority = EStrategyOrderAuthority::OfficerAI;

    Cavalry->OrderComponent->SetOrder(SupportOrder);
}

void UStrategyCavalryTaskingComponent::ReleaseTemporaryAttachments(
    bool bIssueReserveMove)
{
    for (const FStrategyTemporaryCavalryAttachment& Attachment : ActiveAttachments)
    {
        ACavalryUnit* Cavalry = Attachment.Cavalry;
        if (!IsValid(Cavalry) || !Cavalry->CommandComponent)
        {
            continue;
        }

        AStrategyUnit* PreviousParent = Attachment.PreviousCommandParent;
        Cavalry->CommandComponent->SetCurrentCommandParent(PreviousParent);

        if (!bIssueReserveMove ||
            !IsValid(PreviousParent) ||
            !Cavalry->OrderComponent ||
            CavalryHasProtectedDirectOrder(Cavalry))
        {
            continue;
        }

        const float FacingYaw = PreviousParent->GetActorRotation().Yaw;

        FStrategyOrder ReserveOrder;
        ReserveOrder.Type = EStrategyOrderType::Move;
        ReserveOrder.TargetLocation =
            CalculateReservePosition(PreviousParent, FacingYaw, 1.0f);
        ReserveOrder.FacingYaw = FacingYaw;
        ReserveOrder.bHasFacing = true;
        ReserveOrder.Authority = EStrategyOrderAuthority::OfficerAI;

        Cavalry->OrderComponent->SetOrder(ReserveOrder);
    }

    ActiveAttachments.Reset();
}

FVector UStrategyCavalryTaskingComponent::CalculateReservePosition(
    const AStrategyUnit* Anchor,
    float FacingYaw,
    float LateralSign) const
{
    if (!IsValid(Anchor))
    {
        return FVector::ZeroVector;
    }

    const FRotator FacingRotation(0.0f, FacingYaw, 0.0f);
    const FVector Forward = FacingRotation.Vector();
    const FVector Right = FRotationMatrix(FacingRotation).GetScaledAxis(EAxis::Y);

    return Anchor->GetActorLocation()
        - Forward * ReserveRearOffsetCm
        + Right * ReserveLateralOffsetCm * LateralSign;
}
