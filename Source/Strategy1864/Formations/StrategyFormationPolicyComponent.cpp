#include "StrategyFormationPolicyComponent.h"

#include "StrategyFormationComponent.h"
#include "../Combat/StrategyThreatReactionComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Units/CavalryUnit.h"
#include "../Combat/StrategyFireControlComponent.h"
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
        // Halted or given another task while in column: it forms up where it stands.
        if (bMarching)
        {
            Deploy();
        }
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
        // At the goal: out of column into its formation again.
        if (bMarching)
        {
            Deploy();
        }
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

bool UStrategyFormationPolicyComponent::MarchesInColumn() const
{
    return OwnerUnit && OwnerUnit->FormationComponent &&
        (OwnerUnit->Echelon == EStrategyEchelon::Company || OwnerUnit->Echelon == EStrategyEchelon::Cavalry);
}

bool UStrategyFormationPolicyComponent::IsColumn(EStrategyFormationType Formation) const
{
    return Formation == EStrategyFormationType::MarchColumn || Formation == EStrategyFormationType::CavalryColumn || Formation == EStrategyFormationType::DefileColumn;
}

EStrategyFormationType UStrategyFormationPolicyComponent::GetDestinationFormation() const
{
    if (OwnerUnit && OwnerUnit->FormationComponent &&
        !IsColumn(OwnerUnit->FormationComponent->CurrentFormation))
        return OwnerUnit->FormationComponent->CurrentFormation;
    // A square is previewed only while the unit is actually keeping that square.
    if (BattleFormation != EStrategyFormationType::Square && !IsColumn(BattleFormation) &&
        !(OwnerUnit && OwnerUnit->IsA<ACavalryUnit>() && BattleFormation == EStrategyFormationType::Line))
        return BattleFormation;
    return OwnerUnit && OwnerUnit->IsA<ACavalryUnit>() ? EStrategyFormationType::CavalryLine : EStrategyFormationType::Line;
}

EStrategyFormationType UStrategyFormationPolicyComponent::ColumnFormation() const
{
    return OwnerUnit && OwnerUnit->IsA<ACavalryUnit>() ? EStrategyFormationType::CavalryColumn : EStrategyFormationType::MarchColumn;
}

float UStrategyFormationPolicyComponent::DeployDistanceCm(const AStrategyUnit* Enemy) const
{
    // The longest reach on either side (the enemy's rifles and guns, or our own), and a margin to form up in.
    auto Reach = [](const AStrategyUnit* U)
    {
        float R = U ? U->MaximumFireRangeCm : 0.0f;
        if (U && U->FireControlComponent)
        {
            R = FMath::Max(R, U->FireControlComponent->LongRangeCm);
        }
        return R;
    };
    return FMath::Max(Reach(OwnerUnit), Reach(Enemy)) + DeploySafetyBufferCm;
}

void UStrategyFormationPolicyComponent::Deploy()
{
    if (OwnerUnit && OwnerUnit->ThreatReactionComponent &&
        OwnerUnit->ThreatReactionComponent->ReactToCavalryThreat()) return;
    bMarching = false;
    if (!OwnerUnit || !OwnerUnit->FormationComponent)
    {
        return;
    }
    // A defile (a bridge, a lane) is left by the unit itself when it is through.
    const EStrategyFormationType Now = OwnerUnit->FormationComponent->CurrentFormation;
    if (IsColumn(Now) && Now != EStrategyFormationType::DefileColumn)
    {
        OwnerUnit->FormationComponent->SetFormation(BattleFormation);
        OwnerUnit->SetUnitState(EStrategyUnitState::Reforming);
    }
}

void UStrategyFormationPolicyComponent::ApplyInitialMovementFormation(const FStrategyOrder& Order)
{
    if (OwnerUnit && OwnerUnit->ThreatReactionComponent &&
        OwnerUnit->ThreatReactionComponent->ReactToCavalryThreat()) return;
    if (!MarchesInColumn())
    {
        return;
    }
    const EStrategyFormationType Now = OwnerUnit->FormationComponent->CurrentFormation;
    if (!IsColumn(Now))
    {
        BattleFormation = Now;
    }
    else if (!bMarching)
    {
        BattleFormation = OwnerUnit->IsA<ACavalryUnit>() ? EStrategyFormationType::CavalryLine : EStrategyFormationType::Line;
    }

    const float DistanceCm = FVector::Dist2D(
        OwnerUnit->GetActorLocation(),
        Order.TargetLocation);
    float EnemyDistanceCm = TNumericLimits<float>::Max();
    AStrategyUnit* Enemy = FindNearestEnemy(EnemyDistanceCm);
    const bool bEnemyClose = Enemy && EnemyDistanceCm <= DeployDistanceCm(Enemy);

    // A march: in column on the way, unless the enemy is already within reach (then it moves in its formation).
    if (DistanceCm >= LongMoveColumnThresholdCm && !bEnemyClose)
    {
        if (Now != EStrategyFormationType::DefileColumn)
        {
            OwnerUnit->FormationComponent->SetFormation(ColumnFormation());
        }
        bMarching = true;
    }
    else if (IsColumn(Now) && Now != EStrategyFormationType::DefileColumn)
    {
        OwnerUnit->FormationComponent->SetFormation(BattleFormation);
        bMarching = false;
    }
}

void UStrategyFormationPolicyComponent::EvaluateEarlyDeployment()
{
    if (OwnerUnit && OwnerUnit->ThreatReactionComponent &&
        OwnerUnit->ThreatReactionComponent->ReactToCavalryThreat()) return;
    if (!bMarching || !MarchesInColumn() || !IsColumn(OwnerUnit->FormationComponent->CurrentFormation))
    {
        return;
    }

    float EnemyDistanceCm = TNumericLimits<float>::Max();
    AStrategyUnit* NearestEnemy = FindNearestEnemy(EnemyDistanceCm);
    if (NearestEnemy && EnemyDistanceCm <= DeployDistanceCm(NearestEnemy))
    {
        Deploy();
        return;
    }
    // Close to the goal: it forms up for the last stretch, so it arrives in order.
    if (OwnerUnit->OrderComponent)
    {
        const FStrategyOrder Order = OwnerUnit->OrderComponent->GetCurrentOrder();
        if (FVector::Dist2D(OwnerUnit->GetActorLocation(), Order.TargetLocation) < 2500.0f)
        {
            Deploy();
        }
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
