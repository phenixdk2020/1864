#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Campaign1851SaveGame.generated.h"

/** A line in the state's account book. */
USTRUCT()
struct FCampaign1851Transaction
{
	GENERATED_BODY()

	UPROPERTY() FDateTime Date;
	UPROPERTY() double Amount = 0.0;   // rigsdaler; negative = spent
	UPROPERTY() FString Text;
};

/** One town's garrison complex: days built per module (-1 = not started) and the module under way. */
USTRUCT()
struct FCampaign1851ProjectSave
{
	GENERATED_BODY()

	/** Town name rather than index, so saves survive changes to the town list. */
	UPROPERTY() FString City;
	UPROPERTY() TArray<float> ModuleDays;
	UPROPERTY() int32 ActiveModule = INDEX_NONE;
	/** "Garrison" (or empty in old saves) or a town building key; its plot and heading. */
	UPROPERTY() FString Kind;
	UPROPERTY() FVector2D PlotKm = FVector2D::ZeroVector;
	UPROPERTY() float Yaw = 0.f;
};

/** A link of the road network with work done on it: paved, railway built, or a project under way. */
USTRUCT()
struct FCampaign1851LinkSave
{
	GENERATED_BODY()

	/** The link's towns by name. */
	UPROPERTY() FString A;
	UPROPERTY() FString B;
	UPROPERTY() bool bChaussee = false;
	UPROPERTY() bool bRailway = false;
	/** ECampaign1851LinkWork of the project under way (0 = none), its days done, and what it costs. */
	UPROPERTY() uint8 Work = 0;
	UPROPERTY() float DaysBuilt = 0.f;
};

/** A regiment's state: where it stands or which stretch it is on, where it is going, its strength. */
USTRUCT()
struct FCampaign1851RegimentSave
{
	GENERATED_BODY()

	UPROPERTY() FString Id;
	/** The town it stands in, or the town the current stretch started from. */
	UPROPERTY() FString Town;
	/** Marching: the end of the current stretch, the days spent on it and the final destination. */
	UPROPERTY() FString LegTo;
	UPROPERTY() float LegElapsed = 0.f;
	UPROPERTY() FString Destination;
	UPROPERTY() int32 Men = 0;
	UPROPERTY() float Morale = 0.8f;
	/** Pace (km/day) and column of the march under way. */
	UPROPERTY() float Pace = 0.f;
	UPROPERTY() int32 Group = 0;
};

/**
 * A saved 1851 campaign (Saved/SaveGames/<slot>.sav). Holds the construction projects, the
 * camera view and the selected town. Bump SaveVersion when the layout changes and migrate in
 * ACampaign1851PlayerController::LoadFromSlot.
 */
UCLASS()
class GAME1864_API UCampaign1851SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 1: projects and view. 2: + campaign date and speed. 3: + treasury and account book. 4: + town buildings. 5: + roads and railways. 6: + regiments. */
	static constexpr int32 CurrentVersion = 6;

	UPROPERTY() int32 SaveVersion = CurrentVersion;
	UPROPERTY() FDateTime SavedAt;
	/** One line for the load menu, e.g. "Aalborg: kaserne, stalde". */
	UPROPERTY() FString Summary;

	UPROPERTY() FVector CameraTarget = FVector::ZeroVector;
	UPROPERTY() float CameraDistanceKm = 960.f;
	UPROPERTY() float CameraYaw = 0.f;
	UPROPERTY() FString SelectedCity;

	UPROPERTY() TArray<FCampaign1851ProjectSave> Projects;

	/** Days since 1 July 1851, and the game speed step. */
	UPROPERTY() double CampaignDays = 0.0;
	UPROPERTY() int32 Speed = 1;

	/** The state's cash (rigsdaler) and its recent account book. */
	UPROPERTY() double Treasury = 0.0;
	UPROPERTY() TArray<FCampaign1851Transaction> Ledger;

	/** Chausséer and railways built by the player, and link projects under way. */
	UPROPERTY() TArray<FCampaign1851LinkSave> Links;

	/** The regiments (missing ones stay in their garrisons). */
	UPROPERTY() TArray<FCampaign1851RegimentSave> Regiments;
};
