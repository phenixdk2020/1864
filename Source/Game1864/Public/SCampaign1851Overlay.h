#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class ACampaign1851Map;
class ACampaign1851Camera;
class APlayerController;
struct FCampaign1851City;

/**
 * Screen overlay for the 1851 campaign map, painted directly each frame: title cartouche, legend,
 * compass, scale bar, Bornholm inset, city info and all place names (projected, zoom-filtered and
 * collision-resolved by priority). Dark navy panels with gold rules and a serif face.
 */
class GAME1864_API SCampaign1851Overlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCampaign1851Overlay) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ACampaign1851Map>, Map)
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controller)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920.f, 1080.f); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

	/** City to show in the info panel; -1 hides it. */
	void SetSelectedCity(int32 Index) { SelectedCity = Index; }
	int32 GetSelectedCity() const { return SelectedCity; }

	enum class EButton : uint8 { None, Build, ShowOnMap, BuildModule, Menu, SaveSlot, LoadSlot, CloseMenu, NewGame, Speed };

	/** A row of the game menu (save/load). */
	struct FSlotInfo
	{
		FString Label;   // "Autogem", "Plads 1", ...
		bool bExists = false;
		bool bCanSave = true;
		FString Info;    // date and summary, or "Tom"
	};
	void OpenMenu(const TArray<FSlotInfo>& Slots) { MenuSlots = Slots; bMenuOpen = true; bConfirmNewGame = false; }
	/** "New game" needs a second click; the button then reads "BEKRÆFT: NYT SPIL". */
	void SetConfirmNewGame(bool bConfirm) { bConfirmNewGame = bConfirm; }
	bool IsConfirmingNewGame() const { return bConfirmNewGame; }
	void CloseMenu() { bMenuOpen = false; }
	bool IsMenuOpen() const { return bMenuOpen; }
	/** A short message at the top of the screen (fades after a few seconds). */
	void ShowToast(const FString& Text) { Toast = Text; ToastTime = FPlatformTime::Seconds(); }
	/** The panel button under a viewport pixel (as from APlayerController::GetMousePosition). */
	EButton HitButton(const FVector2D& ViewportPixel, int32* OutModule = nullptr) const;

private:
	struct FPlaced
	{
		FVector2D Min, Max;
	};

	bool ToLocal(const FGeometry& Geometry, const FVector& World, FVector2D& OutLocal) const;
	int32 PaintLabels(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, float DistanceKm) const;
	void PaintPanel(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	void PaintText(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FString& Text, const FVector2D& Pos,
		const FSlateFontInfo& Font, const FLinearColor& Colour, float AlignX = 0.f, bool bShadow = true) const;
	void PaintTitle(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintLegend(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintCompass(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, float Yaw) const;
	void PaintScaleBar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintBornholm(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintInfo(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** Progress rings over towns with a building project. */
	void PaintProjects(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintButton(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size, const FString& Text, EButton Action, int32 Module = INDEX_NONE, bool bHighlight = false) const;
	/** Date, season and the speed buttons, top centre. */
	void PaintCalendar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintBar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, float Width, float Fraction) const;
	void PaintMenu(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintToast(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintDot(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Centre, float Diameter, const FLinearColor& Colour) const;
	FVector2D Measure(const FString& Text, const FSlateFontInfo& Font) const;

	TWeakObjectPtr<ACampaign1851Map> Map;
	TWeakObjectPtr<APlayerController> Controller;
	int32 SelectedCity = INDEX_NONE;
	TSharedPtr<FSlateBrush> BornholmBrush;
	TSharedPtr<FSlateBrush> DotBrush;
	TArray<TSharedPtr<FSlateBrush>> ModuleBrushes;   // card images per garrison module

	struct FButtonRect { FVector2D Min, Max; EButton Action; int32 Module; };
	mutable TArray<FButtonRect> Buttons;   // local units, rebuilt every paint
	mutable float PaintScale = 1.f;

	bool bMenuOpen = false;
	bool bConfirmNewGame = false;
	TArray<FSlotInfo> MenuSlots;
	FString Toast;
	double ToastTime = -100.0;
};
