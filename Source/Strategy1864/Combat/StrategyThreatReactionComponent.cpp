#include "StrategyThreatReactionComponent.h"

#include "StrategyVisibilityComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Units/StrategyUnit.h"
#include "EngineUtils.h"

UStrategyThreatReactionComponent::UStrategyThreatReactionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyThreatReactionComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategyThreatReactionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit ||
        OwnerUnit->Echelon != EStrategyEchelon::Company ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed)
    {
        return;
    }

    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator < EvaluationIntervalSeconds)
    {
        return;
    }

    const float EvaluationDelta = EvaluationAccumulator;
    EvaluationAccumulator = 0.0f;

    AStrategyUnit* Threat = FindVisibleEnemyCavalry();
    if (Threat)
    {
        NoThreatSeconds = 0.0f;
        EnterSquare();
        return;
    }

    if (bRespondingToCavalry)
    {
        TryLeaveSquare(EvaluationDelta);
    }
}

AStrategyUnit* UStrategyThreatReactionComponent::FindVisibleEnemyCavalry() const
{
    if (!OwnerUnit || !OwnerUnit->VisibilityComponent || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* BestThreat = nullptr;
    float BestDistanceCm = TNumericLimits<float>::Max();

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;
        if (!IsValid(Candidate) ||
            Candidate == OwnerUnit ||
            Candidate->Echelon != EStrategyEchelon::Cavalry ||
            !Candidate->IsCombatEffective() ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerUnit->Side)
        {
            continue;
        }

        const float DistanceCm = FVector::Dist2D(
            OwnerUnit->GetActorLocation(),
            Candidate->GetActorLocation());

        if (DistanceCm > CavalryThreatDistanceCm ||
            DistanceCm >= BestDistanceCm ||
            !OwnerUnit->VisibilityComponent->HasLineOfSightTo(Candidate))
        {
            continue;
        }

        BestDistanceCm = DistanceCm;
        BestThreat = Candidate;
    }

    return BestThreat;
}

void UStrategyThreatReactionComponent::EnterSquare()
{
    if (!OwnerUnit || !OwnerUnit->FormationComponent)
    {
        return;
    }

    if (!bRespondingToCavalry)
    {
        PreThreatFormation = OwnerUnit->FormationComponent->CurrentFormation;
    }

    bRespondingToCavalry = true;

    if (OwnerUnit->FormationComponent->CurrentFormation != EStrategyFormationType::Square)
    {
        OwnerUnit->FormationComponent->SetFormation(EStrategyFormationType::Square);
        OwnerUnit->SetUnitState(EStrategyUnitState::Reforming);
    }
}

void UStrategyThreatReactionComponent::TryLeaveSquare(float DeltaTime)
{
    if (!OwnerUnit || !OwnerUnit->FormationComponent)
    {
        return;
    }

    NoThreatSeconds += DeltaTime;
    if (NoThreatSeconds < SquareReleaseDelaySeconds)
    {
        return;
    }

    if (OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::Square)
    {
        OwnerUnit->FormationComponent->SetFormation(PreThreatFormation);
    }

    bRespondingToCavalry = false;
    NoThreatSeconds = 0.0f;

    if (OwnerUnit->UnitState == EStrategyUnitState::Reforming)
    {
        OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
    }
}
