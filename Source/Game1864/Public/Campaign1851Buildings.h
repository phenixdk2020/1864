#pragma once

#include "CoreMinimal.h"

/**
 * Building types of the 1851 campaign: price, build time, upkeep and requirements, from
 * Data/Campaign1851/Buildings1851.csv (written by Tools/Campaign/building_costs.py).
 */
struct FCampaign1851BuildingDef
{
	FString Key;
	FString Name;
	FString Category;
	FString Owner;      // "Stat (militær)", "Stat", "By", "Privat"
	FString Size;       // XS .. XXL
	FString Type;       // bindingsværk, grundmur, industri, jordværk, stenværk
	int32 CostRd = 0;
	int32 Days = 0;
	int32 UpkeepRdPerYear = 0;
	FString Requires;
	FString Provides;
	FString Image;
};

namespace Campaign1851Buildings
{
	/** Loads the table once (safe to call again). */
	bool Load();
	const FCampaign1851BuildingDef* Find(const FString& Key);
	const TArray<FCampaign1851BuildingDef>& All();

	/** Share of the building price paid when the work starts (materials are ordered). */
	constexpr float DownPayment = 0.2f;

	/**
	 * Work speed for a construction type at a date: frost halves masonry, nearly stops
	 * earthworks and slows timber framing (December to February).
	 */
	float WorkRate(const FString& Type, const FDateTime& Date);
}
