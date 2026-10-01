#include "StrategyMortarFireComponent.h"
#include "StrategyMortarDeploymentComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Engineering/StrategyDefensivePosition.h"

UStrategyMortarFireComponent::UStrategyMortarFireComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyMortarFireComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    Deployment = GetOwner()
        ? GetOwner()->FindComponentByClass<UStrategyMortarDeploymentComponent>()
        : nullptr;

    const int32 Seed = OwnerUnit
        ? static_cast<int32>(GetTypeHash(OwnerUnit->StableUnitId) ^ 0x4D4F5254)
        : GetUniqueID();
    RandomStream.Initialize(Seed);
}

bool UStrategyMortarFireComponent::SetUnitTarget(AStrategyUnit* NewTarget)
{
    if (!OwnerUnit ||
        !IsValid(NewTarget) ||
        NewTarget->Side == OwnerUnit->Side ||
        !IsTargetInRange(NewTarget->GetActorLocation()))
    {
        return false;
    }

    UnitTarget = NewTarget;
    PositionTarget = nullptr;
    AreaTarget = NewTarget->GetActorLocation();
    FireMode = EStrategyMortarFireMode::UnitTarget;
    return true;
}

bool UStrategyMortarFireComponent::SetAreaTarget(const FVector& NewTarget)
{
    if (!IsTargetInRange(NewTarget))
    {
        return false;
    }

    UnitTarget = nullptr;
    PositionTarget = nullptr;
    AreaTarget = NewTarget;
    FireMode = EStrategyMortarFireMode::AreaTarget;
    return true;
}

bool UStrategyMortarFireComponent::SetFortificationTarget(
    AStrategyDefensivePosition* NewTarget)
{
    if (!IsValid(NewTarget) || !IsTargetInRange(NewTarget->GetActorLocation()))
    {
        return false;
    }

    PositionTarget = NewTarget;
    UnitTarget = nullptr;
    AreaTarget = NewTarget->GetActorLocation();
    FireMode = EStrategyMortarFireMode::AreaTarget;
    return true;
}

void UStrategyMortarFireComponent::HoldFire()
{
    FireMode = EStrategyMortarFireMode::Hold;
}

bool UStrategyMortarFireComponent::FireOneBomb()
{
    if (!OwnerUnit ||
        !Deployment ||
        !Deployment->CanFire() ||
        FireMode == EStrategyMortarFireMode::Hold ||
        AmmunitionBombs <= 0 ||
        ReloadRemainingSeconds > 0.0f)
    {
        return false;
    }

    FVector TargetLocation = AreaTarget;
    if (FireMode == EStrategyMortarFireMode::UnitTarget && IsValid(UnitTarget))
    {
        TargetLocation = UnitTarget->GetActorLocation();
    }

    if (!IsTargetInRange(TargetLocation))
    {
        return false;
    }

    --AmmunitionBombs;
    ReloadRemainingSeconds = FMath::Max(0.5f, ReloadSeconds);

    if (IsValid(PositionTarget))
    {
        PositionTarget->ApplyStructuralDamage(
            FortificationDamagePerBomb *
            RandomStream.FRandRange(0.75f, 1.25f));
    }
    else if (IsValid(UnitTarget))
    {
        const float Distance =
            FVector::Dist2D(OwnerUnit->GetActorLocation(), TargetLocation);

        const float RangeFactor =
            FMath::Clamp(1.0f - (Distance / FMath::Max(1.0f, MaxRangeCm)) * 0.35f, 0.45f, 1.0f);

        if (RandomStream.FRand() < InfantryHitChancePerBomb * RangeFactor)
        {
            UnitTarget->ApplyStrengthLoss(RandomStream.RandRange(1, 4));
        }
    }

    return true;
}

void UStrategyMortarFireComponent::ResupplyBombs(int32 Bombs)
{
    AmmunitionBombs =
        FMath::Clamp(
            AmmunitionBombs + FMath::Max(0, Bombs),
            0,
            FMath::Max(1, MaxAmmunitionBombs));
}

bool UStrategyMortarFireComponent::IsTargetInRange(
    const FVector& TargetLocation) const
{
    if (!OwnerUnit)
    {
        return false;
    }

    const float Distance =
        FVector::Dist2D(OwnerUnit->GetActorLocation(), TargetLocation);

    return Distance >= FMath::Max(0.0f, MinRangeCm) &&
           Distance <= FMath::Max(MinRangeCm, MaxRangeCm);
}

FVector UStrategyMortarFireComponent::BuildHighArcApex(
    const FVector& TargetLocation) const
{
    if (!OwnerUnit)
    {
        return TargetLocation;
    }

    const FVector Mid =
        (OwnerUnit->GetActorLocation() + TargetLocation) * 0.5f;

    const float Distance =
        FVector::Dist2D(OwnerUnit->GetActorLocation(), TargetLocation);

    return Mid + FVector(0.0f, 0.0f, FMath::Clamp(Distance * 0.55f, 1800.0f, 10000.0f));
}

float UStrategyMortarFireComponent::GetEstimatedTimeOfFlight(
    const FVector& TargetLocation) const
{
    if (!OwnerUnit)
    {
        return 0.0f;
    }

    const float Distance =
        FVector::Dist2D(OwnerUnit->GetActorLocation(), TargetLocation);

    return FMath::Clamp(Distance / 3500.0f, 1.5f, 8.0f);
}

void UStrategyMortarFireComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ReloadRemainingSeconds =
        FMath::Max(0.0f, ReloadRemainingSeconds - DeltaTime);

    if (ReloadRemainingSeconds <= 0.0f &&
        FireMode != EStrategyMortarFireMode::Hold)
    {
        FireOneBomb();
    }
}
