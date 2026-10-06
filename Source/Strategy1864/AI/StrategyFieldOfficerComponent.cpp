#include "StrategyFieldOfficerComponent.h"

#include "StrategyAITelemetryComponent.h"
#include "StrategyDoctrineComponent.h"
#include "StrategyOfficerProfileComponent.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryFireMissionComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyUnit.h"
#include "../Visual/StrategyEquipmentVisualComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

UStrategyFieldOfficerComponent::UStrategyFieldOfficerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyFieldOfficerComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    // Each officer looks up at his own moment (not all on the same frame).
    Accumulator = FMath::FRandRange(0.0f, ThinkSeconds);
}

void UStrategyFieldOfficerComponent::Decide(const FString& Task, const FString& Reason)
{
    if (OwnerUnit && OwnerUnit->AITelemetryComponent)
    {
        OwnerUnit->AITelemetryComponent->SetDecision(Task, Reason);
    }
}

float UStrategyFieldOfficerComponent::Aggression() const
{
    float A = 50.0f;
    if (OwnerUnit && OwnerUnit->DoctrineComponent)
    {
        A = OwnerUnit->DoctrineComponent->GetEffectiveAggression(OwnerUnit->OfficerProfileComponent);
    }
    else if (OwnerUnit && OwnerUnit->OfficerProfileComponent)
    {
        A = OwnerUnit->OfficerProfileComponent->Aggression;
    }
    return A;
}

bool UStrategyFieldOfficerComponent::IsOffensive() const
{
    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        return false;
    }
    const EStrategyOrderType T = OwnerUnit->OrderComponent->GetCurrentOrder().Type;
    if (T == EStrategyOrderType::AttackHere || T == EStrategyOrderType::Advance || T == EStrategyOrderType::Charge)
    {
        return true;
    }
    // Without an order of attack, an offensive doctrine (or a hot-headed officer) seeks the fight all the same.
    return T == EStrategyOrderType::None && OwnerUnit->DoctrineComponent && OwnerUnit->DoctrineComponent->Doctrine == EStrategyDoctrine::Offensive;
}

bool UStrategyFieldOfficerComponent::PlayerOrderUnderWay() const
{
    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        return false;
    }
    const FStrategyOrder O = OwnerUnit->OrderComponent->GetCurrentOrder();
    return O.Authority == EStrategyOrderAuthority::DirectPlayer && OwnerUnit->OrderComponent->IsPhysicallyExecuting();
}

AStrategyUnit* UStrategyFieldOfficerComponent::NearestEnemy(float& OutDistance, float MaxCm, int32 Kind) const
{
    OutDistance = MaxCm;
    AStrategyUnit* Best = nullptr;
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* U = *It;
        if (!IsValid(U) || U == OwnerUnit || U->Side == EStrategySide::Neutral || U->Side == OwnerUnit->Side || !U->IsCombatEffective() ||
            U->Echelon == EStrategyEchelon::Headquarters || U->Echelon == EStrategyEchelon::Supply || U->Echelon == EStrategyEchelon::Battalion ||
            U->Echelon == EStrategyEchelon::Regiment || U->Echelon == EStrategyEchelon::Brigade || U->Echelon == EStrategyEchelon::Division ||
            (Kind == 1 && U->Echelon == EStrategyEchelon::Cavalry) || (Kind == 2 && U->Echelon != EStrategyEchelon::Cavalry))
        {
            continue;
        }
        const float D = FVector::Dist2D(U->GetActorLocation(), OwnerUnit->GetActorLocation());
        if (D < OutDistance)
        {
            OutDistance = D;
            Best = U;
        }
    }
    return Best;
}

bool UStrategyFieldOfficerComponent::IsWavering(const AStrategyUnit* U) const
{
    if (!U)
    {
        return false;
    }
    return U->Morale < 45.0f || U->Cohesion < 40.0f || U->CurrentStrength < U->InitialStrength * 0.6f ||
        U->UnitState == EStrategyUnitState::Routed || U->UnitState == EStrategyUnitState::Reforming;
}

