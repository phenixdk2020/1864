#pragma once

#include "CoreMinimal.h"
#include "../Units/StrategyUnit.h"
#include "StrategyArtilleryTypes.h"
#include "StrategyArtilleryBatteryUnit.generated.h"

class UStrategyArtilleryDeploymentComponent;
class UStrategyArtilleryAmmunitionComponent;
class UStrategyArtilleryFireMissionComponent;
class UStrategyArtilleryDamageComponent;
class UStrategyArtilleryCaptureComponent;
class UStrategyArtilleryTraverseComponent;
class UStrategyArtilleryRepairComponent;
class UStrategyArtilleryPositioningComponent;
class UStrategyArtilleryProjectilePresentationComponent;
class UStrategyArtilleryCrewAnimationComponent;

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyArtilleryBatteryUnit : public AStrategyUnit
{
    GENERATED_BODY()

public:
    AStrategyArtilleryBatteryUnit();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    FStrategyArtilleryGunProfile GunProfile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    int32 GunCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    int32 DisabledGunCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    int32 DestroyedGunCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    int32 CrewStrength = 72;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    int32 DriverStrength = 18;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    int32 HorseStrength = 48;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    int32 HorsesRequiredForFullMobility = 36;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    int32 DriversRequiredForFullMobility = 12;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    EStrategyArtilleryOwnershipState OwnershipState =
        EStrategyArtilleryOwnershipState::Operational;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryDeploymentComponent> DeploymentComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryAmmunitionComponent> ArtilleryAmmunitionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryFireMissionComponent> ArtilleryFireMissionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryDamageComponent> ArtilleryDamageComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryCaptureComponent> ArtilleryCaptureComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryTraverseComponent> ArtilleryTraverseComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryRepairComponent> ArtilleryRepairComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryPositioningComponent> ArtilleryPositioningComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryProjectilePresentationComponent> ProjectilePresentationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    TObjectPtr<UStrategyArtilleryCrewAnimationComponent> CrewAnimationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    int32 PersonnelEvacuatedOnAbandon = 0;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    int32 GetPhysicallyAvailableGunCount() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    int32 GetCrewLimitedGunCount() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    int32 GetOperationalGunCount() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    float GetHorseMobilityFactor() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    float GetTowedMobilityFactor() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool CanNormalMove() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool CanFireBattery() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    bool AbandonBattery();

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    void ApplyBatteryDamage(
        int32 PersonnelLoss,
        int32 HorseLoss,
        int32 GunDisabled,
        int32 GunDestroyed);
};
