#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Campaign1851Scenery.h"
#include "Campaign1851ConstructionSite.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

/** Where a town building goes (ACampaign1851Map::FindBuildingPlot). */
enum class ECampaign1851PlotRule : uint8
{
	Garrison,    // the garrison plot reserved when the map is built
	InTown,      // inside the town (houses on the plot are cleared)
	Edge,        // at the edge of town
	Outside,     // in the country, 1-3 km out
	Shore,       // on the shore, facing the sea
	Strategic    // a fixed strategic point (a fort)
};

/**
 * One building of a site, in piece units (the site is scaled by the map's scenery scale). Price,
 * days, upkeep and construction type come from Buildings1851.csv under Key.
 */
struct FCampaign1851SiteModule
{
	FString Key;
	FString Name;
	Campaign1851Scenery::ESitePiece Piece = Campaign1851Scenery::ESitePiece::Barracks;
	FVector2D Slot = FVector2D::ZeroVector;
	float Yaw = 0.f;
	float Length = 10.f, Width = 5.f, Eave = 5.f, Top = 8.f;
	FString Card;               // asset path of the card image for the town panel
	bool bScaffold = true;      // earthworks rise without scaffold or crane
	bool bCrane = true;
	bool bFlag = false;         // Dannebrog at FlagPos when finished
	FVector2D FlagPos = FVector2D::ZeroVector;
	// Town buildings only: where and when they may be built.
	ECampaign1851PlotRule Rule = ECampaign1851PlotRule::Garrison;
	float PlotRadiusKm = 0.2f;
	int32 MinPopulation = 0;
	bool bNeedsCoast = false;
	int32 FromYear = 0;

	int32 Cost() const;
	float Days() const;
	int32 Upkeep() const;
	FString Type() const;
	/** Money per full working day once the down payment is made. */
	double CostPerDay() const;
};

/**
 * A building project on the campaign map, shown as a live building site (design manual 20.16.6
 * project model, and the modular barracks idea in 20.16.5). Either a garrison complex (the infantry
 * barracks first, then stables, depot and infirmary around the parade ground, one at a time) or a
 * single town building (arsenal, lazaret, coastal battery, ...).
 *
 * Each module goes through the same stages: staking out, foundation (scaffold goes up), walls
 * rising, roof, fitting out (scaffold comes down). The building "grows" through
 * M_Campaign1851Construction, which clips everything above BuildTop. A wagon shuttles materials
 * between the town and the site and a timber crane swings while walls and roofs go up; Dannebrog
 * is hoisted over a finished barracks, battery or fort.
 */
UCLASS()
class GAME1864_API ACampaign1851ConstructionSite : public AActor
{
	GENERATED_BODY()

public:
	ACampaign1851ConstructionSite();

	/** The garrison complex: barracks (module 0), stables, depot, infirmary. */
	static const TArray<FCampaign1851SiteModule>& GarrisonModules();
	/** Buildings a town can raise on their own plots. */
	static const TArray<FCampaign1851SiteModule>& TownBuildings();
	static const FCampaign1851SiteModule* FindTownBuilding(const FString& Key);

	/** Places the site and starts module 0. WagonPath runs from the town centre to the gate (world space). */
	void Setup(int32 InCityIndex, const TArray<FVector>& InWagonPath, UMaterialInterface* Material, const TArray<FCampaign1851SiteModule>& InModules, bool bInGarrison);

	/**
	 * Driven by the campaign calendar (ACampaign1851Map::AdvanceTime): DeltaDays of work, and
	 * DeltaSeconds for the animations (0 while the game is paused, so the site freezes too).
	 */
	void Advance(float DeltaDays, float DeltaSeconds);

