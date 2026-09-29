#pragma once

#include "CoreMinimal.h"

/**
 * Field fortifications (skanser) the player raises anywhere on the monarchy's land: a small lunette or a
 * large closed redoubt, armed with fortress guns and strengthened step by step. Their data goes to the 3D
 * battles (Saved/Battle/Fortifications.json, Docs/Fortifications1851.md).
 *
 * Historical frame: the Dybbøl position of 1864 had ten redoubts (seven large, three small) with some
 * 66 guns and 11 mortars; the strongest, Redoubt IV, had 12 guns. Garrison sizes are estimates.
 */
enum class ECampaign1851FortWork : uint8
{
	None,
	Build,     // raising the earthwork (with its first guns)
	Guns,      // two more gun platforms and guns
	Defence    // the next level of defence
};

struct FCampaign1851Fort
{
	int32 Id = 0;
	FString Name;
	FVector2D Km = FVector2D::ZeroVector;
	float Yaw = 0.f;             // world yaw of the front (the side facing the enemy)
	bool bLarge = false;
	int32 Guns = 0;              // guns in place
	int32 Defence = 1;           // 1..4 (Campaign1851Forts::DefenceName)
	int32 Garrison = 0;          // infantry in it now (the battles read it); up to InfantryCapacity
	bool bBuilt = false;
	ECampaign1851FortWork Work = ECampaign1851FortWork::None;
	float DaysBuilt = 0.f;
	float WorkDays = 1.f;
	int32 WorkCost = 0;
	bool bStalled = false;

	float Progress() const { return Work == ECampaign1851FortWork::None ? 0.f : FMath::Clamp(DaysBuilt / FMath::Max(WorkDays, 1.f), 0.f, 1.f); }
};

namespace Campaign1851Forts
{
	/** Guns a fort can take, and those it is built with. */
	inline int32 MaxGuns(bool bLarge) { return bLarge ? 12 : 4; }
	inline int32 StartGuns(bool bLarge) { return bLarge ? 6 : 2; }
	constexpr int32 GunStep = 2;
	/** Price and days of the earthwork (engineers and hired labourers; frost slows it as any earthwork). */
	inline int32 BuildCost(bool bLarge) { return bLarge ? 28000 : 9000; }
	inline float BuildDays(bool bLarge) { return bLarge ? 90.f : 45.f; }
	/** Two more gun platforms with their fortress guns. */
	constexpr int32 GunsCost = 3600;
	constexpr float GunsDays = 15.f;
	constexpr int32 MaxDefence = 4;
	/** "Brystværn og grav", "Palisader og ulvegrave", "Bombesikkert blokhus", "Traverser og bombesikre magasiner". */
	const TCHAR* DefenceName(int32 Level);
	/** What the next level adds, for the player. */
	const TCHAR* DefenceNote(int32 Level);
	inline int32 DefenceCost(int32 Level, bool bLarge) { static const int32 C[] = { 0, 0, 3000, 6000, 9000 }; return C[FMath::Clamp(Level, 0, 4)] * (bLarge ? 2 : 1); }
	inline float DefenceDays(int32 Level) { static const float D[] = { 0.f, 0.f, 20.f, 35.f, 50.f }; return D[FMath::Clamp(Level, 0, 4)]; }
	/** Cover for the men inside against fire (0-100 %), by level. */
	inline int32 CoverPercent(int32 Level) { static const int32 C[] = { 0, 40, 55, 70, 85 }; return C[FMath::Clamp(Level, 0, 4)]; }
	/** Parapet height and ditch depth in metres. */
	inline float ParapetHeightM(bool bLarge, int32 Level) { return (bLarge ? 3.2f : 2.5f) + (Level >= 4 ? 0.3f : 0.f); }
	inline float DitchDepthM(bool bLarge) { return bLarge ? 2.8f : 2.0f; }
	/** Infantry the fort holds (estimate: half a company, a company), and gunners per gun. */
	inline int32 InfantryCapacity(bool bLarge) { return bLarge ? 250 : 120; }
	constexpr int32 GunnersPerGun = 7;
	/** Upkeep a year (repairs, guards, powder kept dry). */
	inline int32 UpkeepPerYear(bool bLarge) { return bLarge ? 800 : 300; }
	const TCHAR* GunType();
	/** Gun platforms in the fort's own frame (piece units, front towards +X) and the way each gun points. */
	void GunSlots(bool bLarge, TArray<FVector2f>& OutPos, TArray<float>& OutYaw);
	/** "N", "NØ", "Ø", ... for a world yaw (0 = east on the map, 90 = south). */
	FString Compass(float Yaw);
}
