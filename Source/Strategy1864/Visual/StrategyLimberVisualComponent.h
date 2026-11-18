#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyLimberVisualComponent.generated.h"

class AStrategyArtilleryBatteryUnit;
class UAnimSequence;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * A field battery's limbers and their teams (PROJECT 1864, Allan 2026-11-08: "den vogn til kanonen og så skal der heste på -
 * limber og unlimber"): for every gun the limber (the wagon), a team of up to six rigged horses in pairs with a driver on each
 * near-side horse. On the march the limber is hooked to the trail and pulls the gun; when the battery unlimbers the team drives
 * round to its post behind the gun (pole to the rear), and when it limbers up it drives round again to the trail. The team steers
 * like a team (forward only, a turning circle), its horses in the cavalry's gaits by its speed. Replaces the old static team in
 * UStrategyArtilleryVisualComponent::UpdateTransport. Mortars have none.
 */
UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyLimberVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyLimberVisualComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** True once the teams are drawn (the old static team is then hidden). */
    bool IsDrawing() const { return Teams.Num() > 0; }

private:
    struct FLimberHorse
    {
        TObjectPtr<USkeletalMeshComponent> Horse;
        TObjectPtr<USkeletalMeshComponent> Driver;
        TObjectPtr<UAnimSequence> ClipNow;
        TObjectPtr<UAnimSequence> DriverClipNow;
        int32 Gait = 0;
        float GaitPhase = 0.f;
    };
    struct FLimberTeam
    {
        TObjectPtr<UStaticMeshComponent> Limber;
        TObjectPtr<UStaticMeshComponent> Pole;
        TArray<FLimberHorse> Horses;
        FVector Place = FVector::ZeroVector;   // the limber's middle on the ground
        float Yaw = 0.f;                       // the way the team faces
        float Speed = 0.f;
        bool bPlaced = false;
        bool bHooked = false;                  // on the march: fixed to the gun's trail
        bool bParked = false;                  // in action: standing at its post (stays put while the gun recoils and traverses)
        // The drive to the goal: a smooth curve from the team's place and heading to the goal's (Hermite), walked by its length.
        bool bDriving = false;
        FVector DriveFrom = FVector::ZeroVector, DriveFromAhead = FVector::ZeroVector;
        FVector DriveTo = FVector::ZeroVector, DriveToAhead = FVector::ZeroVector;
        float DriveTangent = 0.f;
        float DriveLength = 0.f;
        float DriveDone = 0.f;
        float DriveSpeed = 0.f;
    };

    bool BuildTeams();
    void PlayHorseClip(FLimberHorse& LimberHorse, UAnimSequence* LimberClip, float LimberRate);

    UPROPERTY(Transient) TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;
    UPROPERTY(Transient) TObjectPtr<USkeletalMesh> HorseMesh;
    UPROPERTY(Transient) TObjectPtr<USkeletalMesh> DriverMesh;
    UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequence>> HorseClips;    // stand, walk, trot, canter, gallop
    UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequence>> DriverClips;   // seat, walk, trot, canter, gallop
    UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequence>> HorseStandExtras;   // other ways to stand (Idle2, Eat)
    TArray<FLimberTeam> Teams;
    float HorseScale = 1.f;
    float HorseLift = 0.f;
    float HorseYaw = 0.f;
    float SaddleHeightCm = 120.f;
    bool bSaddleBone = false;
    bool bBuildTried = false;
    double LimberProbeLast = -100.0;
};
