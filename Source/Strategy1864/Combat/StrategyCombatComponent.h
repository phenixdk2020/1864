#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyCombatComponent.generated.h"

class AStrategyUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FStrategyVolleyResolved,
    AStrategyUnit*,
    Target,
    int32,
    Shots,
    int32,
    Hits);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FStrategyVolleyVisualEvent,
    FVector,
    Origin,
    FVector,
    Direction,
    int32,
    Shots,
    int32,
    Hits);

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyCombatComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintAssignable, Category="Strategy|Combat")
    FStrategyVolleyResolved OnVolleyResolved;

    UPROPERTY(BlueprintAssignable, Category="Strategy|Presentation")
    FStrategyVolleyVisualEvent OnVolleyVisualEvent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    int32 AmmunitionRounds = 11400; // Initialized to men * carrying capacity in BeginPlay.
    UPROPERTY(EditAnywhere, Category="Strategy|Combat")
    float CartridgesCapacityPerMan = 60.0f;

    /** The enemy men this unit has hit in the battle (for its service record in the campaign). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Combat")
    int32 TotalHitsInflicted = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    int32 MaxAmmunitionRounds = 11400;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Combat")
    bool bOutOfAmmo = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float ReloadSeconds = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float BaseHitChance = 0.035f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    int32 MaxShotsPerVolley = 190;

    /** The target of the last volley and how many of the company could bear on it (angle and range). */
    UPROPERTY(Transient, BlueprintReadOnly, Category="Strategy|Combat")
    TObjectPtr<AStrategyUnit> LastVolleyTarget;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Strategy|Combat")
    int32 LastBearingCount = 0;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Strategy|Combat")
    int32 LastBearingTotal = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Combat")
    float ReloadRemainingSeconds = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Strategy|Combat")
    float LastFiredTimeSeconds = -1000000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float UnderFireDurationSeconds = 2.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Combat")
    float UnderFireRemainingSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float RoutMoraleThreshold = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float RoutCohesionThreshold = 10.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Combat")
    bool TryFireAt(AStrategyUnit* Target);

    UFUNCTION(BlueprintCallable, Category="Strategy|Combat")
    void NotifyIncomingVolley(int32 Hits, bool bLongRangeFire = false);

    UFUNCTION(BlueprintPure, Category="Strategy|Combat")
    bool IsReloading() const { return ReloadRemainingSeconds > 0.0f; }

    UFUNCTION(BlueprintCallable, Category="Strategy|QA")
    void SetDeterministicRandomSeed(int32 Seed);

    UFUNCTION(BlueprintCallable, Category="Strategy|Combat")
    void ResupplyAmmunition(int32 Rounds);

    UFUNCTION(BlueprintPure, Category="Strategy|Combat")
    bool IsOutOfAmmo() const { return bOutOfAmmo; }

    // Balance estimates, deliberately tunable rather than historical casualty claims.
    UPROPERTY(EditAnywhere, Category="Strategy|Square")
    float SquareInfantryCasualtyMultiplier = 1.2f;
    UPROPERTY(EditAnywhere, Category="Strategy|Square")
    float SquareArtilleryCasualtyMultiplier = 1.5f;
    void CancelMarchUnderFire();
    void EvaluateRoutState();
    int32 ScaleIncomingCasualties(int32 Casualties, bool bArtillery) const;
    void ConfigureCartridgesPerMan(float CartridgesPerMan = 60.0f);

    // Shared by movement and HUD; does not depend on reload or formation readiness.
    AStrategyUnit* FindBestTarget(bool bRequireFireCone = true) const;

private:

    int32 ResolveHits(
        int32 ShotCount,
        float DistanceCm,
        const AStrategyUnit* Target);
    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    TWeakObjectPtr<AStrategyUnit> CachedNearestEnemy;
    float NearestEnemyRefreshSeconds = 0.0f;

    FRandomStream RandomStream;
};
