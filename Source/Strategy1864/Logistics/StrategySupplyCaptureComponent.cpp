#include "StrategySupplyCaptureComponent.h"

#include "StrategySupplyWagonUnit.h"
#include "StrategySupplyComponent.h"
#include "../Units/StrategyUnit.h"
#include "EngineUtils.h"

UStrategySupplyCaptureComponent::UStrategySupplyCaptureComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategySupplyCaptureComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerWagon = Cast<AStrategySupplyWagonUnit>(GetOwner());
}

void UStrategySupplyCaptureComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerWagon ||
        OwnerWagon->OwnershipState !=
            EStrategySupplyOwnershipState::Abandoned)
    {
        CaptureProgressSeconds = 0.0f;
        ActiveCaptor = nullptr;
        return;
    }

    if (HasFriendlyProtection())
    {
        CaptureProgressSeconds = 0.0f;
        ActiveCaptor = nullptr;
        return;
    }

    AStrategyUnit* Captor = FindCapturingEnemy();
    if (!IsValid(Captor))
    {
        CaptureProgressSeconds = 0.0f;
        ActiveCaptor = nullptr;
        return;
    }

    if (ActiveCaptor != Captor)
    {
        ActiveCaptor = Captor;
        CaptureProgressSeconds = 0.0f;
    }

    CaptureProgressSeconds += DeltaTime;

    if (CaptureProgressSeconds >= CaptureHoldSeconds)
    {
        CompleteCapture(Captor);
    }
}

AStrategyUnit* UStrategySupplyCaptureComponent::FindCapturingEnemy() const
{
    if (!OwnerWagon || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* Best = nullptr;
    float BestDistance = CaptureRadiusCm;

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate == OwnerWagon ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerWagon->Side ||
            !Candidate->IsCombatEffective())
        {
            continue;
        }

        const float Distance =
            FVector::Dist2D(
                OwnerWagon->GetActorLocation(),
                Candidate->GetActorLocation());

        if (Distance <= BestDistance)
        {
            BestDistance = Distance;
            Best = Candidate;
        }
    }

    return Best;
}

bool UStrategySupplyCaptureComponent::HasFriendlyProtection() const
{
    if (!OwnerWagon || !GetWorld())
    {
        return false;
    }

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        const AStrategyUnit* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate == OwnerWagon ||
            Candidate->Side != OwnerWagon->Side ||
            !Candidate->IsCombatEffective())
        {
            continue;
        }

        if (FVector::Dist2D(
                OwnerWagon->GetActorLocation(),
                Candidate->GetActorLocation()) <=
            CaptureRadiusCm * 1.5f)
        {
            return true;
        }
    }

    return false;
}

void UStrategySupplyCaptureComponent::CompleteCapture(
    AStrategyUnit* Captor)
{
    if (!OwnerWagon || !IsValid(Captor))
    {
        return;
    }

    OwnerWagon->Side = Captor->Side;
    OwnerWagon->OwnershipState =
        EStrategySupplyOwnershipState::Captured;

    OwnerWagon->bPlayerControllable =
        Captor->Side == EStrategySide::Denmark ||
        Captor->Side == EStrategySide::Allied;

    // Captured cargo may be used in place, but the wagon remains immobile
    // until a later driver/horse reassignment system is implemented.
    OwnerWagon->SetUnitState(EStrategyUnitState::Ready);

    if (OwnerWagon->SupplyComponent)
    {
        OwnerWagon->SupplyComponent->bActsAsSupplySource = true;
    }

    OwnerWagon->RefreshDebugLabel();

    CaptureProgressSeconds = 0.0f;
    ActiveCaptor = nullptr;
}
