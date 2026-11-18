#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyGunCrewVisualComponent.generated.h"

class AStrategyArtilleryBatteryUnit;
class UAnimSequence;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * A battery's gun crews on the field (PROJECT 1864, user 2026-11-05): one figure for every crew station of
 * UStrategyArtilleryCrewAnimationComponent (up to eight a gun), standing at its post by its piece and playing the clip of
 * its station's activity (sponge, load, ram, prime, aim, push at the wheels, stand by). On the march they walk beside the
 * pieces; as the battery loses men they fall, the reserve first and the gun commanders last. Without the crew clips
 * (/Game/Units/Artillery/Crew) they stand in the soldiers' idle.
 */
UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyGunCrewVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyGunCrewVisualComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    struct FGunCrewFigure
    {
        TObjectPtr<USkeletalMeshComponent> Man;
        TObjectPtr<UStaticMeshComponent> Prop;
        TObjectPtr<UAnimSequence> ClipNow;
        int32 Station = INDEX_NONE;
        bool bFallen = false;
        FVector WorldPlace = FVector::ZeroVector;
        bool bPlaced = false;
        float YawVelocity = 0.f;
        float Yaw = 0.f;
    };

    bool BuildCrew();
    /** The post of a station by its piece: offset in the gun's frame (X along the muzzle, Y to its right) and the way he faces. */
    static void StationPost(uint8 StationRole, bool bInAction, int32 StationOrder, uint8 StationActivity, FVector& OutOffset, float& OutFacingYaw);
    UAnimSequence* ClipForStation(int32 StationIndex, bool bInAction, bool bWalking) const;

    UPROPERTY(Transient) TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;
    UPROPERTY(Transient) TObjectPtr<USkeletalMesh> CrewMesh;
    UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequence>> CrewClips;   // see the clip index in the .cpp
    TArray<FGunCrewFigure> Figures;
    int32 InitialCrewStrength = 0;
    bool bBuildTried = false;
    double CrewProbeLast = -100.0;
};
