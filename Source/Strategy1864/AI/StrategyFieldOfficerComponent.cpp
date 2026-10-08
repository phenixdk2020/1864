#include "StrategyFieldOfficerComponent.h"

#include "StrategyAITelemetryComponent.h"
#include "StrategyDoctrineComponent.h"
#include "StrategyOfficerProfileComponent.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryFireMissionComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyUnit.h"
#include "../Visual/StrategyEquipmentVisualComponent.h"
#include "Algo/Sort.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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
        if (!OwnerUnit->FormationComponent ||
            OwnerUnit->FormationComponent->CurrentFormation != EStrategyFormationType::Square)
        {
            OwnerUnit->EquipmentVisualComponent->SetBayonetFixed(false);
        }
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

void UStrategyFieldOfficerComponent::AssignFlanks(AStrategyUnit* Enemy)
{
    if (!OwnerUnit || !Enemy || !OwnerUnit->CommandComponent || !IsValid(OwnerUnit->CommandComponent->CurrentCommandParent))
    {
        return;
    }
    // The companies of the same staff that have the enemy within about 900 m.
    TArray<AStrategyUnit*> Group;
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* U = *It;
        if (IsValid(U) && U->Side == OwnerUnit->Side && U->Echelon == EStrategyEchelon::Company && U->IsCombatEffective() && U->CommandComponent &&
            U->CommandComponent->CurrentCommandParent == OwnerUnit->CommandComponent->CurrentCommandParent &&
            FVector::Dist2D(U->GetActorLocation(), Enemy->GetActorLocation()) < 90000.0f)
        {
            Group.Add(U);
        }
    }
    const float Now = GetWorld()->GetTimeSeconds();
    if (Group.Num() < 2)
    {
        FlankRole = 0;
        FlankUntil = Now + 10.0f;
        return;
    }
    // The battalion leader's grasp of the manoeuvre: tactical skill first, then initiative, staff work and boldness. A poor one has
    // them all walk straight at the enemy; a middling one flanks with the two next to the base only; a good one with all.
    float Skill = 0.5f;
    if (const UStrategyOfficerProfileComponent* Leader = OwnerUnit->CommandComponent->CurrentCommandParent->OfficerProfileComponent)
    {
        Skill = (Leader->TacticalSkill * 0.5f + Leader->Initiative * 0.25f + Leader->StaffQuality * 0.15f + Leader->Aggression * 0.10f) / 100.0f;
    }
    if (Skill < 0.35f)
    {
        for (AStrategyUnit* U : Group)
        {
            if (UStrategyFieldOfficerComponent* C = U->FindComponentByClass<UStrategyFieldOfficerComponent>())
            {
                C->FlankRole = 0;
                C->FlankUntil = Now + 60.0f;
                C->FlankSkill = Skill;
            }
        }
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FLANK: the leader of %s (grasp %.2f) has no plan: the companies go straight in"), *OwnerUnit->DisplayName.ToString(), Skill);
        return;
    }
    const int32 MaxPlaces = Skill < 0.55f ? 1 : 9;
    // A reserve: a cautious, level-headed leader with four companies or more keeps the rearmost back, behind the fire base, to
    // put in where it is needed (-Strategy1864HoldReserve forces it for a test).
    AStrategyUnit* Reserve = nullptr;
    {
        const UStrategyOfficerProfileComponent* Leader = OwnerUnit->CommandComponent->CurrentCommandParent->OfficerProfileComponent;
        const float Hold = Leader ? Leader->Caution * 0.4f + Leader->TacticalSkill * 0.3f - Leader->Aggression * 0.2f : 20.0f;
        if (Group.Num() >= 4 && (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864HoldReserve")) || Hold > 32.0f))
        {
            float Farthest = -1.0f;
            for (AStrategyUnit* U : Group)
            {
                const float D = FVector::Dist2D(U->GetActorLocation(), Enemy->GetActorLocation());
                if (D > Farthest) { Farthest = D; Reserve = U; }
            }
            Group.Remove(Reserve);
        }
    }
    FVector Centre = FVector::ZeroVector;
    for (const AStrategyUnit* U : Group) { Centre += U->GetActorLocation(); }
    Centre /= float(Group.Num());
    const FVector Back = (Centre - Enemy->GetActorLocation()).GetSafeNormal2D();
    const FVector Side(-Back.Y, Back.X, 0.0f);
    auto Lateral = [&](const AStrategyUnit* U) { return FVector::DotProduct(U->GetActorLocation() - Enemy->GetActorLocation(), Side); };
    Algo::Sort(Group, [&](const AStrategyUnit* A, const AStrategyUnit* B) { return Lateral(A) < Lateral(B); });
    // The base: the middle one (with two companies, the one nearer the line to the enemy).
    int32 Base = Group.Num() / 2;
    if (Group.Num() == 2 && FMath::Abs(Lateral(Group[0])) < FMath::Abs(Lateral(Group[1]))) { Base = 0; }
    for (int32 i = 0; i < Group.Num(); ++i)
    {
        UStrategyFieldOfficerComponent* C = Group[i]->FindComponentByClass<UStrategyFieldOfficerComponent>();
        if (!C) { continue; }
        C->FlankRole = i == Base ? 1 : 2;
        C->FlankK = FMath::Abs(i - Base);
        C->FlankSkill = Skill;
        // Beyond the places the leader plans for, or a captain who is poorly disciplined and does his own thing: straight in.
        const UStrategyOfficerProfileComponent* Captain = Group[i]->OfficerProfileComponent;
        if (C->FlankK > MaxPlaces || (Captain && Captain->Discipline < 40.0f && FMath::FRand() < 0.35f))
        {
            C->FlankRole = 0;
        }
        C->FlankSign = i < Base ? -1.0f : 1.0f;
        C->FlankUntil = Now + 90.0f;
        C->FlankEnemy = Enemy;
        C->FlankBase = Group[Base];
    }
    if (Reserve)
    {
        if (UStrategyFieldOfficerComponent* C = Reserve->FindComponentByClass<UStrategyFieldOfficerComponent>())
        {
            C->FlankRole = 3;
            C->FlankK = 0;
            C->FlankSkill = Skill;
            C->FlankUntil = Now + 90.0f;
            C->FlankSince = Now;
            C->FlankEnemy = Enemy;
            C->FlankBase = Group[Base];
        }
    }
    for (AStrategyUnit* U : Group)
    {
        if (UStrategyFieldOfficerComponent* C = U->FindComponentByClass<UStrategyFieldOfficerComponent>()) { C->FlankSince = Now; }
    }
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FLANK: %d companies against %s: %s is the fire base%s"), Group.Num() + (Reserve ? 1 : 0), *Enemy->DisplayName.ToString(), *Group[Base]->DisplayName.ToString(),
        Reserve ? *FString::Printf(TEXT(", %s in reserve"), *Reserve->DisplayName.ToString()) : TEXT(""));
}