bool UStrategyFieldOfficerComponent::IsOpenToCharge(const AStrategyUnit* U, FString& OutWhy) const
{
    if (!U)
    {
        return false;
    }
    const EStrategyFormationType F = U->FormationComponent ? U->FormationComponent->CurrentFormation : EStrategyFormationType::Line;
    if (F == EStrategyFormationType::Square)
    {
        return false;   // formed square: the horse breaks on it
    }
    if (U->Echelon == EStrategyEchelon::Artillery)
    {
        OutWhy = TEXT("batteriet står uden dækning");
        return true;
    }
    if (F == EStrategyFormationType::MarchColumn || F == EStrategyFormationType::DefileColumn)
    {
        OutWhy = TEXT("fjenden er i kolonne");
        return true;
    }
    if (U->UnitState == EStrategyUnitState::Routed)
    {
        OutWhy = TEXT("fjenden flygter");
        return true;
    }
    if (IsWavering(U))
    {
        OutWhy = TEXT("fjenden vakler");
        return true;
    }
    return false;
}

bool UStrategyFieldOfficerComponent::FallBack(const FString& Why, float DistanceCm)
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastFallBackTime < 45.0f || !OwnerUnit->OrderComponent)
    {
        return false;
    }
    float EnemyDistance = 0.0f;
    const AStrategyUnit* Enemy = NearestEnemy(EnemyDistance, 150000.0f);
    const FVector Away = Enemy ? (OwnerUnit->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D() : -OwnerUnit->GetActorForwardVector().GetSafeNormal2D();
    FStrategyOrder Order;
    Order.Type = EStrategyOrderType::Withdraw;
    Order.TargetLocation = OwnerUnit->GetActorLocation() + Away * DistanceCm;
    Order.FacingYaw = (-Away).Rotation().Yaw;
    Order.bHasFacing = true;
    Order.Authority = EStrategyOrderAuthority::OfficerAI;
    if (OwnerUnit->OrderComponent->SetOrder(Order))
    {
        LastFallBackTime = Now;
        EndCharge();
        Decide(TEXT("Trækker sig tilbage"), Why);
        return true;
    }
    return false;
}

void UStrategyFieldOfficerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!OwnerUnit || !OwnerUnit->bOfficerAIEnabled || !OwnerUnit->OrderComponent ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed || OwnerUnit->UnitState == EStrategyUnitState::Destroyed || !OwnerUnit->IsCombatEffective())
    {
        if (bCharging) { EndCharge(); }
        return;
    }
    if (OwnerUnit->Echelon != EStrategyEchelon::Company && OwnerUnit->Echelon != EStrategyEchelon::Cavalry && OwnerUnit->Echelon != EStrategyEchelon::Artillery)
    {
        return;
    }
    // The charge is watched every frame (the shock comes when the lines meet).
    if (bCharging && OwnerUnit->Echelon == EStrategyEchelon::Company)
    {
        UpdateBayonetCharge();
    }
    if (!bCharging && BayonetUntil > 0.0f && GetWorld()->GetTimeSeconds() > BayonetUntil && OwnerUnit->EquipmentVisualComponent)
    {
        OwnerUnit->EquipmentVisualComponent->SetBayonetFixed(false);
        BayonetUntil = 0.0f;
    }
    // A good officer looks more often; one under stress less clearly.
    float Interval = ThinkSeconds;
    if (OwnerUnit->OfficerProfileComponent)
    {
        Interval *= FMath::Lerp(1.4f, 0.7f, OwnerUnit->OfficerProfileComponent->TacticalSkill / 100.0f);
    }
    Accumulator += DeltaTime;
    if (Accumulator < Interval)
    {
        return;
    }
    Accumulator = 0.0f;
    float Distance = 0.0f;
    AStrategyUnit* Enemy = NearestEnemy(Distance, 150000.0f);
    switch (OwnerUnit->Echelon)
    {
    case EStrategyEchelon::Company: ThinkInfantry(Enemy, Distance); break;
    case EStrategyEchelon::Cavalry: ThinkCavalry(Enemy, Distance); break;
    default: ThinkArtillery(Enemy, Distance); break;
    }
}

