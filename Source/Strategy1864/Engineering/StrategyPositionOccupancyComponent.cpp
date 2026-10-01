#include "StrategyPositionOccupancyComponent.h"
#include "StrategyDefensivePosition.h"
#include "../Units/StrategyUnit.h"
#include "EngineUtils.h"

UStrategyPositionOccupancyComponent::UStrategyPositionOccupancyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyPositionOccupancyComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

bool UStrategyPositionOccupancyComponent::OccupyPosition(
    AStrategyDefensivePosition* Position)
{
    if (!OwnerUnit || !IsValid(Position))
    {
        return false;
    }

    if (FVector::Dist2D(OwnerUnit->GetActorLocation(), Position->GetActorLocation()) >
        FMath::Max(100.0f, MaximumOccupationDistanceCm))
    {
        return false;
    }

    if (OccupiedPosition == Position)
    {
        return Position->CanOccupy(OwnerUnit);
    }

    LeavePosition();

    if (!Position->Occupy(OwnerUnit))
    {
        return false;
    }

    OccupiedPosition = Position;
    return true;
}

void UStrategyPositionOccupancyComponent::LeavePosition()
{
    if (IsValid(OccupiedPosition) &&
        OwnerUnit &&
        OccupiedPosition->OccupyingUnitId == OwnerUnit->StableUnitId)
    {
        OccupiedPosition->Vacate();
    }

    OccupiedPosition = nullptr;
}

AStrategyDefensivePosition*
UStrategyPositionOccupancyComponent::FindNearestUsablePosition(
    float SearchRadiusCm) const
{
    if (!OwnerUnit || !GetWorld())
    {
        return nullptr;
    }

    AStrategyDefensivePosition* Best = nullptr;
    float BestDistance = FMath::Max(100.0f, SearchRadiusCm);

    for (TActorIterator<AStrategyDefensivePosition> It(GetWorld()); It; ++It)
    {
        AStrategyDefensivePosition* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->CanOccupy(OwnerUnit))
        {
            continue;
        }

        const float Distance =
            FVector::Dist2D(OwnerUnit->GetActorLocation(), Candidate->GetActorLocation());

        if (Distance <= BestDistance)
        {
            BestDistance = Distance;
            Best = Candidate;
        }
    }

    return Best;
}

bool UStrategyPositionOccupancyComponent::AutoOccupyNearest(float SearchRadiusCm)
{
    AStrategyDefensivePosition* Position =
        FindNearestUsablePosition(SearchRadiusCm);

    return Position && OccupyPosition(Position);
}

float UStrategyPositionOccupancyComponent::GetIncomingHitMultiplier(
    const FVector& ShooterLocation) const
{
    return IsValid(OccupiedPosition)
        ? OccupiedPosition->CalculateIncomingHitMultiplier(ShooterLocation)
        : 1.0f;
}

bool UStrategyPositionOccupancyComponent::HasArtilleryEmplacement() const
{
    return IsValid(OccupiedPosition) &&
        OccupiedPosition->SupportsArtilleryEmplacement();
}

bool UStrategyPositionOccupancyComponent::IsPositionFacingThreat(
    const FVector& ThreatLocation) const
{
    if (!IsValid(OccupiedPosition))
    {
        return false;
    }

    FVector ToThreat = ThreatLocation - OccupiedPosition->GetActorLocation();
    ToThreat.Z = 0.0f;
    ToThreat.Normalize();

    return FVector::DotProduct(
        OccupiedPosition->GetActorForwardVector().GetSafeNormal2D(),
        ToThreat) > 0.25f;
}

bool UStrategyPositionOccupancyComponent::IsPositionUsable() const
{
    return IsValid(OccupiedPosition) && OccupiedPosition->IsUsable();
}

float UStrategyPositionOccupancyComponent::GetPositionCondition() const
{
    return IsValid(OccupiedPosition) ? OccupiedPosition->Condition : 0.0f;
}

FString UStrategyPositionOccupancyComponent::GetPositionStatusText() const
{
    if (!IsValid(OccupiedPosition))
    {
        return TEXT("OPEN");
    }

    return FString::Printf(
        TEXT("%s %.0f%% %s"),
        *UEnum::GetValueAsString(OccupiedPosition->PositionType),
        OccupiedPosition->Condition,
        OccupiedPosition->bBreached ? TEXT("BREACHED") : TEXT("INTACT"));
}
