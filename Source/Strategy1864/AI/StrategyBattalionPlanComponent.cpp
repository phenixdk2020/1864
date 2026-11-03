#include "StrategyBattalionPlanComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyContactComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Units/StrategyUnit.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

UStrategyBattalionPlanComponent::UStrategyBattalionPlanComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f;
}

void UStrategyBattalionPlanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (GetWorld() && GetWorld()->GetTimeSeconds() >= NextEvaluateTime)
    {
        Evaluate();
    }
}

float UStrategyBattalionPlanComponent::FightingValue(const AStrategyUnit* Unit)
{
    if (!IsValid(Unit) || !Unit->IsCombatEffective()) return 0.0f;
    // Men, steadied by morale; a horseman and a gun count for more than a musket in the open.
    const float ArmWeight = Unit->Echelon == EStrategyEchelon::Cavalry ? 1.3f : Unit->Echelon == EStrategyEchelon::Artillery ? 2.0f : 1.0f;
    return float(Unit->CurrentStrength) * (0.5f + 0.5f * FMath::Clamp(Unit->Morale, 0.0f, 100.0f) / 100.0f) * ArmWeight;
}

float UStrategyBattalionPlanComponent::ThreatScore(const AStrategyUnit* Enemy, const FVector& From)
{
    if (!IsValid(Enemy)) return 0.0f;
    FVector EnemyFront = Enemy->GetActorForwardVector();
    EnemyFront.Z = 0.0f;
    const FVector ToUs = (From - Enemy->GetActorLocation()).GetSafeNormal2D();
    const float Facing = FVector::DotProduct(EnemyFront.GetSafeNormal(), ToUs) > 0.5f ? 1.25f : 1.0f;   // turned towards us: he can fire on us now
    const float Distance = FVector::Dist2D(From, Enemy->GetActorLocation());
    return FightingValue(Enemy) * Facing / (1.0f + Distance / 60000.0f);
}

void UStrategyBattalionPlanComponent::GatherUnits(TArray<AStrategyUnit*>& OutCompanies, TArray<AStrategyUnit*>& OutSquadrons) const
{
    const AStrategyUnit* Hq = Cast<AStrategyUnit>(GetOwner());
    if (!Hq) return;
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* PlanUnit = *It;
        if (!IsValid(PlanUnit) || PlanUnit->Side != Hq->Side || !PlanUnit->IsCombatEffective() || !PlanUnit->CommandComponent ||
            PlanUnit->CommandComponent->CurrentCommandParent != Hq) continue;
        if (PlanUnit->Echelon == EStrategyEchelon::Company) OutCompanies.Add(PlanUnit);
        else if (PlanUnit->Echelon == EStrategyEchelon::Cavalry) OutSquadrons.Add(PlanUnit);
    }
}