float UStrategyFieldOfficerComponent::PreferredFraction() const
{
    // How close he likes to come, of his fire distance: the doctrine and his boldness (a bold officer closes in, a cautious one holds off).
    if (OwnerUnit && OwnerUnit->DoctrineComponent)
    {
        return FMath::Clamp(OwnerUnit->DoctrineComponent->GetPreferredEngagementRangeFraction(OwnerUnit->OfficerProfileComponent), 0.55f, 0.9f);
    }
    return 0.8f;
}

bool UStrategyFieldOfficerComponent::FlankPlan(AStrategyUnit* Enemy, float Radius, FVector& OutGoal, FString& OutNote, bool& bOutMustMove)
{
    FVector Foe = FVector::ZeroVector;
    OutGoal = ApproachGoalAt(Enemy, Radius, OutNote, Foe);
    bOutMustMove = (FlankRole == 2 || FlankRole == 3) && FVector::Dist2D(OwnerUnit->GetActorLocation(), OutGoal) > 2000.0f;
    return FlankRole != 0;
}

FVector UStrategyFieldOfficerComponent::ApproachGoal(AStrategyUnit* Enemy, float Range, FString& OutNote, FVector& OutFoe)
{
    return ApproachGoalAt(Enemy, Range * PreferredFraction(), OutNote, OutFoe);
}

