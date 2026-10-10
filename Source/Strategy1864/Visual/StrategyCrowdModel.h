#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/Object.h"
#include "StrategyCrowdModel.generated.h"

class UAnimSequence;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;
class UTexture;
class USkeletalMeshComponent;
class UInstancedStaticMeshComponent;

USTRUCT()
struct FStrategyCrowdAuxGroup
{
    GENERATED_BODY()
    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> Instances;
    TMap<TWeakObjectPtr<USkeletalMeshComponent>, int32> Slots;
    TArray<int32> FreeSlots;
};

struct FStrategyCrowdAuxFigure
{
    float NearBlend = 0.f;
    uint64 BlendFrame = MAX_uint64;
    bool bLiving = true;
    bool bBaked = false;
};

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
USTRUCT()
struct FStrategyCrowdClip
{
    GENERATED_BODY()
    UPROPERTY()
    int32 StartRow = 0;
    UPROPERTY()
    int32 Frames = 1;
    UPROPERTY()
    float Length = 0.0f;
    UPROPERTY()
    TArray<FTransform> RiflePoses;
};

/** Persistent editor-baked infantry plus rifle. UV1-4 hold bones/weights, RGB vertex colours hold
 *  garment masks; the float texture stores three matrix texels per bone at 15 samples/second.
 *  Runtime loads this asset and gives every instance its own clip clock and uniform palette.
 */
UCLASS(BlueprintType)
class STRATEGY1864_API UStrategyCrowdModel : public UObject
{
    GENERATED_BODY()

public:
    static constexpr float FramesPerSecond = 15.0f;
    static constexpr int32 CustomDataFloats = 17; // clock, colours/switches, complementary coverage

    /** Editor Python entry point; creates a persistent model, but never saves or starts an editor. */
    UFUNCTION(BlueprintCallable, Category="Strategy|Crowd")
    static UStrategyCrowdModel* BakeAsset(UWorld* World, USkeletalMesh* Soldier, UStaticMesh* Rifle, const TArray<UAnimSequence*>& Animations);

    /** Load the saved model; null when missing/incomplete. No runtime baking. */
    static UStrategyCrowdModel* Find(UWorld* World, USkeletalMesh* Soldier, UStaticMesh* Rifle, const TArray<UAnimSequence*>& Clips, const FStrategyCrowdRifleGrip& Grip);

    UStaticMesh* GetMesh() const { return Mesh; }
    int32 GetNumMaterials() const { return Materials.Num(); }
    UMaterialInterface* GetMaterial(int32 Index) const;
    bool RiflePose(const UAnimSequence* Clip, float Position, bool bLoop, FTransform& Out) const;

    /** An instance's custom data: the clip's first row, its frames, the start time, and the frame rate (negative:
     *  play once and hold the last frame). Now/HeldPosition preserve a frozen fractional phase.
     *  False when the clip was not baked. */
    bool MakeCustomData(const UAnimSequence* Clip, float StartTime, float PlayRate, bool bLoop, float Out[CustomDataFloats], float Now = 0.f, float HeldPosition = 0.f) const;

private:
    static FString AssetPath(const USkeletalMesh* Soldier, const UStaticMesh* Rifle);
    void BindMaterials();
    bool Bake(UWorld* World, USkeletalMesh* Soldier, UStaticMesh* Rifle, const TArray<UAnimSequence*>& InClips, const FStrategyCrowdRifleGrip& Grip);

    UPROPERTY()
    TObjectPtr<UStaticMesh> Mesh;

    UPROPERTY()
    TObjectPtr<UTexture2D> BoneTexture;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

    UPROPERTY()
    TMap<TObjectPtr<UAnimSequence>, FStrategyCrowdClip> Clips;

    UPROPERTY()
    TArray<TObjectPtr<UTexture>> DiffuseTextures;

    UPROPERTY()
    int32 BakeVersion = 0;
};

/** The world's baked crowd models (one per soldier mesh and rifle). */
UCLASS()
class STRATEGY1864_API UStrategyCrowdSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Mounted/staff figures use the same saved assets and stable slots as infantry. */
    static bool DrawAuxiliary(USkeletalMeshComponent* Figure, USkeletalMesh* SourceMesh, UAnimSequence* Clip,
        float Position, bool bLiving = true, float Opacity = 1.f);
    static void RemoveAuxiliary(USkeletalMeshComponent* Figure);
    static int32 CountFallen(UWorld* World);
    void SweepAuxiliary();

    UPROPERTY()
    TMap<FString, TObjectPtr<UStrategyCrowdModel>> Models;

    UPROPERTY(Transient)
    TMap<FString, FStrategyCrowdAuxGroup> AuxiliaryGroups;
    TMap<TWeakObjectPtr<USkeletalMeshComponent>, FStrategyCrowdAuxFigure> AuxiliaryFigures;

    /** Keys that failed (not retried). */
    TSet<FString> Failed;

    uint64 NearSelectionFrame = MAX_uint64;
    TSet<TWeakObjectPtr<USkeletalMeshComponent>> NearFigures;
    int32 NearGeometryCount = 0;
    int32 NearBudget = 24;
};