void UStrategyFieldOfficerComponent::ThinkInfantry(AStrategyUnit* Enemy, float Distance)
{
    // Heavy losses or a broken spirit: back out of the fire, to rally.
    if (OwnerUnit->CurrentStrength < OwnerUnit->InitialStrength * BreakShare || OwnerUnit->Morale < 22.0f)
    {
        FallBack(FString::Printf(TEXT("Svære tab: %d af %d mand, moral %.0f"), OwnerUnit->CurrentStrength, OwnerUnit->InitialStrength, OwnerUnit->Morale), 25000.0f);
        return;
    }
    if (bCharging || PlayerOrderUnderWay())
    {
        return;
    }
    if (!Enemy)
    {
        Decide(TEXT("Afventer"), TEXT("Ingen fjende i syne"));
        return;
    }
    const bool bEnemySide = OwnerUnit->Side != EStrategySide::Denmark;
    const float Range = OwnerUnit->FireControlComponent ? FMath::Max(3000.0f, OwnerUnit->FireControlComponent->GetActiveRangeCm()) : 15000.0f;
    const float Now = GetWorld()->GetTimeSeconds();
    // The bayonet: the enemy wavers within ninety metres, the men are steady, the officer is willing.
    const float ChargeReach = FMath::Lerp(6000.0f, 11000.0f, Aggression() / 100.0f);
    if ((IsOffensive() || bEnemySide) && Distance < ChargeReach && IsWavering(Enemy) && OwnerUnit->Morale > 50.0f &&
        Now - LastChargeTime > 30.0f && Enemy->Echelon != EStrategyEchelon::Cavalry)
    {
        StartCharge(Enemy, FString::Printf(TEXT("%s vakler %.0f m borte: fæld bajonet!"), *Enemy->DisplayName.ToString(), Distance / 100.0f));
        return;
    }
    // The enemy's own companies are led to their range by his battle AI; the Danish captain closes himself.
    if (bEnemySide)
    {
        return;
    }
    // Horse close by: the company stands in its formation (the square against cavalry) and does not walk at it.
    float HorseDistance = 0.0f;
    const AStrategyUnit* Horse = NearestEnemy(HorseDistance, 15000.0f, 2);
    if (Horse && !IsWavering(Horse))
    {
        Decide(TEXT("Holder formationen"), FString::Printf(TEXT("Fjendtligt rytteri %.0f m borte (%s)"), HorseDistance / 100.0f, *Horse->DisplayName.ToString()));
        return;
    }
    // The objective of an advance is the enemy's foot or guns.
    {
        float FootDistance = 0.0f;
        if (AStrategyUnit* Foot = NearestEnemy(FootDistance, 150000.0f, 1))
        {
            Enemy = Foot;
            Distance = FootDistance;
        }
    }
    const FStrategyOrder Order = OwnerUnit->OrderComponent->GetCurrentOrder();
    const bool bMoving = OwnerUnit->OrderComponent->IsPhysicallyExecuting();
    if (IsOffensive() && Distance > Range * 0.95f && !bMoving)
    {
        // On to the fire distance, straight at the enemy, but not far beyond the ground he was sent to.
        const FVector Away = (OwnerUnit->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
        FVector Goal = Enemy->GetActorLocation() + Away * Range * 0.8f;
        if (Order.IsValidOrder() && FVector::Dist2D(Goal, Order.TargetLocation) > 40000.0f)
        {
            Goal = Order.TargetLocation + (Goal - Order.TargetLocation).GetSafeNormal2D() * 40000.0f;
        }
        FStrategyOrder Advance;
        Advance.Type = EStrategyOrderType::Advance;
        Advance.TargetLocation = Goal;
        Advance.FacingYaw = (Enemy->GetActorLocation() - Goal).Rotation().Yaw;
        Advance.bHasFacing = true;
        Advance.Authority = EStrategyOrderAuthority::OfficerAI;
        if (OwnerUnit->OrderComponent->SetOrder(Advance))
        {
            Decide(TEXT("Rykker frem"), FString::Printf(TEXT("%s er %.0f m borte; vi skyder på %.0f m"), *Enemy->DisplayName.ToString(), Distance / 100.0f, Range / 100.0f));
        }
        return;
    }
    if (!bMoving && Distance < 50000.0f)
    {
        // The front to the enemy: a line fires only ahead.
        const float ToEnemy = (Enemy->GetActorLocation() - OwnerUnit->GetActorLocation()).Rotation().Yaw;
        const float Off = FMath::Abs(FMath::FindDeltaAngleDegrees(OwnerUnit->GetActorRotation().Yaw, ToEnemy));
        if (Off > 45.0f)
        {
            FStrategyOrder Hold;
            Hold.Type = EStrategyOrderType::Hold;
            Hold.TargetLocation = OwnerUnit->GetActorLocation();
            Hold.FacingYaw = ToEnemy;
            Hold.bHasFacing = true;
            Hold.Authority = EStrategyOrderAuthority::OfficerAI;
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FRONT: %s yaw %.0f to enemy %.0f (%s at %s, me at %s)"), *OwnerUnit->DisplayName.ToString(),
                OwnerUnit->GetActorRotation().Yaw, ToEnemy, *Enemy->DisplayName.ToString(), *Enemy->GetActorLocation().ToCompactString(), *OwnerUnit->GetActorLocation().ToCompactString());
            if (OwnerUnit->OrderComponent->SetOrder(Hold))
            {
                Decide(TEXT("Svinger fronten"), FString::Printf(TEXT("%s kommer fra siden (%.0f°)"), *Enemy->DisplayName.ToString(), Off));
            }
            return;
        }
        Decide(Distance <= Range ? TEXT("Skyder") : TEXT("Holder stillingen"),
            FString::Printf(TEXT("%s %.0f m borte"), *Enemy->DisplayName.ToString(), Distance / 100.0f));
    }
}

