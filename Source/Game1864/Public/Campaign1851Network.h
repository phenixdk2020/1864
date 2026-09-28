#pragma once

#include "CoreMinimal.h"

/**
 * Roads and railways of the 1851 campaign map (design manual 20.16.1: infrastructure is made of
 * connections, not regional buffs). The network is a set of links between neighbouring towns,
 * routed by tools/map1851/build_map.py; each link can be improved by a project: a paved chaussée
 * instead of the dirt road, or a railway along its own alignment.
 */

/** Work on a link. */
enum class ECampaign1851LinkWork : uint8
{
	None,
	Chaussee,   // pave the main road ("chaussé", macadam): faster marches in all weather
	Railway     // lay a railway between the two towns, with a station at each end
};

/** A main road between two neighbouring towns, possibly with a ferry crossing on the way. */
struct FCampaign1851Link
{
	int32 A = INDEX_NONE;
	int32 B = INDEX_NONE;
	float RoadKm = 0.f;
	float FerryKm = 0.f;
	FString Ferry;                 // name of the crossing, empty for a road over land
	TArray<FVector2D> Km;          // the road, projected km, A to B
	TArray<FVector2D> RailPath;    // where a railway between A and B would run; empty if none can be laid
	float RailKm = 0.f;

	bool bChaussee = false;        // paved now
	bool bRailway = false;         // an open railway joins A and B
	bool bHistoricChaussee = false;

	/** The project under way on this link (one at a time). */
	ECampaign1851LinkWork Work = ECampaign1851LinkWork::None;
	float DaysBuilt = 0.f;
	float WorkDays = 1.f;
	int32 WorkCost = 0;
	bool bStalled = false;

	bool HasFerry() const { return FerryKm > 0.f; }
	float Progress() const { return Work == ECampaign1851LinkWork::None ? 0.f : FMath::Clamp(DaysBuilt / FMath::Max(WorkDays, 1.f), 0.f, 1.f); }
};

/** An open (or historically building) railway line, drawn on the map with trains running on it. */
struct FCampaign1851Railway
{
	FString Name;
	TArray<FString> Stations;      // in order, including stops that are not map towns
	TArray<int32> Towns;           // map towns on the line, in order
	TArray<FVector2D> Km;          // the line, projected km
	float LengthKm = 0.f;
	FDateTime Begun;               // historical lines only; equal to Opened when already open in 1851
	FDateTime Opened;
	int32 Link = INDEX_NONE;       // a line the player built on a link
	bool bAnnounced = false;       // opening reported to the player

	bool IsOpen(const FDateTime& Now) const { return Now >= Opened; }
	float BuildProgress(const FDateTime& Now) const
	{
		const double Span = (Opened - Begun).GetTotalDays();
		return Span <= 0.0 ? 1.f : float(FMath::Clamp((Now - Begun).GetTotalDays() / Span, 0.0, 1.0));
	}
};

namespace Campaign1851Network
{
	/** Paving a main road: rigsdaler per km, and working days (earthworks: slow in frost). */
	constexpr double ChausseeRdPerKm = 2500.0;
	constexpr float ChausseeDaysBase = 40.f;
	constexpr float ChausseeDaysPerKm = 2.5f;
	constexpr double ChausseeUpkeepPerKm = 40.0;
	/**
	 * The state's share of a railway per km: lines of the 1850s were built by concession companies
	 * with private (largely English) capital; the state paid land, a share and an interest guarantee.
	 */
	constexpr double RailwayRdPerKm = 12000.0;
	constexpr float RailwayDaysBase = 150.f;
	constexpr float RailwayDaysPerKm = 6.5f;
	constexpr double RailwayUpkeepPerKm = 150.0;
	/** Marching speeds (km per day) and a train journey's loading time. */
	constexpr float MarchKmPerDayRoad = 20.f;
	constexpr float MarchKmPerDayChaussee = 27.f;
	constexpr float TrainKmPerDay = 300.f;
	constexpr float TrainLoadingDays = 1.f;
	constexpr float FerryDays = 0.5f;
	/** Construction type for Campaign1851Buildings::WorkRate. */
	inline const TCHAR* WorkType() { return TEXT("jordværk"); }
	const TCHAR* WorkName(ECampaign1851LinkWork Work);
}
