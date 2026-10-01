#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyArtilleryTypes.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyArtilleryFireMissionComponent.generated.h"

class AStrategyArtilleryBatteryUnit;
class AStrategyUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FStrategyArtilleryShotResolved,
    AStrategyUnit*,
    Target,
    EStrategyArtilleryAmmoType,
    AmmoType,
    int32,
    GunsFired,
    int32,
    Casualties);

DECLARE_MULTICAST_DELEGATE_FourParams(
    FStrategyArtilleryShotResolvedNative,
    AStrategyUnit*,
    EStrategyArtilleryAmmoType,
    int32,
    int32);

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryFireMissionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryFireMissionComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintAssignable, Category="Strategy|Artillery")
    FStrategyArtilleryShotResolved OnArtilleryShotResolved;

    // Native C++ event used by internal systems that do not require
    // Blueprint/reflection dispatch.
    FStrategyArtilleryShotResolvedNative OnArtilleryShotResolvedNative;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    EStrategyArtilleryFireMode FireMode =
        EStrategyArtilleryFireMode::ManualTarget;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<AStrategyUnit> ManualTarget;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    bool bHasManualAreaTarget = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    FVector ManualAreaTarget = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    float ManualAreaRadiusCm = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    EStrategyArtilleryTargetPriority TargetPriority =
        EStrategyArtilleryTargetPriority::Balanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    bool bAutoSelectAmmunition = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    bool bConserveAmmunition = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery", meta=(ClampMin="0.0", ClampMax="0.95"))
    float MinimumReserveFraction = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    int32 MaxSalvosPerMission = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float MaxMissionDurationSeconds = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    int32 MissionSalvosFired = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    float MissionElapsedSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float FiringFatiguePerGun = 0.20f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    float ReloadRemainingSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float EvaluationIntervalSeconds = 0.20f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    bool SetManualTarget(
        AStrategyUnit* Target,
        EStrategyOrderAuthority Authority);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    bool SetManualAreaTarget(
        const FVector& TargetLocation,
        float RadiusCm,
        EStrategyOrderAuthority Authority);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    void SetAutoTargetEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    void SetHoldFire(bool bHold);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    bool SelectAmmo(EStrategyArtilleryAmmoType AmmoType);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    void SetMissionLimits(int32 MaxSalvos, float MaxDurationSeconds);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    void SetConserveAmmunition(bool bConserve, float ReserveFraction);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    void SetTargetPriority(EStrategyArtilleryTargetPriority NewPriority)
    {
        TargetPriority = NewPriority;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    float GetMinimumRangeCm(
        EStrategyArtilleryAmmoType AmmoType) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    float GetMaximumRangeCm(
        EStrategyArtilleryAmmoType AmmoType) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool CanEngageTarget(const AStrategyUnit* Target) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool CanEngageLocation(const FVector& TargetLocation) const;

private:
    bool CanObserveTarget(const AStrategyUnit* Target) const;
    bool CanObserveLocation(const FVector& TargetLocation) const;
    bool HasUsableAmmoForTarget(const AStrategyUnit* Target) const;
    bool SelectBestAmmoForTarget(const AStrategyUnit* Target);
    bool ShouldStopForMissionLimit() const;
    void ResetMissionCounters();
    AStrategyUnit* FindBestAutoTarget() const;
    bool FireAt(AStrategyUnit* Target);
    bool FireAtLocation(const FVector& TargetLocation);
    int32 ResolveCasualties(
        AStrategyUnit* Target,
        int32 GunsFired,
        float DistanceCm,
        TArray<FVector>& OutImpactLocations,
        TArray<uint8>& OutHitFlags,
        TArray<int32>& OutCasualtiesPerProjectile);
    float CalculateTargetScore(const AStrategyUnit* Target) const;
    float GetBaseGunHitChance(
        EStrategyArtilleryAmmoType AmmoType) const;
    FIntPoint GetCasualtyRange(
        EStrategyArtilleryAmmoType AmmoType) const;
    void CompleteManualMission(bool bFailed);

    UPROPERTY()
    TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;

    EStrategyArtilleryFireMode PreviousNonHoldMode =
        EStrategyArtilleryFireMode::ManualTarget;

    float EvaluationAccumulator = 0.0f;
    FRandomStream RandomStream;
};
