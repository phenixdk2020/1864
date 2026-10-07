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
    /** The duel's two companies (their fire cones are drawn by the HUD). */
    const TArray<TObjectPtr<AStrategyCompanyUnit>>& GetDuelCompanies() const { return DuelCompanies; }

    /** The battle from the campaign: true when it was set up (the field and the units). */
    bool IsCampaignBattle() const { return bCampaignBattle; }

    /** The pioneers (research Pontonnerkorpset): may they lay a pontoon bridge, and order one by a staff (it is
     *  laid over the broad river nearest the staff, within 600 m, after five minutes). */
    bool CanLayPontoonBridges() const { return bCampaignBattle && bPioneerBridges; }
    /** Testing: the enemy attacks (his staff orders the attack on the Danish line, his officers close) or defends
     *  (holds where he stands, front to the Danes, fires only at those who come within his reach). */
    void SetEnemyAttacking(bool bAttack);
    bool IsEnemyAttacking() const { return bEnemyAttacking; }
    bool bEnemyAttacking = true;
    /** Testing: the enemy fires (true), or holds fire whatever happens (false). */
    void SetEnemyFiring(bool bFire);
    bool IsEnemyFiring() const { return bEnemyFiring; }
    bool bEnemyFiring = true;
    float EnemyFireTimer = 0.0f;
    void EnforceEnemyHoldFire();

    /** The battle's standing (every battle): the men each side began with and has left (fighting units only:
     *  companies, squadrons, batteries; no staffs or wagons), and how many of its units have broken. */
    void GetBattleScore(int32& OutDanesStart, int32& OutDanesNow, int32& OutEnemyStart, int32& OutEnemyNow, int32& OutDanesBroken, int32& OutEnemyBroken) const;

    /** The outcome once decided (a side below 35 % of its men, or every unit of it broken): empty while the
     *  battle goes on. */
    const FString& GetBattleOutcome() const { return BattleOutcome; }
    bool IsDanishVictory() const { return bDanishVictory; }

    /** The small test battle (map *Skirmish* or -Strategy1864Skirmish=<1-4>): a Danish battalion staff with two
     *  companies under the player against one to four enemy companies with their staff and the AI. */
    bool IsSkirmish() const { return bSkirmish; }
    bool OrderPontoonBridge(const AStrategyUnit* By);
    /** Seconds until the next pontoon bridge is laid (0: none being laid). */
    float GetPontoonSecondsLeft() const;

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
    float FieldCameraYaw = 0.0f;
    bool bPioneerBridges = false;
    bool bSkirmish = false;
    FString BattleOutcome;
    bool bDanishVictory = false;
    /** -Strategy1864Shots=sec:unit:distance,...: the view goes to the unit and a screenshot is saved (QA of the look). */
    void TickShots();
    TArray<FString> ShotPlan;
    int32 NextShot = 0;
    float ShotTakeAt = -1.0f;
    bool bShotsParsed = false;
    UPROPERTY() TObjectPtr<AActor> ShotCamera;
    float BattleScoreTimer = 0.0f;
    void BuildSkirmish(int32 EnemyCompanies);
    void UpdateBattleOutcome();
    struct FPendingPontoon { FVector Where = FVector::ZeroVector; float ReadyAt = 0.0f; };
    TArray<FPendingPontoon> PendingPontoons;
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
