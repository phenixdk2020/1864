#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Campaign1851SaveGame.generated.h"

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
 * ACampaign1851PlayerController::ApplySave.
 */
UCLASS()
class GAME1864_API UCampaign1851SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY() int32 SaveVersion = CurrentVersion;
	UPROPERTY() FDateTime SavedAt;
	/** One line for the load menu, e.g. "Aalborg: kaserne, stalde". */
	UPROPERTY() FString Summary;

	UPROPERTY() FVector CameraTarget = FVector::ZeroVector;
	UPROPERTY() float CameraDistanceKm = 960.f;
	UPROPERTY() float CameraYaw = 0.f;
	UPROPERTY() FString SelectedCity;

	UPROPERTY() TArray<FCampaign1851ProjectSave> Projects;
};
