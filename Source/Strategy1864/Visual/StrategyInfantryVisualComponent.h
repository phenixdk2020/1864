#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyInfantryVisualComponent.generated.h"

class AStrategyCompanyUnit;
class UAnimSequence;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry", meta=(ClampMin="1"))
    int32 VisualScaleDivisor = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry", meta=(ClampMin="0"))
    int32 MaxVisualSoldiers = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry", meta=(ClampMin="0.02"))
    float RefreshIntervalSeconds = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    FName RightHandBoneName = TEXT("mixamorig:RightHand");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Infantry")
    FTransform WeaponRelativeTransform = FTransform::Identity;

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

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Infantry")
    void SetEnabled(bool bNewEnabled);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Infantry")
    void SetVisualScaleDivisor(int32 NewDivisor);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Infantry")
    void RefreshVisuals();

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Infantry")
    int32 GetRenderedSoldierCount() const
    {
        return SoldierComponents.Num();
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
    void RefreshAnimation(bool bForce = false);
    void RefreshWeaponMeshes();
    UAnimSequence* ResolveAnimation(bool& bOutLooping) const;
    void DestroyVisualComponents();

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

    int32 CachedStrength = INDEX_NONE;
    uint8 CachedFormationValue = 255;
    bool bCachedBayonetFixed = false;
    bool bLastAnimationLooping = false;
    bool bLoadAttempted = false;
};
