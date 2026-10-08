#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Campaign1851Network.h"
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

	/** Saves / loads the campaign: "CampaignSave Slot1", "CampaignLoad Autosave". */
	UFUNCTION(Exec)
	void CampaignSave(const FString& Slot);
	UFUNCTION(Exec)
	void CampaignLoad(const FString& Slot);
	/** Starts over: removes every building project, resets the view and overwrites the autosave. */
	UFUNCTION(Exec)
	void CampaignNewGame();

	/** Test money: sets the treasury, e.g. "CampaignMoney 5000000". */
	UFUNCTION(Exec)
	void CampaignMoney(float Amount);

	/** Starts the barracks in a town and flies to its building site, e.g. "CampaignBuild Aalborg". */
	UFUNCTION(Exec)
	void CampaignBuild(const FString& CityName);

private:
	bool CursorGround(FVector& Out) const;
	bool ScreenGround(const FVector2D& Screen, FVector& Out) const;
	void TryInit();
	void PickCity();
	/** Town marker under the cursor (within ~16 px), or INDEX_NONE. */
	int32 CityUnderCursor() const;
	/** The regiments that stand or march together with this one (its town's stack, or its column). */
	TArray<int32> StackOf(int32 Regiment) const;
	void SelectRegiments(const TArray<int32>& Regiments);
	/** Sends the selected regiments as one column to a town, or to a point in the field (CityIndex = INDEX_NONE). */
	void MarchSelected(int32 CityIndex, const FVector2D& TargetKm);
	/** Opens the march order dialog for the selection towards a town or a point. */
	void OpenOrderDialog(int32 CityIndex, const FVector2D& TargetKm);
	/** Works out the dialog's times and columns again (after a change of ways). */
	void RefreshOrderDialog();
	/** Orders the dialog's columns: every way its own column. */
	void ExecuteOrderDialog();
	/** Order-of-battle tree: a row pressed (click or the start of a drag). */
	int32 TreePressKey = INDEX_NONE;
	FVector2D TreePressAt = FVector2D::ZeroVector;
	bool bTreeDragging = false;
	void TreeClick(int32 Key);
	void TreeDrop(int32 Source, int32 Target);

	/** For testing: puts the garrisons into a field army of two divisions and a reserve. */
	UFUNCTION(Exec)
	void CampaignTestFieldArmy();

	/** Right mouse: a click (not a drag) gives the march order. */
	FVector2D RightDownAt = FVector2D::ZeroVector;
	bool bRightDragged = false;
	void FocusPlot(int32 CityIndex);
	/** Flies the camera to a building site, looking at its front. */
	void FocusSite(const class ACampaign1851ConstructionSite* Site);
	/** Starts a chaussée or railway on a link from the panel; a message if it cannot. */
	void BuildLink(int32 Link, ECampaign1851LinkWork Work);
	/** Flies the camera over a link. */
	void FocusLink(int32 Link);
	/** Starts a town building from the panel; a message if it cannot. */
	void BuildTownBuilding(int32 CityIndex, const FString& Key);

	/** Writes the campaign to a save slot; bQuiet skips the on-screen message (autosave). */
	bool SaveToSlot(const FString& Slot, bool bQuiet = false);
	bool LoadFromSlot(const FString& Slot);
	void OpenGameMenu();
	FString SlotLabel(const FString& Slot) const;

	/** Save slots in menu order. */
	static const TArray<FString>& SaveSlots();
	bool bCampaignStarted = false;
	// -CampaignAutoClick=seconds:x:y;seconds:x:y: left clicks made by the game itself (QA of the clicks), at the given pixels.
	bool bAutoMouse = false;
	FVector2D AutoMouse = FVector2D::ZeroVector;
	float AutoReleaseAt = -1.f;
	int32 NextAutoClick = 0;
	bool bAutoClicksParsed = false;
	TArray<FVector> AutoClicks;
	bool bAutoClickFrame = false;   // the tick of an automatic click
	bool LeftJustPressed() const { return bAutoClickFrame || WasInputKeyJustPressed(EKeys::LeftMouseButton); }
	bool PointerPosition(float& X, float& Y) const;
	void TickAutoClicks();
	float AutosaveTimer = 0.f;
	/** Test: a town building to put the camera on once it stands (-CampaignFocusBuilding=Town:Key). */
	FString FocusBuildingOrder;
	/** Test: resolve battles automatically as they come. */
	bool bAutoBattles = false;
	/** Back from a 3D battle: the test flags of the command line are not applied again. */
	bool bResumedFromBattle = false;
	/** The fort under the mouse (id, 0 = none). */
	int32 FortUnderCursor() const;
	/** Speed to return to when un-pausing with the space bar. */
	int32 SpeedBeforePause = 1;

	/** Last camera view, so the autosave on exit works after the camera pawn is gone. */
	FVector LastCameraTarget = FVector::ZeroVector;
	float LastCameraDistanceKm = 960.f;
	float LastCameraYaw = 0.f;

	TWeakObjectPtr<ACampaign1851Map> Map;
	TSharedPtr<SCampaign1851Overlay> Overlay;
	FVector2D LastMouse = FVector2D::ZeroVector;
	bool bInitialised = false;

	/** Onto the battlefield model (and back to the map view it came from). */
	void SetBattleView(bool bEnter);
	/** The map view to come back to from the battlefield model. */
	FVector SavedMapTarget = FVector::ZeroVector;
	float SavedMapDistance = 600.f;
	float SavedMapYaw = 0.f;
	FVector2D SavedMapCentre = FVector2D::ZeroVector;
	FVector2D SavedMapHalf = FVector2D(1.0, 1.0);
};
