#pragma once

#include "CoreMinimal.h"

/**
 * Regiments on the 1851 campaign map (design manual 7: the unit model; its simulation numbers are
 * authoritative, the counters and miniatures only show them). They stand in their garrison towns and
 * march along the road network (Campaign1851Network): on foot at the road or chaussée pace, by train
 * where a railway is open, across the ferries.
 */

/** Arm of service; decides the counter symbol and the miniature. */
enum class ECampaign1851Arm : uint8
{
	Infantry,
	Guard,
	Jager,
	Cavalry,
	Artillery,        // foot artillery: horse-drawn guns, the gunners walk
	HorseArtillery    // horse artillery: every gunner mounted, keeps up with the cavalry
};

/** One stretch of a march: a link walked (or ridden by train) from one town to the next. */
struct FCampaign1851Leg
{
	int32 Link = INDEX_NONE;
	int32 From = INDEX_NONE;
	int32 To = INDEX_NONE;
	bool bRail = false;
	float Days = 0.f;
};

struct FCampaign1851Regiment
{
	FString Id;
	FString Name;
	FString Nation = TEXT("DK");
	ECampaign1851Arm Arm = ECampaign1851Arm::Infantry;

	int32 Men = 0;
	int32 MaxMen = 0;
	int32 Horses = 0;
	int32 Guns = 0;
	float Morale = 0.8f;

	int32 Home = INDEX_NONE;    // garrison town
	int32 Town = INDEX_NONE;    // where it stands; INDEX_NONE while marching

	/** March orders: the legs to go, the one under way and the time spent on it (days). */
	TArray<FCampaign1851Leg> Route;
	int32 Leg = 0;
	float LegElapsed = 0.f;
	/** Road pace of the column it marches with (km/day; the slowest arm and the column's length). */
	float PaceKmPerDay = 20.f;
	/** Regiments ordered together share a group and march as one column (0 = alone). */
	int32 Group = 0;
	FVector2D Km = FVector2D::ZeroVector;   // position on the map now (projected km)
	FVector2D Heading = FVector2D(1.0, 0.0);

	bool IsMarching() const { return Route.IsValidIndex(Leg); }
	int32 Destination() const { return Route.Num() > 0 ? Route.Last().To : Town; }
	/** Days left to the destination. */
	float DaysLeft() const
	{
		float Days = 0.f;
		for (int32 l = Leg; l < Route.Num(); ++l)
		{
			Days += Route[l].Days - (l == Leg ? LegElapsed : 0.f);
		}
		return FMath::Max(Days, 0.f);
	}
};

namespace Campaign1851Army
{
	ECampaign1851Arm ParseArm(const FString& Text);
	/** "Linjeinfanteri", "Garde", "Jægere", "Kavaleri", "Artilleri", "Ridende artilleri". */
	const TCHAR* ArmName(ECampaign1851Arm Arm);
	/** Marching pace of the arm on a road (km/day); a chaussée adds a third. */
	float MarchKmPerDay(ECampaign1851Arm Arm);
	/**
	 * Pace of a column (km/day on a road): the slowest arm in it, and a long column is slower still,
	 * 4 % per thousand men beyond two thousand, down to three quarters. OutWhy explains it.
	 */
	float ColumnPace(const TArray<const FCampaign1851Regiment*>& Column, FString* OutWhy = nullptr);
}
