#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyVisualFormationPath.h"
#include "StrategyHQVisualComponent.generated.h"

class AStrategyHQUnit;
class UAnimSequence;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A headquarters on the field (PROJECT 1864): the commander's staff shown as two or three horsemen (a pair for a battalion
 * or regiment, a wedge of three for a brigade or division) instead of the grey block. They ride the cavalry's rigged horse
 * (gait and stride by speed) and are the officer figure (sabre in its scabbard), and follow the headquarters, facing the way
 * it moves. Without the rigged horse or the officer the old static horse and seated soldier stand in.
 */
UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyHQVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyHQVisualComponent();

    bool GetDrawnCenter(FVector& OutTrainCenter) const;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;

private:
    bool Build();
    int32 RiderCount() const;

    struct FStaffRider
    {
        TObjectPtr<UStaticMeshComponent> Horse;
        TObjectPtr<USkeletalMeshComponent> HorseSk;
        TObjectPtr<USkeletalMeshComponent> Rider;
        TObjectPtr<UStaticMeshComponent> Sabre;
        TObjectPtr<UStaticMeshComponent> Scabbard;
        TObjectPtr<UAnimSequence> HorseClipNow;
        FVector Offset = FVector::ZeroVector;
        float Phase = 0.0f;
        float GaitPhase = 0.0f;   // the horse's stride (0-1), the rider's clip follows it
        int32 Gait = 0;           // 0 stand, 1 walk, 2 trot, 3 canter, 4 gallop
    };

    void PlayStaffHorseClip(FStaffRider& StaffRider, UAnimSequence* StaffClip, float StaffRate);

    UPROPERTY() TObjectPtr<AStrategyHQUnit> OwnerHQ;
    UPROPERTY() TObjectPtr<USceneComponent> Pivot;
    UPROPERTY() TObjectPtr<UStaticMesh> HorseModel;
    UPROPERTY() TObjectPtr<USkeletalMesh> RiderModel;
    UPROPERTY() TObjectPtr<UAnimSequence> SeatClip;
    UPROPERTY() TObjectPtr<USkeletalMesh> HorseSkeletal;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> HorseGaitClips;   // stand, walk, trot, canter, gallop
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> RiderGaitClips;   // the held seat, then the rider's walk, rising trot, canter, gallop
    bool bSkeletalHorse = false;
    bool bSaddleBone = false;
    bool bOfficerRider = false;
    float StaffSpeed = 0.0f;   // the staff's pace, smoothed (cm/s)
    TArray<FStaffRider> Riders;
    FVector ShownLocation = FVector::ZeroVector;
    FVector ShownVelocity = FVector::ZeroVector;
    float YawVelocity = 0.f;
    FVector LastLocation = FVector::ZeroVector;
    float Yaw = 0.0f;
    float Pace = 0.0f;
    float HorseScale = 1.0f;
    float HorseYaw = 0.0f;
    float HorseLift = 0.0f;
    float SaddleHeightCm = 120.0f;
    bool bBuilt = false;
};
