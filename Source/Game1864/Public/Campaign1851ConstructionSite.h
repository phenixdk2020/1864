#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Campaign1851ConstructionSite.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * A building project on the campaign map, shown as a live building site (design manual 20.16.6:
 * proposed -> under construction -> finished). v1: the infantry barracks.
 *
 * Stages by progress: staking out, foundation (scaffold goes up), walls rising storey by storey,
 * roof, finishing (scaffold comes down, Dannebrog is hoisted). A wagon shuttles materials between
 * the town and the site and a timber crane swings while the walls and roof go up. The building
 * "grows" through M_Campaign1851Construction, which clips everything above BuildTop.
 *
 * The actor is scaled by the map's scenery scale, so its components are modelled in piece units.
 */
UCLASS()
class GAME1864_API ACampaign1851ConstructionSite : public AActor
{
	GENERATED_BODY()

public:
	ACampaign1851ConstructionSite();

	virtual void Tick(float DeltaSeconds) override;

	/** Campaign days that pass per real second (a stand-in until the campaign has a clock). */
	UPROPERTY(EditAnywhere, Category = "Construction")
	float DaysPerSecond = 3.f;

	UPROPERTY(EditAnywhere, Category = "Construction")
	float DurationDays = 90.f;

	/** Places the site. WagonPath runs from the town centre to the site gate (world space, on the terrain). */
	void Setup(int32 InCityIndex, const FString& InName, const TArray<FVector>& InWagonPath, UMaterialInterface* Material);

	int32 GetCityIndex() const { return CityIndex; }
	const FString& GetBuildingName() const { return BuildingName; }
	float GetProgress() const { return FMath::Clamp(ElapsedDays / DurationDays, 0.f, 1.f); }
	float GetElapsedDays() const { return FMath::Min(ElapsedDays, DurationDays); }
	bool IsDone() const { return ElapsedDays >= DurationDays; }
	/** Danish stage name for the UI. */
	FString GetStageName() const;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ground;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Building;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Scaffold;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CraneMast;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CraneJib;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Wagon;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Flagpole;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Flag;

private:
	void Apply(float DeltaSeconds);
	void MoveWagon(float DeltaSeconds);
	/** Clip height in piece units above the site origin -> world Z for the material. */
	void SetClip(UMaterialInstanceDynamic* Mid, float PieceHeight) const;

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BuildingMid;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ScaffoldMid;

	int32 CityIndex = INDEX_NONE;
	FString BuildingName;
	float ElapsedDays = 0.f;
	float Clock = 0.f;

	TArray<FVector> WagonPath;
	TArray<float> WagonDistance;   // cumulative, world units
	float WagonAt = 0.f;
	float WagonDirection = 1.f;
	float WagonPause = 0.f;
};
