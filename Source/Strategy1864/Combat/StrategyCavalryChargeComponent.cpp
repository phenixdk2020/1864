#include "StrategyCavalryChargeComponent.h"
#include "../Audio/StrategyBattleAudio.h"
#include "StrategyCombatComponent.h"
#include "EngineUtils.h"

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

    if (!bChargeActive)
        if (UStrategyBattleAudio* ChargeAudio = UStrategyBattleAudio::Find(this)) ChargeAudio->Charge(OwnerCavalry, true);
    bChargeActive = true;
    bLastChargeRepulsed = false;
    ChargeMomentum = 0.0f;
    ChargeConfidence = FMath::Clamp(OwnerCavalry->Morale / 100.0f * OwnerCavalry->Cohesion / 100.0f, 0.0f, 1.0f);
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

    if (!OwnerCavalry->IsCombatEffective()) { EndCharge(); return; }
    const FVector CurrentLocation = OwnerCavalry->GetActorLocation();
    ChargeMomentum = FMath::Clamp(ChargeMomentum + FVector::Dist2D(PreviousLocation, CurrentLocation) /
        FMath::Max(1.0f, MomentumBuildDistanceCm), 0.0f, 1.0f);
    ChargeConfidence = FMath::Clamp(OwnerCavalry->Morale / 100.0f * OwnerCavalry->Cohesion / 100.0f, 0.0f, 1.0f);
    // Sweep the approach too: a horse refuses steady bayonets before physical contact.
    AStrategyUnit* RepellingSquare = nullptr;
    float NearestSquareAlong = MAX_flt;
    const FVector ChargeDirection = (Order.TargetLocation - PreviousLocation).GetSafeNormal2D();
    for (TActorIterator<AStrategyUnit> ChargeIt(GetWorld()); ChargeIt; ++ChargeIt)
    {
        AStrategyUnit* SquareCandidate = *ChargeIt;
        if (!IsValid(SquareCandidate) || SquareCandidate->Side == OwnerCavalry->Side ||
            SquareCandidate->Side == EStrategySide::Neutral || !IsSteadySquare(SquareCandidate)) { continue; }
        const FVector SquareOffset = SquareCandidate->GetActorLocation() - PreviousLocation;
        const float SquareAlong = FVector::DotProduct(SquareOffset, ChargeDirection);
        const float SquareAcross = (SquareOffset - ChargeDirection * SquareAlong).Size2D();
        if (SquareAlong >= 0.0f && SquareAlong <= FVector::Dist2D(PreviousLocation, CurrentLocation) + SquareRepelDistanceCm &&
            SquareAcross <= ContactRadiusCm + 180.0f && SquareAlong < NearestSquareAlong)
        { RepellingSquare = SquareCandidate; NearestSquareAlong = SquareAlong; }
    }
    if (RepellingSquare)
    {
        OwnerCavalry->SetActorLocation(PreviousLocation + ChargeDirection * FMath::Max(0.0f, NearestSquareAlong - SquareRepelDistanceCm));
        RepelCharge(RepellingSquare);
        return;
    }
    FVector ContactLocation = CurrentLocation;

    if (AStrategyUnit* Target =
        DetectEnemyContact(PreviousLocation, CurrentLocation, ContactLocation))
    {
        OwnerCavalry->SetActorLocation(ContactLocation);
        OwnerCavalry->MovementExecutor->StopMovement();
        OwnerCavalry->SetUnitState(EStrategyUnitState::Engaged);

        if (IsSteadySquare(Target)) { RepelCharge(Target); return; }
        ResolveImpact(Target);

        if (OwnerCavalry->IsCombatEffective()) { OwnerCavalry->OrderComponent->CompleteExecution(); }
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

    AStrategyUnit* NearestContact = nullptr;
    float NearestContactTime = MAX_flt;
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

        if (Hit.Time < NearestContactTime)
        {
            NearestContactTime = Hit.Time;
            OutContactLocation = Hit.Location;
            NearestContact = Candidate;
        }
    }

    return NearestContact;
}

