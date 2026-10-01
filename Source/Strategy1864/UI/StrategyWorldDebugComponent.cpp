#include "StrategyWorldDebugComponent.h"

#include "../Command/StrategyCommandComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"
#include "DrawDebugHelpers.h"

UStrategyWorldDebugComponent::UStrategyWorldDebugComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyWorldDebugComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategyWorldDebugComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit || !OwnerUnit->bSelected || !GetWorld())
    {
        return;
    }

    DrawUnitRecursive(OwnerUnit, 0);
}

void UStrategyWorldDebugComponent::DrawUnitRecursive(
    AStrategyUnit* Unit,
    int32 Depth) const
{
    if (!IsValid(Unit) || Depth > 5)
    {
        return;
    }

    DrawMissionForUnit(Unit);
    DrawRouteForUnit(Unit);

    if (Unit->FormationComponent &&
        Unit->FormationComponent->UsesSquareVisualOwnership())
    {
        const float HalfExtent =
            FMath::Max(
                300.0f,
                Unit->FormationComponent->EstimateFrontageCm(
                    FMath::Max(1, Unit->CurrentStrength)) * 0.5f);

        const FRotator Rotation(0.0f, Unit->GetActorRotation().Yaw, 0.0f);
        const FVector Forward = Rotation.Vector();
        const FVector Right =
            FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
        const FVector Center =
            Unit->GetActorLocation() + FVector(0.0f, 0.0f, 25.0f);

        const FVector A = Center + Forward * HalfExtent + Right * HalfExtent;
        const FVector B = Center + Forward * HalfExtent - Right * HalfExtent;
        const FVector C = Center - Forward * HalfExtent - Right * HalfExtent;
        const FVector D = Center - Forward * HalfExtent + Right * HalfExtent;

        DrawDebugLine(GetWorld(), A, B, FColor(255, 220, 80), false, 0.0f, 0, 3.0f);
        DrawDebugLine(GetWorld(), B, C, FColor(255, 220, 80), false, 0.0f, 0, 3.0f);
        DrawDebugLine(GetWorld(), C, D, FColor(255, 220, 80), false, 0.0f, 0, 3.0f);
        DrawDebugLine(GetWorld(), D, A, FColor(255, 220, 80), false, 0.0f, 0, 3.0f);
    }

    if (!bDrawSelectedCommandTree || !Unit->CommandComponent)
    {
        return;
    }

    const FVector ParentPoint =
        Unit->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);

    for (AStrategyUnit* Child : Unit->CommandComponent->CurrentSubordinates)
    {
        if (!IsValid(Child))
        {
            continue;
        }

        const FVector ChildPoint =
            Child->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);

        DrawDebugLine(
            GetWorld(),
            ParentPoint,
            ChildPoint,
            FColor(90, 180, 255),
            false,
            0.0f,
            0,
            Depth == 0 ? 3.0f : 1.5f);

        DrawUnitRecursive(Child, Depth + 1);
    }
}

void UStrategyWorldDebugComponent::DrawMissionForUnit(
    const AStrategyUnit* Unit) const
{
    if (!bDrawMissionTargets || !IsValid(Unit) || !Unit->OrderComponent)
    {
        return;
    }

    const FStrategyOrder Order = Unit->OrderComponent->GetCurrentOrder();
    if (!Order.IsValidOrder())
    {
        return;
    }

    const FVector Target = Order.TargetLocation + FVector(0.0f, 0.0f, 20.0f);

    DrawDebugCircle(
        GetWorld(),
        Target,
        180.0f,
        24,
        FColor(255, 220, 80),
        false,
        0.0f,
        0,
        2.0f,
        FVector(1,0,0),
        FVector(0,1,0),
        false);

    if (Order.bHasFacing)
    {
        const FVector FacingEnd =
            Target + FRotator(0.0f, Order.FacingYaw, 0.0f).Vector() * 800.0f;

        DrawDebugDirectionalArrow(
            GetWorld(),
            Target,
            FacingEnd,
            120.0f,
            FColor(255, 220, 80),
            false,
            0.0f,
            0,
            2.5f);
    }
}

void UStrategyWorldDebugComponent::DrawRouteForUnit(
    const AStrategyUnit* Unit) const
{
    if (!bDrawRoutes || !IsValid(Unit) || !Unit->MovementExecutor)
    {
        return;
    }

    const TArray<FVector> Route = Unit->MovementExecutor->GetRoutePoints();
    if (Route.Num() == 0)
    {
        return;
    }

    FVector Previous = Unit->GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);

    for (const FVector& Point : Route)
    {
        const FVector RaisedPoint = Point + FVector(0.0f, 0.0f, 35.0f);

        DrawDebugLine(
            GetWorld(),
            Previous,
            RaisedPoint,
            FColor(80, 255, 160),
            false,
            0.0f,
            0,
            1.5f);

        Previous = RaisedPoint;
    }
}
