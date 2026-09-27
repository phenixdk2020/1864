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
	void PaintDot(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Centre, float Diameter, const FLinearColor& Colour) const;
	FVector2D Measure(const FString& Text, const FSlateFontInfo& Font) const;

	TWeakObjectPtr<ACampaign1851Map> Map;
	TWeakObjectPtr<APlayerController> Controller;
	int32 SelectedCity = INDEX_NONE;
	TSharedPtr<FSlateBrush> BornholmBrush;
	TSharedPtr<FSlateBrush> DotBrush;
};