void UStrategyFieldOfficerComponent::ThinkCavalry(AStrategyUnit* Enemy, float Distance)
{
    ACavalryUnit* Cavalry = Cast<ACavalryUnit>(OwnerUnit);
    if (OwnerUnit->CurrentStrength < OwnerUnit->InitialStrength * 0.45f || OwnerUnit->Morale < 25.0f)
    {
        FallBack(FString::Printf(TEXT("Eskadronen er slået: %d af %d ryttere"), OwnerUnit->CurrentStrength, OwnerUnit->InitialStrength), 40000.0f);
        return;
    }
    const FStrategyOrder Order = OwnerUnit->OrderComponent->GetCurrentOrder();
    if (Order.Type == EStrategyOrderType::Charge && OwnerUnit->OrderComponent->IsPhysicallyExecuting())
    {
        return;
    }
    if (PlayerOrderUnderWay() || !Enemy)
    {
        return;
    }
    const float Now = GetWorld()->GetTimeSeconds();
    // A target open to a charge within six hundred metres (the nearest such, not just the nearest enemy).
    AStrategyUnit* Target = nullptr;
    FString Why;
    float Best = FMath::Lerp(40000.0f, 70000.0f, Aggression() / 100.0f);
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* U = *It;
        FString W;
        if (!IsValid(U) || U->Side == OwnerUnit->Side || U->Side == EStrategySide::Neutral || !U->IsCombatEffective() ||
            (U->Echelon != EStrategyEchelon::Company && U->Echelon != EStrategyEchelon::Artillery && U->Echelon != EStrategyEchelon::Cavalry))
        {
            continue;
        }
        const float D = FVector::Dist2D(U->GetActorLocation(), OwnerUnit->GetActorLocation());
        if (D < Best && IsOpenToCharge(U, W))
        {
            Best = D;
            Target = U;
            Why = W;
        }
    }
    if (Target && OwnerUnit->Morale > 55.0f && Now - LastChargeTime > 45.0f && Cavalry)
    {
        FStrategyOrder Charge;
        Charge.Type = EStrategyOrderType::Charge;
        Charge.TargetLocation = Target->GetActorLocation();
        Charge.FacingYaw = (Target->GetActorLocation() - OwnerUnit->GetActorLocation()).Rotation().Yaw;
        Charge.bHasFacing = true;
        Charge.Authority = EStrategyOrderAuthority::OfficerAI;
        if (OwnerUnit->OrderComponent->SetOrder(Charge))
        {
            LastChargeTime = Now;
            Decide(TEXT("Chok!"), FString::Printf(TEXT("%s: %s (%.0f m)"), *Target->DisplayName.ToString(), *Why, Best / 100.0f));
        }
        return;
    }
    // Formed infantry close by: out of its fire (a squadron cannot stand against a line or a square).
    if (Enemy->Echelon == EStrategyEchelon::Company && Distance < 25000.0f && !IsWavering(Enemy))
    {
        FallBack(FString::Printf(TEXT("Holder afstand til formeret fodfolk (%s, %.0f m)"), *Enemy->DisplayName.ToString(), Distance / 100.0f), 20000.0f);
        return;
    }
    Decide(TEXT("Lurer"), FString::Printf(TEXT("Venter på en åbning; nærmeste fjende %.0f m"), Distance / 100.0f));
}

