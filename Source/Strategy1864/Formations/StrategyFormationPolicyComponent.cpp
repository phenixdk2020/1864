#include "StrategyFormationPolicyComponent.h"

#include "StrategyFormationComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"
#include "EngineUtils.h"

UStrategyFormationPolicyComponent::UStrategyFormationPolicyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyFormationPolicyComponent::BeginPlay()
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
        &UStrategyFormationPolicyComponent::HandleOrderChanged);
}

void UStrategyFormationPolicyComponent::HandleOrderChanged(const FStrategyOrder& NewOrder)
{
    if (!OwnerUnit || !OwnerUnit->FormationComponent)
    {
        return;
    }

    if (!NewOrder.IsValidOrder() || !IsMovementMission(NewOrder.Type))
    {
        SetComponentTickEnabled(false);
        return;
    }

    ApplyInitialMovementFormation(NewOrder);
    ThreatScanAccumulator = 0.0f;
    SetComponentTickEnabled(true);
}

void UStrategyFormationPolicyComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit || !OwnerUnit->OrderComponent || !OwnerUnit->MovementExecutor)
    {
        SetComponentTickEnabled(false);
        return;
    }

    if (!OwnerUnit->OrderComponent->IsPhysicallyExecuting() ||
        !OwnerUnit->MovementExecutor->HasMovementGoal())
    {
        SetComponentTickEnabled(false);
        return;
    }

    ThreatScanAccumulator += DeltaTime;
    if (ThreatScanAccumulator < ThreatScanIntervalSeconds)
    {
        return;
    }

    ThreatScanAccumulator = 0.0f;
    EvaluateEarlyDeployment();
}

bool UStrategyFormationPolicyComponent::IsMovementMission(EStrategyOrderType Type) const
{
    switch (Type)
    {
        case EStrategyOrderType::Move:
        case EStrategyOrderType::AttackHere:
        case EStrategyOrderType::DefendHere:
        case EStrategyOrderType::Advance:
        case EStrategyOrderType::Withdraw:
        case EStrategyOrderType::Assemble:
            return true;

        default:
            return false;
    }
}

void UStrategyFormationPolicyComponent::ApplyInitialMovementFormation(const FStrategyOrder& Order)
{
    if (!OwnerUnit || !OwnerUnit->FormationComponent)
    {
        return;
    }

    const float DistanceCm = FVector::Dist2D(
        OwnerUnit->GetActorLocation(),
        Order.TargetLocation);

    if (DistanceCm >= LongMoveColumnThresholdCm)
    {
        OwnerUnit->FormationComponent->SetFormation(
            EStrategyFormationType::MarchColumn);
    }
    else
    {
        OwnerUnit->FormationComponent->SetFormation(
            EStrategyFormationType::Line);
    }
}

void UStrategyFormationPolicyComponent::EvaluateEarlyDeployment()
{
    if (!OwnerUnit ||
        !OwnerUnit->FormationComponent ||
        OwnerUnit->FormationComponent->CurrentFormation != EStrategyFormationType::MarchColumn)
    {
        return;
    }

    float EnemyDistanceCm = TNumericLimits<float>::Max();
    AStrategyUnit* NearestEnemy = FindNearestEnemy(EnemyDistanceCm);
    if (!NearestEnemy)
    {
        return;
    }

    const float DeployDistanceCm =
        FMath::Max(
            OwnerUnit->MaximumFireRangeCm,
            NearestEnemy->MaximumFireRangeCm) +
        DeploySafetyBufferCm;

    if (EnemyDistanceCm <= DeployDistanceCm)
    {
        OwnerUnit->FormationComponent->SetFormation(
            EStrategyFormationType::Line);

        OwnerUnit->SetUnitState(EStrategyUnitState::Reforming);
    }
}

AStrategyUnit* UStrategyFormationPolicyComponent::FindNearestEnemy(float& OutDistanceCm) const
{
    OutDistanceCm = TNumericLimits<float>::Max();

    if (!OwnerUnit || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* BestUnit = nullptr;

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;
        if (!IsValid(Candidate) ||
            Candidate == OwnerUnit ||
            !Candidate->IsCombatEffective() ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerUnit->Side)
        {
            continue;
        }

        const float DistanceCm = FVector::Dist2D(
            OwnerUnit->GetActorLocation(),
            Candidate->GetActorLocation());

        if (DistanceCm < OutDistanceCm)
        {
            OutDistanceCm = DistanceCm;
            BestUnit = Candidate;
        }
    }

    return BestUnit;
}
