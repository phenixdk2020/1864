#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyVisualFormationPath.h"
#include "StrategyCavalryVisualComponent.generated.h"

class ACavalryUnit;
class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimSequence;

/**
 * The squadron as horsemen: one horse and rider per few men in the formation's ranks. The horse is the static
 * model moved by hand (the body rises and pitches with the pace: walk, trot, gallop), the rider sits in the
 * saddle and draws his sabre for the charge. A hit brings horse and rider down; they stay on the field.
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
    float HorseLengthCm = 250.0f;

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
        TObjectPtr<UStaticMeshComponent> Horse;
        TObjectPtr<USkeletalMeshComponent> Rider;
        TObjectPtr<UStaticMeshComponent> Sabre;
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
        TObjectPtr<UStaticMeshComponent> Horse;
        TObjectPtr<USkeletalMeshComponent> Rider;
        TObjectPtr<UStaticMeshComponent> Sabre;
        FRotator From = FRotator::ZeroRotator;
        float Age = 0.0f;
        float Side = 1.0f;
    };

    bool LoadAssets();
    void EnsureCount(int32 Count);
    void Layout(bool bPreserveSlots);
    FHorseman MakeHorseman();
    void Fall(int32 Index);
    void UpdatePace(float DeltaTime);

    UPROPERTY() TObjectPtr<ACavalryUnit> OwnerCavalry;
    UPROPERTY() TObjectPtr<UStaticMesh> HorseModel;
    UPROPERTY() TObjectPtr<UStaticMesh> SabreModel;
    UPROPERTY() TObjectPtr<USkeletalMesh> RiderModel;
    UPROPERTY() TObjectPtr<UAnimSequence> SeatClip;
    UPROPERTY() TObjectPtr<UAnimSequence> FallClip;

    FStrategyVisualFormationPath VisualPath;
    TArray<FHorseman> Horsemen;
    TArray<FFallen> Fallen;
    FVector LastLocation = FVector::ZeroVector;
    float SpeedCmS = 0.0f;
    float HorseScale = 1.0f;
    float HorseYaw = 0.0f;       // the model's own forward
    float HorseLift = 0.0f;      // its feet to the ground
    float SaddleHeightCm = 120.0f;
    float RiderSeatCm = 45.0f;   // the seated pose's pelvis above its root
    FVector FigureLocalCentroid = FVector::ZeroVector;
    int32 CentroidFigureCount = 0;
    int32 CachedMenPerHorseman = INDEX_NONE;
    int32 CachedStrength = INDEX_NONE;
    uint8 CachedFormation = 255;
    bool bReady = false;
};
