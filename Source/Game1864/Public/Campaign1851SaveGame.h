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
	/** 1: projects and view. 2: + campaign date and speed. 3: + treasury and account book. */
	static constexpr int32 CurrentVersion = 3;

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
};
