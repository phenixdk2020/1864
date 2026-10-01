#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Visual/StrategyHumanVisualTypes.h"
#include "StrategyArtilleryTypes.h"
#include "StrategyArtilleryCrewAnimationComponent.generated.h"

class AStrategyArtilleryBatteryUnit;
class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryCrewAnimationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryCrewAnimationComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery|CrewAnimation")
    TArray<FStrategyArtilleryCrewStation> CrewStations;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery|CrewAnimation")
    EStrategyArtilleryCrewActivity CurrentBatteryActivity =
        EStrategyArtilleryCrewActivity::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|CrewAnimation")
    int32 VisualCrewStationsPerGun = 8;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery|CrewAnimation")
    float DrillElapsedSeconds = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery|CrewAnimation")
    float DrillDurationSeconds = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|CrewAnimation")
    void RebuildCrewStations();

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery|CrewAnimation")
    int32 GetActiveCrewStationCount() const
    {
        return CrewStations.Num();
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery|CrewAnimation")
    EStrategyArtilleryCrewActivity GetActivityForStation(
        int32 StationIndex) const;

    void HandleArtilleryShotResolved(
        AStrategyUnit* Target,
        EStrategyArtilleryAmmoType AmmoType,
        int32 GunsFired,
        int32 Casualties);

private:
    void SetAllStationsActivity(
        EStrategyArtilleryCrewActivity Activity);

    void UpdateBatteryActivity();
    void UpdateReloadDrill();
    void SetReloadPhaseByFraction(float Fraction);

    UPROPERTY()
    TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;

    bool bReloadDrillActive = false;
};
