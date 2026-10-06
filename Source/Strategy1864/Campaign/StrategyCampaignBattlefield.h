#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyCampaignBattlefield.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMesh;

/**
 * The battlefield from the campaign (PROJECT 1864 Game1864): built from the generator's file
 * Saved/Battle/Battlefield_<Name>.json and its picture (.png). The ground at real scale (1 unit = 1 cm,
 * the height grid as it is), coloured from the picture, with collision so the units' terrain queries
 * stand on it; the roads, chausséer, lanes, tracks, railways and rivers as ribbons, the houses, farms and
 * churches, the woods, and the field boundaries (knicks, dikes, ditches). Battlefield x east, y north,
 * origin south-west; world +X north (the QA's "towards the enemy"), +Y east, centred on the actor.
 *
 * The look (the lit battle materials in /Game/Battle, from Content/Python/import_battle_graphics.py): the ground
 * with a tiled grass and dirt detail, spruces, pines and broadleaf trees of alpha cards, bushes and trees on the
 * knicks, post-and-rail fences along the lanes, and grass clumps in tiles round the camera (only near the
 * ground, kept off the ways, the water and the buildings). Without those assets it falls back to the campaign
 * map's unlit pieces.
 */
UCLASS()
class STRATEGY1864_API AStrategyCampaignBattlefield : public AActor
{
    GENERATED_BODY()

public:
    AStrategyCampaignBattlefield();
    virtual void Tick(float DeltaSeconds) override;

    /** Read the file (a full path, or a name under Saved/Battle) and build everything; false when it fails. */
    bool BuildFromFile(const FString& FileName);

    /** The test fields' ground instead of the flat QA box: a gently rolling meadow (grass in patches, a dirt
     *  track, a wood and copses round the edges, hedges, fences), SizeM square, with collision. */
    void BuildMeadow(float InSizeM, int32 Seed);

    /** The field's size in cm (a square), and the ground height at a world point (cm). */
    float GetSizeCm() const { return SizeCm; }
    float GroundZ(const FVector& World) const;

    /** Battlefield metres (x east, y north) to world. */
    FVector FieldToWorld(double XM, double YM, double LiftCm = 0.0) const;

    /** Open ground at a world point: not a town, a wood, water or the sea, not on a way, a knick or a house. */
    bool IsOpenGround(const FVector& World) const;

    /** Water at a world point (a river, a lake, the sea), and how wide the river there is (m; 0 when none). */
    bool IsWater(const FVector& World, float* OutWidthM = nullptr) const;

    /** A broad river (this wide or more) needs a bridge; a brook can be waded (design 46). */
    static constexpr float BroadRiverM = 15.0f;

    /** The way across the broad rivers between two points: the near and far end of each bridge to use (the one
     *  giving the shortest way), in order. False when a broad river lies across the way with no bridge. */
    bool RouteAcrossRivers(const FVector& Start, const FVector& End, TArray<FVector>& OutVia) const;

    /** The pace in water: 1 on dry ground and on the bridges, slow wading through a brook. */
    float WadingFactor(const FVector& World) const;

    /** The pioneers lay a pontoon bridge over the broad river nearest a point (within MaxDistanceCm); false when
     *  there is none that near. */
    bool LayPontoonBridge(const FVector& Near, float MaxDistanceCm);

    /** The bridges (world centre and length), for the map and the HUD. */
    int32 GetBridgeCount() const { return Bridges.Num(); }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Battlefield")
    FString Place;

    /** Grass clumps round the camera (-Strategy1864Grass=0 turns them off). */
    UPROPERTY(EditAnywhere, Category="Strategy|Battlefield")
    bool bStreamGrass = true;

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> Root;

    UPROPERTY()
    TObjectPtr<UProceduralMeshComponent> Ground;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UActorComponent>> Parts;

    int32 Grid = 256;
    float SizeCm = 800000.0f;
    float MinHeightM = 0.0f;
    TArray<float> HeightM;   // Grid x Grid, row 0 at the south
    FString KindsGrid;       // Grid x Grid, the generator's cell kinds (. field ~ sea m meadow w wood t town o water)

    /** The rivers (field metres) with their widths, for the crossings. */
    struct FRiver { TArray<FVector2D> Points; float WidthM = 12.0f; };
    TArray<FRiver> Rivers;

    /** A crossing of a river: the centre and the direction across (field metres), its length, the river. */
    struct FBridge { FVector2D Centre = FVector2D::ZeroVector; FVector2D Across = FVector2D(1.0, 0.0); float LengthM = 30.0f; int32 River = INDEX_NONE; bool bPontoon = false; };
    TArray<FBridge> Bridges;

    FVector2D ToField(const FVector& World) const;
    /** The river nearest a field point: its index, the distance (m) and the direction across it there. */
    int32 NearestRiver(const FVector2D& P, double& OutDistanceM, FVector2D& OutAcross) const;
    /** A bridge on the map (a deck over the water at the banks' height). */
    void AddBridgeMesh(const FBridge& Bridge, UMaterialInterface* Material);

    float HeightAtM(double XM, double YM) const;

    // ---- the look
    /** The ground's colours ((ColourN+1)^2, row 0 south) and where no grass grows (the ways, water, buildings). */
    TArray<FColor> GroundColours;
    int32 ColourN = 0;
    TBitArray<> NoGrass;
    int32 NoGrassN = 0;
    void ResetNoGrass();
    void MarkNoGrass(const TArray<FVector2D>& Line, double HalfWidthM);
    void MarkNoGrassDisc(const FVector2D& Centre, double RadiusM);
    /** 0..1: how much grass grows at a field point (from the ground colour, 0 on the marked ways). */
    float GrassAt(double XM, double YM) const;
    /** 0..1: ripe grain (the ochre of the ground picture: red over green, not the brown of ploughed land). */
    float CropAt(double XM, double YM) const;

    /** The battle's materials and foliage meshes (null when not imported). */
    UMaterialInterface* GroundMaterial() const;
    UMaterialInterface* SceneryMaterial(UMaterialInterface* Fallback) const;
    UMaterialInterface* WaterMaterial(UMaterialInterface* Fallback) const;

    /** Add instances of a mesh (one hierarchical instanced component), culled beyond CullCm (0: never). */
    UInstancedStaticMeshComponent* AddInstanced(UStaticMesh* Mesh, const TArray<FTransform>& Instances, bool bShadows, float CullCm, const TCHAR* Name);

    /** Trees, bushes and fences from the battle's meshes: woods (by kind), knick lines, fence lines. */
    void PlaceTree(TMap<UStaticMesh*, TArray<FTransform>>& Out, int32 Kind, const FVector& Local, float Scale, uint32 Hash) const;
    void PlaceKnick(TMap<UStaticMesh*, TArray<FTransform>>& Bushes, TMap<UStaticMesh*, TArray<FTransform>>& Trees, const TArray<FVector2D>& Line, uint32 Seed) const;
    void PlaceFence(TArray<FTransform>& Out, const TArray<FVector2D>& Line, double SideM, float GapChance, uint32 Seed) const;

    // ---- grass tiles round the camera
    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMesh>> GrassMeshes;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> GrassPool;

    TArray<TArray<int32>> FreeGrass;             // per grass mesh: free components in the pool
    TMap<FIntPoint, TArray<int32>> GrassTiles;   // tile -> components in the pool
    float GrassTimer = 0.0f;
    void SetupGrass();
    void UpdateGrass();
    void BuildGrassTile(const FIntPoint& Tile, TArray<int32>& Components);
};
