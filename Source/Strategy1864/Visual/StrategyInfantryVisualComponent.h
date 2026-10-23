#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyVisualFormationPath.h"
#include "StrategyColumnDrill.h"
#include "StrategyCrowdModel.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyInfantryVisualComponent.generated.h"

class AStrategyCompanyUnit;
class UAnimSequence;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
class UStrategyCrowdModel;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyInfantryVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    static void SelectNearestFigures(UWorld* World, const FVector& Camera, int32 Budget = 24);
    UStrategyInfantryVisualComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    bool bEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    bool bAnimateIdle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry", meta=(ClampMin="1"))
    int32 VisualScaleDivisor = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry", meta=(ClampMin="0"))
    int32 MaxVisualSoldiers = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry", meta=(ClampMin="0.02"))
    float RefreshIntervalSeconds = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    FName RightHandBoneName = TEXT("RightHand");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    FName LeftHandBoneName = TEXT("LeftHand");

    // Point each rifle from the right hand towards the left hand every refresh, so it lies along the
    // soldier's grip in every animation (the hand bone's own axes differ between animations).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    bool bAlignRifleBetweenHands = true;

    // Where the right hand holds the rifle, as a share of its length from the butt.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry", meta=(ClampMin="0.0", ClampMax="1.0"))
    float RifleGripFraction = 0.22f;

    // The rifle mesh's barrel points along -X instead of +X.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    bool bRifleBarrelAlongNegativeX = false;

    /** Visual reform budget including the orderly departure wave. */
    UPROPERTY(EditAnywhere, Category="Strategy|Visual|Infantry", meta=(ClampMin="1.0"))
    float FormationChangeDeadlineSeconds = 7.f;

    // Each soldier hit falls with a death animation and stays lying where he fell.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    bool bLeaveCorpses = true;

    // Black-powder smoke from the muzzles at every shot.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    bool bMuzzleSmoke = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> RaiseToAimAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> DeathWalkingAsset;
    /** Deaths that start from the shoulder-arms march (column): forward, on the back, crumpling to the knees (2026-10-22). */
    TSoftObjectPtr<UAnimSequence> DeathMarchFrontAsset;
    TSoftObjectPtr<UAnimSequence> DeathMarchBackAsset;
    TSoftObjectPtr<UAnimSequence> DeathMarchCrumpleAsset;

    /** The firing cycle uses a reload matching the soldier's current stance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> LoadAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> RiseFromLoadAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> ReadyAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> AimHoldAsset;

    // The imported Livgarden faces +Y; strategy formations face +X.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    float SoldierMeshYawOffset = -90.0f;

    // Rifle asset points along local X; rotate it into the imported hand grip, then turned 180 degrees about
    // its own X and 180 degrees about its own Y (it sat the wrong way round in the hand).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    FTransform WeaponRelativeTransform = FTransform(
        FQuat(FRotator(-8.0f, 90.0f, 0.0f)) * FQuat(FVector::XAxisVector, UE_PI) * FQuat(FVector::YAxisVector, UE_PI),
        FVector(-4.0f, 0.0f, 10.0f));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Assets")
    TSoftObjectPtr<USkeletalMesh> SoldierMeshAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Assets")
    TSoftObjectPtr<UStaticMesh> RifleMeshAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Assets")
    TSoftObjectPtr<UStaticMesh> RifleBayonetMeshAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> IdleStandingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> WalkStandingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> RunStandingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> AimStandingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> FireStandingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> ReloadStandingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> IdleKneelingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> AimKneelingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> FireKneelingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> ReloadKneelingAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> IdleProneAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> CrawlProneAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> FireProneAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> ReloadProneAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> BayonetChargeAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> BayonetThrustAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> DeathAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> DeathAsset2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Animation")
    TSoftObjectPtr<UAnimSequence> DeathAsset3;

    /** VAT is the default at every distance. Only the nearest CrowdNearestCount living figures inside
     *  CrowdFarCm receive skeletal geometry. 0 disables VAT (also -Strategy1864Crowd=0). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    float CrowdFarCm = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    float CrowdNearCm = 1800.0f; // selection radius; default rendering stays VAT, skeletal near is opt-in

    /** Global nearest-figure budget; the first active company's setting is used for the frame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry", meta=(ClampMin="0"))
    int32 CrowdNearestCount = 24;

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Infantry")
    bool IsCrowdMode() const { return bCrowdMode; }

    /** Presentation completion: the last living figure has halted (independent of simulation arrival). */
    bool CanUseColumnDrill() const;
    /** The colours register here: the company then has a colour bearer. */
    void RegisterColours() { bHasColours = true; }
    /** Test only (-Strategy1864DrillTest KILLCOLOURS): the colour bearer falls now. */
    void DebugKillColourBearer() { if (bColourBearerAlive && ColourBearer) KillColourBearer(); }
    /** Test only (-Strategy1864DrillTest CROWDSTAT): where the drawn men are and what the baked crowd draws. */
    FString DebugCrowdStat() const;
    /** How well the drawn men stand on their places (2026-10-22): the share within FormationReadyGapCm of the formation's own
        place, the worst and 95th-percentile gap, the closest two men (only when asked: n squared) and the colour bearer's gap to
        the nearest man. */
    struct FFormationFit { int32 Men = 0; int32 Placed = 0; float Ready = 1.f; float MaxGapCm = 0.f; float P95GapCm = 0.f; float MinNeighbourCm = 0.f; float BearerGapCm = -1.f; int32 ClosestA = INDEX_NONE; int32 ClosestB = INDEX_NONE; };
    FFormationFit MeasureFormationFit(bool bNeighbours) const;
    /** Where the formation itself puts this man once it stands (the new line on arrival or in a drill; its frame or trail otherwise). */
    FVector FormationPlace(int32 PlaceIndex, bool bPlaceColumn) const;
    /** The metric line (Strategy1864FormationMetric) with the status the HUD shows; with -Strategy1864DebugFormation also the
        places (blue), the men (yellow) and a red line for every man more than 2 m from his place, for one second. */
    void LogFormationMetric(const FString& ShownStatus) const;
    static constexpr float FormationReadyGapCm = 60.f;
    /** Test only: rifle hits so far by layer counted from the shooters (0 = nearest them); [ahead, oblique/flank, behind][layer]. */
    int32 HitLayerTally[3][3] = {};
    /** Where the colours stand (the bearer's hand) or lie (fallen with him). False: no bearer yet, the flag keeps its old place. */
    bool GetColourTransform(FVector& OutColourLocation, float& OutColourYaw, bool& bOutColoursDown) const;
    /** The pole's lean back from vertical while carried (reference: Reference/Units/MultiView/Flag/Fane_March_*.png). */
    static constexpr float ColourCarryLeanDeg = 12.f;
    /** In column the colour bearer walks this far ahead of the front row (one row, 75 cm, put him nearly on their heels with the
        walk's long stride: user's video 2026-10-22). */
    static constexpr float ColourColumnLeadCm = 180.f;
    bool IsVisualFormationComplete() const { return !bEnabled || !bFormationPlanPending; }
    /** The column is filing into line (the drill): the men still walk in column, though the unit's formation is already Line. */
    bool IsDrillingToLine() const { return DrillActive && !DrillToColumn; }
    bool IsDrilling() const { return DrillActive; }
    /** Men are stepping up into gaps or side-stepping towards the colours. */
    bool IsClosingRanks() const;
    bool HasVisualArrivalFrontReached() const { return bArrivalReform && bArrivalFrontReached; }

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Infantry")
    void SetEnabled(bool bNewEnabled);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Infantry")
    void SetVisualScaleDivisor(int32 NewDivisor);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Infantry")
    void RefreshVisuals();

    /** Cached centroid of the living figures, in unit-local space (also valid in crowd mode). */
    bool GetFigureLocalCentroid(FVector& OutCentroid) const;

    /** World-space front corners of the living drawn figures, including settling and pose padding. HUD only. */
    bool GetDrawnFireFront(FVector& OutLeft, FVector& OutRight) const;

    bool GetFormationLocalBounds(FBox& OutBounds) const;

    /** Actual living men in a world-space drawn-facing frame; includes curved columns and settling. */
    bool GetDrawnFormation(FTransform& OutFrame, FBox& OutBounds) const;
    bool GetDrawnFormationEnds(FVector& OutFront, FVector& OutRear) const;
    bool GetDrawnColourPosition(const FVector& Offset, FVector& OutPosition) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Infantry")
    int32 GetRenderedSoldierCount() const
    {
        return SoldierComponents.Num();
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Infantry")
    int32 GetCorpseCount() const
    {
        return CorpseComponents.Num();
    }

private:
    void UpdateNearestFigures(const FVector& Camera);
    void WriteCrowdColours(float* Out) const;
    float CrowdLastColours[13] = {};
    TSet<TWeakObjectPtr<USkeletalMeshComponent>> CrowdNearFigures;
    void EnsureWeaponComponent(int32 Index);
    UStaticMeshComponent* CreateWeaponComponent(USkeletalMeshComponent* Soldier);
    void UpdateStanceTransitions();
    UPROPERTY(Transient)
    TArray<TObjectPtr<UAnimSequence>> CrowdStanceClips;
    TMap<TWeakObjectPtr<USkeletalMeshComponent>, int32> CrowdLivingIds;
    TMap<TWeakObjectPtr<USkeletalMeshComponent>, int32> CrowdFallenIds;
    TArray<int32> CrowdFreeLivingIds;
    TArray<int32> CrowdFreeFallenIds;
    UFUNCTION()
    void HandleCasualtyVisualEvent(int32 AppliedLoss, FVector SourceLocation);
    void UpdateCorpses();
    TArray<float> CorpseBirthTimes;
    TArray<bool> CorpseFading;

    UFUNCTION()
    void HandleVolleyVisualEvent(
        FVector Origin,
        FVector Direction,
        int32 Shots,
        int32 Hits);

    bool EnsureAssetsLoaded();
    int32 GetDesiredVisualCount() const;
    void EnsureVisualCount(int32 DesiredCount);
    void RebuildFormation();
    void UpdateFormationBounds();
    void RefreshAnimation(bool bForce = false);
    void RefreshWeaponMeshes();
    UAnimSequence* ResolveAnimation(bool& bOutLooping) const;
    void DestroyVisualComponents();
    /** Soldiers hit: out of the ranks, a death animation, and left lying (Docs: the duel test). */
    void KillSoldiers(int32 Count, const FVector* Near = nullptr, const FVector* ConeOrigin = nullptr);
    /** Men lost to a shot still in the air fall when it strikes, next to the strike (the artillery's registry). */
    void QueueKills(int32 Count);
    /** A marching company raises dust behind it (a few puffs along its rear rank). */
    void MarchDust(float Now);
    float NextMarchDust = 0.0f;
    void ProcessPendingKills();
    /** After losses in a line: the man directly behind a gap steps forward in his own file (2nd to 1st rank, then 3rd to 2nd). */
    void ScheduleStepUp();   // StepUpFiles after a short, morale-scaled delay
    void StepUpFiles();
    void StepUpColumn();
    struct FPendingKill { int32 Count = 0; FVector Location = FVector::ZeroVector; float Time = 0.0f; bool bCone = false; FVector Origin = FVector::ZeroVector; };
    TArray<FPendingKill> PendingKills;
    int32 PendingKillCount = 0;
    /** Rifles along the grip, from the right hand towards the left. */
    void AlignWeapons();
    /** Each soldier's own shot: raise, fire, smoke; then back to the company's animation. */
    void UpdatePersonalActions();
    void PlayOnSoldier(USkeletalMeshComponent* Soldier, UAnimSequence* Sequence, bool bLooping, bool bRandomStart);
    void SpawnMuzzleSmoke(const USkeletalMeshComponent* Soldier, int32 Index);

    UPROPERTY(Transient)
    TObjectPtr<AStrategyCompanyUnit> OwnerCompany;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMesh> LoadedSoldierMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UAnimSequence>> LoadedBattleAnimations;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> LoadedBattleFadeMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> LoadedRifleMesh;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> LoadedRifleBayonetMesh;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> LastAnimationAsset;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> SoldierComponents;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> WeaponComponents;

    /** The fallen: they stay where they fell (detached from the company), their rifles with them. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> CorpseComponents;

    /** Per soldier (parallel to SoldierComponents): when his own shot starts, and until when it plays. */
    TArray<float> SoldierFireAt;
    TArray<float> SoldierBusyUntil;
    TArray<uint8> SoldierFirePhase;   // a EFirePhase for each soldier
    TArray<int32> SoldierSlots;       // each soldier's formation slot (its rank: slot % ranks, for the fire drill)

    /** Each man goes to his place in a new formation (running), and changes stance one after the other: not a teleport. */
    struct FSettle
    {
        float DrillS0 = 0.f;
        float DrillLateral = 0.f;
        float DrillFinalLateral = 0.f;
        float DrillLaneShift = 0.f;
        float DrillDelta = 0.f;
        bool DrillStopped = false;
        float DrillStopTime = -1.f;    // when he halted on his line place (ShoulderDown starts then)
        FVector CloseFrom = FVector::ZeroVector;   // closing ranks: side-step from this slot to CloseTo, starting at CloseStart
        FVector CloseTo = FVector::ZeroVector;
        float CloseStart = -1.f;
        float CloseDuration = 0.f;
        int32 SnakeRow = 0;    // snake column: row behind the head and lane (0..2) while the line->column spacing ramps
        int32 SnakeLane = 1;
        FVector Goal = FVector::ZeroVector;
        FVector Slot = FVector::ZeroVector;
        FVector WorldVelocity = FVector::ZeroVector;
        float Pace = FMath::FRandRange(0.95f, 1.05f);
        float WalkPhase = FMath::FRand();
        float StartDelay = 0.f;
        float ReformDeadline = -1.f;
        FVector ReformTarget = FVector::ZeroVector;
        FVector ReformStartLocal = FVector::ZeroVector;   // v2 rigid morph: where he stood in the unit's frame when the change began
        float ReformStartTime = 0.f;
        float ReformDuration = 0.f;
        float StartRemaining = 0.f;
        float HaltRemaining = 0.f;
        float SlotYaw = 0.f;
        float Yaw = 0.0f;
        float YawVelocity = 0.f;
        bool bPlaced = false;     // has been put in his first place
        bool bActive = false;     // is on his way to Goal
        float SwitchAt = -1.0f;   // when he takes up the pending clip (stance change), -1: none
        uint8 VisualStance = 0;
        float StanceEnd = -1.f;
        float NearBlend = 0.f;
    };
    /** The column carries the rifle on the shoulder: one cycle up when the company goes into column, one down when it forms line. */
    UPROPERTY(Transient) TSoftObjectPtr<UAnimSequence> WalkShoulderAsset;
    UPROPERTY(Transient) TSoftObjectPtr<UAnimSequence> ShoulderUpAsset;
    UPROPERTY(Transient) TSoftObjectPtr<UAnimSequence> ShoulderDownAsset;
    /** Closing ranks: the men side-step towards the colours, facing the enemy (right step, and its mirror to the left). */
    UPROPERTY(Transient) TSoftObjectPtr<UAnimSequence> SideStepAsset;
    UPROPERTY(Transient) TSoftObjectPtr<UAnimSequence> SideStepLeftAsset;
    /** The colour bearer's own walk and stand (pole in the right hand at chest height; Tools/Animation/build_colour_bearer.py). */
    UPROPERTY(Transient) TSoftObjectPtr<UAnimSequence> ColourWalkAsset;
    UPROPERTY(Transient) TSoftObjectPtr<UAnimSequence> ColourIdleAsset;
    bool bShoulderColumn = false;
    bool bShoulderRising = false;
    float ShoulderTransitionEnd = -1.f;   // safety cap only; the real end is when the Up/Down clips have played to their end (UpdateSettling)
    float ShoulderTransitionStart = -1.f;
    bool bScaleOnlyRebuild = false;
    /** Losses leave holes (survivors keep their places); a few quiet seconds after the last loss the men close ranks into the compact line. */
    bool bClosingRanksPending = false;
    bool bStepUpPending = false;
    float StepUpDueSeconds = 0.f;
    float LastLossTimeSeconds = -1000.f;
    float FirstPendingLossTimeSeconds = -1000.f;   // the first loss not yet closed up (ranks close even under steady losses)
    bool bRigidCompany = true;   // -Strategy1864Rigid=0 returns to the per-man chase with the spring-smoothed centre
    FStrategyVisualFormationPath VisualPath;
    TArray<FSettle> SoldierSettle;       // parallel to SoldierComponents
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> PendingSequence;
    bool bPendingLoop = true;
    bool bPendingHold = false;
    bool bWasSettling = false;
    bool bWasVisualTravel = false;
    bool bWasDisengaging = false;   // AFBRYD was active last frame: the men gather in their slots when it ends
    void UpdateSettling(float DeltaTime);
    void PlanOrderlyReform(bool bDestination, const FTransform& ReformFrame);
    bool PlanColumnDrill();
    /** The colour bearer (user 2026-10-18): one man of the company, out of the file/rank grid, in FRONT of the line and at the head
     *  of the column, carrying the colours. Shot, the colours fall; after 2-4 s the nearest front-rank man drops his rifle, picks
     *  them up and takes the bearer's place. */
    void UpdateColourBearer(float BearerDt);
    /** The centre of a hand's palm: halfway from the hand bone to its first child (the grip of a rifle or a colour pole). */
    static FVector PalmLocation(const USkeletalMeshComponent* PalmMesh, FName PalmHandBone);
    void PromoteColourBearer(bool bBearerInitial);
    void KillColourBearer();
    FVector ColourBearerTarget(float& OutBearerYaw) const;
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> ColourBearer;
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> FallenColourBearers;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> DroppedRifles;
    bool bHasColours = false;
    bool bColourBearerAlive = false;
    bool bColoursDown = false;
    float ColourPromoteAt = -1.f;
    float ColourBearerYawVelocity = 0.f;
    FVector ColourLastGoal = FVector::ZeroVector;   // -Strategy1864DebugFormation: logs a jump of the bearer's target
    /** The bearer's own world place and facing, kept across a jump of the unit's actor (he is attached to its root). */
    FVector ColourBearerWorld = FVector::ZeroVector;
    FRotator ColourBearerWorldRotation = FRotator::ZeroRotator;
    bool bColourBearerWorldValid = false;
    // Taking up the fallen colours: 1 = kneeling down to them, 2 = rising with them; ends at ColourPickupEnd.
    int32 ColourPickupPhase = 0;
    float ColourPickupEnd = 0.f;
    FVector ColourDownLocation = FVector::ZeroVector;
    float ColourDownYaw = 0.f;
    /** "Slut op mod fanen": after losses only the files outside a gap side-step inwards, facing the front, as a ripple. */
    bool CloseRanksTowardColours();
    void UpdateCloseRanks();
    bool bClosingRanksStep = false;
    float CloseDiagNext = 0.f;   // -Strategy1864DebugFormation: next CloseDiag line
    /** Line -> column as a snake: the trail is seeded along the line, the head starts at the leading flank, the men follow. */
    bool SeedColumnSnake(const FVector& SnakeFront, const FVector& SnakeAxis, float SnakeFlank, float SnakeWidth, int32 SnakeRows);
    void UpdateSnakeRamp();
    float SnakeHoldUntil = -1.f;      // the men face right/left on the spot before the head moves
    float SnakeHoldStart = -1.f;      // ... and shoulder their rifles (ShoulderUp, standing) during that hold
    bool bSnakeRamp = false;          // rows open 75 -> 90 cm and lanes close 90 -> 75 cm over the first metres
    float SnakeRampFrom = 0.f;
    float SnakeRowSpacing = 0.f;      // row spacing of a column seeded from a line (= file spacing); 0 = generated slots (rank spacing)
    bool UpdateColumnDrill(float DrillDt);
    void AbortColumnDrill(const TCHAR* DrillWhy = TEXT("ukendt"));
    void LogColumnDrillDistance(bool DrillComplete);
    bool DrillActive = false;
    bool DrillToColumn = false;
    bool DrillArrival = false;
    bool DrillRebuildAborted = false;
    int32 DrillOrderSerial = INDEX_NONE;
    int32 DrillStrength = 0;
    uint8 DrillFormation = 255;
    float DrillStarted = 0.f;
    float DrillPlannedLateralSpacing = 0.f;
    float DrillPlannedRankSpacing = 0.f;
    float DrillSpeed = 160.f;
    float DrillDuration = 0.f;
    float DrillMarchDuration = 0.f;
    float DrillArcEnd = 0.f;
    float DrillStraightA = 0.f;
    float DrillRadius = 300.f;
    float DrillLogAt = 0.f;
    FTransform DrillFinalFrame = FTransform::Identity;
    FStrategyOrder DrillOrder;
    TArray<FStrategyColumnDrill::FSample> DrillPath;
    void KeepReformSpacing(float DeltaTime);
    TArray<FVector> FormationStepStarts;
    TArray<FVector> FormationStepPositions;
    TMap<FIntPoint, int32> FormationSpacingGrid;
    TArray<int32> FormationSpacingNext;
    bool bArrivalReform = false;
    bool bArrivalFrontReached = false;
    int32 ArrivalOrderSerial = INDEX_NONE;
    FVector ArrivalDestination = FVector::ZeroVector;
    FVector ArrivalFormationOrigin = FVector::ZeroVector;
    float ArrivalTravelSpeed = 160.f;
    float ArrivalFacing = 0.f;
    float FormationPlanStarted = -1.f;
    float FormationLongestPath = 0.f;
    int32 FormationCrossingsAvoided = 0;
    bool bFormationPlanPending = false;
    void ApplyPendingStance(float Now);
    void DebugSmooth();
    double SmoothLogAt = 0.0;
    FVector SmoothPreviousAnchor = FVector::ZeroVector;
    float SmoothMaxAnchor = 0.f;
    float SmoothMaxRealDelta = 0.f;
    FVector SmoothPreviousCenter = FVector::ZeroVector;
    float SmoothMaxCenter = 0.f;
    TWeakObjectPtr<USkeletalMeshComponent> SmoothSamples[5];
    FVector SmoothPreviousMen[5];
    float SmoothMaxMen[5] = {};
    bool bSmoothSampled = false;

    /** Is an enemy inside the company's chosen range and cone (it may fire). */
    bool IsEnemyInRange() const;
    /** In battle (halted in line facing an enemy within its long range): the soldiers run the firing cycle. */
    bool IsInFiringLine() const;

    FVector FigureLocalCentroid = FVector::ZeroVector;
    int32 CentroidFigureCount = 0;
    int32 CachedStrength = INDEX_NONE;
    uint8 CachedFormationValue = 255;
    float CachedCoverLateralSpacing = -1.0f;
    float CachedCoverRankSpacing = -1.0f;
    int32 CachedSpecialRanks = -1;
    int32 CachedScreenStrength = -1;
    bool bCachedBayonetFixed = false;
    bool bLastAnimationLooping = false;
    bool bLastHoldingPose = false;
    bool bLoadAttempted = false;
    FBox FormationLocalBounds = FBox(ForceInit);

    // ---- the hybrid (baked far men)
    /** What a man plays (so either form can take over where the other was): the clip, when it started (its
     *  position = (VisualAnimationTime - start) * rate), the rate (0: HeldPosition), looping or once. */
    struct FPlayedClip
    {
        TWeakObjectPtr<UAnimSequence> Clip;
        float Start = 0.0f;
        float Rate = 1.0f;
        float HeldPosition = 0.f;
        bool bLoop = true;
    };
    float VisualAnimationTime = 0.f;
    bool MakeCrowdClipData(const FPlayedClip& Played, float* Out) const;
    TArray<FPlayedClip> SoldierClips;   // parallel to SoldierComponents
    TArray<FPlayedClip> CorpseClips;    // parallel to CorpseComponents

    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> CrowdLiving;

    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> CrowdFallen;

    UPROPERTY(Transient)
    TObjectPtr<UStrategyCrowdModel> CrowdModel;

    UPROPERTY(Transient)
    TArray<FStrategyCrowdAuxGroup> CrowdLODBands; // living/fallen pairs for LOD1..3, plus shadowed LOD1 below 25 m
    TMap<TWeakObjectPtr<USkeletalMeshComponent>, int32> CrowdFigureBands;
    double CrowdBandLogAt = 0.0;
    int32 ResolveCrowdBand(USkeletalMeshComponent* Figure) const;
    bool bResumeCrowdPosesNextTick = false;
    TArray<FTransform> CrowdLivingScratch;
    TArray<FTransform> CrowdFallenScratch;
    bool bCrowdMode = false;
    bool bCrowdDirty = false;       // the instances to rebuild (men added, fallen, moved in the formation)
    bool bCrowdDataDirty = false;   // a man's clip changed
    bool bCrowdFailed = false;

    void RecordClip(USkeletalMeshComponent* Soldier, UAnimSequence* Clip, bool bLoop, float Position, float Rate);
    void UpdateCrowdMode();
    bool EnsureCrowdModel();
    void EnterCrowdMode();
    void LeaveCrowdMode();
    void RebuildCrowdInstances();
    void RestoreClip(USkeletalMeshComponent* Soldier, const FPlayedClip& Played, float Now) const;
    UStaticMesh* CurrentRifleMesh() const;
};