void UStrategyFieldOfficerComponent::ThinkArtillery(AStrategyUnit* Enemy, float Distance)
{
    AStrategyArtilleryBatteryUnit* Battery = Cast<AStrategyArtilleryBatteryUnit>(OwnerUnit);
    if (!Battery || !Battery->ArtilleryFireMissionComponent)
    {
        return;
    }
    if (!bArtilleryAuto)
    {
        // The battery chief chooses his own targets (and his ammunition: case shot at close range).
        Battery->ArtilleryFireMissionComponent->SetAutoTargetEnabled(true);
        bArtilleryAuto = true;
    }
    if (Enemy)
    {
        Decide(Distance < 40000.0f ? TEXT("Kardæsk!") : TEXT("Skyder"), FString::Printf(TEXT("%s %.0f m borte"), *Enemy->DisplayName.ToString(), Distance / 100.0f));
    }
}

void UStrategyFieldOfficerComponent::StartCharge(AStrategyUnit* Target, const FString& Why)
{
    if (!OwnerUnit->OrderComponent)
    {
        return;
    }
    FStrategyOrder Charge;
    Charge.Type = EStrategyOrderType::Charge;
    Charge.TargetLocation = Target->GetActorLocation();
    Charge.FacingYaw = (Target->GetActorLocation() - OwnerUnit->GetActorLocation()).Rotation().Yaw;
    Charge.bHasFacing = true;
    Charge.Authority = EStrategyOrderAuthority::OfficerAI;
    if (!OwnerUnit->OrderComponent->SetOrder(Charge))
    {
        return;
    }
    ChargeTarget = Target;
    bCharging = true;
    LastChargeTime = GetWorld()->GetTimeSeconds();
    if (OwnerUnit->MovementExecutor)
    {
        NormalSpeed = OwnerUnit->MovementExecutor->MoveSpeedCmPerSecond;
        OwnerUnit->MovementExecutor->MoveSpeedCmPerSecond = NormalSpeed * 1.9f;   // at the double
    }
    if (OwnerUnit->EquipmentVisualComponent)
    {
        OwnerUnit->EquipmentVisualComponent->SetBayonetFixed(true);
    }
    Decide(TEXT("Bajonetangreb"), Why);
}

void UStrategyFieldOfficerComponent::EndCharge()
{
    if (!bCharging)
    {
        return;
    }
    bCharging = false;
    if (OwnerUnit && OwnerUnit->MovementExecutor && NormalSpeed > 0.0f)
    {
        OwnerUnit->MovementExecutor->MoveSpeedCmPerSecond = NormalSpeed;
    }
    BayonetUntil = GetWorld()->GetTimeSeconds() + 30.0f;
}

void UStrategyFieldOfficerComponent::UpdateBayonetCharge()
{
    AStrategyUnit* Target = ChargeTarget.Get();
    const FStrategyOrder Order = OwnerUnit->OrderComponent->GetCurrentOrder();
    if (!IsValid(Target) || !Target->IsCombatEffective() || Order.Type != EStrategyOrderType::Charge)
    {
        EndCharge();
        return;
    }
    // Follow the enemy if he moves; strike when the lines meet.
    const float D = FVector::Dist2D(Target->GetActorLocation(), OwnerUnit->GetActorLocation());
    if (D < 1800.0f)
    {
        ResolveShock(Target);
        return;
    }
    if (FVector::Dist2D(Order.TargetLocation, Target->GetActorLocation()) > 1500.0f)
    {
        FStrategyOrder Chase = Order;
        Chase.TargetLocation = Target->GetActorLocation();
        OwnerUnit->OrderComponent->SetOrder(Chase);
    }
}

