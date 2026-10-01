#include "StrategyRoutRecoveryComponent.h"

#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"
#include "StrategyOfficerProfileComponent.h"
#include "StrategyNCOComponent.h"
#include "EngineUtils.h"

UStrategyRoutRecoveryComponent::UStrategyRoutRecoveryComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyRoutRecoveryComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategyRoutRecoveryComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit ||
        OwnerUnit->UnitState != EStrategyUnitState::Routed ||
        OwnerUnit->CurrentStrength <= 0)
    {
        bFallbackIssued = false;
        return;
    }

    float NearestDistance = TNumericLimits<float>::Max();
    AStrategyUnit* Enemy = FindNearestEnemy(NearestDistance);

    if (!bFallbackIssued && IsValid(Enemy))
    {
        StartFallback(Enemy);
    }

    TryRally(DeltaTime, NearestDistance);
}

AStrategyUnit* UStrategyRoutRecoveryComponent::FindNearestEnemy(
    float& OutDistanceCm) const
{
    OutDistanceCm = TNumericLimits<float>::Max();

    if (!OwnerUnit || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* Best = nullptr;

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;
        if (!IsValid(Candidate) ||
            Candidate == OwnerUnit ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerUnit->Side ||
            !Candidate->IsCombatEffective())
        {
            continue;
        }

        const float Distance =
            FVector::Dist2D(
                OwnerUnit->GetActorLocation(),
                Candidate->GetActorLocation());

        if (Distance < OutDistanceCm)
        {
            OutDistanceCm = Distance;
            Best = Candidate;
        }
    }

    return Best;
}

void UStrategyRoutRecoveryComponent::StartFallback(AStrategyUnit* Enemy)
{
    if (!OwnerUnit || !OwnerUnit->OrderComponent || !IsValid(Enemy))
    {
        return;
    }

    FVector Away =
        OwnerUnit->GetActorLocation() - Enemy->GetActorLocation();
    Away.Z = 0.0f;
    Away = Away.GetSafeNormal();

    if (Away.IsNearlyZero())
    {
        Away = -OwnerUnit->GetActorForwardVector().GetSafeNormal2D();
    }

    OwnerUnit->OrderComponent->ClearOrder();

    FStrategyOrder Fallback;
    Fallback.Type = EStrategyOrderType::Withdraw;
    Fallback.TargetLocation =
        OwnerUnit->GetActorLocation() + Away * FallbackDistanceCm;
    Fallback.FacingYaw = Away.Rotation().Yaw;
    Fallback.bHasFacing = true;
    Fallback.Authority = EStrategyOrderAuthority::OfficerAI;

    if (OwnerUnit->OrderComponent->SetOrder(Fallback))
    {
        // Preserve routed behaviour while the fallback movement executes.
        OwnerUnit->SetUnitState(EStrategyUnitState::Routed);
        bFallbackIssued = true;
    }
}

void UStrategyRoutRecoveryComponent::TryRally(
    float DeltaTime,
    float NearestEnemyDistanceCm)
{
    if (!OwnerUnit ||
        NearestEnemyDistanceCm < SafeEnemyDistanceCm)
    {
        return;
    }

    const float InspirationMultiplier =
        OwnerUnit->OfficerProfileComponent
        ? FMath::Lerp(
            0.80f,
            1.25f,
            FMath::Clamp(
                OwnerUnit->OfficerProfileComponent->Inspiration / 100.0f,
                0.0f,
                1.0f))
        : 1.0f;

    const float NCOMultiplier =
        OwnerUnit->NCOComponent
        ? OwnerUnit->NCOComponent->GetRallyMultiplier()
        : 1.0f;

    OwnerUnit->Morale = FMath::Clamp(
        OwnerUnit->Morale +
        MoraleRecoveryPerSecond *
        InspirationMultiplier *
        NCOMultiplier *
        DeltaTime,
        0.0f,
        100.0f);

    OwnerUnit->Cohesion = FMath::Clamp(
        OwnerUnit->Cohesion +
        CohesionRecoveryPerSecond *
        InspirationMultiplier *
        NCOMultiplier *
        DeltaTime,
        0.0f,
        100.0f);

    if (OwnerUnit->Morale < RallyMoraleThreshold ||
        OwnerUnit->Cohesion < RallyCohesionThreshold)
    {
        return;
    }

    OwnerUnit->OrderComponent->ClearOrder();
    OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
    bFallbackIssued = false;
}
