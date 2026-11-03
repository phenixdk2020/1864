#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyBattalionPlanComponent.generated.h"

class AStrategyUnit;

UENUM()
enum class EStrategyBattalionPhase : uint8
{
    Approach,   // nobody in the fight yet
    Pin,        // the fire base holds the enemy under fire
    Flank,      // the flanks are going in on the pinned enemy
    Exploit     // the enemy wavers: follow up
};

// The battalion leader's view of the fight, on his HQ: what he can see of the enemy (only contacts his units have),
// his strength against it, whether he can afford a reserve, the phase of the attack, and where his squadron waits and rallies.
UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyBattalionPlanComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyBattalionPlanComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Own strength over the seen enemy's at which the reserve is held for the decision (put in late); outnumbered it goes in early.
    UPROPERTY(EditAnywhere, Category="Strategy|Battalion Plan")
    float ReserveRatio = 1.5f;

    // Doctrine distances (Docs/Taktik-Doktrin-1825-1864.md): a squadron waits at least 400 m from the enemy (an attack starts
    // 450-650 m out, Taktik 1867 p.210), and re-forms about 240 m behind the line (support distance, Cavalry Catechism p.107).
    UPROPERTY(EditAnywhere, Category="Strategy|Battalion Plan")
    float CavalryWaitMinCm = 40000.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Battalion Plan")
    float RallyBehindCm = 24000.0f;

    UPROPERTY(EditAnywhere, Category="Strategy|Battalion Plan")
    float EvaluateIntervalSeconds = 5.0f;

    // How long a contact counts in the plan after it was last seen (estimate, not from a source).
    UPROPERTY(EditAnywhere, Category="Strategy|Battalion Plan")
    float SeenMemorySeconds = 20.0f;

    void Evaluate();

    bool WantsReserve() const { return bHoldReserve; }
    EStrategyBattalionPhase GetPhase() const { return Phase; }
    AStrategyUnit* GetMainTarget() const { return MainTarget.Get(); }
    bool HasEnemyCavalryNear() const { return bEnemyCavalryNear; }
    // Enemy horse near, or a seen enemy beyond the end of the line: the reserve is wanted on that wing.
    bool HasWingThreat() const { return bEnemyCavalryNear || bWingThreat; }
    bool IsOutnumbered() const { return StrengthRatio > 0.0f && StrengthRatio < 1.0f; }
    float GetStrengthRatio() const { return StrengthRatio; }
    // The fire base's share of its men below which the reserve goes in: 0.75 with the odds, 0.9 when outnumbered.
    float GetReserveReleaseShare() const;

    // The squadron's place: beside the open flank of the battalion and back from its line, out of musket range.
    bool GetCavalryWaitPoint(const AStrategyUnit* Squadron, FVector& OutPoint) const;
    // Behind the middle of the battalion, where a squadron re-forms after a charge.
    bool GetRallyPoint(FVector& OutPoint) const;

    // The threat one seen enemy unit poses: strength, arm, morale, facing and distance.
    static float ThreatScore(const AStrategyUnit* Enemy, const FVector& From);
    // The fighting value of a unit, as the leader reckons it (the same scale for own and enemy).
    static float FightingValue(const AStrategyUnit* Unit);

private:
    void GatherUnits(TArray<AStrategyUnit*>& OutCompanies, TArray<AStrategyUnit*>& OutSquadrons) const;

    float NextEvaluateTime = 0.0f;
    bool bHoldReserve = false;
    bool bEnemyCavalryNear = false;
    bool bWingThreat = false;
    float StrengthRatio = 0.0f;
    EStrategyBattalionPhase Phase = EStrategyBattalionPhase::Approach;
    TWeakObjectPtr<AStrategyUnit> MainTarget;
    FVector LineCentre = FVector::ZeroVector;
    FVector TowardEnemy = FVector::ForwardVector;
    float LineHalfWidth = 0.0f;
    bool bHasLine = false;
};