void UStrategyBattalionPlanComponent::Evaluate()
{
    UWorld* World = GetWorld();
    const AStrategyUnit* Hq = Cast<AStrategyUnit>(GetOwner());
    if (!World || !Hq) return;
    const float Now = World->GetTimeSeconds();
    NextEvaluateTime = Now + EvaluateIntervalSeconds;

    TArray<AStrategyUnit*> PlanCompanies, PlanSquadrons;
    GatherUnits(PlanCompanies, PlanSquadrons);
    if (PlanCompanies.IsEmpty()) { bHasLine = false; return; }

    // Only what his units have seen lately (fog of war), never the enemy's true strength. A contact counts for SeenMemorySeconds
    // after it was last seen, so a man lost from sight for a scan or behind smoke does not make the enemy vanish from the plan.
    TSet<AStrategyUnit*> PlanSeen;
    TArray<AStrategyUnit*> PlanObservers = PlanCompanies;
    PlanObservers.Append(PlanSquadrons);
    for (AStrategyUnit* PlanOwn : PlanObservers)
    {
        if (!PlanOwn->ContactComponent) continue;
        for (const FStrategyContactRecord& PlanContact : PlanOwn->ContactComponent->GetKnownContacts())
        {
            AStrategyUnit* PlanFoe = PlanContact.ObservedUnit.Get();
            if ((PlanContact.bCurrentlyVisible || PlanContact.SecondsSinceSeen <= SeenMemorySeconds) && IsValid(PlanFoe) && PlanFoe->Side != Hq->Side &&
                PlanFoe->Side != EStrategySide::Neutral && PlanFoe->IsCombatEffective())
                PlanSeen.Add(PlanFoe);
        }
    }

    float PlanOwnValue = 0.0f, PlanEnemyValue = 0.0f;
    LineCentre = FVector::ZeroVector;
    for (AStrategyUnit* PlanOwn : PlanCompanies) { PlanOwnValue += FightingValue(PlanOwn); LineCentre += PlanOwn->GetActorLocation(); }
    for (AStrategyUnit* PlanOwn : PlanSquadrons) { PlanOwnValue += FightingValue(PlanOwn); }
    LineCentre /= float(PlanCompanies.Num());

    AStrategyUnit* PlanBest = nullptr;
    float PlanBestThreat = 0.0f;
    bEnemyCavalryNear = false;
    for (AStrategyUnit* PlanFoe : PlanSeen)
    {
        PlanEnemyValue += FightingValue(PlanFoe);
        const float PlanThreat = ThreatScore(PlanFoe, LineCentre);
        if (PlanThreat > PlanBestThreat) { PlanBestThreat = PlanThreat; PlanBest = PlanFoe; }
        if (PlanFoe->Echelon == EStrategyEchelon::Cavalry && FVector::Dist2D(PlanFoe->GetActorLocation(), LineCentre) < 50000.0f) bEnemyCavalryNear = true;
    }
    MainTarget = PlanBest;
    StrengthRatio = PlanEnemyValue > 0.0f ? PlanOwnValue / PlanEnemyValue : 0.0f;

    if (PlanBest)
    {
        TowardEnemy = (PlanBest->GetActorLocation() - LineCentre).GetSafeNormal2D();
        const FVector PlanSide(-TowardEnemy.Y, TowardEnemy.X, 0.0f);
        LineHalfWidth = 0.0f;
        for (AStrategyUnit* PlanOwn : PlanCompanies)
            LineHalfWidth = FMath::Max(LineHalfWidth, FMath::Abs(FVector::DotProduct(PlanOwn->GetActorLocation() - LineCentre, PlanSide)));
        bHasLine = true;
        // A threat to a wing: an enemy beyond the end of the line (60 m out) and level with or behind its front, within 500 m.
        bWingThreat = false;
        for (const AStrategyUnit* PlanFoe : PlanSeen)
        {
            const FVector PlanRel = PlanFoe->GetActorLocation() - LineCentre;
            if (PlanFoe != PlanBest && PlanRel.Size2D() < 50000.0f && FMath::Abs(FVector::DotProduct(PlanRel, PlanSide)) > LineHalfWidth + 6000.0f &&
                FVector::DotProduct(PlanRel, TowardEnemy) < 20000.0f)
                bWingThreat = true;
        }
    }
    else
    {
        bWingThreat = false;
    }

    // A reserve is always kept by a battalion of four companies or more (British Brigade Drill 1862 p.147, Prussian Taktik 1867
    // p.142, see Docs/Taktik-Doktrin-1825-1864.md); the odds he sees only decide how soon it goes in (GetReserveReleaseShare).
    const bool bNewHold = PlanCompanies.Num() >= 4 && PlanBest;
    if (bNewHold != bHoldReserve)
    {
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-RESERVE %s %s grund=%s ratio=%.2f egne=%.0f set-fjende=%.0f kompagnier=%d"), *Hq->DisplayName.ToString(),
            bNewHold ? TEXT("hold") : TEXT("ingen"), bNewHold ? TEXT("doktrin") : (PlanBest ? TEXT("under-4-kompagnier") : TEXT("ingen-fjende-set")),
            StrengthRatio, PlanOwnValue, PlanEnemyValue, PlanCompanies.Num());
    }
    bHoldReserve = bNewHold;

    // The phase of the attack, from the main target's state.
    EStrategyBattalionPhase PlanPhase = EStrategyBattalionPhase::Approach;
    FString PlanWhy = TEXT("ingen i ildkamp");
    if (PlanBest && (PlanBest->UnitState == EStrategyUnitState::Routed || PlanBest->Morale < 35.0f))
    {
        PlanPhase = EStrategyBattalionPhase::Exploit;
        PlanWhy = FString::Printf(TEXT("%s vakler (moral %.0f)"), *PlanBest->DisplayName.ToString(), PlanBest->Morale);
    }
    else if (PlanBest && PlanBest->CombatComponent && PlanBest->CombatComponent->UnderFireRemainingSeconds > 0.0f)
    {
        PlanPhase = Phase == EStrategyBattalionPhase::Approach ? EStrategyBattalionPhase::Pin : EStrategyBattalionPhase::Flank;
        PlanWhy = FString::Printf(TEXT("%s er under ild"), *PlanBest->DisplayName.ToString());
    }
    if (PlanPhase != Phase)
    {
        static const TCHAR* PlanNames[] = { TEXT("naermer-sig"), TEXT("fastholder"), TEXT("omgaar"), TEXT("udnytter") };
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-BATTALION %s fase=%s grund=%s ratio=%.2f maal=%s"), *Hq->DisplayName.ToString(), PlanNames[uint8(PlanPhase)], *PlanWhy,
            StrengthRatio, PlanBest ? *PlanBest->DisplayName.ToString() : TEXT("-"));
        Phase = PlanPhase;
    }
}

