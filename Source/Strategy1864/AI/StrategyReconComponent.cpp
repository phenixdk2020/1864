#include "StrategyReconComponent.h"

#include "../Combat/StrategyContactComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"

UStrategyReconComponent::UStrategyReconComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyReconComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());

    if (OwnerUnit && OwnerUnit->OrderComponent)
    {
        OwnerUnit->OrderComponent->OnOrderChanged.AddDynamic(
            this,
            &UStrategyReconComponent::HandleOrderChanged);
    }
}

void UStrategyReconComponent::HandleOrderChanged(
    const FStrategyOrder& NewOrder)
{
    if (NewOrder.Type == EStrategyOrderType::ScoutHere)
    {
        ReconState = EStrategyReconState::Seek;
        StateSeconds = 0.0f;
        return;
    }

    ReconState = EStrategyReconState::Idle;
    StateSeconds = 0.0f;
}

void UStrategyReconComponent::OnScoutDestinationReached()
{
    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        return;
    }

    const FStrategyOrder Order =
        OwnerUnit->OrderComponent->GetCurrentOrder();

    if (Order.Type != EStrategyOrderType::ScoutHere)
    {
        return;
    }

    ReconState = EStrategyReconState::Recon;
    StateSeconds = 0.0f;
    OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
    OwnerUnit->OrderComponent->BeginExecution();
}

bool UStrategyReconComponent::IsReconActive() const
{
    return ReconState != EStrategyReconState::Idle;
}

void UStrategyReconComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit ||
        !OwnerUnit->OrderComponent ||
        !IsReconActive())
    {
        return;
    }

    const FStrategyOrder Order =
        OwnerUnit->OrderComponent->GetCurrentOrder();

    if (Order.Type != EStrategyOrderType::ScoutHere)
    {
        ReconState = EStrategyReconState::Idle;
        return;
    }

    if (ReconState == EStrategyReconState::Seek)
    {
        return;
    }

    StateSeconds += DeltaTime;

    if (ReconState == EStrategyReconState::Recon)
    {
        const TArray<FStrategyContactRecord> Contacts =
            OwnerUnit->ContactComponent
            ? OwnerUnit->ContactComponent->GetKnownContacts()
            : TArray<FStrategyContactRecord>();

        const bool bCurrentContact =
            Contacts.ContainsByPredicate(
                [](const FStrategyContactRecord& Contact)
                {
                    return Contact.bCurrentlyVisible;
                });

        const bool bAnyKnown = Contacts.Num() > 0;

        if (bCurrentContact)
        {
            ReconState = EStrategyReconState::Contact;
            StateSeconds = 0.0f;
            return;
        }

        if (StateSeconds >= ReconDwellSeconds)
        {
            ReconState =
                bAnyKnown
                ? EStrategyReconState::LastKnown
                : EStrategyReconState::Report;
            StateSeconds = 0.0f;
            return;
        }
    }
    else if (ReconState == EStrategyReconState::Contact)
    {
        ReconState = EStrategyReconState::Screen;
        StateSeconds = 0.0f;
    }
    else if (ReconState == EStrategyReconState::Screen)
    {
        if (StateSeconds >= ContactScreenSeconds)
        {
            ReconState = EStrategyReconState::Report;
            StateSeconds = 0.0f;
        }
    }
    else if (ReconState == EStrategyReconState::LastKnown ||
             ReconState == EStrategyReconState::Report)
    {
        CompleteRecon();
    }
}

void UStrategyReconComponent::CompleteRecon()
{
    if (OwnerUnit && OwnerUnit->OrderComponent)
    {
        OwnerUnit->OrderComponent->CompleteExecution();
    }

    ReconState = EStrategyReconState::Idle;
    StateSeconds = 0.0f;
}
