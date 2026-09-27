#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Campaign1851PlayerController.generated.h"

class ACampaign1851Map;
class ACampaign1851Camera;
class SCampaign1851Overlay;

/**
 * Input for the 1851 campaign map: WASD/arrows or right/middle drag pans, the wheel zooms towards
 * the cursor, Q/E rotates, Home resets, left click selects a city. Owns the Slate overlay.
 *
 * Console / command line: CampaignView <lat> <lon> <distanceKm> [yaw] jumps the camera, e.g.
 * "CampaignView 57.05 9.92 25" for Aalborg close up; -CampaignView=57.05,9.92,25 does the same at start.
 */
UCLASS()
class GAME1864_API ACampaign1851PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACampaign1851PlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(Exec)
	void CampaignView(float Lat, float Lon, float DistanceKm, float Yaw = 0.f);

private:
	bool CursorGround(FVector& Out) const;
	bool ScreenGround(const FVector2D& Screen, FVector& Out) const;
	void TryInit();
	void PickCity();

	TWeakObjectPtr<ACampaign1851Map> Map;
	TSharedPtr<SCampaign1851Overlay> Overlay;
	FVector2D LastMouse = FVector2D::ZeroVector;
	bool bInitialised = false;
};
