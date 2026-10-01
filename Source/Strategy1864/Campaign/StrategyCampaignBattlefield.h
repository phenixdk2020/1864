#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyCampaignBattlefield.generated.h"

class UProceduralMeshComponent;

/**
 * The battlefield from the campaign (PROJECT 1864 Game1864): built from the generator's file
 * Saved/Battle/Battlefield_<Name>.json and its picture (.png). The ground at real scale (1 unit = 1 cm,
 * the height grid as it is), coloured from the picture, with collision so the units' terrain queries
 * stand on it; the roads, chausséer, lanes, tracks, railways and rivers as ribbons, the houses, farms and
 * churches, the woods, and the field boundaries (knicks, dikes, ditches). Battlefield x east, y north,
 * origin south-west; world +X north (the QA's "towards the enemy"), +Y east, centred on the actor.
 */
UCLASS()
class STRATEGY1864_API AStrategyCampaignBattlefield : public AActor
{
    GENERATED_BODY()

public:
    AStrategyCampaignBattlefield();

    /** Read the file (a full path, or a name under Saved/Battle) and build everything; false when it fails. */
    bool BuildFromFile(const FString& FileName);

    /** The field's size in cm (a square), and the ground height at a world point (cm). */
    float GetSizeCm() const { return SizeCm; }
    float GroundZ(const FVector& World) const;

    /** Battlefield metres (x east, y north) to world. */
    FVector FieldToWorld(double XM, double YM, double LiftCm = 0.0) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Battlefield")
    FString Place;

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

    float HeightAtM(double XM, double YM) const;
};
