#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyVisualFormationPath.h"
#include "StrategyCavalryVisualComponent.generated.h"

class ACavalryUnit;
class UStaticMesh;
class UStaticMeshComponent;
class UMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimSequence;

/**
 * The squadron as horsemen: one horse and rider per few men in the formation's ranks. The horse is the rigged
 * SK_Horse_1864 (Content/Units/Horse) playing A_Horse_Stand/Walk/Trot/Canter/Gallop by its speed (the thresholds of
 * UStrategyHorseAnimationStateComponent) with a play rate that follows the speed; it leans in turns. A charge gives the
 * gallop, a charge repelled by a square gives A_Horse_Rear, a hit gives A_Horse_Fall. The rider (baked crowd clip) sits on
 * the horse's saddle_root bone and plays the rider clip matching the horse clip, draws his sabre at the start of the
 * charge, slashes at contact and falls with his own clip. Without SK_Horse_1864 the old static horse with a sine bob is used.
 */
UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyCavalryVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyCavalryVisualComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Men per horseman shown. */
    UPROPERTY(EditAnywhere, Category="Strategy|Visual|Cavalry")
    int32 MenPerHorseman = 2;

    UPROPERTY(EditAnywhere, Category="Strategy|Visual|Cavalry")
    float HorseLengthCm = 250.0f;   // the static fallback horse

    /** The skeletal horse's nose-to-tail length (the mesh is 199 cm unscaled; 1.1 is the scale the rider clips were built for). */
    UPROPERTY(EditAnywhere, Category="Strategy|Visual|Cavalry")
    float SkeletalHorseLengthCm = 219.0f;

    UPROPERTY(EditAnywhere, Category="Strategy|Visual|Cavalry")
    float FileSpacingCm = 130.0f;

    UPROPERTY(EditAnywhere, Category="Strategy|Visual|Cavalry")
    float RankSpacingCm = 330.0f;

    /** Cached centroid of the living horsemen, in unit-local space. */
    bool GetFigureLocalCentroid(FVector& OutCentroid) const;
    int32 GetRenderedHorsemanCount() const { return Horsemen.Num() + Fallen.Num(); }

private:
    struct FHorseman
    {
        TObjectPtr<UMeshComponent> Horse;               // skeletal (HorseSk) or the static fallback
        TObjectPtr<USkeletalMeshComponent> HorseSk;
        TObjectPtr<USkeletalMeshComponent> Rider;
        TObjectPtr<UStaticMeshComponent> Sabre;
        TObjectPtr<UAnimSequence> HorseClipNow;
        int32 CavGait = 0;                      // EStrategyHorseGait as an index
        float CavSpeed = 0.f;                   // eased speed of this horse
        float CavLean = 0.f;                    // degrees, into the turn
        float CavPhase = 0.f;                   // 0..1 of the horse's gait clip (the horse is the master)
        float CavDrawAge = -1.f;                // rider: sabre draw clip clock (negative while waiting)
        float CavSlashAge = -1.f;               // rider: slash clock (negative while waiting)
        float CavRearAge = -1.f;                // horse: rear clock (negative while waiting)
        bool bCavSlash = false;
        bool bCavRear = false;
        FVector Slot = FVector::ZeroVector;     // local to the unit
        FVector Shown = FVector::ZeroVector;    // local, eased towards the slot
        FVector ShownVelocity = FVector::ZeroVector;
        float FacingYaw = 0.f;
        float FacingVelocity = 0.f;
        float GroundZ = 0.f;
        bool bGroundPlaced = false;
        bool bPlaced = false;
        float Phase = 0.0f;
        float NextDust = 0.0f;
    };
    struct FFallen
    {
        TObjectPtr<UMeshComponent> Horse;
        TObjectPtr<USkeletalMeshComponent> Rider;
        TObjectPtr<UStaticMeshComponent> Sabre;
        FRotator From = FRotator::ZeroRotator;
        float Age = 0.0f;
        float Side = 1.0f;
    };

    bool LoadAssets();
    USkeletalMesh* ChooseRiderModel() const;
    bool bRiderChosen = false;
    void EnsureCount(int32 Count);
    void Layout(bool bPreserveSlots);
    FHorseman MakeHorseman();
    void Fall(int32 Index);
    void UpdatePace(float DeltaTime);
    void PlayHorseClip(FHorseman& H, UAnimSequence* Clip, bool bLoop, float NormalisedPhase, float Rate);
    UAnimSequence* HorseClipForGait(int32 GaitIndex) const;
    UAnimSequence* RiderClipForGait(int32 GaitIndex) const;

    UPROPERTY() TObjectPtr<ACavalryUnit> OwnerCavalry;
    UPROPERTY() TObjectPtr<UStaticMesh> HorseModel;
    UPROPERTY() TObjectPtr<UStaticMesh> SabreModel;
    UPROPERTY() TObjectPtr<UStaticMesh> SaddleModel;
    UPROPERTY() TObjectPtr<USkeletalMesh> RiderModel;
    /** A hidden rider posed at each charging rider's clip and time to place his sabre in his hand (the baked riders have no bones). */
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> SabrePoser;
    float SabreProbeLast = -100.f;
    UPROPERTY() TObjectPtr<UAnimSequence> SeatClip;
    UPROPERTY() TObjectPtr<UAnimSequence> FallClip;
    UPROPERTY() TObjectPtr<USkeletalMesh> HorseSkeletal;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> HorseGaitClips;   // Stand, Walk, Trot, Canter, Gallop
    UPROPERTY() TObjectPtr<UAnimSequence> HorseRearClip;
    UPROPERTY() TObjectPtr<UAnimSequence> HorseFallClip;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> RiderGaitClips;   // seat (held), walk, rising trot, canter, gallop
    UPROPERTY() TObjectPtr<UAnimSequence> RiderChargeClip;          // standing in the stirrups
    UPROPERTY() TObjectPtr<UAnimSequence> RiderDrawClip;
    UPROPERTY() TObjectPtr<UAnimSequence> RiderSlashClip;
    UPROPERTY() TObjectPtr<UAnimSequence> RiderFallClip;

    FStrategyVisualFormationPath VisualPath;
    TArray<FHorseman> Horsemen;
    TArray<FFallen> Fallen;
    FVector LastLocation = FVector::ZeroVector;
    float SpeedCmS = 0.0f;
    float HorseScale = 1.0f;
    float HorseYaw = 0.0f;       // the model's own forward
    float HorseLift = 0.0f;      // its feet to the ground
    float SaddleHeightCm = 120.0f;
    float SaddleLiftCm = 6.0f;   // the rider's pelvis above the saddle_root bone
    bool bSkeletalHorse = false;
    bool bSaddleBone = false;
    bool bChargePrev = false;
    float RiderSeatCm = 45.0f;   // the seated pose's pelvis above its root
    FVector FigureLocalCentroid = FVector::ZeroVector;
    int32 CentroidFigureCount = 0;
    int32 CachedMenPerHorseman = INDEX_NONE;
    int32 CachedStrength = INDEX_NONE;
    uint8 CachedFormation = 255;
    bool bReady = false;
};
