#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Campaign1851ConstructionSite.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * A garrison complex on the campaign map, grown as a live building site (design manual 20.16.6
 * project model, and the modular barracks idea in 20.16.5): the infantry barracks first, then
 * modules around the parade ground (stables, depot, infirmary), one project at a time.
 *
 * Each module goes through the same stages: staking out, foundation (scaffold goes up), walls
 * rising, roof, fitting out (scaffold comes down). The building "grows" through
 * M_Campaign1851Construction, which clips everything above BuildTop. A wagon shuttles materials
 * between the town and the site and a timber crane swings while walls and roofs go up; Dannebrog
 * is hoisted when the barracks is finished.
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

	/** Module 0 is the barracks; the others need it finished. */
	static int32 NumModules();
	static FString ModuleName(int32 Module);
	/** Asset path of the module's card image for the town panel. */
	static const TCHAR* ModuleCard(int32 Module);
	static float ModuleDays(int32 Module);

	/** Places the site and starts the barracks. WagonPath runs from the town centre to the gate (world space). */
	void Setup(int32 InCityIndex, const TArray<FVector>& InWagonPath, UMaterialInterface* Material);

	/** True when the barracks is finished, nothing else is being built and the module is not built yet. */
	bool CanStartModule(int32 Module) const;
	void StartModule(int32 Module);

	int32 GetCityIndex() const { return CityIndex; }
	/** The module under construction, or INDEX_NONE. */
	int32 GetActiveModule() const { return Active; }
	bool IsModuleStarted(int32 Module) const { return Elapsed.IsValidIndex(Module) && Elapsed[Module] >= 0.f; }
	bool IsModuleDone(int32 Module) const { return IsModuleStarted(Module) && Elapsed[Module] >= ModuleDays(Module); }
	float GetModuleProgress(int32 Module) const;
	float GetModuleElapsedDays(int32 Module) const { return IsModuleStarted(Module) ? FMath::Min(Elapsed[Module], ModuleDays(Module)) : 0.f; }
	/** Danish stage name for the UI. */
	FString GetStageName(int32 Module) const;

	bool IsBarracksDone() const { return IsModuleDone(0); }

	/** Days built per module (-1 = not started), for saving. */
	const TArray<float>& GetModuleDaysBuilt() const { return Elapsed; }
	/** Restores a saved state (after Setup); missing modules count as not started. */
	void RestoreState(const TArray<float>& InDays, int32 InActive);

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ground;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CraneMast;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CraneJib;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Wagon;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Flagpole;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Flag;

	/** Per module: the building and its scaffold (created in Setup). */
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Buildings;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Scaffolds;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> BuildingMids;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> ScaffoldMids;

private:
	void Apply(float DeltaSeconds);
	void MoveWagon(float DeltaSeconds);
	/** Clip height in piece units above the site origin -> world Z for the material. */
	void SetClip(UMaterialInstanceDynamic* Mid, float PieceHeight) const;

	int32 CityIndex = INDEX_NONE;
	TArray<float> Elapsed;   // days per module; -1 = not started
	int32 Active = INDEX_NONE;
	float Clock = 0.f;

	TArray<FVector> WagonPath;
	TArray<float> WagonDistance;   // cumulative, world units
	float WagonAt = 0.f;
	float WagonDirection = 1.f;
	float WagonPause = 0.f;
};