void UStrategyFieldOfficerComponent::ResolveShock(AStrategyUnit* Enemy)
{
    // Who gives way: numbers, spirit and order on each side (a square stands better; a line taken in flank worse).
    auto Weight = [](const AStrategyUnit* U)
    {
        float W = FMath::Max(1, U->CurrentStrength) * (U->Morale / 100.0f) * (0.7f + U->Cohesion / 300.0f);
        if (U->FormationComponent && U->FormationComponent->CurrentFormation == EStrategyFormationType::Square) { W *= 1.25f; }
        return W;
    };
    const float Ratio = FMath::Clamp(Weight(OwnerUnit) / Weight(Enemy), 0.4f, 2.5f);
    const int32 EnemyLoss = FMath::RoundToInt(OwnerUnit->CurrentStrength * FMath::FRandRange(0.05f, 0.10f) * Ratio);
    const int32 OwnLoss = FMath::RoundToInt(Enemy->CurrentStrength * FMath::FRandRange(0.03f, 0.07f) / Ratio);
    Enemy->ApplyStrengthLoss(EnemyLoss);
    OwnerUnit->ApplyStrengthLoss(OwnLoss);
    Enemy->Morale = FMath::Clamp(Enemy->Morale - 20.0f * Ratio, 0.0f, 100.0f);
    Enemy->Cohesion = FMath::Clamp(Enemy->Cohesion - 25.0f * Ratio, 0.0f, 100.0f);
    const bool bWon = Ratio > 1.15f || Enemy->Morale < 30.0f;
    if (bWon)
    {
        // The enemy breaks and runs.
        Enemy->SetUnitState(EStrategyUnitState::Routed);
        if (Enemy->OrderComponent)
        {
            const FVector Away = (Enemy->GetActorLocation() - OwnerUnit->GetActorLocation()).GetSafeNormal2D();
            FStrategyOrder Flee;
            Flee.Type = EStrategyOrderType::Withdraw;
            Flee.TargetLocation = Enemy->GetActorLocation() + Away * 30000.0f;
            Flee.Authority = EStrategyOrderAuthority::OfficerAI;
            Enemy->OrderComponent->SetOrder(Flee);
        }
        OwnerUnit->Morale = FMath::Clamp(OwnerUnit->Morale + 8.0f, 0.0f, 100.0f);
    }
    else
    {
        OwnerUnit->Morale = FMath::Clamp(OwnerUnit->Morale - 15.0f, 0.0f, 100.0f);
        OwnerUnit->Cohesion = FMath::Clamp(OwnerUnit->Cohesion - 15.0f, 0.0f, 100.0f);
    }
    // The charge is over: the company halts where it struck and re-forms.
    FStrategyOrder Halt;
    Halt.Type = EStrategyOrderType::Hold;
    Halt.TargetLocation = OwnerUnit->GetActorLocation();
    Halt.FacingYaw = (Enemy->GetActorLocation() - OwnerUnit->GetActorLocation()).Rotation().Yaw;
    Halt.bHasFacing = true;
    Halt.Authority = EStrategyOrderAuthority::OfficerAI;
    OwnerUnit->OrderComponent->SetOrder(Halt);
    EndCharge();
    Decide(bWon ? TEXT("Bajonetangrebet lykkedes") : TEXT("Bajonetangrebet slået tilbage"),
        FString::Printf(TEXT("%s mistede %d, vi %d"), *Enemy->DisplayName.ToString(), EnemyLoss, OwnLoss));
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-BAYONET: %s -> %s ratio %.2f enemy -%d own -%d %s"), *OwnerUnit->DisplayName.ToString(), *Enemy->DisplayName.ToString(),
        Ratio, EnemyLoss, OwnLoss, bWon ? TEXT("won") : TEXT("repulsed"));
}
