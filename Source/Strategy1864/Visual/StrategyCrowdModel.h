#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/Object.h"
#include "StrategyCrowdModel.generated.h"

class UAnimSequence;
class UMaterialInstanceDynamic;
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;

/** How a rifle sits in the hands (the same rule as the visual component's AlignWeapons). */
struct FStrategyCrowdRifleGrip
{
    FName RightHand;
    FName LeftHand;
    FTransform HandTransform = FTransform::Identity;   // when the hands are together: the right hand's own grip
    float GripFraction = 0.22f;
    bool bBarrelAlongNegativeX = false;
};

/** One baked animation: its rows in the bone texture. */
struct FStrategyCrowdClip
{
    int32 StartRow = 0;
    int32 Frames = 1;
    float Length = 0.0f;
};

/**
 * The far soldiers (PROJECT 1864, the hybrid of baked and full animation): a soldier mesh (a reduced LOD) and his
 * rifle as one static mesh whose vertices carry their bones (bone indices in UV1-2, weights in UV3-4; the rifle
 * on a virtual bone), and the animations baked into a float texture of skinning matrices (three texels a bone,
 * a row a frame at 15 frames a second). M_CrowdVAT skins in the vertex shader; each instance's custom data says
 * which clip, from when, how fast and whether it loops, so every man moves on his own and the dead fall and stay
 * down. Baked once a world from the skeletal mesh's render data and the clips the component uses.
 */
UCLASS(Transient)
class STRATEGY1864_API UStrategyCrowdModel : public UObject
{
    GENERATED_BODY()

public:
    static constexpr float FramesPerSecond = 15.0f;
    static constexpr int32 CustomDataFloats = 4;

    /** The model for these (baked on first use and kept for the world); null when it cannot be baked. */
    static UStrategyCrowdModel* Find(UWorld* World, USkeletalMesh* Soldier, UStaticMesh* Rifle, const TArray<UAnimSequence*>& Clips, const FStrategyCrowdRifleGrip& Grip);

    UStaticMesh* GetMesh() const { return Mesh; }
    int32 GetNumMaterials() const { return Materials.Num(); }
    UMaterialInterface* GetMaterial(int32 Index) const;

    /** An instance's custom data: the clip's first row, its frames, the start time, and the frame rate (negative:
     *  play once and hold the last frame). Now/HeldPosition preserve a frozen fractional phase.
     *  False when the clip was not baked. */
    bool MakeCustomData(const UAnimSequence* Clip, float StartTime, float PlayRate, bool bLoop, float Out[CustomDataFloats], float Now = 0.f, float HeldPosition = 0.f) const;

private:
    bool Bake(UWorld* World, USkeletalMesh* Soldier, UStaticMesh* Rifle, const TArray<UAnimSequence*>& InClips, const FStrategyCrowdRifleGrip& Grip);

    UPROPERTY()
    TObjectPtr<UStaticMesh> Mesh;

    UPROPERTY()
    TObjectPtr<UTexture2D> BoneTexture;

    UPROPERTY()
    TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

    TMap<const UAnimSequence*, FStrategyCrowdClip> Clips;
};

/** The world's baked crowd models (one per soldier mesh and rifle). */
UCLASS()
class STRATEGY1864_API UStrategyCrowdSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TMap<FString, TObjectPtr<UStrategyCrowdModel>> Models;

    /** Keys that failed (not retried). */
    TSet<FString> Failed;
};
