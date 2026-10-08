#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyFieldOfficerComponent.generated.h"

class AStrategyUnit;

/**
 * The officer in the field: within the order he has, he leads his unit himself. The captain of a company closes
 * to his fire distance on an attack, turns his front to the enemy on the defence, charges with the bayonet when
 * the enemy wavers close in, and falls back when his losses are too heavy; the cavalry officer charges what is
 * open to a charge (a column, a wavering line, an uncovered battery) and keeps off formed infantry; the battery
 * chief picks his own targets. A direct order of the player is carried out first. What he thinks is written to
 * the unit's AI telemetry (shown in the command panel).
 */
UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyFieldOfficerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyFieldOfficerComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Seconds between the officer's looks at the situation (longer for a slow officer, shorter for a good one). */
    UPROPERTY(EditAnywhere, Category="Strategy|Officer")
    float ThinkSeconds = 1.5f;

    /** Below this share of its men the unit falls back. */
    UPROPERTY(EditAnywhere, Category="Strategy|Officer")
    float BreakShare = 0.4f;

    UFUNCTION(BlueprintPure, Category="Strategy|Officer")
    bool IsCharging() const { return bCharging; }
    void SetDeterministicRandomSeed(int32 Seed);
    bool IsTakingFireCover() const { return bTakingFireCover; }
    bool IsStandingUpFromFireCover() const;
    void NotifyIncomingFire(bool bLongRange);
    void LeaveAutomaticFireCover();

    // Balance estimates: good officers react in 3 s, middling in 8 s, poor never.
    UPROPERTY(EditAnywhere, Category="Strategy|Officer|FireCover")
    float GoodFireCoverDelaySeconds = 3.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Officer|FireCover")
    float MiddlingFireCoverDelaySeconds = 8.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Officer|FireCover")
    float FireCoverReleaseSeconds = 10.0f;
    UPROPERTY(EditAnywhere, Category="Strategy|Officer|FireCover")
    float FireCoverStandUpSeconds = 3.0f;

    /** The flank plan for a company of a group that closes on an enemy (for the enemy's own battle AI too): the place it is to
     *  take at the fire distance Radius from him (or a waypoint round the base's line of fire); false when the leader's plan has
     *  it attack straight. OutMustMove: it is not at its place yet. */
    bool FlankPlan(AStrategyUnit* Enemy, float Radius, FVector& OutGoal, FString& OutNote, bool& bOutMustMove);

private:
    bool UpdateAutomaticLooseOrderUnderFire();
    bool bTakingFireCover = false;
    float FireCoverSince = -1.0f;
    float LastIncomingFireTime = -1000000.0f;
    float LastLongRangeFireTime = -1000000.0f;
    float FireCoverStandUpUntil = -1.0f;
    float FireCoverPreviousLateralSpacing = 75.0f;
    float FireCoverPreviousRankSpacing = 90.0f;
    void ThinkInfantry(AStrategyUnit* Enemy, float Distance);
    /** Companies of one battalion closing on the same enemy do not all walk at him and stand in each other's way: the middle
     *  one is the fire base (it halts at its fire distance and shoots), the others go round to the flanks, outside the base's
     *  line of fire, and take their places at an angle to the enemy so that they fire into his side. */
    void AssignFlanks(AStrategyUnit* Enemy);
    FVector ApproachGoal(AStrategyUnit* Enemy, float Range, FString& OutNote, FVector& OutFoe);
    FVector ApproachGoalAt(AStrategyUnit* Enemy, float Radius, FString& OutNote, FVector& OutFoe);
    float PreferredFraction() const;
    void ThinkCavalry(AStrategyUnit* Enemy, float Distance);
    void ThinkArtillery(AStrategyUnit* Enemy, float Distance);
    void UpdateBayonetCharge();
    void ResolveShock(AStrategyUnit* Enemy);
    void StartCharge(AStrategyUnit* Target, const FString& Why);
    void EndCharge();
    bool FallBack(const FString& Why, float DistanceCm);
    void Decide(const FString& Task, const FString& Reason);

    /** Kind: 0 any, 1 foot and guns only (no cavalry), 2 cavalry only. */
    AStrategyUnit* NearestEnemy(float& OutDistance, float MaxCm, int32 Kind = 0) const;
    bool IsWavering(const AStrategyUnit* Unit) const;
    bool IsOpenToCharge(const AStrategyUnit* Unit, FString& OutWhy) const;
    bool IsOffensive() const;
    bool PlayerOrderUnderWay() const;
    float Aggression() const;

    UPROPERTY() TObjectPtr<AStrategyUnit> OwnerUnit;
    TWeakObjectPtr<AStrategyUnit> ChargeTarget;
    FRandomStream DecisionRandom;
    float ReserveHeldSince = -1.0f;
    bool bReserveReleased = false;
    float Accumulator = 0.0f;
    float NextOfficerDebugLogTime = 0.0f;
    int32 FlankRole = 0;          // 0 none, 1 fire base, 2 flank
    int32 FlankK = 0;             // how many places from the base
    float FlankSign = 0.0f;       // -1 / +1: which side of the base
    float FlankUntil = 0.0f;
    float FlankSince = 0.0f;      // when the plan was made (a reserve is put in after a time at the latest)
    float FlankSkill = 0.5f;      // the battalion leader's grasp of it (0-1)
    TWeakObjectPtr<AStrategyUnit> FlankEnemy;
    TWeakObjectPtr<AStrategyUnit> FlankBase;
    float LastChargeTime = -1000.0f;
    float LastFallBackTime = -1000.0f;
    float BayonetUntil = 0.0f;
    float NormalSpeed = 0.0f;
    bool bCharging = false;
    bool bArtilleryAuto = false;
};
