#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyOOBTestScenario.generated.h"

class AStrategyCompanyUnit;
class AStrategyHQUnit;
class AStrategyUnit;
class AStrategyRiverBarrier;
class ACavalryUnit;
class AStrategyNavigationObstacle;
class AStrategyArtilleryBatteryUnit;
class AStrategySupplyWagonUnit;
class AStrategyTerrainFeature;
class AStrategyMortarBatteryUnit;
class AStrategyDefensivePosition;
class USceneComponent;
class UStaticMeshComponent;
enum class EStrategyTerrainFeatureType : uint8;

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyOOBTestScenario : public AActor
{
    GENERATED_BODY()

public:
    AStrategyOOBTestScenario();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bBuildOnBeginPlay = true;

    // A focused model/combat test; the full OOB remains available when disabled.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bLivgardenVsSwedishTest = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    FVector Origin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    float CompanySpacing = 1400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnEnemyQAUnits = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnRiverQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnCavalryQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnObstacleQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnArtilleryQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnSupplyWagonQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnTerrainQA = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnMortarQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnFortificationQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    bool bSpawnSpecialistQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test|Visual")
    bool bSpawnRealInfantryVisualQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test|FlatMap")
    bool bUseFlatQAMap = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test|FlatMap", meta=(ClampMin="10000.0"))
    float FlatMapSizeCm = 60000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Test|FlatMap")
    TObjectPtr<USceneComponent> QAMapRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Test|FlatMap")
    TObjectPtr<UStaticMeshComponent> QAFlatGround;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test")
    int32 QARandomSeed = 1864;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test|Visual")
    bool bDrawRuntimeQAVisuals = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test|Visual")
    float QARiverVisualHalfLengthCm = 15000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Test|Visual")
    float QAVisualThickness = 5.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Test")
    void BuildTestOOB();

    UFUNCTION(BlueprintCallable, Category="Strategy|Test")
    void ClearSpawnedUnits();

    UFUNCTION(BlueprintCallable, Category="Strategy|Test")
    void ResetScenario();

    UFUNCTION(BlueprintCallable, Category="Strategy|Test")
    bool ValidateStableIdsAndHierarchy(TArray<FString>& OutErrors) const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Test")
    bool RunRegressionChecklist(TArray<FString>& OutFailures) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Test")
    TArray<AStrategyUnit*> GetSpawnedUnits() const;

private:
    AStrategyHQUnit* SpawnHQ(
        const FName StableId,
        const FString& Name,
        uint8 HQLevelValue,
        const FVector& Location,
        AStrategyUnit* OrganicParent);

    AStrategyCompanyUnit* SpawnCompany(
        const FName StableId,
        const FString& Name,
        int32 CompanyNumber,
        const FVector& Location,
        AStrategyUnit* OrganicParent,
        uint8 SideValue);

    ACavalryUnit* SpawnCavalry(
        const FName StableId,
        const FString& Name,
        const FVector& Location,
        AStrategyUnit* OrganicParent);

    AStrategyArtilleryBatteryUnit* SpawnArtilleryBattery(
        const FName StableId,
        const FString& Name,
        const FVector& Location,
        AStrategyUnit* OrganicParent);

    AStrategySupplyWagonUnit* SpawnSupplyWagon(
        const FName StableId,
        const FString& Name,
        const FVector& Location,
        AStrategyUnit* OrganicParent);

    AStrategyMortarBatteryUnit* SpawnMortarBattery(
        const FName StableId,
        const FString& Name,
        const FVector& Location,
        AStrategyUnit* OrganicParent);

    AStrategyTerrainFeature* SpawnTerrainFeature(
        EStrategyTerrainFeatureType FeatureType,
        const FVector& Location,
        const FVector2D& RadiusCm,
        float PeakHeightCm,
        float YawDegrees = 0.0f);

    UPROPERTY()
    TArray<TObjectPtr<AStrategyUnit>> SpawnedUnitObjects;

    UPROPERTY()
    TObjectPtr<AStrategyRiverBarrier> SpawnedRiverBarrier;

    UPROPERTY()
    TObjectPtr<AStrategyNavigationObstacle> SpawnedNavigationObstacle;

    UPROPERTY()
    TArray<TObjectPtr<AStrategyTerrainFeature>> SpawnedTerrainFeatures;

    UPROPERTY()
    TArray<TObjectPtr<AStrategyDefensivePosition>> SpawnedDefensivePositions;

    void DrawRuntimeQAVisuals() const;
    void ConfigureRuntimeQALabel(AStrategyUnit* Unit) const;

public:
    /** The battle from the campaign: true when it was set up (the field and the units). */
    bool IsCampaignBattle() const { return bCampaignBattle; }

    /** Writes Saved/Battle/BattleResult_<N>.json (losses per campaign unit, enemy losses, outcome) and,
     *  when the battle came from the campaign, goes back to the campaign map. */
    UFUNCTION(BlueprintCallable, Category="Strategy|Campaign")
    void FinishCampaignBattle();

private:
    /** Builds the campaign's battlefield and its units (BattleRequest, Units.json, Battlefield_*.json). */
    bool BuildCampaignBattle(const FString& BattlefieldFile, int32 BattleId);

    UPROPERTY(Transient)
    TObjectPtr<class AStrategyCampaignBattlefield> CampaignField;

    bool bCampaignBattle = false;
    bool bReturnToCampaign = false;
    bool bFieldCameraPlaced = true;
    bool bCampaignFinished = false;
    FVector FieldCameraTarget = FVector::ZeroVector;
    int32 CampaignBattleId = 0;
    /** The units of the battle and the campaign unit each belongs to ("" for the enemy). */
    TMap<TWeakObjectPtr<AStrategyUnit>, FString> CampaignUnitOf;

    /** The duel: each company advances until the enemy is inside its own active fire range, then holds and
     *  fires; the fire cones of both are drawn. */
    void TickDuel(float DeltaSeconds);
    void DrawDuelCones() const;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AStrategyCompanyUnit>> DuelCompanies;

    float DuelAccumulator = 0.0f;
    bool bDuelCameraPlaced = false;
};