void UStrategyCavalryChargeComponent::EndCharge()
{
    if (UStrategyBattleAudio* ChargeAudio = UStrategyBattleAudio::Find(this)) ChargeAudio->Charge(OwnerCavalry, false);
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

bool UStrategyCavalryChargeComponent::IsSteadySquare(const AStrategyUnit* Target) const
{
    return Target && Target->IsCombatEffective() && Target->FormationComponent &&
        Target->FormationComponent->CurrentFormation == EStrategyFormationType::Square &&
        (!Target->FormationTransition || !Target->FormationTransition->IsReforming()) &&
        Target->Morale >= SteadySquareMorale && Target->Cohesion >= SteadySquareCohesion &&
        Target->CombatComponent && Target->CombatComponent->AmmunitionRounds > 0;
}

void UStrategyCavalryChargeComponent::RepelCharge(AStrategyUnit* Target)
{
    // Approach volleys use the ordinary fire system and consume ammunition normally.
    if (Target->CombatComponent) { Target->CombatComponent->TryFireAt(OwnerCavalry); }
    bLastChargeRepulsed = true;
    ChargeMomentum = 0.0f;
    OwnerCavalry->Morale = FMath::Max(0.0f, OwnerCavalry->Morale - RepulseMoraleLoss * (1.0f + ChargeConfidence));
    OwnerCavalry->Cohesion = FMath::Max(0.0f, OwnerCavalry->Cohesion - RepulseMoraleLoss * ChargeConfidence);
    if (OwnerCavalry->CombatComponent) { OwnerCavalry->CombatComponent->EvaluateRoutState(); }
    OwnerCavalry->MovementExecutor->StopMovement();
    if (OwnerCavalry->IsCombatEffective()) { OwnerCavalry->OrderComponent->CompleteExecution(); }
    EndCharge();
    if (OwnerCavalry->IsCombatEffective())
    {
        FStrategyOrder RepulseOrder;
        RepulseOrder.Type = EStrategyOrderType::Withdraw;
        RepulseOrder.Authority = EStrategyOrderAuthority::OfficerAI;
        const FVector RepulseAway = (OwnerCavalry->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
        RepulseOrder.TargetLocation = OwnerCavalry->GetActorLocation() + RepulseAway * SquareRepelDistanceCm;
        OwnerCavalry->OrderComponent->SetOrder(RepulseOrder);
    }
}

void UStrategyCavalryChargeComponent::ResolveImpact(AStrategyUnit* Target)
{
    const EStrategyFormationType ImpactFormation = Target->FormationComponent ?
        Target->FormationComponent->CurrentFormation : EStrategyFormationType::Line;
    const float ImpactFacing = FVector::DotProduct(Target->GetActorForwardVector().GetSafeNormal2D(),
        (OwnerCavalry->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D());
    const float DirectionShock = ImpactFacing < -0.5f ? RearImpactMultiplier * FlankShockFactor :
        ImpactFacing < 0.5f ? FlankImpactMultiplier * FlankShockFactor : 1.0f;
    const float FormationShock = ImpactFormation == EStrategyFormationType::Square ? BrokenSquareImpactMultiplier :
        (ImpactFormation == EStrategyFormationType::MarchColumn || ImpactFormation == EStrategyFormationType::DefileColumn ? ColumnImpactMultiplier : 1.0f);
    const float AttackingPower = OwnerCavalry->CurrentStrength * ChargeConfidence * FMath::Lerp(0.25f, 1.0f, ChargeMomentum);
    const float DefendingPower = FMath::Max(1.0f, Target->CurrentStrength *
        FMath::Max(0.1f, Target->Morale / 100.0f) * FMath::Max(0.1f, Target->Cohesion / 100.0f));
    const float ImpactRatio = FMath::Clamp(AttackingPower / DefendingPower, 0.2f, 3.0f);
    const int32 ImpactLoss = FMath::RoundToInt(AttackingPower * ImpactLossPerRider * DirectionShock * FormationShock * ImpactRatio);
    if (UStrategyBattleAudio* ImpactAudio = UStrategyBattleAudio::Find(this)) ImpactAudio->Clash(Target->GetActorLocation());
    Target->ApplyStrengthLoss(ImpactLoss);
    const float ImpactShare = float(ImpactLoss) / FMath::Max(1, Target->InitialStrength);
    Target->Morale = FMath::Max(0.0f, Target->Morale - ImpactMoraleShock * ImpactShare * DirectionShock);
    Target->Cohesion = FMath::Max(0.0f, Target->Cohesion - ImpactCohesionShock * ImpactShare * FormationShock);
    const int32 CavalryLoss = FMath::RoundToInt(DefendingPower * ImpactLossPerRider / FMath::Max(1.0f, DirectionShock * FormationShock * ImpactRatio));
    OwnerCavalry->ApplyStrengthLoss(CavalryLoss);
    OwnerCavalry->Morale = FMath::Max(0.0f, OwnerCavalry->Morale - ImpactMoraleShock * float(CavalryLoss) / FMath::Max(1, OwnerCavalry->InitialStrength));
    OwnerCavalry->Cohesion = FMath::Max(0.0f, OwnerCavalry->Cohesion - ImpactCohesionShock * float(CavalryLoss) / FMath::Max(1, OwnerCavalry->InitialStrength));
    if (ImpactFormation == EStrategyFormationType::Square && Target->IsCombatEffective() && Target->FormationComponent &&
        (Target->Cohesion < SteadySquareCohesion || Target->Morale < SteadySquareMorale))
    { Target->FormationComponent->SetFormation(EStrategyFormationType::Line); }
    if (Target->CombatComponent) { Target->CombatComponent->EvaluateRoutState(); }
    if (OwnerCavalry->CombatComponent) { OwnerCavalry->CombatComponent->EvaluateRoutState(); }
    ChargeMomentum *= FMath::Clamp(1.0f - float(CavalryLoss) / FMath::Max(1, OwnerCavalry->InitialStrength), 0.0f, 1.0f);
}
