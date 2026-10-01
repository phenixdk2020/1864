#include "StrategyAutonomousBattleAIComponent.h"

#include "../Combat/StrategyContactComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"
#include "StrategyDoctrineComponent.h"
#include "StrategyAutonomyComponent.h"
#include "StrategyAIDifficultyComponent.h"
#include "StrategyAITelemetryComponent.h"
#include "StrategyMissionConstraintsComponent.h"
#include "StrategyOfficerProfileComponent.h"
#include "EngineUtils.h"

UStrategyAutonomousBattleAIComponent::UStrategyAutonomousBattleAIComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyAutonomousBattleAIComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());

    const int32 Seed =
        OwnerUnit
        ? static_cast<int32>(GetTypeHash(OwnerUnit->StableUnitId)) ^ 0x1864A1
        : GetUniqueID();

    DecisionRandom.Initialize(Seed);
}

void UStrategyAutonomousBattleAIComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsAutonomousEnemy() ||
        !OwnerUnit->bOfficerAIEnabled ||
        !OwnerUnit->OrderComponent ||
        !OwnerUnit->FireControlComponent ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed)
    {
        return;
    }

    float ReactionMultiplier = 1.0f;

    if (OwnerUnit->AIDifficultyComponent)
    {
        ReactionMultiplier *=
            OwnerUnit->AIDifficultyComponent->GetReactionTimeMultiplier();
    }

    if ((OwnerUnit->UnitState == EStrategyUnitState::UnderFire ||
         OwnerUnit->UnitState == EStrategyUnitState::Engaged) &&
        OwnerUnit->OfficerProfileComponent)
    {
        ReactionMultiplier *=
            OwnerUnit->OfficerProfileComponent->GetStressReactionMultiplier();
    }

    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator <
        EvaluationIntervalSeconds * FMath::Max(0.10f, ReactionMultiplier))
    {
        return;
    }

    EvaluationAccumulator = 0.0f;

    if (OwnerUnit->OrderComponent->IsPhysicallyExecuting() ||
        OwnerUnit->OrderComponent->HasStandingIntent())
    {
        return;
    }

    const bool bHasCommandParent =
        OwnerUnit->CommandComponent &&
        IsValid(OwnerUnit->CommandComponent->CurrentCommandParent);

    if (OwnerUnit->AutonomyComponent &&
        !OwnerUnit->AutonomyComponent->AllowsLocalRetask(bHasCommandParent))
    {
        return;
    }

    AStrategyUnit* Enemy = FindCurrentVisibleEnemy();
    if (!IsValid(Enemy))
    {
        if (OwnerUnit->AITelemetryComponent)
        {
            OwnerUnit->AITelemetryComponent->SetDecision(
                TEXT("Holding"),
                TEXT("No current visible enemy contact"));
        }
        return;
    }

    const float RangeCm =
        FMath::Max(
            1000.0f,
            OwnerUnit->FireControlComponent->GetActiveRangeCm());

    float PreferredFraction =
        OwnerUnit->DoctrineComponent
        ? OwnerUnit->DoctrineComponent->GetPreferredEngagementRangeFraction(
            OwnerUnit->OfficerProfileComponent)
        : DesiredRangeFraction;

    float NoiseAmplitude =
        OwnerUnit->AIDifficultyComponent
        ? OwnerUnit->AIDifficultyComponent->GetDecisionNoiseAmplitude()
        : 0.08f;

    if (OwnerUnit->OfficerProfileComponent)
    {
        NoiseAmplitude *=
            FMath::Lerp(
                1.25f,
                0.65f,
                OwnerUnit->OfficerProfileComponent->GetDecisionStability());
    }

    PreferredFraction =
        FMath::Clamp(
            PreferredFraction +
            DecisionRandom.FRandRange(-NoiseAmplitude, NoiseAmplitude),
            0.25f,
            0.95f);

    const float DesiredDistance =
        RangeCm * PreferredFraction;

    const float CurrentDistance =
        FVector::Dist2D(
            OwnerUnit->GetActorLocation(),
            Enemy->GetActorLocation());

    if (CurrentDistance <= DesiredDistance)
    {
        if (OwnerUnit->AITelemetryComponent)
        {
            OwnerUnit->AITelemetryComponent->SetDecision(
                TEXT("Hold engagement range"),
                TEXT("Preferred engagement range reached"));
        }
        return;
    }

    FVector FromEnemy =
        OwnerUnit->GetActorLocation() - Enemy->GetActorLocation();
    FromEnemy.Z = 0.0f;
    FromEnemy = FromEnemy.GetSafeNormal();

    if (FromEnemy.IsNearlyZero())
    {
        FromEnemy = -OwnerUnit->GetActorForwardVector().GetSafeNormal2D();
    }

    FVector Goal =
        Enemy->GetActorLocation() + FromEnemy * DesiredDistance;

    if (OwnerUnit->MissionConstraintsComponent)
    {
        Goal =
            OwnerUnit->MissionConstraintsComponent->ClampGoalToMissionArea(Goal);
    }

    FStrategyOrder Order;
    Order.Type = EStrategyOrderType::Advance;
    Order.TargetLocation = Goal;
    Order.FacingYaw =
        (Enemy->GetActorLocation() - Goal).Rotation().Yaw;
    Order.bHasFacing = true;
    Order.Authority = EStrategyOrderAuthority::OfficerAI;

    if (OwnerUnit->OrderComponent->SetOrder(Order) &&
        OwnerUnit->AITelemetryComponent)
    {
        OwnerUnit->AITelemetryComponent->SetDecision(
            TEXT("Advance to engagement range"),
            TEXT("Closing to doctrine/officer preferred range"));
    }
}

AStrategyUnit* UStrategyAutonomousBattleAIComponent::FindCurrentVisibleEnemy() const
{
    if (!OwnerUnit || !OwnerUnit->ContactComponent || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* Best = nullptr;
    float BestDistance = TNumericLimits<float>::Max();

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate == OwnerUnit ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerUnit->Side ||
            !Candidate->IsCombatEffective() ||
            !OwnerUnit->ContactComponent->HasCurrentContact(Candidate) ||
            (OwnerUnit->MissionConstraintsComponent &&
             !OwnerUnit->MissionConstraintsComponent->CanPursueTarget(Candidate)))
        {
            continue;
        }

        const float Distance =
            FVector::Dist2D(
                OwnerUnit->GetActorLocation(),
                Candidate->GetActorLocation());

        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = Candidate;
        }
    }

    return Best;
}

bool UStrategyAutonomousBattleAIComponent::IsAutonomousEnemy() const
{
    if (!bEnableForNonPlayerSides || !OwnerUnit)
    {
        return false;
    }

    const bool bEnemySide =
        OwnerUnit->Side == EStrategySide::Prussia ||
        OwnerUnit->Side == EStrategySide::Austria ||
        OwnerUnit->Side == EStrategySide::Enemy;

    return bEnemySide &&
        OwnerUnit->Echelon == EStrategyEchelon::Company;
}
