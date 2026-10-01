#include "StrategyLocalDeconflictionComponent.h"

#include "StrategyMovementExecutorComponent.h"
#include "../Units/StrategyUnit.h"
#include "EngineUtils.h"

UStrategyLocalDeconflictionComponent::UStrategyLocalDeconflictionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyLocalDeconflictionComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategyLocalDeconflictionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit ||
        OwnerUnit->Echelon != EStrategyEchelon::Company ||
        !OwnerUnit->MovementExecutor ||
        !OwnerUnit->MovementExecutor->HasMovementGoal())
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

    FVector Separation = FVector::ZeroVector;

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Other = *It;

        if (!IsValid(Other) ||
            Other == OwnerUnit ||
            Other->Echelon != EStrategyEchelon::Company ||
            Other->Side != OwnerUnit->Side ||
            Other->UnitState == EStrategyUnitState::Destroyed)
        {
            continue;
        }

        FVector Away = OwnerUnit->GetActorLocation() - Other->GetActorLocation();
        Away.Z = 0.0f;

        const float Distance = Away.Size();
        if (Distance <= KINDA_SMALL_NUMBER ||
            Distance >= MinimumCompanyCenterSeparationCm)
        {
            continue;
        }

        const float Overlap =
            MinimumCompanyCenterSeparationCm - Distance;

        Separation += Away.GetSafeNormal() * (Overlap * 0.5f);
    }

    if (Separation.IsNearlyZero())
    {
        return;
    }

    const float MaxStep =
        MaxSidestepCmPerSecond * EvaluationDelta;

    const FVector Applied =
        Separation.GetClampedToMaxSize(MaxStep);

    OwnerUnit->AddActorWorldOffset(
        Applied,
        true);
}
