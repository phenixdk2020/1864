#include "StrategyFieldworksComponent.h"

#include "../Engineering/StrategyDefensivePosition.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Engineering/StrategyPositionOccupancyComponent.h"
#include "../Navigation/StrategyNavigationObstacle.h"
#include "Engine/World.h"

UStrategyFieldworksComponent::UStrategyFieldworksComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyFieldworksComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

bool UStrategyFieldworksComponent::BeginHastyFieldworks()
{
    if (!OwnerUnit ||
        OwnerUnit->Echelon != EStrategyEchelon::Company ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed ||
        (OwnerUnit->MovementExecutor &&
         OwnerUnit->MovementExecutor->HasMovementGoal()))
    {
        return false;
    }

    if (HasCompletedFieldworks())
    {
        return true;
    }

    const float Multiplier =
        bEngineerUnit
        ? FMath::Clamp(EngineerBuildTimeMultiplier, 0.10f, 1.0f)
        : 1.0f;

    BuildRemainingSeconds =
        FMath::Max(0.5f, BuildSeconds * Multiplier);

    bBuilding = true;
    SetComponentTickEnabled(true);
    return true;
}

void UStrategyFieldworksComponent::CancelFieldworks()
{
    bBuilding = false;
    BuildRemainingSeconds = 0.0f;
    SetComponentTickEnabled(false);
}

bool UStrategyFieldworksComponent::HasCompletedFieldworks() const
{
    return IsValid(CompletedPosition);
}

void UStrategyFieldworksComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bBuilding || !OwnerUnit)
    {
        SetComponentTickEnabled(false);
        return;
    }

    if ((OwnerUnit->MovementExecutor &&
         OwnerUnit->MovementExecutor->HasMovementGoal()) ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed)
    {
        CancelFieldworks();
        return;
    }

    BuildRemainingSeconds =
        FMath::Max(0.0f, BuildRemainingSeconds - DeltaTime);

    if (BuildRemainingSeconds <= 0.0f)
    {
        CompleteFieldworks();
    }
}

void UStrategyFieldworksComponent::CompleteFieldworks()
{
    if (!OwnerUnit || !GetWorld())
    {
        CancelFieldworks();
        return;
    }

    CompletedPosition =
        GetWorld()->SpawnActor<AStrategyDefensivePosition>(
            AStrategyDefensivePosition::StaticClass(),
            OwnerUnit->GetActorLocation(),
            OwnerUnit->GetActorRotation());

    if (CompletedPosition)
    {
        const float Frontage =
            OwnerUnit->FormationComponent
            ? OwnerUnit->FormationComponent->EstimateFrontageCm(
                FMath::Max(1, OwnerUnit->CurrentStrength))
            : 4800.0f;

        CompletedPosition->PositionType = RequestedPositionType;
        CompletedPosition->OwningSide = OwnerUnit->Side;
        CompletedPosition->LengthCm = FMath::Max(1600.0f, Frontage);
        CompletedPosition->CapacityMen = FMath::Max(40, OwnerUnit->CurrentStrength);
        CompletedPosition->ConstructionProgress = 1.0f;
        CompletedPosition->Condition = bEngineerUnit ? 100.0f : 80.0f;

        if (CompletedPosition->NavigationObstacle)
        {
            CompletedPosition->NavigationObstacle->HalfExtentCm =
                FVector(
                    FMath::Max(100.0f, CompletedPosition->DepthCm * 0.5f),
                    FMath::Max(200.0f, CompletedPosition->LengthCm * 0.5f),
                    120.0f);
            CompletedPosition->NavigationObstacle->ClearanceCm =
                RequestedPositionType == EStrategyDefensivePositionType::AbatisObstacle
                ? 500.0f
                : 150.0f;
        }

        if (OwnerUnit->PositionOccupancyComponent)
        {
            OwnerUnit->PositionOccupancyComponent->OccupyPosition(
                CompletedPosition);
        }
    }

    bBuilding = false;
    BuildRemainingSeconds = 0.0f;
    SetComponentTickEnabled(false);
}