FVector UStrategyFieldOfficerComponent::ApproachGoalAt(AStrategyUnit* Enemy, float Radius, FString& OutNote, FVector& OutFoe)
{
    const FVector Own = OwnerUnit->GetActorLocation();
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now > FlankUntil || !FlankEnemy.IsValid() || !FlankEnemy->IsCombatEffective())
    {
        AssignFlanks(Enemy);
    }
    // The whole group works on the same enemy company (the one the fire base faces).
    const AStrategyUnit* Foe = FlankRole != 0 && FlankEnemy.IsValid() ? FlankEnemy.Get() : Enemy;
    const FVector Target = Foe->GetActorLocation();
    OutFoe = Target;
    FVector Goal = Target + (Own - Target).GetSafeNormal2D() * Radius;
    if (FlankRole == 0)
    {
        return Goal;
    }
    // The axis from the enemy back to the company's own side; the base takes the place straight in front of him.
    const AStrategyUnit* Base = FlankBase.Get();
    const FVector BasePlace = Base ? Base->GetActorLocation() : Own;
    FVector Back = (BasePlace - Target).GetSafeNormal2D();
    if (FlankRole == 3)
    {
        // The reserve stands ninety metres behind the fire base, until the base is hurt, the time is up, or the base is gone.
        const bool bRelease = !Base || !Base->IsCombatEffective() || Base->CurrentStrength < Base->InitialStrength * 0.75f || Base->Morale < 55.0f || Now - FlankSince > 240.0f;
        if (bRelease)
        {
            FlankRole = 0;
            OutNote = TEXT("reserven sættes ind");
            return Target + (Own - Target).GetSafeNormal2D() * Radius;
        }
        OutNote = TEXT("i reserve");
        return BasePlace + Back * 9000.0f;
    }
    if (FlankRole == 1)
    {
        OutNote = TEXT("ildbasen: holder og skyder");
        return Target + Back * Radius;
    }
    // A flank: at an angle to the enemy (45 degrees for the next to the base, 90 for the second), at the same fire distance.
    const float Angle = FMath::Min(80.0f, 45.0f * FlankK) * FMath::Lerp(0.7f, 1.1f, FlankSkill);   // a good leader sends them wider round
    Goal = Target + Back.RotateAngleAxis(FlankSign * Angle, FVector::UpVector) * Radius;
    OutNote = TEXT("flanken: ind fra siden");
    // Not through the base's line of fire: when the way to the place crosses the cone from the base to the enemy, go round it first.
    if (Base && FlankSkill >= 0.5f)
    {
        const FVector Axis = (Target - BasePlace).GetSafeNormal2D();
        const float Length = FVector::Dist2D(BasePlace, Target) + 500.0f;
        const float HalfAngle = FMath::Cos(FMath::DegreesToRadians(32.0f));
        bool bCrosses = false;
        for (float T = 0.0f; T <= 1.0f && !bCrosses; T += 0.05f)
        {
            const FVector P = FMath::Lerp(Own, Goal, T);
            const FVector V = P - BasePlace;
            const float Along = FVector::DotProduct(V, Axis);
            bCrosses = Along > 0.0f && Along < Length && FVector::DotProduct(V.GetSafeNormal2D(), Axis) > HalfAngle;
        }
        if (bCrosses)
        {
            const FVector Round = BasePlace + Axis.RotateAngleAxis(-FlankSign * (46.0f + 14.0f * (FlankK - 1)), FVector::UpVector) * (Length * (0.6f + 0.12f * (FlankK - 1)));
            OutNote = TEXT("går udenom egen ild");
            return Round;
        }
    }
    return Goal;
}

void UStrategyFieldOfficerComponent::ThinkInfantry(AStrategyUnit* Enemy, float Distance)
{
    // Heavy losses or a broken spirit: back out of the fire, to rally.
    const float Nerve = OwnerUnit->OfficerProfileComponent ? OwnerUnit->OfficerProfileComponent->GetDecisionStability() : 0.5f;
    if (OwnerUnit->CurrentStrength < OwnerUnit->InitialStrength * FMath::Lerp(BreakShare + 0.12f, BreakShare - 0.10f, Nerve) || OwnerUnit->Morale < FMath::Lerp(30.0f, 16.0f, Nerve))
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
    if (IsOffensive() && !bMoving)
    {
        FString ApproachNote;
        FVector Foe = Enemy->GetActorLocation();
        FVector Goal = ApproachGoal(Enemy, Range, ApproachNote, Foe);
        // On to the fire distance, or (for a flank) to the place at an angle to the enemy, even when already within range.
        const bool bFlankToGo = (FlankRole == 2 || FlankRole == 3) && FVector::Dist2D(OwnerUnit->GetActorLocation(), Goal) > 2000.0f;
        if (Distance > Range * 0.95f || bFlankToGo)
        {
            if (Order.IsValidOrder() && FVector::Dist2D(Goal, Order.TargetLocation) > 40000.0f)
            {
                Goal = Order.TargetLocation + (Goal - Order.TargetLocation).GetSafeNormal2D() * 40000.0f;
            }
            FStrategyOrder Advance;
            Advance.Type = EStrategyOrderType::Advance;
            Advance.TargetLocation = Goal;
            Advance.FacingYaw = (Foe - Goal).Rotation().Yaw;
            Advance.bHasFacing = true;
            Advance.Authority = EStrategyOrderAuthority::OfficerAI;
            // A short move (under 90 m) with the enemy close ahead: sidestep, the front stays to him (no turning the back or the flank to the fire).
            if (FVector::Dist2D(OwnerUnit->GetActorLocation(), Goal) < 9000.0f && Distance < Range * 1.8f)
            {
                Advance.bKeepFacing = true;
                if (!ApproachNote.IsEmpty()) { ApproachNote += TEXT(" · sidetrin"); }
                else { ApproachNote = TEXT("sidetrin"); }
            }
            if (OwnerUnit->OrderComponent->SetOrder(Advance))
            {
                UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FLANK: %s goes to %.0f,%.0f (%s), enemy at %.0f,%.0f"), *OwnerUnit->DisplayName.ToString(), Goal.X, Goal.Y, *ApproachNote, Foe.X, Foe.Y);
                Decide(TEXT("Rykker frem"), FString::Printf(TEXT("%s er %.0f m borte; vi skyder på %.0f m%s%s"), *Enemy->DisplayName.ToString(), Distance / 100.0f, Range / 100.0f, ApproachNote.IsEmpty() ? TEXT("") : TEXT(" · "), *ApproachNote));
            }
            return;
        }
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
