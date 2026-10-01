#include "StrategyCavalryChargeComponent.h"

#include "../Formations/StrategyFormationComponent.h"
#include "../Formations/StrategyFormationTransitionComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyUnit.h"
#include "Engine/World.h"

UStrategyCavalryChargeComponent::UStrategyCavalryChargeComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyCavalryChargeComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCavalry = Cast<ACavalryUnit>(GetOwner());
    if (!OwnerCavalry || !OwnerCavalry->OrderComponent)
    {
        SetComponentTickEnabled(false);
        return;
    }

    OwnerCavalry->OrderComponent->OnOrderChanged.AddDynamic(
        this,
        &UStrategyCavalryChargeComponent::HandleOrderChanged);
}

void UStrategyCavalryChargeComponent::HandleOrderChanged(
    const FStrategyOrder& NewOrder)
{
    if (NewOrder.Type == EStrategyOrderType::Charge)
    {
        BeginCharge();
    }
    else if (bChargeActive)
    {
        EndCharge();
    }
}

void UStrategyCavalryChargeComponent::BeginCharge()
{
    if (!OwnerCavalry || !OwnerCavalry->MovementExecutor)
    {
        return;
    }

    if (OwnerCavalry->FormationTransition &&
        OwnerCavalry->FormationTransition->IsReforming())
    {
        bChargeActive = false;
        SetComponentTickEnabled(false);
        return;
    }

    if (!bChargeActive)
    {
        PreviousMoveSpeed = OwnerCavalry->MovementExecutor->MoveSpeedCmPerSecond;
    }

    bChargeActive = true;
    PreviousLocation = OwnerCavalry->GetActorLocation();
    OwnerCavalry->MovementExecutor->MoveSpeedCmPerSecond = ChargeSpeedCmPerSecond;

    if (OwnerCavalry->FormationComponent)
    {
        OwnerCavalry->FormationComponent->SetFormation(
            EStrategyFormationType::CavalryLine);
    }

    SetComponentTickEnabled(true);
}

void UStrategyCavalryChargeComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bChargeActive ||
        !OwnerCavalry ||
        !OwnerCavalry->OrderComponent ||
        !OwnerCavalry->MovementExecutor)
    {
        SetComponentTickEnabled(false);
        return;
    }

    const FStrategyOrder Order = OwnerCavalry->OrderComponent->GetCurrentOrder();
    if (Order.Type != EStrategyOrderType::Charge ||
        !OwnerCavalry->OrderComponent->IsPhysicallyExecuting())
    {
        EndCharge();
        return;
    }

    const FVector CurrentLocation = OwnerCavalry->GetActorLocation();
    FVector ContactLocation = CurrentLocation;

    if (AStrategyUnit* Target =
        DetectEnemyContact(PreviousLocation, CurrentLocation, ContactLocation))
    {
        OwnerCavalry->SetActorLocation(ContactLocation);
        OwnerCavalry->MovementExecutor->StopMovement();
        OwnerCavalry->SetUnitState(EStrategyUnitState::Engaged);

        Target->ApplyStrengthLoss(ChargeImpactCasualties);
        Target->Morale = FMath::Clamp(Target->Morale - 5.0f, 0.0f, 100.0f);
        Target->Cohesion = FMath::Clamp(Target->Cohesion - 8.0f, 0.0f, 100.0f);

        OwnerCavalry->OrderComponent->CompleteExecution();
        EndCharge();
        return;
    }

    PreviousLocation = CurrentLocation;
}

AStrategyUnit* UStrategyCavalryChargeComponent::DetectEnemyContact(
    const FVector& From,
    const FVector& To,
    FVector& OutContactLocation) const
{
    if (!OwnerCavalry || !GetWorld())
    {
        return nullptr;
    }

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CavalryCharge), false);
    Params.AddIgnoredActor(OwnerCavalry);

    const bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        From,
        To,
        FQuat::Identity,
        ECC_Visibility,
        FCollisionShape::MakeSphere(ContactRadiusCm),
        Params);

    if (!bHit)
    {
        return nullptr;
    }

    for (const FHitResult& Hit : Hits)
    {
        AStrategyUnit* Candidate = Cast<AStrategyUnit>(Hit.GetActor());
        if (!IsValid(Candidate) ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerCavalry->Side ||
            !Candidate->IsCombatEffective())
        {
            continue;
        }

        OutContactLocation = Hit.Location;
        return Candidate;
    }

    return nullptr;
}

void UStrategyCavalryChargeComponent::EndCharge()
{
    if (OwnerCavalry && OwnerCavalry->MovementExecutor)
    {
        OwnerCavalry->MovementExecutor->MoveSpeedCmPerSecond =
            PreviousMoveSpeed > 0.0f
            ? PreviousMoveSpeed
            : 900.0f;
    }

    bChargeActive = false;
    SetComponentTickEnabled(false);
}
