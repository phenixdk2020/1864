#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyArtilleryProjectileTypes.h"
#include "StrategyArtilleryProjectilePresentationComponent.generated.h"

class AStrategyArtilleryBatteryUnit;
class AStrategyArtilleryProjectilePresentation;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryProjectilePresentationComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryProjectilePresentationComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    int32 MaxVisibleProjectilesPerSalvo = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    int32 TrajectorySampleCount = 36;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    bool bEnableRoundShotRicochet = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    bool bDrawDebugTrajectory = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    float VirtualGunSpacingCm = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    float VirtualMuzzleForwardOffsetCm = 220.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    float VirtualMuzzleHeightCm = 145.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    int32 MaximumHistoryRecords = 64;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Projectile")
    TArray<FStrategyArtilleryProjectileHistoryRecord> RecentHistory;

    void PresentResolvedSalvo(
        EStrategyArtilleryAmmoType AmmoType,
        const FVector& AimLocation,
        const TArray<FVector>& ImpactLocations,
        const TArray<uint8>& HitFlags,
        const TArray<int32>& CasualtiesPerProjectile);

    UFUNCTION(BlueprintCallable, Category="Strategy|Projectile")
    void SetDebugTrajectoryEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category="Strategy|Projectile")
    bool IsDebugTrajectoryEnabled() const
    {
        return bDrawDebugTrajectory;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Projectile")
    AStrategyArtilleryProjectilePresentation* GetLatestActiveProjectile() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Projectile")
    int32 GetActiveProjectileCount() const;

private:
    FVector GetVirtualMuzzleLocation(
        int32 GunIndex,
        int32 VisibleCount) const;

    void AddHistory(
        const FStrategyArtilleryProjectileSpec& Spec,
        const FVector& FinalImpact);

    UPROPERTY()
    TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;

    UPROPERTY()
    TArray<TObjectPtr<AStrategyArtilleryProjectilePresentation>> ActiveProjectiles;

    int32 NextShotSerial = 1;
};