	bool IsGarrison() const { return bGarrison; }
	/** "Garrison" or the town building's key. */
	FString GetKind() const { return bGarrison ? FString(TEXT("Garrison")) : Modules[0].Key; }
	int32 NumModules() const { return Modules.Num(); }
	const FCampaign1851SiteModule& GetModule(int32 Module) const { return Modules[FMath::Clamp(Module, 0, Modules.Num() - 1)]; }
	FString ModuleName(int32 Module) const { return GetModule(Module).Name; }
	float ModuleDays(int32 Module) const { return GetModule(Module).Days(); }
	int32 ModuleCost(int32 Module) const { return GetModule(Module).Cost(); }
	FString ModuleType(int32 Module) const { return GetModule(Module).Type(); }
	double ModuleCostPerDay(int32 Module) const { return GetModule(Module).CostPerDay(); }

	/** Garrison only: true when the barracks is finished, nothing else is being built and the module is not built yet. */
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

	/** Garrison: barracks finished. Town building: the building finished. */
	bool IsBarracksDone() const { return IsModuleDone(0); }

	/** Days built per module (-1 = not started), for saving. */
	const TArray<float>& GetModuleDaysBuilt() const { return Elapsed; }
	/** Restores a saved state (after Setup); missing modules count as not started. */
	void RestoreState(const TArray<float>& InDays, int32 InActive);

	/** Set by the map's treasury when there was no money for the day's work. */
	void SetStalled(bool bInStalled) { bStalled = bInStalled; }
	bool IsStalled() const { return bStalled; }
	/** Upkeep per year of the finished modules. */
	int32 GetYearlyUpkeep() const;
	/** Pulling it down: the modules come down over the days, the labourers paid by the map. */
	void StartDemolition(float Days, float Wages)
	{
		bDemolishing = true;
		DemolishDays = FMath::Max(Days, 1.f);
		DemolishWages = Wages;
		DemolishDone = 0.f;
		DemolishFrom = Elapsed;
		Active = INDEX_NONE;
	}
	/** Restores a demolition under way from a save. */
	void RestoreDemolition(float Days, float Wages, float Done, const TArray<float>& From)
	{
		bDemolishing = true;
		DemolishDays = FMath::Max(Days, 1.f);
		DemolishWages = Wages;
		DemolishDone = 0.f;
		DemolishFrom = From;
		AdvanceDemolition(Done);
	}
	/** Work on the demolition; true when it is down. */
	bool AdvanceDemolition(float Work)
	{
		DemolishDone += Work;
		const float Left = FMath::Clamp(1.f - DemolishDone / DemolishDays, 0.f, 1.f);
		for (int32 m = 0; m < Elapsed.Num(); ++m)
		{
			Elapsed[m] = DemolishFrom.IsValidIndex(m) && DemolishFrom[m] >= 0.f ? DemolishFrom[m] * Left : -1.f;
		}
		Apply(0.f);
		return DemolishDone >= DemolishDays;
	}
	bool IsDemolishing() const { return bDemolishing; }
	float GetDemolishDays() const { return DemolishDays; }
	float GetDemolishWages() const { return DemolishWages; }
	float GetDemolishDone() const { return DemolishDone; }
	const TArray<float>& GetDemolishFrom() const { return DemolishFrom; }
	/** Raised by private investors: the state pays neither wages nor upkeep. */
	void SetPrivate(bool bIn) { bPrivate = bIn; }
	bool IsPrivate() const { return bPrivate; }

	/** Plot centre (projected km) and radius, for saving and for keeping plots apart. */
	FVector2D PlotKm = FVector2D::ZeroVector;
	float PlotRadiusKm = 0.2f;

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

	TArray<FCampaign1851SiteModule> Modules;
	bool bGarrison = true;
	FVector FlagSpot = FVector::ZeroVector;
	int32 CityIndex = INDEX_NONE;
	TArray<float> Elapsed;   // days per module; -1 = not started
	int32 Active = INDEX_NONE;
	float Clock = 0.f;
	bool bStalled = false;
	bool bPrivate = false;
	bool bDemolishing = false;
	float DemolishDays = 1.f, DemolishWages = 0.f, DemolishDone = 0.f;
	TArray<float> DemolishFrom;

	TArray<FVector> WagonPath;
	TArray<float> WagonDistance;   // cumulative, world units
	float WagonAt = 0.f;
	float WagonDirection = 1.f;
	float WagonPause = 0.f;
};
