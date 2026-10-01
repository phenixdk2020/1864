#include "StrategyDefensivePosition.h"
#include "../Navigation/StrategyNavigationObstacle.h"
#include "Engine/World.h"

AStrategyDefensivePosition::AStrategyDefensivePosition()
{
    PrimaryActorTick.bCanEverTick = false;
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void AStrategyDefensivePosition::BeginPlay()
{
    Super::BeginPlay();

    if (!GetWorld())
    {
        return;
    }

    NavigationObstacle =
        GetWorld()->SpawnActor<AStrategyNavigationObstacle>(
            AStrategyNavigationObstacle::StaticClass(),
            GetActorLocation(),
            GetActorRotation());

    RefreshNavigationObstacle();
}

void AStrategyDefensivePosition::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(NavigationObstacle))
    {
        NavigationObstacle->Destroy();
    }

    NavigationObstacle = nullptr;
    Super::EndPlay(EndPlayReason);
}

void AStrategyDefensivePosition::RefreshNavigationObstacle()
{
    if (!IsValid(NavigationObstacle))
    {
        return;
    }

    NavigationObstacle->SetActorLocation(GetActorLocation());
    NavigationObstacle->SetActorRotation(GetActorRotation());
    NavigationObstacle->ObstacleType = EStrategyObstacleType::Fieldworks;
    NavigationObstacle->HalfExtentCm =
        FVector(
            FMath::Max(100.0f, DepthCm * 0.5f),
            FMath::Max(200.0f, LengthCm * 0.5f),
            120.0f);
    NavigationObstacle->ClearanceCm =
        PositionType == EStrategyDefensivePositionType::AbatisObstacle
        ? 500.0f
        : 150.0f;
}

bool AStrategyDefensivePosition::CanOccupy(const AStrategyUnit* Unit) const
{
    return IsValid(Unit) &&
        IsUsable() &&
        (OccupyingUnitId.IsNone() || OccupyingUnitId == Unit->StableUnitId) &&
        Unit->CurrentStrength <= FMath::Max(1, CapacityMen);
}

bool AStrategyDefensivePosition::Occupy(AStrategyUnit* Unit)
{
    if (!CanOccupy(Unit))
    {
        return false;
    }

    OccupyingUnitId = Unit->StableUnitId;
    if (OwningSide == EStrategySide::Neutral)
    {
        OwningSide = Unit->Side;
    }
    return true;
}

void AStrategyDefensivePosition::Vacate()
{
    OccupyingUnitId = NAME_None;
}

float AStrategyDefensivePosition::ApplyStructuralDamage(float Damage)
{
    const float Applied = FMath::Clamp(Damage, 0.0f, Condition);
    Condition -= Applied;
    if (Condition <= 20.0f)
    {
        bBreached = true;
    }
    return Applied;
}

float AStrategyDefensivePosition::Repair(float Amount)
{
    if (Amount <= 0.0f)
    {
        return 0.0f;
    }

    const float Before = Condition;
    Condition = FMath::Clamp(Condition + Amount, 0.0f, 100.0f);
    if (Condition > 35.0f)
    {
        bBreached = false;
    }
    return Condition - Before;
}

bool AStrategyDefensivePosition::Capture(EStrategySide NewSide)
{
    if (!OccupyingUnitId.IsNone() || NewSide == EStrategySide::Neutral)
    {
        return false;
    }

    OwningSide = NewSide;
    return true;
}

void AStrategyDefensivePosition::MarkBreached(bool bNewBreached)
{
    bBreached = bNewBreached;
}

float AStrategyDefensivePosition::CalculateIncomingHitMultiplier(
    const FVector& ShooterLocation) const
{
    if (!IsUsable())
    {
        return 1.0f;
    }

    FVector ToShooter = ShooterLocation - GetActorLocation();
    ToShooter.Z = 0.0f;
    ToShooter.Normalize();

    const FVector ProtectedDirection =
        GetActorForwardVector().GetSafeNormal2D();

    const float Dot = FMath::Clamp(
        FVector::DotProduct(ProtectedDirection, ToShooter),
        -1.0f,
        1.0f);

    const float Angle =
        FMath::RadiansToDegrees(FMath::Acos(Dot));

    const float ConditionFactor =
        FMath::Lerp(1.0f, 0.0f, FMath::Clamp(Condition / 100.0f, 0.0f, 1.0f));

    const float Base =
        Angle <= ProtectedHalfAngleDegrees
        ? FrontalHitMultiplier
        : RearHitMultiplier;

    const float BreachPenalty = bBreached ? 0.25f : 0.0f;

    return FMath::Clamp(
        Base + ConditionFactor * 0.20f + BreachPenalty,
        0.20f,
        1.0f);
}

bool AStrategyDefensivePosition::SupportsArtilleryEmplacement() const
{
    return PositionType == EStrategyDefensivePositionType::GunEmplacement ||
           PositionType == EStrategyDefensivePositionType::Redoubt;
}

bool AStrategyDefensivePosition::IsUsable() const
{
    return ConstructionProgress >= 0.50f && Condition > 0.0f;
}
