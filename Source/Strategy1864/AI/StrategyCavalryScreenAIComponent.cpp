#include "StrategyCavalryScreenAIComponent.h"

#include "../Combat/StrategyVisibilityComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyUnit.h"
#include "EngineUtils.h"

UStrategyCavalryScreenAIComponent::UStrategyCavalryScreenAIComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyCavalryScreenAIComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCavalry = Cast<ACavalryUnit>(GetOwner());
}

void UStrategyCavalryScreenAIComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerCavalry ||
        !OwnerCavalry->bOfficerAIEnabled ||
        HasProtectedMission() ||
        !OwnerCavalry->CommandComponent ||
        !OwnerCavalry->OrderComponent)
    {
        return;
    }

    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator < EvaluationIntervalSeconds)
    {
        return;
    }

    EvaluationAccumulator = 0.0f;

    AStrategyUnit* Parent =
        OwnerCavalry->CommandComponent->CurrentCommandParent;
    AStrategyUnit* Enemy = FindNearestVisibleEnemy();

    if (!IsValid(Parent) || !IsValid(Enemy))
    {
        return;
    }

    FVector TowardEnemy =
        Enemy->GetActorLocation() - Parent->GetActorLocation();
    TowardEnemy.Z = 0.0f;
    TowardEnemy = TowardEnemy.GetSafeNormal();

    if (TowardEnemy.IsNearlyZero())
    {
        return;
    }

    const FVector Goal =
        Parent->GetActorLocation() +
        TowardEnemy * ScreenDistanceFromParentCm;

    if (FVector::Dist2D(Goal, LastScreenGoal) < RepositionThresholdCm &&
        FVector::Dist2D(OwnerCavalry->GetActorLocation(), Goal) <
            RepositionThresholdCm)
    {
        return;
    }

    FStrategyOrder ScreenOrder;
    ScreenOrder.Type = EStrategyOrderType::Move;
    ScreenOrder.TargetLocation = Goal;
    ScreenOrder.FacingYaw = TowardEnemy.Rotation().Yaw;
    ScreenOrder.bHasFacing = true;
    ScreenOrder.Authority = EStrategyOrderAuthority::OfficerAI;

    if (OwnerCavalry->OrderComponent->SetOrder(ScreenOrder))
    {
        LastScreenGoal = Goal;
    }
}

AStrategyUnit* UStrategyCavalryScreenAIComponent::FindNearestVisibleEnemy() const
{
    if (!OwnerCavalry ||
        !OwnerCavalry->VisibilityComponent ||
        !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* Best = nullptr;
    float BestDistance = DetectionRangeCm;

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate == OwnerCavalry ||
            !Candidate->IsCombatEffective() ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerCavalry->Side)
        {
            continue;
        }

        const float Distance =
            FVector::Dist2D(
                OwnerCavalry->GetActorLocation(),
                Candidate->GetActorLocation());

        if (Distance >= BestDistance ||
            !OwnerCavalry->VisibilityComponent->HasLineOfSightTo(Candidate))
        {
            continue;
        }

        BestDistance = Distance;
        Best = Candidate;
    }

    return Best;
}

bool UStrategyCavalryScreenAIComponent::HasProtectedMission() const
{
    if (!OwnerCavalry || !OwnerCavalry->OrderComponent)
    {
        return true;
    }

    const FStrategyOrder Current =
        OwnerCavalry->OrderComponent->GetCurrentOrder();

    if (!Current.IsValidOrder())
    {
        return false;
    }

    if (Current.Type == EStrategyOrderType::Charge)
    {
        return true;
    }

    if (OwnerCavalry->OrderComponent->IsPhysicallyExecuting())
    {
        return true;
    }

    return Current.IsStandingIntent();
}
