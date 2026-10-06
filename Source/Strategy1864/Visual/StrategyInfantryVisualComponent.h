#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyInfantryVisualComponent.generated.h"

class AStrategyCompanyUnit;
class UAnimSequence;
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

    /** The hybrid: beyond CrowdFarCm from the camera the company's men (and its fallen) are drawn baked (one
     *  instanced mesh, M_CrowdVAT, each man his own clip and time) instead of as animated skeletal meshes; back to
     *  full animation inside CrowdNearCm. 0 turns it off (also -Strategy1864Crowd=0; -Strategy1864CrowdFar=<cm>). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    float CrowdFarCm = 7000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    float CrowdNearCm = 5500.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Infantry")
    bool IsCrowdMode() const { return bCrowdMode; }

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Infantry")
    void SetEnabled(bool bNewEnabled);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Infantry")
    void SetVisualScaleDivisor(int32 NewDivisor);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Infantry")
    void RefreshVisuals();

    bool GetFormationLocalBounds(FBox& OutBounds) const;

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
    void KillSoldiers(int32 Count);
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

    /** Is an enemy inside the company's chosen range and cone (it may fire). */
    bool IsEnemyInRange() const;
    /** In battle (halted in line facing an enemy within its long range): the soldiers run the firing cycle. */
    bool IsInFiringLine() const;

    int32 CachedStrength = INDEX_NONE;
    uint8 CachedFormationValue = 255;
    bool bCachedBayonetFixed = false;
    bool bLastAnimationLooping = false;
    bool bLastHoldingPose = false;
    bool bLoadAttempted = false;
    FBox FormationLocalBounds = FBox(ForceInit);

    // ---- the hybrid (baked far men)
    /** What a man plays (so either form can take over where the other was): the clip, when it started (its
     *  position = (now - start) * rate), the rate (0: a held pose), looping or once. */
    struct FPlayedClip
    {
        TWeakObjectPtr<UAnimSequence> Clip;
        float Start = 0.0f;
        float Rate = 1.0f;
        bool bLoop = true;
    };
    TArray<FPlayedClip> SoldierClips;   // parallel to SoldierComponents
    TArray<FPlayedClip> CorpseClips;    // parallel to CorpseComponents

    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> CrowdLiving;

    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> CrowdFallen;

    UPROPERTY(Transient)
    TObjectPtr<UStrategyCrowdModel> CrowdModel;

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