bool UStrategyBattalionPlanComponent::GetCavalryWaitPoint(const AStrategyUnit* Squadron, FVector& OutPoint) const
{
    if (!bHasLine || !IsValid(Squadron)) return false;
    const FVector WaitSide(-TowardEnemy.Y, TowardEnemy.X, 0.0f);
    const float WaitSign = FVector::DotProduct(Squadron->GetActorLocation() - LineCentre, WaitSide) >= 0.0f ? 1.0f : -1.0f;
    // Beside the outer company and thirty metres back from the line, so he can ride round the enemy's flank from there.
    OutPoint = LineCentre + WaitSide * WaitSign * (LineHalfWidth + 9000.0f) - TowardEnemy * 3000.0f;
    // Out of musket range of the main target (about 300 m).
    if (const AStrategyUnit* WaitTarget = MainTarget.Get())
    {
        const FVector WaitFromTarget = OutPoint - WaitTarget->GetActorLocation();
        if (WaitFromTarget.Size2D() < CavalryWaitMinCm)
        {
            const FVector WaitAway = WaitFromTarget.GetSafeNormal2D().IsNearlyZero() ? -TowardEnemy : WaitFromTarget.GetSafeNormal2D();
            OutPoint = WaitTarget->GetActorLocation() + WaitAway * CavalryWaitMinCm;
        }
    }
    OutPoint.Z = Squadron->GetActorLocation().Z;
    return true;
}

bool UStrategyBattalionPlanComponent::GetRallyPoint(FVector& OutPoint) const
{
    if (!bHasLine) return false;
    OutPoint = LineCentre - TowardEnemy * RallyBehindCm;
    return true;
}

float UStrategyBattalionPlanComponent::GetReserveReleaseShare() const
{
    // With the odds (1.5 : 1 or better) he can wait for the decision: the reserve goes in when the fire base is down to 75 %.
    // Outnumbered (1 : 1 or worse) he must close a gap at once: already at 90 %. In between it slides. Shares are estimates.
    return FMath::GetMappedRangeValueClamped(FVector2D(1.0f, ReserveRatio), FVector2D(0.9f, 0.75f), StrengthRatio);
}
