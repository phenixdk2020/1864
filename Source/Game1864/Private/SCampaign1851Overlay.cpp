#include "SCampaign1851Overlay.h"

#include <limits>

#include "Campaign1851Buildings.h"
#include "Campaign1851ConstructionSite.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Campaign1851Camera.h"
#include "Campaign1851Map.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(212, 176, 102));
	const FLinearColor Panel = FLinearColor(0.004f, 0.006f, 0.010f, 0.9f);
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(242, 235, 214));
	const FLinearColor SeaInk = FLinearColor::FromSRGBColor(FColor(173, 199, 219, 217));
	const FLinearColor MutedInk = FLinearColor::FromSRGBColor(FColor(184, 181, 171, 204));
	const FLinearColor CityRed = FLinearColor::FromSRGBColor(FColor(184, 26, 18));
	const FLinearColor Shadow = FLinearColor(0.f, 0.f, 0.f, 0.75f);


	/** Polyline with an explicit point type (brace lists are ambiguous between FVector2D and FVector2f). */
	void DrawLines(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const TArray<FVector2D>& Points, const FLinearColor& Colour, float Thickness)
	{
		FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour, true, Thickness);
	}

	enum class EFace : uint8 { Regular, Italic, Bold };

	/**
	 * Serif face loaded from the Windows font folder at runtime (development only; a shipped build
	 * needs a redistributable font asset). Falls back to the engine default.
	 */
	FSlateFontInfo Serif(int32 Size, EFace Face = EFace::Regular)
	{
		static TMap<int32, TSharedPtr<const FCompositeFont>> Cache;
		const TCHAR* File = Face == EFace::Italic ? TEXT("georgiai.ttf") : Face == EFace::Bold ? TEXT("georgiab.ttf") : TEXT("georgia.ttf");
		const FString Path = FString(TEXT("C:/Windows/Fonts/")) + File;
		if (!FPaths::FileExists(Path))
		{
			return FCoreStyle::GetDefaultFontStyle(Face == EFace::Bold ? "Bold" : Face == EFace::Italic ? "Italic" : "Regular", Size);
		}
		TSharedPtr<const FCompositeFont>& Font = Cache.FindOrAdd(int32(Face));
		if (!Font.IsValid())
		{
			Font = MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		}
		return FSlateFontInfo(Font, Size);
	}

	FString Spaced(const FString& Text)
	{
		FString Out;
		for (int32 i = 0; i < Text.Len(); ++i)
		{
			Out.AppendChar(Text[i]);
			if (i + 1 < Text.Len())
			{
				Out += Text[i] == TEXT(' ') ? TEXT("   ") : TEXT(" ");
			}
		}
		return Out;
	}

	FString Thousands(int32 Value)
	{
		FString Digits = FString::FromInt(Value), Out;
		for (int32 i = 0; i < Digits.Len(); ++i)
		{
			if (i > 0 && (Digits.Len() - i) % 3 == 0)
			{
				Out.AppendChar(TEXT('.'));
			}
			Out.AppendChar(Digits[i]);
		}
		return Out;
	}

	FString RegionName(const FString& Code)
	{
		if (Code == TEXT("K")) return TEXT("Kongeriget Danmark");
		if (Code == TEXT("S")) return TEXT("Hertugdømmet Slesvig");
		if (Code == TEXT("H")) return TEXT("Hertugdømmerne Holsten og Lauenborg");
		return TEXT("Udland");
	}
}

void SCampaign1851Overlay::Construct(const FArguments& InArgs)
{
	Map = InArgs._Map;
	Controller = InArgs._Controller;
	SetVisibility(EVisibility::HitTestInvisible);

	// Always a circle, whatever the drawn size.
	DotBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, 0.f);
	DotBrush->OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	// The brushes do not keep their textures alive: the overlay holds them, or the garbage collector frees them under Slate.
	auto CardBrush = [this](const FString& Path)
	{
		TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
		if (UTexture2D* Card = LoadObject<UTexture2D>(nullptr, *Path))
		{
			CardTextures.Emplace(Card);
			Brush->SetResourceObject(Card);
			Brush->ImageSize = FVector2D(Card->GetSizeX(), Card->GetSizeY());
		}
		return Brush;
	};
	for (const FCampaign1851SiteModule& M : ACampaign1851ConstructionSite::GarrisonModules())
	{
		ModuleBrushes.Add(CardBrush(M.Card));
	}
	for (const FCampaign1851SiteModule& M : ACampaign1851ConstructionSite::TownBuildings())
	{
		TownBrushes.Add(CardBrush(M.Card));
	}
	// The soldiers in their uniforms (Tools/Campaign/make_uniform_cards.py) and the colours.
	for (const TCHAR* Arm : { TEXT("Infantry"), TEXT("Guard"), TEXT("Jager"), TEXT("Cavalry"), TEXT("Artillery"), TEXT("HorseArtillery"), TEXT("Hussar") })
	{
		UniformBrushes.Add(CardBrush(FString::Printf(TEXT("/Game/Campaign1851/Uniforms/T_Uniform_%s.T_Uniform_%s"), Arm, Arm)));
	}
	FlagBrush = CardBrush(TEXT("/Game/Units/Flags/T_Flag_DK.T_Flag_DK"));
	for (int32 n = 0; n < 12; ++n)
	{
		OfficerPortraits.Add(CardBrush(FString::Printf(TEXT("/Game/Campaign1851/Portraits/T_Portrait_Officer_%02d.T_Portrait_Officer_%02d"), n, n)));
		GeneralPortraits.Add(CardBrush(FString::Printf(TEXT("/Game/Campaign1851/Portraits/T_Portrait_General_%02d.T_Portrait_General_%02d"), n, n)));
	}
	// The officers by age (young 30, older 30, old 15 pictures) and the ministers (25).
	{
		static const TCHAR* Groups[] = { TEXT("Ung"), TEXT("Aeldre"), TEXT("Gammel") };
		static const int32 Counts[] = { 30, 30, 15 };
		for (int32 g = 0; g < 3; ++g)
		{
			for (int32 n = 0; n < Counts[g]; ++n)
			{
				AgePortraits[g].Add(CardBrush(FString::Printf(TEXT("/Game/Campaign1851/Portraits/T_Portrait_Officer_%s_%02d.T_Portrait_Officer_%s_%02d"), Groups[g], n, Groups[g], n)));
			}
		}
		for (int32 n = 0; n < 25; ++n)
		{
			MinisterPortraits.Add(CardBrush(FString::Printf(TEXT("/Game/Campaign1851/Portraits/T_Portrait_Minister_%02d.T_Portrait_Minister_%02d"), n, n)));
		}
	}
	BornholmBrush = MakeShared<FSlateBrush>();
	if (Map.IsValid() && Map->BornholmTexture)
	{
		BornholmBrush->SetResourceObject(Map->BornholmTexture);
		BornholmBrush->ImageSize = FVector2D(Map->BornholmTexture->GetSizeX(), Map->BornholmTexture->GetSizeY());
	}
}

// ------------------------------------------------------------------ helpers

bool SCampaign1851Overlay::ToLocal(const FGeometry& Geometry, const FVector& World, FVector2D& OutLocal) const
{
	FVector2D Screen;
	if (!Controller.IsValid() || !Controller->ProjectWorldLocationToScreen(World, Screen, false))
	{
		return false;
	}
	OutLocal = Screen / Geometry.Scale;
	const FVector2D Size = Geometry.GetLocalSize();
	return OutLocal.X > -200.f && OutLocal.Y > -100.f && OutLocal.X < Size.X + 200.f && OutLocal.Y < Size.Y + 100.f;
}

FVector2D SCampaign1851Overlay::Measure(const FString& Text, const FSlateFontInfo& Font) const
{
	return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font);
}

void SCampaign1851Overlay::PaintText(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FString& Text,
	const FVector2D& Pos, const FSlateFontInfo& Font, const FLinearColor& Colour, float AlignX, bool bShadow) const
{
	const FVector2D Size = Measure(Text, Font);
	const FVector2D TopLeft = Pos - FVector2D(Size.X * AlignX, Size.Y * 0.5f);
	if (bShadow)
	{
		FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(TopLeft + FVector2D(1.2f, 1.2f))),
			Text, Font, ESlateDrawEffect::None, Shadow.CopyWithNewOpacity(Shadow.A * Colour.A));
	}
	FSlateDrawElement::MakeText(Out, Layer + 1, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(TopLeft)), Text, Font, ESlateDrawEffect::None, Colour);
}

void SCampaign1851Overlay::PaintPanel(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, Panel);
	auto Frame = [&](float Inset, const FLinearColor& Colour, float Thickness)
	{
		const FVector2D A = Pos + FVector2D(Inset, Inset), B = Pos + Size - FVector2D(Inset, Inset);
		TArray<FVector2D> Points = { A, FVector2D(B.X, A.Y), B, FVector2D(A.X, B.Y), A };
		FSlateDrawElement::MakeLines(Out, Layer + 1, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour, true, Thickness);
	};
	Frame(0.f, Gold.CopyWithNewOpacity(0.85f), 1.5f);
	Frame(6.f, Gold.CopyWithNewOpacity(0.45f), 1.f);
}

void SCampaign1851Overlay::PaintDot(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Centre, float Diameter, const FLinearColor& Colour) const
{
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2D(Diameter, Diameter), FSlateLayoutTransform(Centre - FVector2D(Diameter * 0.5f))),
		DotBrush.Get(), ESlateDrawEffect::None, Colour);
}

// ------------------------------------------------------------------ paint

int32 SCampaign1851Overlay::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	if (!Map.IsValid() || !Map->IsReady() || !Controller.IsValid())
	{
		return Layer;
	}
	const ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(Controller->GetPawn());
	const float DistanceKm = Camera ? Camera->GetDistanceKm() : 600.f;

	Buttons.Reset();
	Tips.Reset();
	PaintScale = Geometry.Scale;
	if (Map->IsBattleView())
	{
		// On the battlefield model: no map signs; its name, and the way back.
		const FCampaign1851Battlefield& B = Map->GetBattlefield();
		const FVector2D Size(760.f, 64.f);
		const FVector2D Pos((Geometry.GetLocalSize().X - Size.X) * 0.5f, 28.f);
		PaintPanel(Geometry, Out, Layer + 20, Pos, Size);
		PaintText(Geometry, Out, Layer + 22, FString::Printf(TEXT("Slagmarken ved %s"), *B.Place), Pos + FVector2D(20.f, 22.f), Serif(18), Ink, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 22, FString::Printf(TEXT("%.0f × %.0f km  ·  %s  ·  ingen enheder endnu  ·  hjul: zoom  ·  træk/WASD: panorér  ·  Q/E: drej"), B.SizeKm, B.SizeKm, *Map->GetSeasonAndWeather()),
			Pos + FVector2D(20.f, 46.f), Serif(10, EFace::Italic), MutedInk, Size.X - 270.f);
		PaintButton(Geometry, Out, Layer + 22, Pos + FVector2D(Size.X - 230.f, 16.f), FVector2D(210.f, 32.f), TEXT("TILBAGE TIL KORTET"), EButton::BattleViewLeave);
		return Layer + 30;
	}
	Layer = PaintLabels(Geometry, Out, Layer, DistanceKm) + 2;
	PaintProjects(Geometry, Out, Layer);
	PaintLinkWorks(Geometry, Out, Layer);
	PaintArmy(Geometry, Out, Layer);
	Layer += 6;
	PaintTitle(Geometry, Out, Layer);
	PaintLegend(Geometry, Out, Layer);
	PaintCompass(Geometry, Out, Layer, Camera ? Camera->GetYaw() : 0.f);
	PaintScaleBar(Geometry, Out, Layer);
	// (Bornholm is on the map itself now; no inset.)
	UnitCardAnchor = BuildingCardAnchor = FVector2D(-1.f, -1.f);
	PaintInfo(Geometry, Out, Layer);
	PaintSidePanels(Geometry, Out, Layer + 2);
	PaintButton(Geometry, Out, Layer, FVector2D(28.f, 206.f), FVector2D(150.f, 28.f), TEXT("SPILMENU  (M)"), EButton::Menu);
	PaintButton(Geometry, Out, Layer, FVector2D(186.f, 206.f), FVector2D(120.f, 28.f), TEXT("SKANSER"), EButton::FortTool, 0, bFortTool);
	PaintButton(Geometry, Out, Layer, FVector2D(314.f, 206.f), FVector2D(120.f, 28.f), TEXT("AVISEN"), EButton::OpenGazette, 0, Window == EWindow::Gazette);
	PaintButton(Geometry, Out, Layer, FVector2D(442.f, 206.f), FVector2D(130.f, 28.f), TEXT("SLAGMARK"), EButton::OpenBattlefield, 0, Window == EWindow::Battlefield);
	PaintButton(Geometry, Out, Layer, FVector2D(580.f, 206.f), FVector2D(130.f, 28.f), TEXT("MATERIEL"), EButton::OpenMateriel, 0, Window == EWindow::Materiel);
	PaintCalendar(Geometry, Out, Layer);
	PaintTreasury(Geometry, Out, Layer);
	if (Window != EWindow::None)
	{
		// A window takes the clicks: the bar (to switch or close) and its own buttons stay live.
		Buttons.Reset();
		Tips.Reset();
		PaintWindow(Geometry, Out, Layer + 10);
		PaintMenuBar(Geometry, Out, Layer + 14);
		if (Window == EWindow::Chart)
		{
			// Appointing from the chart: the list of free officers (or one's card) over the window's corner.
			const FVector2D Corner(Geometry.GetLocalSize().X - 600.f, Geometry.GetLocalSize().Y - 110.f);
			if (Map->GetOfficers().IsValidIndex(InspectedOfficer))
			{
				PaintOfficerCard(Geometry, Out, Layer + 30, Corner);
			}
			else if (Picker != EPicker::None)
			{
				PaintOfficerPicker(Geometry, Out, Layer + 30, Corner);
			}
		}
	}
	else
	{
		PaintMenuBar(Geometry, Out, Layer);
	}
	if (Window == EWindow::None && BuildingCardAnchor.X >= 0.f && ACampaign1851ConstructionSite::TownBuildings().IsValidIndex(BuildingInfo))
	{
		PaintBuildingCard(Geometry, Out, Layer + 34, BuildingCardAnchor, BuildingInfo);
	}
	if (bUnitCard && Window == EWindow::None && SelectedRegiments.Num() == 1 && Map->GetRegiments().IsValidIndex(SelectedRegiments[0]) && UnitCardAnchor.X >= 0.f)
	{
		FVector2D Anchor = UnitCardAnchor;
		if (bOOB)
		{
			Anchor.X = FMath::Max(Anchor.X, TreeMax.X + 10.f);
		}
		PaintUnitCard(Geometry, Out, Layer + 34, Anchor, SelectedRegiments[0]);
	}
	if (bLedgerOpen)
	{
		PaintLedger(Geometry, Out, Layer + 4);
	}
	if (OrderDialog.bOpen)
	{
		PaintOrderDialog(Geometry, Out, Layer + 8);
	}
	if (Window == EWindow::None)
	{
		PaintBattle(Geometry, Out, Layer + 30);
	}
	PaintToast(Geometry, Out, Layer + 40);   // above the windows
	if (bMenuOpen)
	{
		// The menu takes the clicks: only its own buttons stay live.
		Buttons.Reset();
		Tips.Reset();
		PaintMenu(Geometry, Out, Layer + 8);
	}
	if (bConfirmOpen)
	{
		Tips.Reset();
		PaintConfirm(Geometry, Out, Layer + 60);
	}
	else if (bTransferOpen)
	{
		Tips.Reset();
		PaintTransfer(Geometry, Out, Layer + 60);
	}
	PaintTooltip(Geometry, Out, Layer + 70);

	const FVector2D Size = Geometry.GetLocalSize();
	PaintText(Geometry, Out, Layer, TEXT("Klik: by eller regiment  ·  Højreklik: march  ·  Hjul: zoom  ·  Træk/WASD: panorer  ·  Q/E: drej  ·  Mellemrum: pause  ·  1-5: fart  ·  M: menu  ·  F5/F9"),
		FVector2D(Size.X * 0.5f, Size.Y - 42.f), Serif(12), MutedInk, 0.5f);
	PaintText(Geometry, Out, Layer, TEXT("v00.00.55 SLAGMARK I 3D, LANDE, BUDGETTER — UNREAL"), FVector2D(Size.X * 0.5f, Size.Y - 20.f), Serif(9), MutedInk.CopyWithNewOpacity(0.5f), 0.5f);
	return Layer + 16;
}

int32 SCampaign1851Overlay::PaintLabels(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, float D) const
{
	struct FItem
	{
		FString Text;
		FVector World;
		FSlateFontInfo Font;
		FLinearColor Colour;
		float AlignX = 0.5f;
		FVector2D Offset = FVector2D::ZeroVector;
		int32 Priority = 0;
	};
	TArray<FItem> Items;

	for (const FCampaign1851Label& L : Map->GetLabels())
	{
		FItem I;
		I.World = L.World;
		if (L.Kind == TEXT("sea")) { I.Text = Spaced(L.Text); I.Font = Serif(18, EFace::Italic); I.Colour = SeaInk; I.Priority = 900000; }
		else if (L.Kind == TEXT("strait")) { if (D >= 400.f) continue; I.Text = L.Text; I.Font = Serif(11, EFace::Italic); I.Colour = SeaInk; I.Priority = 500; }
		else if (L.Kind == TEXT("land")) { if (D <= 45.f) continue; I.Text = Spaced(L.Text); I.Font = Serif(20, EFace::Italic); I.Colour = FLinearColor::FromSRGBColor(FColor(245, 237, 204, 200)); I.Priority = 700000; }
		else if (L.Kind == TEXT("duchy")) { I.Text = Spaced(L.Text.ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å"))); I.Font = Serif(13); I.Colour = Gold; I.Priority = 800000; }
		else if (L.Kind == TEXT("river")) { if (D >= 160.f) continue; I.Text = L.Text; I.Font = Serif(10, EFace::Italic); I.Colour = SeaInk; I.Priority = 450; }
		else if (L.Kind == TEXT("amt")) { if (D <= 30.f || D >= 330.f) continue; I.Text = Spaced(L.Text.ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å"))); I.Font = Serif(10); I.Colour = FLinearColor::FromSRGBColor(FColor(232, 214, 160, 190)); I.Priority = 600; }
		else { I.Text = L.Text; I.Font = Serif(13, EFace::Italic); I.Colour = MutedInk; I.Priority = 400; }
		Items.Add(I);
	}

	const int32 Threshold = D > 420.f ? 7500 : D > 220.f ? 2500 : D > 110.f ? 1200 : 0;
	for (const FCampaign1851City& C : Map->GetCities())
	{
		if (C.bBornholm)
		{
			continue;
		}
		FItem I;
		I.World = C.World;
		if (C.bForeign)
		{
			if (D >= 380.f) continue;
			I.Text = C.Name; I.Font = Serif(11, EFace::Italic); I.Colour = MutedInk; I.AlignX = 1.f; I.Offset = FVector2D(-8.f, 0.f); I.Priority = 100 + C.Population / 1000;
		}
		else
		{
			if (!C.bCapital && C.Population < Threshold) continue;
			I.Text = C.Name; I.Font = C.bCapital ? Serif(20, EFace::Bold) : Serif(13); I.Colour = Ink; I.AlignX = 0.f;
			I.Offset = FVector2D(C.bCapital ? 13.f : 9.f, 0.f); I.Priority = C.bCapital ? 1000000 : 1000 + C.Population / 10;
		}
		Items.Add(I);
	}
	Items.Sort([](const FItem& A, const FItem& B) { return A.Priority > B.Priority; });

	TArray<FPlaced> Placed;
	for (const FItem& I : Items)
	{
		FVector2D P;
		if (!ToLocal(Geometry, I.World, P))
		{
			continue;
		}
		P += I.Offset;
		const FVector2D Size = Measure(I.Text, I.Font);
		const FVector2D Min(P.X - Size.X * I.AlignX - 4.f, P.Y - Size.Y * 0.5f - 2.f);
		const FVector2D Max = Min + Size + FVector2D(8.f, 4.f);
		bool bBlocked = false;
		for (const FPlaced& Other : Placed)
		{
			if (Min.X < Other.Max.X && Max.X > Other.Min.X && Min.Y < Other.Max.Y && Max.Y > Other.Min.Y)
			{
				bBlocked = true;
				break;
			}
		}
		if (bBlocked)
		{
			continue;
		}
		Placed.Add({ Min, Max });
		PaintText(Geometry, Out, Layer, I.Text, P, I.Font, I.Colour, I.AlignX);
	}
	return Layer + 2;
}

void SCampaign1851Overlay::PaintTitle(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const FVector2D Pos(28.f, 28.f), Size(430.f, 168.f);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	const float Cx = Pos.X + Size.X * 0.5f;
	// Gold lozenge flanked by rules.
	TArray<FVector2D> Lozenge = { {Cx, Pos.Y + 9.f}, {Cx + 5.f, Pos.Y + 14.f}, {Cx, Pos.Y + 19.f}, {Cx - 5.f, Pos.Y + 14.f}, {Cx, Pos.Y + 9.f} };
	DrawLines(Geometry, Out, Layer + 2, Lozenge, Gold, 2.f);
	DrawLines(Geometry, Out, Layer + 2, { {Cx - 82.f, Pos.Y + 14.f}, {Cx - 12.f, Pos.Y + 14.f} }, Gold, 1.5f);
	DrawLines(Geometry, Out, Layer + 2, { {Cx + 12.f, Pos.Y + 14.f}, {Cx + 82.f, Pos.Y + 14.f} }, Gold, 1.5f);
	PaintText(Geometry, Out, Layer + 2, TEXT("DANMARK"), FVector2D(Cx, Pos.Y + 62.f), Serif(46), Ink, 0.5f);
	PaintText(Geometry, Out, Layer + 2, *FString::FromInt(ACampaign1851Map::ActiveScenario().Year), FVector2D(Cx, Pos.Y + 112.f), Serif(29), Ink, 0.5f);
	DrawLines(Geometry, Out, Layer + 2, { {Pos.X + 86.f, Pos.Y + 138.f}, {Pos.X + Size.X - 86.f, Pos.Y + 138.f} }, Gold, 1.5f);
	PaintText(Geometry, Out, Layer + 2, TEXT("KONGERIGET  ·  HERTUGDØMMERNE"), FVector2D(Cx, Pos.Y + 152.f), Serif(11), Gold, 0.5f, false);
}

void SCampaign1851Overlay::PaintLegend(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const FVector2D ScreenSize = Geometry.GetLocalSize();
	const FVector2D Size(360.f, 386.f);
	const FVector2D Pos(ScreenSize.X - Size.X - 28.f, 28.f);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintText(Geometry, Out, Layer + 2, TEXT("Byer efter befolkning (ca. 1850)"), FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + 28.f), Serif(14), Ink, 0.5f, false);

	const TPair<const TCHAR*, int32> Rows[] = { {TEXT("> 50.000"), 60000}, {TEXT("20.000 – 50.000"), 30000}, {TEXT("10.000 – 20.000"), 15000}, {TEXT("5.000 – 10.000"), 7000}, {TEXT("< 5.000"), 2000} };
	float Y = Pos.Y + 60.f;
	for (const auto& Row : Rows)
	{
		PaintDot(Geometry, Out, Layer + 2, FVector2D(Pos.X + 58.f, Y), 13.f * ACampaign1851Map::SizeClass(Row.Value), CityRed);
		PaintText(Geometry, Out, Layer + 2, Row.Key, FVector2D(Pos.X + 100.f, Y), Serif(13), Ink, 0.f, false);
		Y += 30.f;
	}
	Y += 4.f;
	DrawLines(Geometry, Out, Layer + 2, { {Pos.X + 29.f, Y}, {Pos.X + Size.X - 29.f, Y} }, Gold.CopyWithNewOpacity(0.5f), 1.f);
	Y += 24.f;
	DrawLines(Geometry, Out, Layer + 2, { {Pos.X + 38.f, Y}, {Pos.X + 78.f, Y} }, FLinearColor::FromSRGBColor(FColor(120, 30, 24)), 3.f);
	PaintText(Geometry, Out, Layer + 2, TEXT("Monarkiets ydre grænse"), FVector2D(Pos.X + 100.f, Y), Serif(13), Ink, 0.f, false);
	Y += 28.f;
	for (int32 i = 0; i < 4; ++i)
	{
		DrawLines(Geometry, Out, Layer + 2, { {Pos.X + 38.f + i * 10.f, Y}, {Pos.X + 44.f + i * 10.f, Y} }, Gold, 3.f);
	}
	PaintText(Geometry, Out, Layer + 2, TEXT("Kongeå- og Ejdergrænsen"), FVector2D(Pos.X + 100.f, Y), Serif(13), Ink, 0.f, false);
	Y += 28.f;
	DrawLines(Geometry, Out, Layer + 2, { {Pos.X + 37.f, Y}, {Pos.X + 79.f, Y} }, MutedInk, 8.f);
	DrawLines(Geometry, Out, Layer + 2, { {Pos.X + 38.f, Y}, {Pos.X + 78.f, Y} }, FLinearColor(0.02f, 0.02f, 0.02f), 6.f);
	for (int32 i = 0; i < 3; ++i)
	{
		DrawLines(Geometry, Out, Layer + 3, { {Pos.X + 40.f + i * 14.f, Y}, {Pos.X + 47.f + i * 14.f, Y} }, Ink, 2.f);
	}
	PaintText(Geometry, Out, Layer + 2, TEXT("Jernbane"), FVector2D(Pos.X + 100.f, Y), Serif(13), Ink, 0.f, false);
	Y += 28.f;
	DrawLines(Geometry, Out, Layer + 2, { {Pos.X + 38.f, Y}, {Pos.X + 78.f, Y} }, FLinearColor::FromSRGBColor(FColor(222, 214, 192)), 4.f);
	PaintText(Geometry, Out, Layer + 2, TEXT("Chaussé"), FVector2D(Pos.X + 100.f, Y), Serif(13), Ink, 0.f, false);
	DrawLines(Geometry, Out, Layer + 2, { {Pos.X + 190.f, Y}, {Pos.X + 230.f, Y} }, FLinearColor::FromSRGBColor(FColor(176, 136, 92)), 3.f);
	PaintText(Geometry, Out, Layer + 2, TEXT("Landevej"), FVector2D(Pos.X + 244.f, Y), Serif(13), Ink, 0.f, false);

	int32 Count = 0;
	for (const FCampaign1851City& C : Map->GetCities())
	{
		Count += C.bForeign ? 0 : 1;
	}
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("I alt: %d byer i monarkiet"), Count), FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + Size.Y - 22.f), Serif(12, EFace::Italic), MutedInk, 0.5f, false);
}

void SCampaign1851Overlay::PaintCompass(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, float Yaw) const
{
	const FVector2D Centre(100.f, Geometry.GetLocalSize().Y - 100.f);
	PaintDot(Geometry, Out, Layer, Centre, 96.f, FLinearColor(0.f, 0.f, 0.f, 0.45f));
	const float Rad = FMath::DegreesToRadians(-Yaw);
	auto At = [&](float AngleDeg, float Radius)
	{
		const float A = FMath::DegreesToRadians(AngleDeg) + Rad;
		return Centre + FVector2D(FMath::Sin(A), -FMath::Cos(A)) * Radius;
	};
	// Eight-point star: long cardinal points, short intercardinals.
	TArray<FVector2D> Star;
	for (int32 k = 0; k <= 16; ++k)
	{
		const float Angle = k * 22.5f;
		const float Radius = (k % 4 == 0) ? 58.f : (k % 2 == 0) ? 32.f : 9.f;
		Star.Add(At(Angle, Radius));
	}
	DrawLines(Geometry, Out, Layer + 1, Star, Gold, 2.f);
	for (int32 k = 0; k < 8; ++k)
	{
		DrawLines(Geometry, Out, Layer + 1, { Centre, At(k * 45.f, (k % 2 == 0) ? 58.f : 32.f) }, Gold.CopyWithNewOpacity(0.55f), 1.f);
	}
	const TPair<const TCHAR*, float> Letters[] = { {TEXT("N"), 0.f}, {TEXT("Ø"), 90.f}, {TEXT("S"), 180.f}, {TEXT("V"), 270.f} };
	for (const auto& L : Letters)
	{
		PaintText(Geometry, Out, Layer + 2, L.Key, At(L.Value, 72.f), Serif(15), Ink, 0.5f);
	}
}

void SCampaign1851Overlay::PaintScaleBar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	// Ground distance across 10% of the view width, measured at 70% down the screen.
	const FVector2D ViewSize = Geometry.GetLocalSize() * Geometry.Scale;
	FVector WA, DA, WB, DB;
	if (!Controller->DeprojectScreenPositionToWorld(ViewSize.X * 0.5f, ViewSize.Y * 0.7f, WA, DA) ||
		!Controller->DeprojectScreenPositionToWorld(ViewSize.X * 0.6f, ViewSize.Y * 0.7f, WB, DB) || DA.Z >= 0.f || DB.Z >= 0.f)
	{
		return;
	}
	const FVector GA = WA + DA * (-WA.Z / DA.Z), GB = WB + DB * (-WB.Z / DB.Z);
	const float KmPerUnit = float(FVector::Dist(GA, GB) / ACampaign1851Map::KmToUnits) / (Geometry.GetLocalSize().X * 0.1f);
	const float Steps[] = { 1.f, 2.f, 5.f, 10.f, 20.f, 25.f, 50.f, 100.f, 150.f, 200.f };
	float Step = Steps[0];
	for (float S : Steps)
	{
		if (3.f * S / KmPerUnit <= 300.f)
		{
			Step = S;
		}
	}
	const float Seg = Step / KmPerUnit;
	const FVector2D Origin(200.f, Geometry.GetLocalSize().Y - 44.f);
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	for (int32 i = 0; i < 3; ++i)
	{
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2D(Seg, 6.f), FSlateLayoutTransform(Origin + FVector2D(i * Seg, 0.f))),
			White, ESlateDrawEffect::None, i % 2 == 0 ? Ink : FLinearColor(0.01f, 0.01f, 0.01f, 0.9f));
	}
	for (int32 i = 0; i < 4; ++i)
	{
		PaintText(Geometry, Out, Layer + 1, i == 3 ? FString::Printf(TEXT("%.0f km"), Step * 3) : FString::Printf(TEXT("%.0f"), Step * i),
			Origin + FVector2D(i * Seg, -12.f), Serif(12), Ink, 0.5f);
	}
}

void SCampaign1851Overlay::PaintBornholm(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	if (!BornholmBrush.IsValid() || !BornholmBrush->GetResourceObject())
	{
		return;
	}
	const float Aspect = float(BornholmBrush->ImageSize.X / FMath::Max(1.0, BornholmBrush->ImageSize.Y));
	const FVector2D Image(170.f * Aspect, 170.f);
	const FVector2D Size(Image.X + 16.f, Image.Y + 44.f);
	const FVector2D Pos = Geometry.GetLocalSize() - Size - FVector2D(28.f, 28.f);
	// The map's views, above the box.
	{
		const TCHAR* Views[] = { TEXT("NORMAL"), TEXT("FORSYNING"), TEXT("KONTROL") };
		const TCHAR* ViewTips[] = { TEXT("Det almindelige kort"), TEXT("Forsyning: depoternes rækkevidde og enhedernes forsyning (F)"), TEXT("Kontrol: besatte byer, og hvor langt befrielsen er") };
		const int32 Current = bSupplyMap ? 1 : MapView == 2 ? 2 : 0;
		const float W = (Size.X - 8.f) / 3.f;
		for (int32 v = 0; v < 3; ++v)
		{
			PaintButton(Geometry, Out, Layer + 1, FVector2D(Pos.X + v * (W + 4.f), Pos.Y - 30.f), FVector2D(W, 24.f), Views[v], EButton::MapView, v, Current == v);
			AddTip(FVector2D(Pos.X + v * (W + 4.f), Pos.Y - 30.f), FVector2D(W, 24.f), ViewTips[v]);
		}
	}
	// The control view: a red tag on every occupied town, with the days of liberation.
	if (MapView == 2 && !bSupplyMap && Controller.IsValid())
	{
		for (const FCampaign1851City& C : Map->GetCities())
		{
			if (C.Occupier.IsEmpty() || C.bBornholm)
			{
				continue;
			}
			FVector2D P;
			if (ToLocal(Geometry, C.World, P))
			{
				const FString TagText = C.bCeded ? FString::Printf(TEXT("%s  ·  afstået"), *C.Name)
					: FString::Printf(TEXT("%s  ·  BESAT (%s)%s"), *C.Name, *C.Occupier, C.LiberationDays > 0.f ? *FString::Printf(TEXT("  ·  befries %.0f/2"), C.LiberationDays) : TEXT(""));
				const FVector2D TagSize = Measure(TagText, Serif(11)) + FVector2D(14.f, 6.f);
				const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
				FSlateDrawElement::MakeBox(Out, Layer + 4, Geometry.ToPaintGeometry(TagSize, FSlateLayoutTransform(P + FVector2D(10.f, -TagSize.Y - 4.f))), White, ESlateDrawEffect::None, FLinearColor(0.45f, 0.06f, 0.05f, 0.9f));
				PaintText(Geometry, Out, Layer + 5, TagText, P + FVector2D(17.f, -TagSize.Y * 0.5f - 4.f), Serif(11), Ink, 0.f, false);
			}
		}
	}
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	const FVector2D ImagePos = Pos + FVector2D(8.f, 8.f);
	FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(Image, FSlateLayoutTransform(ImagePos)), BornholmBrush.Get());
	PaintText(Geometry, Out, Layer + 3, TEXT("Bornholm"), FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + Size.Y - 18.f), Serif(14, EFace::Italic), Ink, 0.5f);

	for (const FCampaign1851City& C : Map->GetCities())
	{
		if (!C.bBornholm)
		{
			continue;
		}
		const FVector2D Uv = Map->GetBornholmExtent().ToUv(C.Lat, C.Lon);
		const FVector2D P = ImagePos + FVector2D(Uv.X * Image.X, (1.0 - Uv.Y) * Image.Y);
		PaintDot(Geometry, Out, Layer + 3, P, 11.f, CityRed);
		PaintText(Geometry, Out, Layer + 3, C.Name, P + FVector2D(8.f, 0.f), Serif(12), Ink, 0.f);
	}
}

void SCampaign1851Overlay::PaintInfo(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	if (bFortTool)
	{
		PaintFortTool(Geometry, Out, Layer);
	}
	if (Map->FortIndex(SelectedFort) != INDEX_NONE)
	{
		PaintFort(Geometry, Out, Layer);
		return;
	}
	if (Map->BridgeIndex(SelectedBridge) != INDEX_NONE)
	{
		PaintBridge(Geometry, Out, Layer);
		return;
	}
	if (SelectedRegiments.Num() > 0)
	{
		PaintArmyInfo(Geometry, Out, Layer);
		return;
	}
	if (!Cities.IsValidIndex(SelectedCity))
	{
		PaintAmtInfo(Geometry, Out, Layer);
		return;
	}
	const FCampaign1851City& C = Cities[SelectedCity];
	// One side panel at a time, chosen with the tabs on the town card.
	if (TownTab == 2)
	{
		PaintTownBuildings(Geometry, Out, Layer);
	}
	else if (TownTab == 3)
	{
		PaintTownLinks(Geometry, Out, Layer, 28.f + 440.f + 10.f);
	}
	// Room for the depot and the materials store, when the town has them.
	const FCampaign1851DepotCapacity DepotRoom = Map->DepotCapacity(SelectedCity);
	const bool bDepotLine = !C.bForeign && DepotRoom.Food + DepotRoom.Fodder + DepotRoom.Ammo > 0.f;
	const bool bMaterialsLine = !C.bForeign && Map->GetMaterialsIn(SelectedCity) >= 1.0;
	const FVector2D Size(440.f, C.bForeign ? 150.f : 226.f + (bDepotLine ? 20.f : 0.f) + (bMaterialsLine ? 20.f : 0.f));
	const ACampaign1851ConstructionSite* Site = C.bHasPlot ? Map->FindProject(SelectedCity) : nullptr;
	const int32 ModuleRows = Site && Site->IsBarracksDone() ? Site->NumModules() - 1 : 0;
	const float RowHeight = 54.f;
	const float CardHeight = 150.f + (ModuleRows > 0 ? 34.f + ModuleRows * RowHeight + 10.f : 0.f);
	const FVector2D Pos(28.f, Geometry.GetLocalSize().Y - 190.f - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseSelection);
	PaintText(Geometry, Out, Layer + 2, C.Name, Pos + FVector2D(22.f, 32.f), Serif(24), Ink, 0.f);
	PaintText(Geometry, Out, Layer + 2, C.bForeign ? TEXT("Udenlandsk by") : C.bCapital ? TEXT("Hovedstad") : TEXT("Købstad"), Pos + FVector2D(22.f, 64.f), Serif(14, EFace::Italic), Gold, 0.f, false);
	const FCampaign1851Amt* Amt = Map->FindAmt(C.AmtId);
	PaintText(Geometry, Out, Layer + 2, C.bForeign ? TEXT("Uden for monarkiet") : Amt ? FString::Printf(TEXT("%s  ·  %s"), *Amt->Name, *ACampaign1851Map::RegionName(Amt->Region)) : RegionName(C.Region),
		Pos + FVector2D(22.f, 92.f), Serif(13), Ink, 0.f, false);
	if (!C.Occupier.IsEmpty())
	{
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("BESAT af %s"), C.Occupier == TEXT("AT") ? TEXT("Østrig") : TEXT("Preussen")), Pos + FVector2D(Size.X - 30.f, 62.f), Serif(13), FLinearColor(0.95f, 0.4f, 0.35f), 1.f, false);
	}
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("ca. %s indbyggere (%d)  ·  vækst %.1f %%/år"), *Thousands(C.Population), Map->GetDate().GetYear(), Map->UrbanGrowthRate(SelectedCity)), Pos + FVector2D(22.f, 120.f), Serif(13), Ink, 0.f, false);
	if (!C.bForeign)
	{
		const int32 AmtIndex = Map->AmtIndexOfTown(SelectedCity);
		if (bMaterialsLine)
		{
			PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Byggematerialer: %s rd. på lager (op til halvdelen af byggeri inden for 30 km)"), *Thousands(int32(Map->GetMaterialsIn(SelectedCity)))),
				Pos + FVector2D(22.f, 166.f + (bDepotLine ? 20.f : 0.f)), Serif(11, EFace::Italic), Ink, Size.X - 44.f);
		}
		const FCampaign1851DepotCapacity DepotCap = Map->DepotCapacity(SelectedCity);
		if (DepotCap.Food + DepotCap.Fodder + DepotCap.Ammo > 0.f)
		{
			const FCampaign1851DepotStock Stock = Map->DepotStock(SelectedCity);
			PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Depot: %s/%s rationer  ·  foder %s  ·  ammunition %.0f/%.0f"),
				*Thousands(int32(Stock.Food)), *Thousands(int32(DepotCap.Food)), *Thousands(int32(Stock.Fodder)), Stock.Ammo, DepotCap.Ammo),
				Pos + FVector2D(22.f, 166.f), Serif(11, EFace::Italic), Ink, Size.X - 44.f);
		}
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Skat %s rd./år  ·  reserve %s mand (+%s/år)"),
			*Thousands(FMath::RoundToInt(C.Population * ACampaign1851Map::UrbanTaxRate())), *Thousands(FMath::FloorToInt(Map->GetManpower(AmtIndex))),
			*Thousands(FMath::RoundToInt(Map->YearlyClass(AmtIndex)))),
			Pos + FVector2D(22.f, 146.f), Serif(12, EFace::Italic), Gold, Size.X - 44.f);
		const TCHAR* Tabs[] = { TEXT("GARNISON"), TEXT("BYGNINGER"), TEXT("VEJE OG BANER") };
		const float TabW = (Size.X - 44.f - 16.f) / 3.f;
		for (int32 t = 0; t < 3; ++t)
		{
			const bool bEnabled = t != 0 || C.bHasPlot;
			PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(22.f + t * (TabW + 8.f), Size.Y - 44.f), FVector2D(TabW, 28.f), Tabs[t], EButton::TownTab, t + 1, TownTab == t + 1, !bEnabled);
		}
	}
	if (!C.bHasPlot || TownTab != 1)
	{
		return;
	}

	// Garrison (design manual 20.16.5-7): the barracks card, then the modules around the parade ground.
	const FVector2D Card(28.f + 440.f + 10.f, Geometry.GetLocalSize().Y - 190.f - CardHeight);
	PaintPanel(Geometry, Out, Layer, Card, FVector2D(Size.X, CardHeight));
	PaintCloseX(Geometry, Out, Layer + 3, Card + FVector2D(Size.X, 0.f), CloseTownTab);
	PaintText(Geometry, Out, Layer + 2, TEXT("G A R N I S O N"), Card + FVector2D(22.f, 22.f), Serif(11), Gold, 0.f, false);
	if (ModuleBrushes.IsValidIndex(0) && ModuleBrushes[0]->GetResourceObject())
	{
		FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(100.f, 100.f), FSlateLayoutTransform(Card + FVector2D(18.f, 36.f))), ModuleBrushes[0].Get());
	}
	const FVector2D Text = Card + FVector2D(134.f, 0.f);
	const FCampaign1851SiteModule& Barracks = ACampaign1851ConstructionSite::GarrisonModules()[0];
	PaintText(Geometry, Out, Layer + 2, Barracks.Name, Text + FVector2D(0.f, 50.f), Serif(16), Ink, 0.f, false);
	if (!Site)
	{
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s rd.  ·  %d dage"), *Thousands(Barracks.Cost()), int32(Barracks.Days())),
			Text + FVector2D(0.f, 76.f), Serif(12, EFace::Italic), MutedInk, 0.f, false);
		const bool bAfford = Map->CanAfford(Barracks.Cost());
		PaintButton(Geometry, Out, Layer + 2, Text + FVector2D(0.f, 98.f), FVector2D(150.f, 30.f), TEXT("BYG KASERNE"), EButton::Build, INDEX_NONE, false, !bAfford);
		if (!bAfford)
		{
			PaintText(Geometry, Out, Layer + 2, TEXT("Ikke råd"), Text + FVector2D(160.f, 113.f), Serif(11, EFace::Italic), Gold, 0.f, false);
		}
		return;
	}
	const bool bBarracksDone = Site->IsBarracksDone();
	const FString Status = bBarracksDone
		? TEXT("Færdig  ·  klar til garnisonen")
		: ProgressLine(Site, 0);
	PaintText(Geometry, Out, Layer + 2, Status, Text + FVector2D(0.f, 76.f), Serif(12, EFace::Italic), bBarracksDone ? Gold : Ink, 0.f, false);
	if (!bBarracksDone)
	{
		PaintBar(Geometry, Out, Layer + 2, Text + FVector2D(0.f, 92.f), 220.f, Site->GetModuleProgress(0));
	}
	PaintButton(Geometry, Out, Layer + 2, Text + FVector2D(0.f, 106.f), FVector2D(150.f, 26.f), TEXT("VIS PÅ KORTET"), EButton::ShowOnMap);
	{
		const int32 Code = SelectedCity * 100 + 99;
		PaintButton(Geometry, Out, Layer + 2, Text + FVector2D(158.f, 106.f), FVector2D(118.f, 26.f), Site->IsDemolishing() ? TEXT("RIVES NED") : DemolishArmed == Code ? TEXT("BEKRÆFT") : TEXT("NEDRIV"),
			EButton::Demolish, Code, DemolishArmed == Code, Site->IsDemolishing());
		if (DemolishArmed == Code)
		{
			PaintTextFit(Geometry, Out, Layer + 2, Map->DemolishText(Site), Text + FVector2D(0.f, 94.f), Serif(10, EFace::Italic), Gold, Size.X - 150.f);
		}
	}
	if (bBarracksDone)
	{
		// A new battalion of recruits from the amt's reserve (design: manpower, conscription of 1849).
		const FString Why = Map->RaiseBlockReason(SelectedCity);
		PaintButton(Geometry, Out, Layer + 2, Text + FVector2D(0.f, 136.f), FVector2D(220.f, 26.f), FString::Printf(TEXT("OPRET BATAILLON  %s rd."), *Thousands(Campaign1851Army::RaiseCost())),
			EButton::RaiseBattalion, 0, false, !Why.IsEmpty());
		PaintTextFit(Geometry, Out, Layer + 2, Why.IsEmpty() ? FString::Printf(TEXT("760 mand fra amtet  ·  +%s rd./md."), *Thousands(int32(Campaign1851Army::RaisedUpkeepPerMonth))) : Why,
			Text + FVector2D(228.f, 149.f), Serif(10, EFace::Italic), MutedInk, 140.f);
	}

	for (int32 m = 1; m <= ModuleRows; ++m)
	{
		const FVector2D Row = Card + FVector2D(0.f, 184.f + (m - 1) * RowHeight);
		TArray<FVector2D> Rule = { Row + FVector2D(18.f, -2.f), Row + FVector2D(Size.X - 18.f, -2.f) };
		FSlateDrawElement::MakeLines(Out, Layer + 2, Geometry.ToPaintGeometry(), Rule, ESlateDrawEffect::None, Gold.CopyWithNewOpacity(0.3f), true, 1.f);
		if (ModuleBrushes.IsValidIndex(m) && ModuleBrushes[m]->GetResourceObject())
		{
			FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(46.f, 46.f), FSlateLayoutTransform(Row + FVector2D(18.f, 4.f))), ModuleBrushes[m].Get());
		}
		PaintText(Geometry, Out, Layer + 2, Site->ModuleName(m), Row + FVector2D(74.f, 16.f), Serif(14), Ink, 0.f, false);
		const int32 Days = int32(Site->ModuleDays(m));
		if (Site->IsModuleDone(m))
		{
			PaintText(Geometry, Out, Layer + 2, TEXT("Færdig"), Row + FVector2D(74.f, 36.f), Serif(11, EFace::Italic), Gold, 0.f, false);
		}
		else if (Site->GetActiveModule() == m)
		{
			PaintText(Geometry, Out, Layer + 2, ProgressLine(Site, m), Row + FVector2D(74.f, 36.f), Serif(11, EFace::Italic), Ink, 0.f, false);
			PaintBar(Geometry, Out, Layer + 2, Row + FVector2D(250.f, 13.f), 110.f, Site->GetModuleProgress(m));
		}
		else if (Site->CanStartModule(m))
		{
			const bool bAfford = Map->CanAfford(Site->ModuleCost(m));
			PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s rd.  ·  %d dage%s"), *Thousands(Site->ModuleCost(m)), Days, bAfford ? TEXT("") : TEXT("  ·  ikke råd")),
				Row + FVector2D(74.f, 36.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
			PaintButton(Geometry, Out, Layer + 2, Row + FVector2D(262.f, 12.f), FVector2D(98.f, 26.f), TEXT("BYG"), EButton::BuildModule, m, false, !bAfford);
		}
		else
		{
			PaintText(Geometry, Out, Layer + 2, TEXT("Venter på byggepladsen"), Row + FVector2D(74.f, 36.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
		}
	}
}

void SCampaign1851Overlay::PaintBar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, float Width, float Fraction) const
{
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2D(Width, 6.f), FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, Gold.CopyWithNewOpacity(0.25f));
	FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(FVector2D(Width * FMath::Clamp(Fraction, 0.f, 1.f), 6.f), FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, Gold);
}

void SCampaign1851Overlay::PaintButton(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size,
	const FString& Text, EButton Action, int32 Module, bool bHighlight, bool bDisabled) const
{
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None,
		bHighlight ? Gold.CopyWithNewOpacity(0.85f) : FLinearColor::FromSRGBColor(FColor(58, 40, 22, 235)));
	TArray<FVector2D> Frame = { Pos, Pos + FVector2D(Size.X, 0.f), Pos + Size, Pos + FVector2D(0.f, Size.Y), Pos };
	FSlateDrawElement::MakeLines(Out, Layer + 1, Geometry.ToPaintGeometry(), Frame, ESlateDrawEffect::None, Gold, true, 1.2f);
	// The text stays inside its box: a smaller type when it is long, cut short as a last resort.
	const FLinearColor TextColour = bHighlight ? FLinearColor::FromSRGBColor(FColor(30, 22, 12)) : bDisabled ? MutedInk.CopyWithNewOpacity(0.45f) : Ink;
	const float Room = Size.X - 8.f;
	FSlateFontInfo Font = Serif(11);
	for (int32 Pt = 10; Pt >= 8 && Measure(Text, Font).X > Room; --Pt)
	{
		Font = Serif(Pt);
	}
	if (Measure(Text, Font).X > Room)
	{
		PaintTextFit(Geometry, Out, Layer + 1, Text, FVector2D(Pos.X + 4.f, Pos.Y + Size.Y * 0.5f), Font, TextColour, Room);
	}
	else
	{
		PaintText(Geometry, Out, Layer + 1, Text, Pos + Size * 0.5f, Font, TextColour, 0.5f, false);
	}
	if (!bDisabled)
	{
		Buttons.Add({ Pos, Pos + Size, Action, Module });
	}
	const FString Tip = ButtonTip(Action, Module);
	if (!Tip.IsEmpty())
	{
		Tips.Add({ Pos, Pos + Size, bDisabled ? Tip + TEXT("  (ikke muligt lige nu)") : Tip });
	}
}

FString SCampaign1851Overlay::ButtonTip(EButton Action, int32 Module) const
{
	switch (Action)
	{
	case EButton::Menu: return TEXT("Spilmenuen: gem, indlæs, nyt spil og afslut (M)");
	case EButton::FortTool: return TEXT("Skanser: byg og bemand feltbefæstninger på kortet");
	case EButton::OpenGazette: return TEXT("Avisen: nyheder, statistik, grafer og opslagsværket");
	case EButton::OpenBattlefield: return TEXT("Slagmarken (test): generér en slagmark et sted på kortet og se den i 3D");
	case EButton::OpenMateriel: return TEXT("Materiel: geværer, kanoner, heste og råvarer på lager; køb og produktion");
	case EButton::Speed: return TEXT("Tidens gang: pause og hastighed 1-5 (mellemrum: pause)");
	case EButton::Treasury: return TEXT("Statskassen: klik for regnskabet med indtægter og udgifter");
	case EButton::Footing: return Map.IsValid() && Map->GetFooting() == ECampaign1851Footing::Peace
		? TEXT("Mobilisér hæren: de hjemsendte (65 %) kaldes ind over 2-3 uger. Koster 20.000 rd. nu og 3 rd. pr. ekstra mand om måneden; skatterne falder 10 %, og spændingen stiger")
		: TEXT("Hjemsend: hæren går på fredsfod, og 65 % af mandskabet sendes hjem (billigere, men langsom at kalde ind igen)");
	case EButton::TrainingProgram: return TEXT("Øvelser: hvad regimentet træner i garnison (eksercits, skydeøvelser, felt, march, bajonet, blandet). Indøver også de udforskede ildmetoder");
	case EButton::ProgramPick: return TEXT("Vælg øvelsesprogram: eksercits og skydeøvelser indøver ildmetoderne; hvile sparer penge, men færdighederne falder");
	case EButton::RouteMode: return TEXT("Rute: veje og tog (hurtigst), kun veje (til fods) eller lige linje over markerne (langsom)");
	case EButton::ArmyHalt: return TEXT("Holdt: enheden standser, hvor den er");
	case EButton::ArmyHome: return TEXT("Hjem: tilbage til garnisonsbyen");
	case EButton::ArmyCancel: return TEXT("Fortryd ordren: tilbage til hvor ordren fandt den");
	case EButton::OfficerChange: return TEXT("Skift chef: vælg en anden officer til enheden");
	case EButton::GeneralChange: return TEXT("General: en general, hvis hovedkvarter marcherer med enheden");
	case EButton::OfficerRecruit: return TEXT("Hvervning: ansæt en ny officer (løn hver måned)");
	case EButton::OfficerPromote: return TEXT("Forfrem: højere grad giver kommando over større enheder (og højere løn)");
	case EButton::OfficerDismiss: return TEXT("Afsked: officeren forlader hæren");
	case EButton::OfficerInfo: return TEXT("Officerens kort: evner, erfaring, alder og karriere");
	case EButton::OpenOOB: return TEXT("Kampordenen: hærens organisation; træk enheder mellem formationer");
	case EButton::RaiseBattalion: return TEXT("Opret en ny bataljon: koster rekrutter fra amterne, geværer fra lageret og penge");
	case EButton::SupplySend: return TEXT("Send forsyninger: en trænkolonne fra depotet til enheden");
	case EButton::SupplyBuy: return TEXT("Køb forsyninger: proviant, foder og ammunition til depotet");
	case EButton::BattleFight3D: return TEXT("Udkæmp slaget i 3D: du fører tropperne selv på slagmarken");
	case EButton::BattleAuto: return TEXT("Afgør automatisk: slaget beregnes ud fra styrke, terræn, officerer og doktrin");
	case EButton::BattleRetreat: return TEXT("Tilbagetog: undgå slaget (moralen falder, og fjenden rykker frem)");
	case EButton::Diplomacy: return TEXT("Diplomati: gesandter, traktater, garantier og alliancer");
	case EButton::MakePeace: return TEXT("Fredsforslag: tilbyd fjenden fred på de viste vilkår");
	case EButton::ResearchStart: return TEXT("Start forskningen: betales måned for måned; kun ét projekt ad gangen");
	case EButton::ResearchPick: return TEXT("Klik for detaljer: virkning, pris og hvad det kræver");
	case EButton::DoctrineSet: return TEXT("Doktrin: hvordan hæren kæmper. Et skift koster penge, lidt moral og en omstillingstid");
	case EButton::Delegate: return TEXT("Ministeriets tilstand: MANUEL (du bestemmer), RÅDGIVER (ministeriet foreslår, du udfører) eller AUTO (ministeriet handler selv inden for reserven)");
	case EButton::DelegateAll: return TEXT("Sæt alle ministerier på samme tilstand");
	case EButton::Reserve: return TEXT("Reserven: det beløb ministrene ikke må røre i statskassen");
	case EButton::DecisionExecute: return TEXT("Udfør ministerens forslag");
	case EButton::MinisterDismiss: return TEXT("Udskift ministeren: vælg en ny blandt kandidaterne (strømning og evner)");
	case EButton::MinistryBudget: return TEXT("Ministeriets budget: hvor meget det må bruge om måneden");
	case EButton::Loan: return TEXT("Statslån: penge nu mod renter og afdrag");
	case EButton::Blockade: return TEXT("Blokade: flåden spærrer fjendens havne (kræver overlegenhed)");
	case EButton::ShipOrder: return TEXT("Byg skib: orlogsværftet bygger det over måneder");
	case EButton::TrainOrder: case EButton::TrainMove: return TEXT("Tog: kør tropper med jernbanen inden for samme banenet");
	case EButton::BattleViewEnter: return TEXT("Se slagmarken i 3D");
	case EButton::Build: case EButton::BuildTown: return TEXT("Byg: bygningen opføres over tid; prisen trækkes, efterhånden som der bygges");
	case EButton::Demolish: return TEXT("Nedriv bygningen (pengene kommer ikke igen)");
	case EButton::FortGuns: return TEXT("Kanoner i skansen: flere kanoner, stærkere forsvar");
	case EButton::FortTrenches: return TEXT("Løbegrave: dækning for infanteriet foran skansen");
	case EButton::ForeignTab: return TEXT("Skift mellem landene");
	case EButton::WindowClose: case EButton::ClosePanel: return TEXT("Luk");
	case EButton::TableSort: return TEXT("Sortér efter kolonnen (klik igen: den anden vej)");
	default: return FString();
	}
}

void SCampaign1851Overlay::PaintTooltip(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	// The topmost tip under the cursor, after a short moment, beside the cursor (kept on the screen).
	const FVector2D Cursor = Geometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
	const FTipRect* Hit = nullptr;
	for (int32 t = Tips.Num() - 1; t >= 0; --t)
	{
		if (Cursor.X >= Tips[t].Min.X && Cursor.Y >= Tips[t].Min.Y && Cursor.X <= Tips[t].Max.X && Cursor.Y <= Tips[t].Max.Y)
		{
			Hit = &Tips[t];
			break;
		}
	}
	const double Now = FPlatformTime::Seconds();
	if (!Hit)
	{
		TipShown.Reset();
		return;
	}
	if (Hit->Text != TipShown)
	{
		TipShown = Hit->Text;
		TipSince = Now;
	}
	if (Now - TipSince < 0.45)
	{
		return;
	}
	// Wrapped to 420 px.
	const FSlateFontInfo Font = Serif(12);
	TArray<FString> Words, Lines;
	Hit->Text.ParseIntoArray(Words, TEXT(" "));
	FString Line;
	for (const FString& Word : Words)
	{
		const FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		if (Measure(Try, Font).X > 420.f && !Line.IsEmpty())
		{
			Lines.Add(Line);
			Line = Word;
		}
		else
		{
			Line = Try;
		}
	}
	Lines.Add(Line);
	float W = 0.f;
	for (const FString& L : Lines) { W = FMath::Max(W, Measure(L, Font).X); }
	const FVector2D Size(W + 24.f, Lines.Num() * 18.f + 16.f);
	const FVector2D Screen = Geometry.GetLocalSize();
	FVector2D Pos = Cursor + FVector2D(18.f, 20.f);
	Pos.X = FMath::Min(Pos.X, Screen.X - Size.X - 8.f);
	Pos.Y = Pos.Y + Size.Y > Screen.Y - 8.f ? Cursor.Y - Size.Y - 8.f : Pos.Y;
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	DrawLines(Geometry, Out, Layer + 1, { Pos, Pos + FVector2D(Size.X, 0.f), Pos + Size, Pos + FVector2D(0.f, Size.Y), Pos }, Gold, 1.f);
	for (int32 l = 0; l < Lines.Num(); ++l)
	{
		PaintText(Geometry, Out, Layer + 2, Lines[l], Pos + FVector2D(12.f, 8.f + l * 18.f), Font, Ink, 0.f, false);
	}
}

void SCampaign1851Overlay::PaintConfirm(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	// The question in the middle of the screen; only its two buttons are live.
	Buttons.Reset();
	const FVector2D Screen = Geometry.GetLocalSize();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Screen, FSlateLayoutTransform(FVector2D::ZeroVector)), White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.45f));
	const FSlateFontInfo Font = Serif(14);
	TArray<FString> Words, Lines;
	ConfirmText.ParseIntoArray(Words, TEXT(" "));
	FString Line;
	for (const FString& Word : Words)
	{
		const FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		if (Measure(Try, Font).X > 560.f && !Line.IsEmpty()) { Lines.Add(Line); Line = Word; }
		else { Line = Try; }
	}
	Lines.Add(Line);
	const FVector2D Size(620.f, 150.f + Lines.Num() * 22.f);
	const FVector2D Pos((Screen - Size) * 0.5f);
	PaintPanel(Geometry, Out, Layer + 1, Pos, Size);
	DrawLines(Geometry, Out, Layer + 2, { Pos, Pos + FVector2D(Size.X, 0.f), Pos + Size, Pos + FVector2D(0.f, Size.Y), Pos }, Gold, 1.5f);
	PaintText(Geometry, Out, Layer + 3, ConfirmTitle, Pos + FVector2D(30.f, 40.f), Serif(20), Ink, 0.f, false);
	for (int32 l = 0; l < Lines.Num(); ++l)
	{
		PaintText(Geometry, Out, Layer + 3, Lines[l], Pos + FVector2D(30.f, 80.f + l * 22.f), Font, Ink, 0.f, false);
	}
	PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(30.f, Size.Y - 52.f), FVector2D(260.f, 32.f), TEXT("JA, GØR DET"), EButton::ConfirmYes, 0, true);
	PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 230.f, Size.Y - 52.f), FVector2D(200.f, 32.f), TEXT("NEJ"), EButton::ConfirmNo, 0);
}

void SCampaign1851Overlay::PaintTransfer(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	// How many men go from one company to the other: the number is changed with the buttons; only this window is live.
	Buttons.Reset();
	const FVector2D Screen = Geometry.GetLocalSize();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Screen, FSlateLayoutTransform(FVector2D::ZeroVector)), White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.45f));
	const bool bBattery = Map->GetRegiments().IsValidIndex(TransferReg) && Map->GetRegiments()[TransferReg].Arm == ECampaign1851Arm::Artillery;
	const FVector2D Size(560.f, bBattery ? 350.f : 300.f);
	const FVector2D Pos((Screen - Size) * 0.5f);
	PaintPanel(Geometry, Out, Layer + 1, Pos, Size);
	DrawLines(Geometry, Out, Layer + 2, { Pos, Pos + FVector2D(Size.X, 0.f), Pos + Size, Pos + FVector2D(0.f, Size.Y), Pos }, Gold, 1.5f);
	PaintText(Geometry, Out, Layer + 3, bBattery ? TEXT("Flyt mænd og kanoner mellem sektioner") : TEXT("Flyt mænd mellem kompagnier"), Pos + FVector2D(30.f, 36.f), Serif(20), Ink, 0.f, false);
	const int32 PreviewCount = TransferMax > 0 ? TransferCount : 0;
	const int32 A = Map->CompanyMen(TransferReg, TransferFrom), B = Map->CompanyMen(TransferToReg, TransferTo), Cap = Map->CompanyCapacity(TransferToReg), CapFrom = Map->CompanyCapacity(TransferReg);
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	auto Label = [&](int32 Reg, int32 K)
	{
		const bool bCompany = Regs.IsValidIndex(Reg) && Regs[Reg].Captains.Num() > 0;
		const FString Unit = TransferReg != TransferToReg && Regs.IsValidIndex(Reg) ? FString::Printf(TEXT("%s, "), *Regs[Reg].Name) : FString();
		return FString::Printf(TEXT("%s%d. %s"), *Unit, bCompany ? Map->CompanyNumber(Reg, K) : K + 1, bCompany ? TEXT("kompagni") : bBattery ? TEXT("sektion") : TEXT("eskadron"));
	};
	PaintTextFit(Geometry, Out, Layer + 3, FString::Printf(TEXT("%s:  %d/%d  →  %d"), *Label(TransferReg, TransferFrom), A, CapFrom, A - PreviewCount), Pos + FVector2D(30.f, 82.f), Serif(15), Ink, Size.X - 60.f);
	PaintTextFit(Geometry, Out, Layer + 3, FString::Printf(TEXT("%s:  %d/%d  →  %d"), *Label(TransferToReg, TransferTo), B, Cap, B + PreviewCount), Pos + FVector2D(30.f, 110.f), Serif(15), Ink, Size.X - 60.f);
	PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("Flyt  %d  mand  (højst %d)"), PreviewCount, TransferMax), Pos + FVector2D(Size.X * 0.5f, 160.f), Serif(22, EFace::Bold), Gold, 0.5f, false);
	const float BW = 70.f, BY = 190.f;
	const TCHAR* Labels[] = { TEXT("-10"), TEXT("-1"), TEXT("+1"), TEXT("+10"), TEXT("ALLE"), TEXT("LIGE") };
	const int32 Modules[] = { 990, 999, 1001, 1010, 5000, 5001 };
	for (int32 i = 0; i < 6; ++i)
	{
		PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(30.f + i * (BW + 12.f), BY), FVector2D(BW, 30.f), Labels[i], EButton::TransferAdj, Modules[i]);
	}
	PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(30.f, Size.Y - 52.f), FVector2D(150.f, 32.f), TEXT("FLYT"), EButton::TransferYes, 0, true, TransferMax <= 0);
	if (bBattery)
	{
		PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(30.f, 232.f), FVector2D(240.f, 30.f),
			FString::Printf(TEXT("FLYT 1 KANON (%d → %d)"), Map->SectionResource(TransferReg, TransferFrom, 0), Map->SectionResource(TransferToReg, TransferTo, 0)),
			EButton::TransferGun, 0, false, Map->SectionResource(TransferReg, TransferFrom, 0) <= 0);
	}
	if (TransferReg != TransferToReg)
	{
		PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(195.f, Size.Y - 52.f), FVector2D(210.f, 32.f), TEXT("FLYT HELE ENHEDEN"), EButton::TransferWhole, 0);
	}
	PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 150.f, Size.Y - 52.f), FVector2D(120.f, 32.f), TEXT("FORTRYD"), EButton::TransferNo, 0);
}

SCampaign1851Overlay::EButton SCampaign1851Overlay::HitButton(const FVector2D& ViewportPixel, int32* OutModule) const
{
	const FVector2D Local = ViewportPixel / FMath::Max(PaintScale, 0.01f);
	for (int32 b = Buttons.Num() - 1; b >= 0; --b)
	{
		const FButtonRect& B = Buttons[b];
		if (Local.X >= B.Min.X && Local.Y >= B.Min.Y && Local.X <= B.Max.X && Local.Y <= B.Max.Y)
		{
			if (OutModule)
			{
				*OutModule = B.Module;
			}
			return B.Action;
		}
	}
	return EButton::None;
}

void SCampaign1851Overlay::PaintProjects(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	TMap<int32, int32> RingsPerTown;
	for (const ACampaign1851ConstructionSite* Site : Map->GetProjects())
	{
		FVector2D P;
		const int32 Module = Site ? Site->GetActiveModule() : INDEX_NONE;
		if (Module == INDEX_NONE || !Cities.IsValidIndex(Site->GetCityIndex()) || !ToLocal(Geometry, Cities[Site->GetCityIndex()].World, P))
		{
			continue;
		}
		const float Progress = Site->GetModuleProgress(Module);
		const float Rate = FMath::Max(0.1f, Campaign1851Buildings::WorkRate(Site->ModuleType(Module), Map->GetDate()));
		const int32 DaysLeft = FMath::CeilToInt((Site->ModuleDays(Module) - Site->GetModuleElapsedDays(Module)) / Rate);
		// A ring above the town dot: faint full circle, gold arc for the progress (clockwise from the top).
		const FVector2D Centre = P + FVector2D(0.f, -30.f - 32.f * RingsPerTown.FindOrAdd(Site->GetCityIndex())++);
		const float Radius = 12.f;
		auto Arc = [&](float Fraction, const FLinearColor& Colour, float Thickness)
		{
			TArray<FVector2D> Points;
			const int32 Steps = FMath::Max(2, FMath::CeilToInt(48 * Fraction));
			for (int32 s = 0; s <= Steps; ++s)
			{
				const float A = -UE_HALF_PI + UE_TWO_PI * Fraction * s / Steps;
				Points.Add(Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius);
			}
			FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour, true, Thickness);
		};
		PaintDot(Geometry, Out, Layer, Centre, Radius * 2.f + 4.f, Panel.CopyWithNewOpacity(0.8f));
		Arc(1.f, Gold.CopyWithNewOpacity(0.3f), 2.f);
		Arc(FMath::Max(1.f - Progress, 0.01f), Gold, 3.f);   // what is left: the ring empties as the work is done
		PaintText(Geometry, Out, Layer + 1, FString::FromInt(DaysLeft), Centre, Serif(9), Ink, 0.5f, false);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%s · %s"), *Site->ModuleName(Module), Site->IsStalled() ? TEXT("standset, mangler penge") : *Site->GetStageName(Module)),
			Centre + FVector2D(Radius + 8.f, 0.f), Serif(11, EFace::Italic), Ink, 0.f);
	}
}

void SCampaign1851Overlay::PaintLinkWorks(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	const TArray<FCampaign1851Link>& Links = Map->GetLinks();
	for (int32 i = 0; i < Links.Num(); ++i)
	{
		const FCampaign1851Link& L = Links[i];
		const TArray<FVector2D>& Line = L.Work == ECampaign1851LinkWork::Railway ? L.RailPath : L.Km;
		FVector2D P;
		if (L.Work == ECampaign1851LinkWork::None || !ToLocal(Geometry, Map->WorldAtKm(ACampaign1851Map::AlongLine(Line, ACampaign1851Map::LineLength(Line) * 0.5)), P))
		{
			continue;
		}
		const float Radius = 11.f, Progress = L.Progress();
		const float Rate = FMath::Max(0.1f, Campaign1851Buildings::WorkRate(Campaign1851Network::WorkType(), Map->GetDate()));
		const int32 DaysLeft = FMath::CeilToInt((L.WorkDays - L.DaysBuilt) / Rate);
		auto Arc = [&](float Fraction, const FLinearColor& Colour, float Thickness)
		{
			TArray<FVector2D> Points;
			const int32 Steps = FMath::Max(2, FMath::CeilToInt(48 * Fraction));
			for (int32 s = 0; s <= Steps; ++s)
			{
				const float A = -UE_HALF_PI + UE_TWO_PI * Fraction * s / Steps;
				Points.Add(P + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius);
			}
			FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour, true, Thickness);
		};
		PaintDot(Geometry, Out, Layer, P, Radius * 2.f + 4.f, Panel.CopyWithNewOpacity(0.8f));
		Arc(1.f, Gold.CopyWithNewOpacity(0.3f), 2.f);
		Arc(FMath::Max(1.f - Progress, 0.01f), Gold, 3.f);
		PaintText(Geometry, Out, Layer + 1, FString::FromInt(DaysLeft), P, Serif(9), Ink, 0.5f, false);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%s %s–%s%s"), L.Work == ECampaign1851LinkWork::Railway ? TEXT("Jernbane") : TEXT("Chaussé"),
			*Cities[L.A].Name, *Cities[L.B].Name, L.bStalled ? TEXT(" · standset") : TEXT("")), P + FVector2D(Radius + 8.f, 0.f), Serif(11, EFace::Italic), Ink, 0.f);
	}
}

// ------------------------------------------------------------------ army

namespace
{
	const FLinearColor DanishBlue = FLinearColor::FromSRGBColor(FColor(36, 70, 150, 235));
	const FLinearColor RouteBlue = FLinearColor::FromSRGBColor(FColor(110, 180, 255));

	/** A small locomotive (boiler, cab, chimney, wheels) centred at C, about 2.2 S wide. */
	void PaintLoco(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& C, float S, const FLinearColor& Colour)
	{
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		auto Rect = [&](float X0, float Y0, float X1, float Y1)
		{
			FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2D((X1 - X0) * S, (Y1 - Y0) * S), FSlateLayoutTransform(C + FVector2D(X0, Y0) * S)), White, ESlateDrawEffect::None, Colour);
		};
		Rect(-1.1f, -0.25f, 0.45f, 0.35f);   // boiler
		Rect(0.35f, -0.75f, 1.05f, 0.35f);   // cab
		Rect(-0.9f, -0.75f, -0.6f, -0.25f);  // chimney
		Rect(-1.2f, 0.35f, 1.1f, 0.45f);     // frame
		for (float X : { -0.75f, -0.15f, 0.65f })
		{
			TArray<FVector2D> Wheel;
			for (int32 a = 0; a <= 10; ++a)
			{
				Wheel.Add(C + (FVector2D(X, 0.62f) + FVector2D(FMath::Cos(a * UE_TWO_PI / 10.f), FMath::Sin(a * UE_TWO_PI / 10.f)) * 0.22f) * S);
			}
			DrawLines(Geometry, Out, Layer, Wheel, Colour, 1.5f);
		}
	}

	/** NATO-style symbol inside a counter's frame. */
	void PaintArmSymbol(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Min, const FVector2D& Max, ECampaign1851Arm Arm)
	{
		const FLinearColor Line = FLinearColor::White;
		DrawLines(Geometry, Out, Layer, { Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y), Min }, Line, 1.4f);
		if (Arm == ECampaign1851Arm::Cavalry)
		{
			DrawLines(Geometry, Out, Layer, { FVector2D(Min.X, Max.Y), FVector2D(Max.X, Min.Y) }, Line, 1.4f);
		}
		else if (Arm == ECampaign1851Arm::Artillery || Arm == ECampaign1851Arm::HorseArtillery)
		{
			if (Arm == ECampaign1851Arm::HorseArtillery)   // horse artillery: the gun dot with the cavalry slash
			{
				DrawLines(Geometry, Out, Layer, { FVector2D(Min.X, Max.Y), FVector2D(Max.X, Min.Y) }, Line, 1.4f);
			}
			TArray<FVector2D> Dot;
			const FVector2D C = (Min + Max) * 0.5f;
			for (int32 s = 0; s <= 12; ++s)
			{
				Dot.Add(C + FVector2D(FMath::Cos(s * UE_TWO_PI / 12.f), FMath::Sin(s * UE_TWO_PI / 12.f)) * 3.2f);
			}
			DrawLines(Geometry, Out, Layer, Dot, Line, 3.f);
		}
		else
		{
			DrawLines(Geometry, Out, Layer, { Min, Max }, Line, 1.4f);
			DrawLines(Geometry, Out, Layer, { FVector2D(Min.X, Max.Y), FVector2D(Max.X, Min.Y) }, Line, 1.4f);
			if (Arm == ECampaign1851Arm::Guard)   // a bar over the frame for the guard
			{
				DrawLines(Geometry, Out, Layer, { Min + FVector2D(4.f, -3.f), FVector2D(Max.X - 4.f, Min.Y - 3.f) }, Line, 1.4f);
			}
			else if (Arm == ECampaign1851Arm::Jager)   // light infantry: a small horn under the frame
			{
				DrawLines(Geometry, Out, Layer, { FVector2D(Min.X + 6.f, Max.Y + 3.f), FVector2D(Max.X - 6.f, Max.Y + 3.f) }, Line, 1.4f);
			}
		}
	}
}

void SCampaign1851Overlay::PaintArmy(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	const FDateTime Now = Map->GetDate();

	// Training labels share the army marker projection at every zoom level.
	for (int32 i = 0; i < Regs.Num(); ++i)
	{
		const FCampaign1851Regiment& R = Regs[i];
		FVector2D At;
		if (!R.bTraining || !ToLocal(Geometry, Map->RegimentWorld(i), At)) { continue; }
		int32 Offset = 0;
		for (int32 j = 0; j < i; ++j) { Offset += Regs[j].bTraining && Regs[j].Town == R.Town ? 1 : 0; }
		At += FVector2D(32.f, 20.f + Offset * 20.f);
		PaintButton(Geometry, Out, Layer + 10, At, FVector2D(150.f, 18.f),
			FString::Printf(TEXT("%s: træner %.0f %%"), *R.Name, R.RaisingProgress * 100.f), EButton::RegimentPiece, i);
	}

	// Routes of the selected regiments: what is left of the march, and the arrival time.
	TSet<int32> DrawnGroups;
	for (int32 i : SelectedRegiments)
	{
		if (!Regs.IsValidIndex(i) || !Regs[i].IsMarching() || (Regs[i].Group && DrawnGroups.Contains(Regs[i].Group)))
		{
			continue;
		}
		DrawnGroups.Add(Regs[i].Group);
		const FCampaign1851Regiment& R = Regs[i];
		for (int32 l = R.Leg; l < R.Route.Num(); ++l)
		{
			TArray<FVector2D> Line = Map->LegLine(R.Route[l]);
			if (l == R.Leg)
			{
				// From where the column is now.
				const double Length = ACampaign1851Map::LineLength(Line);
				const double Done = Length * FMath::Clamp(R.LegElapsed / FMath::Max(R.Route[l].Days, 0.001f), 0.f, 1.f);
				TArray<FVector2D> Rest = { R.Km };
				double At = 0.0;
				for (int32 p = 0; p + 1 < Line.Num(); ++p)
				{
					At += FVector2D::Distance(Line[p], Line[p + 1]);
					if (At > Done)
					{
						Rest.Add(Line[p + 1]);
					}
				}
				Line = MoveTemp(Rest);
			}
			TArray<FVector2D> Screen;
			const int32 Step = FMath::Max(1, Line.Num() / 40);
			for (int32 p = 0; p < Line.Num(); p += Step)
			{
				FVector2D S;
				if (ToLocal(Geometry, Map->WorldAtKm(Line[p]), S))
				{
					Screen.Add(S);
				}
			}
			FVector2D Last;
			if (Line.Num() > 0 && ToLocal(Geometry, Map->WorldAtKm(Line.Last()), Last))
			{
				Screen.Add(Last);
			}
			if (Screen.Num() >= 2)
			{
				DrawLines(Geometry, Out, Layer, Screen, Shadow, 5.f);
				DrawLines(Geometry, Out, Layer + 1, Screen, R.Route[l].bRail ? Ink : RouteBlue, 2.5f);
			}
		}
		FVector2D End;
		if (ToLocal(Geometry, Map->WorldAtKm(R.DestinationKm()), End))
		{
			PaintDot(Geometry, Out, Layer + 1, End, 14.f, RouteBlue);
			PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("ankomst %s %s"), *ACampaign1851Map::FormatClock(Now + FTimespan::FromDays(R.DaysLeft())),
				*ACampaign1851Map::FormatDate(Now + FTimespan::FromDays(R.DaysLeft()), true)), End + FVector2D(12.f, 16.f), Serif(11, EFace::Italic), RouteBlue, 0.f);
		}
	}

	// Bridges (close in): a small clickable sign; red when blown, dashed where a pontoon bridge could be laid.
	if (Map->GetCameraDistanceKm() < 120.f)
	{
		for (const FCampaign1851Bridge& Bd : Map->GetBridges())
		{
			FVector2D P;
			if (!ToLocal(Geometry, Map->WorldAtKm(Bd.Km), P))
			{
				continue;
			}
			const bool bSite = Bd.State == EBridgeState::Site;
			if (bSite && Map->GetCameraDistanceKm() > 60.f)
			{
				continue;
			}
			const TCHAR* Sign = Bd.State == EBridgeState::Blown ? TEXT("BRO X") : Bd.State == EBridgeState::Building ? TEXT("BRO ...") : bSite ? TEXT("(bro)") : TEXT("BRO");
			PaintButton(Geometry, Out, Layer + 6, P - FVector2D(22.f, 9.f), FVector2D(44.f, 18.f), FString(), EButton::BridgeSelect, Bd.Id, SelectedBridge == Bd.Id);
			PaintText(Geometry, Out, Layer + 7, Sign, P, Serif(9), Bd.State == EBridgeState::Blown ? FLinearColor(1.f, 0.45f, 0.4f) : bSite ? MutedInk : Ink, 0.5f);
		}
	}

	// The enemy: red counters with the corps' strength (at every zoom), as far as it is known: where it was
	// last seen or reported, faded when the sighting is old.
	const TArray<FCampaign1851EnemyCorps>& Corps = Map->GetEnemyCorps();
	for (int32 k = 0; k < Corps.Num(); ++k)
	{
		const FCampaign1851EnemyCorps& C = Corps[k];
		if (C.SeenDay < 0.0)
		{
			continue;   // never seen nor reported
		}
		const bool bNow = C.bSeen;
		const float Fade = bNow ? 1.f : 0.55f;
		FVector2D P;
		if (!ToLocal(Geometry, Map->WorldAtKm(bNow ? C.Km : C.SeenKm), P))
		{
			continue;
		}
		// Beside the Danish counters, and beside each other where several stand together.
		int32 Before = 0;
		for (int32 j = 0; j < k; ++j)
		{
			Before += Corps[j].SeenDay >= 0.0 && FVector2D::Distance(Corps[j].bSeen ? Corps[j].Km : Corps[j].SeenKm, bNow ? C.Km : C.SeenKm) < 3.0 ? 1 : 0;
		}
		P += FVector2D(60.f + Before * 60.f, 0.f);
		const FVector2D EBox(50.f, 32.f);
		const FVector2D Min = P - FVector2D(EBox.X * 0.5f, EBox.Y + 10.f);
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		FSlateDrawElement::MakeBox(Out, Layer + 7, Geometry.ToPaintGeometry(EBox, FSlateLayoutTransform(Min)), White, ESlateDrawEffect::None, FLinearColor(0.55f, 0.08f, 0.08f, 0.92f * Fade));
		const FVector2D Max = Min + EBox;
		const FLinearColor Edge = (C.bEngaged ? Gold : Ink).CopyWithNewOpacity(Fade);
		DrawLines(Geometry, Out, Layer + 8, { Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y), Min }, Edge, C.bEngaged ? 2.5f : 1.2f);
		DrawLines(Geometry, Out, Layer + 9, { Min + FVector2D(8.f, 6.f), Max - FVector2D(8.f, 6.f) }, Ink.CopyWithNewOpacity(Fade), 1.5f);
		DrawLines(Geometry, Out, Layer + 9, { FVector2D(Min.X + 8.f, Max.Y - 6.f), FVector2D(Max.X - 8.f, Min.Y + 6.f) }, Ink.CopyWithNewOpacity(Fade), 1.5f);
		const FLinearColor Pink(1.f, 0.75f, 0.7f, Fade);
		PaintText(Geometry, Out, Layer + 9, bNow && Map->IsAtWar() ? FString::Printf(TEXT("ca. %s"), *Thousands(C.SeenMen)) : !Map->IsAtWar() ? Thousands(C.Men)
			: FString::Printf(TEXT("ca. %s?"), *Thousands(C.SeenMen)), FVector2D(P.X, Min.Y - 9.f), Serif(10), Pink, 0.5f);
		PaintText(Geometry, Out, Layer + 9, C.Nation == TEXT("AT") ? TEXT("ØSTRIG") : C.Nation == TEXT("DE") ? TEXT("FORBUNDET") : TEXT("PREUSSEN"), FVector2D(P.X, Max.Y + 8.f), Serif(9), Pink, 0.5f);
		if (!bNow)
		{
			const FDateTime Seen = ACampaign1851Map::StartDate() + FTimespan::FromDays(C.SeenDay);
			PaintText(Geometry, Out, Layer + 9, FString::Printf(TEXT("meldt %s %s"), *ACampaign1851Map::FormatClock(Seen), *ACampaign1851Map::FormatDate(Seen, true)),
				FVector2D(P.X, Max.Y + 22.f), Serif(9, EFace::Italic), Pink, 0.5f);
		}
	}

	// The supply map: each depot's reach (a day's march) and every unit's supply in colour.
	if (bSupplyMap)
	{
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			const FCampaign1851DepotStock S = Map->DepotStock(c);
			if (S.Food + S.Fodder + S.Ammo <= 1.f)
			{
				continue;
			}
			TArray<FVector2D> Ring;
			for (int32 a = 0; a <= 36; ++a)
			{
				const double A = a * UE_TWO_PI / 36.0;
				FVector2D P;
				if (ToLocal(Geometry, Map->WorldAtKm(Map->TownKm(c) + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Campaign1851Supply::DepotReachKm), P))
				{
					Ring.Add(P);
				}
			}
			if (Ring.Num() > 2)
			{
				DrawLines(Geometry, Out, Layer, Ring, FLinearColor(0.35f, 0.8f, 0.35f, 0.8f), 2.5f);
			}
		}
		for (int32 i = 0; i < Regs.Num(); ++i)
		{
			const FCampaign1851Regiment& R = Regs[i];
			FVector2D P;
			if (!ToLocal(Geometry, Map->RegimentWorld(i), P))
			{
				continue;
			}
			const bool bGarrison = !R.IsMarching() && !R.IsInField();
			const FLinearColor Status = bGarrison ? FLinearColor(0.35f, 0.75f, 0.35f) : R.Food < 1.f ? FLinearColor(0.85f, 0.25f, 0.2f) : R.Food < 2.f ? FLinearColor(0.9f, 0.75f, 0.25f) : FLinearColor(0.35f, 0.75f, 0.35f);
			PaintDot(Geometry, Out, Layer + 6, P + FVector2D(0.f, 8.f), 12.f, Status);
		}
		for (const FCampaign1851SupplyColumn& C : Map->GetSupplyColumns())
		{
			FVector2D P;
			if (ToLocal(Geometry, Map->WorldAtKm(C.Km), P))
			{
				PaintDot(Geometry, Out, Layer + 6, P, 9.f, FLinearColor(0.75f, 0.55f, 0.3f));
			}
		}
	}

	// Close in, the miniatures themselves are the regiments: each can be clicked; the counters give way.
	if (Map->GetCameraDistanceKm() < ACampaign1851Map::MiniatureViewKm)
	{
		const FSlateBrush* Ring = FCoreStyle::Get().GetBrush("WhiteBrush");
		for (int32 i = 0; i < Regs.Num(); ++i)
		{
			const FCampaign1851Regiment& R = Regs[i];
			if (R.IsMarching() && R.Route[R.Leg].bRail)
			{
				// Riding a train: a sign over the train (one for the column), with the locomotive, the unit and
				// where it goes; it can be clicked like the miniature.
				bool bFirstOfColumn = true;
				for (int32 j = 0; j < i && R.Group != 0; ++j)
				{
					bFirstOfColumn &= !(Regs[j].Group == R.Group && Regs[j].IsMarching() && Regs[j].Route[Regs[j].Leg].bRail);
				}
				FVector2D P;
				if (!bFirstOfColumn || !ToLocal(Geometry, Map->RegimentWorld(i) + FVector(0.0, 0.0, 30.0), P))
				{
					continue;
				}
				int32 Units = 0, Men = 0;
				bool bSel = false;
				for (int32 j = 0; j < Regs.Num(); ++j)
				{
					if (j == i || (R.Group != 0 && Regs[j].Group == R.Group && Regs[j].IsMarching() && Regs[j].Route[Regs[j].Leg].bRail))
					{
						++Units;
						Men += Regs[j].Men;
						bSel |= SelectedRegiments.Contains(j);
					}
				}
				int32 Last = R.Leg;
				while (R.Route.IsValidIndex(Last + 1) && R.Route[Last + 1].bRail) { ++Last; }
				const int32 To = R.Route[Last].To;
				const FString Label = FString::Printf(TEXT("%s%s  ·  %d  ·  til %s"), *R.Name, Units > 1 ? *FString::Printf(TEXT(" m.fl. (%d)"), Units) : TEXT(""), Men,
					Cities.IsValidIndex(To) ? *Cities[To].Name : TEXT("?"));
				const FVector2D LabelSize = Measure(Label, Serif(11)) + FVector2D(40.f, 8.f);
				const FVector2D At(P.X - LabelSize.X * 0.5f, P.Y - LabelSize.Y - 10.f);
				FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(At)), Ring, ESlateDrawEffect::None, bSel ? DanishBlue * 1.35f : DanishBlue);
				DrawLines(Geometry, Out, Layer + 3, { At, FVector2D(At.X + LabelSize.X, At.Y), At + LabelSize, FVector2D(At.X, At.Y + LabelSize.Y), At }, bSel ? Gold : Gold.CopyWithNewOpacity(0.6f), bSel ? 2.f : 1.f);
				PaintLoco(Geometry, Out, Layer + 3, At + FVector2D(16.f, LabelSize.Y * 0.5f - 2.f), 7.f, Ink);
				PaintText(Geometry, Out, Layer + 3, Label, At + FVector2D(32.f, LabelSize.Y * 0.5f), Serif(11), Ink, 0.f, false);
				DrawLines(Geometry, Out, Layer + 2, { FVector2D(P.X, At.Y + LabelSize.Y), P }, Gold.CopyWithNewOpacity(0.7f), 1.f);
				Buttons.Add({ At, At + LabelSize, EButton::RegimentPiece, i });
				continue;
			}
			const FVector World = Map->RegimentWorld(i);
			FVector2D P, Edge;
			if (!ToLocal(Geometry, World, P) || !ToLocal(Geometry, World + FVector(25.0, 0.0, 0.0), Edge))
			{
				continue;
			}
			const float Radius = FMath::Clamp(float(FVector2D::Distance(P, Edge)), 16.f, 90.f);
			const FVector2D Min = P - FVector2D(Radius, Radius * 1.2f), Max = P + FVector2D(Radius, Radius * 0.5f);
			Buttons.Add({ Min, Max, EButton::RegimentPiece, i });
			if (SelectedRegiments.Contains(i))
			{
				// A gold frame round the chosen miniature, with its name and strength beneath.
				DrawLines(Geometry, Out, Layer + 2, { Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y), Min }, Gold, 2.f);
				const FString Label = FString::Printf(TEXT("%s  ·  %d"), *R.Name, R.Men);
				const FVector2D LabelSize = Measure(Label, Serif(11)) + FVector2D(12.f, 6.f);
				// Labels of chosen units close together stack downwards instead of overlapping.
				int32 Below = 0;
				for (int32 j : SelectedRegiments)
				{
					FVector2D Q;
					Below += j < i && ToLocal(Geometry, Map->RegimentWorld(j), Q) && FVector2D::Distance(Q, P) < 220.f ? 1 : 0;
				}
				const FVector2D At(P.X - LabelSize.X * 0.5f, Max.Y + 4.f + Below * (LabelSize.Y + 2.f));
				FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(At)), Ring, ESlateDrawEffect::None, Panel);
				PaintText(Geometry, Out, Layer + 3, Label, At + FVector2D(6.f, LabelSize.Y * 0.5f), Serif(11), Ink, 0.f, false);
			}
		}
		return;
	}

	// Stacks: every regiment in a town together, every column on the march together.
	TMap<int64, TArray<int32>> Stacks;
	for (int32 i = 0; i < Regs.Num(); ++i)
	{
		const FCampaign1851Regiment& R = Regs[i];
		// Stacks: a town's regiments; a column on the march; regiments halted together in the field (same ~200 m).
		const int64 Field = (int64(3) << 32) + (int64(FMath::RoundToInt(R.Km.X * 5.0) & 0xFFFF) << 16) + (FMath::RoundToInt(R.Km.Y * 5.0) & 0xFFFF);
		const int64 Key = R.IsInField() ? Field : !R.IsMarching() ? int64(R.Town) : R.Group ? (int64(1) << 32) + R.Group : (int64(2) << 32) + i;
		Stacks.FindOrAdd(Key).Add(i);
	}
	const FVector2D Box(46.f, 30.f);
	for (const TPair<int64, TArray<int32>>& Stack : Stacks)
	{
		const FCampaign1851Regiment& Top = Regs[Stack.Value[0]];
		FVector2D P;
		const bool bOnGround = Top.IsMarching() || Top.IsInField();
		const FVector World = bOnGround ? Map->RegimentWorld(Stack.Value[0]) : Cities[Top.Town].World;
		if (!ToLocal(Geometry, World, P))
		{
			continue;
		}
		// In a town the counter stands above the dot; on the march on the column.
		// Above the column (or the town dot), so the miniature below stays in view.
		const FVector2D Min = P + (bOnGround ? FVector2D(-Box.X * 0.5f, -Box.Y - 26.f) : FVector2D(-Box.X * 0.5f, -Box.Y - 14.f));
		bool bSelected = false;
		int32 Men = 0;
		ECampaign1851Arm Arm = Top.Arm;
		int32 Biggest = 0;
		for (int32 i : Stack.Value)
		{
			bSelected |= SelectedRegiments.Contains(i);
			Men += Regs[i].Men;
			if (Regs[i].Men > Biggest)
			{
				Biggest = Regs[i].Men;
				Arm = Regs[i].Arm;
			}
		}
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		if (Stack.Value.Num() > 1)   // cards behind: it is a stack
		{
			FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(Box, FSlateLayoutTransform(Min + FVector2D(4.f, -4.f))), White, ESlateDrawEffect::None, DanishBlue * 0.7f);
		}
		FSlateDrawElement::MakeBox(Out, Layer + 3, Geometry.ToPaintGeometry(Box, FSlateLayoutTransform(Min)), White, ESlateDrawEffect::None, bSelected ? DanishBlue * 1.35f : DanishBlue);
		const FVector2D Max = Min + Box;
		DrawLines(Geometry, Out, Layer + 4, { Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y), Min }, bSelected ? Gold : Gold.CopyWithNewOpacity(0.6f), bSelected ? 2.5f : 1.f);
		PaintArmSymbol(Geometry, Out, Layer + 5, Min + FVector2D(9.f, 7.f), Max - FVector2D(Stack.Value.Num() > 1 ? 15.f : 9.f, 7.f), Arm);
		if (Stack.Value.Num() > 1)
		{
			PaintText(Geometry, Out, Layer + 5, FString::FromInt(Stack.Value.Num()), FVector2D(Max.X - 7.f, Min.Y + Box.Y * 0.5f), Serif(10, EFace::Bold), Ink, 0.5f, false);
		}
		PaintText(Geometry, Out, Layer + 5, FString::Printf(TEXT("%d"), Men), FVector2D(Min.X + Box.X * 0.5f, Min.Y - (Stack.Value.Num() > 1 ? 13.f : 9.f)), Serif(9), Ink, 0.5f);
		if (Top.IsMarching() && Top.Route[Top.Leg].bRail)
		{
			// On the rails: a locomotive on a piece of track under the counter, and where and when it gets off.
			const FVector2D Rail(Min.X - 6.f, Max.Y + 12.f);
			FSlateDrawElement::MakeBox(Out, Layer + 3, Geometry.ToPaintGeometry(FVector2D(Box.X + 12.f, 16.f), FSlateLayoutTransform(Rail - FVector2D(0.f, 10.f))), White, ESlateDrawEffect::None, FLinearColor(0.03f, 0.03f, 0.04f, 0.85f));
			DrawLines(Geometry, Out, Layer + 4, { Rail + FVector2D(2.f, 3.f), Rail + FVector2D(Box.X + 10.f, 3.f) }, Gold, 1.f);
			for (float X = 4.f; X < Box.X + 10.f; X += 5.f)
			{
				DrawLines(Geometry, Out, Layer + 4, { Rail + FVector2D(X, 1.f), Rail + FVector2D(X, 5.f) }, Gold.CopyWithNewOpacity(0.6f), 1.f);
			}
			PaintLoco(Geometry, Out, Layer + 5, Rail + FVector2D(Box.X * 0.5f + 6.f, -3.f), 6.f, Ink);
			int32 Last = Top.Leg;
			float Left = Top.Route[Top.Leg].Days - Top.LegElapsed;
			while (Top.Route.IsValidIndex(Last + 1) && Top.Route[Last + 1].bRail) { ++Last; Left += Top.Route[Last].Days; }
			const FDateTime There = Map->GetDate() + FTimespan::FromDays(Left);
			const FString Where = FString::Printf(TEXT("med tog til %s  ·  %s"), Cities.IsValidIndex(Top.Route[Last].To) ? *Cities[Top.Route[Last].To].Name : TEXT("?"), *ACampaign1851Map::FormatClock(There));
			const FVector2D WhereSize = Measure(Where, Serif(9)) + FVector2D(8.f, 4.f);
			const FVector2D WhereAt(Min.X + Box.X * 0.5f - WhereSize.X * 0.5f, Rail.Y + 8.f);
			FSlateDrawElement::MakeBox(Out, Layer + 3, Geometry.ToPaintGeometry(WhereSize, FSlateLayoutTransform(WhereAt)), White, ESlateDrawEffect::None, FLinearColor(0.03f, 0.03f, 0.04f, 0.75f));
			PaintText(Geometry, Out, Layer + 4, Where, WhereAt + FVector2D(4.f, WhereSize.Y * 0.5f), Serif(9, EFace::Italic), Gold, 0.f, false);
		}
		Buttons.Add({ Min, Max, EButton::Regiment, Stack.Value[0] });
	}
}

void SCampaign1851Overlay::PaintTextFit(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FString& Text, const FVector2D& Pos,
	const FSlateFontInfo& Font, const FLinearColor& Colour, float MaxWidth, float AlignX) const
{
	// Too long for its place: cut it and end with "...".
	FString Shown = Text;
	if (Measure(Shown, Font).X > MaxWidth)
	{
		while (Shown.Len() > 1 && Measure(Shown + TEXT("..."), Font).X > MaxWidth)
		{
			Shown.LeftChopInline(1);
		}
		Shown = Shown.TrimEnd() + TEXT("...");
	}
	PaintText(Geometry, Out, Layer, Shown, Pos, Font, Colour, AlignX, false);
}

void SCampaign1851Overlay::PaintArmyInfo(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	const TArray<FCampaign1851Officer>& Officers = Map->GetOfficers();
	TArray<const FCampaign1851Regiment*> Sel;
	for (int32 i : SelectedRegiments)
	{
		if (Regs.IsValidIndex(i))
		{
			Sel.Add(&Regs[i]);
		}
	}
	if (Sel.Num() == 0)
	{
		return;
	}
	const FCampaign1851Regiment& First = *Sel[0];
	const bool bSingle = Sel.Num() == 1;
	const int32 ChiefIndex = bSingle ? First.Chief : INDEX_NONE;
	const FCampaign1851Officer* Chief = Officers.IsValidIndex(ChiefIndex) ? &Officers[ChiefIndex] : nullptr;
	const FCampaign1851Officer* General = Map->ColumnGeneral(SelectedRegiments);
	const int32 GeneralIndex = General ? int32(General - Officers.GetData()) : INDEX_NONE;
	FString Why;
	float Pace = Campaign1851Army::ColumnPace(Sel, &Why);
	if (const int32 Staff = Map->ColumnStaff(SelectedRegiments))
	{
		const float Factor = Campaign1851Army::StaffPaceFactor(Staff);
		Pace *= Factor;
		if (FMath::Abs(Factor - 1.f) > 0.001f)
		{
			Why += FString::Printf(TEXT("%sstabens arbejde: %+d %%"), Why.IsEmpty() ? TEXT("") : TEXT("  ·  "), FMath::RoundToInt((Factor - 1.f) * 100.f));
		}
	}

	// ---- Layout: the panel grows upwards; a stack's table gets the room left under the treasury panel.
	const FVector2D Size0(580.f, 0.f);
	const float Inner = Size0.X - 44.f;
	float Height = 32.f + 30.f + 30.f;                         // title, subtitle
	Height += 22.f * 7.f + 10.f;                                // strength, experience, place, present, pace, trains, supply
	Height += Why.IsEmpty() ? 0.f : 20.f;
	Height += First.IsMarching() ? 22.f : 0.f;
	Height += bSingle ? 3 * 21.f + 8.f + 22.f : 0.f;            // training bars (the table shows them for a stack), fire methods
	Height += 30.f + 30.f;                                      // programme, route
	Height += 12.f + (bSingle ? 44.f : 0.f) + 44.f;             // chief, general
	Height += 76.f;                                             // buttons and hint
	const float RowHeight = 21.f;
	const float Room = Geometry.GetLocalSize().Y - 190.f - 360.f - Height - 34.f;
	const int32 Fit = FMath::Max(3, FMath::FloorToInt(Room / RowHeight));
	const int32 Rows = bSingle ? 0 : FMath::Min(Sel.Num(), Sel.Num() > Fit ? FMath::Max(0, Fit - 1) : Fit);
	const bool bMore = !bSingle && Rows < Sel.Num();
	Height += bSingle ? 0.f : 30.f + (Rows + (bMore ? 1 : 0)) * RowHeight;
	const FVector2D Size(Size0.X, Height);
	// Under the treasury panel (it ends at 312): if it does not fit above the bottom bar, it takes some of the bar's room.
	FVector2D Pos(28.f, Geometry.GetLocalSize().Y - 190.f - Size.Y);
	if (Pos.Y < 322.f)
	{
		Pos.Y = FMath::Max(322.f, Geometry.GetLocalSize().Y - 70.f - Size.Y);
	}
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseSelection);

	// ---- Totals and strength-weighted means.
	float Y = Pos.Y + 32.f;
	int32 Men = 0, MaxMen = 0, Horses = 0, Guns = 0, Cost = 0;
	float Xp = 0.f, Morale = 0.f, Cohesion = 0.f;
	float Skills[int32(ECampaign1851Skill::Count)] = {};
	for (const FCampaign1851Regiment* R : Sel)
	{
		Men += R->Men;
		MaxMen += R->MaxMen;
		Horses += R->Horses;
		Guns += R->Guns;
		Xp += R->Experience * R->Men;
		Morale += R->Morale * R->Men;
		Cohesion += R->Cohesion * R->Men;
		for (int32 s = 0; s < int32(ECampaign1851Skill::Count); ++s)
		{
			Skills[s] += R->Skills[s] * R->Men;
		}
		Cost += R->IsMarching() ? 0 : FMath::RoundToInt(Campaign1851Army::ProgramCostPerMonth(R->Program) * R->Men / 760.f);
	}
	const float W = float(FMath::Max(Men, 1));
	Xp /= W; Morale /= W; Cohesion /= W;
	for (float& V : Skills)
	{
		V /= W;
	}

	PaintTextFit(Geometry, Out, Layer + 2, bSingle ? First.Name : FString::Printf(TEXT("Kolonne: %d enheder"), Sel.Num()), FVector2D(Pos.X + 22.f, Y), Serif(22), Ink, Inner - (bSingle ? 170.f : 0.f));
	// A seen enemy corps within reach: attack it.
	{
		double Km = 0.0;
		const int32 Corps = Map->EngageableCorps(SelectedRegiments, &Km);
		if (Map->GetEnemyCorps().IsValidIndex(Corps))
		{
			const FVector2D At(Pos.X + Size.X - (bSingle ? 356.f : 196.f), Y - 12.f);
			PaintButton(Geometry, Out, Layer + 3, At, FVector2D(150.f, 24.f), FString::Printf(TEXT("ANGRIB (%.0f KM)"), Km), EButton::Engage, Corps);
			AddTip(At, FVector2D(150.f, 24.f), FString::Printf(TEXT("Angrib %s, der er opklaret %.0f km herfra: slaget tilbydes straks (udkæmp i 3D, afgør automatisk eller træk tilbage), og de valgte enheder kæmper."),
				*Map->GetEnemyCorps()[Corps].Name, Km));
		}
	}
	if (bSingle)
	{
		PaintButton(Geometry, Out, Layer + 3, FVector2D(Pos.X + Size.X - 196.f, Y - 12.f), FVector2D(150.f, 24.f), TEXT("ENHEDSKORT"), EButton::UnitCard, 0, bUnitCard);
		// The card itself is painted on top of everything (Paint), beside this panel (and beside the order of battle, if that is open).
		UnitCardAnchor = FVector2D(Pos.X + Size.X + 10.f, Pos.Y + Size.Y);
	}
	Y += 30.f;
	const bool bFootBattery = bSingle && First.Arm == ECampaign1851Arm::Artillery && First.Mortars == 0;
	if (bFootBattery)
	{
		FString HorseWhy;
		const bool bCan = Map->CanUpgradeToHorseBattery(SelectedRegiments[0], &HorseWhy);
		const FVector2D At(Pos.X + Size.X - 196.f, Y - 12.f);
		PaintButton(Geometry, Out, Layer + 3, At, FVector2D(150.f, 24.f), TEXT("GØR RIDENDE"), EButton::HorseBattery, SelectedRegiments[0], false, !bCan);
		AddTip(At, FVector2D(150.f, 24.f), FString::Printf(TEXT("Gør fodbatteriet ridende: alle kanonerer til hest, så det kan følge rytteriet. %d heste, %s rd.%s"),
			ACampaign1851Map::HorseBatteryHorses, *Thousands(int32(ACampaign1851Map::HorseBatteryCost)), bCan ? TEXT("") : *FString::Printf(TEXT("  (%s)"), *HorseWhy)));
	}
	PaintTextFit(Geometry, Out, Layer + 2, bSingle ? FString::Printf(TEXT("%s  ·  garnison %s"), Campaign1851Army::ArmName(First.Arm), *Cities[First.Home].Name) : FString(TEXT("Marcherer samlet i den langsomstes tempo")),
		FVector2D(Pos.X + 22.f, Y), Serif(13, EFace::Italic), Gold, bFootBattery ? Inner - 170.f : Inner);
	Y += 30.f;
	const float ValueX = 130.f;
	auto Line = [&](const FString& Label, const FString& Value)
	{
		PaintText(Geometry, Out, Layer + 2, Label, FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 2, Value, FVector2D(Pos.X + ValueX, Y), Serif(13), Ink, Size.X - ValueX - 22.f);
		Y += 22.f;
	};
	Line(TEXT("Styrke"), FString::Printf(TEXT("%d / %d mand%s%s"), Men, MaxMen, Horses > 50 ? *FString::Printf(TEXT("  ·  %d heste"), Horses) : TEXT(""),
		Guns > 0 ? *FString::Printf(TEXT("  ·  %d kanoner"), Guns) : Sel.ContainsByPredicate([](const FCampaign1851Regiment* R) { return R->Mortars > 0; })
			? *FString::Printf(TEXT("  ·  morterer på vogne"))
			: TEXT("")));
	Line(TEXT("Erfaring"), FString::Printf(TEXT("%s (%.0f)  ·  moral %.0f %%  ·  samhørighed %.0f"), Campaign1851Army::ExperienceName(Xp), Xp, Morale * 100.f, Cohesion));
	if (bSingle)
	{
		for (int32 s = 0; s < int32(ECampaign1851Skill::Count); ++s)
		{
			const FVector2D Cell(Pos.X + 22.f + (s % 2) * 272.f, Y + (s / 2) * 21.f);
			PaintText(Geometry, Out, Layer + 2, Campaign1851Army::SkillName(ECampaign1851Skill(s)), Cell, Serif(11, EFace::Italic), Gold, 0.f, false);
			PaintBar(Geometry, Out, Layer + 2, Cell + FVector2D(104.f, -3.f), 116.f, Skills[s] / 100.f);
			PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%.0f"), Skills[s]), Cell + FVector2D(228.f, 0.f), Serif(11), Ink, 0.f, false);
		}
		Y += 3 * 21.f + 8.f;
		// The fire methods: researched ones with their training (ready from 60).
		{
			static const TCHAR* DrillTopics[4] = { TEXT("tworank"), TEXT("firebyrank"), TEXT("volley"), TEXT("independent") };
			static const TCHAR* DrillNames[4] = { TEXT("To geledder"), TEXT("Geledild"), TEXT("Salve"), TEXT("Fri ild") };
			FString Drills;
			for (int32 d = 0; d < 4; ++d)
			{
				if (Map->HasResearch(DrillTopics[d]))
				{
					Drills += FString::Printf(TEXT("%s%s %.0f%s"), Drills.IsEmpty() ? TEXT("") : TEXT("  ·  "), DrillNames[d], First.FireDrills[d], First.FireDrills[d] >= 60.f ? TEXT(" ✓") : TEXT(""));
				}
			}
			Line(TEXT("Ildmetoder"), Drills.IsEmpty() ? FString(TEXT("Forreste geled (forsk i nye ildmetoder)")) : Drills);
		}
	}
	const bool bSameProgram = !Sel.ContainsByPredicate([&First](const FCampaign1851Regiment* R) { return R->Program != First.Program; });
	PaintText(Geometry, Out, Layer + 2, TEXT("Øvelser"), FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + ValueX, Y - 12.f), FVector2D(Size.X - ValueX - 22.f, 24.f),
		FString::Printf(TEXT("%s  ·  %s rd./md.%s"), bSameProgram ? Campaign1851Army::ProgramName(First.Program) : TEXT("Forskellige"), *Thousands(Cost),
			First.IsMarching() ? TEXT("  (ikke på march)") : TEXT("")), EButton::TrainingProgram);
	Y += 30.f;
	PaintText(Geometry, Out, Layer + 2, TEXT("Rute"), FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
	{
		const ECampaign1851RouteMode Modes[] = { ECampaign1851RouteMode::RoadsAndRail, ECampaign1851RouteMode::RoadsOnly, ECampaign1851RouteMode::Direct };
		const TCHAR* Labels[] = { TEXT("VEJE OG TOG"), TEXT("KUN VEJE"), TEXT("LIGE LINJE") };
		const float W3 = (Size.X - ValueX - 22.f - 16.f) / 3.f;
		for (int32 m = 0; m < 3; ++m)
		{
			PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + ValueX + m * (W3 + 8.f), Y - 12.f), FVector2D(W3, 24.f), Labels[m], EButton::RouteMode, m, RouteMode == Modes[m]);
		}
	}
	Y += 30.f;
	FString Where;
	if (First.IsMarching())
	{
		const FCampaign1851Leg& Leg = First.Route[First.Leg];
		Where = Leg.bWait ? FString::Printf(TEXT("Venter på tog i %s"), *Map->DescribePlace(Leg.From, Leg.FromKm))
			: FString::Printf(TEXT("%s fra %s til %s"), Leg.bRail ? TEXT("Med tog") : Leg.bOffRoad ? TEXT("Gennem terrænet") : TEXT("På march"),
				*Map->DescribePlace(Leg.From, Leg.FromKm), *Map->DescribePlace(Leg.To, Leg.ToKm));
	}
	else
	{
		Where = FString::Printf(TEXT("I %s"), *Map->DescribePlace(First.Town, First.Km));
	}
	Line(TEXT("Hvor"), Where);
	{
		int32 PresentSel = 0, MenSel = 0;
		for (const FCampaign1851Regiment* R : Sel) { PresentSel += R->PresentMen(); MenSel += R->Men; }
		Line(TEXT("Til stede"), FString::Printf(TEXT("%s af %s mand  ·  %s"), *Thousands(PresentSel), *Thousands(MenSel), Campaign1851Mobilisation::FootingName(Map->GetFooting())));
	}
	Line(TEXT("Marchfart"), FString::Printf(TEXT("%.0f km/dag på landevej, %.0f på chaussé"), Pace, Pace * Campaign1851Network::MarchKmPerDayChaussee / Campaign1851Network::MarchKmPerDayRoad));
	int32 TrainsNeeded = 0;
	for (const FCampaign1851Regiment* R : Sel)
	{
		TrainsNeeded += Campaign1851Army::TrainsNeeded(*R);
	}
	Line(TEXT("Med tog"), FString::Printf(TEXT("fylder %d tog  ·  ledige %d af %d"), TrainsNeeded, Map->FreeTroopTrains(), Map->GetTroopTrains()));
	{
		// Supply of the first unit (the least supplied, if several).
		const FCampaign1851Regiment* Low = Sel[0];
		for (const FCampaign1851Regiment* R : Sel)
		{
			Low = R->Food < Low->Food ? R : Low;
		}
		const int32 Depot = Map->DepotFor(Low->Km);
		const int32 LowIndex = int32(Low - Map->GetRegiments().GetData());
		const bool bColumn = Map->GetSupplyColumns().ContainsByPredicate([LowIndex](const FCampaign1851SupplyColumn& C) { return !C.bFort && C.Target == LowIndex && C.State == ESupplyColumnState::Outbound; });
		Line(TEXT("Forsyning"), Campaign1851Supply::Describe(*Low) + (bColumn ? FString(TEXT("  ·  trænkolonne på vej")) : FString()) + (Low->IsMarching() || Low->IsInField()
			? (Depot != INDEX_NONE ? FString::Printf(TEXT("  ·  depot %s"), *Map->GetCities()[Depot].Name) : FString(TEXT("  ·  intet depot i nærheden"))) : FString(TEXT("  ·  garnison"))));
	}
	if (!Why.IsEmpty())
	{
		PaintTextFit(Geometry, Out, Layer + 2, Why, FVector2D(Pos.X + 22.f, Y), Serif(11, EFace::Italic), MutedInk, Inner);
		Y += 20.f;
	}
	if (First.IsMarching())
	{
		const FDateTime Arrive = Map->GetDate() + FTimespan::FromDays(First.DaysLeft());
		Line(TEXT("Mål"), FString::Printf(TEXT("%s  ·  ankomst %s %s"), *Map->DescribePlace(First.Destination(), First.DestinationKm()), *ACampaign1851Map::FormatClock(Arrive), *ACampaign1851Map::FormatDate(Arrive, true)));
	}

	// ---- Officers: click the name for the full card; the button changes him.
	Y += 12.f;
	DrawLines(Geometry, Out, Layer + 2, { FVector2D(Pos.X + 18.f, Y - 8.f), FVector2D(Pos.X + Size.X - 18.f, Y - 8.f) }, Gold.CopyWithNewOpacity(0.3f), 1.f);
	const FVector2D Small(104.f, 22.f);
	auto OfficerBlock = [&](const TCHAR* Label, const FCampaign1851Officer* O, int32 Index, EButton Change, const TCHAR* ChangeText, const TCHAR* Empty)
	{
		PaintText(Geometry, Out, Layer + 2, Label, FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
		const float NameX = Pos.X + 90.f, NameW = Size.X - 90.f - Small.X - 30.f;
		if (O)
		{
			// The name is a (quiet) button: an underline shows it can be clicked.
			const FString Text = FString::Printf(TEXT("%s %s  %s"), *O->Rank, *O->Name, *FString::ChrN(Campaign1851Army::Stars(O->Experience), TEXT('*')));
			PaintTextFit(Geometry, Out, Layer + 2, Text, FVector2D(NameX, Y), Serif(13), Ink, NameW);
			const float Wd = FMath::Min(Measure(Text, Serif(13)).X, NameW);
			DrawLines(Geometry, Out, Layer + 2, { FVector2D(NameX, Y + 9.f), FVector2D(NameX + Wd, Y + 9.f) }, Ink.CopyWithNewOpacity(0.35f), 1.f);
			Buttons.Add({ FVector2D(NameX, Y - 11.f), FVector2D(NameX + Wd, Y + 11.f), EButton::OfficerInfo, Index });
		}
		else
		{
			PaintTextFit(Geometry, Out, Layer + 2, Empty, FVector2D(NameX, Y), Serif(13), MutedInk, NameW);
		}
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - Small.X - 18.f, Y - 11.f), Small, ChangeText, Change);
		Y += 22.f;
		if (O)
		{
			PaintTextFit(Geometry, Out, Layer + 2, OfficerStatLine(*O), FVector2D(NameX, Y), Serif(10), MutedInk, Size.X - 90.f - 22.f);
		}
		Y += 22.f;
	};
	if (bSingle)
	{
		OfficerBlock(TEXT("Chef"), Chief, ChiefIndex, EButton::OfficerChange, TEXT("SKIFT CHEF"), TEXT("ingen chef"));
	}
	OfficerBlock(TEXT("General"), General, GeneralIndex, EButton::GeneralChange, General ? TEXT("SKIFT") : TEXT("UDNÆVN"), TEXT("ingen general over stakken"));

	// ---- A stack: its units as a table (click a row to pick that unit alone; shift-click drops it).
	if (!bSingle)
	{
		Y += 8.f;
		struct FCol { const TCHAR* Head; float X; float Align; };
		const FCol Cols[] = { {TEXT("Enhed"), 0.f, 0.f}, {TEXT("Mand"), 178.f, 1.f}, {TEXT("Erf"), 212.f, 1.f},
			{TEXT("Lad"), 244.f, 1.f}, {TEXT("Skyd"), 276.f, 1.f}, {TEXT("Eks"), 308.f, 1.f}, {TEXT("Felt"), 340.f, 1.f}, {TEXT("Udh"), 372.f, 1.f}, {TEXT("Baj"), 404.f, 1.f},
			{TEXT("Moral"), 444.f, 1.f} };
		const float X0 = Pos.X + 26.f;
		for (const FCol& C : Cols)
		{
			PaintText(Geometry, Out, Layer + 3, C.Head, FVector2D(X0 + C.X, Y), Serif(10, EFace::Italic), Gold, C.Align, false);
		}
		Y += 18.f;
		for (int32 r = 0; r < Rows; ++r)
		{
			const FCampaign1851Regiment& R = *Sel[r];
			PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 18.f, Y - 10.f), FVector2D(Size.X - 36.f, RowHeight - 2.f), FString(), EButton::RegimentRow, SelectedRegiments[r]);
			PaintTextFit(Geometry, Out, Layer + 4, R.Name, FVector2D(X0, Y), Serif(11), Ink, 150.f);
			const float Values[] = { float(R.Men), R.Experience, R.Skills[0], R.Skills[1], R.Skills[2], R.Skills[3], R.Skills[4], R.Skills[5] };
			for (int32 c = 0; c < UE_ARRAY_COUNT(Values); ++c)
			{
				PaintText(Geometry, Out, Layer + 4, FString::Printf(TEXT("%.0f"), Values[c]), FVector2D(X0 + Cols[c + 1].X, Y), Serif(11), Ink, 1.f, false);
			}
			PaintText(Geometry, Out, Layer + 4, FString::Printf(TEXT("%.0f %%"), R.Morale * 100.f), FVector2D(X0 + Cols[9].X, Y), Serif(11), Ink, 1.f, false);
			Y += RowHeight;
		}
		if (bMore)
		{
			PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("+ %d enheder mere"), Sel.Num() - Rows), FVector2D(Pos.X + Size.X * 0.5f, Y), Serif(11, EFace::Italic), MutedInk, 0.5f, false);
			Y += RowHeight;
		}
	}

	const FVector2D ButtonSize(150.f, 28.f);
	const float ButtonY = Pos.Y + Size.Y - 70.f;
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, ButtonY), ButtonSize, TEXT("TIL GARNISON"), EButton::ArmyHome);
	if (!First.IsMarching())
	{
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - 22.f - ButtonSize.X, ButtonY), ButtonSize, TEXT("KAMPORDEN"), EButton::OpenOOB, 0, bOOB);
	}
	if (First.IsInField())
	{
		const int32 FirstIndex = SelectedRegiments.IsValidIndex(0) ? SelectedRegiments[0] : INDEX_NONE;
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 32.f + ButtonSize.X, ButtonY), ButtonSize, TEXT("SEND FORSYNING"), EButton::SupplySend, FirstIndex, false, Map->FreeSupplyColumns() <= 0);
	}
	if (First.IsMarching())
	{
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 32.f + ButtonSize.X, ButtonY), ButtonSize, TEXT("STOP"), EButton::ArmyHalt);
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 42.f + 2.f * ButtonSize.X, ButtonY), ButtonSize, TEXT("SLET ORDRE"), EButton::ArmyCancel);
	}
	PaintTextFit(Geometry, Out, Layer + 2, TEXT("Højreklik på en by eller i terrænet: march dertil  ·  Shift-klik: vælg flere  ·  Esc: fravælg"), FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + Size.Y - 20.f),
		Serif(10, EFace::Italic), MutedInk, Inner, 0.5f);

}

const FSlateBrush* SCampaign1851Overlay::PortraitFor(const FString& Name, int32 Kind, int32 Age) const
{
	// Officers by age (a general is older than a captain), the same face for the whole career whatever the rank: the
	// age group's pool when it has pictures, else the old pools.
	if (Kind != 2 && Age >= 0)
	{
		const int32 Group = Age < 38 ? 0 : Age < 50 ? 1 : 2;
		const TArray<TSharedPtr<FSlateBrush>>& Aged = AgePortraits[Group];
		TArray<const FSlateBrush*> Have;
		for (const TSharedPtr<FSlateBrush>& B : Aged)
		{
			if (B.IsValid() && B->GetResourceObject()) { Have.Add(B.Get()); }
		}
		if (Have.Num() > 0)
		{
			return Have[GetTypeHash(Name) % uint32(Have.Num())];
		}
	}
	const TArray<TSharedPtr<FSlateBrush>>& Pool = Kind == 2 ? MinisterPortraits : Kind == 1 ? GeneralPortraits : OfficerPortraits;
	if (Pool.Num() == 0)
	{
		return nullptr;
	}
	// The person's own picture; if that file is missing, the next one that exists (a pool of fewer than twelve works).
	const int32 Start = int32(GetTypeHash(Name) % uint32(Pool.Num()));
	for (int32 k = 0; k < Pool.Num(); ++k)
	{
		const TSharedPtr<FSlateBrush>& B = Pool[(Start + k) % Pool.Num()];
		if (B.IsValid() && B->GetResourceObject())
		{
			return B.Get();
		}
	}
	return nullptr;
}

void SCampaign1851Overlay::PaintPortrait(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size, const FString& Name, int32 Kind) const
{
	if (const FSlateBrush* B = PortraitFor(Name, Kind))
	{
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), B);
		DrawLines(Geometry, Out, Layer + 1, { Pos, Pos + FVector2D(Size.X, 0.f), Pos + Size, Pos + FVector2D(0.f, Size.Y), Pos }, Gold, 1.f);
	}
}

void SCampaign1851Overlay::PaintPortraitBox(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size, const FString& Name, int32 Kind, int32 Age, int32 Rank) const
{
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	// The mount, then the picture inside it, then the double gold frame.
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, FLinearColor(0.035f, 0.028f, 0.02f, 1.f));
	const float Pad = 8.f;
	if (const FSlateBrush* B = PortraitFor(Name, Kind, Age))
	{
		FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(Size - FVector2D(2.f * Pad, 2.f * Pad), FSlateLayoutTransform(Pos + FVector2D(Pad, Pad))), B);
	}
	else
	{
		PaintText(Geometry, Out, Layer + 1, TEXT("(portræt)"), Pos + Size * 0.5f, Serif(11, EFace::Italic), MutedInk, 0.5f, false);
	}
	auto Frame = [&](float Inset, const FLinearColor& Colour, float Thickness)
	{
		const FVector2D A = Pos + FVector2D(Inset, Inset), B = Pos + Size - FVector2D(Inset, Inset);
		DrawLines(Geometry, Out, Layer + 2, { A, FVector2D(B.X, A.Y), B, FVector2D(A.X, B.Y), A }, Colour, Thickness);
	};
	Frame(0.f, Gold, 2.f);
	Frame(Pad - 2.f, Gold.CopyWithNewOpacity(0.5f), 1.f);
	if (Rank >= 0)
	{
		PaintRankBadge(Geometry, Out, Layer + 3, Pos + Size - FVector2D(Pad + 2.f, Pad + 2.f), FMath::Clamp(Size.X / 178.f, 0.35f, 1.2f), Rank);
	}
}

void SCampaign1851Overlay::PaintRankBadge(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Corner, float Scale, int32 Rank) const
{
	// An epaulette in the lower right corner: silver for the captain to the colonel, gold for the generals; one to three stars,
	// a fringe for the colonel and above.  Kaptajn 0, Major 1, Oberstløjtnant 2, Oberst 3, Generalmajor 4, Generalløjtnant 5, General 6.
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const bool bGeneral = Rank >= 4;
	const FLinearColor Metal = bGeneral ? FLinearColor::FromSRGBColor(FColor(214, 176, 78)) : FLinearColor::FromSRGBColor(FColor(206, 208, 214));
	const FLinearColor Dark = FLinearColor::FromSRGBColor(FColor(30, 36, 62));
	const float W = 62.f * Scale, H = 22.f * Scale;
	const FVector2D Min = Corner - FVector2D(W, H);
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2D(W + 4.f * Scale, H + 4.f * Scale), FSlateLayoutTransform(Min - FVector2D(2.f * Scale, 2.f * Scale))), White, ESlateDrawEffect::None, FLinearColor(0.02f, 0.02f, 0.03f, 0.9f));
	FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(FVector2D(W, H), FSlateLayoutTransform(Min)), White, ESlateDrawEffect::None, Dark);
	FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(W, 3.f * Scale), FSlateLayoutTransform(Min)), White, ESlateDrawEffect::None, Metal);
	FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(W, 3.f * Scale), FSlateLayoutTransform(Min + FVector2D(0.f, H - 3.f * Scale))), White, ESlateDrawEffect::None, Metal);
	// The stars: captain one, major two, lieutenant colonel three, colonel three with fringe; generals one to three.
	static const int32 Stars[] = { 1, 2, 3, 3, 1, 2, 3 };
	const int32 N = Stars[FMath::Clamp(Rank, 0, 6)];
	const float Step = 14.f * Scale;
	const float X0 = Min.X + (W - (N - 1) * Step) * 0.5f;
	for (int32 i = 0; i < N; ++i)
	{
		const FVector2D C(X0 + i * Step, Min.Y + H * 0.5f);
		const float R = 5.f * Scale;
		TArray<FVector2D> Pts;
		for (int32 p = 0; p <= 10; ++p)
		{
			const float Ang = -PI * 0.5f + p * PI / 5.f, Rad = (p % 2 == 0) ? R : R * 0.42f;
			Pts.Add(C + FVector2D(FMath::Cos(Ang) * Rad, FMath::Sin(Ang) * Rad));
		}
		DrawLines(Geometry, Out, Layer + 3, Pts, Metal, FMath::Max(1.f, 1.6f * Scale));
	}
	if (Rank >= 3)
	{
		for (float X = Min.X + 2.f * Scale; X < Min.X + W; X += 5.f * Scale)
		{
			FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(1.5f * Scale, 5.f * Scale), FSlateLayoutTransform(FVector2D(X, Min.Y + H))), White, ESlateDrawEffect::None, Metal);
		}
	}
}

void SCampaign1851Overlay::PaintMinisterCard(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size, int32 Portfolio) const
{
	const ECampaign1851Portfolio P = ECampaign1851Portfolio(Portfolio);
	const FCampaign1851Minister& M = Map->GetMinister(P);
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	Buttons.Add({ Pos, Pos + Size, EButton::Block, 0 });
	// The box of the card: the same mount as the portrait's, on the window's panel.
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, FLinearColor(0.045f, 0.035f, 0.025f, 1.f));
	DrawLines(Geometry, Out, Layer + 1, { Pos, FVector2D(Pos.X + Size.X, Pos.Y), Pos + Size, FVector2D(Pos.X, Pos.Y + Size.Y), Pos }, Gold.CopyWithNewOpacity(0.85f), 1.5f);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(Size.X - 40.f, 10.f), FVector2D(28.f, 24.f), TEXT("X"), EButton::MinisterInfo, Portfolio);
	PaintText(Geometry, Out, Layer + 2, FString(Campaign1851Nations::PortfolioName(P)).ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å")),
		Pos + FVector2D(22.f, 26.f), Serif(11), Gold, 0.f, false);
	const FVector2D Picture(Pos.X + 22.f, Pos.Y + 48.f);
	PaintPortraitBox(Geometry, Out, Layer + 2, Picture, FVector2D(190.f, 238.f), M.Name, 2);
	const float TX = Picture.X + 212.f;
	float Y = Picture.Y + 4.f;
	PaintTextFit(Geometry, Out, Layer + 2, M.Name, FVector2D(TX, Y), Serif(22), Ink, Size.X - (TX - Pos.X) - 24.f);
	Y += 32.f;
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s af %s"), Campaign1851Nations::PortfolioName(P), Map->GetNations().IsValidIndex(Map->GetPlayerNation()) ? *Map->GetNations()[Map->GetPlayerNation()].Name : TEXT("")),
		FVector2D(TX, Y), Serif(13, EFace::Italic), Gold, Size.X - (TX - Pos.X) - 24.f);
	Y += 24.f;
	const FDateTime Appointed = ACampaign1851Map::StartDate() + FTimespan::FromDays(M.Since);
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Minister siden %s"), *ACampaign1851Map::FormatDate(Appointed, true)), FVector2D(TX, Y), Serif(12), Ink, Size.X - (TX - Pos.X) - 24.f);
	Y += 22.f;
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Strømning: %s"), Campaign1851Politics::CurrentName(M.Line)), FVector2D(TX, Y), Serif(12), M.Line == Map->GetGovernment() ? Gold : Ink, Size.X - (TX - Pos.X) - 24.f);
	Y += 20.f;
	PaintTextFit(Geometry, Out, Layer + 2, M.Line == Map->GetGovernment() ? TEXT("regeringens egen linje: sidder sikkert") : TEXT("ikke regeringens linje: skiftes ved næste regeringsskifte"),
		FVector2D(TX, Y), Serif(10, EFace::Italic), MutedInk, Size.X - (TX - Pos.X) - 24.f);
	Y += 30.f;
	struct FQuality { const TCHAR* Name; uint8 Value; const TCHAR* Meaning; };
	const FQuality Qualities[] = {
		{ TEXT("Dygtig"), M.Skill, TEXT("hvor godt ressortet vælger, og hvor langt pengene rækker") },
		{ TEXT("Sparsom"), M.Thrift, TEXT("hvor meget ressortet holder tilbage") },
		{ TEXT("Forsigtig"), M.Caution, TEXT("hvor dristige udenrigs-, flåde- og finansskridt er") } };
	for (const FQuality& Q : Qualities)
	{
		PaintText(Geometry, Out, Layer + 2, Q.Name, FVector2D(TX, Y), Serif(12), Ink, 0.f, false);
		PaintBar(Geometry, Out, Layer + 2, FVector2D(TX + 90.f, Y - 3.f), 150.f, Q.Value / 10.f);
		PaintText(Geometry, Out, Layer + 2, FString::FromInt(Q.Value), FVector2D(TX + 252.f, Y), Serif(12), Ink, 0.f, false);
		AddTip(FVector2D(TX - 4.f, Y - 12.f), FVector2D(Size.X - (TX - Pos.X) - 16.f, 22.f), FString::Printf(TEXT("%s (1-10): %s"), Q.Name, Q.Meaning));
		Y += 24.f;
	}
	// What the ministry is doing: its mode, its money, its latest decisions.
	float RY = Picture.Y + 258.f;
	const FCampaign1851Nation* Me = Map->GetNations().IsValidIndex(Map->GetPlayerNation()) ? &Map->GetNations()[Map->GetPlayerNation()] : nullptr;
	DrawLines(Geometry, Out, Layer + 2, { FVector2D(Pos.X + 22.f, RY - 8.f), FVector2D(Pos.X + Size.X - 22.f, RY - 8.f) }, Gold.CopyWithNewOpacity(0.35f), 1.f);
	if (Me)
	{
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("Arbejdsform: %s  ·  må bruge %s rd. om måneden  ·  pulje %s rd."), Campaign1851Nations::DelegationName(Me->Modes[Portfolio]),
			*Thousands(int32(Map->GetMinistryBudget(P))), *Thousands(int32(Map->GetMinistryPot(P)))), FVector2D(Pos.X + 22.f, RY), Serif(12), Ink, 0.f, false);
	}
	RY += 28.f;
	PaintText(Geometry, Out, Layer + 2, TEXT("S E N E S T E   B E S L U T N I N G E R"), FVector2D(Pos.X + 22.f, RY), Serif(11), Gold, 0.f, false);
	RY += 24.f;
	const TArray<FCampaign1851Decision>& Decisions = Map->GetDecisions();
	int32 Shown = 0;
	for (int32 i = Decisions.Num() - 1; i >= 0 && Shown < 6 && RY < Pos.Y + Size.Y - 50.f; --i)
	{
		const FCampaign1851Decision& D = Decisions[i];
		if (D.Portfolio != P || D.Nation != Map->GetPlayerNation())
		{
			continue;
		}
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s  ·  %s"), *ACampaign1851Map::FormatDate(ACampaign1851Map::StartDate() + FTimespan::FromDays(D.Day), true), *D.Action),
			FVector2D(Pos.X + 22.f, RY), Serif(12), D.bAdvice && !D.bDone ? Gold : Ink, Size.X - 44.f);
		PaintTextFit(Geometry, Out, Layer + 2, D.Reasons, FVector2D(Pos.X + 22.f, RY + 15.f), Serif(10, EFace::Italic), MutedInk, Size.X - 44.f);
		RY += 38.f;
		++Shown;
	}
	if (Shown == 0)
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("Ingen endnu: ministeren træffer beslutninger ved månedsskiftet, når ressortet er RÅDGIVER eller AUTO."), FVector2D(Pos.X + 22.f, RY), Serif(11, EFace::Italic), MutedInk, 0.f, false);
	}
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - 190.f, Pos.Y + Size.Y - 40.f), FVector2D(168.f, 28.f), TEXT("UDSKIFT MINISTER"), EButton::MinisterDismiss, Portfolio);
}

void SCampaign1851Overlay::PaintUnitCard(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft, int32 RegimentIndex) const
{
	const FCampaign1851Regiment& R = Map->GetRegiments()[RegimentIndex];
	const TArray<FCampaign1851Officer>& Officers = Map->GetOfficers();
	const int32 Parts = Map->SubUnitCount(RegimentIndex);
	const int32 Co = Parts > 1 && UnitCardCompany >= 0 && UnitCardCompany < Parts ? UnitCardCompany : INDEX_NONE;
	const float ChipsH = Parts > 1 ? 26.f : 0.f;
	const FVector2D Size(470.f, 640.f + ChipsH);
	const FVector2D Pos(BottomLeft.X, FMath::Max(130.f, BottomLeft.Y - Size.Y));
	// In the front: an opaque box over everything under it, which also takes the clicks.
	Buttons.Add({ Pos, Pos + Size, EButton::Block, 0 });
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(0.045f, 0.035f, 0.025f, 1.f));
	PaintPanel(Geometry, Out, Layer + 1, Pos, Size);
	PaintText(Geometry, Out, Layer + 2, TEXT("E N H E D S K O R T"), Pos + FVector2D(22.f, 26.f), Serif(11), Gold, 0.f, false);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(Size.X - 46.f, 12.f), FVector2D(28.f, 24.f), TEXT("X"), EButton::UnitCard, 0);
	const bool bCompanies = R.Captains.Num() > 0;
	PaintTextFit(Geometry, Out, Layer + 2, Co != INDEX_NONE ? FString::Printf(TEXT("%d. %s"), bCompanies ? Map->CompanyNumber(RegimentIndex, Co) : Co + 1, bCompanies ? TEXT("Kompagni") : R.Arm == ECampaign1851Arm::Artillery ? TEXT("Sektion") : TEXT("Eskadron")) : R.Name, Pos + FVector2D(22.f, 54.f), Serif(20), Ink, Size.X - 44.f);
	PaintTextFit(Geometry, Out, Layer + 2, Co != INDEX_NONE ? FString::Printf(TEXT("%s  ·  %s"), *R.Name, Campaign1851Army::ArmName(R.Arm))
		: FString::Printf(TEXT("%s  ·  %s%s"), Campaign1851Army::ArmName(R.Arm), Campaign1851Army::ExperienceName(R.Experience), Parts > 1 ? TEXT("  ·  gennemsnit for enheden") : TEXT("")),
		Pos + FVector2D(22.f, 78.f), Serif(12, EFace::Italic), Gold, Size.X - 44.f);
	// The unit's companies (squadrons): the card of one of them, or the average of the unit.
	if (Parts > 1)
	{
		PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(22.f, 92.f), FVector2D(52.f, 20.f), TEXT("ALLE"), EButton::UnitCardPart, 0, Co == INDEX_NONE);
		for (int32 k = 0; k < Parts && k < 10; ++k)
		{
			PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(80.f + k * 30.f, 92.f), FVector2D(26.f, 20.f), *FString::FromInt(bCompanies ? Map->CompanyNumber(RegimentIndex, k) : k + 1), EButton::UnitCardPart, k + 1, Co == k);
		}
	}
	// The soldier in his uniform, and the colours.
	const bool bHussar = R.Arm == ECampaign1851Arm::Cavalry && R.Name.Contains(TEXT("usar"));
	const int32 Arm = bHussar ? 6 : FMath::Clamp(int32(R.Arm), 0, 5);
	const FVector2D Picture(Pos.X + 22.f, Pos.Y + 96.f + ChipsH);
	if (Map->ActiveScenario().Id == TEXT("1825"))
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("UNIFORM 1825"), Picture + FVector2D(75.f, 65.f), Serif(12), Gold, 0.5f, false);
		PaintTextFit(Geometry, Out, Layer + 2, Map->ArmyUniformText(R.Arm), Picture + FVector2D(0.f, 100.f), Serif(10), Ink, 150.f);
		PaintTextFit(Geometry, Out, Layer + 2, Map->ArmyWeaponText(R.Arm), Picture + FVector2D(0.f, 135.f), Serif(10), Ink, 150.f);
		AddTip(Picture, FVector2D(150.f, 225.f), Map->ArmyUniformText(R.Arm) + TEXT(". ") + Map->ArmyWeaponText(R.Arm));
	}
	else if (UniformBrushes.IsValidIndex(Arm) && UniformBrushes[Arm]->GetResourceObject())
	{
		FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(150.f, 225.f), FSlateLayoutTransform(Picture)), UniformBrushes[Arm].Get());
	}
	else
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("(uniform kommer)"), Picture + FVector2D(75.f, 115.f), Serif(11, EFace::Italic), MutedInk, 0.5f, false);
	}
	if (FlagBrush.IsValid() && FlagBrush->GetResourceObject())
	{
		FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(105.f, 84.f), FSlateLayoutTransform(Picture + FVector2D(170.f, 0.f))), FlagBrush.Get());   // the flag stretched a quarter wider
		PaintText(Geometry, Out, Layer + 2, TEXT("Fanen"), Picture + FVector2D(222.f, 94.f), Serif(10, EFace::Italic), MutedInk, 0.5f, false);
	}
	// The figures beside the picture.
	float Y = Picture.Y + 114.f;
	const float LX = Picture.X + 170.f;
	auto Fact = [&](const FString& Label, const FString& Value, const FString& Tip)
	{
		PaintText(Geometry, Out, Layer + 2, Label, FVector2D(LX, Y), Serif(11, EFace::Italic), Gold, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 2, Value, FVector2D(LX + 100.f, Y), Serif(12), Ink, Size.X - (LX - Pos.X) - 122.f);
		if (!Tip.IsEmpty()) { AddTip(FVector2D(LX - 4.f, Y - 10.f), FVector2D(Size.X - (LX - Pos.X) - 18.f, 20.f), Tip); }
		Y += 20.f;
	};
	if (Co != INDEX_NONE)
	{
		Fact(TEXT("Mand"), FString::Printf(TEXT("%d af %d"), Map->SubUnitMen(RegimentIndex, Co), Map->CompanyCapacity(RegimentIndex)), TEXT("Kompagniets mand og dets fulde styrke (flyt mænd mellem kompagnier i kamporden)"));
	}
	else
	{
		Fact(TEXT("Mand"), FString::Printf(TEXT("%d af %d"), R.Men, R.MaxMen), TEXT("Mand i rullerne og etablissementets fulde styrke"));
	}
	Fact(TEXT("Erfaring"), FString::Printf(TEXT("%.0f"), R.Experience), TEXT("Erfaring 0-100: felttjeneste og slag"));
	Fact(TEXT("Moral"), FString::Printf(TEXT("%.0f %%"), R.Morale * 100.f), TEXT("Kampviljen: chefens inspiration, træningen, sejre og nederlag"));
	Fact(TEXT("Samhørighed"), FString::Printf(TEXT("%.0f"), R.Cohesion), TEXT("Hvor godt enheden hænger sammen (falder på lange marcher og i slag)"));
	Fact(TEXT("Øvelser"), Campaign1851Army::ProgramName(R.Program), TEXT("Garnisonens træningsprogram. Eksercits og skydeøvelser indøver også de udforskede ildmetoder"));
	// A foot battery can be made a horse battery.
	if (R.Arm == ECampaign1851Arm::Artillery && R.Mortars == 0 && R.Guns > 0)
	{
		FString Why;
		const bool bCan = Map->CanUpgradeToHorseBattery(RegimentIndex, &Why);
		PaintButton(Geometry, Out, Layer + 2, FVector2D(LX, Y + 4.f), FVector2D(Size.X - (LX - Pos.X) - 22.f, 26.f), TEXT("GØR RIDENDE"), EButton::HorseBattery, RegimentIndex, false, !bCan);
		AddTip(FVector2D(LX, Y + 4.f), FVector2D(Size.X - (LX - Pos.X) - 22.f, 26.f), FString::Printf(
			TEXT("Gør batteriet ridende: alle kanonerer til hest, følger rytteriet. 6 kanoner (2 tilbage på lager), 180 mand, %d heste (fra lageret), %s rd. Eksercitsen falder en tid.%s"),
			ACampaign1851Map::HorseBatteryHorses, *Thousands(int32(ACampaign1851Map::HorseBatteryCost)), bCan ? TEXT("") : *FString::Printf(TEXT("  %s"), *Why)));
		Y += 32.f;
	}
	// The sick and wounded: in the lazaret, back to their own unit (keeping its experience) within weeks.
	Y = Picture.Y + 246.f;
	const float X = Pos.X + 22.f;
	auto Line = [&](const FString& Label, const FString& Value, const FString& Tip)
	{
		PaintText(Geometry, Out, Layer + 2, Label, FVector2D(X, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 2, Value, FVector2D(X + 130.f, Y), Serif(12), Ink, Size.X - 174.f);
		if (!Tip.IsEmpty()) { AddTip(FVector2D(X - 4.f, Y - 10.f), FVector2D(Size.X - 36.f, 20.f), Tip); }
		Y += 22.f;
	};
	if (Co == INDEX_NONE)
	{
		const ACampaign1851ConstructionSite* Lazaret = Map->FindBuilding(R.Home, TEXT("Field_Hospital"));
		const bool bCare = Map->HasResearch(TEXT("sanitation")) || (Lazaret && !Lazaret->IsDemolishing() && Lazaret->IsModuleDone(0));
		const float Rate = (bCare ? 0.05f : 0.03f) * (Map->HasResearch(TEXT("hospitals")) ? 1.4f : 1.f);
		const int32 Half = FMath::RoundToInt(FMath::Loge(2.f) / Rate);
		Line(TEXT("Sårede og syge"), R.Sick > 0 ? FString::Printf(TEXT("%d på lazaret  ·  halvdelen tilbage om ca. %d dage%s"), R.Sick, Half, bCare ? TEXT(" (lazaret)") : TEXT(""))
			: FString(TEXT("ingen")), TEXT("Syge er ikke med i mandskabstallet. Dagligt vender 5 % tilbage med færdigt lazaret i hjemgarnisonen eller sanitetsvæsen, ellers 3 %; henholdsvis 0,2 % og 0,4 % dør. Militærhospitaler øger tilbagekomsten med 40 %. Mænd ud over etaten hjemsendes."));
	}
	// The chief: the battalion's, or the captain of the company shown.
	const int32 ChiefIndex = Co != INDEX_NONE && R.Captains.IsValidIndex(Co) ? R.Captains[Co] : (Co != INDEX_NONE ? INDEX_NONE : R.Chief);
	if (Officers.IsValidIndex(ChiefIndex))
	{
		PaintPortraitBox(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - 82.f, Y - 6.f), FVector2D(62.f, 78.f), Officers[ChiefIndex].Name, Officers[ChiefIndex].bGeneral ? 1 : 0,
			Map->GetDate().GetYear() - Officers[ChiefIndex].Born, Campaign1851Army::RankIndex(Officers[ChiefIndex].Rank));
	}
	Line(Co != INDEX_NONE ? TEXT("Kaptajn") : TEXT("Chef"), Officers.IsValidIndex(ChiefIndex) ? FString::Printf(TEXT("%s %s (%d)"), *Officers[ChiefIndex].Rank, *Officers[ChiefIndex].Name, Campaign1851Army::OfficerRating(Officers[ChiefIndex]))
		: FString(TEXT("ingen")), TEXT("Chefens samlede vurdering 0-100 (åbn officerens kort for evnerne)"));
	if (Co != INDEX_NONE)
	{
		PaintTextFit(Geometry, Out, Layer + 2, TEXT("Erfaring, moral, øvelser og tjeneste er hele enhedens."), FVector2D(X, Y - 2.f), Serif(10, EFace::Italic), MutedInk, Size.X - 44.f);
		Y += 18.f;
	}
	{
		static const TCHAR* Topics[4] = { TEXT("tworank"), TEXT("firebyrank"), TEXT("volley"), TEXT("independent") };
		static const TCHAR* Names[4] = { TEXT("2 gld"), TEXT("geled"), TEXT("salve"), TEXT("fri") };
		FString Drills;
		for (int32 d = 0; d < 4; ++d)
		{
			if (Map->HasResearch(Topics[d]))
			{
				Drills += FString::Printf(TEXT("%s%s %.0f%s"), Drills.IsEmpty() ? TEXT("") : TEXT("  ·  "), Names[d], R.FireDrills[d], R.FireDrills[d] >= 60.f ? TEXT(" ✓") : TEXT(""));
			}
		}
		Line(TEXT("Ildmetoder"), Drills.IsEmpty() ? FString(TEXT("forreste geled")) : Drills,
			TEXT("Udforskede ildmetoder indøves i garnison (eksercits, skydeøvelser eller blandet) og kan bruges i slaget fra 60"));
	}
	// The service record.
	Y += 8.f;
	PaintText(Geometry, Out, Layer + 2, TEXT("T J E N E S T E"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 22.f;
	Line(TEXT("Slag"), FString::Printf(TEXT("%d  ·  faldne %d  ·  sårede %d  ·  fanget %d"), R.Service.Num(), R.TotalKilled, R.TotalWounded, R.TotalCaptured), FString());
	Line(TEXT("Fjender ude"), FString::Printf(TEXT("%d (dræbte og sårede)"), R.TotalEnemyKilled), TEXT("Fjendtlige soldater enheden har sat ud af kampen: talt i 3D-slagene, fordelt efter styrke i de automatisk afgjorte"));
	static const TCHAR* Results[4] = { TEXT("nederlag"), TEXT("uafgjort"), TEXT("sejr"), TEXT("tilbagetog") };
	const int32 First = FMath::Max(0, R.Service.Num() - 8);
	for (int32 e = R.Service.Num() - 1; e >= First; --e)
	{
		const FCampaign1851ServiceEntry& E = R.Service[e];
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s  ·  %s  ·  %s%s  ·  faldne %d, sårede %d, fjender %d"),
			*ACampaign1851Map::FormatDate(ACampaign1851Map::StartDate() + FTimespan::FromDays(E.Day), true), *E.Place, Results[FMath::Min<int32>(E.Result, 3)],
			E.bFrom3D ? TEXT(" (3D)") : TEXT(""), E.Killed, E.Wounded, E.EnemyKilled), FVector2D(X, Y), Serif(11), Ink, Size.X - 44.f);
		Y += 19.f;
	}
	if (R.Service.Num() == 0)
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("Ingen slag endnu i dette felttog"), FVector2D(X, Y), Serif(11, EFace::Italic), MutedInk, 0.f, false);
	}
}

void SCampaign1851Overlay::PaintOrderDialog(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const FOrderDialog& D = OrderDialog;
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const int32 Rows = FMath::Min(D.Units.Num(), 12);
	const float RowH = 28.f;
	const FVector2D Size(880.f, 150.f + Rows * RowH + D.Columns.Num() * 24.f + 84.f);
	const FVector2D Screen = Geometry.GetLocalSize();
	const FVector2D Pos(Screen.X - Size.X - 28.f, FMath::Max(130.f, Screen.Y - 190.f - Size.Y));
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseOrder);
	PaintText(Geometry, Out, Layer + 2, TEXT("M A R C H O R D R E"), Pos + FVector2D(22.f, 26.f), Serif(11), Gold, 0.f, false);
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("til %s"), *D.Goal), Pos + FVector2D(22.f, 52.f), Serif(18), Ink, Size.X - 80.f);
	const TCHAR* Ways[] = { TEXT("TIL FODS"), TEXT("MED TOG"), TEXT("LIGE LINJE"), TEXT("OPDEL") };
	// All at once, with the time each way would take the whole selection.
	float Y = Pos.Y + 92.f;
	PaintText(Geometry, Out, Layer + 2, TEXT("Alle"), FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
	const float WW = 180.f;
	for (int32 w = 0; w < 3; ++w)
	{
		const bool bAll = D.Ways.Num() > 0 && !D.Ways.ContainsByPredicate([w](uint8 X) { return X != w; });
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 150.f + w * (WW + 8.f), Y - 13.f), FVector2D(WW, 26.f), FString::Printf(TEXT("%s  %s"), Ways[w], *D.AllTimes[w]), EButton::OrderAll, w, bAll);
	}
	Y += 38.f;
	// Each unit its own way.
	for (int32 r = 0; r < Rows; ++r)
	{
		const int32 u = D.Units[r];
		PaintTextFit(Geometry, Out, Layer + 2, Regs.IsValidIndex(u) ? Regs[u].Name : FString(), FVector2D(Pos.X + 22.f, Y), Serif(12), Ink, 124.f);
		for (int32 w = 0; w < 4; ++w)
		{
			// The fourth: split off and stay here (halting if on the march).
			const float Width = w == 3 ? 150.f : WW;
			PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 150.f + w * (WW + 8.f), Y - 12.f), FVector2D(Width, 24.f), Ways[w], EButton::OrderUnit, r * 4 + w, D.Ways.IsValidIndex(r) && D.Ways[r] == w);
		}
		Y += RowH;
	}
	Y += 10.f;
	DrawLines(Geometry, Out, Layer + 2, { FVector2D(Pos.X + 18.f, Y - 8.f), FVector2D(Pos.X + Size.X - 18.f, Y - 8.f) }, Gold.CopyWithNewOpacity(0.3f), 1.f);
	for (const FString& C : D.Columns)
	{
		PaintTextFit(Geometry, Out, Layer + 2, C, FVector2D(Pos.X + 22.f, Y + 6.f), Serif(12), Ink, Size.X - 44.f);
		Y += 24.f;
	}
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, Pos.Y + Size.Y - 48.f), FVector2D(160.f, 30.f), TEXT("UDFØR"), EButton::OrderExecute);
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 192.f, Pos.Y + Size.Y - 48.f), FVector2D(160.f, 30.f), TEXT("ANNULLÉR"), EButton::OrderCancel);
	// The help on its own line above the buttons (it ran under them).
	PaintTextFit(Geometry, Out, Layer + 2, TEXT("Hver vej bliver sin egen kolonne  ·  OPDEL: udskiller enheden, den holder stand  ·  Shift+højreklik: straks"), FVector2D(Pos.X + 22.f, Pos.Y + Size.Y - 62.f), Serif(10, EFace::Italic), MutedInk, Size.X - 44.f);
}

bool SCampaign1851Overlay::IsOverChart(const FVector2D& ViewportPixel) const
{
	const FVector2D Local = ViewportPixel / FMath::Max(PaintScale, 0.01f);
	return Window == EWindow::Chart && Local.X >= ChartMin.X && Local.Y >= ChartMin.Y && Local.X <= ChartMax.X && Local.Y <= ChartMax.Y;
}

void SCampaign1851Overlay::PaintFortTool(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851Fort>& Forts = Map->GetForts();
	const int32 Rows = FMath::Min(Forts.Num(), 8);
	const FVector2D Size(540.f, 200.f + Rows * 28.f);
	// Beside the fort's own panel when one is chosen, else in its place at the bottom left.
	const bool bBeside = Map->FortIndex(SelectedFort) != INDEX_NONE || SelectedRegiments.Num() > 0 || Map->GetCities().IsValidIndex(SelectedCity) || SelectedAmt != 0;
	const FVector2D Pos(bBeside ? 28.f + 560.f + 10.f : 28.f, FMath::Max(130.f, Geometry.GetLocalSize().Y - 190.f - Size.Y));
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseFortPanel);
	PaintText(Geometry, Out, Layer + 2, TEXT("S K A N S E R"), Pos + FVector2D(22.f, 26.f), Serif(11), Gold, 0.f, false);
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Vælg type og klik på kortet  ·  kanoner på lager: %d (sparer %s rd. pr. kanon)  ·  trænkolonner %d/%d ledige"),
		Map->GetGunStock(), *Thousands(Campaign1851Forts::GunPrice), Map->FreeSupplyColumns(), Map->GetSupplyColumnCount()),
		Pos + FVector2D(22.f, 50.f), Serif(10, EFace::Italic), MutedInk, Size.X - 44.f - 150.f);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(Size.X - 172.f, 38.f), FVector2D(150.f, 24.f), FString::Printf(TEXT("KØB KOLONNE %s"), *Thousands(Campaign1851Supply::ColumnCost)), EButton::SupplyBuy);
	for (int32 k = 0; k < 2; ++k)
	{
		const bool bLarge = k == 1;
		const float Y = Pos.Y + 70.f + k * 50.f;
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, Y), FVector2D(150.f, 28.f), bLarge ? TEXT("STOR SKANSE") : TEXT("LILLE SKANSE"), EButton::FortChoose, k + 1,
			FortPlacing == k + 1, !Map->CanAfford(Campaign1851Forts::BuildCost(bLarge)));
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s rd.  ·  %.0f dage  ·  %d-%d kanoner  ·  %d mand"), *Thousands(Campaign1851Forts::BuildCost(bLarge)), Campaign1851Forts::BuildDays(bLarge),
			Campaign1851Forts::StartGuns(bLarge), Campaign1851Forts::MaxGuns(bLarge), Campaign1851Forts::InfantryCapacity(bLarge)), FVector2D(Pos.X + 184.f, Y + 9.f), Serif(11), Ink, Size.X - 206.f);
		PaintTextFit(Geometry, Out, Layer + 2, bLarge ? TEXT("lukket skanse som Dybbøls skanse IV (12 kanoner)") : TEXT("lunette: to facer, åben bagtil"),
			FVector2D(Pos.X + 184.f, Y + 26.f), Serif(10, EFace::Italic), MutedInk, Size.X - 206.f);
	}
	if (FortPlacing != 0)
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("Klik på kortet, hvor skansen skal ligge  ·  Esc: fortryd"), Pos + FVector2D(22.f, 180.f), Serif(12, EFace::Italic), Gold, 0.f, false);
	}
	else if (Forts.Num() == 0)
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("Ingen skanser endnu"), Pos + FVector2D(22.f, 180.f), Serif(12, EFace::Italic), MutedInk, 0.f, false);
	}
	for (int32 r = 0; r < Rows; ++r)
	{
		const FCampaign1851Fort& F = Forts[r];
		const float Y = Pos.Y + 196.f + r * 28.f;
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 18.f, Y - 12.f), FVector2D(Size.X - 36.f, 25.f), FString(), EButton::FortSelect, F.Id, SelectedFort == F.Id);
		PaintTextFit(Geometry, Out, Layer + 3, F.Name, FVector2D(Pos.X + 28.f, Y), Serif(12), SelectedFort == F.Id ? FLinearColor::FromSRGBColor(FColor(30, 22, 12)) : Ink, 220.f);
		PaintTextFit(Geometry, Out, Layer + 3, F.bBuilt ? FString::Printf(TEXT("%d/%d kanoner  ·  niveau %d"), F.Guns, Campaign1851Forts::MaxGuns(F.bLarge), F.Defence)
			: FString::Printf(TEXT("under anlæg %.0f %%"), F.Progress() * 100.f), FVector2D(Pos.X + Size.X - 30.f, Y), Serif(10, EFace::Italic), SelectedFort == F.Id ? FLinearColor::FromSRGBColor(FColor(30, 22, 12)) : MutedInk, 200.f, 1.f);
	}
}

void SCampaign1851Overlay::PaintBridge(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const FCampaign1851Bridge& B = Map->GetBridges()[Map->BridgeIndex(SelectedBridge)];
	const FVector2D Size(440.f, 230.f);
	const FVector2D Pos(28.f, Geometry.GetLocalSize().Y - 190.f - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseSelection);
	float Y = Pos.Y + 34.f;
	PaintTextFit(Geometry, Out, Layer + 2, B.Name, FVector2D(Pos.X + 22.f, Y), Serif(22), Ink, Size.X - 80.f);
	Y += 32.f;
	const TCHAR* State = B.State == EBridgeState::Intact ? (B.FerryKm > 0.f ? TEXT("pontonbro: vejen går over sundet") : TEXT("intakt"))
		: B.State == EBridgeState::Blown ? TEXT("SPRÆNGT: vejen er afskåret")
		: B.State == EBridgeState::Building ? TEXT("under bygning")
		: TEXT("færge: her kan lægges en pontonbro");
	PaintText(Geometry, Out, Layer + 2, State, FVector2D(Pos.X + 22.f, Y), Serif(13, EFace::Italic), B.State == EBridgeState::Blown ? FLinearColor(1.f, 0.45f, 0.4f) : Gold, 0.f, false);
	Y += 26.f;
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("Længde ca. %.0f m%s"), B.LengthM, B.State == EBridgeState::Building ? *FString::Printf(TEXT("  ·  færdig om %.0f dage"), B.DaysLeft) : TEXT("")),
		FVector2D(Pos.X + 22.f, Y), Serif(13), Ink, 0.f, false);
	Y += 24.f;
	PaintTextFit(Geometry, Out, Layer + 2, B.FerryKm > 0.f ? TEXT("En bro over sundet lader også fjenden gå over uden både, hvad flåden end gør — medmindre den sprænges.")
		: TEXT("En sprængt bro skærer vejen over for begge sider; marcher og kolonner må finde en anden vej."),
		FVector2D(Pos.X + 22.f, Y), Serif(10, EFace::Italic), MutedInk, Size.X - 44.f);
	const float BY = Pos.Y + Size.Y - 50.f;
	auto Action = [&](float X, EBridgeAction A, const TCHAR* Label)
	{
		const FString Why = Map->BridgeBlockReason(B.Id, A);
		PaintButton(Geometry, Out, Layer + 2, FVector2D(X, BY), FVector2D(190.f, 30.f), Why.IsEmpty() || Why == TEXT("-") ? FString(Label) : Why, EButton::BridgeDo, B.Id * 10 + int32(A), false, !Why.IsEmpty());
	};
	if (B.State == EBridgeState::Intact)
	{
		Action(Pos.X + 22.f, EBridgeAction::Blow, TEXT("SPRÆNG  (500 rd.)"));
	}
	else if (B.State == EBridgeState::Blown)
	{
		Action(Pos.X + 22.f, EBridgeAction::Rebuild, TEXT("GENOPBYG  (4.000 rd.)"));
	}
	else if (B.State == EBridgeState::Site)
	{
		Action(Pos.X + 22.f, EBridgeAction::Build, *FString::Printf(TEXT("LÆG PONTONBRO  (%s)"), *Thousands(int32(Map->PontoonCost()))));
	}
}

void SCampaign1851Overlay::PaintFort(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const int32 Index = Map->FortIndex(SelectedFort);
	if (Index == INDEX_NONE)
	{
		return;
	}
	const FCampaign1851Fort& F = Map->GetForts()[Index];
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const TArray<FCampaign1851Officer>& Officers = Map->GetOfficers();
	const TArray<int32> Candidates = bFortPickCompany ? Map->FortCandidates(F.Id) : TArray<int32>();
	const int32 CompanyRows = F.Companies.Num();
	const int32 PickRows = bFortPickCompany ? FMath::Max(1, FMath::Min(Candidates.Num(), 8)) : 0;
	const FVector2D Size(560.f, 392.f + CompanyRows * 24.f + (bFortPickCompany ? 30.f + PickRows * 24.f : 0.f) + (DemolishArmed == 1000000 + F.Id ? 22.f : 0.f));
	const FVector2D Pos(28.f, FMath::Max(130.f, Geometry.GetLocalSize().Y - 190.f - Size.Y));
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseFort);
	PaintTextFit(Geometry, Out, Layer + 2, F.Name, Pos + FVector2D(22.f, 34.f), Serif(22), Ink, Size.X - 80.f);
	PaintText(Geometry, Out, Layer + 2, F.bLarge ? TEXT("Stor skanse (lukket)") : TEXT("Lille skanse (lunette)"), Pos + FVector2D(22.f, 62.f), Serif(13, EFace::Italic), Gold, 0.f, false);
	float Y = Pos.Y + 92.f;
	auto Line = [&](const TCHAR* Label, const FString& Value)
	{
		PaintText(Geometry, Out, Layer + 2, Label, FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 2, Value, FVector2D(Pos.X + 130.f, Y), Serif(12), Ink, Size.X - 152.f);
		Y += 22.f;
	};
	if (F.Work != ECampaign1851FortWork::None)
	{
		const TCHAR* What = F.Work == ECampaign1851FortWork::Build ? TEXT("Anlægges") : F.Work == ECampaign1851FortWork::Guns ? TEXT("To kanoner mere")
			: F.Work == ECampaign1851FortWork::Trenches ? TEXT("Løbegrave") : F.Work == ECampaign1851FortWork::Demolish ? TEXT("Sløjfes") : Campaign1851Forts::DefenceName(F.Defence + 1);
		Line(TEXT("Arbejde"), FString::Printf(TEXT("%s  ·  %.0f %%  ·  %.0f dage tilbage%s"), What, F.Progress() * 100.f, FMath::Max(0.f, F.WorkDays - F.DaysBuilt), F.bStalled ? TEXT("  ·  ingen penge") : TEXT("")));
	}
	int32 Inside = 0, Reserve = 0;
	Map->FortMen(F, Inside, Reserve);
	Line(TEXT("Kanoner"), FString::Printf(TEXT("%d af %d  ·  %s  ·  %d kanonerer"), F.Guns, Campaign1851Forts::MaxGuns(F.bLarge), Campaign1851Forts::GunType(), F.Guns * Campaign1851Forts::GunnersPerGun));
	Line(TEXT("Forsvar"), FString::Printf(TEXT("%d  ·  %s%s"), F.Defence, Campaign1851Forts::DefenceName(F.Defence), F.bTrenches ? TEXT("  ·  løbegrave") : TEXT("")));
	Line(TEXT("Inde"), FString::Printf(TEXT("%d af %d mand  ·  dækning %d %%  ·  brystværn %.1f m, grav %.1f m"), Inside, Campaign1851Forts::InfantryCapacity(F.bLarge),
		Campaign1851Forts::CoverPercent(F.Defence), Campaign1851Forts::ParapetHeightM(F.bLarge, F.Defence), Campaign1851Forts::DitchDepthM(F.bLarge)));
	Line(TEXT("Reserve"), FString::Printf(TEXT("%d mand bag skansen  ·  dækning %d %%%s"), Reserve, Campaign1851Forts::ReserveCover(F.bTrenches),
		Reserve > 0 ? TEXT("  ·  rykker ind, når der bliver plads") : TEXT("")));
	Line(TEXT("Front mod"), Campaign1851Forts::Compass(F.Yaw));
	Line(TEXT("Magasin"), FString::Printf(TEXT("%.0f skud pr. kanon  ·  %.0f patroner pr. mand  ·  proviant %.0f dage"), F.RoundsPerGun, F.CartridgesPerMan, F.FoodDays));
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - 190.f, Y - 33.f), FVector2D(168.f, 22.f), TEXT("SEND FORSYNING"), EButton::SupplySend, 1000000 + F.Id, false, !F.bBuilt || Map->FreeSupplyColumns() <= 0);
	// The companies holding it, in the order they fill it.
	Y += 4.f;
	PaintText(Geometry, Out, Layer + 2, TEXT("K O M P A G N I E R"), FVector2D(Pos.X + 22.f, Y), Serif(10), Gold, 0.f, false);
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - 170.f, Y - 11.f), FVector2D(148.f, 22.f), TEXT("+ KOMPAGNI"), EButton::FortPickCompany, F.Id, bFortPickCompany, !F.bBuilt);
	Y += 24.f;
	if (F.Companies.Num() == 0)
	{
		PaintText(Geometry, Out, Layer + 2, F.bBuilt ? TEXT("Ingen besætning: send et kompagni fra en bataljon i nærheden") : TEXT("Besættes, når skansen er færdig"),
			FVector2D(Pos.X + 22.f, Y), Serif(11, EFace::Italic), MutedInk, 0.f, false);
		Y += 24.f;
	}
	int32 Room = Campaign1851Forts::InfantryCapacity(F.bLarge);
	for (int32 c = 0; c < F.Companies.Num(); ++c)
	{
		const FCampaign1851FortCompany& C = F.Companies[c];
		if (!Regs.IsValidIndex(C.Regiment))
		{
			continue;
		}
		const FCampaign1851Regiment& R = Regs[C.Regiment];
		const int32 In = FMath::Min(C.Men, Room);
		Room -= In;
		const int32 Captain = R.Captains.IsValidIndex(C.Company) ? R.Captains[C.Company] : INDEX_NONE;
		const FString Where = In == C.Men ? FString(TEXT("inde")) : In == 0 ? FString(TEXT("reserve")) : FString::Printf(TEXT("%d inde, %d i reserve"), In, C.Men - In);
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%d. Kompagni (%s)  ·  %s  ·  %d mand  ·  %s"), Map->CompanyNumber(C.Regiment, C.Company), *R.Name,
			Officers.IsValidIndex(Captain) ? *FString::Printf(TEXT("Kaptajn %s"), *Officers[Captain].Name) : TEXT("ingen kaptajn"), C.Men, *Where),
			FVector2D(Pos.X + 22.f, Y), Serif(11), Ink, Size.X - 150.f);
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - 118.f, Y - 11.f), FVector2D(96.f, 22.f), TEXT("TRÆK UD"), EButton::FortReturn, c);
		Y += 24.f;
	}
	if (bFortPickCompany)
	{
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("Kompagnier fra bataljoner inden for %.0f km:"), Campaign1851Forts::CompanyReachKm), FVector2D(Pos.X + 22.f, Y + 4.f), Serif(11, EFace::Italic), Gold, 0.f, false);
		Y += 28.f;
		if (Candidates.Num() == 0)
		{
			PaintText(Geometry, Out, Layer + 2, TEXT("Ingen: før en bataljon hen til skansen først"), FVector2D(Pos.X + 22.f, Y), Serif(11, EFace::Italic), MutedInk, 0.f, false);
			Y += 24.f;
		}
		for (int32 r = 0; r < Candidates.Num() && r < 8; ++r)
		{
			const int32 Reg = Candidates[r] / 10, K = Candidates[r] % 10;
			const FCampaign1851Regiment& R = Regs[Reg];
			const int32 Captain = R.Captains.IsValidIndex(K) ? R.Captains[K] : INDEX_NONE;
			PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 18.f, Y - 11.f), FVector2D(Size.X - 36.f, 22.f), FString(), EButton::FortAddCompany, Candidates[r]);
			PaintTextFit(Geometry, Out, Layer + 3, FString::Printf(TEXT("%d. Kompagni (%s)  ·  %s  ·  %d mand"), Map->CompanyNumber(Reg, K), *R.Name,
				Officers.IsValidIndex(Captain) ? *FString::Printf(TEXT("Kaptajn %s"), *Officers[Captain].Name) : TEXT("ingen kaptajn"), Map->CompanyMen(Reg, K)),
				FVector2D(Pos.X + 28.f, Y), Serif(11), Ink, Size.X - 60.f);
			Y += 24.f;
		}
	}
	// Works.
	Y += 8.f;
	const bool bIdle = F.bBuilt && F.Work == ECampaign1851FortWork::None;
	const bool bMoreGuns = F.Guns < Campaign1851Forts::MaxGuns(F.bLarge);
	const bool bStronger = F.Defence < Campaign1851Forts::MaxDefence;
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, Y), FVector2D(166.f, 28.f),
		bMoreGuns ? FString::Printf(TEXT("+2 KANONER  %s"), *Thousands(Campaign1851Forts::GunsCost)) : FString(TEXT("ALLE KANONER")), EButton::FortGuns, F.Id, false, !bIdle || !bMoreGuns);
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 196.f, Y), FVector2D(170.f, 28.f),
		bStronger ? FString::Printf(TEXT("FORSTÆRK  %s"), *Thousands(Campaign1851Forts::DefenceCost(F.Defence + 1, F.bLarge))) : FString(TEXT("FULDT UDBYGGET")), EButton::FortDefence, F.Id, false, !bIdle || !bStronger);
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 374.f, Y), FVector2D(164.f, 28.f),
		F.bTrenches ? FString(TEXT("LØBEGRAVE BYGGET")) : FString::Printf(TEXT("LØBEGRAVE  %s"), *Thousands(Campaign1851Forts::TrenchesCost(F.bLarge))), EButton::FortTrenches, F.Id, false, !bIdle || F.bTrenches);
	Y += 34.f;
	PaintTextFit(Geometry, Out, Layer + 2, bStronger ? FString::Printf(TEXT("Næste forstærkning: %s: %s"), Campaign1851Forts::DefenceName(F.Defence + 1), Campaign1851Forts::DefenceNote(F.Defence + 1))
		: FString(TEXT("Løbegrave: dækning for reserven og forbindelse til skanser inden for 3 km")), FVector2D(Pos.X + 22.f, Y + 4.f), Serif(10, EFace::Italic), MutedInk, Size.X - 44.f);
	Y += 24.f;
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, Y), FVector2D(124.f, 26.f), TEXT("DREJ VENSTRE"), EButton::FortTurn, -1);
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 152.f, Y), FVector2D(124.f, 26.f), TEXT("DREJ HØJRE"), EButton::FortTurn, 1);
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 282.f, Y), FVector2D(130.f, 26.f), TEXT("VIS PÅ KORTET"), EButton::FortShow, F.Id);
	const int32 Code = 1000000 + F.Id;
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 418.f, Y), FVector2D(120.f, 26.f), DemolishArmed == Code ? TEXT("BEKRÆFT") : TEXT("SLØJF"), EButton::Demolish, Code,
		DemolishArmed == Code, F.Work == ECampaign1851FortWork::Demolish);
	if (DemolishArmed == Code)
	{
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Sløjfes: %d kanoner til lageret, materialer ca. %d rd., kompagnierne går hjem"), F.Guns, FMath::RoundToInt(F.Invested * 0.1)),
			FVector2D(Pos.X + 22.f, Y + 38.f), Serif(10, EFace::Italic), Gold, Size.X - 44.f);
	}
}

void SCampaign1851Overlay::PaintSupply(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	PaintText(Geometry, Out, Layer + 1, TEXT("Forsyning"), Pos + FVector2D(24.f, 34.f), Serif(22), Ink, 0.f);
	PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("Lager: %s geværer  ·  %d kanoner  ·  %s heste  ·  trænkolonner %d af %d ledige  ·  forråd %s rd./md."),
		*Thousands(Map->GetRifles()), Map->GetGunStock(), *Thousands(Map->GetHorseStock()), Map->FreeSupplyColumns(), Map->GetSupplyColumnCount(), *Thousands(int32(Map->StockingCostPerMonth()))),
		Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Size.X - 520.f);
	PaintButton(Geometry, Out, Layer + 1, Pos + FVector2D(Size.X - 470.f, 50.f), FVector2D(190.f, 28.f), bSupplyMap ? TEXT("SKJUL KORTET (F)") : TEXT("FORSYNINGSKORT (F)"), EButton::SupplyMap, 0, bSupplyMap);
	PaintButton(Geometry, Out, Layer + 1, Pos + FVector2D(Size.X - 272.f, 50.f), FVector2D(200.f, 28.f), FString::Printf(TEXT("KØB KOLONNE  %s"), *Thousands(Campaign1851Supply::ColumnCost)), EButton::SupplyBuy);
	// Depots.
	float Y = Pos.Y + 112.f;
	const float X = Pos.X + 24.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("D E P O T E R"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 24.f;
	const float DCols[] = { 0.f, 150.f, 300.f, 430.f, 540.f };
	const TCHAR* DHeads[] = { TEXT("By"), TEXT("Proviant"), TEXT("Foder"), TEXT("Ammunition"), TEXT("Materialer") };
	for (int32 c = 0; c < 5; ++c)
	{
		PaintText(Geometry, Out, Layer + 1, DHeads[c], FVector2D(X + DCols[c], Y), Serif(10, EFace::Italic), MutedInk, 0.f, false);
	}
	Y += 20.f;
	int32 Shown = 0;
	for (int32 c = 0; c < Cities.Num() && Shown < 14; ++c)
	{
		const FCampaign1851DepotCapacity Cap = Map->DepotCapacity(c);
		const double Materials = Map->GetMaterialsIn(c);
		if (Cap.Food + Cap.Fodder + Cap.Ammo <= 0.f && Materials < 1.0)
		{
			continue;
		}
		const FCampaign1851DepotStock S = Map->DepotStock(c);
		PaintTextFit(Geometry, Out, Layer + 1, Cities[c].Name, FVector2D(X + DCols[0], Y), Serif(12), Ink, 140.f);
		PaintText(Geometry, Out, Layer + 1, Cap.Food > 0.f ? FString::Printf(TEXT("%s / %s"), *Thousands(int32(S.Food)), *Thousands(int32(Cap.Food))) : FString(TEXT("-")), FVector2D(X + DCols[1], Y), Serif(12), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Cap.Fodder > 0.f ? FString::Printf(TEXT("%s / %s"), *Thousands(int32(S.Fodder)), *Thousands(int32(Cap.Fodder))) : FString(TEXT("-")), FVector2D(X + DCols[2], Y), Serif(12), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Cap.Ammo > 0.f ? FString::Printf(TEXT("%.0f / %.0f"), S.Ammo, Cap.Ammo) : FString(TEXT("-")), FVector2D(X + DCols[3], Y), Serif(12), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Materials >= 1.0 ? FString::Printf(TEXT("%s rd."), *Thousands(int32(Materials))) : FString(TEXT("-")), FVector2D(X + DCols[4], Y), Serif(12), Ink, 0.f, false);
		Y += 22.f;
		++Shown;
	}
	if (Shown == 0)
	{
		PaintTextFit(Geometry, Out, Layer + 1, TEXT("Ingen depoter: byg Depot og magasin (garnison), Kornmagasin eller Arsenal"), FVector2D(X, Y), Serif(12, EFace::Italic), MutedInk, 640.f);
		Y += 22.f;
	}
	// Columns under way.
	Y += 18.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("T R Æ N K O L O N N E R   U N D E R V E J S"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 24.f;
	const TArray<FCampaign1851SupplyColumn>& Columns = Map->GetSupplyColumns();
	if (Columns.Num() == 0)
	{
		PaintText(Geometry, Out, Layer + 1, TEXT("Ingen"), FVector2D(X, Y), Serif(12, EFace::Italic), MutedInk, 0.f, false);
		Y += 22.f;
	}
	for (int32 i = 0; i < Columns.Num() && i < 8; ++i)
	{
		const FCampaign1851SupplyColumn& C = Columns[i];
		const FString To = C.bFort ? (Map->FortIndex(C.Target) != INDEX_NONE ? Map->GetForts()[Map->FortIndex(C.Target)].Name : FString())
			: (Regs.IsValidIndex(C.Target) ? Regs[C.Target].Name : FString());
		PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("Kolonne %d  ·  %s  ·  %s %s  ·  %s rationer"), C.Id, Cities.IsValidIndex(C.Depot) ? *Cities[C.Depot].Name : TEXT(""),
			C.State == ESupplyColumnState::Outbound ? TEXT("på vej til") : TEXT("kører hjem fra"), *To, *Thousands(int32(C.Food))), FVector2D(X, Y), Serif(12), Ink, 640.f);
		Y += 22.f;
	}
	// Units in the field, the worst supplied first.
	const float RX = Pos.X + 720.f;
	float RY = Pos.Y + 112.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("E N H E D E R   I   F E L T E N"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
	RY += 24.f;
	TArray<int32> Field;
	for (int32 i = 0; i < Regs.Num(); ++i)
	{
		if (Regs[i].IsMarching() || Regs[i].IsInField())
		{
			Field.Add(i);
		}
	}
	Field.Sort([&Regs](int32 A, int32 B) { return Regs[A].Food < Regs[B].Food; });
	if (Field.Num() == 0)
	{
		PaintText(Geometry, Out, Layer + 1, TEXT("Hele hæren står i garnison"), FVector2D(RX, RY), Serif(12, EFace::Italic), MutedInk, 0.f, false);
	}
	for (int32 k = 0; k < Field.Num() && RY < Pos.Y + Size.Y - 40.f; ++k)
	{
		const FCampaign1851Regiment& R = Regs[Field[k]];
		const FLinearColor Status = R.Food < 1.f ? FLinearColor(0.85f, 0.25f, 0.2f) : R.Food < 2.f ? FLinearColor(0.9f, 0.75f, 0.25f) : FLinearColor(0.35f, 0.75f, 0.35f);
		PaintDot(Geometry, Out, Layer + 1, FVector2D(RX + 6.f, RY), 10.f, Status);
		PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("%s  ·  %s  ·  %s"), *R.Name, *Map->DescribePlace(R.Town, R.Km), *Campaign1851Supply::Describe(R)),
			FVector2D(RX + 18.f, RY), Serif(12), Ink, Size.X - 720.f - 170.f);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(Pos.X + Size.X - 150.f, RY - 11.f), FVector2D(126.f, 22.f), TEXT("SEND"), EButton::SupplySend, Field[k], false, Map->FreeSupplyColumns() <= 0);
		RY += 26.f;
	}
}

void SCampaign1851Overlay::PaintBattle(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851Battle>& Battles = Map->GetBattles();
	if (Battles.Num() == 0)
	{
		return;
	}
	const FCampaign1851Battle& B = Battles[0];
	const int32 Ci = Map->CorpsIndexOf(B);
	const FCampaign1851EnemyCorps* C = Ci != INDEX_NONE ? &Map->GetEnemyCorps()[Ci] : nullptr;
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const FVector2D Screen = Geometry.GetLocalSize();
	const FVector2D Size(760.f, 250.f);
	const FVector2D Pos((Screen.X - Size.X) * 0.5f, Screen.Y - 190.f - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	const FString Place = Map->GetCities().IsValidIndex(B.Town) ? Map->GetCities()[B.Town].Name : FString();
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("Slaget ved %s"), *Place), Pos + FVector2D(24.f, 34.f), Serif(24), FLinearColor(0.95f, 0.45f, 0.4f), 0.f);
	int32 Men = 0, Guns = 0;
	for (int32 i : B.Regiments)
	{
		Men += Regs.IsValidIndex(i) ? Regs[i].PresentMen() : 0;
		Guns += Regs.IsValidIndex(i) ? Regs[i].Guns : 0;
	}
	int32 FortMen = 0, FortGuns = 0;
	for (int32 Id : B.Forts)
	{
		const int32 Fi = Map->FortIndex(Id);
		if (Fi != INDEX_NONE)
		{
			int32 In = 0, Res = 0;
			Map->FortMen(Map->GetForts()[Fi], In, Res);
			FortMen += In + Res;
			FortGuns += Map->GetForts()[Fi].Guns;
		}
	}
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Danske: %d enheder, %s mand, %d kanoner%s"), B.Regiments.Num(), *Thousands(Men), Guns,
		B.Forts.Num() > 0 ? *FString::Printf(TEXT("  ·  %d skanser med %s mand og %d kanoner"), B.Forts.Num(), *Thousands(FortMen), FortGuns) : TEXT("")),
		Pos + FVector2D(24.f, 76.f), Serif(13), Ink, Size.X - 48.f);
	if (C)
	{
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Fjenden: %s, %s mand, %d kanoner"), *C->Name, *Thousands(C->Men), C->Guns), Pos + FVector2D(24.f, 102.f), Serif(13), Ink, Size.X - 48.f);
	}
	const float Odds = Map->BattleOdds(B);
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("Chance for dansk sejr (skøn): %.0f %%"), Odds * 100.f), Pos + FVector2D(24.f, 130.f), Serif(13, EFace::Italic), Gold, 0.f, false);
	if (B.bWaiting)
	{
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Venter på 3D-slaget: resultatet læses fra Saved/Battle/BattleResult_%d.json"), B.Id), Pos + FVector2D(24.f, 160.f), Serif(12, EFace::Italic), Ink, Size.X - 48.f);
		PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(24.f, Size.Y - 52.f), FVector2D(300.f, 32.f), TEXT("AFGØR AUTOMATISK I STEDET"), EButton::BattleAuto, B.Id);
		return;
	}
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(24.f, Size.Y - 52.f), FVector2D(220.f, 32.f), TEXT("UDKÆMP I 3D"), EButton::BattleFight3D, B.Id);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(Size.X - 170.f, 14.f), FVector2D(146.f, 24.f), TEXT("SE SLAGMARKEN"), EButton::BattlefieldAtBattle, B.Id);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(256.f, Size.Y - 52.f), FVector2D(250.f, 32.f), TEXT("AFGØR AUTOMATISK"), EButton::BattleAuto, B.Id);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(518.f, Size.Y - 52.f), FVector2D(218.f, 32.f), TEXT("TRÆK TILBAGE"), EButton::BattleRetreat, B.Id);
}

void SCampaign1851Overlay::PaintCouncil(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const TArray<FCampaign1851Nation>& Nations = Map->GetNations();
	PaintText(Geometry, Out, Layer + 1, TEXT("Statsrådet"), Pos + FVector2D(24.f, 34.f), Serif(22), Ink, 0.f);
	PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("Seed %d  ·  historisk afvigelse %d %%  ·  landene vokser og regeringerne beslutter selv; intet spil udvikler sig ens"),
		Map->GetSeed(), FMath::RoundToInt(Map->GetDeviation() * 100.f)), Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Size.X - 170.f);
	const float LeftW = 660.f;
	float Y = Pos.Y + 106.f;
	const float X = Pos.X + 24.f;
	// The nations.
	PaintText(Geometry, Out, Layer + 1, TEXT("N A T I O N E R N E"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 24.f;
	{
		int32 Active = 0;
		for (const FCampaign1851Nation& N : Nations) { Active += N.bActive ? 1 : 0; }
		PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("%d lande følges: tal, forhold og hærtyper"), Active), FVector2D(X, Y), Serif(12), Ink, LeftW - 220.f);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(X + LeftW - 200.f, Y - 12.f), FVector2D(180.f, 24.f), TEXT("ALLE LANDE"), EButton::MainMenu, int32(EWindow::Nations));
		Y += 34.f;
	}
	// The government and opinion.
	{
		const ECampaign1851Current Gov = Map->GetGovernment();
		PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("Regeringen %s  ·  %s  ·  siden %s"), *Map->GetPrimeMinister(), Campaign1851Politics::CurrentName(Gov),
			*ACampaign1851Map::FormatDate(ACampaign1851Map::StartDate() + FTimespan::FromDays(Map->GetGovernmentSince()), true)), FVector2D(X, Y), Serif(13), Gold, LeftW - 10.f);
		PaintTextFit(Geometry, Out, Layer + 1, Campaign1851Politics::CurrentEffect(Gov), FVector2D(X, Y + 17.f), Serif(10, EFace::Italic), MutedInk, LeftW - 10.f);
		if (ACampaign1851Map::ActiveScenario().Year < 1850 && Map->GetDate().GetYear() < 1848)
		{
			AddTip(FVector2D(X, Y - 14.f), FVector2D(LeftW - 10.f, 40.f),
				TEXT("1825: Statsrådet og rådgiverne er en forenklet model af enevælden. Strømningerne viser politiske tendenser, ikke partier eller valg."));
		}
		Y += 40.f;
		for (int32 c = 0; c < 3; ++c)
		{
			const ECampaign1851Current Cur = ECampaign1851Current(c);
			PaintText(Geometry, Out, Layer + 1, Campaign1851Politics::CurrentName(Cur), FVector2D(X + c * 220.f, Y), Serif(11), Ink, 0.f, false);
			PaintBar(Geometry, Out, Layer + 1, FVector2D(X + c * 220.f, Y + 10.f), 150.f, Map->GetSupport(Cur) / 100.f);
			PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%.0f %%"), Map->GetSupport(Cur)), FVector2D(X + c * 220.f + 158.f, Y + 12.f), Serif(10), Ink, 0.f, false);
		}
		Y += 34.f;
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Stemningen i landet %.0f / 100  ·  skatter × %.2f  ·  indkaldelse × %.2f"), Map->GetMood(), Map->TaxMoodFactor(), Map->CallInMoodFactor()),
			FVector2D(X, Y), Serif(12), Map->GetMood() < 30.f ? FLinearColor(0.95f, 0.4f, 0.35f) : Ink, 0.f, false);
		Y += 30.f;
	}
	// A nation on the abstract model: the player sets its government's priorities.
	if (Map->GetPlayedNation() != Map->GetPlayerNation() && Nations.IsValidIndex(Map->GetPlayedNation()))
	{
		const FCampaign1851Nation& Me = Nations[Map->GetPlayedNation()];
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("D U   S P I L L E R   %s"), *Me.Name.ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å"))), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, TEXT("Abstrakt model: sæt regeringens prioriteter; Danmark styres af AI på kortet"), FVector2D(X, Y + 22.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
		Y += 50.f;
		for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
		{
			PaintText(Geometry, Out, Layer + 1, Campaign1851Nations::PortfolioName(ECampaign1851Portfolio(p)), FVector2D(X, Y), Serif(14), Ink, 0.f, false);
			PaintBar(Geometry, Out, Layer + 1, FVector2D(X + 250.f, Y - 5.f), 200.f, Me.Weights[p] / 3.f);
			PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%.1f"), Me.Weights[p]), FVector2D(X + 460.f, Y), Serif(12), Ink, 0.f, false);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 500.f, Y - 12.f), FVector2D(40.f, 24.f), TEXT("-"), EButton::NationWeight, p * 2);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 546.f, Y - 12.f), FVector2D(40.f, 24.f), TEXT("+"), EButton::NationWeight, p * 2 + 1);
			Y += 36.f;
		}
	}
	// The player's ministries.
	else if (Nations.IsValidIndex(Map->GetPlayerNation()))
	{
		const FCampaign1851Nation& Me = Nations[Map->GetPlayerNation()];
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("R E S S O R T E R   ·   %s"), *Me.Name.ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å"))), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
		// All at once.
		PaintText(Geometry, Out, Layer + 1, TEXT("alle:"), FVector2D(X + 330.f, Y), Serif(10, EFace::Italic), MutedInk, 0.f, false);
		for (int32 m = 0; m < 3; ++m)
		{
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 368.f + m * 96.f, Y - 11.f), FVector2D(90.f, 22.f), Campaign1851Nations::DelegationName(ECampaign1851Delegation(m)), EButton::DelegateAll, m);
		}
		Y += 20.f;
		PaintTextFit(Geometry, Out, Layer + 1, TEXT("MANUEL: du bestemmer selv  ·  RÅDGIVER: ministeren foreslår, du trykker UDFØR  ·  AUTO: ministeren handler selv (klik skifter)"),
			FVector2D(X, Y), Serif(9, EFace::Italic), MutedInk, LeftW - 10.f);
		Y += 20.f;
		for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
		{
			const ECampaign1851Portfolio P = ECampaign1851Portfolio(p);
			const FCampaign1851Minister& Min = Map->GetMinister(P);
			PaintText(Geometry, Out, Layer + 1, Campaign1851Nations::PortfolioName(P), FVector2D(X, Y), Serif(13), Ink, 0.f, false);
			PaintTextFit(Geometry, Out, Layer + 1, Campaign1851Nations::PortfolioScope(P), FVector2D(X, Y + 15.f), Serif(9, EFace::Italic), MutedInk, 150.f);
			PaintPortrait(Geometry, Out, Layer + 1, FVector2D(X + 158.f, Y - 8.f), FVector2D(28.f, 35.f), Min.Name, 2);
			PaintTextFit(Geometry, Out, Layer + 1, Min.Name, FVector2D(X + 192.f, Y), Serif(12), MinisterInfo == p ? Ink : Gold, 150.f);
			// Click the picture or the name: his card.
			Buttons.Add({ FVector2D(X + 156.f, Y - 9.f), FVector2D(X + 350.f, Y + 28.f), EButton::MinisterInfo, p });
			AddTip(FVector2D(X + 156.f, Y - 9.f), FVector2D(194.f, 37.f), TEXT("Klik: ministerens kort med portræt, evner og seneste beslutninger"));
			PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("dygtig %d · sparsom %d · forsigtig %d"), Min.Skill, Min.Thrift, Min.Caution), FVector2D(X + 192.f, Y + 15.f), Serif(9, EFace::Italic), MutedInk, 200.f);

			const int32 Mode = int32(Me.Modes[p]);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 400.f, Y - 6.f), FVector2D(130.f, 24.f), Campaign1851Nations::DelegationName(ECampaign1851Delegation(Mode)),
				EButton::Delegate, p * 3 + (Mode + 1) % 3, Mode == 2);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 540.f, Y - 6.f), FVector2D(110.f, 24.f), TEXT("UDSKIFT"), EButton::MinisterDismiss, p, MinisterPick == p);
			// What AUTO may spend: a monthly allowance into a pot of at most three months.
			const double Allow = Map->GetMinistryBudget(P), Pot = Map->GetMinistryPot(P);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 400.f, Y + 21.f), FVector2D(22.f, 17.f), TEXT("-"), EButton::MinistryBudget, p * 2);
			PaintTextFit(Geometry, Out, Layer + 1, Allow <= 0.0 ? FString(TEXT("AUTO må intet bruge")) : FString::Printf(TEXT("%s rd./md.  ·  pulje %s"), *Thousands(int32(Allow)), *Thousands(int32(Pot))),
				FVector2D(X + 428.f, Y + 30.f), Serif(10, EFace::Italic), Mode == 2 ? Gold : MutedInk, 194.f);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 628.f, Y + 21.f), FVector2D(22.f, 17.f), TEXT("+"), EButton::MinistryBudget, p * 2 + 1);
			Y += 48.f;
		}
		Y += 6.f;
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Mindste kassebeholdning: %s rd."), *Thousands(int32(Me.Reserve))), FVector2D(X, Y), Serif(13), Ink, 0.f, false);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 330.f, Y - 12.f), FVector2D(60.f, 24.f), TEXT("-"), EButton::Reserve, 0);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 396.f, Y - 12.f), FVector2D(60.f, 24.f), TEXT("+"), EButton::Reserve, 1);
		PaintText(Geometry, Out, Layer + 1, TEXT("ministerierne bruger kun penge over denne grænse"), FVector2D(X, Y + 20.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 470.f, Y - 12.f), FVector2D(180.f, 24.f), TEXT("BUDGET OG KASSE"), EButton::MainMenu, int32(EWindow::Budget));
	}
	// The decisions and recommendations, newest first (or the candidates for a minister's post).
	const float RX = Pos.X + 24.f + LeftW + 20.f, RW = Size.X - LeftW - 68.f;
	float RY = Pos.Y + 106.f;
	if (MinisterPick >= 0 && MinisterPick < int32(ECampaign1851Portfolio::Count))
	{
		const ECampaign1851Portfolio P = ECampaign1851Portfolio(MinisterPick);
		DrawLines(Geometry, Out, Layer + 1, { FVector2D(RX - 12.f, RY - 10.f), FVector2D(RX - 12.f, Pos.Y + Size.Y - 24.f) }, Gold.CopyWithNewOpacity(0.3f), 1.f);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("K A N D I D A T E R   ·   %s"), *FString(Campaign1851Nations::PortfolioName(P)).ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å"))), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + RW - 90.f, RY - 12.f), FVector2D(90.f, 24.f), TEXT("LUK"), EButton::MinisterPickClose, 0);
		RY += 30.f;
		const FCampaign1851Minister& Now = Map->GetMinister(P);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Nu: %s (%s)  ·  dygtig %d · sparsom %d · forsigtig %d"), *Now.Name, Campaign1851Politics::CurrentName(Now.Line), Now.Skill, Now.Thrift, Now.Caution),
			FVector2D(RX, RY), Serif(12, EFace::Italic), MutedInk, 0.f, false);
		RY += 34.f;
		const TArray<FCampaign1851Minister> Candidates = Map->MinisterCandidates(P);
		for (int32 c = 0; c < Candidates.Num(); ++c)
		{
			const FCampaign1851Minister& M = Candidates[c];
			PaintText(Geometry, Out, Layer + 1, M.Name, FVector2D(RX, RY), Serif(15), Ink, 0.f, false);
			PaintText(Geometry, Out, Layer + 1, Campaign1851Politics::CurrentName(M.Line), FVector2D(RX, RY + 20.f), Serif(10, EFace::Italic), M.Line == Map->GetGovernment() ? Gold : MutedInk, 0.f, false);
			const TCHAR* Names[] = { TEXT("dygtig"), TEXT("sparsom"), TEXT("forsigtig") };
			const uint8 Values[] = { M.Skill, M.Thrift, M.Caution };
			for (int32 q = 0; q < 3; ++q)
			{
				PaintText(Geometry, Out, Layer + 1, Names[q], FVector2D(RX + 230.f + q * 150.f, RY), Serif(10, EFace::Italic), MutedInk, 0.f, false);
				PaintBar(Geometry, Out, Layer + 1, FVector2D(RX + 230.f + q * 150.f, RY + 10.f), 110.f, Values[q] / 10.f);
				PaintText(Geometry, Out, Layer + 1, FString::FromInt(Values[q]), FVector2D(RX + 348.f + q * 150.f, RY + 14.f), Serif(11), Ink, 0.f, false);
			}
			PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + RW - 120.f, RY - 4.f), FVector2D(120.f, 26.f), TEXT("UDNÆVN"), EButton::MinisterAppoint, MinisterPick * 10 + c);
			RY += 52.f;
		}
		PaintTextFit(Geometry, Out, Layer + 1, TEXT("En minister fra regeringens egen strømning (guld) sidder sikrest; de andre skiftes ud ved næste regeringsskifte."), FVector2D(RX, RY + 10.f),
			Serif(10, EFace::Italic), MutedInk, RW);
		return;
	}
	DrawLines(Geometry, Out, Layer + 1, { FVector2D(RX - 12.f, RY - 10.f), FVector2D(RX - 12.f, Pos.Y + Size.Y - 24.f) }, Gold.CopyWithNewOpacity(0.3f), 1.f);
	if (MinisterInfo >= 0 && MinisterInfo < int32(ECampaign1851Portfolio::Count))
	{
		PaintMinisterCard(Geometry, Out, Layer + 3, FVector2D(RX, RY - 14.f), FVector2D(RW, Pos.Y + Size.Y - 30.f - (RY - 14.f)), MinisterInfo);
		return;
	}
	PaintText(Geometry, Out, Layer + 1, TEXT("B E S L U T N I N G E R   O G   A N B E F A L I N G E R"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
	PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + RW - 180.f, RY - 12.f), FVector2D(180.f, 24.f), TEXT("HÆRENS STATUS"), EButton::MainMenu, int32(EWindow::ArmyStatus));
	AddTip(FVector2D(RX + RW - 180.f, RY - 12.f), FVector2D(180.f, 24.f), TEXT("Krigsministerens oversigt: hæren pr. våbenart, tabene på begge sider og det erobrede udstyr."));
	RY += 28.f;
	const TArray<FCampaign1851Decision>& Decisions = Map->GetDecisions();
	if (Decisions.Num() == 0)
	{
		PaintTextFit(Geometry, Out, Layer + 1, TEXT("Endnu ingen: ministerierne træffer beslutninger ved hver månedsskifte (sæt et ressort til RÅDGIVER eller AUTO)."), FVector2D(RX, RY + 10.f), Serif(12, EFace::Italic), MutedInk, RW);
	}
	const int32 Rows = FMath::FloorToInt((Pos.Y + Size.Y - 40.f - RY) / 46.f);
	for (int32 r = 0; r < Rows && r < Decisions.Num(); ++r)
	{
		const int32 i = Decisions.Num() - 1 - r;
		const FCampaign1851Decision& D = Decisions[i];
		const FString Who = Nations.IsValidIndex(D.Nation) ? Nations[D.Nation].Id : FString();
		const FString When = ACampaign1851Map::FormatDate(ACampaign1851Map::StartDate() + FTimespan::FromDays(D.Day), true);
		PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("%s  ·  %s  ·  %s"), *When, *Who, Campaign1851Nations::PortfolioName(D.Portfolio)), FVector2D(RX, RY), Serif(10, EFace::Italic), MutedInk, RW - 130.f);
		PaintTextFit(Geometry, Out, Layer + 1, D.Action, FVector2D(RX, RY + 15.f), Serif(13), D.bAdvice && !D.bDone ? Gold : Ink, RW - 130.f);
		PaintTextFit(Geometry, Out, Layer + 1, D.Reasons, FVector2D(RX, RY + 31.f), Serif(10, EFace::Italic), MutedInk, RW - 10.f);
		if (D.bAdvice && !D.bDone)
		{
			PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + RW - 118.f, RY + 2.f), FVector2D(110.f, 24.f), TEXT("UDFØR"), EButton::DecisionExecute, i);
		}
		else
		{
			PaintText(Geometry, Out, Layer + 1, D.bAdvice ? TEXT("fulgt") : TEXT("udført"), FVector2D(RX + RW - 12.f, RY + 15.f), Serif(10, EFace::Italic), Gold, 1.f, false);
		}
		RY += 46.f;
	}
}

void SCampaign1851Overlay::PaintForeign(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const TArray<FCampaign1851Nation>& Nations = Map->GetNations();
	PaintText(Geometry, Out, Layer + 1, TEXT("Udenrigsministeriet"), Pos + FVector2D(24.f, 34.f), Serif(22), Ink, 0.f);
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Forholdet til udlandet: gesandter, handelstraktater, en alliance med Sverige-Norge og stormagternes garanti for helstaten"),
		Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Size.X - 170.f);
	const float X = Pos.X + 24.f;
	float Y = Pos.Y + 106.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("F O R H O L D E T   T I L   D A N M A R K"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	PaintButton(Geometry, Out, Layer + 1, FVector2D(Pos.X + Size.X - 260.f, Y - 14.f), FVector2D(180.f, 26.f), TEXT("ALLE LANDE"), EButton::MainMenu, int32(EWindow::Nations));
	Y += 24.f;
	// The tension with the German Confederation (war at 80).
	PaintText(Geometry, Out, Layer + 1, Map->IsAtWar() ? TEXT("KRIG med Preussen og Østrig") : TEXT("Spænding, Tyske Forbund"), FVector2D(X, Y + 8.f), Serif(13), Map->IsAtWar() ? FLinearColor(0.95f, 0.4f, 0.35f) : Ink, 0.f, false);
	PaintBar(Geometry, Out, Layer + 1, FVector2D(X + 210.f, Y + 3.f), 220.f, Map->GetTension() / 100.f);
	PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%.0f / 100  (krig ved 80)"), Map->GetTension()), FVector2D(X + 444.f, Y + 8.f), Serif(11), Ink, 0.f, false);
	AddTip(FVector2D(X, Y - 4.f), FVector2D(560.f, 24.f), TEXT("Spændingen med Det tyske forbund stiger med de historiske begivenheder og jeres politik (Ejderpolitikken skærper den, garantier dæmper den). Ved 80 bryder krigen ud."));
	Y += 32.f;
	// The countries by group, one tab at a time.
	{
		const TCHAR* Tabs[] = { TEXT("STORMAGTER"), TEXT("NORDEN"), TEXT("TYSKE STATER"), TEXT("VESTEN") };
		for (int32 t = 0; t < 4; ++t)
		{
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 360.f + t * 150.f, Y - 14.f), FVector2D(142.f, 24.f), Tabs[t], EButton::ForeignTab, t, ForeignTab == t);
		}
		Y += 20.f;
	}
	const TCHAR* Groups[] = { TEXT("stormagt"), TEXT("norden"), TEXT("tysk"), TEXT("vest") };
	const FString Group = Groups[FMath::Clamp(ForeignTab, 0, 3)];
	const float Cols[] = { 0.f, 170.f, 400.f, 520.f, 660.f, 800.f, 940.f };
	const TCHAR* Heads[] = { TEXT("Land"), TEXT("Forhold"), TEXT("Handel rd./år"), TEXT("Gesandt"), TEXT("Handelstraktat"), TEXT("Alliance"), TEXT("Garanti") };
	for (int32 c = 0; c < 7; ++c)
	{
		PaintText(Geometry, Out, Layer + 1, Heads[c], FVector2D(X + Cols[c], Y), Serif(10, EFace::Italic), MutedInk, 0.f, false);
	}
	Y += 26.f;
	const TCHAR* Labels[] = { TEXT("GESANDT"), TEXT("TRAKTAT"), TEXT("ALLIANCE"), TEXT("GARANTI") };
	const double Costs[] = { ACampaign1851Map::EnvoyCost, ACampaign1851Map::TreatyCost, ACampaign1851Map::AllianceCost, ACampaign1851Map::GuaranteeCost };
	for (int32 n = 0; n < Nations.Num(); ++n)
	{
		const FCampaign1851Nation& N = Nations[n];
		if (n == Map->GetPlayerNation() || !N.bActive || N.Group != Group)
		{
			continue;
		}
		PaintTextFit(Geometry, Out, Layer + 1, N.Name, FVector2D(X + Cols[0], Y), Serif(13), Ink, 160.f);
		// The relation as a bar from -100 to 100.
		const FLinearColor RelColour = N.Relation >= 40.f ? FLinearColor(0.45f, 0.75f, 0.4f) : N.Relation >= 0.f ? Ink : FLinearColor(0.95f, 0.4f, 0.35f);
		PaintBar(Geometry, Out, Layer + 1, FVector2D(X + Cols[1], Y - 5.f), 150.f, (N.Relation + 100.f) / 200.f);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%+.0f"), N.Relation), FVector2D(X + Cols[1] + 160.f, Y), Serif(12), RelColour, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, N.bTrade ? Thousands(N.TradeValue) : FString::Printf(TEXT("(%s)"), *Thousands(N.TradeValue)), FVector2D(X + Cols[2], Y), Serif(12), N.bTrade ? Ink : MutedInk, 0.f, false);
		for (int32 a = 0; a < 4; ++a)
		{
			const FString Why = Map->DiplomacyBlockReason(n, ACampaign1851Map::EDiplomacyAction(a));
			if (Why == TEXT("-"))
			{
				continue;
			}
			const bool bHas = (a == 1 && N.bTrade) || (a == 2 && N.bAlliance) || (a == 3 && N.bGuarantee);
			if (bHas)
			{
				PaintText(Geometry, Out, Layer + 1, a == 1 ? TEXT("indgået") : a == 2 ? TEXT("allieret") : TEXT("givet"), FVector2D(X + Cols[3 + a], Y), Serif(12, EFace::Italic), Gold, 0.f, false);
				continue;
			}
			// A blocked action shows why in its (dimmed) button.
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + Cols[3 + a], Y - 12.f), FVector2D(132.f, 24.f), Why.IsEmpty() ? FString::Printf(TEXT("%s %dk"), Labels[a], int32(Costs[a] / 1000.0)) : Why,
				EButton::Diplomacy, n * 10 + a, false, !Why.IsEmpty());
		}
		PaintTextFit(Geometry, Out, Layer + 1, N.Note, FVector2D(X, Y + 16.f), Serif(9, EFace::Italic), MutedInk, 380.f);
		Y += 48.f;
	}
	Y += 10.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("I N D T Æ G T E R   F R A   U D L A N D E T"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 26.f;
	PaintText(Geometry, Out, Layer + 1, Map->IsSoundDuesAbolished() ? TEXT("Øresundstolden er ophævet: kapitaliseringen betales i 20 år") : TEXT("Øresundstolden (ophæves ved Københavnstraktaten 1857)"),
		FVector2D(X, Y), Serif(13), Ink, 0.f, false);
	PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("I alt %s rd. om året  ·  %d handelstraktater (+%.2f %% vækst i handelsbyerne)"), *Thousands(int32(Map->ForeignIncomePerYear())),
		Map->TradeTreaties(), 0.05f * Map->TradeTreaties()), FVector2D(X, Y + 22.f), Serif(12), Ink, 0.f, false);
	Y += 60.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("K R I G   O G   F R E D"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 26.f;
	PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Spænding %.0f / 100  ·  garantierne dæmper stigninger til %.0f %%"), Map->GetTension(), Map->GuaranteeDamping() * 100.f),
		FVector2D(X, Y), Serif(13), Ink, 0.f, false);
	Y += 28.f;
	if (Map->IsAtWar())
	{
		PaintText(Geometry, Out, Layer + 1, Map->HasPeaceConference() ? TEXT("Fredskonferencen i London er samlet") : TEXT("Ingen konference endnu (kræver en garantimagt med forhold 40 og 60 dages krig)"),
			FVector2D(X, Y), Serif(12, EFace::Italic), Map->HasPeaceConference() ? Gold : MutedInk, 0.f, false);
		Y += 24.f;
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Krigsstilling %+.2f  ·  tab: danske %s, fjendens %s"), Map->WarScore(), *Thousands(Map->GetDanishWarLosses()), *Thousands(Map->GetEnemyWarLosses())),
			FVector2D(X, Y), Serif(12), Ink, 0.f, false);
		Y += 26.f;
		const TArray<FCampaign1851PeaceOffer> Offers = Map->PeaceOffers();
		for (int32 o = 0; o < Offers.Num(); ++o)
		{
			const FCampaign1851PeaceOffer& Offer = Offers[o];
			PaintText(Geometry, Out, Layer + 1, Offer.Name, FVector2D(X, Y), Serif(13), Offer.bAccepted ? Ink : MutedInk, 0.f, false);
			PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("%d byer afstås  ·  %s"), Offer.Ceded.Num(), *Offer.Why), FVector2D(X + 200.f, Y), Serif(11, EFace::Italic), MutedInk, 600.f);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 820.f, Y - 12.f), FVector2D(140.f, 24.f), TEXT("TILBYD"), EButton::MakePeace, o, false, !Offer.bAccepted);
			Y += 30.f;
		}
	}
	else
	{
		PaintText(Geometry, Out, Layer + 1, TEXT("Fred. En alliance hæver spændingen med Berlin og Wien lidt; stormagternes garanti dæmper den."), FVector2D(X, Y), Serif(12, EFace::Italic), MutedInk, 0.f, false);
	}
}

void SCampaign1851Overlay::PaintBranchSymbol(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, int32 Branch, const FVector2D& C, float R) const
{
	// A plain emblem in a round frame: red cross, star fort, telegraph, crossed rifles, cannon.
	TArray<FVector2D> Ring;
	for (int32 a = 0; a <= 32; ++a)
	{
		Ring.Add(C + FVector2D(FMath::Cos(a * UE_TWO_PI / 32.f), FMath::Sin(a * UE_TWO_PI / 32.f)) * R);
	}
	DrawLines(Geometry, Out, Layer, Ring, Gold.CopyWithNewOpacity(0.7f), 1.5f);
	const float S = R * 0.62f;
	const FLinearColor Mark = Ink;
	auto L = [&](std::initializer_list<FVector2D> Pts, const FLinearColor& Colour, float Thick)
	{
		TArray<FVector2D> P;
		for (const FVector2D& Q : Pts) { P.Add(C + Q * S); }
		DrawLines(Geometry, Out, Layer + 1, P, Colour, Thick);
	};
	switch (Branch)
	{
	case 0:   // the red cross of the ambulances
		L({ FVector2D(0.f, -0.8f), FVector2D(0.f, 0.8f) }, FLinearColor(0.85f, 0.15f, 0.12f), 5.f);
		L({ FVector2D(-0.8f, 0.f), FVector2D(0.8f, 0.f) }, FLinearColor(0.85f, 0.15f, 0.12f), 5.f);
		break;
	case 1:   // a star fort seen from above
	{
		TArray<FVector2D> Star;
		for (int32 k = 0; k <= 10; ++k)
		{
			const float A = -UE_HALF_PI + k * UE_PI / 5.f;
			Star.Add(C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * S * (k % 2 == 0 ? 1.f : 0.5f));
		}
		DrawLines(Geometry, Out, Layer + 1, Star, Mark, 2.f);
		break;
	}
	case 2:   // a telegraph pole and its wire
		L({ FVector2D(0.f, 0.9f), FVector2D(0.f, -0.9f) }, Mark, 2.f);
		L({ FVector2D(-0.5f, -0.6f), FVector2D(0.5f, -0.6f) }, Mark, 2.f);
		L({ FVector2D(-0.35f, -0.3f), FVector2D(0.35f, -0.3f) }, Mark, 2.f);
		L({ FVector2D(-0.95f, -0.45f), FVector2D(-0.5f, -0.6f), FVector2D(0.5f, -0.6f), FVector2D(0.95f, -0.45f) }, Gold, 1.f);
		break;
	case 3:   // two rifles crossed
		L({ FVector2D(-0.8f, 0.8f), FVector2D(0.8f, -0.8f) }, Mark, 2.5f);
		L({ FVector2D(0.8f, 0.8f), FVector2D(-0.8f, -0.8f) }, Mark, 2.5f);
		L({ FVector2D(-0.8f, 0.8f), FVector2D(-0.55f, 0.55f) }, Gold, 4.f);
		L({ FVector2D(0.8f, 0.8f), FVector2D(0.55f, 0.55f) }, Gold, 4.f);
		break;
	case 5:   // a horseshoe and a sabre
	{
		TArray<FVector2D> Shoe;
		for (int32 a = 0; a <= 12; ++a)
		{
			const float A = UE_PI * (0.15f + 0.7f * a / 12.f) + UE_PI;
			Shoe.Add(C + FVector2D(FMath::Cos(A) * 0.6f, -FMath::Sin(A) * 0.7f + 0.1f) * S);
		}
		DrawLines(Geometry, Out, Layer + 1, Shoe, Gold, 3.f);
		L({ FVector2D(-0.7f, 0.8f), FVector2D(0.75f, -0.85f) }, Mark, 2.f);
		L({ FVector2D(-0.75f, 0.55f), FVector2D(-0.45f, 0.85f) }, Mark, 3.f);
		break;
	}
	case 6:   // a staff flag on its pole
		L({ FVector2D(-0.6f, 0.9f), FVector2D(-0.6f, -0.9f) }, Mark, 2.5f);
		L({ FVector2D(-0.6f, -0.9f), FVector2D(0.7f, -0.6f), FVector2D(-0.6f, -0.25f) }, FLinearColor(0.85f, 0.15f, 0.12f), 3.f);
		L({ FVector2D(-0.6f, -0.9f), FVector2D(-0.6f, -0.25f) }, FLinearColor(0.85f, 0.15f, 0.12f), 3.f);
		break;
	case 7:   // the trades: a sheaf of corn, bound in the middle
		for (int32 k = -2; k <= 2; ++k)
		{
			L({ FVector2D(k * 0.12f, 0.8f), FVector2D(k * 0.32f, -0.8f) }, Mark, 2.f);
		}
		L({ FVector2D(-0.45f, 0.1f), FVector2D(0.45f, 0.1f) }, FLinearColor(0.85f, 0.65f, 0.2f), 3.f);
		break;
	default:  // a field gun: barrel and wheel
	{
		L({ FVector2D(-0.9f, -0.35f), FVector2D(0.5f, 0.15f) }, Mark, 4.f);
		TArray<FVector2D> Wheel;
		for (int32 a = 0; a <= 16; ++a)
		{
			Wheel.Add(C + (FVector2D(0.2f, 0.35f) + FVector2D(FMath::Cos(a * UE_TWO_PI / 16.f), FMath::Sin(a * UE_TWO_PI / 16.f)) * 0.42f) * S);
		}
		DrawLines(Geometry, Out, Layer + 1, Wheel, Gold, 2.f);
		L({ FVector2D(0.2f, 0.35f), FVector2D(0.95f, 0.75f) }, Mark, 2.f);
		break;
	}
	}
}

void SCampaign1851Overlay::PaintResearch(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	PaintText(Geometry, Out, Layer + 1, TEXT("Forskning og doktriner"), Pos + FVector2D(24.f, 34.f), Serif(22), Ink, 0.f);
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Krigsministeriet og Indenrigsministeriet forsker hver med ét projekt ad gangen, betalt måned for måned; doktrinen bestemmer, hvordan hæren kæmper"),
		Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Size.X - 170.f);
	const float X = Pos.X + 24.f;
	float Y = Pos.Y + 106.f;
	const bool bDoc = ResearchTab == 2;   // the doctrines have a tab of their own
	const float LeftW = bDoc ? 0.f : Size.X - 80.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("F O R S K N I N G"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	// Two tracks: the military research (the War Ministry) and the civil (the Interior Ministry); a project at a time in each.
	PaintButton(Geometry, Out, Layer + 3, FVector2D(X + 170.f, Y - 14.f), FVector2D(150.f, 24.f), TEXT("MILITÆR"), EButton::ResearchTab, 0, ResearchTab == 0);
	PaintButton(Geometry, Out, Layer + 3, FVector2D(X + 326.f, Y - 14.f), FVector2D(150.f, 24.f), TEXT("CIVIL"), EButton::ResearchTab, 1, ResearchTab == 1);
	PaintButton(Geometry, Out, Layer + 3, FVector2D(X + 482.f, Y - 14.f), FVector2D(150.f, 24.f), TEXT("DOKTRINER"), EButton::ResearchTab, 2, bDoc);
	Y += 24.f;
	// Columns by branch, rows by year (1852 at the top), a box per topic; click a box to start it.
	const TArray<FCampaign1851ResearchTopic>& Topics = Campaign1851Research::Topics();
	const float YearW = 46.f;
	// The columns of this tab: the branches that have a subject in it.
	const bool bResearchCivil = ResearchTab == 1;
	TArray<int32> TabColumns;
	for (int32 t = 0; t < Topics.Num(); ++t)
	{
		if (Campaign1851Research::IsCivil(t) == bResearchCivil) { TabColumns.AddUnique(Topics[t].Branch); }
	}
	TabColumns.Sort();
	auto ColumnOf = [&TabColumns](int32 Branch) { return FMath::Max(0, TabColumns.IndexOfByKey(Branch)); };
	auto InTab = [&](int32 Topic) { return !bDoc && Campaign1851Research::IsCivil(Topic) == bResearchCivil; };
	if (bDoc) { TabColumns.Reset(); }
	// The level shown: a prerequisite in the other tab (the telegraph for the railway mobilisation) does not push a topic down.
	TFunction<int32(int32)> DisplayTier = [&](int32 Topic) -> int32
	{
		const int32 Need = Topics[Topic].Needs ? Campaign1851Research::FindTopic(Topics[Topic].Needs) : INDEX_NONE;
		return Topics.IsValidIndex(Need) && InTab(Need) ? DisplayTier(Need) + 1 : 0;
	};
	// A branch whose topics stand side by side on a level (the infantry) gets a column as wide as its widest level, so the boxes stay large.
	TArray<float> ColX, ColWid, ColMost;
	{
		TArray<float> Weight;
		float Sum = 0.f;
		for (int32 ci = 0; ci < TabColumns.Num(); ++ci)
		{
			int32 Most = 1;
			for (int32 t = 0; t < Topics.Num(); ++t)
			{
				if (Topics[t].Branch != TabColumns[ci] || !InTab(t)) { continue; }
				int32 Same = 0;
				for (int32 u = 0; u < Topics.Num(); ++u)
				{
					if (Topics[u].Branch == TabColumns[ci] && InTab(u) && DisplayTier(u) == DisplayTier(t)) { ++Same; }
				}
				Most = FMath::Max(Most, Same);
			}
			Weight.Add(float(Most));
			ColMost.Add(float(Most));
			Sum += float(Most);
		}
		float At = X + YearW;
		for (int32 ci = 0; ci < Weight.Num(); ++ci)
		{
			ColX.Add(At);
			ColWid.Add((LeftW - YearW) * Weight[ci] / FMath::Max(1.f, Sum));
			At += ColWid[ci];
		}
	}
	const float Top = Y + 84.f;   // the headings, then the branch symbols
	int32 Years = 1;   // the number of levels
	for (int32 t = 0; t < Topics.Num(); ++t)
	{
		if (InTab(t)) { Years = FMath::Max(Years, DisplayTier(t) + 1); }
	}
	const float RowH = FMath::Min(120.f, (Pos.Y + Size.Y - 30.f - Top) / Years);
	const FVector2D Box(0.f, RowH - 14.f);   // the height of a box (the width is the column's)
	for (int32 ci = 0; ci < TabColumns.Num(); ++ci)
	{
		const int32 c = ci;
		PaintBranchSymbol(Geometry, Out, Layer + 1, TabColumns[ci], FVector2D(ColX[c] + ColWid[c] * 0.5f, Y + 44.f), 22.f);
		PaintTextFit(Geometry, Out, Layer + 1, FString(Campaign1851Research::BranchName(TabColumns[ci])).ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å")), FVector2D(ColX[c] + 11.f, Y), Serif(10), Gold, ColWid[c] - 22.f);
	}
	for (int32 r = 0; r < (bDoc ? 0 : Years); ++r)
	{
		const float RY = Top + r * RowH;
		PaintText(Geometry, Out, Layer + 1, Campaign1851Research::Roman(r), FVector2D(X + 8.f, RY + Box.Y * 0.5f), Serif(16), Gold, 0.f, false);
		DrawLines(Geometry, Out, Layer, { FVector2D(X + YearW - 6.f, RY - 7.f), FVector2D(X + LeftW, RY - 7.f) }, Gold.CopyWithNewOpacity(0.12f), 1.f);
	}
	// Topics on the same level of a branch stand side by side in the column's slots; a lone topic stands in the slot of what it needs.
	auto Share = [&](const FCampaign1851ResearchTopic& T, int32& OutIndex)
	{
		const int32 Self = Campaign1851Research::FindTopic(T.Id);
		const int32 Tier = DisplayTier(Self);
		int32 Count = 0;
		OutIndex = 0;
		for (int32 u = 0; u < Topics.Num(); ++u)
		{
			if (Topics[u].Branch == T.Branch && DisplayTier(u) == Tier && InTab(u))
			{
				if (u == Self) { OutIndex = Count; }
				++Count;
			}
		}
		const int32 Need = T.Needs ? Campaign1851Research::FindTopic(T.Needs) : INDEX_NONE;
		if (Count == 1 && Topics.IsValidIndex(Need) && InTab(Need) && Topics[Need].Branch == T.Branch)
		{
			int32 Slot = 0;
			for (int32 u = 0; u < Need; ++u)
			{
				if (Topics[u].Branch == T.Branch && DisplayTier(u) == DisplayTier(Need) && InTab(u)) { ++Slot; }
			}
			OutIndex = Slot;
		}
		return FMath::Max(1, Count);
	};
	auto TopicBoxSize = [&](const FCampaign1851ResearchTopic& T)
	{
		const int32 Column = ColumnOf(T.Branch);
		const float Most = ColMost.IsValidIndex(Column) ? ColMost[Column] : 1.f;
		return FVector2D((ColWid[Column] - 22.f - 6.f * (Most - 1.f)) / Most, Box.Y);
	};
	auto BoxPos = [&](const FCampaign1851ResearchTopic& T)
	{
		int32 Index = 0;
		Share(T, Index);
		const float W = TopicBoxSize(T).X;
		return FVector2D(ColX[ColumnOf(T.Branch)] + 11.f + Index * (W + 6.f), Top + DisplayTier(Campaign1851Research::FindTopic(T.Id)) * RowH);
	};
	// The lines first, under the boxes.
	for (const FCampaign1851ResearchTopic& T : Topics)
	{
		const int32 Need = T.Needs ? Campaign1851Research::FindTopic(T.Needs) : INDEX_NONE;
		if (Topics.IsValidIndex(Need) && InTab(Campaign1851Research::FindTopic(T.Id)) && InTab(Need))
		{
			const FVector2D From = BoxPos(Topics[Need]) + FVector2D(TopicBoxSize(Topics[Need]).X * 0.5f, Box.Y);
			const FVector2D To = BoxPos(T) + FVector2D(TopicBoxSize(T).X * 0.5f, 0.f);
			const float MidY = To.Y - 7.f;
			const FLinearColor C = Map->HasResearch(Topics[Need].Id) ? Gold : MutedInk.CopyWithNewOpacity(0.6f);
			DrawLines(Geometry, Out, Layer + 1, { From, FVector2D(From.X, MidY), FVector2D(To.X, MidY), To }, C, 2.f);
		}
	}
	const FLinearColor Dark = FLinearColor::FromSRGBColor(FColor(30, 22, 12));
	for (int32 t = 0; t < Topics.Num(); ++t)
	{
		if (!InTab(t))
		{
			continue;
		}
		const FCampaign1851ResearchTopic& T = Topics[t];
		const FString Why = Map->ResearchBlockReason(t);
		const bool bDone = Why == TEXT("færdig");
		const bool bBusy = Map->GetResearching(bResearchCivil) == t;
		const FVector2D P = BoxPos(T);
		const FVector2D TopicBox = TopicBoxSize(T);
		PaintButton(Geometry, Out, Layer + 2, P, TopicBox, FString(), EButton::ResearchPick, t, bDone || ResearchPick == t);
		const FLinearColor Main = bDone ? Dark : Why.IsEmpty() || bBusy ? Ink : MutedInk;
		PaintTextFit(Geometry, Out, Layer + 4, T.Name, P + FVector2D(8.f, 13.f), Serif(12), Main, TopicBox.X - 16.f);
		const FString State = bDone ? FString(TEXT("færdig"))
			: bBusy ? FString::Printf(TEXT("i gang  ·  %d af %d md."), Map->GetResearchMonths(bResearchCivil), T.Months)
			: Why.IsEmpty() ? FString::Printf(TEXT("%d md.  ·  %s rd./md."), T.Months, *Thousands(int32(T.CostPerMonth)))
			: Why;
		PaintTextFit(Geometry, Out, Layer + 4, State, P + FVector2D(8.f, 30.f), Serif(10, EFace::Italic), bDone ? Dark : bBusy ? Gold : MutedInk, TopicBox.X - 16.f);
		if (TopicBox.Y > 56.f)
		{
			PaintTextFit(Geometry, Out, Layer + 4, T.Effect, P + FVector2D(8.f, 46.f), Serif(9, EFace::Italic), bDone ? Dark : MutedInk, TopicBox.X - 16.f);
		}
		if (bBusy)
		{
			PaintBar(Geometry, Out, Layer + 4, P + FVector2D(8.f, TopicBox.Y - 9.f), TopicBox.X - 16.f, float(Map->GetResearchMonths(bResearchCivil)) / float(T.Months));
		}
	}
	// The topic clicked: what it does, what it costs, and START.
	if (Topics.IsValidIndex(ResearchPick))
	{
		const FCampaign1851ResearchTopic& T = Topics[ResearchPick];
		const FString Why = Map->ResearchBlockReason(ResearchPick);
		const FVector2D BoxSize(560.f, 280.f);
		const FVector2D BP(X + (LeftW - BoxSize.X) * 0.5f, Pos.Y + Size.Y - BoxSize.Y - 40.f);
		PaintPanel(Geometry, Out, Layer + 6, BP, BoxSize);
		DrawLines(Geometry, Out, Layer + 7, { BP, BP + FVector2D(BoxSize.X, 0.f), BP + BoxSize, BP + FVector2D(0.f, BoxSize.Y), BP }, Gold, 1.5f);
		PaintBranchSymbol(Geometry, Out, Layer + 8, T.Branch, BP + FVector2D(46.f, 50.f), 26.f);
		PaintText(Geometry, Out, Layer + 8, T.Name, BP + FVector2D(90.f, 40.f), Serif(20), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 8, FString::Printf(TEXT("%s  ·  niveau %s"), Campaign1851Research::BranchName(T.Branch), Campaign1851Research::Roman(DisplayTier(ResearchPick))),
			BP + FVector2D(90.f, 66.f), Serif(12, EFace::Italic), Gold, 0.f, false);
		float TY = BP.Y + 104.f;
		// The effect, wrapped.
		TArray<FString> Words;
		FString(T.Effect).ParseIntoArray(Words, TEXT(" "));
		FString LineText;
		for (const FString& Word : Words)
		{
			const FString Try = LineText.IsEmpty() ? Word : LineText + TEXT(" ") + Word;
			if (Measure(Try, Serif(14)).X > BoxSize.X - 48.f && !LineText.IsEmpty())
			{
				PaintText(Geometry, Out, Layer + 8, LineText, FVector2D(BP.X + 24.f, TY), Serif(14), Ink, 0.f, false);
				TY += 22.f;
				LineText = Word;
			}
			else
			{
				LineText = Try;
			}
		}
		PaintText(Geometry, Out, Layer + 8, LineText, FVector2D(BP.X + 24.f, TY), Serif(14), Ink, 0.f, false);
		TY += 34.f;
		const int32 Need = T.Needs ? Campaign1851Research::FindTopic(T.Needs) : INDEX_NONE;
		PaintTextFit(Geometry, Out, Layer + 8, FString::Printf(TEXT("%d måneder  ·  %s rd. om måneden  ·  i alt %s rd."), T.Months, *Thousands(int32(T.CostPerMonth)), *Thousands(int32(T.CostPerMonth * T.Months))),
			FVector2D(BP.X + 24.f, TY), Serif(12), MutedInk, BoxSize.X - 48.f);
		if (Topics.IsValidIndex(Need))
		{
			PaintTextFit(Geometry, Out, Layer + 8, FString::Printf(TEXT("Kræver først: %s"), Topics[Need].Name), FVector2D(BP.X + 24.f, TY + 20.f), Serif(12, EFace::Italic), Gold, BoxSize.X - 48.f);
		}
		const bool bPickCivil = Campaign1851Research::IsCivil(ResearchPick);
		const bool bBusy = Map->GetResearching(bPickCivil) == ResearchPick;
		const FString Label = Why == TEXT("færdig") ? FString(TEXT("FÆRDIG")) : bBusy ? FString::Printf(TEXT("I GANG  ·  %d AF %d MD."), Map->GetResearchMonths(bPickCivil), T.Months)
			: Why.IsEmpty() ? (Map->GetResearching(bPickCivil) != INDEX_NONE ? FString(TEXT("START (AFBRYDER DET NUVÆRENDE)")) : FString(TEXT("START"))) : Why;
		PaintButton(Geometry, Out, Layer + 8, FVector2D(BP.X + 24.f, BP.Y + BoxSize.Y - 54.f), FVector2D(330.f, 32.f), Label, EButton::ResearchStart, ResearchPick, false, !Why.IsEmpty());
		PaintButton(Geometry, Out, Layer + 8, FVector2D(BP.X + BoxSize.X - 144.f, BP.Y + BoxSize.Y - 54.f), FVector2D(120.f, 32.f), TEXT("LUK"), EButton::ResearchPick, -1);
	}
	// The doctrines.
	if (!bDoc)
	{
		return;
	}
	const float RX = X, RW = FMath::Min(900.f, Size.X - 80.f);
	float RY = Pos.Y + 150.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("D O K T R I N E R"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
	RY += 20.f;
	PaintTextFit(Geometry, Out, Layer + 1, Map->IsDoctrineChanging()
		? FString::Printf(TEXT("Hæren omstiller sig til %s (kampkraft −10 %%)"), *ACampaign1851Map::FormatDate(ACampaign1851Map::StartDate() + FTimespan::FromDays(Map->GetDoctrineSettledDay()), true))
		: FString::Printf(TEXT("Et skift koster %s rd., lidt moral og %.0f dages omstilling"), *Thousands(int32(ACampaign1851Map::DoctrineChangeCost)), ACampaign1851Map::DoctrineChangeDays),
		FVector2D(RX, RY + 6.f), Serif(10, EFace::Italic), Map->IsDoctrineChanging() ? Gold : MutedInk, RW);
	RY += 36.f;
	for (int32 l = 0; l < 3; ++l)
	{
		PaintText(Geometry, Out, Layer + 1, Campaign1851Research::LevelName(l), FVector2D(RX, RY), Serif(14), Ink, 0.f, false);
		RY += 20.f;
		const int32 Choices = Campaign1851Research::DoctrineChoices(l);
		const float BW = (RW - 8.f * (Choices - 1)) / Choices;
		for (int32 c = 0; c < Choices; ++c)
		{
			PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + c * (BW + 8.f), RY), FVector2D(BW, 26.f), FString(Campaign1851Research::DoctrineName(l, c)).ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å")),
				EButton::DoctrineSet, l * 10 + c, Map->GetDoctrine(l) == c, Map->IsDoctrineChanging() && Map->GetDoctrine(l) != c);
		}
		RY += 34.f;
		PaintTextFit(Geometry, Out, Layer + 1, Campaign1851Research::DoctrineEffect(l, Map->GetDoctrine(l)), FVector2D(RX, RY + 4.f), Serif(10, EFace::Italic), MutedInk, RW);
		RY += 34.f;
	}
	RY += 10.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("V I R K N I N G   I   S L A G"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
	RY += 26.f;
	auto Line = [&](const FString& Text) { PaintTextFit(Geometry, Out, Layer + 1, Text, FVector2D(RX, RY), Serif(12), Ink, RW); RY += 20.f; };
	Line(FString::Printf(TEXT("Egne tab × %.2f"), Map->DanishLossFactor()));
	Line(FString::Printf(TEXT("Skansernes dækning +%.0f %%-point"), Map->FortCoverBonus()));
	Line(FString::Printf(TEXT("Kanoner × %.2f  ·  infanteri × %.2f"), Map->DanishGunFactor(), Map->InfantryFactor()));
	Line(FString::Printf(TEXT("Proviant båret: %.0f dage  ·  indkaldelse × %.2f"), Map->FoodCap(), Map->CallInFactor()));
}

void SCampaign1851Overlay::PaintNavy(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	PaintText(Geometry, Out, Layer + 1, TEXT("Flåden"), Pos + FVector2D(24.f, 34.f), Serif(22), Ink, 0.f);
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Så længe flåden er fjenden overlegen, kan hans korps ikke gå over Bælterne; en blokade koster ham forstærkninger og fremskynder freden"),
		Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Size.X - 170.f);
	const float X = Pos.X + 24.f;
	float Y = Pos.Y + 106.f;
	const float LeftW = 640.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("S K I B E N E"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 24.f;
	const TArray<FCampaign1851ShipClass>& Classes = Campaign1851Navy::Classes();
	const TArray<FCampaign1851Ship>& Ships = Map->GetShips();
	const double Today = (Map->GetDate() - ACampaign1851Map::StartDate()).GetTotalDays();
	for (const FCampaign1851Ship& S : Ships)
	{
		if (Y > Pos.Y + Size.Y - 40.f)
		{
			PaintText(Geometry, Out, Layer + 1, TEXT("..."), FVector2D(X, Y), Serif(12), MutedInk, 0.f, false);
			break;
		}
		const bool bReady = Today >= S.ReadyDay;
		PaintText(Geometry, Out, Layer + 1, S.Name, FVector2D(X, Y), Serif(13), bReady ? Ink : MutedInk, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Classes.IsValidIndex(S.Class) ? Classes[S.Class].Name : TEXT(""), FVector2D(X + 230.f, Y), Serif(12), MutedInk, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, bReady ? FString::Printf(TEXT("fra %d"), S.Built)
			: FString::Printf(TEXT("klar %s"), *ACampaign1851Map::FormatDate(ACampaign1851Map::StartDate() + FTimespan::FromDays(S.ReadyDay), true)),
			FVector2D(X + 400.f, Y), Serif(12), bReady ? MutedInk : Gold, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Classes.IsValidIndex(S.Class) ? FString::Printf(TEXT("%.0f"), Classes[S.Class].Strength) : FString(), FVector2D(X + LeftW - 20.f, Y), Serif(12), Ink, 1.f, false);
		Y += 22.f;
	}
	// Right: the balance at sea, the stance and the dockyard.
	const float RX = X + LeftW + 30.f, RW = Size.X - LeftW - 90.f;
	float RY = Pos.Y + 106.f;
	PaintPanel(Geometry, Out, Layer, FVector2D(RX - 12.f, Pos.Y + 86.f), FVector2D(RW + 24.f, Size.Y - 100.f));
	PaintText(Geometry, Out, Layer + 1, TEXT("H E R R E D Ø M M E T   T I L   S Ø S"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
	RY += 28.f;
	const float Dk = Map->DanishSeaStrength(), En = Map->EnemySeaStrength();
	PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Danmark %.0f  ·  fjenden %.0f"), Dk, En), FVector2D(RX, RY), Serif(14), Ink, 0.f, false);
	PaintText(Geometry, Out, Layer + 1, !Map->IsAtWar() ? TEXT("fred: farvandene er åbne") : Map->HasSeaControl() ? TEXT("flåden behersker farvandene") : TEXT("fjenden er overlegen til søs"),
		FVector2D(RX, RY + 20.f), Serif(11, EFace::Italic), Map->HasSeaControl() || !Map->IsAtWar() ? Gold : FLinearColor(0.95f, 0.4f, 0.35f), 0.f, false);
	PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Vedligehold %s rd. om året"), *Thousands(int32(Map->NavyUpkeepPerYear()))), FVector2D(RX, RY + 42.f), Serif(12), Ink, 0.f, false);
	RY += 76.f;
	PaintButton(Geometry, Out, Layer + 1, FVector2D(RX, RY), FVector2D(RW * 0.5f - 4.f, 26.f), TEXT("FORSVAR"), EButton::Blockade, 0, !Map->IsBlockade());
	PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + RW * 0.5f + 4.f, RY), FVector2D(RW * 0.5f - 4.f, 26.f), TEXT("BLOKADE"), EButton::Blockade, 1, Map->IsBlockade());
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Blokade: styrken tæller 20 % mindre, men i krig prisepenge, halve forstærkninger til fjenden og konference efter 45 dage; Storbritannien bryder sig ikke om den"),
		FVector2D(RX, RY + 40.f), Serif(10, EFace::Italic), MutedInk, RW);
	RY += 80.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("O R L O G S V Æ R F T E T"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
	RY += 28.f;
	for (int32 c = 0; c < Classes.Num(); ++c)
	{
		const FCampaign1851ShipClass& K = Classes[c];
		if (K.Cost <= 0.0)
		{
			continue;
		}
		const FString Why = Map->ShipBlockReason(c);
		PaintText(Geometry, Out, Layer + 1, K.Name, FVector2D(RX, RY), Serif(13), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("styrke %.0f  ·  %d md.  ·  %s rd."), K.Strength, K.Months, *Thousands(int32(K.Cost))), FVector2D(RX, RY + 17.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + RW - 140.f, RY - 6.f), FVector2D(140.f, 24.f), Why.IsEmpty() ? FString(TEXT("BESTIL")) : Why, EButton::ShipOrder, c, false, !Why.IsEmpty());
		RY += 42.f;
	}
}

namespace
{
	struct FLexiconEntry { const TCHAR* Title; const TCHAR* Text; };
	const FLexiconEntry Lexicon[] = {
		{ TEXT("Tiden"), TEXT("Kampagnen løber fra 1. juli 1851. Fart 1-5 lader timerne gå, fart 6 tager én dag pr. slag. Ved hvert månedsskifte lukkes regnskabet: skatter ind, drift og lønninger ud, og ministerierne træffer beslutninger. Vejret følger årstiden og kampagnens seed.") },
		{ TEXT("Byer og byggeri"), TEXT("Klik på en by for at bygge garnisoner, depoter, lazaretter og civile bygninger. Civile bygninger får byen og amtet til at vokse og giver told og afgifter. Byggeri koster dagløn hver dag og går langsommere i frost. Nedrevne bygninger og skanser giver materialer til genbrug.") },
		{ TEXT("Hæren"), TEXT("Bataljoner, regimenter og batterier har mand, heste, kanoner, erfaring, træning, moral og samhørighed. I fred er de fleste hjemsendt; mobilisering kalder dem ind over et par uger. Nye bataljoner koster rekrutter fra amterne og geværer fra lageret. Syge og sårede ligger på lazaret og kommer tilbage over uger.") },
		{ TEXT("Officerer"), TEXT("Hver enhed har en chef, hver formation en general med næstkommanderende og stabschef. Officerernes evner (føring, taktik, stab, initiativ m.fl.) virker på enhederne. Hvert nytår går de gamle på pension, nogle dør, og officersskolen udnævner nye kaptajner.") },
		{ TEXT("Forsyning"), TEXT("Enhederne bærer 4 dages proviant (6 med konserves). I garnison er de fuldt forsynede; i felten spiser de af depoter inden for en dagsmarch, køber i byerne eller får trænkolonner. Uden proviant falder kampkraften, og flere bliver syge.") },
		{ TEXT("Skanser"), TEXT("Skanser bygges med SKANSER-knappen, forsynes med kanoner og besættes af kompagnier. Dækningen ganger forsvarernes styrke; løbegrave forbinder skanser og dækker reserven. Skanser kan rives ned; kanonerne går på lager.") },
		{ TEXT("Krig og slag"), TEXT("Spændingen med Det tyske forbund stiger med de historiske begivenheder (varieret af seed). Ved 80 kommer krigen. Fjendens korps ses kun, når de er meldt. Når et korps møder danske tropper eller skanser, kan slaget udkæmpes i 3D, afgøres automatisk eller undgås ved tilbagetog.") },
		{ TEXT("Flåden"), TEXT("Så længe flåden er fjenden overlegen, kan hans korps ikke gå over Bælterne; smalle sunde kræver både, og i en isvinter kan de gås over. En blokade koster fjenden forstærkninger og bringer freden nærmere, men irriterer Storbritannien.") },
		{ TEXT("Udenrigs og fred"), TEXT("Gesandter forbedrer forholdet, handelstraktater giver told og vækst, Sverige-Norge kan blive allieret, og stormagterne kan garantere helstaten. I krig kan freden sluttes på fire vilkår; fjenden tager kun imod, hvad krigsstillingen retfærdiggør.") },
		{ TEXT("Forskning og doktrin"), TEXT("Krigsministeriet udvikler ét projekt ad gangen (fra sanitetsvæsenet 1852 til bagladegeværet 1860). Doktrinen på tre niveauer bestemmer, hvordan hæren kæmper; et skift tager 60 dage.") },
		{ TEXT("Politik og stemning"), TEXT("Helstaten, Ejderpolitikken og skandinavismen deler opinionen. Regeringen følger den førende strømning, og stemningen i landet virker på skatterne og indkaldelsen. Sejre, gode kornpriser og fred løfter stemningen; mobilisering, tab og besatte byer trykker den.") },
		{ TEXT("Økonomi"), TEXT("Skatterne kommer fra amterne. Korn, kvæg og smør eksporteres med 2 % told; priserne svinger med høsten, Krimkrigen og krisen i 1857. Staten kan låne i London og Hamborg; renten stiger med gælden, krig og utilfredshed.") },
	};
}

void SCampaign1851Overlay::PaintGazette(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const FDateTime Now = Map->GetDate();
	PaintText(Geometry, Out, Layer + 1, TEXT("Berlingske Tidende"), Pos + FVector2D(Size.X * 0.5f, 40.f), Serif(28), Ink, 0.5f);
	PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%s  ·  kampagnens avis, marked, statistik og opslagsværk"), *ACampaign1851Map::FormatDate(Now)), Pos + FVector2D(Size.X * 0.5f, 72.f),
		Serif(12, EFace::Italic), Gold, 0.5f);
	const TCHAR* Tabs[] = { TEXT("AVISEN"), TEXT("MARKEDET"), TEXT("STATISTIK"), TEXT("OPSLAG") };
	for (int32 t = 0; t < 4; ++t)
	{
		PaintButton(Geometry, Out, Layer + 1, FVector2D(Pos.X + 24.f + t * 150.f, Pos.Y + 92.f), FVector2D(142.f, 26.f), Tabs[t], EButton::GazetteTab, t, GazetteTab == t);
	}
	const float X = Pos.X + 24.f, W = Size.X - 48.f;
	float Y = Pos.Y + 144.f;
	const float Bottom = Pos.Y + Size.Y - 24.f;
	if (GazetteTab == 0)
	{
		// The news, newest first, by the day.
		const TArray<TPair<double, FString>>& Log = Map->GetNewsLog();
		if (Log.Num() == 0)
		{
			PaintText(Geometry, Out, Layer + 1, TEXT("Endnu intet at berette."), FVector2D(X, Y), Serif(14, EFace::Italic), MutedInk, 0.f, false);
		}
		const float ColW = (W - 30.f) * 0.5f;
		int32 Column = 0;
		double LastDay = -1.0;
		for (int32 n = Log.Num() - 1; n >= 0 && Column < 2; --n)
		{
			const int32 Day = FMath::FloorToInt(Log[n].Key);
			const float CX = X + Column * (ColW + 30.f);
			if (Day != FMath::FloorToInt(LastDay))
			{
				LastDay = Log[n].Key;
				PaintText(Geometry, Out, Layer + 1, ACampaign1851Map::FormatDate(ACampaign1851Map::StartDate() + FTimespan::FromDays(Day)), FVector2D(CX, Y + 4.f), Serif(11), Gold, 0.f, false);
				Y += 22.f;
			}
			const bool bHeadline = n == Log.Num() - 1;
			PaintTextFit(Geometry, Out, Layer + 1, Log[n].Value, FVector2D(CX, Y), bHeadline ? Serif(17) : Serif(13), Ink, ColW);
			Y += bHeadline ? 30.f : 22.f;
			if (Y > Bottom - 20.f)
			{
				++Column;
				Y = Pos.Y + 144.f;
				LastDay = -1.0;   // the date again at the top of the next column
			}
		}
	}
	else if (GazetteTab == 1)
	{
		PaintText(Geometry, Out, Layer + 1, TEXT("U D F Ø R S L E N"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
		Y += 28.f;
		const TCHAR* Heads[] = { TEXT("Vare"), TEXT("Prisindeks (1851 = 1,00)"), TEXT("Udførsel rd./år"), TEXT("Told rd./år") };
		const float Cols[] = { 0.f, 180.f, 420.f, 620.f };
		for (int32 c = 0; c < 4; ++c)
		{
			PaintText(Geometry, Out, Layer + 1, Heads[c], FVector2D(X + Cols[c], Y), Serif(11, EFace::Italic), MutedInk, 0.f, false);
		}
		Y += 24.f;
		for (int32 g = 0; g < int32(ECampaign1851Good::Count); ++g)
		{
			const ECampaign1851Good G = ECampaign1851Good(g);
			PaintText(Geometry, Out, Layer + 1, Campaign1851Economy::GoodName(G), FVector2D(X, Y), Serif(14), Ink, 0.f, false);
			PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%.2f"), Map->PriceIndex(G)), FVector2D(X + Cols[1], Y), Serif(14), Map->PriceIndex(G) >= 1.f ? Ink : CityRed, 0.f, false);
			PaintText(Geometry, Out, Layer + 1, Thousands(FMath::RoundToInt(Map->ExportValuePerYear(G))), FVector2D(X + Cols[2], Y), Serif(14), Ink, 0.f, false);
			PaintText(Geometry, Out, Layer + 1, Thousands(FMath::RoundToInt(Map->ExportValuePerYear(G) * 0.02)), FVector2D(X + Cols[3], Y), Serif(14), Ink, 0.f, false);
			Y += 26.f;
		}
		Y += 20.f;
		PaintTextFit(Geometry, Out, Layer + 1, TEXT("Priserne følger høsten og tiderne: Krimkrigen (1853-56) gør kornet dyrt, krisen i 1857 knækker priserne, og en krig lukker Hamborg for kvæget. Gode priser løfter stemningen på landet."),
			FVector2D(X, Y), Serif(12, EFace::Italic), MutedInk, W);
		Y += 40.f;
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Statsgæld %s rd. til %.1f %%  ·  nyt lån til %.1f %%  (lån og afdrag i STATSKASSEN)"), *Thousands(FMath::RoundToInt(Map->GetDebt())), Map->GetDebtRate() * 100.f, Map->CreditRate() * 100.f),
			FVector2D(X, Y), Serif(13), Ink, 0.f, false);
	}
	else if (GazetteTab == 2)
	{
		// Six small charts of the monthly record.
		const TArray<FCampaign1851Record>& H = Map->GetHistory();
		struct FSeries { const TCHAR* Name; TFunction<double(const FCampaign1851Record&)> Get; };
		const FSeries Series[] = {
			{ TEXT("Befolkning"), [](const FCampaign1851Record& R) { return R.Population; } },
			{ TEXT("Statskassen (rd.)"), [](const FCampaign1851Record& R) { return R.Treasury; } },
			{ TEXT("Hæren (mand)"), [](const FCampaign1851Record& R) { return R.ArmyMen; } },
			{ TEXT("Jernbane (km)"), [](const FCampaign1851Record& R) { return R.RailKm; } },
			{ TEXT("Spænding og stemning"), [](const FCampaign1851Record& R) { return double(R.Tension); } },
			{ TEXT("Kornprisindeks"), [](const FCampaign1851Record& R) { return double(R.Grain); } },
		};
		if (H.Num() < 2)
		{
			PaintText(Geometry, Out, Layer + 1, TEXT("Statistikken føres ved hvert månedsskifte; kom tilbage om et par måneder."), FVector2D(X, Y), Serif(14, EFace::Italic), MutedInk, 0.f, false);
			return;
		}
		const float CW = (W - 40.f) / 3.f, CH = (Bottom - Y - 40.f) / 2.f;
		for (int32 s = 0; s < 6; ++s)
		{
			const FVector2D C0(X + (s % 3) * (CW + 20.f), Y + (s / 3) * (CH + 30.f));
			double Lo = TNumericLimits<double>::Max(), Hi = TNumericLimits<double>::Lowest();
			for (const FCampaign1851Record& R : H)
			{
				Lo = FMath::Min(Lo, Series[s].Get(R));
				Hi = FMath::Max(Hi, Series[s].Get(R));
			}
			if (s == 4)
			{
				Lo = 0.0;
				Hi = 100.0;
			}
			if (Hi - Lo < 1e-6)
			{
				Hi = Lo + 1.0;
			}
			PaintText(Geometry, Out, Layer + 1, Series[s].Name, C0, Serif(13), Ink, 0.f, false);
			PaintText(Geometry, Out, Layer + 1, s == 5 ? FString::Printf(TEXT("%.2f"), Series[s].Get(H.Last())) : Thousands(FMath::RoundToInt(Series[s].Get(H.Last()))), C0 + FVector2D(CW, 0.f), Serif(13), Gold, 1.f, false);
			const FVector2D Box(C0.X, C0.Y + 18.f);
			const FVector2D BoxSize(CW, CH - 24.f);
			DrawLines(Geometry, Out, Layer + 1, { Box, FVector2D(Box.X, Box.Y + BoxSize.Y), Box + BoxSize }, Gold.CopyWithNewOpacity(0.4f), 1.f);
			auto Plot = [&](TFunctionRef<double(const FCampaign1851Record&)> Get, const FLinearColor& Colour)
			{
				TArray<FVector2D> Points;
				for (int32 i = 0; i < H.Num(); ++i)
				{
					const float PX = Box.X + BoxSize.X * i / float(H.Num() - 1);
					const float PY = Box.Y + BoxSize.Y * (1.f - float((Get(H[i]) - Lo) / (Hi - Lo)));
					Points.Add(FVector2D(PX, PY));
				}
				DrawLines(Geometry, Out, Layer + 2, Points, Colour, 2.f);
			};
			Plot(Series[s].Get, Ink);
			if (s == 4)
			{
				Plot([](const FCampaign1851Record& R) { return double(R.Mood); }, Gold);
				PaintText(Geometry, Out, Layer + 1, TEXT("spænding (hvid), stemning (guld)"), Box + FVector2D(6.f, 12.f), Serif(9, EFace::Italic), MutedInk, 0.f, false);
			}
			const FDateTime First = ACampaign1851Map::StartDate() + FTimespan::FromDays(H[0].Day);
			const FDateTime Last = ACampaign1851Map::StartDate() + FTimespan::FromDays(H.Last().Day);
			PaintText(Geometry, Out, Layer + 1, ACampaign1851Map::FormatDate(First, true), Box + FVector2D(0.f, BoxSize.Y + 12.f), Serif(9), MutedInk, 0.f, false);
			PaintText(Geometry, Out, Layer + 1, ACampaign1851Map::FormatDate(Last, true), Box + FVector2D(BoxSize.X, BoxSize.Y + 12.f), Serif(9), MutedInk, 1.f, false);
		}
	}
	else
	{
		// The reference book: the entries on the left, the text on the right.
		const int32 Count = UE_ARRAY_COUNT(Lexicon);
		for (int32 e = 0; e < Count; ++e)
		{
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X, Y + e * 32.f), FVector2D(240.f, 26.f), Lexicon[e].Title, EButton::GazetteTab, 100 + e, LexiconEntry == e);
		}
		const int32 E = FMath::Clamp(LexiconEntry, 0, Count - 1);
		PaintText(Geometry, Out, Layer + 1, Lexicon[E].Title, FVector2D(X + 280.f, Y + 4.f), Serif(20), Ink, 0.f, false);
		// Wrap the text by words.
		TArray<FString> Words;
		FString(Lexicon[E].Text).ParseIntoArray(Words, TEXT(" "));
		const FSlateFontInfo Font = Serif(14);
		const float TW = W - 300.f;
		FString LineText;
		float LY = Y + 44.f;
		for (const FString& Word : Words)
		{
			const FString Try = LineText.IsEmpty() ? Word : LineText + TEXT(" ") + Word;
			if (Measure(Try, Font).X > TW && !LineText.IsEmpty())
			{
				PaintText(Geometry, Out, Layer + 1, LineText, FVector2D(X + 280.f, LY), Font, Ink, 0.f, false);
				LY += 24.f;
				LineText = Word;
			}
			else
			{
				LineText = Try;
			}
		}
		PaintText(Geometry, Out, Layer + 1, LineText, FVector2D(X + 280.f, LY), Font, Ink, 0.f, false);
	}
}

void SCampaign1851Overlay::PaintEnd(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const TArray<FCampaign1851ScoreLine> Lines = Map->FinalScore();
	float Total = 0.f;
	for (const FCampaign1851ScoreLine& L : Lines)
	{
		Total += L.Points;
	}
	PaintText(Geometry, Out, Layer + 1, TEXT("Kampagnens udfald"), Pos + FVector2D(Size.X * 0.5f, 60.f), Serif(30), Ink, 0.5f);
	PaintText(Geometry, Out, Layer + 1, ACampaign1851Map::FormatDate(Map->GetDate()), Pos + FVector2D(Size.X * 0.5f, 100.f), Serif(14, EFace::Italic), Gold, 0.5f);
	float Y = Pos.Y + 170.f;
	const float X = Pos.X + Size.X * 0.5f - 360.f;
	for (const FCampaign1851ScoreLine& L : Lines)
	{
		PaintText(Geometry, Out, Layer + 1, L.Text, FVector2D(X, Y), Serif(16), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%+.0f"), L.Points), FVector2D(X + 720.f, Y), Serif(16), L.Points >= 0.f ? Gold : CityRed, 1.f, false);
		Y += 34.f;
	}
	DrawLines(Geometry, Out, Layer + 1, { FVector2D(X, Y), FVector2D(X + 720.f, Y) }, Gold.CopyWithNewOpacity(0.6f), 1.f);
	Y += 20.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("I alt"), FVector2D(X, Y), Serif(20), Ink, 0.f, false);
	PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%.0f"), Total), FVector2D(X + 720.f, Y), Serif(20), Gold, 1.f, false);
	Y += 60.f;
	PaintText(Geometry, Out, Layer + 1, ACampaign1851Map::FinalGrade(Total), FVector2D(Pos.X + Size.X * 0.5f, Y), Serif(22, EFace::Italic), Gold, 0.5f);
	PaintText(Geometry, Out, Layer + 1, TEXT("Luk vinduet for at spille videre, eller start et nyt spil i spilmenuen (M)."), FVector2D(Pos.X + Size.X * 0.5f, Y + 50.f), Serif(12, EFace::Italic), MutedInk, 0.5f);
}

void SCampaign1851Overlay::PaintMateriel(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	PaintText(Geometry, Out, Layer + 1, TEXT("Materiel og nye enheder"), Pos + FVector2D(24.f, 34.f), Serif(22), Ink, 0.f);
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Råmaterialer til værkerne, udstyret på lager, og indkaldelse af nye enheder: type, garnison, kommando og øvelser efter eget valg"),
		Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Size.X - 170.f);
	const float X = Pos.X + 24.f;
	float Y = Pos.Y + 110.f;
	const float LeftW = 760.f;
	PaintPanel(Geometry, Out, Layer, Pos + FVector2D(12.f, 86.f), FVector2D(LeftW + 12.f, Size.Y - 100.f));
	// Raw materials.
	PaintText(Geometry, Out, Layer + 1, TEXT("R Å M A T E R I A L E R"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 26.f;
	const TCHAR* Heads[] = { TEXT("Materiale"), TEXT("Lager"), TEXT("+ md."), TEXT("− md."), TEXT("Pris") };
	const float Cols[] = { 0.f, 170.f, 270.f, 350.f, 430.f };
	for (int32 c = 0; c < 5; ++c)
	{
		PaintText(Geometry, Out, Layer + 1, Heads[c], FVector2D(X + Cols[c], Y), Serif(10, EFace::Italic), MutedInk, 0.f, false);
	}
	Y += 24.f;
	float Made[int32(ECampaign1851Raw::Count)], Used[int32(ECampaign1851Raw::Count)];
	Map->RawFlow(Made, Used);
	FString NoImport;
	const bool bImport = Map->CanImport(&NoImport);
	for (int32 r = 0; r < int32(ECampaign1851Raw::Count); ++r)
	{
		const ECampaign1851Raw R = ECampaign1851Raw(r);
		const Campaign1851Resources::FRawInfo& I = Campaign1851Resources::Info(R);
		const bool bShort = Used[r] > Made[r] && Map->GetRaw(R) < Used[r];
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%s (%s)"), I.Name, I.Unit), FVector2D(X, Y), Serif(13), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Thousands(FMath::RoundToInt(Map->GetRaw(R))), FVector2D(X + Cols[1], Y), Serif(13), bShort ? CityRed : Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Thousands(FMath::RoundToInt(Made[r])), FVector2D(X + Cols[2], Y), Serif(12), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Thousands(FMath::RoundToInt(Used[r])), FVector2D(X + Cols[3], Y), Serif(12), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%.0f rd."), Map->RawPrice(R)), FVector2D(X + Cols[4], Y), Serif(12), MutedInk, 0.f, false);
		const float Step = R == ECampaign1851Raw::Cloth || R == ECampaign1851Raw::Leather ? 100.f : 10.f;
		for (int32 b = 0; b < 2; ++b)
		{
			const float Amount = Step * (b == 0 ? 1.f : 10.f);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 520.f + b * 118.f, Y - 11.f), FVector2D(112.f, 22.f), FString::Printf(TEXT("KØB %s"), *Thousands(FMath::RoundToInt(Amount))),
				EButton::RawBuy, r * 10 + b, false, !bImport || Map->GetTreasury() < Map->RawPrice(R) * Amount);
		}
		Y += 26.f;
	}
	PaintTextFit(Geometry, Out, Layer + 1, bImport ? FString(TEXT("Jern og kul kommer mest fra udlandet; savværker, kulminen, klædefabrikker, garverier og krudtværket laver resten. Intendanturen på AUTO køber selv.")) : NoImport,
		FVector2D(X, Y + 4.f), Serif(10, EFace::Italic), bImport ? MutedInk : CityRed, LeftW - 20.f);
	Y += 36.f;
	// Equipment in store.
	PaintText(Geometry, Out, Layer + 1, TEXT("U D S T Y R   P Å   L A G E R"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 26.f;
	const FString Kit[] = {
		FString::Printf(TEXT("Geværer %s"), *Thousands(Map->GetRifles())), FString::Printf(TEXT("Kanoner %d"), Map->GetGunStock()), FString::Printf(TEXT("Heste %s"), *Thousands(Map->GetHorseStock())),
		FString::Printf(TEXT("Uniformer %s"), *Thousands(FMath::RoundToInt(Map->GetRaw(ECampaign1851Raw::Cloth)))), FString::Printf(TEXT("Remtøj og sadler %s"), *Thousands(FMath::RoundToInt(Map->GetRaw(ECampaign1851Raw::Leather)))),
		FString::Printf(TEXT("Krudt %s tønder"), *Thousands(FMath::RoundToInt(Map->GetRaw(ECampaign1851Raw::Powder)))) };
	for (int32 k = 0; k < 6; ++k)
	{
		PaintText(Geometry, Out, Layer + 1, Kit[k], FVector2D(X + (k % 3) * 250.f, Y + (k / 3) * 24.f), Serif(13), Ink, 0.f, false);
	}
	Y += 58.f;
	PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Morterer %d  ·  vogne %d"), Map->GetMortarStock(), Map->GetWagonStock()), FVector2D(X, Y), Serif(13), Ink, 0.f, false);
	PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 250.f, Y - 12.f), FVector2D(220.f, 24.f), FString::Printf(TEXT("KØB 2 MORTERER (%s rd.)"), *Thousands(int32(2 * Campaign1851Resources::MortarPrice))),
		EButton::KitBuy, 1, false, !bImport);
	PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 480.f, Y - 12.f), FVector2D(200.f, 24.f), FString::Printf(TEXT("KØB 10 VOGNE (%s rd.)"), *Thousands(int32(10 * Campaign1851Resources::WagonPrice))),
		EButton::KitBuy, 0);
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Morterer til belejring og skanser; hver morter kører på to vogne. Arsenalet støber én om måneden, vognfabrikken bygger 20 vogne."),
		FVector2D(X, Y + 22.f), Serif(10, EFace::Italic), MutedInk, LeftW - 20.f);
	// Initial raising/training queue, below the stock panel.
	Y += 70.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("INDKALDELSE OG GRUNDUDDANNELSE"), FVector2D(X, Y), Serif(12), Gold, 0.f, false);
	Y += 28.f;
	TArray<int32> Raising;
	const TArray<FCampaign1851Regiment>& AllUnits = Map->GetRegiments();
	for (int32 i = 0; i < AllUnits.Num(); ++i) { if (AllUnits[i].bTraining) { Raising.Add(i); } }
	const int32 PageRows = FMath::Max(1, FMath::FloorToInt((Pos.Y + Size.Y - Y - 60.f) / 54.f));
	const int32 Pages = FMath::Max(1, FMath::DivideAndRoundUp(Raising.Num(), PageRows));
	RaisingPageIndex = FMath::Clamp(RaisingPageIndex, 0, Pages - 1);
	if (Raising.IsEmpty()) { PaintText(Geometry, Out, Layer + 1, TEXT("Ingen enheder under indkaldelse."), FVector2D(X, Y), Serif(12, EFace::Italic), MutedInk, 0.f, false); }
	for (int32 Row = RaisingPageIndex * PageRows; Row < FMath::Min(Raising.Num(), (RaisingPageIndex + 1) * PageRows); ++Row)
	{
		const int32 Index = Raising[Row];
		const FCampaign1851Regiment& Unit = AllUnits[Index];
		PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("%s  |  %s  |  %s"), *Unit.Name, Campaign1851Resources::Type(Unit.RaisingType).Name,
			Map->GetCities().IsValidIndex(Unit.Home) ? *Map->GetCities()[Unit.Home].Name : TEXT("?")), FVector2D(X, Y), Serif(12), Ink, LeftW - 180.f);
		PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("Uddannet %.0f %%  |  %s  |  %s"), Unit.RaisingProgress * 100.f,
			Unit.RaisingDaysLeft() >= 0 ? *FString::Printf(TEXT("%d dage tilbage"), Unit.RaisingDaysLeft()) : TEXT("pause"), Campaign1851Army::ProgramName(Unit.Program)), FVector2D(X, Y + 22.f), Serif(11), MutedInk, LeftW - 180.f);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(X + LeftW - 174.f, Y - 8.f), FVector2D(164.f, 28.f), TEXT("INDSÆT TIDLIGT"), EButton::UnitDeployEarly, Index);
		Y += 54.f;
	}
	if (Pages > 1)
	{
		PaintButton(Geometry, Out, Layer + 1, FVector2D(X, Y + 4.f), FVector2D(44.f, 24.f), TEXT("<"), EButton::RaisingPage, -1, false, RaisingPageIndex == 0);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Side %d / %d"), RaisingPageIndex + 1, Pages), FVector2D(X + 58.f, Y + 16.f), Serif(11), Ink, 0.f, false);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 180.f, Y + 4.f), FVector2D(44.f, 24.f), TEXT(">"), EButton::RaisingPage, 1, false, RaisingPageIndex + 1 == Pages);
	}
	// ---- New unit.
	const float RX = X + LeftW + 30.f, RW = Size.X - LeftW - 90.f;
	float RY = Pos.Y + 110.f;
	PaintPanel(Geometry, Out, Layer, FVector2D(RX - 12.f, Pos.Y + 86.f), FVector2D(RW + 24.f, Size.Y - 100.f));
	PaintText(Geometry, Out, Layer + 1, TEXT("I N D K A L D   E N   N Y   E N H E D"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
	RY += 22.f;
	const float TW = (RW - 5.f * 6.f) / 6.f;
	for (int32 t = 0; t < Campaign1851Resources::UnitTypes; ++t)
	{
		const TCHAR* Short[] = { TEXT("BATAILLON"), TEXT("JÆGERE"), TEXT("DRAGONER"), TEXT("BATTERI"), TEXT("RID. BATTERI"), TEXT("MORTERER") };
		PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + t * (TW + 6.f), RY), FVector2D(TW, 28.f), Short[t], EButton::UnitType, t, RaiseType == t);
	}
	RY += 40.f;
	const Campaign1851Resources::FUnitType Preview = Campaign1851Resources::SizedType(RaiseType, RaiseSize);
	const int32 PictureArms[] = { 0, 2, 3, 4, 5, 4 };
	const int32 PictureArm = PictureArms[FMath::Clamp(RaiseType, 0, 5)];
	const FVector2D Picture(RX, RY);
	if (Map->ActiveScenario().Id != TEXT("1825") && UniformBrushes.IsValidIndex(PictureArm) && UniformBrushes[PictureArm]->GetResourceObject())
	{
		FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(FVector2D(68.f, 102.f), FSlateLayoutTransform(Picture)), UniformBrushes[PictureArm].Get(), ESlateDrawEffect::None, FLinearColor::White);
	}
	else
	{
		// Clean Slate silhouette: rifleman, mounted trooper, or gun/mortar crew.
		const FVector2D Body = Picture + FVector2D(30.f, 40.f);
		PaintDot(Geometry, Out, Layer + 1, Body - FVector2D(0.f, 22.f), 12.f, Ink);
		DrawLines(Geometry, Out, Layer + 1, { Body, Body + FVector2D(0.f, 30.f) }, Ink, 12.f);
		DrawLines(Geometry, Out, Layer + 1, { Body + FVector2D(-18.f, 18.f), Body, Body + FVector2D(18.f, 18.f) }, Ink, 5.f);
		DrawLines(Geometry, Out, Layer + 1, { Body + FVector2D(-12.f, 58.f), Body + FVector2D(0.f, 30.f), Body + FVector2D(12.f, 58.f) }, Ink, 6.f);
		if (RaiseType <= 1) { DrawLines(Geometry, Out, Layer + 1, { Body + FVector2D(22.f, -25.f), Body + FVector2D(22.f, 55.f) }, Ink, 3.f); }
		else if (RaiseType == 2 || RaiseType == 4)
		{
			DrawLines(Geometry, Out, Layer + 1, { Body + FVector2D(-20.f, 36.f), Body + FVector2D(25.f, 36.f), Body + FVector2D(28.f, 14.f) }, Ink, 10.f);
		}
		else
		{
			PaintDot(Geometry, Out, Layer + 1, Body + FVector2D(18.f, 48.f), 20.f, Ink);
			DrawLines(Geometry, Out, Layer + 1, { Body + FVector2D(-8.f, 36.f), Body + FVector2D(35.f, RaiseType == 5 ? 8.f : 30.f) }, Ink, 8.f);
		}
	}
	const TCHAR* Roles[] = { TEXT("Linjeinfanteri: holder linjen og stormer."), TEXT("Jægere: spredt orden og skærmydsler."), TEXT("Dragoner: opklaring og flankeangreb."), TEXT("Fodartilleri: ildstøtte på afstand."), TEXT("Ridende artilleri: hurtig ildstøtte."), TEXT("Morterer: belejring og skanser.") };
	const ECampaign1851Program PreviewProgram = ECampaign1851Program(FMath::Clamp(RaiseProgram, 0, int32(ECampaign1851Program::Count) - 1));
	FCampaign1851Regiment PreviewUnit;
	PreviewUnit.Program = PreviewProgram;
	PaintTextFit(Geometry, Out, Layer + 1, Preview.Name, Picture + FVector2D(90.f, 12.f), Serif(16), Ink, RW - 96.f);
	PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("%d mand  |  %d kanoner  |  %d morterer  |  %d heste"), Preview.Men, Preview.Guns, Preview.Mortars, Preview.Horses), Picture + FVector2D(90.f, 36.f), Serif(12), Ink, RW - 96.f);
	PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("%s rd.  |  Grunduddannelse: %s"), *Thousands(int32(Map->UnitCost(RaiseType, RaiseSize))), PreviewUnit.RaisingRate() > 0.f ? *FString::Printf(TEXT("%d dage"), PreviewUnit.RaisingDaysLeft()) : TEXT("pause (hvile)")), Picture + FVector2D(90.f, 60.f), Serif(12), Gold, RW - 96.f);
	PaintTextFit(Geometry, Out, Layer + 1, Roles[FMath::Clamp(RaiseType, 0, 5)], Picture + FVector2D(90.f, 84.f), Serif(12, EFace::Italic), MutedInk, RW - 96.f);
	RY += 124.f;
	const TArray<int32> Towns = Map->RaiseTowns();
	const int32 Town = Towns.Num() > 0 ? Towns[((RaiseTownPick % Towns.Num()) + Towns.Num()) % Towns.Num()] : INDEX_NONE;
	const TArray<FCampaign1851Command>& Commands = Map->GetCommands();
	const int32 Command = Commands.Num() > 0 ? ((RaiseCommand % Commands.Num()) + Commands.Num()) % Commands.Num() : INDEX_NONE;
	auto Chooser = [&](const TCHAR* Label, const FString& Value, EButton Action)
	{
		PaintText(Geometry, Out, Layer + 1, Label, FVector2D(RX, RY), Serif(12, EFace::Italic), Gold, 0.f, false);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + 150.f, RY - 12.f), FVector2D(34.f, 24.f), TEXT("<"), Action, -1);
		PaintTextFit(Geometry, Out, Layer + 1, Value, FVector2D(RX + 196.f, RY), Serif(14), Ink, RW - 250.f);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + RW - 34.f, RY - 12.f), FVector2D(34.f, 24.f), TEXT(">"), Action, 1);
		RY += 36.f;
	};
	Chooser(TEXT("Størrelse"), FString::Printf(TEXT("%s | %d mand | %d heste | %d skyts"), *Campaign1851Resources::RaiseSizeName(RaiseType, RaiseSize), Preview.Men, Preview.Horses, Preview.Guns + Preview.Mortars), EButton::UnitSize);
	Chooser(TEXT("Garnison"), Town != INDEX_NONE ? Map->GetCities()[Town].Name : FString(TEXT("ingen by med kaserne")), EButton::UnitTown);
	Chooser(TEXT("Hører under"), Command != INDEX_NONE ? Commands[Command].Name : FString(TEXT("-")), EButton::UnitCommand);
	Chooser(TEXT("Øvelser"), Campaign1851Army::ProgramName(ECampaign1851Program(FMath::Clamp(RaiseProgram, 0, int32(ECampaign1851Program::Count) - 1))), EButton::UnitProgram);
	RY += 8.f;
	// What it takes, and what there is.
	const Campaign1851Resources::FUnitType T = Campaign1851Resources::SizedType(RaiseType, RaiseSize);
	const int32 Amt = Town != INDEX_NONE ? Map->AmtIndexOfTown(Town) : INDEX_NONE;
	const float Reserve = Map->GetAmtManpower().IsValidIndex(Amt) ? Map->GetAmtManpower()[Amt] : 0.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("D E T   K R Æ V E R"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
	RY += 24.f;
	auto Need = [&](const FString& What, float Want, float Have, const FString& Note)
	{
		const bool bOk = Have >= Want;
		PaintText(Geometry, Out, Layer + 1, What, FVector2D(RX, RY), Serif(13), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Thousands(FMath::RoundToInt(Want)), FVector2D(RX + 230.f, RY), Serif(13), Ink, 1.f, false);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("har %s"), *Thousands(FMath::RoundToInt(Have))), FVector2D(RX + 250.f, RY), Serif(12), bOk ? MutedInk : CityRed, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 1, Note, FVector2D(RX + 390.f, RY), Serif(10, EFace::Italic), MutedInk, RW - 390.f);
		RY += 23.f;
	};
	Need(TEXT("Rekrutter fra amtet"), T.Men, Reserve, TEXT(""));
	if (T.Rifles > 0) Need(TEXT("Geværer"), T.Rifles, Map->GetRifles(), TEXT("det manglende købes i udlandet"));
	if (T.Guns > 0) Need(TEXT("Kanoner"), T.Guns, Map->GetGunStock(), TEXT(""));
	if (T.Mortars > 0) Need(TEXT("Morterer"), T.Mortars, Map->GetMortarStock(), TEXT(""));
	if (T.Wagons > 0) Need(TEXT("Vogne"), T.Wagons, Map->GetWagonStock(), TEXT("de manglende købes i landet"));
	Need(TEXT("Uniformer (klæde)"), T.Uniforms, Map->GetRaw(ECampaign1851Raw::Cloth), TEXT(""));
	Need(TEXT("Remtøj og sadler (læder)"), T.Leather, Map->GetRaw(ECampaign1851Raw::Leather), TEXT(""));
	Need(TEXT("Heste"), T.Horses, Map->GetHorseStock(), TEXT("det manglende købes i amterne"));
	Need(TEXT("Penge (rd.)"), float(Map->UnitCost(RaiseType, RaiseSize)), float(Map->GetTreasury()), TEXT("udrustning og sold"));
	RY += 14.f;
	const FString Why = Map->UnitBlockReason(RaiseType, Town, RaiseSize);
	PaintButton(Geometry, Out, Layer + 1, FVector2D(RX, RY), FVector2D(RW, 34.f), Why.IsEmpty() ? FString::Printf(TEXT("OPRET %s"), *FString(T.Name).ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å"))) : Why,
		EButton::UnitRaise, 0, false, !Why.IsEmpty());
}

void SCampaign1851Overlay::PaintBattlefield(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const FCampaign1851Battlefield& B = Map->GetBattlefield();
	PaintText(Geometry, Out, Layer + 1, TEXT("Slagmarken"), Pos + FVector2D(24.f, 34.f), Serif(22), Ink, 0.f);
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Terrænet til et 3D-slag, bygget ud fra stedet på kortet: højder, skov, vand, byer, gårde, veje, jernbaner og skanser. Enhederne kommer senere."),
		Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Size.X - 170.f);
	const float ImageSide = FMath::Min(Size.Y - 130.f, Size.X - 520.f);
	const FVector2D ImagePos = Pos + FVector2D(24.f, 100.f);
	if (!B.IsValid() || !Map->GetBattlefieldTexture())
	{
		PaintText(Geometry, Out, Layer + 1, TEXT("Tryk GENERÉR HER for at bygge slagmarken, hvor kortet står centreret."), ImagePos + FVector2D(0.f, 20.f), Serif(14, EFace::Italic), MutedInk, 0.f, false);
	}
	else
	{
		if (!BattlefieldBrush.IsValid())
		{
			BattlefieldBrush = MakeShared<FSlateBrush>();
		}
		if (BattlefieldBrush->GetResourceObject() != Map->GetBattlefieldTexture())
		{
			BattlefieldBrush->SetResourceObject(Map->GetBattlefieldTexture());
			BattlefieldBrush->ImageSize = FVector2D(512.f, 512.f);
		}
		FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(FVector2D(ImageSide, ImageSide), FSlateLayoutTransform(ImagePos)), BattlefieldBrush.Get());
		const float SizeM = B.SizeKm * 1000.f;
		auto ToImage = [&](const FVector2D& M) { return ImagePos + FVector2D(M.X / SizeM, 1.f - M.Y / SizeM) * ImageSide; };
		// Town names and the forts.
		for (const FCampaign1851BattleTown& T : B.Towns)
		{
			if (T.M.X > 0.f && T.M.Y > 0.f && T.M.X < SizeM && T.M.Y < SizeM)
			{
				PaintText(Geometry, Out, Layer + 3, T.Name, ToImage(T.M), Serif(15, EFace::Bold), Ink, 0.5f);
			}
		}
		for (int32 f = 0; f < B.Forts.Num(); ++f)
		{
			PaintText(Geometry, Out, Layer + 3, B.Forts[f].Name, ToImage(B.FortM[f]) + FVector2D(0.f, -16.f), Serif(10), Gold, 0.5f);
		}
		// Scale bar (1 km) and north.
		const float Km = ImageSide / B.SizeKm;
		const FVector2D S0 = ImagePos + FVector2D(16.f, ImageSide - 20.f);
		DrawLines(Geometry, Out, Layer + 3, { S0, S0 + FVector2D(Km, 0.f) }, Ink, 3.f);
		PaintText(Geometry, Out, Layer + 3, TEXT("1 km"), S0 + FVector2D(Km * 0.5f, -12.f), Serif(11), Ink, 0.5f);
		PaintText(Geometry, Out, Layer + 3, TEXT("N"), ImagePos + FVector2D(ImageSide - 24.f, 18.f), Serif(14), Ink, 0.5f);
		DrawLines(Geometry, Out, Layer + 2, { ImagePos, ImagePos + FVector2D(ImageSide, 0.f), ImagePos + FVector2D(ImageSide, ImageSide), ImagePos + FVector2D(0.f, ImageSide), ImagePos }, Gold, 1.5f);
	}
	// Right: facts, size and the buttons.
	const float RX = ImagePos.X + ImageSide + 30.f;
	float Y = Pos.Y + 110.f;
	auto Line = [&](const FString& Label, const FString& Value)
	{
		PaintText(Geometry, Out, Layer + 1, Label, FVector2D(RX, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 1, Value, FVector2D(RX + 150.f, Y), Serif(13), Ink, Pos.X + Size.X - RX - 180.f);
		Y += 24.f;
	};
	if (B.IsValid())
	{
		int32 Wood = 0, Sea = 0, Town = 0;
		float Lo = 1e9f, Hi = -1e9f;
		for (int32 k = 0; k < B.Kind.Num(); ++k)
		{
			Wood += B.Kind[k] == uint8(EBattlefieldCell::Wood) ? 1 : 0;
			Sea += B.Kind[k] == uint8(EBattlefieldCell::Sea) ? 1 : 0;
			Town += B.Kind[k] == uint8(EBattlefieldCell::Town) ? 1 : 0;
			if (B.Kind[k] != uint8(EBattlefieldCell::Sea)) { Lo = FMath::Min(Lo, B.HeightM[k]); Hi = FMath::Max(Hi, B.HeightM[k]); }
		}
		const float Cells = FMath::Max(1, B.Kind.Num()) / 100.f;
		Line(TEXT("Ved"), B.Place);
		Line(TEXT("Midtpunkt"), FString::Printf(TEXT("%.4f° N, %.4f° Ø"), B.Lat, B.Lon));
		Line(TEXT("Størrelse"), FString::Printf(TEXT("%.0f × %.0f km  ·  256 × 256 felter à %.0f m"), B.SizeKm, B.SizeKm, B.SizeKm * 1000.f / 256.f));
		Line(TEXT("Dato"), FString::Printf(TEXT("%s  ·  %s"), *ACampaign1851Map::FormatDate(ACampaign1851Map::StartDate() + FTimespan::FromDays(B.Day)), *Map->GetSeasonAndWeather()));
		Line(TEXT("Højde"), Hi > Lo ? FString::Printf(TEXT("%.0f – %.0f m"), Lo, Hi) : FString(TEXT("-")));
		Line(TEXT("Skov / vand / by"), FString::Printf(TEXT("%.0f %% / %.0f %% / %.0f %%"), Wood / Cells, Sea / Cells, Town / Cells));
		Line(TEXT("Bygninger"), FString::Printf(TEXT("%d  ·  %d gårde"), B.Buildings.Num(), B.Farms));
		Line(TEXT("Veje"), FString::Printf(TEXT("%d landeveje, %d markveje, %d chausséer, %d jernbaner"), B.Roads.Num(), B.Lanes.Num(), B.Chaussees.Num(), B.Rails.Num()));
		Line(TEXT("Skanser"), FString::FromInt(B.Forts.Num()));
		{
			TArray<FString> Names;
			for (const FCampaign1851BattleRiver& Rv : B.Rivers) { Names.AddUnique(Rv.Name); }
			const int32 Fords = B.Crossings.FilterByPredicate([](const FCampaign1851BattleCrossing& C) { return C.Kind == TEXT("ford"); }).Num();
			Line(TEXT("Vand"), Names.Num() + B.Lakes.Num() == 0 ? FString(TEXT("-"))
				: FString::Printf(TEXT("%s%s  ·  %d broer, %d vadesteder"), *FString::Join(Names, TEXT(", ")), B.Lakes.Num() > 0 ? *FString::Printf(TEXT(", %d sø(er)"), B.Lakes.Num()) : TEXT(""),
					B.Crossings.Num() - Fords + B.Bridges.Num(), Fords));
			const TCHAR* HedgeName = B.Hedges.Num() == 0 ? TEXT("-") : B.Hedges[0].Kind == EHedgeKind::Knick ? TEXT("knicks (levende hegn på vold)") : B.Hedges[0].Kind == EHedgeKind::Dike ? TEXT("sten- og jorddiger") : TEXT("grøfter");
			Line(TEXT("Markskel"), B.Hedges.Num() == 0 ? FString(TEXT("-")) : FString::Printf(TEXT("%d stræk %s"), B.Hedges.Num(), HedgeName));
		}
		Line(TEXT("Fil"), Map->BattlefieldFile());
		Y += 16.f;
	}
	PaintText(Geometry, Out, Layer + 1, TEXT("S T Ø R R E L S E"), FVector2D(RX, Y), Serif(11), Gold, 0.f, false);
	Y += 18.f;
	const int32 Sizes[] = { 4, 8, 12 };
	for (int32 s = 0; s < 3; ++s)
	{
		PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + s * 96.f, Y), FVector2D(90.f, 26.f), FString::Printf(TEXT("%d KM"), Sizes[s]), EButton::BattlefieldSize, Sizes[s],
			FMath::RoundToInt(Map->BattlefieldSizeKm) == Sizes[s]);
	}
	Y += 44.f;
	PaintButton(Geometry, Out, Layer + 1, FVector2D(RX, Y), FVector2D(280.f, 32.f), TEXT("GENERÉR HER"), EButton::BattlefieldHere, 0);
	PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + 290.f, Y), FVector2D(250.f, 32.f), TEXT("GÅ IND PÅ SLAGMARKEN"), EButton::BattleViewEnter, 0, false, !Map->GetBattlefield().IsValid());
	PaintButton(Geometry, Out, Layer + 1, FVector2D(RX, Y + 78.f), FVector2D(540.f, 36.f), TEXT("TEST ET SLAG PÅ DENNE SLAGMARK (3D)"), EButton::TestBattle, 0, false, !Map->GetBattlefield().IsValid());
	AddTip(FVector2D(RX, Y + 78.f), FVector2D(540.f, 36.f), TEXT("Åbner 3D-slaget på den genererede slagmark med din hær (og en fjende) til at prøve: kamporden, kanoner, rytteri, officerernes beslutninger. Det påvirker ikke kampagnen; AFSLUT SLAGET fører tilbage hertil. Under INDSTILLINGER i slaget kan du slå fjendens skud fra."));
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Midt på kortets udsnit. Et slag bygger sin egen slagmark (SE SLAGMARKEN i slagpanelet)."), FVector2D(RX, Y + 46.f), Serif(10, EFace::Italic), MutedInk, Pos.X + Size.X - RX - 30.f);
	// Legend.
	Y += 130.f;
	const TPair<const TCHAR*, FLinearColor> Legend[] = {
		{ TEXT("Mark"), FLinearColor::FromSRGBColor(FColor(172, 160, 104)) }, { TEXT("Eng og strand"), FLinearColor::FromSRGBColor(FColor(118, 146, 104)) },
		{ TEXT("Skov"), FLinearColor::FromSRGBColor(FColor(52, 84, 46)) }, { TEXT("By"), FLinearColor::FromSRGBColor(FColor(158, 130, 104)) },
		{ TEXT("Hav"), FLinearColor::FromSRGBColor(FColor(62, 96, 124)) }, { TEXT("Chaussé / landevej"), FLinearColor::FromSRGBColor(FColor(224, 206, 160)) },
		{ TEXT("Jernbane"), FLinearColor::FromSRGBColor(FColor(52, 46, 42)) }, { TEXT("Huse og gårde"), FLinearColor::FromSRGBColor(FColor(170, 64, 48)) },
		{ TEXT("Skanse og løbegrav"), FLinearColor::FromSRGBColor(FColor(150, 128, 84)) }, { TEXT("Å, sø og vadested"), FLinearColor::FromSRGBColor(FColor(74, 112, 140)) },
		{ TEXT("Knick (hegn på vold)"), FLinearColor::FromSRGBColor(FColor(40, 66, 30)) }, { TEXT("Sten- og jorddige"), FLinearColor::FromSRGBColor(FColor(160, 156, 146)) },
		{ TEXT("Grøft i marsken"), FLinearColor::FromSRGBColor(FColor(70, 104, 128)) } };
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	// Two columns.
	const int32 Half = (UE_ARRAY_COUNT(Legend) + 1) / 2;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Legend); ++i)
	{
		const float LX = RX + (i < Half ? 0.f : 250.f), LY = Y + (i % Half) * 20.f;
		FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(FVector2D(18.f, 12.f), FSlateLayoutTransform(FVector2D(LX, LY - 6.f))), White, ESlateDrawEffect::None, Legend[i].Value);
		PaintText(Geometry, Out, Layer + 1, Legend[i].Key, FVector2D(LX + 28.f, LY), Serif(12), Ink, 0.f, false);
	}
	Y += Half * 20.f;
	PaintText(Geometry, Out, Layer + 1, TEXT("Højdekurver for hver 5 m; skyggen falder fra nordvest."), FVector2D(RX, Y + 6.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
}

void SCampaign1851Overlay::PaintArmyStatus(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const float X = Pos.X + 24.f;
	float Y = Pos.Y + 110.f;
	// ---------------------------------------------------------------- the army by arm
	struct FSum { int32 Units = 0, Men = 0, Present = 0, Sick = 0, Horses = 0, Guns = 0, Mortars = 0, Field = 0; float Exp = 0.f; };
	const TCHAR* Kinds[] = { TEXT("Linjeinfanteri"), TEXT("Garden"), TEXT("Jægere"), TEXT("Rytteri"), TEXT("Fodartilleri"), TEXT("Ridende artilleri"), TEXT("Morterer") };
	FSum Sums[7], Total;
	for (const FCampaign1851Regiment& R : Regs)
	{
		const int32 k = R.Mortars > 0 ? 6 : FMath::Clamp(int32(R.Arm), 0, 5);
		for (FSum* S : { &Sums[k], &Total })
		{
			S->Units += 1;
			S->Men += R.Men;
			S->Present += R.PresentMen();
			S->Sick += R.Sick;
			S->Horses += R.Horses;
			S->Guns += R.Guns;
			S->Mortars += R.Mortars;
			S->Field += R.Formation != 0 ? 1 : 0;
			S->Exp += R.Experience * R.Men;
		}
	}
	PaintText(Geometry, Out, Layer + 1, TEXT("H Æ R E N   P R .   V Å B E N A R T"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 26.f;
	const TCHAR* Heads[] = { TEXT("Våbenart"), TEXT("Enheder"), TEXT("I felten"), TEXT("Mand"), TEXT("Til stede"), TEXT("Syge og sårede"), TEXT("Heste"), TEXT("Kanoner"), TEXT("Morterer"), TEXT("Erfaring") };
	const float ColX[] = { 0.f, 220.f, 320.f, 420.f, 530.f, 650.f, 800.f, 900.f, 1000.f, 1100.f };
	for (int32 c = 0; c < 10; ++c)
	{
		PaintText(Geometry, Out, Layer + 1, Heads[c], FVector2D(X + ColX[c], Y), Serif(11, EFace::Italic), MutedInk, 0.f, false);
	}
	Y += 22.f;
	auto Row = [&](const FString& Name, const FSum& S, bool bTotal)
	{
		const FLinearColor C = bTotal ? Gold : Ink;
		const FString Cells[] = { Name, FString::FromInt(S.Units), FString::FromInt(S.Field), Thousands(S.Men), Thousands(S.Present), Thousands(S.Sick), Thousands(S.Horses),
			FString::FromInt(S.Guns), FString::FromInt(S.Mortars), S.Men > 0 ? FString::Printf(TEXT("%.0f"), S.Exp / S.Men) : FString(TEXT("-")) };
		for (int32 c = 0; c < 10; ++c)
		{
			PaintText(Geometry, Out, Layer + 1, Cells[c], FVector2D(X + ColX[c], Y), Serif(13, bTotal ? EFace::Bold : EFace::Regular), C, 0.f, false);
		}
		Y += 22.f;
	};
	for (int32 k = 0; k < 7; ++k)
	{
		if (Sums[k].Units > 0)
		{
			Row(Kinds[k], Sums[k], false);
		}
	}
	DrawLines(Geometry, Out, Layer + 1, { FVector2D(X, Y - 10.f), FVector2D(X + 1180.f, Y - 10.f) }, Gold.CopyWithNewOpacity(0.4f), 1.f);
	Row(TEXT("Hele hæren"), Total, true);
	PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("På lager: %s geværer  ·  %d kanoner  ·  %s heste  ·  %d vogne  ·  %d morterer"),
		*Thousands(Map->GetRifles()), Map->GetGunStock(), *Thousands(Map->GetHorseStock()), Map->GetWagonStock(), Map->GetMortarStock()), FVector2D(X, Y + 4.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
	Y += 46.f;

	// ---------------------------------------------------------------- the losses, both sides, and the booty
	int32 Fallen = 0, Wounded = 0, Taken = 0, Kills = 0;
	for (const FCampaign1851Regiment& R : Regs)
	{
		Fallen += R.TotalKilled;
		Wounded += R.TotalWounded;
		Taken += R.TotalCaptured;
		Kills += R.TotalEnemyKilled;
	}
	const float ColW = 380.f;
	auto Block = [&](float BX, const TCHAR* Head, const TArray<TPair<FString, FString>>& Lines)
	{
		float BY = Y;
		PaintText(Geometry, Out, Layer + 1, Head, FVector2D(BX, BY), Serif(11), Gold, 0.f, false);
		BY += 26.f;
		for (const TPair<FString, FString>& L : Lines)
		{
			PaintText(Geometry, Out, Layer + 1, L.Key, FVector2D(BX, BY), Serif(13), Ink, 0.f, false);
			PaintText(Geometry, Out, Layer + 1, L.Value, FVector2D(BX + ColW - 60.f, BY), Serif(14, EFace::Bold), Ink, 1.f, false);
			BY += 22.f;
		}
	};
	Block(X, TEXT("V O R E S   T A B"), {
		{ TEXT("Faldne"), Thousands(Fallen) },
		{ TEXT("Sårede (i alt)"), Thousands(Wounded) },
		{ TEXT("I lazarettet nu"), Thousands(Map->SickTotal()) },
		{ TEXT("Taget til fange"), Thousands(Taken) },
		{ TEXT("Fanger hos fjenden nu"), Thousands(Map->GetDanesCaptured()) },
		{ TEXT("Tab i alt (også skanserne)"), Thousands(Map->GetDanishWarLosses()) } });
	Block(X + ColW + 20.f, TEXT("F J E N D E N S   T A B"), {
		{ TEXT("Dræbt"), Thousands(Map->GetEnemyKilled()) },
		{ TEXT("Såret"), Thousands(Map->GetEnemyWounded()) },
		{ TEXT("Taget til fange"), Thousands(Map->GetEnemyCapturedTotal()) },
		{ TEXT("Fanger hos os nu"), Thousands(Map->GetEnemyCaptured()) },
		{ TEXT("Sat ud af kampen i alt"), Thousands(Map->GetEnemyWarLosses()) },
		{ TEXT("Heraf af vores enheder"), Thousands(Kills) } });
	Block(X + 2.f * (ColW + 20.f), TEXT("E R O B R E T   U D S T Y R"), {
		{ TEXT("Geværer"), Thousands(Map->GetCapturedRifles()) },
		{ TEXT("Kanoner"), FString::FromInt(Map->GetCapturedGuns()) },
		{ TEXT("Heste"), Thousands(Map->GetCapturedHorses()) },
		{ TEXT("Vogne"), FString::FromInt(Map->GetCapturedWagons()) },
		{ TEXT("Faner"), FString::FromInt(Map->GetCapturedColours()) } });
	AddTip(FVector2D(X + 2.f * (ColW + 20.f), Y - 14.f), FVector2D(ColW, 140.f), TEXT("Det fjenden efterlader på slagmarken, når vi holder den: geværerne, hestene og vognene går på lager, kanonerne i statens kanonbeholdning."));
	Y += 26.f + 6.f * 22.f + 24.f;

	// ---------------------------------------------------------------- the units that have fought most
	TArray<int32> Best;
	for (int32 i = 0; i < Regs.Num(); ++i)
	{
		if (Regs[i].Service.Num() > 0) { Best.Add(i); }
	}
	Best.Sort([&Regs](int32 A, int32 B) { return Regs[A].TotalEnemyKilled > Regs[B].TotalEnemyKilled; });
	PaintText(Geometry, Out, Layer + 1, TEXT("E N H E D E R N E   I   K A M P"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 24.f;
	if (Best.Num() == 0)
	{
		PaintText(Geometry, Out, Layer + 1, TEXT("Hæren har endnu ikke været i kamp."), FVector2D(X, Y), Serif(12, EFace::Italic), MutedInk, 0.f, false);
	}
	for (int32 n = 0; n < Best.Num() && Y < Pos.Y + Size.Y - 30.f; ++n)
	{
		const FCampaign1851Regiment& R = Regs[Best[n]];
		PaintTextFit(Geometry, Out, Layer + 1, R.Name, FVector2D(X, Y), Serif(12), Ink, 260.f);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%d slag  ·  faldne %d  ·  sårede %d  ·  fangne %d  ·  fjender sat ud af kampen %d"),
			R.Service.Num(), R.TotalKilled, R.TotalWounded, R.TotalCaptured, R.TotalEnemyKilled), FVector2D(X + 280.f, Y), Serif(11, EFace::Italic), MutedInk, 0.f, false);
		Y += 20.f;
	}
}

void SCampaign1851Overlay::PaintOOBChart(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const TArray<FCampaign1851Formation>& Forms = Map->GetFormations();
	const TArray<FCampaign1851Officer>& Officers = Map->GetOfficers();
	const TArray<FCampaign1851Command>& Commands = Map->GetCommands();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const FLinearColor Dark = FLinearColor::FromSRGBColor(FColor(30, 22, 12));
	auto OfficerText = [&](int32 O) { return Officers.IsValidIndex(O) ? FString::Printf(TEXT("%s %s %s"), *Officers[O].Rank, *Officers[O].Name, *FString::ChrN(Campaign1851Army::Stars(Officers[O].Experience), TEXT('*'))) : FString(TEXT("ubesat")); };
	// A colour for each arm: infantry blue, cavalry red, guns green, mortars violet.
	const FLinearColor ArmColours[] = { FLinearColor::FromSRGBColor(FColor(26, 40, 70, 245)), FLinearColor::FromSRGBColor(FColor(84, 30, 34, 245)),
		FLinearColor::FromSRGBColor(FColor(40, 64, 34, 245)), FLinearColor::FromSRGBColor(FColor(62, 38, 78, 245)) };
	const TCHAR* ArmLegend[] = { TEXT("Fodfolk"), TEXT("Rytteri"), TEXT("Kanoner"), TEXT("Morterer") };
	auto ArmSlot = [](const FCampaign1851Regiment& R) { return R.Mortars > 0 ? 3 : R.Guns > 0 ? 2 : R.Arm == ECampaign1851Arm::Cavalry ? 1 : 0; };

	// ---------------------------------------------------------------- left: the units in garrison, to drag in
	const float PoolW = 260.f;
	const int32 GarrisonKey = TreeKey(ETreeKind::Garrisons, 0);
	PaintButton(Geometry, Out, Layer + 2, Pos, FVector2D(PoolW, 26.f), [&]() -> const TCHAR*
	{
		// The garrison's list (when the chosen units stand in garrison), or the unit being split (when it is out in the field).
		bool bAllHome = true;
		for (int32 u : OOBFilter) { bAllHome &= Regs.IsValidIndex(u) && Regs[u].Formation == 0; }
		return OOBFilter.Num() == 0 || bAllHome ? TEXT("I GARNISON") : (OOBFilter.Num() == 1 ? TEXT("DEN VALGTE ENHED") : TEXT("DE VALGTE ENHEDER"));
	}(), EButton::TreeRow, GarrisonKey, bDragging && HoverKey == GarrisonKey && OOBFilter.Num() == 0);
	if (OOBFilter.Num() > 0)
	{
		// The chosen units (wherever they stand: in garrison or out in the field), unfolded with their companies or squadrons:
		// drag one to the right to make it a unit of its own, or onto another half to move it there.
		TArray<int32> Set;
		for (int32 u : OOBFilter) { if (Regs.IsValidIndex(u) && u != OOBBuilding) { Set.AddUnique(u); } }
		for (int32 u : TArray<int32>(Set)) { for (int32 i = 0; i < Regs.Num(); ++i) { if (i != OOBBuilding && Map->IsSplitPair(u, i)) { Set.AddUnique(i); } } }
		float Y = Pos.Y + 44.f;
		for (int32 u : Set)
		{
			const FCampaign1851Regiment& R = Regs[u];
			const int32 UnitKey = TreeKey(ETreeKind::Regiment, u);
			PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X, Y - 10.f), FVector2D(PoolW, 22.f), FString(), EButton::TreeRow, UnitKey, bDragging && HoverKey == UnitKey && UnitKey != DragKey);
			PaintTextFit(Geometry, Out, Layer + 3, FString::Printf(TEXT("%s [%s]"), *R.Name, Campaign1851Army::ArmMark(R.Arm)), FVector2D(Pos.X + 8.f, Y), Serif(12, EFace::Bold), Gold, PoolW - 70.f);
			PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("%d/%d"), R.Men, R.MaxMen), FVector2D(Pos.X + PoolW - 6.f, Y), Serif(11), Ink, 1.f, false);
			Y += 26.f;
			const int32 Parts = Map->SubUnitCount(u);
			for (int32 k = 0; k < Parts; ++k)
			{
				const int32 Key = TreeKey(ETreeKind::Company, u * 10 + k);
				PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 14.f, Y - 9.f), FVector2D(PoolW - 14.f, 20.f), FString(), EButton::TreeRow, Key, bDragging && DragKey == Key);
				if (R.Captains.Num() > 0)
				{
					PaintTextFit(Geometry, Out, Layer + 3, FString::Printf(TEXT("%d. Kompagni"), Map->CompanyNumber(u, k)), FVector2D(Pos.X + 24.f, Y), Serif(11), Ink, 100.f);
					PaintTextFit(Geometry, Out, Layer + 3, Officers.IsValidIndex(R.Captains[k]) ? Officers[R.Captains[k]].Name : FString(TEXT("ingen kaptajn")), FVector2D(Pos.X + 120.f, Y), Serif(9, EFace::Italic), MutedInk, PoolW - 190.f);
				}
				else
				{
					PaintTextFit(Geometry, Out, Layer + 3, FString::Printf(TEXT("%d. %s"), k + 1, R.Arm == ECampaign1851Arm::Artillery ? TEXT("Sektion") : TEXT("Eskadron")), FVector2D(Pos.X + 24.f, Y), Serif(11), Ink, 100.f);
				}
				PaintText(Geometry, Out, Layer + 3, R.Arm == ECampaign1851Arm::Artillery ? FString::Printf(TEXT("%d m · %d k · %d h"), Map->SubUnitMen(u, k), Map->SectionResource(u, k, 0), Map->SectionResource(u, k, 1)) : Map->CompanyCapacity(u) > 0 ? FString::Printf(TEXT("%d/%d"), Map->SubUnitMen(u, k), Map->CompanyCapacity(u)) : FString::FromInt(Map->SubUnitMen(u, k)), FVector2D(Pos.X + PoolW - 6.f, Y), Serif(10), Ink, 1.f, false);
				Y += 22.f;
			}
			if (Parts > 1)
			{
				PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 14.f, Y - 9.f), FVector2D(PoolW - 14.f, 20.f), R.Captains.Num() > 0 ? TEXT("Udjævn kompagnierne") : R.Arm == ECampaign1851Arm::Artillery ? TEXT("Udjævn mandskabet") : TEXT("Udjævn eskadronerne"), EButton::EqualizeUnit, u);
				Y += 24.f;
			}
			Y += 10.f;
		}
		PaintTextFit(Geometry, Out, Layer + 3, TEXT("Træk til højre: ny felthær uden ekstra HQ. Træk hen på en anden halvdel: flyt. Træk et kompagni, en eskadron eller en batterisektion hen på en anden (også i en anden enhed, der står samme sted): vælg hvor mange mand der flyttes. Det sidste samler halvdelene. Batterier: m = mand, k = kanoner, h = heste."), FVector2D(Pos.X, Y + 6.f), Serif(10, EFace::Italic), MutedInk, PoolW);
	}
	else
	{
		float Y = Pos.Y + 42.f;
		const float Bottom = Pos.Y + Size.Y - 12.f;
		bool bFull = false;
		for (int32 c = -1; c < Commands.Num() && !bFull; ++c)
		{
			TArray<int32> Units;
			for (int32 i = 0; i < Regs.Num(); ++i)
			{
				if (OOBFilter.Num() > 0 && !OOBFilter.Contains(i))
				{
					continue;
				}
				if (Regs[i].Formation == 0 && (Regs[i].Command == c || (c < 0 && !Commands.IsValidIndex(Regs[i].Command))))
				{
					Units.Add(i);
				}
			}
			if (Units.Num() == 0)
			{
				continue;
			}
			// A general command's heading can be dragged into the chart as a whole: it becomes a division.
			if (c >= 0)
			{
				const int32 CommandKey = TreeKey(ETreeKind::Command, c);
				PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X, Y - 9.f), FVector2D(PoolW, 17.f), FString(), EButton::TreeRow, CommandKey, bDragging && DragKey == CommandKey);
			}
			PaintTextFit(Geometry, Out, Layer + 3, c < 0 ? FString(TEXT("Uden kommando")) : FString::Printf(TEXT("%s  (træk hele)"), *Commands[c].Name), FVector2D(Pos.X + 4.f, Y), Serif(11, EFace::Italic), Gold, PoolW - 8.f);
			Y += 17.f;
			for (int32 i : Units)
			{
				if (Y > Bottom)
				{
					PaintText(Geometry, Out, Layer + 3, TEXT("..."), FVector2D(Pos.X + 12.f, Y - 6.f), Serif(11), MutedInk, 0.f, false);
					bFull = true;
					break;
				}
				const int32 Key = TreeKey(ETreeKind::Regiment, i);
				const bool bSel = SelectedRegiments.Contains(i);
				PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 8.f, Y - 8.f), FVector2D(PoolW - 8.f, 16.f), FString(), EButton::TreeRow, Key, bSel);
				FSlateDrawElement::MakeBox(Out, Layer + 3, Geometry.ToPaintGeometry(FVector2D(8.f, 12.f), FSlateLayoutTransform(FVector2D(Pos.X + 11.f, Y - 6.f))), White, ESlateDrawEffect::None,
					ArmColours[ArmSlot(Regs[i])].CopyWithNewOpacity(1.f) * 1.6f);
				PaintTextFit(Geometry, Out, Layer + 3, FString::Printf(TEXT("%s [%s]"), *Regs[i].Name, Campaign1851Army::ArmMark(Regs[i].Arm)), FVector2D(Pos.X + 24.f, Y), Serif(10), bSel ? Dark : Ink, PoolW - 80.f);
				PaintText(Geometry, Out, Layer + 3, FString::FromInt(Regs[i].Men), FVector2D(Pos.X + PoolW - 6.f, Y), Serif(10), bSel ? Dark : MutedInk, 1.f, false);
				// A small fold-out to see its companies (squadrons).
				const bool bOpenUnit = PoolOpen.Contains(i);
				const int32 UnitParts = Map->SubUnitCount(i);
				if (UnitParts > 0)
				{
					PaintButton(Geometry, Out, Layer + 3, FVector2D(Pos.X + PoolW - 46.f, Y - 6.f), FVector2D(16.f, 13.f), bOpenUnit ? TEXT("-") : TEXT("+"), EButton::PoolFold, i);
				}
				Y += 16.5f;
				if (bOpenUnit)
				{
					for (int32 k = 0; k < UnitParts && Y <= Bottom; ++k)
					{
						const int32 PartKey = TreeKey(ETreeKind::Company, i * 10 + k);
						PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 24.f, Y - 7.f), FVector2D(PoolW - 24.f, 14.f), FString(), EButton::TreeRow, PartKey, bDragging && DragKey == PartKey);
						const FString Who = Regs[i].Captains.IsValidIndex(k) && Officers.IsValidIndex(Regs[i].Captains[k]) ? Officers[Regs[i].Captains[k]].Name : FString();
						PaintTextFit(Geometry, Out, Layer + 3, Regs[i].Captains.Num() > 0 ? FString::Printf(TEXT("%d. Kompagni"), Map->CompanyNumber(i, k)) : FString::Printf(TEXT("%d. %s"), k + 1, Regs[i].Arm == ECampaign1851Arm::Artillery ? TEXT("Sektion") : TEXT("Eskadron")), FVector2D(Pos.X + 30.f, Y), Serif(9), MutedInk, 80.f);
						PaintTextFit(Geometry, Out, Layer + 3, Who, FVector2D(Pos.X + 112.f, Y), Serif(9, EFace::Italic), MutedInk, 100.f);
						PaintText(Geometry, Out, Layer + 3, Regs[i].Arm == ECampaign1851Arm::Artillery ? FString::Printf(TEXT("%d m · %d k · %d h"), Map->SubUnitMen(i, k), Map->SectionResource(i, k, 0), Map->SectionResource(i, k, 1)) : FString::FromInt(Map->SubUnitMen(i, k)), FVector2D(Pos.X + PoolW - 6.f, Y), Serif(9), MutedInk, 1.f, false);
						Y += 14.f;
					}
				}
			}
			Y += 3.f;
		}
	}

	{
		const ETreeKind DragKind = TreeKind(DragKey);
		const bool bFromField = bDragging && ((DragKind == ETreeKind::Company && Regs.IsValidIndex(TreeId(DragKey) / 10) && Regs[TreeId(DragKey) / 10].Formation != 0) || (DragKind == ETreeKind::Regiment && Regs.IsValidIndex(TreeId(DragKey)) && Regs[TreeId(DragKey)].Formation != 0) || DragKind == ETreeKind::Formation);
		if (bFromField)
		{
			const FVector2D ZoneMin(Pos.X, Pos.Y + 30.f), ZoneSize(PoolW, Size.Y - 42.f);
			const bool bHover = HoverKey == GarrisonKey;
			FSlateDrawElement::MakeBox(Out, Layer + 6, Geometry.ToPaintGeometry(ZoneSize, FSlateLayoutTransform(ZoneMin)), White, ESlateDrawEffect::None,
				bHover ? Gold.CopyWithNewOpacity(0.85f) : FLinearColor(0.05f, 0.04f, 0.03f, 0.88f));
			PaintTextFit(Geometry, Out, Layer + 7, TEXT("Slip her"), ZoneMin + FVector2D(PoolW * 0.5f, ZoneSize.Y * 0.5f - 12.f), Serif(16, EFace::Bold), bHover ? Dark : Ink, PoolW - 20.f, 0.5f);
			PaintTextFit(Geometry, Out, Layer + 7, DragKind == ETreeKind::Formation ? TEXT("hele formationen tilbage i garnison") : TEXT("tilbage i garnison"),
				ZoneMin + FVector2D(PoolW * 0.5f, ZoneSize.Y * 0.5f + 10.f), Serif(11, EFace::Italic), bHover ? Dark : MutedInk, PoolW - 20.f, 0.5f);
			Buttons.Add({ ZoneMin, ZoneMin + ZoneSize, EButton::TreeRow, GarrisonKey });
		}
	}

	// ---------------------------------------------------------------- right: the field army as an org chart
	const FVector2D Area(Pos.X + PoolW + 18.f, Pos.Y);
	const FVector2D AreaSize(Size.X - PoolW - 18.f - 190.f, Size.Y - 18.f);
	ChartMin = Area;
	ChartMax = Area + AreaSize;
	DrawLines(Geometry, Out, Layer + 1, { Area, FVector2D(Area.X, Area.Y + AreaSize.Y) }, Gold.CopyWithNewOpacity(0.35f), 1.f);

	auto Dashed = [&](const FVector2D& Min, const FVector2D& BoxSize, const FLinearColor& Colour)
	{
		const FVector2D Max = Min + BoxSize;
		const FVector2D Corners[] = { Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y), Min };
		for (int32 e = 0; e < 4; ++e)
		{
			const FVector2D A = Corners[e], B = Corners[e + 1];
			const float Len = FVector2D::Distance(A, B);
			for (float t = 0.f; t < Len; t += 12.f)
			{
				DrawLines(Geometry, Out, Layer + 6, { A + (B - A) * (t / Len), A + (B - A) * (FMath::Min(t + 7.f, Len) / Len) }, Colour, 1.5f);
			}
		}
	};
	// A drop slot: "drag here to make a new ..." (a new formation under Parent).
	auto Slot = [&](const FVector2D& Min, const FVector2D& BoxSize, int32 Parent, const FString& A, const FString& B)
	{
		const int32 Key = TreeKey(ETreeKind::NewFormation, Parent);
		const bool bHover = bDragging && HoverKey == Key;
		FSlateDrawElement::MakeBox(Out, Layer + 5, Geometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(Min)), White, ESlateDrawEffect::None,
			bHover ? Gold.CopyWithNewOpacity(0.85f) : FLinearColor(0.05f, 0.04f, 0.03f, 0.9f));
		Dashed(Min, BoxSize, Gold);
		PaintTextFit(Geometry, Out, Layer + 7, A, FVector2D(Min.X + BoxSize.X * 0.5f, Min.Y + BoxSize.Y * 0.5f - 8.f), Serif(BoxSize.Y > 100.f ? 16 : 11, EFace::Bold), bHover ? Dark : Ink, BoxSize.X - 12.f, 0.5f);
		PaintTextFit(Geometry, Out, Layer + 7, B, FVector2D(Min.X + BoxSize.X * 0.5f, Min.Y + BoxSize.Y * 0.5f + 10.f), Serif(BoxSize.Y > 100.f ? 12 : 9, EFace::Italic), bHover ? Dark : MutedInk, BoxSize.X - 12.f, 0.5f);
		Buttons.Add({ Min, Min + BoxSize, EButton::TreeRow, Key });
	};

	// A permanent new-tree target, outside the scrolling chart and inside the window.
	Slot(FVector2D(Pos.X + Size.X - 174.f, Pos.Y + 64.f), FVector2D(174.f, 150.f), 99999,
		TEXT("Træk herover"), TEXT("Ny felthær uden ekstra HQ"));

	// Layout. A formation's box on top; below it in a row: its sub-formations, one column per battalion
	// (its companies stacked under it) and one column for the rest (cavalry, batteries, the ammunition wagons).
	struct FNode { int32 Id = 0; TArray<int32> Kids; TArray<int32> Battalions; TArray<int32> Support; bool bLog = false; bool bOpen = true; float SubW = 0.f; };
	TArray<FNode> Nodes;
	TFunction<int32(int32)> Build = [&](int32 Id) -> int32
	{
		const int32 Me = Nodes.AddDefaulted();
		Nodes[Me].Id = Id;
		const int32 Index = Map->FormationIndex(Id);
		Nodes[Me].bOpen = !Collapsed.Contains(TreeKey(Id == 0 ? ETreeKind::FieldArmy : ETreeKind::Formation, Id));
		if (!Nodes[Me].bOpen)
		{
			return Me;
		}
		Nodes[Me].bLog = Index != INDEX_NONE && Forms[Index].Echelon == ECampaign1851Echelon::Division;
		for (const FCampaign1851Formation& F : Forms)
		{
			if (F.Parent == Id && !(Id == 0 && F.Echelon == ECampaign1851Echelon::Army))
			{
				const int32 Kid = Build(F.Id);
				Nodes[Me].Kids.Add(Kid);
			}
		}
		for (int32 i = 0; i < Regs.Num(); ++i)
		{
			if (Id != 0 && Regs[i].Formation == Id)
			{
				(Campaign1851Army::CompaniesFor(Regs[i].Arm) > 0 ? Nodes[Me].Battalions : Nodes[Me].Support).Add(i);
			}
		}
		return Me;
	};
	TArray<int32> OOBRoots;
	const int32 OOBLegacyRoot = Build(0);
	if (Forms.Num() == 0 || Forms.ContainsByPredicate([](const FCampaign1851Formation& OOBForm) { return OOBForm.Parent == 0 && OOBForm.Echelon != ECampaign1851Echelon::Army; })) { OOBRoots.Add(OOBLegacyRoot); }
	for (const FCampaign1851Formation& OOBArmy : Forms)
	{
		if (OOBArmy.Parent == 0 && OOBArmy.Echelon == ECampaign1851Echelon::Army) { OOBRoots.Add(Build(OOBArmy.Id)); }
	}
	const float BoxW = 160.f, Gap = 14.f, VGap = 82.f;
	const float HQH = 108.f, UnitH = 78.f, CompH = 42.f;
	auto Parts = [&](const FNode& N)
	{
		TArray<float> W;
		for (int32 K : N.Kids) { W.Add(Nodes[K].SubW); }
		for (int32 b = 0; b < N.Battalions.Num(); ++b) { W.Add(BoxW); }
		if (N.Support.Num() > 0 || N.bLog) { W.Add(BoxW); }
		return W;
	};
	TFunction<float(int32)> MeasureNode = [&](int32 N) -> float
	{
		for (int32 K : Nodes[N].Kids)
		{
			MeasureNode(K);
		}
		float Total = 0.f;
		const TArray<float> W = Parts(Nodes[N]);
		for (float V : W) { Total += V; }
		Total += FMath::Max(0, W.Num() - 1) * Gap;
		Nodes[N].SubW = FMath::Max(BoxW, Total);
		return Nodes[N].SubW;
	};
	float OOBForestWidth = 0.f;
	for (int32 OOBRoot : OOBRoots) { OOBForestWidth += MeasureNode(OOBRoot) + Gap; }
	const float MaxScroll = FMath::Max(0.f, OOBForestWidth + 24.f - AreaSize.X);
	ChartScroll = FMath::Clamp(ChartScroll, 0.f, MaxScroll);
	ChartScrollY = FMath::Clamp(ChartScrollY, 0.f, 1200.f);

	const int32 OOBChartButtonStart = Buttons.Num();
	Out.PushClip(FSlateClippingZone(Geometry.ToPaintGeometry(AreaSize, FSlateLayoutTransform(Area))));
	auto Line = [&](const FVector2D& A, const FVector2D& B) { DrawLines(Geometry, Out, Layer + 2, { A, B }, Gold.CopyWithNewOpacity(0.8f), 1.5f); };
	auto Visible = [&](const FVector2D& Min, const FVector2D& Max) { return Max.X > Area.X && Min.X < Area.X + AreaSize.X && Max.Y > Area.Y && Min.Y < Area.Y + AreaSize.Y; };
	// A box: 0 = headquarters, 1 = unit, 2 = company, 3 = ammunition wagons (no unit of its own yet).
	auto Box = [&](const FVector2D& Min, const FVector2D& BoxSize, int32 Key, int32 Style, const TArray<FString>& Lines, bool bSelected, int32 Arm = -1)
	{
		const bool bTarget = bDragging && Key != 0 && HoverKey == Key && Key != DragKey;
		const bool bLit = bSelected || bTarget;
		const FLinearColor Fill = bLit ? Gold.CopyWithNewOpacity(0.9f)
			: Arm >= 0 ? (Style == 2 ? (ArmColours[Arm] * 0.75f).CopyWithNewOpacity(0.94f) : ArmColours[Arm])
			: Style == 0 ? FLinearColor::FromSRGBColor(FColor(46, 34, 20, 245))
			: Style == 2 ? FLinearColor::FromSRGBColor(FColor(20, 30, 50, 235))
			: Style == 3 ? FLinearColor::FromSRGBColor(FColor(36, 36, 32, 235))
			: FLinearColor::FromSRGBColor(FColor(26, 40, 70, 245));
		const FLinearColor Text = bLit ? Dark : Ink;
		FSlateDrawElement::MakeBox(Out, Layer + 3, Geometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(Min)), White, ESlateDrawEffect::None, Fill);
		const FVector2D Max = Min + BoxSize;
		DrawLines(Geometry, Out, Layer + 4, { Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y), Min }, Style == 3 ? MutedInk : Gold, Style == 0 ? 2.f : 1.f);
		const float Step = Style == 2 ? 12.f : 13.5f;
		for (int32 l = 0; l < Lines.Num(); ++l)
		{
			const FSlateFontInfo Font = l == 0 ? Serif(Style == 2 ? 10 : 11, EFace::Bold) : l == 1 && Style != 2 ? Serif(9, EFace::Italic) : Serif(Style == 2 ? 9 : 10);
			PaintTextFit(Geometry, Out, Layer + 5, Lines[l], FVector2D(Min.X + BoxSize.X * 0.5f, Min.Y + 9.f + l * Step), Font, l == 1 && Style != 2 && !bLit ? Gold : Text, BoxSize.X - 10.f, 0.5f);
		}
		if (Key != 0 && Visible(Min, Max))
		{
			Buttons.Add({ FVector2D(FMath::Max(Min.X, Area.X), FMath::Max(Min.Y, Area.Y)), FVector2D(FMath::Min(Max.X, Area.X + AreaSize.X), FMath::Min(Max.Y, Area.Y + AreaSize.Y)), EButton::TreeRow, Key });
		}
	};
	auto Toggle = [&](const FVector2D& Min, int32 Key, bool bOpen)
	{
		if (Visible(Min, Min + FVector2D(16.f, 16.f)))
		{
			PaintButton(Geometry, Out, Layer + 6, Min, FVector2D(16.f, 16.f), bOpen ? TEXT("-") : TEXT("+"), EButton::TreeToggle, Key);
		}
	};

	TFunction<void(int32, float, float)> Place = [&](int32 N, float Left, float Top)
	{
		const FNode& Node = Nodes[N];
		const float Centre = Left + Node.SubW * 0.5f;
		const FVector2D Min(Centre - BoxW * 0.5f, Top);
		const int32 Index = Map->FormationIndex(Node.Id);
		const int32 HQKey = TreeKey(Index == INDEX_NONE ? ETreeKind::FieldArmy : ETreeKind::Formation, Node.Id);
		if (Index == INDEX_NONE)
		{
			int32 Men = 0;
			for (const FCampaign1851Regiment& R : Regs)
			{
				Men += R.Formation != 0 ? R.Men : 0;
			}
			Box(Min, FVector2D(BoxW, HQH), HQKey, 0, { TEXT("FELTHÆREN"), TEXT("Øverstkommanderende"), FString::Printf(TEXT("%d formationer"), Forms.Num()), FString::Printf(TEXT("%s mand"), *Thousands(Men)) }, false);
		}
		else
		{
			const FCampaign1851Formation& F = Forms[Index];
			const TArray<int32> All = Map->FormationRegiments(Node.Id);
			int32 Men = 0;
			bool bAll = All.Num() > 0;
			for (int32 i : All)
			{
				Men += Regs[i].Men;
				bAll &= SelectedRegiments.Contains(i);
			}
			bool bActing = false;
			const int32 Leader = Map->ActingCommander(F.Id, &bActing);
			Box(Min, FVector2D(BoxW, HQH), HQKey, 0, { FString::Printf(TEXT("%s [%s]"), *F.Name.ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å")), Campaign1851Army::EchelonMark(F.Echelon)),
				Campaign1851Army::FormationRole(F.Echelon),
				bActing ? FString::Printf(TEXT("fungerende: %s"), *OfficerText(Leader)) : OfficerText(F.Commander),
				FString::Printf(TEXT("NK: %s"), *OfficerText(F.Deputy)),
				FString::Printf(TEXT("%s: %s"), F.Echelon == ECampaign1851Echelon::Division ? TEXT("Stab") : TEXT("Adj."), *OfficerText(F.StaffChief)),
				FString::Printf(TEXT("%s mand  ·  %d enh."), *Thousands(Men), All.Num()) }, bAll);
			// "+" at the end of an officer line: fill (or change) that post. The rest of the box selects and drags.
			const EButton Posts[] = { EButton::FormationChief, EButton::FormationDeputy, EButton::FormationStaff };
			for (int32 Post = 0; Post < 3; ++Post)
			{
				const float LineY = Min.Y + 9.f + (2 + Post) * 13.5f;
				const FVector2D PlusMin(Min.X + BoxW - 17.f, LineY - 6.f);
				if (Visible(PlusMin, PlusMin + FVector2D(14.f, 12.f)))
				{
					PaintButton(Geometry, Out, Layer + 6, PlusMin, FVector2D(14.f, 12.f), TEXT("+"), Posts[Post], F.Id);
				}
			}
			if (F.Echelon != ECampaign1851Echelon::Army)
			{
				const FVector2D ChiefMin(Min.X + BoxW - 34.f, Min.Y + 30.f);
				if (Officers.IsValidIndex(F.Commander) && Visible(ChiefMin, ChiefMin + FVector2D(14.f, 12.f)))
				{
					PaintButton(Geometry, Out, Layer + 6, ChiefMin, FVector2D(14.f, 12.f), TEXT("-"), EButton::FormationChiefRemove, F.Id);
				}
				const FVector2D RemoveMin(Min.X + 5.f, Min.Y + HQH - 17.f);
				if (Visible(RemoveMin, RemoveMin + FVector2D(86.f, 13.f)))
				{
					PaintButton(Geometry, Out, Layer + 6, RemoveMin, FVector2D(86.f, 13.f), TEXT("OPLØS STAB"), EButton::FormationDissolve, F.Id);
				}
			}
		}
		Toggle(Min + FVector2D(BoxW - 17.f, HQH - 17.f), HQKey, Node.bOpen);
		// Explicit HQ insertion: lower formations and direct units move under the new staff.
		const ECampaign1851Echelon OOBLevels[] = { ECampaign1851Echelon::Regiment, ECampaign1851Echelon::Brigade, ECampaign1851Echelon::Division };
		const TCHAR* OOBLabels[] = { TEXT("OPRET REGIMENT-HQ"), TEXT("OPRET BRIGADE-HQ"), TEXT("OPRET DIVISIONS-HQ") };
		for (int32 OOBLevel = 0; OOBLevel < 3; ++OOBLevel)
		{
			if (Node.bOpen && Map->CanInsertFormationHQ(Node.Id, OOBLevels[OOBLevel]))
			{
				const FVector2D OOBButtonMin = Min + FVector2D(0.f, HQH + 4.f + OOBLevel * 19.f);
				if (Visible(OOBButtonMin, OOBButtonMin + FVector2D(BoxW, 17.f)))
				{
					PaintButton(Geometry, Out, Layer + 6, OOBButtonMin, FVector2D(BoxW, 17.f), OOBLabels[OOBLevel], EButton::FormationInsertHQ, Node.Id * 10 + int32(OOBLevels[OOBLevel]));
				}
			}
		}
		if (!Node.bOpen)
		{
			return;
		}
		const TArray<float> W = Parts(Node);
		float Total = 0.f;
		for (float V : W) { Total += V; }
		Total += FMath::Max(0, W.Num() - 1) * Gap;
		float X = Left + (Node.SubW - Total) * 0.5f;
		const float RowTop = Top + HQH + VGap;
		TArray<float> Centres;
		for (int32 K : Node.Kids)
		{
			Centres.Add(X + Nodes[K].SubW * 0.5f);
			Place(K, X, RowTop);
			X += Nodes[K].SubW + Gap;
		}
		for (int32 i : Node.Battalions)
		{
			const FCampaign1851Regiment& R = Regs[i];
			Centres.Add(X + BoxW * 0.5f);
			const int32 Senior = Map->SeniorCaptain(i);
			const bool OOBSingleCompany = R.bDetached && Map->SubUnitCount(i) == 1;
			const FString OOBUnitTitle = OOBSingleCompany ? FString::Printf(TEXT("%d. Kompagni [I]"), Map->CompanyNumber(i, 0)) : FString::Printf(TEXT("%s [%s]"), *R.Name, Campaign1851Army::ArmMark(R.Arm));
			Box(FVector2D(X, RowTop), FVector2D(BoxW, UnitH), TreeKey(OOBSingleCompany ? ETreeKind::Company : ETreeKind::Regiment, OOBSingleCompany ? i * 10 : i), 1, { OOBUnitTitle,
				OOBSingleCompany ? TEXT("Kompagnichef") : Campaign1851Army::UnitRole(R.Arm), OOBSingleCompany ? OfficerText(Senior) : Officers.IsValidIndex(R.Chief) ? OfficerText(R.Chief) : FString::Printf(TEXT("fungerende: %s"), *OfficerText(Senior)),
				FString::Printf(TEXT("NK: %s"), *OfficerText(Senior)), FString::Printf(TEXT("%d/%d mand"), R.Men, R.MaxMen) }, SelectedRegiments.Contains(i), ArmSlot(R));
			// Its companies, stacked under it.
			float Y = RowTop + UnitH + 10.f;
			const float Spine = X + 8.f;
			for (int32 k = 0; k < (OOBSingleCompany ? 0 : R.Captains.Num()); ++k)
			{
				Line(FVector2D(Spine, Y - 10.f), FVector2D(Spine, Y + CompH * 0.5f));
				Line(FVector2D(Spine, Y + CompH * 0.5f), FVector2D(X + 16.f, Y + CompH * 0.5f));
				Box(FVector2D(X + 16.f, Y), FVector2D(BoxW - 16.f, CompH), TreeKey(ETreeKind::Company, i * 10 + k), 2,
					{ FString::Printf(TEXT("%d. Kompagni [I]"), Map->CompanyNumber(i, k)), OfficerText(R.Captains[k]),
					  FString::Printf(TEXT("%d/%d mand"), Map->CompanyMen(i, k), R.MaxMen / FMath::Max(1, R.Captains.Num())) }, false, ArmSlot(R));
				Y += CompH + 5.f;
			}
			X += BoxW + Gap;
		}
		if (Node.Support.Num() > 0 || Node.bLog)
		{
			Centres.Add(X + BoxW * 0.5f);
			float Y = RowTop;
			for (int32 s = 0; s < Node.Support.Num(); ++s)
			{
				const FCampaign1851Regiment& R = Regs[Node.Support[s]];
				const bool OOBSinglePart = R.bDetached && Map->SubUnitCount(Node.Support[s]) == 1;
				if (s > 0)
				{
					Line(FVector2D(X + BoxW * 0.5f, Y - 10.f), FVector2D(X + BoxW * 0.5f, Y));
				}
				Box(FVector2D(X, Y), FVector2D(BoxW, UnitH), TreeKey(OOBSinglePart ? ETreeKind::Company : ETreeKind::Regiment, OOBSinglePart ? Node.Support[s] * 10 : Node.Support[s]), 1, { FString::Printf(TEXT("%s [%s]"), *R.Name, Campaign1851Army::ArmMark(R.Arm)),
					Campaign1851Army::UnitRole(R.Arm), OfficerText(R.Chief), R.Mortars > 0 ? FString::Printf(TEXT("%d mand  ·  %d morterer"), R.Men, R.Mortars)
					: R.Guns > 0 ? FString::Printf(TEXT("%d mand  ·  %d kanoner"), R.Men, R.Guns) : FString::Printf(TEXT("%d/%d mand  ·  %d heste"), R.Men, R.MaxMen, R.Horses) },
					SelectedRegiments.Contains(Node.Support[s]), ArmSlot(R));
				Y += UnitH + 10.f;
				for (int32 OOBPart = 0; OOBPart < (OOBSinglePart ? 0 : Map->SubUnitCount(Node.Support[s])); ++OOBPart)
				{
					Box(FVector2D(X + 16.f, Y), FVector2D(BoxW - 16.f, CompH), TreeKey(ETreeKind::Company, Node.Support[s] * 10 + OOBPart), 2,
						{ FString::Printf(TEXT("%d. %s"), OOBPart + 1, R.Arm == ECampaign1851Arm::Artillery ? TEXT("Sektion") : TEXT("Eskadron")),
						FString::Printf(TEXT("%d mand"), Map->SubUnitMen(Node.Support[s], OOBPart)) }, false, ArmSlot(R));
					Y += CompH + 5.f;
				}
			}
			if (Node.bLog)
			{
				if (Node.Support.Num() > 0)
				{
					Line(FVector2D(X + BoxW * 0.5f, Y - 10.f), FVector2D(X + BoxW * 0.5f, Y));
				}
				Box(FVector2D(X, Y), FVector2D(BoxW, 50.f), 0, 3, { TEXT("Ammunitionsvogne [LOG]"), TEXT("Logistisk chef"), TEXT("(kommer med forsyningen)") }, false);
			}
		}
		if (Centres.Num() > 0)
		{
			const float BarY = Top + HQH + VGap * 0.5f;
			Line(FVector2D(Centre, Top + HQH), FVector2D(Centre, BarY));
			Line(FVector2D(FMath::Min(Centres[0], Centre), BarY), FVector2D(FMath::Max(Centres.Last(), Centre), BarY));
			for (float C : Centres)
			{
				Line(FVector2D(C, BarY), FVector2D(C, RowTop));
			}
		}
	};
	float OOBRootX = Area.X + 12.f + FMath::Max(0.f, (AreaSize.X - OOBForestWidth) * 0.5f) - ChartScroll;
	for (int32 OOBRoot : OOBRoots)
	{
		Place(OOBRoot, OOBRootX, Area.Y + 28.f - ChartScrollY);
		OOBRootX += Nodes[OOBRoot].SubW + Gap;
	}
	Out.PopClip();
	// Hit rectangles follow the same clip as the drawing, including officer/HQ buttons.
	for (int32 OOBRectIndex = OOBChartButtonStart; OOBRectIndex < Buttons.Num(); ++OOBRectIndex)
	{
		FButtonRect& OOBRect = Buttons[OOBRectIndex];
		OOBRect.Min.X = FMath::Max(OOBRect.Min.X, Area.X);
		OOBRect.Min.Y = FMath::Max(OOBRect.Min.Y, Area.Y);
		OOBRect.Max.X = FMath::Min(OOBRect.Max.X, Area.X + AreaSize.X);
		OOBRect.Max.Y = FMath::Min(OOBRect.Max.Y, Area.Y + AreaSize.Y);
	}
	// The colours of the arms, top right.
	for (int32 a = 0; a < 4; ++a)
	{
		const FVector2D At(Area.X + AreaSize.X - (4 - a) * 96.f, Area.Y + 6.f);
		FSlateDrawElement::MakeBox(Out, Layer + 6, Geometry.ToPaintGeometry(FVector2D(18.f, 12.f), FSlateLayoutTransform(At)), White, ESlateDrawEffect::None, ArmColours[a] * 1.6f);
		PaintText(Geometry, Out, Layer + 6, ArmLegend[a], At + FVector2D(24.f, 6.f), Serif(10), Ink, 0.f, false);
	}

	PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("Klik: vælg  ·  + ved en officer: udnævn/skift  ·  træk til listen til venstre: tilbage i garnison  ·  træk en kasse hen på en anden: flyt  ·  HQ-knapper: indsæt hovedkvarter  ·  -/+: fold  ·  hjul: rul til siden, Shift+hjul: op/ned%s"),
		MaxScroll > 0.f ? *FString::Printf(TEXT("  (%.0f %%)"), 100.f * ChartScroll / MaxScroll) : TEXT("")),
		FVector2D(Area.X, Pos.Y + Size.Y - 2.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
	if (bDragging)
	{
		const FString Label = TreeKeyText(DragKey);
		const FVector2D At = DragPos + FVector2D(14.f, 10.f);
		const FVector2D LabelSize = Measure(Label, Serif(12)) + FVector2D(16.f, 8.f);
		FSlateDrawElement::MakeBox(Out, Layer + 8, Geometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(At)), White, ESlateDrawEffect::None, Gold.CopyWithNewOpacity(0.9f));
		PaintText(Geometry, Out, Layer + 9, Label, At + FVector2D(8.f, LabelSize.Y * 0.5f), Serif(12), Dark, 0.f, false);
	}
}

void SCampaign1851Overlay::PaintSidePanels(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	// The tree stands beside the left panel; the other side panels beside the tree when it is open.
	const float Bottom = Geometry.GetLocalSize().Y - 190.f;
	const float Left = 28.f + 580.f + 10.f;
	if (bOOB)
	{
		PaintOOB(Geometry, Out, Layer, FVector2D(Left, Bottom));
	}
	const FVector2D Beside(bOOB ? Left + 620.f + 10.f : Left, Bottom);
	if (bTrainingMenu && SelectedRegiments.Num() > 0)
	{
		PaintTrainingMenu(Geometry, Out, Layer + 4, Beside);
	}
	else if (Map->GetOfficers().IsValidIndex(InspectedOfficer) && Window == EWindow::None)
	{
		PaintOfficerCard(Geometry, Out, Layer + 4, Beside);
	}
	else if (Picker != EPicker::None)
	{
		PaintOfficerPicker(Geometry, Out, Layer + 4, Beside);
	}
}

void SCampaign1851Overlay::BuildTreeRows(TArray<FTreeRow>& Rows) const
{
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const TArray<FCampaign1851Formation>& Forms = Map->GetFormations();
	const TArray<FCampaign1851Officer>& Officers = Map->GetOfficers();
	const TArray<FCampaign1851Command>& Commands = Map->GetCommands();
	auto OfficerText = [&](int32 O) { return Officers.IsValidIndex(O) ? FString::Printf(TEXT("%s %s %s"), *Officers[O].Rank, *Officers[O].Name, *FString::ChrN(Campaign1851Army::Stars(Officers[O].Experience), TEXT('*'))) : FString(); };
	auto RegimentRow = [&](int32 i, int32 Depth, bool bForceOpen = false)
	{
		const FCampaign1851Regiment& R = Regs[i];
		FTreeRow Row;
		Row.Key = TreeKey(ETreeKind::Regiment, i);
		Row.Depth = Depth;
		Row.Text = FString::Printf(TEXT("%s [%s]"), *R.Name, Campaign1851Army::ArmMark(R.Arm));
		Row.Info = FString::Printf(TEXT("%s  ·  %d/%d  ·  %s"), Officers.IsValidIndex(R.Chief) ? *OfficerText(R.Chief) : TEXT("ingen chef"), R.Men, R.MaxMen,
			R.IsMarching() ? TEXT("på march") : *Map->DescribePlace(R.Town, R.Km));
		Row.bSelected = SelectedRegiments.Contains(i);
		Row.bHasChildren = R.Captains.Num() > 0;
		Row.bOpen = bForceOpen || Collapsed.Contains(Row.Key);
		Rows.Add(Row);
		if (Row.bHasChildren && Row.bOpen)
		{
			for (int32 k = 0; k < R.Captains.Num(); ++k)
			{
				FTreeRow Company;
				Company.Key = TreeKey(ETreeKind::Company, i * 10 + k);
				Company.Depth = Depth + 1;
				Company.Text = FString::Printf(TEXT("%d. Kompagni [I]"), Map->CompanyNumber(i, k));
				const int32 FortIdx = R.CompanyFort.IsValidIndex(k) ? Map->FortIndex(R.CompanyFort[k]) : INDEX_NONE;
				Company.Info = FString::Printf(TEXT("%s  ·  %d/%d%s"), Officers.IsValidIndex(R.Captains[k]) ? *OfficerText(R.Captains[k]) : TEXT("ingen kaptajn"),
					Map->CompanyMen(i, k), R.MaxMen / R.Captains.Num(), FortIdx != INDEX_NONE ? *FString::Printf(TEXT("  ·  i %s"), *Map->GetForts()[FortIdx].Name) : TEXT(""));
				Rows.Add(Company);
			}
		}
	};
	// One unit only (KAMPORDEN from its panel): the unit and its companies.
	if (Regs.IsValidIndex(OOBFocus))
	{
		RegimentRow(OOBFocus, 0, true);
		// Its other halves (split off it, or it off them): drag companies between them.
		for (int32 i = 0; i < Regs.Num(); ++i)
		{
			if (Map->IsSplitPair(OOBFocus, i))
			{
				RegimentRow(i, 0, true);
			}
		}
		return;
	}
	// The field army: formations and their units, top down.
	TFunction<void(int32, int32)> AddFormation = [&](int32 Id, int32 Depth)
	{
		const int32 Index = Map->FormationIndex(Id);
		const FCampaign1851Formation& F = Forms[Index];
		const TArray<int32> All = Map->FormationRegiments(Id);
		int32 Men = 0;
		bool bAllSelected = All.Num() > 0;
		for (int32 i : All)
		{
			Men += Regs[i].Men;
			bAllSelected &= SelectedRegiments.Contains(i);
		}
		FTreeRow Row;
		Row.Key = TreeKey(ETreeKind::Formation, Id);
		Row.Depth = Depth;
		Row.Text = FString::Printf(TEXT("%s [%s]"), *F.Name, Campaign1851Army::EchelonMark(F.Echelon));
		Row.Info = FString::Printf(TEXT("%s mand  ·  %s"), *Thousands(Men), Officers.IsValidIndex(F.Commander) ? *OfficerText(F.Commander) : TEXT("ingen chef"));
		Row.bSelected = bAllSelected;
		Row.bFormation = true;
		Row.bHasChildren = true;
		Row.bOpen = !Collapsed.Contains(Row.Key);
		Rows.Add(Row);
		if (!Row.bOpen)
		{
			return;
		}
		for (const FCampaign1851Formation& Sub : Forms)
		{
			if (Sub.Parent == Id)
			{
				AddFormation(Sub.Id, Depth + 1);
			}
		}
		for (int32 i = 0; i < Regs.Num(); ++i)
		{
			if (Regs[i].Formation == Id)
			{
				RegimentRow(i, Depth + 1);
			}
		}
	};
	FTreeRow Field;
	Field.Key = TreeKey(ETreeKind::FieldArmy, 0);
	Field.Text = TEXT("FELTHÆREN");
	int32 FieldMen = 0;
	for (const FCampaign1851Regiment& R : Regs)
	{
		FieldMen += R.Formation != 0 ? R.Men : 0;
	}
	Field.Info = Forms.Num() > 0 ? FString::Printf(TEXT("%s mand"), *Thousands(FieldMen)) : FString(TEXT("ingen formationer: træk enheder til en ny felthær"));
	Field.bHasChildren = Forms.Num() > 0;
	Field.bOpen = !Collapsed.Contains(Field.Key);
	const bool OOBLegacyVisible = Forms.Num() == 0 || Forms.ContainsByPredicate([](const FCampaign1851Formation& OOBForm) { return OOBForm.Parent == 0 && OOBForm.Echelon != ECampaign1851Echelon::Army; });
	if (OOBLegacyVisible) { Rows.Add(Field); }
	for (const FCampaign1851Formation& OOBForm : Forms)
	{
		if (OOBForm.Parent == 0 && (OOBForm.Echelon == ECampaign1851Echelon::Army || Field.bOpen))
		{
			AddFormation(OOBForm.Id, OOBForm.Echelon == ECampaign1851Echelon::Army ? 0 : 1);
		}
	}
	// The garrisons: units in no formation, by general command and arm.
	FTreeRow Garrison;
	Garrison.Key = TreeKey(ETreeKind::Garrisons, 0);
	Garrison.Text = TEXT("GARNISONER");
	Garrison.Info = TEXT("uden for felthæren");
	Garrison.bHasChildren = true;
	Garrison.bOpen = !Collapsed.Contains(Garrison.Key);
	Rows.Add(Garrison);
	if (!Garrison.bOpen)
	{
		return;
	}
	const TCHAR* Groups[] = { TEXT("Fodfolk"), TEXT("Rytteri"), TEXT("Artilleri") };
	for (int32 c = 0; c < Commands.Num(); ++c)
	{
		TArray<int32> In[3];
		for (int32 i = 0; i < Regs.Num(); ++i)
		{
			if (Regs[i].Formation == 0 && Regs[i].Command == c)
			{
				const ECampaign1851Arm A = Regs[i].Arm;
				In[A == ECampaign1851Arm::Cavalry ? 1 : (A == ECampaign1851Arm::Artillery || A == ECampaign1851Arm::HorseArtillery) ? 2 : 0].Add(i);
			}
		}
		FTreeRow Row;
		Row.Key = TreeKey(ETreeKind::Command, c);
		Row.Depth = 1;
		Row.Text = Commands[c].Name;
		Row.Info = Officers.IsValidIndex(Commands[c].General) ? OfficerText(Commands[c].General) : FString(TEXT("ingen kommanderende general"));
		Row.bHasChildren = In[0].Num() + In[1].Num() + In[2].Num() > 0;
		Row.bOpen = !Collapsed.Contains(Row.Key) && Row.bHasChildren;
		Rows.Add(Row);
		if (!Row.bOpen)
		{
			continue;
		}
		for (int32 g = 0; g < 3; ++g)
		{
			if (In[g].Num() == 0)
			{
				continue;
			}
			FTreeRow Group;
			Group.Key = TreeKey(ETreeKind::ArmGroup, c * 3 + g);
			Group.Depth = 2;
			Group.Text = FString::Printf(TEXT("%s (%d)"), Groups[g], In[g].Num());
			Group.bHasChildren = true;
			Group.bOpen = !Collapsed.Contains(Group.Key);
			Rows.Add(Group);
			if (Group.bOpen)
			{
				for (int32 i : In[g])
				{
					RegimentRow(i, 3);
				}
			}
		}
	}
}

bool SCampaign1851Overlay::IsOverTree(const FVector2D& ViewportPixel) const
{
	const FVector2D Local = ViewportPixel / FMath::Max(PaintScale, 0.01f);
	return bOOB && Local.X >= TreeMin.X && Local.Y >= TreeMin.Y && Local.X <= TreeMax.X && Local.Y <= TreeMax.Y;
}

FString SCampaign1851Overlay::TreeKeyText(int32 Key) const
{
	const int32 Id = Key % 100000;
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	switch (ETreeKind(Key / 100000))
	{
	case ETreeKind::Regiment: return Map->GetRegiments().IsValidIndex(Id) ? Map->GetRegiments()[Id].Name : FString();
	case ETreeKind::Formation: return Map->FormationIndex(Id) != INDEX_NONE ? Map->GetFormations()[Map->FormationIndex(Id)].Name : FString();
	case ETreeKind::Company:
		return Regs.IsValidIndex(Id / 10) && Regs[Id / 10].Arm == ECampaign1851Arm::Artillery
			? FString::Printf(TEXT("%d. Sektion"), Id % 10 + 1)
			: FString::Printf(TEXT("%d. Kompagni"), Map->CompanyNumber(Id / 10, Id % 10));
	default: return FString();
	}
}

void SCampaign1851Overlay::PaintOOB(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft) const
{
	TArray<FTreeRow> Rows;
	BuildTreeRows(Rows);
	const float RowH = 23.f;
	const float Top = 130.f, Bottom = Geometry.GetLocalSize().Y - 190.f;
	const int32 Visible = FMath::Max(6, FMath::FloorToInt((Bottom - Top - 110.f) / RowH));
	TreeScroll = FMath::Clamp(TreeScroll, 0, FMath::Max(0, Rows.Num() - Visible));
	const int32 Shown = FMath::Min(Visible, Rows.Num() - TreeScroll);
	const FVector2D Size(620.f, 96.f + Shown * RowH + 20.f);
	const FVector2D Pos(BottomLeft.X, Bottom - Size.Y);
	TreeMin = Pos;
	TreeMax = Pos + Size;
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseOOB);
	PaintText(Geometry, Out, Layer + 2, TEXT("K A M P O R D E N"), Pos + FVector2D(22.f, 26.f), Serif(11), Gold, 0.f, false);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(160.f, 12.f), FVector2D(100.f, 26.f), TEXT("NY DIVISION"), EButton::TreeNew, int32(ECampaign1851Echelon::Division));
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(264.f, 12.f), FVector2D(100.f, 26.f), TEXT("NY BRIGADE"), EButton::TreeNew, int32(ECampaign1851Echelon::Brigade));
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(368.f, 12.f), FVector2D(100.f, 26.f), TEXT("NYT REGIMENT"), EButton::TreeNew, int32(ECampaign1851Echelon::Regiment));
	// The big diagram of the order of battle (the KAMPORDEN window of the menu).
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(472.f, 12.f), FVector2D(100.f, 26.f), TEXT("OVERSIGT"), EButton::MainMenu, int32(EWindow::Chart), Window == EWindow::Chart);
	if (Map->GetRegiments().IsValidIndex(OOBFocus))
	{
		// One unit: back to the whole army, or split it in two.
		PaintText(Geometry, Out, Layer + 2, TEXT("Én enhed  ·  del den, træk kompagnier mellem halvdelene, eller saml dem igen"), Pos + FVector2D(22.f, 56.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
		PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(Size.X - 280.f, 44.f), FVector2D(120.f, 24.f), TEXT("HELE HÆREN"), EButton::OOBFocusClear, 0);
		FString Why;
		const FCampaign1851Regiment& Focus = Map->GetRegiments()[OOBFocus];
		const bool bCanSplit = !Focus.IsMarching() && Map->SubUnitCount(OOBFocus) >= 2 && (Focus.Arm == ECampaign1851Arm::Artillery ? Focus.Men >= 2 : Focus.Captains.Num() >= 2 && Focus.Men >= 100);
		const int32 Partner = Map->MergePartner(OOBFocus);
		if (Focus.Arm == ECampaign1851Arm::Artillery && Focus.Mortars == 0)
		{
			const bool bCan = Map->CanUpgradeToHorseBattery(OOBFocus, &Why);
			PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(Size.X - 152.f, 44.f), FVector2D(120.f, 24.f), TEXT("GØR RIDENDE"), EButton::HorseBattery, OOBFocus, false, !bCan);
			AddTip(Pos + FVector2D(Size.X - 152.f, 44.f), FVector2D(120.f, 24.f), bCan ? FString(TEXT("Gør fodbatteriet ridende: alle kanonerer til hest, så det kan følge rytteriet.")) : Why);
		}
		else if (Partner != INDEX_NONE)
		{
			const int32 Keep = Focus.bDetached && !Map->GetRegiments()[Partner].bDetached ? Partner : OOBFocus;
			const int32 Absorb = Keep == OOBFocus ? Partner : OOBFocus;
			PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(Size.X - 152.f, 44.f), FVector2D(120.f, 24.f), TEXT("SAML IGEN"), EButton::MergeUnit, Keep * 10000 + Absorb);
			AddTip(Pos + FVector2D(Size.X - 152.f, 44.f), FVector2D(120.f, 24.f), TEXT("Saml de to halvdele til én enhed igen (de skal stå samme sted). Du kan også trække den ene halvdel hen på den anden, eller trække enkelte kompagnier mellem dem."));
		}
		else
		{
			// Opens the big order of battle with the unit; there the splitting is done (and the companies moved between the halves).
			PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(Size.X - 152.f, 44.f), FVector2D(120.f, 24.f), TEXT("DEL I TO"), EButton::OpenOOB, 0, false, !bCanSplit);
			AddTip(Pos + FVector2D(Size.X - 152.f, 44.f), FVector2D(120.f, 24.f), TEXT("Åbner kamporden med enheden: der deler du den i to (halvdelen af kompagnierne med deres kaptajner og mænd bliver en halvbataljon), og trækker kompagnier fra den ene halvdel til den anden, eller samler dem igen."));
		}
	}
	else
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("Træk enheder og formationer på plads  ·  klik: vælg  ·  hjul: rul"), Pos + FVector2D(22.f, 56.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
	}
	float Y = Pos.Y + 86.f;
	for (int32 r = 0; r < Shown; ++r)
	{
		const FTreeRow& Row = Rows[TreeScroll + r];
		const float X = Pos.X + 16.f + Row.Depth * 20.f;
		const bool bDropTarget = bDragging && HoverKey == Row.Key && Row.Key != DragKey;
		const FLinearColor Text = Row.bSelected ? FLinearColor::FromSRGBColor(FColor(30, 22, 12)) : Ink;
		PaintButton(Geometry, Out, Layer + 2, FVector2D(X, Y - 10.f), FVector2D(Pos.X + Size.X - 16.f - X - (Row.bFormation ? 124.f : 0.f), RowH - 2.f), FString(), EButton::TreeRow, Row.Key, Row.bSelected || bDropTarget);
		if (Row.bHasChildren)
		{
			PaintButton(Geometry, Out, Layer + 3, FVector2D(X + 2.f, Y - 8.f), FVector2D(18.f, 18.f), Row.bOpen ? TEXT("-") : TEXT("+"), EButton::TreeToggle, Row.Key);
		}
		const bool bHeader = Row.Depth == 0;
		PaintTextFit(Geometry, Out, Layer + 4, Row.Text, FVector2D(X + 26.f, Y + 1.f), bHeader ? Serif(12) : Serif(12), bHeader ? Gold : Text, 230.f - Row.Depth * 10.f);
		PaintTextFit(Geometry, Out, Layer + 4, Row.Info, FVector2D(X + 260.f - Row.Depth * 10.f, Y + 1.f), Serif(10, EFace::Italic), Row.bSelected ? Text : MutedInk,
			Pos.X + Size.X - 30.f - (Row.bFormation ? 124.f : 0.f) - (X + 260.f - Row.Depth * 10.f));
		if (Row.bFormation)
		{
			const int32 Id = Row.Key % 100000;
			PaintButton(Geometry, Out, Layer + 3, FVector2D(Pos.X + Size.X - 136.f, Y - 10.f), FVector2D(56.f, RowH - 2.f), TEXT("CHEF"), EButton::FormationChief, Id);
			PaintButton(Geometry, Out, Layer + 3, FVector2D(Pos.X + Size.X - 76.f, Y - 10.f), FVector2D(58.f, RowH - 2.f), TEXT("OPLØS"), EButton::FormationDissolve, Id);
		}
		Y += RowH;
	}
	if (Rows.Num() > Visible)
	{
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("række %d-%d af %d"), TreeScroll + 1, TreeScroll + Shown, Rows.Num()), FVector2D(Pos.X + Size.X - 22.f, Pos.Y + Size.Y - 12.f),
			Serif(9, EFace::Italic), MutedInk, 1.f, false);
	}
	if (bDragging)
	{
		// What is being dragged follows the cursor.
		const FString Label = TreeKeyText(DragKey);
		const FVector2D At = DragPos + FVector2D(14.f, 10.f);
		const FVector2D LabelSize = Measure(Label, Serif(12)) + FVector2D(16.f, 8.f);
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		FSlateDrawElement::MakeBox(Out, Layer + 8, Geometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(At)), White, ESlateDrawEffect::None, Gold.CopyWithNewOpacity(0.9f));
		PaintText(Geometry, Out, Layer + 9, Label, At + FVector2D(8.f, LabelSize.Y * 0.5f), Serif(12), FLinearColor::FromSRGBColor(FColor(30, 22, 12)), 0.f, false);
	}
}

void SCampaign1851Overlay::PaintTrainingMenu(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft) const
{
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const TArray<FCampaign1851Officer>& Officers = Map->GetOfficers();
	if (SelectedRegiments.Num() == 0 || !Regs.IsValidIndex(SelectedRegiments[0]))
	{
		return;
	}
	// Figures for the first selected unit (its skills and its chief); the cost for all selected.
	const FCampaign1851Regiment& R = Regs[SelectedRegiments[0]];
	const int32 Lead = Officers.IsValidIndex(R.Chief) ? Officers[R.Chief].Stat(ECampaign1851OfficerStat::Leadership) : 3;
	float Men = 0.f;
	for (int32 i : SelectedRegiments)
	{
		Men += Regs.IsValidIndex(i) ? Regs[i].Men : 0;
	}
	const int32 NumPrograms = int32(ECampaign1851Program::Count);
	const float RowHeight = 50.f;
	const FVector2D Size(600.f, 96.f + NumPrograms * RowHeight + 50.f);
	const FVector2D Pos(BottomLeft.X, BottomLeft.Y - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseTraining);
	PaintText(Geometry, Out, Layer + 2, TEXT("Ø V E L S E R"), Pos + FVector2D(22.f, 26.f), Serif(11), Gold, 0.f, false);
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s%s  ·  chefens føring %d (loft %d)"), *R.Name,
		SelectedRegiments.Num() > 1 ? *FString::Printf(TEXT(" m.fl. (%d)"), SelectedRegiments.Num()) : TEXT(""), Lead, 60 + 4 * Lead),
		Pos + FVector2D(Size.X - 56.f, 26.f), Serif(11, EFace::Italic), MutedInk, Size.X - 230.f, 1.f);
	PaintText(Geometry, Out, Layer + 2, TEXT("Program"), Pos + FVector2D(26.f, 58.f), Serif(10, EFace::Italic), Gold, 0.f, false);
	PaintText(Geometry, Out, Layer + 2, TEXT("Træner (dage til +10)"), Pos + FVector2D(190.f, 58.f), Serif(10, EFace::Italic), Gold, 0.f, false);
	PaintText(Geometry, Out, Layer + 2, TEXT("Pris / md."), Pos + FVector2D(Size.X - 30.f, 58.f), Serif(10, EFace::Italic), Gold, 1.f, false);
	static const TCHAR* Notes[] = {
		TEXT("Ingen udgifter. Godt efter en lang march."),
		TEXT("Formationer, vendinger og ladegreb på eksercerpladsen."),
		TEXT("Skarpe skud: krudt og kugler er den dyre del."),
		TEXT("Dækning, skirmish og terræn; slider på støvler og udstyr."),
		TEXT("Lange marcher i fuld oppakning (rideøvelser for rytteriet)."),
		TEXT("Stormangreb og bajonetfægtning."),
		TEXT("Lidt af det hele; holder alt ved lige, intet i top.") };
	for (int32 p = 0; p < NumPrograms; ++p)
	{
		const ECampaign1851Program Program = ECampaign1851Program(p);
		const FVector2D Row = Pos + FVector2D(16.f, 74.f + p * RowHeight);
		const bool bCurrent = R.Program == Program;
		PaintButton(Geometry, Out, Layer + 2, Row, FVector2D(Size.X - 32.f, RowHeight - 6.f), FString(), EButton::ProgramPick, p);
		if (bCurrent)
		{
			DrawLines(Geometry, Out, Layer + 3, { Row, Row + FVector2D(Size.X - 32.f, 0.f), Row + FVector2D(Size.X - 32.f, RowHeight - 6.f), Row + FVector2D(0.f, RowHeight - 6.f), Row }, Gold, 2.5f);
		}
		PaintText(Geometry, Out, Layer + 4, Campaign1851Army::ProgramName(Program), Row + FVector2D(10.f, 14.f), Serif(13), bCurrent ? Gold : Ink, 0.f, false);
		// What it trains, strongest first, with the days it takes to gain 10.
		TArray<TPair<float, int32>> Parts;
		for (int32 s = 0; s < int32(ECampaign1851Skill::Count); ++s)
		{
			const float Weight = Campaign1851Army::ProgramWeight(Program, ECampaign1851Skill(s));
			if (Weight > 0.f)
			{
				Parts.Add({ Weight, s });
			}
		}
		Parts.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key > B.Key; });
		FString Trains;
		for (const TPair<float, int32>& Part : Parts)
		{
			const ECampaign1851Skill Skill = ECampaign1851Skill(Part.Value);
			const float Days = Campaign1851Army::DaysForTen(Program, Skill, R.Skill(Skill), Lead);
			Trains += FString::Printf(TEXT("%s%s %s"), Trains.IsEmpty() ? TEXT("") : TEXT(",  "), Campaign1851Army::SkillName(Skill),
				Days > 0.f ? *FString::Printf(TEXT("(%.0f d.)"), Days) : TEXT("(ved loftet)"));
		}
		if (Parts.Num() == 0)
		{
			Trains = TEXT("intet; værdierne falder langsomt, udholdenhed hurtigst");
		}
		PaintTextFit(Geometry, Out, Layer + 4, Trains, Row + FVector2D(174.f, 14.f), Serif(11), Ink, Size.X - 32.f - 174.f - 90.f);
		const int32 Cost = FMath::RoundToInt(Campaign1851Army::ProgramCostPerMonth(Program) * Men / 760.f);
		PaintText(Geometry, Out, Layer + 4, Cost > 0 ? FString::Printf(TEXT("%s rd."), *Thousands(Cost)) : FString(TEXT("gratis")), Row + FVector2D(Size.X - 46.f, 14.f), Serif(12), Ink, 1.f, false);
		PaintTextFit(Geometry, Out, Layer + 4, Notes[p], Row + FVector2D(10.f, 32.f), Serif(10, EFace::Italic), MutedInk, Size.X - 60.f);
	}
	PaintText(Geometry, Out, Layer + 2, TEXT("Kun i garnison  ·  en bedre chef træner hurtigere og højere"), FVector2D(Pos.X + 22.f, Pos.Y + Size.Y - 30.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - 122.f, Pos.Y + Size.Y - 44.f), FVector2D(100.f, 28.f), TEXT("LUK"), EButton::PickerClose);
}

FString SCampaign1851Overlay::OfficerStatLine(const FCampaign1851Officer& O) const
{
	FString Line;
	for (int32 s = 0; s < int32(ECampaign1851OfficerStat::Count); ++s)
	{
		if (s == int32(ECampaign1851OfficerStat::Political) && !O.bGeneral)
		{
			continue;   // political weight only matters for generals
		}
		Line += FString::Printf(TEXT("%s%s %d"), s ? TEXT("  ") : TEXT(""), Campaign1851Army::StatShort(ECampaign1851OfficerStat(s)), O.Stats[s]);
	}
	return Line + FString::Printf(TEXT("  ·  erf. %.0f"), O.Experience);
}

void SCampaign1851Overlay::PaintOfficerPicker(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft) const
{
	const bool bGenerals = Picker == EPicker::General || Picker == EPicker::CommandGeneral || Picker == EPicker::FormationGeneral;
	TArray<int32> Pool = Map->OfficerPool(bGenerals);
	if (Picker == EPicker::FormationGeneral || Picker == EPicker::FormationOfficer)
	{
		Pool.RemoveAll([&](int32 OOBOfficer) { return !Map->CanAssignFormationPost(OOBOfficer, PickerFormation, PickerPost); });
	}
	const TArray<FCampaign1851Officer>& Officers = Map->GetOfficers();
	const int32 Rows = FMath::Min(Pool.Num(), 10);
	const float RowHeight = 44.f;
	const FVector2D Size(540.f, 60.f + FMath::Max(Rows, 1) * RowHeight + 56.f);
	const FVector2D Pos(BottomLeft.X, BottomLeft.Y - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), ClosePicker);
	FString Heading = bGenerals ? TEXT("L E D I G E   G E N E R A L E R") : TEXT("L E D I G E   O F F I C E R E R");
	const int32 PickerIndex = Map->FormationIndex(PickerFormation);
	if ((Picker == EPicker::FormationGeneral || Picker == EPicker::FormationOfficer) && PickerIndex != INDEX_NONE)
	{
		const FCampaign1851Formation& F = Map->GetFormations()[PickerIndex];
		Heading = FString::Printf(TEXT("%s, %s"), Campaign1851Army::StaffPostName(F.Echelon, PickerPost), *F.Name);
	}
	PaintTextFit(Geometry, Out, Layer + 2, Heading, Pos + FVector2D(22.f, 26.f), Serif(11), Gold, Size.X - 220.f);
	PaintText(Geometry, Out, Layer + 2, TEXT("Klik for at se og udnævne"), Pos + FVector2D(Size.X - 56.f, 26.f), Serif(10, EFace::Italic), MutedInk, 1.f, false);
	if (Pool.Num() == 0)
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("Ingen ledige; rekruttér en ny"), Pos + FVector2D(22.f, 60.f + RowHeight * 0.5f), Serif(12, EFace::Italic), MutedInk, 0.f, false);
	}
	for (int32 r = 0; r < Rows; ++r)
	{
		const FCampaign1851Officer& O = Officers[Pool[r]];
		const FVector2D Row = Pos + FVector2D(16.f, 48.f + r * RowHeight);
		PaintButton(Geometry, Out, Layer + 2, Row, FVector2D(Size.X - 32.f, RowHeight - 6.f), FString(), EButton::OfficerInfo, Pool[r]);
		PaintTextFit(Geometry, Out, Layer + 4, FString::Printf(TEXT("%s %s  %s  (f. %d)"), *O.Rank, *O.Name, *FString::ChrN(Campaign1851Army::Stars(O.Experience), TEXT('*')), O.Born),
			Row + FVector2D(10.f, 11.f), Serif(12), Ink, Size.X - 52.f);
		PaintTextFit(Geometry, Out, Layer + 4, OfficerStatLine(O), Row + FVector2D(10.f, 28.f), Serif(10), MutedInk, Size.X - 52.f);
	}
	const int32 Cost = Map->OfficerCost(bGenerals);
	const bool bAfford = Map->GetTreasury() >= Cost;
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, Pos.Y + Size.Y - 44.f), FVector2D(250.f, 28.f),
		FString::Printf(TEXT("REKRUTTÉR NY  (%s rd.)"), *Thousands(Cost)), EButton::OfficerRecruit, bGenerals ? 1 : 0, false, !bAfford);
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - 122.f, Pos.Y + Size.Y - 44.f), FVector2D(100.f, 28.f), TEXT("LUK"), EButton::PickerClose);
}

void SCampaign1851Overlay::PaintOfficerCard(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft) const
{
	const FCampaign1851Officer& O = Map->GetOfficers()[InspectedOfficer];
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	static const TCHAR* Meaning[] = {
		TEXT("moral, genrejsning, får underordnede til at handle"),
		TEXT("løfter og stabiliserer troppernes kampvilje"),
		TEXT("handler inden for hensigten uden ny ordre"),
		TEXT("flanker, terræn, reserver og timing"),
		TEXT("ordrer, rapporter, koordination; kolonnens march"),
		TEXT("følger den konkrete ordre"),
		TEXT("1 forsigtig ... 10 dristig: angreb og forfølgelse"),
		TEXT("bevarer overblikket under pres"),
		TEXT("udnævnelser, afskedigelser og prestige"),
		TEXT("opklaring, dækker flankerne, går ikke i en fælde") };
	const int32 CardStats = int32(ECampaign1851OfficerStat::Count);
	const float RowHeight = 31.f;
	const float PortraitH = 224.f;
	const FVector2D Size(540.f, 52.f + PortraitH + 18.f + (CardStats - (O.bGeneral ? 0 : 1)) * RowHeight + 84.f);
	const FVector2D Pos(BottomLeft.X, FMath::Max(70.f, BottomLeft.Y - Size.Y));
	// In the front, opaque, and it takes the clicks.
	Buttons.Add({ Pos, Pos + Size, EButton::Block, 0 });
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(0.045f, 0.035f, 0.025f, 1.f));
	PaintPanel(Geometry, Out, Layer + 1, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseOfficerCard);
	// Left: the portrait in its frame. Right: the name, rank, post, rating and experience.
	const FVector2D Picture(Pos.X + 22.f, Pos.Y + 46.f);
	PaintPortraitBox(Geometry, Out, Layer + 2, Picture, FVector2D(178.f, PortraitH), O.Name, O.bGeneral ? 1 : 0, Map->GetDate().GetYear() - O.Born, Campaign1851Army::RankIndex(O.Rank));
	const float TX = Picture.X + 198.f, TW = Size.X - (TX - Pos.X) - 24.f;
	float Y = Picture.Y + 8.f;
	PaintTextFit(Geometry, Out, Layer + 2, O.Name, FVector2D(TX, Y), Serif(22), Ink, TW - 40.f);
	Y += 30.f;
	const int32 Age = Map->GetDate().GetYear() - O.Born;
	const FString Post = Map->OfficerRole(InspectedOfficer);
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s  ·  %d år"), *O.Rank, Age), FVector2D(TX, Y), Serif(14, EFace::Italic), Gold, TW);
	Y += 24.f;
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s%s"), *Post, O.bRecruited ? TEXT("  ·  ansat under felttoget") : TEXT("")), FVector2D(TX, Y), Serif(12), Ink, TW);
	Y += 34.f;
	// The overall rating, big.
	{
		const int32 Rating = Campaign1851Army::OfficerRating(O);
		PaintText(Geometry, Out, Layer + 2, FString::FromInt(Rating), FVector2D(TX + 4.f, Y - 6.f), Serif(40), Rating >= 70 ? Gold : Rating >= 45 ? Ink : MutedInk, 0.f, false);
		PaintText(Geometry, Out, Layer + 2, TEXT("samlet vurdering"), FVector2D(TX + 80.f, Y + 12.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
		AddTip(FVector2D(TX, Y - 10.f), FVector2D(TW, 54.f), TEXT("Samlet vurdering 0-100: evnerne vægtet efter betydning i felten (føring og taktik tungest; aggressivitet bedst midt imellem; politisk vægt kun for generaler) plus op til 10 for erfaring. 70+ er fremragende, under 45 svag."));
	}
	Y += 56.f;
	PaintText(Geometry, Out, Layer + 2, TEXT("Erfaring"), FVector2D(TX, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
	PaintBar(Geometry, Out, Layer + 2, FVector2D(TX + 66.f, Y - 3.f), TW - 130.f, O.Experience / 100.f);
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%.0f %s"), O.Experience, *FString::ChrN(Campaign1851Army::Stars(O.Experience), TEXT('*'))), FVector2D(TX + TW, Y), Serif(12), Ink, 1.f, false);
	Y = Picture.Y + PortraitH + 22.f;
	DrawLines(Geometry, Out, Layer + 2, { FVector2D(Pos.X + 22.f, Y - 10.f), FVector2D(Pos.X + Size.X - 22.f, Y - 10.f) }, Gold.CopyWithNewOpacity(0.35f), 1.f);
	for (int32 s = 0; s < CardStats; ++s)
	{
		if (s == int32(ECampaign1851OfficerStat::Political) && !O.bGeneral)
		{
			continue;   // only generals have political weight
		}
		PaintText(Geometry, Out, Layer + 2, Campaign1851Army::StatName(ECampaign1851OfficerStat(s)), FVector2D(Pos.X + 22.f, Y), Serif(12), Ink, 0.f, false);
		AddTip(FVector2D(Pos.X + 16.f, Y - 12.f), FVector2D(Size.X - 32.f, RowHeight - 3.f), FString::Printf(TEXT("%s: %s (1-10)"), Campaign1851Army::StatName(ECampaign1851OfficerStat(s)), Meaning[s]));
		PaintBar(Geometry, Out, Layer + 2, FVector2D(Pos.X + 150.f, Y - 3.f), 230.f, O.Stats[s] / 10.f);
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%d"), O.Stats[s]), FVector2D(Pos.X + 396.f, Y), Serif(12), Ink, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 2, Meaning[s], FVector2D(Pos.X + 150.f, Y + 13.f), Serif(9, EFace::Italic), MutedInk, 236.f);
		Y += RowHeight;
	}
	// Appoint from the list (the officer is free and fits the post being filled), or back / close.
	const bool bCanAppoint = O.IsFree() && Picker != EPicker::None && (Picker == EPicker::General || Picker == EPicker::CommandGeneral || Picker == EPicker::FormationGeneral) == O.bGeneral;
	const float ButtonY = Pos.Y + Size.Y - 46.f;
	// Promotion: one rank at a time, when his experience allows it.
	const FString Block = Map->PromotionBlock(InspectedOfficer);
	const int32 Next = Campaign1851Army::RankIndex(O.Rank) + 1;
	if (Block.IsEmpty())
	{
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 182.f, ButtonY), FVector2D(150.f, 28.f), TEXT("FORFREM"), EButton::OfficerPromote, InspectedOfficer);
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("til %s (løn %s rd./år)"), *Campaign1851Army::Ranks()[Next], *Thousands(Campaign1851Army::RankPay(Next))),
			FVector2D(Pos.X + 22.f, ButtonY - 12.f), Serif(10, EFace::Italic), MutedInk, Size.X - 44.f);
	}
	else
	{
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("Forfremmelse: %s"), *Block), FVector2D(Pos.X + 22.f, ButtonY - 12.f), Serif(10, EFace::Italic), MutedInk, Size.X - 44.f);
	}
	if (bCanAppoint)
	{
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, ButtonY), FVector2D(150.f, 28.f), TEXT("UDNÆVN"), EButton::OfficerPick, InspectedOfficer);
	}
	if (!bCanAppoint && O.IsFree())
	{
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, ButtonY), FVector2D(150.f, 28.f), TEXT("AFSKED"), EButton::OfficerDismiss, InspectedOfficer);
	}
	if (O.Away == 2)
	{
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, ButtonY), FVector2D(250.f, 28.f), FString::Printf(TEXT("LØSEKØB (%s rd.)"), *Thousands(Map->RansomCost(InspectedOfficer))), EButton::Ransom, InspectedOfficer);
	}
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Size.X - 122.f, ButtonY), FVector2D(100.f, 28.f), Picker != EPicker::None ? TEXT("TILBAGE") : TEXT("LUK"), EButton::OfficerCardClose);
}

// ------------------------------------------------------------------ main windows (army, officers, treasury, towns)

void SCampaign1851Overlay::PaintCloseX(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& TopRight, int32 What) const
{
	PaintButton(Geometry, Out, Layer, TopRight + FVector2D(-34.f, 8.f), FVector2D(26.f, 24.f), TEXT("X"), EButton::ClosePanel, What);
}

void SCampaign1851Overlay::PaintMenuBar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	// Under the calendar: one window at a time, the open one lit.
	const TCHAR* Labels[] = { TEXT("HÆREN"), TEXT("OFFICERER"), TEXT("KASSEN"), TEXT("BYER"), TEXT("TOG"), TEXT("KAMPORDEN"), TEXT("STATSRÅD"), TEXT("FORSYNING"), TEXT("UDENRIGS"), TEXT("FORSKNING"), TEXT("FLÅDEN"), TEXT("LANDE") };
	const EWindow Opens[] = { EWindow::Army, EWindow::Officers, EWindow::Budget, EWindow::Towns, EWindow::Trains, EWindow::Chart, EWindow::Council, EWindow::Supply, EWindow::Foreign, EWindow::Research, EWindow::Navy, EWindow::Nations };
	// Small square buttons: a drawn symbol above, the name small below.
	const float W = 74.f, H = 46.f, Gap = 3.f;
	const float Total = 12.f * W + 11.f * Gap;
	const float X0 = FMath::Max((Geometry.GetLocalSize().X - Total) * 0.5f, 484.f);
	const FLinearColor Dark = FLinearColor::FromSRGBColor(FColor(30, 22, 12));
	for (int32 i = 0; i < 12; ++i)
	{
		const bool bOn = Window == Opens[i];
		const FVector2D P(X0 + i * (W + Gap), 90.f);
		PaintButton(Geometry, Out, Layer, P, FVector2D(W, H), FString(), EButton::MainMenu, int32(Opens[i]), bOn);
		PaintMenuIcon(Geometry, Out, Layer + 2, i, P + FVector2D(W * 0.5f, 17.f), 11.f, bOn ? Dark : Ink);
		PaintTextFit(Geometry, Out, Layer + 2, Labels[i], FVector2D(P.X + W * 0.5f - FMath::Min(Measure(Labels[i], Serif(8)).X, W - 6.f) * 0.5f, P.Y + H - 9.f), Serif(8), bOn ? Dark : Gold, W - 6.f);
	}
}

void SCampaign1851Overlay::PaintNations(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	// Every country of the campaign (the list of countries and uniforms 1851-1866), by group: who runs it,
	// its figures, its standing towards Denmark and its treaties, and the types of its three arms.
	const TArray<FCampaign1851Nation>& Nations = Map->GetNations();
	PaintText(Geometry, Out, Layer + 1, TEXT("Landene"), Pos + FVector2D(24.f, 34.f), Serif(22), Ink, 0.f);
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Kampagnens lande efter gruppe  ·  * abstrakt model (skøn): landet har intet kort endnu og vokser efter sine tal"),
		Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Size.X - 170.f);
	const float X = Pos.X + 24.f;
	float Y = Pos.Y + 104.f;
	const float Cols[] = { 0.f, 190.f, 380.f, 470.f, 545.f, 645.f, 745.f, 840.f, 910.f, 975.f, 1170.f };
	const TCHAR* Heads[] = { TEXT("Land"), TEXT("Rolle"), TEXT("Periode"), TEXT("Styres af"), TEXT("Befolkning"), TEXT("Udv.-budget"), TEXT("Hær"), TEXT("Bane km"), TEXT("Vækst"), TEXT("Forhold til Danmark"), TEXT("Aftaler") };
	for (int32 c = 0; c < UE_ARRAY_COUNT(Heads); ++c)
	{
		PaintText(Geometry, Out, Layer + 1, Heads[c], FVector2D(X + Cols[c], Y), Serif(10, EFace::Italic), MutedInk, 0.f, false);
	}
	Y += 22.f;
	struct FGroup { const TCHAR* Key; const TCHAR* Name; };
	const FGroup Groups[] = { { TEXT("norden"), TEXT("Norden") }, { TEXT("stormagt"), TEXT("Stormagterne") }, { TEXT("tysk"), TEXT("De tyske stater") }, { TEXT("vest"), TEXT("Vesteuropa") } };
	TArray<bool> Done;
	Done.Init(false, Nations.Num());
	const float RowH = 40.f;
	for (int32 g = 0; g <= UE_ARRAY_COUNT(Groups) && Y < Pos.Y + Size.Y - 40.f; ++g)
	{
		bool bHeading = false;
		for (int32 n = 0; n < Nations.Num() && Y < Pos.Y + Size.Y - 40.f; ++n)
		{
			const FCampaign1851Nation& N = Nations[n];
			if (Done[n] || (g < UE_ARRAY_COUNT(Groups) && N.Group != Groups[g].Key))
			{
				continue;
			}
			Done[n] = true;
			if (!bHeading)
			{
				const FString Name = g < UE_ARRAY_COUNT(Groups) ? FString(Groups[g].Name) : FString(TEXT("Øvrige"));
				PaintText(Geometry, Out, Layer + 1, Name.ToUpper().Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("å"), TEXT("Å")), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
				DrawLines(Geometry, Out, Layer + 1, { FVector2D(X + 150.f, Y), FVector2D(Pos.X + Size.X - 30.f, Y) }, Gold.CopyWithNewOpacity(0.25f), 1.f);
				Y += 20.f;
				bHeading = true;
			}
			const FCampaign1851NationFigures F = Map->NationFigures(n);
			const FLinearColor C = !N.bActive ? MutedInk : N.IsPlayer() ? Gold : Ink;
			PaintTextFit(Geometry, Out, Layer + 1, N.Name + (N.bOnMap ? TEXT("") : TEXT(" *")), FVector2D(X + Cols[0], Y), Serif(13), C, 185.f);
			PaintTextFit(Geometry, Out, Layer + 1, N.Role, FVector2D(X + Cols[1], Y), Serif(11), C, 185.f);
			PaintText(Geometry, Out, Layer + 1, N.Period, FVector2D(X + Cols[2], Y), Serif(11), C, 0.f, false);
			PaintText(Geometry, Out, Layer + 1, !N.bActive ? TEXT("ikke aktiv") : N.IsPlayer() ? TEXT("Spilleren") : TEXT("AI"), FVector2D(X + Cols[3], Y), Serif(11), C, 0.f, false);
			if (N.bActive)
			{
				PaintText(Geometry, Out, Layer + 1, Thousands(int32(F.Population)), FVector2D(X + Cols[4], Y), Serif(12), Ink, 0.f, false);
				PaintText(Geometry, Out, Layer + 1, Thousands(int32(F.YearlyBudget)), FVector2D(X + Cols[5], Y), Serif(12), Ink, 0.f, false);
				PaintText(Geometry, Out, Layer + 1, Thousands(int32(F.ArmyMen)), FVector2D(X + Cols[6], Y), Serif(12), Ink, 0.f, false);
				PaintText(Geometry, Out, Layer + 1, Thousands(int32(F.RailKm)), FVector2D(X + Cols[7], Y), Serif(12), Ink, 0.f, false);
				PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%.1f %%"), F.Growth), FVector2D(X + Cols[8], Y), Serif(12), Ink, 0.f, false);
				if (!N.IsPlayer())
				{
					const FLinearColor RelColour = N.Relation >= 40.f ? FLinearColor(0.45f, 0.75f, 0.4f) : N.Relation >= 0.f ? Ink : FLinearColor(0.95f, 0.4f, 0.35f);
					PaintBar(Geometry, Out, Layer + 1, FVector2D(X + Cols[9], Y - 5.f), 140.f, (N.Relation + 100.f) / 200.f);
					PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%+.0f"), N.Relation), FVector2D(X + Cols[9] + 150.f, Y), Serif(12), RelColour, 0.f, false);
					TArray<FString> Pacts;
					if (N.bTrade) { Pacts.Add(TEXT("handel")); }
					if (N.bAlliance) { Pacts.Add(TEXT("alliance")); }
					if (N.bGuarantee) { Pacts.Add(TEXT("garanti")); }
					PaintTextFit(Geometry, Out, Layer + 1, Pacts.Num() > 0 ? FString::Join(Pacts, TEXT(", ")) : FString(TEXT("-")), FVector2D(X + Cols[10], Y), Serif(11), Ink, Pos.X + Size.X - 40.f - X - Cols[10]);
				}
			}
			// The arms' types (uniforms), small under the line (none listed: no line).
			if (!(N.Infantry.IsEmpty() && N.Cavalry.IsEmpty() && N.Artillery.IsEmpty()))
			PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("Fodfolk: %s   ·   Rytteri: %s   ·   Artilleri: %s"), *N.Infantry, *N.Cavalry, *N.Artillery),
				FVector2D(X + 12.f, Y + 17.f), Serif(9, EFace::Italic), MutedInk, Size.X - 80.f);
			Y += RowH;
		}
	}
}

void SCampaign1851Overlay::PaintMenuIcon(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, int32 Index, const FVector2D& C, float S, const FLinearColor& Colour) const
{
	auto L = [&](std::initializer_list<FVector2D> Pts, float Thick = 1.6f)
	{
		TArray<FVector2D> P;
		for (const FVector2D& Q : Pts) { P.Add(C + Q * S); }
		DrawLines(Geometry, Out, Layer, P, Colour, Thick);
	};
	auto Circle = [&](const FVector2D& At, float R, int32 N = 14)
	{
		TArray<FVector2D> P;
		for (int32 a = 0; a <= N; ++a) { P.Add(C + (At + FVector2D(FMath::Cos(a * UE_TWO_PI / N), FMath::Sin(a * UE_TWO_PI / N)) * R) * S); }
		DrawLines(Geometry, Out, Layer, P, Colour, 1.6f);
	};
	switch (Index)
	{
	case 0:   // the army: two crossed sabres
		L({ FVector2D(-0.9f, 0.9f), FVector2D(0.8f, -0.8f) }, 2.f); L({ FVector2D(0.9f, 0.9f), FVector2D(-0.8f, -0.8f) }, 2.f);
		L({ FVector2D(-0.9f, 0.5f), FVector2D(-0.5f, 0.9f) }); L({ FVector2D(0.9f, 0.5f), FVector2D(0.5f, 0.9f) });
		break;
	case 1:   // officers: an epaulette with a star
		L({ FVector2D(-0.9f, 0.3f), FVector2D(-0.6f, -0.3f), FVector2D(0.6f, -0.3f), FVector2D(0.9f, 0.3f), FVector2D(-0.9f, 0.3f) });
		for (float x = -0.7f; x <= 0.71f; x += 0.35f) { L({ FVector2D(x, 0.3f), FVector2D(x, 0.8f) }, 1.f); }
		L({ FVector2D(0.f, -0.95f), FVector2D(0.f, -0.45f) }); L({ FVector2D(-0.25f, -0.7f), FVector2D(0.25f, -0.7f) });
		break;
	case 2:   // the treasury: a stack of coins
		for (int32 k = 0; k < 3; ++k) { const float y = 0.6f - k * 0.45f; L({ FVector2D(-0.8f, y), FVector2D(0.8f, y), FVector2D(0.8f, y - 0.3f), FVector2D(-0.8f, y - 0.3f), FVector2D(-0.8f, y) }); }
		break;
	case 3:   // towns: a house with a gable
		L({ FVector2D(-0.8f, 0.9f), FVector2D(-0.8f, -0.1f), FVector2D(0.f, -0.9f), FVector2D(0.8f, -0.1f), FVector2D(0.8f, 0.9f), FVector2D(-0.8f, 0.9f) });
		L({ FVector2D(-0.2f, 0.9f), FVector2D(-0.2f, 0.3f), FVector2D(0.2f, 0.3f), FVector2D(0.2f, 0.9f) });
		break;
	case 4:   // trains: a locomotive
		L({ FVector2D(-0.9f, 0.4f), FVector2D(-0.9f, -0.2f), FVector2D(0.3f, -0.2f), FVector2D(0.3f, -0.7f), FVector2D(0.9f, -0.7f), FVector2D(0.9f, 0.4f), FVector2D(-0.9f, 0.4f) });
		L({ FVector2D(-0.6f, -0.2f), FVector2D(-0.6f, -0.8f) }, 2.f);
		Circle(FVector2D(-0.45f, 0.65f), 0.25f, 10); Circle(FVector2D(0.5f, 0.65f), 0.25f, 10);
		break;
	case 5:   // order of battle: a small organisation chart
		L({ FVector2D(-0.3f, -0.95f), FVector2D(0.3f, -0.95f), FVector2D(0.3f, -0.55f), FVector2D(-0.3f, -0.55f), FVector2D(-0.3f, -0.95f) });
		L({ FVector2D(0.f, -0.55f), FVector2D(0.f, -0.2f) }); L({ FVector2D(-0.7f, -0.2f), FVector2D(0.7f, -0.2f) });
		for (float x : { -0.7f, 0.7f })
		{
			L({ FVector2D(x, -0.2f), FVector2D(x, 0.2f) });
			L({ FVector2D(x - 0.25f, 0.2f), FVector2D(x + 0.25f, 0.2f), FVector2D(x + 0.25f, 0.6f), FVector2D(x - 0.25f, 0.6f), FVector2D(x - 0.25f, 0.2f) });
		}
		break;
	case 6:   // council of state: a crown
		L({ FVector2D(-0.8f, 0.6f), FVector2D(-0.9f, -0.5f), FVector2D(-0.4f, 0.f), FVector2D(0.f, -0.8f), FVector2D(0.4f, 0.f), FVector2D(0.9f, -0.5f), FVector2D(0.8f, 0.6f), FVector2D(-0.8f, 0.6f) });
		break;
	case 7:   // supply: a covered wagon
		L({ FVector2D(-0.9f, 0.4f), FVector2D(0.9f, 0.4f) }, 2.f);
		L({ FVector2D(-0.8f, 0.4f), FVector2D(-0.8f, -0.3f), FVector2D(-0.4f, -0.7f), FVector2D(0.4f, -0.7f), FVector2D(0.8f, -0.3f), FVector2D(0.8f, 0.4f) });
		Circle(FVector2D(-0.5f, 0.65f), 0.25f, 10); Circle(FVector2D(0.5f, 0.65f), 0.25f, 10);
		break;
	case 8:   // foreign affairs: a globe
		Circle(FVector2D::ZeroVector, 0.85f, 18);
		L({ FVector2D(-0.85f, 0.f), FVector2D(0.85f, 0.f) }, 1.f); L({ FVector2D(0.f, -0.85f), FVector2D(0.f, 0.85f) }, 1.f);
		L({ FVector2D(-0.4f, -0.75f), FVector2D(-0.5f, 0.f), FVector2D(-0.4f, 0.75f) }, 1.f); L({ FVector2D(0.4f, -0.75f), FVector2D(0.5f, 0.f), FVector2D(0.4f, 0.75f) }, 1.f);
		break;
	case 9:   // research: a flask
		L({ FVector2D(-0.2f, -0.9f), FVector2D(-0.2f, -0.3f), FVector2D(-0.8f, 0.8f), FVector2D(0.8f, 0.8f), FVector2D(0.2f, -0.3f), FVector2D(0.2f, -0.9f) });
		L({ FVector2D(-0.35f, -0.9f), FVector2D(0.35f, -0.9f) }); L({ FVector2D(-0.55f, 0.35f), FVector2D(0.55f, 0.35f) }, 1.f);
		break;
	case 11:  // the countries: a flag on a pole over a hill
		L({ FVector2D(-0.5f, 0.85f), FVector2D(-0.5f, -0.85f) }, 1.6f);
		L({ FVector2D(-0.5f, -0.85f), FVector2D(0.7f, -0.6f), FVector2D(-0.5f, -0.2f) });
		L({ FVector2D(-0.9f, 0.85f), FVector2D(0.9f, 0.85f) });
		break;
	default:  // the navy: an anchor
		L({ FVector2D(0.f, -0.7f), FVector2D(0.f, 0.85f) }, 2.f); L({ FVector2D(-0.4f, -0.45f), FVector2D(0.4f, -0.45f) });
		Circle(FVector2D(0.f, -0.85f), 0.15f, 8);
		L({ FVector2D(-0.8f, 0.3f), FVector2D(-0.6f, 0.7f), FVector2D(0.f, 0.85f), FVector2D(0.6f, 0.7f), FVector2D(0.8f, 0.3f) });
		break;
	}
}

void SCampaign1851Overlay::PaintTable(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, int32 VisibleRows,
	const TArray<FTableColumn>& Columns, TArray<FTableRow> Rows, EButton RowAction, int32 Highlight) const
{
	const float RowHeight = 23.f;
	float Width = 0.f;
	for (const FTableColumn& C : Columns)
	{
		Width += C.Width;
	}
	// Sort by the chosen column: numbers by value, text alphabetically.
	const int32 Col = FMath::Clamp(SortColumn, 0, Columns.Num() - 1);
	Rows.StableSort([Col, this](const FTableRow& A, const FTableRow& B)
	{
		const bool bNumber = A.Keys.IsValidIndex(Col) && !FMath::IsNaN(A.Keys[Col]);
		const bool bLess = bNumber ? A.Keys[Col] < B.Keys[Col] : A.Cells[Col] < B.Cells[Col];
		const bool bMore = bNumber ? A.Keys[Col] > B.Keys[Col] : B.Cells[Col] < A.Cells[Col];
		return bSortDesc ? bMore : bLess;
	});
	// Headings: click to sort (again to turn the order round).
	float X = Pos.X;
	for (int32 c = 0; c < Columns.Num(); ++c)
	{
		const FTableColumn& C = Columns[c];
		const FString Head = C.Title + (c == Col ? (bSortDesc ? TEXT(" v") : TEXT(" ^")) : TEXT(""));
		PaintTextFit(Geometry, Out, Layer + 2, Head, FVector2D(C.bRight ? X + C.Width - 6.f : X + 6.f, Pos.Y + 11.f), Serif(11, EFace::Italic), c == Col ? Ink : Gold, C.Width - 8.f, C.bRight ? 1.f : 0.f);
		Buttons.Add({ FVector2D(X, Pos.Y), FVector2D(X + C.Width, Pos.Y + 22.f), EButton::TableSort, c });
		// What a short heading means.
		{
			static const TMap<FString, FString> Glossary = {
				{ TEXT("Erf"), TEXT("Erfaring 0-100: felttjeneste og slag (veteraner fra 1848-50 starter omkring 55)") },
				{ TEXT("Lad"), TEXT("Ladegreb og ilddisciplin: ladetid, ordnede salver, ammunitionsforbrug") },
				{ TEXT("Skyd"), TEXT("Skydning: træfsikkerhed og afstandsbedømmelse (skyttekunst for artilleriet)") },
				{ TEXT("Eks"), TEXT("Eksercits: formationer, vendinger og udfoldning; holder geledderne") },
				{ TEXT("Felt"), TEXT("Feltøvelse: dækning, spredt orden og brug af terrænet") },
				{ TEXT("Udh"), TEXT("Udholdenhed: marchtempo, træthed og restitution") },
				{ TEXT("Baj"), TEXT("Bajonet og storm: chokket og nærkampen") },
				{ TEXT("Samh"), TEXT("Samhørighed 0-100: hvor godt enheden hænger sammen (falder på lange marcher)") },
				{ TEXT("Moral"), TEXT("Moral: kampviljen (chefens inspiration og træningen løfter den)") },
				{ TEXT("Syge"), TEXT("Syge og sårede på lazaret: de fleste kommer tilbage inden for uger") },
				{ TEXT("Vurd"), TEXT("Samlet vurdering 0-100 ud fra evnerne og erfaringen") },
				{ TEXT("Løn"), TEXT("Løn i rigsdaler om året") } };
			FString Tip;
			if (const FString* Found = Glossary.Find(C.Title))
			{
				Tip = *Found;
			}
			for (int32 st = 0; st < int32(ECampaign1851OfficerStat::Count) && Tip.IsEmpty(); ++st)
			{
				if (C.Title == Campaign1851Army::StatShort(ECampaign1851OfficerStat(st)))
				{
					Tip = FString::Printf(TEXT("%s (1-10)"), Campaign1851Army::StatName(ECampaign1851OfficerStat(st)));
				}
			}
			if (!Tip.IsEmpty())
			{
				AddTip(FVector2D(X, Pos.Y), FVector2D(C.Width, 22.f), Tip);
			}
		}
		X += C.Width;
	}
	DrawLines(Geometry, Out, Layer + 2, { FVector2D(Pos.X, Pos.Y + 24.f), FVector2D(Pos.X + Width, Pos.Y + 24.f) }, Gold.CopyWithNewOpacity(0.5f), 1.f);
	const int32 Pages = FMath::Max(1, FMath::DivideAndRoundUp(Rows.Num(), VisibleRows));
	const int32 ShownPage = FMath::Clamp(Page, 0, Pages - 1);
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	for (int32 r = 0; r < VisibleRows; ++r)
	{
		const int32 Index = ShownPage * VisibleRows + r;
		if (!Rows.IsValidIndex(Index))
		{
			break;
		}
		const FTableRow& Row = Rows[Index];
		const float Y = Pos.Y + 28.f + r * RowHeight;
		const FVector2D Min(Pos.X, Y), Size(Width, RowHeight - 2.f);
		const FLinearColor Band = Row.Id == Highlight ? Gold.CopyWithNewOpacity(0.25f) : r % 2 ? FLinearColor(1.f, 1.f, 1.f, 0.035f) : FLinearColor(0.f, 0.f, 0.f, 0.f);
		FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Min)), White, ESlateDrawEffect::None, Band);
		if (RowAction != EButton::None)
		{
			Buttons.Add({ Min, Min + Size, RowAction, Row.Id });
		}
		X = Pos.X;
		for (int32 c = 0; c < Columns.Num() && c < Row.Cells.Num(); ++c)
		{
			const FTableColumn& C = Columns[c];
			PaintTextFit(Geometry, Out, Layer + 2, Row.Cells[c], FVector2D(C.bRight ? X + C.Width - 6.f : X + 6.f, Y + RowHeight * 0.5f - 1.f), Serif(12), Ink, C.Width - 8.f, C.bRight ? 1.f : 0.f);
			X += C.Width;
		}
	}
	if (Pages > 1)
	{
		const float Y = Pos.Y + 28.f + VisibleRows * RowHeight + 8.f;
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("side %d af %d  ·  %d rækker"), ShownPage + 1, Pages, Rows.Num()), FVector2D(Pos.X + Width - 230.f, Y + 12.f), Serif(11, EFace::Italic), MutedInk, 1.f, false);
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Width - 220.f, Y), FVector2D(104.f, 24.f), TEXT("FORRIGE"), EButton::TablePage, -1, false, ShownPage == 0);
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + Width - 108.f, Y), FVector2D(104.f, 24.f), TEXT("NÆSTE"), EButton::TablePage, 1, false, ShownPage >= Pages - 1);
	}
}

void SCampaign1851Overlay::PaintWindow(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const FVector2D Screen = Geometry.GetLocalSize();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Screen, FSlateLayoutTransform(FVector2D::ZeroVector)), White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	// The order-of-battle chart gets nearly the whole screen; the other windows a fixed size.
	const bool bBig = Window == EWindow::Chart || Window == EWindow::Materiel;
	const FVector2D Size(FMath::Min(bBig ? 1860.f : 1460.f, Screen.X - (bBig ? 40.f : 80.f)), FMath::Min(bBig ? 930.f : 820.f, Screen.Y - (bBig ? 140.f : 200.f)));
	const FVector2D Pos((Screen.X - Size.X) * 0.5f, 132.f);
	if (Window == EWindow::Chart)
	{
		// The order of battle is a work table: an opaque ground, so the text never lies over the map's drawing.
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, FLinearColor(0.05f, 0.04f, 0.03f, 1.f));
	}
	PaintPanel(Geometry, Out, Layer + 1, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseWindow);
	const float Inner = Size.X - 48.f;
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const TArray<FCampaign1851Officer>& Officers = Map->GetOfficers();
	const int32 VisibleRows = FMath::Max(8, FMath::FloorToInt((Size.Y - 170.f) / 23.f));
	auto Title = [&](const TCHAR* Text, const FString& Sub)
	{
		PaintText(Geometry, Out, Layer + 3, Text, Pos + FVector2D(24.f, 34.f), Serif(22), Ink, 0.f);
		PaintTextFit(Geometry, Out, Layer + 3, Sub, Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Inner - 140.f);
	};
	auto Num = [](double V) { return FString::Printf(TEXT("%.0f"), V); };
	const double Text = std::numeric_limits<double>::quiet_NaN();

	if (Window == EWindow::Army)
	{
		int32 Men = 0, Horses = 0, Guns = 0;
		TArray<FTableRow> Rows;
		for (int32 i = 0; i < Regs.Num(); ++i)
		{
			const FCampaign1851Regiment& R = Regs[i];
			Men += R.Men; Horses += R.Horses; Guns += R.Guns;
			const FCampaign1851Officer* Chief = Officers.IsValidIndex(R.Chief) ? &Officers[R.Chief] : nullptr;
			FString Where = R.IsMarching() ? FString::Printf(TEXT("→ %s"), *Map->DescribePlace(R.Destination(), R.DestinationKm())) : Map->DescribePlace(R.Town, R.Km);
			Where.ReplaceInline(TEXT("→"), TEXT("mod"));
			FTableRow Row;
			Row.Id = i;
			Row.Cells = { R.Name, Campaign1851Army::ArmName(R.Arm), Cities[R.Home].Name, Where, Num(R.Men), Num(R.Sick), Num(R.Experience),
				Num(R.Skills[0]), Num(R.Skills[1]), Num(R.Skills[2]), Num(R.Skills[3]), Num(R.Skills[4]), Num(R.Skills[5]),
				FString::Printf(TEXT("%.0f %%"), R.Morale * 100.f), Num(R.Cohesion), Chief ? Chief->Name : FString(TEXT("-")),
				R.IsMarching() ? FString(TEXT("på march")) : FString(Campaign1851Army::ProgramName(R.Program)) };
			Row.Keys = { Text, Text, Text, Text, double(R.Men), double(R.Sick), R.Experience, R.Skills[0], R.Skills[1], R.Skills[2], R.Skills[3], R.Skills[4], R.Skills[5], R.Morale, R.Cohesion, Text, Text };
			Rows.Add(Row);
		}
		int32 Present = 0;
		for (const FCampaign1851Regiment& R : Regs) { Present += R.PresentMen(); }
		Title(TEXT("Hæren"), FString::Printf(TEXT("%s  ·  %s af %s mand til stede  ·  %s heste  ·  %d kanoner  ·  lager: %s geværer, %d kanoner, %s heste"),
			Campaign1851Mobilisation::FootingName(Map->GetFooting()), *Thousands(Present), *Thousands(Men), *Thousands(Horses), Guns, *Thousands(Map->GetRifles()), Map->GetGunStock(), *Thousands(Map->GetHorseStock())));
		PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 300.f, 28.f), FVector2D(220.f, 28.f),
			Map->GetFooting() == ECampaign1851Footing::Peace ? FString::Printf(TEXT("MOBILISÉR  %s rd."), *Thousands(int32(Campaign1851Mobilisation::OrderCost))) : FString(TEXT("HJEMSEND")),
			EButton::Footing, 0, Map->GetFooting() != ECampaign1851Footing::Peace);
		PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 530.f, 28.f), FVector2D(220.f, 28.f), TEXT("HÆRENS STATUS"), EButton::MainMenu, int32(EWindow::ArmyStatus));
		const TArray<FTableColumn> Cols = { {TEXT("Enhed"), 170.f}, {TEXT("Våben"), 120.f}, {TEXT("Garnison"), 100.f}, {TEXT("Hvor"), 150.f},
			{TEXT("Mand"), 60.f, true}, {TEXT("Syge"), 50.f, true}, {TEXT("Erf"), 48.f, true}, {TEXT("Lad"), 46.f, true}, {TEXT("Skyd"), 50.f, true}, {TEXT("Eks"), 46.f, true},
			{TEXT("Felt"), 46.f, true}, {TEXT("Udh"), 46.f, true}, {TEXT("Baj"), 46.f, true}, {TEXT("Moral"), 64.f, true}, {TEXT("Samh"), 54.f, true},
			{TEXT("Chef"), 150.f}, {TEXT("Øvelser"), 110.f} };
		PaintTable(Geometry, Out, Layer + 2, Pos + FVector2D(24.f, 96.f), VisibleRows, Cols, Rows, EButton::TableRow, SelectedRegiments.Num() == 1 ? SelectedRegiments[0] : INDEX_NONE);
	}
	else if (Window == EWindow::Officers)
	{
		const TCHAR* Filters[] = { TEXT("ALLE"), TEXT("GENERALER"), TEXT("CHEFER"), TEXT("LEDIGE") };
		TArray<FTableRow> Rows;
		double Pay = 0.0;
		for (int32 i = 0; i < Officers.Num(); ++i)
		{
			const FCampaign1851Officer& O = Officers[i];
			const bool bFree = O.IsFree();
			if ((OfficerFilter == 1 && !O.bGeneral) || (OfficerFilter == 2 && (O.bGeneral || bFree)) || (OfficerFilter == 3 && !bFree))
			{
				continue;
			}
			const int32 Salary = Campaign1851Army::RankPay(Campaign1851Army::RankIndex(O.Rank));
			Pay += Salary;
			FTableRow Row;
			Row.Id = i;
			Row.Cells = { O.Name, O.Rank, FString::FromInt(Map->GetDate().GetYear() - O.Born),
				Map->OfficerRole(i) };
			Row.Keys = { Text, Text, double(Map->GetDate().GetYear() - O.Born), Text };
			for (int32 s = 0; s < int32(ECampaign1851OfficerStat::Count); ++s)
			{
				const bool bHidden = s == int32(ECampaign1851OfficerStat::Political) && !O.bGeneral;
				Row.Cells.Add(bHidden ? FString(TEXT("-")) : FString::FromInt(O.Stats[s]));
				Row.Keys.Add(bHidden ? 0 : O.Stats[s]);
			}
			Row.Cells.Add(Num(O.Experience));
			Row.Keys.Add(O.Experience);
			const int32 Rating = Campaign1851Army::OfficerRating(O);
			Row.Cells.Add(FString::FromInt(Rating));
			Row.Keys.Add(Rating);
			Row.Cells.Add(Thousands(Salary));
			Row.Keys.Add(Salary);
			Rows.Add(Row);
		}
		Title(TEXT("Officerskorpset"), FString::Printf(TEXT("%d officerer  ·  løn %s rd./år  ·  klik på en officer for hans kort"), Rows.Num(), *Thousands(FMath::RoundToInt(Pay))));
		for (int32 f = 0; f < 4; ++f)
		{
			PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 56.f - (4 - f) * 128.f, 18.f), FVector2D(120.f, 28.f), Filters[f], EButton::OfficerFilter, f, OfficerFilter == f);
		}
		TArray<FTableColumn> Cols = { {TEXT("Navn"), 200.f}, {TEXT("Rang"), 120.f}, {TEXT("Alder"), 46.f, true}, {TEXT("Funktion"), 290.f} };
		for (int32 s = 0; s < int32(ECampaign1851OfficerStat::Count); ++s)
		{
			Cols.Add({ Campaign1851Army::StatShort(ECampaign1851OfficerStat(s)), 42.f, true });
		}
		Cols.Add({ TEXT("Erf"), 44.f, true });
		Cols.Add({ TEXT("Vurd"), 48.f, true });
		Cols.Add({ TEXT("Løn"), 58.f, true });
		PaintTable(Geometry, Out, Layer + 2, Pos + FVector2D(24.f, 96.f), VisibleRows - 2, Cols, Rows, EButton::TableRow, InspectedOfficer);
		const float BottomY = Pos.Y + Size.Y - 48.f;
		PaintButton(Geometry, Out, Layer + 3, FVector2D(Pos.X + 24.f, BottomY), FVector2D(280.f, 28.f), FString::Printf(TEXT("REKRUTTÉR OFFICER  (%s rd.)"), *Thousands(Map->OfficerCost(false))),
			EButton::OfficerRecruit, 0, false, Map->GetTreasury() < Map->OfficerCost(false));
		PaintButton(Geometry, Out, Layer + 3, FVector2D(Pos.X + 316.f, BottomY), FVector2D(280.f, 28.f), FString::Printf(TEXT("REKRUTTÉR GENERAL  (%s rd.)"), *Thousands(Map->OfficerCost(true))),
			EButton::OfficerRecruit, 1, false, Map->GetTreasury() < Map->OfficerCost(true));
		if (Officers.IsValidIndex(InspectedOfficer))
		{
			PaintOfficerCard(Geometry, Out, Layer + 6, Pos + FVector2D(Size.X - 480.f, Size.Y - 20.f));
		}
	}
	else if (Window == EWindow::Budget)
	{
		const TArray<FCampaign1851BudgetLine> Lines = Map->MonthlyBudget();
		double Net = 0.0;
		for (const FCampaign1851BudgetLine& L : Lines)
		{
			Net += L.PerMonth;
		}
		Title(TEXT("Statskassen"), FString::Printf(TEXT("Kassebeholdning %s rd.  ·  netto %s%s rd. pr. måned  ·  %s%s rd. pr. år"), *Thousands(FMath::FloorToInt(Map->GetTreasury())),
			Net < 0 ? TEXT("-") : TEXT("+"), *Thousands(FMath::Abs(FMath::RoundToInt(Net))), Net < 0 ? TEXT("-") : TEXT("+"), *Thousands(FMath::Abs(FMath::RoundToInt(Net * 12.0)))));
		// Left: the month's budget. Right: the account book.
		float Y = Pos.Y + 110.f;
		const float LX = Pos.X + 24.f, LW = 560.f;
		PaintText(Geometry, Out, Layer + 3, TEXT("B U D G E T   P R .   M Å N E D"), FVector2D(LX, Y), Serif(11), Gold, 0.f, false);
		Y += 30.f;
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			PaintText(Geometry, Out, Layer + 3, Pass == 0 ? TEXT("Indtægter") : TEXT("Udgifter"), FVector2D(LX, Y), Serif(13, EFace::Italic), Gold, 0.f, false);
			Y += 26.f;
			for (const FCampaign1851BudgetLine& L : Lines)
			{
				if ((L.PerMonth > 0.5) != (Pass == 0))   // income is money in; every spending line shows, 0 or not
				{
					continue;
				}
				PaintTextFit(Geometry, Out, Layer + 3, L.Text, FVector2D(LX + 16.f, Y), Serif(13), Ink, LW - 180.f);
				PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("%s%s rd."), L.PerMonth < 0 ? TEXT("-") : TEXT(""), *Thousands(FMath::Abs(FMath::RoundToInt(L.PerMonth)))),
					FVector2D(LX + LW, Y), Serif(13), Ink, 1.f, false);
				Y += 24.f;
			}
			Y += 12.f;
		}
		DrawLines(Geometry, Out, Layer + 3, { FVector2D(LX, Y - 8.f), FVector2D(LX + LW, Y - 8.f) }, Gold.CopyWithNewOpacity(0.5f), 1.f);
		PaintText(Geometry, Out, Layer + 3, TEXT("Netto"), FVector2D(LX + 16.f, Y + 6.f), Serif(14), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("%s%s rd."), Net < 0 ? TEXT("-") : TEXT("+"), *Thousands(FMath::Abs(FMath::RoundToInt(Net)))), FVector2D(LX + LW, Y + 6.f), Serif(14), Net < 0 ? CityRed : Ink, 1.f, false);
		PaintText(Geometry, Out, Layer + 3, TEXT("Byggeri og anlæg regnes i dagens tempo (vinter: langsommere). Hærens mandskab lønnes over hærens eget budget."), FVector2D(LX, Y + 40.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
		// State loans.
		Y += 76.f;
		PaintText(Geometry, Out, Layer + 3, TEXT("S T A T S L Å N"), FVector2D(LX, Y), Serif(11), Gold, 0.f, false);
		Y += 26.f;
		PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("Gæld %s rd. til %.1f %%  ·  nyt lån nu til %.1f %%"), *Thousands(FMath::RoundToInt(Map->GetDebt())), Map->GetDebtRate() * 100.f, Map->CreditRate() * 100.f),
			FVector2D(LX, Y), Serif(13), Ink, 0.f, false);
		Y += 22.f;
		PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("Ledig kredit %s rd.  ·  højst 3 års indtægter  ·  renter betales månedligt"), *Thousands(FMath::RoundToInt(FMath::Max(0.0, Map->LoanLimit() - Map->GetDebt())))),
			FVector2D(LX, Y), Serif(10, EFace::Italic), MutedInk, 0.f, false);
		Y += 18.f;
		PaintButton(Geometry, Out, Layer + 3, FVector2D(LX, Y), FVector2D(170.f, 26.f), TEXT("LÅN 100.000"), EButton::Loan, 0, false, !Map->LoanBlockReason(100000.0).IsEmpty());
		PaintButton(Geometry, Out, Layer + 3, FVector2D(LX + 180.f, Y), FVector2D(170.f, 26.f), TEXT("LÅN 250.000"), EButton::Loan, 1, false, !Map->LoanBlockReason(250000.0).IsEmpty());
		PaintButton(Geometry, Out, Layer + 3, FVector2D(LX + 360.f, Y), FVector2D(170.f, 26.f), Map->GetDebt() < 100000.0 ? TEXT("BETAL RESTGÆLD") : TEXT("AFDRAG 100.000"), EButton::Loan, 2, false, Map->GetDebt() <= 0.0 || Map->GetTreasury() < FMath::Min(100000.0, Map->GetDebt()));
		// Right column: the ministries' budgets (what each may spend a month on its own, AUTO) and the reserve, then the account book.
		const float RX = Pos.X + 640.f, RW = Size.X - 640.f - 24.f;
		float MY = Pos.Y + 110.f;
		PaintText(Geometry, Out, Layer + 3, TEXT("M I N I S T E R I E R N E S   B U D G E T T E R"), FVector2D(RX, MY), Serif(11), Gold, 0.f, false);
		MY += 28.f;
		double AllMinistries = 0.0;
		for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
		{
			const ECampaign1851Portfolio P = ECampaign1851Portfolio(p);
			const double Allow = Map->GetMinistryBudget(P), Pot = Map->GetMinistryPot(P);
			AllMinistries += Allow;
			PaintTextFit(Geometry, Out, Layer + 3, Campaign1851Nations::PortfolioName(P), FVector2D(RX + 16.f, MY), Serif(13), Ink, 210.f);
			PaintButton(Geometry, Out, Layer + 3, FVector2D(RX + 240.f, MY - 10.f), FVector2D(24.f, 20.f), TEXT("-"), EButton::MinistryBudget, p * 2);
			PaintText(Geometry, Out, Layer + 3, Allow <= 0.0 ? FString(TEXT("intet")) : FString::Printf(TEXT("%s rd./md."), *Thousands(int32(Allow))), FVector2D(RX + 354.f, MY), Serif(13), Ink, 0.5f, false);
			PaintButton(Geometry, Out, Layer + 3, FVector2D(RX + 440.f, MY - 10.f), FVector2D(24.f, 20.f), TEXT("+"), EButton::MinistryBudget, p * 2 + 1);
			PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("pulje %s"), *Thousands(int32(Pot))), FVector2D(RX + RW, MY), Serif(12, EFace::Italic), MutedInk, 1.f, false);
			MY += 26.f;
		}
		MY += 4.f;
		PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("I alt %s rd. om måneden"), *Thousands(int32(AllMinistries))), FVector2D(RX + 16.f, MY), Serif(12, EFace::Italic), Gold, 0.f, false);
		MY += 26.f;
		PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("Mindste kassebeholdning %s rd."), *Thousands(int32(Map->GetNations().IsValidIndex(Map->GetPlayerNation()) ? Map->GetNations()[Map->GetPlayerNation()].Reserve : 0.0))),
			FVector2D(RX + 16.f, MY), Serif(12, EFace::Italic), Gold, 0.f, false);
		PaintButton(Geometry, Out, Layer + 3, FVector2D(RX + 330.f, MY - 10.f), FVector2D(34.f, 22.f), TEXT("-"), EButton::Reserve, 0);
		PaintButton(Geometry, Out, Layer + 3, FVector2D(RX + 372.f, MY - 10.f), FVector2D(34.f, 22.f), TEXT("+"), EButton::Reserve, 1);
		MY += 34.f;
		DrawLines(Geometry, Out, Layer + 3, { FVector2D(RX, MY - 8.f), FVector2D(RX + RW, MY - 8.f) }, Gold.CopyWithNewOpacity(0.4f), 1.f);
		const TArray<FCampaign1851Transaction>& Ledger = Map->GetLedger();
		PaintText(Geometry, Out, Layer + 3, TEXT("R E G N S K A B"), FVector2D(RX, MY + 10.f), Serif(11), Gold, 0.f, false);
		const int32 Fit = FMath::Max(1, FMath::FloorToInt((Pos.Y + Size.Y - 24.f - (MY + 38.f)) / 23.f));
		const int32 Shown = FMath::Min(Ledger.Num(), FMath::Min(VisibleRows, Fit));
		for (int32 k = 0; k < Shown; ++k)
		{
			const FCampaign1851Transaction& T = Ledger[Ledger.Num() - 1 - k];
			const float TY = MY + 38.f + k * 23.f;
			PaintText(Geometry, Out, Layer + 3, ACampaign1851Map::FormatDate(T.Date, true), FVector2D(RX, TY), Serif(11), MutedInk, 0.f, false);
			PaintTextFit(Geometry, Out, Layer + 3, T.Text, FVector2D(RX + 110.f, TY), Serif(12), Ink, RW - 250.f);
			PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("%s%s"), T.Amount < 0 ? TEXT("-") : TEXT("+"), *Thousands(FMath::Abs(FMath::RoundToInt(T.Amount)))), FVector2D(RX + RW, TY), Serif(12), T.Amount < 0 ? Ink : Gold, 1.f, false);
		}
	}
	else if (Window == EWindow::Trains)
	{
		const int32 Free = Map->FreeTroopTrains(), All = Map->GetTroopTrains();
		Title(TEXT("Troppetog"), FString::Printf(TEXT("%d tog  ·  %d ledige  ·  et tog tager en bataljon (800 mand), 250 heste eller et batteri"), All, Free));
		float Y = Pos.Y + 112.f;
		auto Line = [&](const FString& A, const FString& B)
		{
			PaintText(Geometry, Out, Layer + 3, A, FVector2D(Pos.X + 40.f, Y), Serif(13), Ink, 0.f, false);
			PaintTextFit(Geometry, Out, Layer + 3, B, FVector2D(Pos.X + 200.f, Y), Serif(13), Ink, Size.X - 240.f);
			Y += 26.f;
		};
		// The trains by railway network: a train runs only on its own network; new ones are ordered for a
		// network, and a free one can be shipped to another.
		const TArray<FCampaign1851TroopTrain>& List = Map->GetTroopTrainList();
		const TArray<ACampaign1851Map::FRailNet> Nets = Map->RailNets();
		const TArray<FCampaign1851City>& Towns = Map->GetCities();
		TArray<bool> Shown;
		Shown.Init(false, List.Num());
		const bool bCanOrder = Map->GetTreasury() >= ACampaign1851Map::TroopTrainCost;
		for (int32 n = 0; n < Nets.Num() && Y < Pos.Y + Size.Y - 200.f; ++n)
		{
			const ACampaign1851Map::FRailNet& Net = Nets[n];
			int32 Count = 0, FreeHere = 0;
			for (int32 t = 0; t < List.Num(); ++t)
			{
				if (Net.Stations.Contains(List[t].Station) || (List[t].bBoarded && Net.Stations.Contains(List[t].Release)))
				{
					++Count;
					FreeHere += List[t].IsFree() ? 1 : 0;
				}
			}
			PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("BANEN %s"), *Net.Name.ToUpper().Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å")).Replace(TEXT("æ"), TEXT("Æ"))),
				FVector2D(Pos.X + 24.f, Y), Serif(12, EFace::Bold), Gold, 0.f, false);
			PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("%d tog  ·  %d ledige  ·  %d stationer  ·  nye tog landes i %s"), Count, FreeHere, Net.Stations.Num(), *Towns[Net.Depot].Name),
				FVector2D(Pos.X + 300.f, Y), Serif(11, EFace::Italic), MutedInk, 0.f, false);
			PaintButton(Geometry, Out, Layer + 3, FVector2D(Pos.X + Size.X - 330.f, Y - 12.f), FVector2D(300.f, 24.f),
				FString::Printf(TEXT("BESTIL TIL DENNE BANE  (%s rd.)"), *Thousands(ACampaign1851Map::TroopTrainCost)), EButton::TrainOrder, Net.Depot, false, !bCanOrder);
			Y += 28.f;
			for (int32 t = 0; t < List.Num(); ++t)
			{
				const FCampaign1851TroopTrain& T = List[t];
				if (Shown[t] || !(Net.Stations.Contains(T.Station) || (T.bBoarded && Net.Stations.Contains(T.Release)) || (T.IsRunningEmpty() && Net.Stations.Contains(T.Board))))
				{
					continue;
				}
				Shown[t] = true;
				PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("Tog %d"), T.Id), FVector2D(Pos.X + 40.f, Y), Serif(13), Ink, 0.f, false);
				PaintTextFit(Geometry, Out, Layer + 3, Map->DescribeTrain(t), FVector2D(Pos.X + 120.f, Y), Serif(13), Ink, 380.f);
				// A free train: ship it to another network.
				float BX = Pos.X + 520.f;
				for (int32 m = 0; m < Nets.Num() && T.IsFree(); ++m)
				{
					if (m == n)
					{
						continue;
					}
					PaintButton(Geometry, Out, Layer + 3, FVector2D(BX, Y - 11.f), FVector2D(250.f, 22.f),
						FString::Printf(TEXT("FLYT TIL %s  (%.0f d.)"), *Towns[Nets[m].Depot].Name.ToUpper().Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å")).Replace(TEXT("æ"), TEXT("Æ")), Map->TrainTransferDays(t, Nets[m].Depot)),
						EButton::TrainMove, t * 1000 + Nets[m].Depot, false, Map->GetTreasury() < ACampaign1851Map::TrainTransferCost);
					BX += 258.f;
				}
				Y += 25.f;
			}
			Y += 12.f;
		}
		// Trains on their way to another network, or standing where no railway runs.
		bool bHeading = false;
		for (int32 t = 0; t < List.Num(); ++t)
		{
			if (Shown[t])
			{
				continue;
			}
			if (!bHeading)
			{
				PaintText(Geometry, Out, Layer + 3, TEXT("PÅ VEJ MELLEM BANERNE"), FVector2D(Pos.X + 24.f, Y), Serif(12, EFace::Bold), Gold, 0.f, false);
				Y += 28.f;
				bHeading = true;
			}
			Line(FString::Printf(TEXT("Tog %d"), List[t].Id), Map->DescribeTrain(t));
		}
		PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("Et ledigt tog kan flyttes til en anden bane med skib eller vogn: %s rd. og 10 dage + 1 dag pr. 40 km."), *Thousands(int32(ACampaign1851Map::TrainTransferCost))),
			FVector2D(Pos.X + 24.f, Y + 2.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
		Y += 30.f;
		PaintText(Geometry, Out, Layer + 3, TEXT("B E S T I L T"), FVector2D(Pos.X + 24.f, Y), Serif(11), Gold, 0.f, false);
		Y += 30.f;
		if (Map->GetTrainOrders().Num() == 0)
		{
			Line(TEXT("Ingen tog i bestilling"), FString());
		}
		for (const FVector2D& O : Map->GetTrainOrders())
		{
			const FDateTime Arrive = ACampaign1851Map::StartDate() + FTimespan::FromDays(O.Y);
			const int32 To = int32(O.X) - 1;
			Line(TEXT("1 togsæt fra England"), FString::Printf(TEXT("til %s  ·  leveres ca. %s"), Towns.IsValidIndex(To) ? *Towns[To].Name : TEXT("?"), *ACampaign1851Map::FormatDate(Arrive, true)));
		}
		PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("Lokomotiv og vogne bygges i England (%s rd., levering %.0f dage) og skibes til den bane, de er bestilt til. Uden ledige tog marcherer kolonnen i stedet."),
			*Thousands(ACampaign1851Map::TroopTrainCost), ACampaign1851Map::TroopTrainDeliveryDays),
			FVector2D(Pos.X + 24.f, Y + 14.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
	}
	else if (Window == EWindow::Supply)
	{
		PaintSupply(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::Council)
	{
		PaintCouncil(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::Foreign)
	{
		PaintForeign(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::Research)
	{
		PaintResearch(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::Navy)
	{
		PaintNavy(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::Gazette)
	{
		PaintGazette(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::End)
	{
		PaintEnd(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::Battlefield)
	{
		PaintBattlefield(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::Materiel)
	{
		PaintMateriel(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::Nations)
	{
		PaintNations(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::ArmyStatus)
	{
		Title(TEXT("Hærens status"), TEXT("Krigsministerens oversigt: hæren pr. våbenart, tabene på begge sider og det erobrede udstyr"));
		PaintArmyStatus(Geometry, Out, Layer + 2, Pos, Size);
	}
	else if (Window == EWindow::Chart)
	{
		Title(TEXT("Kamporden"), OOBFilter.Num() > 0 ? FString(TEXT("De valgte enheder: træk kompagnier eller eskadroner til højre for at dele dem")) : FString(TEXT("Felthæren som organisation: hovedkvarterer og deres enheder")));
		if (OOBFilter.Num() > 0)
		{
			const float BX = Pos.X + Size.X - 560.f;
			PaintButton(Geometry, Out, Layer + 3, FVector2D(BX, Pos.Y + 20.f), FVector2D(150.f, 28.f), TEXT("HELE HÆREN"), EButton::OOBFocusClear, 0);
			for (int32 u : OOBFilter)
			{
				const int32 Partner = Regs.IsValidIndex(u) ? Map->MergePartner(u) : INDEX_NONE;
				if (Partner != INDEX_NONE)
				{
					const int32 Keep = Regs[u].bDetached && !Regs[Partner].bDetached ? Partner : u;
					PaintButton(Geometry, Out, Layer + 3, FVector2D(BX + 160.f, Pos.Y + 20.f), FVector2D(150.f, 28.f), TEXT("SAML IGEN"), EButton::MergeUnit, Keep * 10000 + (Keep == u ? Partner : u));
					break;
				}
			}
		}
		PaintOOBChart(Geometry, Out, Layer + 2, Pos + FVector2D(24.f, 96.f), Size - FVector2D(48.f, 120.f));
	}
	else if (Window == EWindow::Towns)
	{
		TArray<FTableRow> Rows;
		int32 Population = 0;
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			const FCampaign1851City& C = Cities[c];
			if (C.bForeign)
			{
				continue;
			}
			Population += C.Population;
			const FCampaign1851Amt* Amt = Map->FindAmt(C.AmtId);
			int32 Units = 0, Men = 0, Built = 0;
			for (int32 i : Map->RegimentsIn(c))
			{
				++Units;
				Men += Regs[i].Men;
			}
			for (const ACampaign1851ConstructionSite* Site : Map->GetProjects())
			{
				if (Site && Site->GetCityIndex() == c)
				{
					for (int32 m = 0; m < Site->NumModules(); ++m)
					{
						Built += Site->IsModuleDone(m) ? 1 : 0;
					}
				}
			}
			FTableRow Row;
			Row.Id = c;
			Row.Cells = { C.Name, Amt ? Amt->Name : FString(TEXT("-")), ACampaign1851Map::RegionName(C.Region), Thousands(C.Population),
				Thousands(FMath::RoundToInt(C.Population * ACampaign1851Map::UrbanTaxRate())), Thousands(FMath::RoundToInt(C.Population * 0.09)),
				FString::FromInt(Units), Thousands(Men), FString::FromInt(Built), Map->HasStation(c) ? FString(TEXT("ja")) : FString(TEXT("-")) };
			Row.Keys = { Text, Text, Text, double(C.Population), C.Population * ACampaign1851Map::UrbanTaxRate(), C.Population * 0.09, double(Units), double(Men), double(Built), Text };
			Rows.Add(Row);
		}
		Title(TEXT("Byerne"), FString::Printf(TEXT("%d købstæder  ·  %s indbyggere i byerne  ·  klik på en by for at gå dertil"), Rows.Num(), *Thousands(Population)));
		const TArray<FTableColumn> Cols = { {TEXT("By"), 170.f}, {TEXT("Amt"), 200.f}, {TEXT("Region"), 220.f}, {TEXT("Indbyggere"), 110.f, true},
			{TEXT("Skat/år"), 100.f, true}, {TEXT("Våbenføre"), 100.f, true}, {TEXT("Enheder"), 90.f, true}, {TEXT("Mand"), 90.f, true},
			{TEXT("Bygninger"), 100.f, true}, {TEXT("Station"), 90.f, true} };
		PaintTable(Geometry, Out, Layer + 2, Pos + FVector2D(24.f, 96.f), VisibleRows, Cols, Rows, EButton::TableRow, SelectedCity);
	}
}

void SCampaign1851Overlay::PaintMenu(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const FVector2D Screen = Geometry.GetLocalSize();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Screen, FSlateLayoutTransform(FVector2D::ZeroVector)), White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.45f));

	const float RowHeight = 62.f;
	const FVector2D Size(720.f, 120.f + MenuSlots.Num() * RowHeight + 104.f);
	const FVector2D Pos = (Screen - Size) * 0.5f;
	PaintPanel(Geometry, Out, Layer + 1, Pos, Size);
	PaintText(Geometry, Out, Layer + 3, TEXT("S P I L L E T"), Pos + FVector2D(Size.X * 0.5f, 40.f), Serif(22), Ink, 0.5f);
	PaintText(Geometry, Out, Layer + 3, TEXT("Gem og indlæs felttoget"), Pos + FVector2D(Size.X * 0.5f, 72.f), Serif(12, EFace::Italic), Gold, 0.5f, false);

	for (int32 i = 0; i < MenuSlots.Num(); ++i)
	{
		const FSlotInfo& S = MenuSlots[i];
		const FVector2D Row = Pos + FVector2D(0.f, 104.f + i * RowHeight);
		TArray<FVector2D> Rule = { Row + FVector2D(24.f, 0.f), Row + FVector2D(Size.X - 24.f, 0.f) };
		FSlateDrawElement::MakeLines(Out, Layer + 3, Geometry.ToPaintGeometry(), Rule, ESlateDrawEffect::None, Gold.CopyWithNewOpacity(0.3f), true, 1.f);
		PaintText(Geometry, Out, Layer + 3, S.Label, Row + FVector2D(32.f, 22.f), Serif(16), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 3, S.Info, Row + FVector2D(32.f, 46.f), Serif(11, EFace::Italic), S.bExists ? Ink : MutedInk, 0.f, false);
		if (S.bCanSave)
		{
			PaintButton(Geometry, Out, Layer + 3, Row + FVector2D(Size.X - 216.f, 10.f), FVector2D(88.f, 28.f), TEXT("GEM"), EButton::SaveSlot, i);
		}
		if (S.bExists)
		{
			PaintButton(Geometry, Out, Layer + 3, Row + FVector2D(Size.X - 118.f, 10.f), FVector2D(94.f, 28.f), TEXT("INDLÆS"), EButton::LoadSlot, i);
		}
	}
	// The scenario of the next new game.
	{
		const int32 Chosen = MenuScenario >= 0 ? MenuScenario : ACampaign1851Map::ScenarioIndex();
		PaintText(Geometry, Out, Layer + 3, TEXT("Scenarie"), Pos + FVector2D(24.f, Size.Y - 104.f), Serif(12, EFace::Italic), Gold, 0.f, false);
		const TArray<ACampaign1851Map::FScenario>& List = ACampaign1851Map::Scenarios();
		for (int32 i = 0; i < List.Num(); ++i)
		{
			const FVector2D At = Pos + FVector2D(110.f + i * 170.f, Size.Y - 112.f);
			PaintButton(Geometry, Out, Layer + 3, At, FVector2D(160.f, 28.f), *List[i].Name, EButton::Scenario, i, Chosen == i);
			AddTip(At, FVector2D(160.f, 28.f), List[i].Text);
		}
	}
	PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(24.f, Size.Y - 50.f), FVector2D(bConfirmNewGame ? 210.f : 130.f, 30.f),
		bConfirmNewGame ? TEXT("BEKRÆFT: NYT SPIL") : TEXT("NYT SPIL"), EButton::NewGame);
	if (bConfirmNewGame)
	{
		PaintText(Geometry, Out, Layer + 3, TEXT("Alle byggerier slettes"), Pos + FVector2D(248.f, Size.Y - 35.f), Serif(11, EFace::Italic), Gold, 0.f, false);
		// How far the new world may stray from history (seeded: every game its own).
		PaintText(Geometry, Out, Layer + 3, TEXT("Afvigelse fra historien"), Pos + FVector2D(24.f, Size.Y - 78.f), Serif(12, EFace::Italic), Gold, 0.f, false);
		const int32 Steps[] = { 0, 10, 20, 35 };
		for (int32 k = 0; k < 4; ++k)
		{
			PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(200.f + k * 74.f, Size.Y - 92.f), FVector2D(68.f, 26.f), FString::Printf(TEXT("%d %%"), Steps[k]), EButton::Deviation, Steps[k],
				FMath::RoundToInt(Map->NewGameDeviation * 100.f) == Steps[k]);
		}
		// The nation to play (Sweden-Norway on the abstract model, a first step).
		PaintText(Geometry, Out, Layer + 3, TEXT("Spil som"), Pos + FVector2D(24.f, Size.Y - 112.f), Serif(12, EFace::Italic), Gold, 0.f, false);
		const TCHAR* Ids[] = { TEXT("DK"), TEXT("SE") };
		const TCHAR* Names[] = { TEXT("DANMARK"), TEXT("SVERIGE-NORGE") };
		for (int32 k = 0; k < 2; ++k)
		{
			PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(200.f + k * 150.f, Size.Y - 126.f), FVector2D(144.f, 26.f), Names[k], EButton::NewGameNation, k, Map->NewGameNation == Ids[k]);
		}
	}
	PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 144.f, Size.Y - 50.f), FVector2D(120.f, 30.f), TEXT("LUK"), EButton::CloseMenu);
	PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 314.f, Size.Y - 50.f), FVector2D(160.f, 30.f), TEXT("AFSLUT SPIL"), EButton::ExitGame);
}

void SCampaign1851Overlay::PaintToast(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const double Age = FPlatformTime::Seconds() - ToastTime;
	if (Toast.IsEmpty() || Age > 3.0)
	{
		return;
	}
	const float Alpha = float(FMath::Clamp(3.0 - Age, 0.0, 1.0));
	const FSlateFontInfo Font = Serif(15, EFace::Italic);
	const FVector2D TextSize = Measure(Toast, Font);
	const FVector2D Size(TextSize.X + 60.f, 44.f);
	// Under the date panel, or under the open window (which covers the date panel's surroundings).
	const FVector2D Pos((Geometry.GetLocalSize().X - Size.X) * 0.5f, Window != EWindow::None ? Geometry.GetLocalSize().Y - 92.f : 100.f);
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, Panel.CopyWithNewOpacity(Panel.A * Alpha));
	TArray<FVector2D> Frame = { Pos, Pos + FVector2D(Size.X, 0.f), Pos + Size, Pos + FVector2D(0.f, Size.Y), Pos };
	FSlateDrawElement::MakeLines(Out, Layer + 1, Geometry.ToPaintGeometry(), Frame, ESlateDrawEffect::None, Gold.CopyWithNewOpacity(Alpha), true, 1.2f);
	PaintText(Geometry, Out, Layer + 1, Toast, Pos + Size * 0.5f, Font, Ink.CopyWithNewOpacity(Alpha), 0.5f, false);
}

void SCampaign1851Overlay::PaintCalendar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	// Hour, date and season, then pause and speeds 1-5 (Hearts of Iron IV style). Kept clear of the title cartouche.
	const FVector2D Size(720.f, 56.f);
	const FVector2D Pos(FMath::Max((Geometry.GetLocalSize().X - Size.X) * 0.5f, 484.f), 28.f);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	const FDateTime Now = Map->GetDate();
	const FString Hour = ACampaign1851Map::FormatClock(Now);
	const FString Date = ACampaign1851Map::FormatDate(Now);
	const FSlateFontInfo DateFont = Serif(20);
	float TextX = Pos.X + 22.f;
	PaintText(Geometry, Out, Layer + 2, Hour, FVector2D(TextX, Pos.Y + Size.Y * 0.5f), DateFont, Gold, 0.f);
	TextX += Measure(TEXT("00:00"), DateFont).X + 14.f;
	const float PauseWidth = 62.f, ButtonWidth = 34.f, Gap = 5.f;
	float X = Pos.X + Size.X - 16.f - PauseWidth - (ACampaign1851Map::NumSpeeds() - 1) * (ButtonWidth + Gap);
	// Long months ("13. september 1851"): the date shrinks so the season stays clear of the buttons.
	// Season above, the day's weather below it.
	const FSlateFontInfo SeasonFont = Serif(11, EFace::Italic);
	const FString Season = Map->GetSeasonName();
	const FString Weather = FString::Printf(TEXT("%s %.0f°%s"), Campaign1851Weather::Name(Map->GetWeather()), Map->GetTemperature(), Map->IsIceWinter() ? TEXT(" · isvinter") : TEXT(""));
	const float SeasonW = FMath::Max(Measure(Season, SeasonFont).X, Measure(Weather, SeasonFont).X);
	// Margins with room to spare: the italic text measures a little short.
	const float Room = X - 24.f - TextX - SeasonW * 1.1f - 14.f;
	const FSlateFontInfo Fitted = Measure(Date, DateFont).X <= Room - 24.f ? DateFont : Measure(Date, Serif(17)).X <= Room - 12.f ? Serif(17) : Serif(15);
	PaintText(Geometry, Out, Layer + 2, Date, FVector2D(TextX, Pos.Y + Size.Y * 0.5f), Fitted, Ink, 0.f);
	const float SX = TextX + Measure(Date, Fitted).X + 12.f;
	PaintText(Geometry, Out, Layer + 2, Season, FVector2D(SX, Pos.Y + Size.Y * 0.5f - 8.f), SeasonFont, Gold, 0.f, false);
	PaintText(Geometry, Out, Layer + 2, Weather, FVector2D(SX, Pos.Y + Size.Y * 0.5f + 9.f), SeasonFont, Map->GetWeather() == ECampaign1851Weather::Clear ? MutedInk : Ink, 0.f, false);
	for (int32 s = 0; s < ACampaign1851Map::NumSpeeds(); ++s)
	{
		const float W = s == 0 ? PauseWidth : ButtonWidth;
		// Speeds up to the current one light up, like a speed gauge.
		const bool bLit = s == 0 ? Map->GetSpeed() == 0 : Map->GetSpeed() >= s;
		PaintButton(Geometry, Out, Layer + 2, FVector2D(X, Pos.Y + 14.f), FVector2D(W, 28.f), ACampaign1851Map::SpeedLabel(s), EButton::Speed, s, bLit);
		X += W + Gap;
	}
}

FString SCampaign1851Overlay::ProgressLine(const ACampaign1851ConstructionSite* Site, int32 Module) const
{
	if (Site->IsStalled())
	{
		return FString::Printf(TEXT("%s  ·  standset, mangler penge"), *Site->GetStageName(Module));
	}
	const float Rate = Campaign1851Buildings::WorkRate(Site->ModuleType(Module), Map->GetDate());
	const float Left = (Site->ModuleDays(Module) - Site->GetModuleElapsedDays(Module)) / FMath::Max(Rate, 0.1f);
	return FString::Printf(TEXT("%s%s  ·  klar ca. %s"), *Site->GetStageName(Module), Rate < 1.f ? TEXT(" (vintertakt)") : TEXT(""),
		*ACampaign1851Map::FormatDate(Map->GetDate() + FTimespan::FromDays(Left), true));
}

void SCampaign1851Overlay::PaintTreasury(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const FVector2D Pos(28.f, 246.f), Size(390.f, 66.f);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintText(Geometry, Out, Layer + 2, TEXT("S T A T S K A S S E N"), Pos + FVector2D(18.f, 18.f), Serif(10), Gold, 0.f, false);
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s rd."), *Thousands(FMath::FloorToInt(Map->GetTreasury()))), Pos + FVector2D(18.f, 43.f), Serif(20), Ink, 0.f);
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("skat +%s / md."), *Thousands(FMath::RoundToInt(Map->YearlyTax() / 12.0))),
		Pos + FVector2D(Size.X - 16.f, 34.f), Serif(11, EFace::Italic), Ink, 1.f, false);
	const int32 Upkeep = FMath::RoundToInt(Map->GetMonthlyUpkeep());
	if (Upkeep > 0)
	{
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("drift −%s / md."), *Thousands(Upkeep)), Pos + FVector2D(Size.X - 16.f, 52.f), Serif(11, EFace::Italic), MutedInk, 1.f, false);
	}
	Buttons.Add({ Pos, Pos + Size, EButton::Treasury, INDEX_NONE });
}

void SCampaign1851Overlay::PaintLedger(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851Transaction>& Ledger = Map->GetLedger();
	const int32 Rows = FMath::Min(Ledger.Num(), 14);
	const FVector2D Size(460.f, 84.f + Rows * 22.f);
	const FVector2D Pos(Geometry.GetLocalSize().X - Size.X - 28.f, 428.f);   // below the legend
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseLedger);
	PaintText(Geometry, Out, Layer + 2, TEXT("R E G N S K A B"), Pos + FVector2D(Size.X * 0.5f, 24.f), Serif(13), Ink, 0.5f, false);
	PaintText(Geometry, Out, Layer + 2, TEXT("Seneste posteringer  ·  klik på statskassen for at lukke"), Pos + FVector2D(Size.X * 0.5f, 46.f), Serif(10, EFace::Italic), Gold, 0.5f, false);
	const FLinearColor In = FLinearColor::FromSRGBColor(FColor(170, 214, 150)), OutInk = FLinearColor::FromSRGBColor(FColor(232, 150, 128));
	for (int32 r = 0; r < Rows; ++r)
	{
		const FCampaign1851Transaction& T = Ledger[Ledger.Num() - 1 - r];
		const float Y = Pos.Y + 72.f + r * 22.f;
		PaintText(Geometry, Out, Layer + 2, ACampaign1851Map::FormatDate(T.Date, true), FVector2D(Pos.X + 18.f, Y), Serif(11), MutedInk, 0.f, false);
		PaintText(Geometry, Out, Layer + 2, T.Text.Len() > 40 ? T.Text.Left(39) + TEXT("…") : T.Text, FVector2D(Pos.X + 118.f, Y), Serif(11), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s%s"), T.Amount >= 0.0 ? TEXT("+") : TEXT("−"), *Thousands(FMath::RoundToInt(FMath::Abs(T.Amount)))),
			FVector2D(Pos.X + Size.X - 18.f, Y), Serif(11), T.Amount >= 0.0 ? In : OutInk, 1.f, false);
	}
}

void SCampaign1851Overlay::PaintAmtInfo(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const FCampaign1851Amt* A = Map->FindAmt(SelectedAmt);
	if (!A)
	{
		return;
	}
	// Garrisons in the amt.
	TArray<FString> Garrisons;
	for (const ACampaign1851ConstructionSite* Site : Map->GetProjects())
	{
		const TArray<FCampaign1851City>& Cities = Map->GetCities();
		if (Site && Cities.IsValidIndex(Site->GetCityIndex()) && Cities[Site->GetCityIndex()].AmtId == A->Id)
		{
			Garrisons.Add(FString::Printf(TEXT("%s%s"), *Cities[Site->GetCityIndex()].Name, Site->IsBarracksDone() ? TEXT("") : TEXT(" (under bygning)")));
		}
	}
	const FVector2D Size(420.f, 342.f);
	const FVector2D Pos(28.f, Geometry.GetLocalSize().Y - 190.f - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseSelection);
	float Y = Pos.Y + 32.f;
	auto Line = [&](const FString& Label, const FString& Value)
	{
		PaintText(Geometry, Out, Layer + 2, Label, FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 2, Value, FVector2D(Pos.X + 158.f, Y), Serif(13), Ink, Size.X - 158.f - 18.f);
		Y += 24.f;
	};
	PaintTextFit(Geometry, Out, Layer + 2, A->Name, FVector2D(Pos.X + 22.f, Y), Serif(24), Ink, Size.X - 70.f);
	Y += 32.f;
	PaintText(Geometry, Out, Layer + 2, ACampaign1851Map::RegionName(A->Region), FVector2D(Pos.X + 22.f, Y), Serif(14, EFace::Italic), Gold, 0.f, false);
	Y += 32.f;
	Line(TEXT("Amtsby"), A->Seat);
	Line(TEXT("Befolkning"), FString::Printf(TEXT("ca. %s  ·  by %s"), *Thousands(A->Population), *Thousands(A->Urban)));
	Line(TEXT("Areal"), FString::Printf(TEXT("%s km²  ·  %d indb./km²"), *Thousands(FMath::RoundToInt(A->AreaKm2)), FMath::RoundToInt(A->Population / FMath::Max(A->AreaKm2, 1.f))));
	Line(TEXT("Købstæder"), A->Towns.Num() > 0 ? FString::Join(A->Towns, TEXT(", ")).Left(36) : TEXT("ingen"));
	Line(TEXT("Skat til anlæg"), FString::Printf(TEXT("%s rd./år"), *Thousands(FMath::RoundToInt(ACampaign1851Map::AmtYearlyTax(*A)))));
	Line(TEXT("Våbenføre mænd"), FString::Printf(TEXT("ca. %s (skøn)"), *Thousands(FMath::RoundToInt(A->Population * 0.09))));
	Line(TEXT("Garnisoner"), Garrisons.Num() > 0 ? FString::Join(Garrisons, TEXT(", ")) : TEXT("ingen"));
	// Who holds it, and how it is won back.
	if (Map->IsAmtOccupied(*A))
	{
		Line(TEXT("Kontrol"), FString::Printf(TEXT("BESAT  ·  befries når %d danske står ved %s i 2 døgn uden fjendtlige korps inden for 10 km"), ACampaign1851Map::LiberationMen, *A->Seat));
	}
	else
	{
		Line(TEXT("Kontrol"), TEXT("dansk  ·  går tabt, hvis fjenden besætter amtsbyen"));
	}
	PaintText(Geometry, Out, Layer + 2, TEXT("Befolkningstal er skøn ud fra folketællingerne omkring 1850"), FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + Size.Y - 16.f),
		Serif(9, EFace::Italic), MutedInk, 0.5f, false);
}

FString SCampaign1851Overlay::LinkProgressLine(int32 Link) const
{
	const FCampaign1851Link& L = Map->GetLinks()[Link];
	const int32 Percent = FMath::FloorToInt(L.Progress() * 100.f);
	if (L.bStalled)
	{
		return FString::Printf(TEXT("%d %%  ·  standset, mangler penge"), Percent);
	}
	const float Rate = Campaign1851Buildings::WorkRate(Campaign1851Network::WorkType(), Map->GetDate());
	const float Left = (L.WorkDays - L.DaysBuilt) / FMath::Max(Rate, 0.1f);
	return FString::Printf(TEXT("%d %%%s  ·  klar ca. %s"), Percent, Rate < 1.f ? TEXT(" (vintertakt)") : TEXT(""),
		*ACampaign1851Map::FormatDate(Map->GetDate() + FTimespan::FromDays(Left), true));
}

void SCampaign1851Overlay::PaintTownLinks(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, float Left) const
{
	const TArray<int32> Rows = Map->LinksOf(SelectedCity);
	if (Rows.Num() == 0)
	{
		return;
	}
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	const float RowHeight = 52.f;
	const FVector2D Size(580.f, 48.f + Rows.Num() * RowHeight + 8.f);
	const FVector2D Pos(Left, Geometry.GetLocalSize().Y - 190.f - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseTownTab);
	PaintText(Geometry, Out, Layer + 2, TEXT("V E J E   O G   J E R N B A N E R"), Pos + FVector2D(22.f, 24.f), Serif(11), Gold, 0.f, false);
	for (int32 r = 0; r < Rows.Num(); ++r)
	{
		const int32 i = Rows[r];
		const FCampaign1851Link& L = Map->GetLinks()[i];
		const FVector2D Row = Pos + FVector2D(0.f, 44.f + r * RowHeight);
		TArray<FVector2D> Rule = { Row + FVector2D(18.f, -2.f), Row + FVector2D(Size.X - 18.f, -2.f) };
		FSlateDrawElement::MakeLines(Out, Layer + 2, Geometry.ToPaintGeometry(), Rule, ESlateDrawEffect::None, Gold.CopyWithNewOpacity(0.25f), true, 1.f);
		// A small symbol: railway (black and white), chaussée (pale stone) or dirt road.
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		const FVector2D Sym = Row + FVector2D(22.f, 20.f);
		if (L.bRailway)
		{
			FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(30.f, 7.f), FSlateLayoutTransform(Sym)), White, ESlateDrawEffect::None, FLinearColor(0.02f, 0.02f, 0.02f));
			for (int32 d = 0; d < 3; ++d)
			{
				FSlateDrawElement::MakeBox(Out, Layer + 3, Geometry.ToPaintGeometry(FVector2D(6.f, 3.f), FSlateLayoutTransform(Sym + FVector2D(2.f + d * 10.f, 2.f))), White, ESlateDrawEffect::None, Ink);
			}
		}
		else
		{
			FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(30.f, L.bChaussee ? 6.f : 4.f), FSlateLayoutTransform(Sym + FVector2D(0.f, 1.f))), White, ESlateDrawEffect::None,
				L.bChaussee ? FLinearColor::FromSRGBColor(FColor(222, 214, 192)) : FLinearColor::FromSRGBColor(FColor(176, 136, 92)));
		}
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("til %s"), *Cities[Map->LinkOther(i, SelectedCity)].Name), Row + FVector2D(62.f, 15.f), Serif(14), Ink, 0.f, false);
		const FVector2D ButtonSize(132.f, 24.f);
		if (L.Work != ECampaign1851LinkWork::None)
		{
			PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s under anlæg  ·  %s"), Campaign1851Network::WorkName(L.Work), *LinkProgressLine(i)),
				Row + FVector2D(62.f, 35.f), Serif(11, EFace::Italic), L.bStalled ? Gold : Ink, 0.f, false);
			PaintBar(Geometry, Out, Layer + 2, Row + FVector2D(Size.X - 2.f * ButtonSize.X - 30.f, 12.f), ButtonSize.X, L.Progress());
			PaintButton(Geometry, Out, Layer + 2, Row + FVector2D(Size.X - ButtonSize.X - 18.f, 8.f), ButtonSize, TEXT("VIS"), EButton::ShowLink, i);
			continue;
		}
		PaintText(Geometry, Out, Layer + 2, Map->LinkTravelText(i) + (L.HasFerry() && !L.Ferry.IsEmpty() ? FString::Printf(TEXT(" (%s)"), *L.Ferry.Left(14)) : FString()),
			Row + FVector2D(62.f, 35.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
		// Buttons from the right: railway, then chaussée.
		float X = Size.X - ButtonSize.X - 18.f;
		for (const ECampaign1851LinkWork Work : { ECampaign1851LinkWork::Railway, ECampaign1851LinkWork::Chaussee })
		{
			if (!Map->LinkBlockReason(i, Work).IsEmpty())
			{
				continue;
			}
			const int32 Cost = Map->LinkWorkCost(i, Work);
			const bool bAfford = Map->CanAfford(Cost);
			PaintButton(Geometry, Out, Layer + 2, Row + FVector2D(X, 8.f), ButtonSize,
				FString::Printf(TEXT("%s %s"), Work == ECampaign1851LinkWork::Railway ? TEXT("BANE") : TEXT("CHAUSSÉ"), *Thousands(Cost)),
				EButton::BuildLink, LinkButton(i, Work == ECampaign1851LinkWork::Railway), false, !bAfford);
			X -= ButtonSize.X + 8.f;
		}
	}
	PaintText(Geometry, Out, Layer + 2, TEXT("Chaussé = stensat hovedlandevej: 1/3 hurtigere march, farbar i al slags vejr  ·  priser er statens andel"), FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + Size.Y - 12.f),
		Serif(9, EFace::Italic), MutedInk, 0.5f, false);
}

void SCampaign1851Overlay::PaintTownBuildings(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851SiteModule>& Types = ACampaign1851ConstructionSite::TownBuildings();
	TArray<int32> Rows;
	for (int32 i = 0; i < Types.Num(); ++i)
	{
		// Two lists: the military buildings, and the civil ones that make the town grow.
		const bool bCivil = Campaign1851Nations::CivilEffect(Types[i].Key) != nullptr;
		if (bCivil == bCivilTab && Map->BuildingBlockReason(SelectedCity, Types[i].Key) != TEXT("-"))
		{
			Rows.Add(i);
		}
	}
	const float RowHeight = 50.f;
	// As many rows as fit above the bottom panels (one fewer, so the list never reaches the top bar); the rest
	// by scrolling (the wheel over the list, or the arrows).
	const int32 MaxRows = FMath::Max(3, int32((Geometry.GetLocalSize().Y - 190.f - 150.f - 56.f) / RowHeight) - 1);
	BuildingRowsTotal = Rows.Num();
	BuildingRowsShown = FMath::Min(Rows.Num(), MaxRows);
	const int32 Scroll = FMath::Clamp(BuildingScroll, 0, FMath::Max(0, Rows.Num() - BuildingRowsShown));
	const FVector2D Size(470.f, 48.f + FMath::Max(BuildingRowsShown, 1) * RowHeight + 8.f);
	// Beside the left panel; beside the order of battle if that is open (the two never lie over each other).
	const FVector2D Pos(bOOB ? FMath::Max(28.f + 440.f + 10.f, TreeMax.X + 10.f) : 28.f + 440.f + 10.f, FMath::Max(150.f, Geometry.GetLocalSize().Y - 190.f - Size.Y));
	BuildingMin = Pos;
	BuildingMax = Pos + Size;
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseTownTab);
	PaintText(Geometry, Out, Layer + 2, TEXT("B Y G N I N G E R"), Pos + FVector2D(22.f, 24.f), Serif(11), Gold, 0.f, false);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(190.f, 10.f), FVector2D(100.f, 26.f), TEXT("MILITÆRE"), EButton::TownBuildingsTab, 0, !bCivilTab);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(296.f, 10.f), FVector2D(100.f, 26.f), TEXT("CIVILE"), EButton::TownBuildingsTab, 1, bCivilTab);
	if (Rows.Num() == 0)
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("Ingen bygninger af denne slags her"), Pos + FVector2D(22.f, 70.f), Serif(12, EFace::Italic), MutedInk, 0.f, false);
		return;
	}
	if (bCivilTab)
	{
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("vækst %.1f %% om året"), Map->UrbanGrowthRate(SelectedCity)), Pos + FVector2D(Size.X - 40.f, 24.f), Serif(10, EFace::Italic), Gold, 1.f, false);
	}
	if (Rows.Num() > BuildingRowsShown)
	{
		// The scrollbar: the arrows and the thumb on the right edge.
		PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 24.f, 44.f), FVector2D(18.f, 18.f), TEXT("^"), EButton::BuildingScroll, 0, false, Scroll == 0);
		PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 24.f, Size.Y - 26.f), FVector2D(18.f, 18.f), TEXT("v"), EButton::BuildingScroll, 2, false, Scroll >= Rows.Num() - BuildingRowsShown);
		const float TrackTop = Pos.Y + 66.f, TrackH = Size.Y - 96.f;
		const float ThumbH = FMath::Max(20.f, TrackH * BuildingRowsShown / Rows.Num());
		const float ThumbY = TrackTop + (TrackH - ThumbH) * Scroll / FMath::Max(1, Rows.Num() - BuildingRowsShown);
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		FSlateDrawElement::MakeBox(Out, Layer + 3, Geometry.ToPaintGeometry(FVector2D(6.f, TrackH), FSlateLayoutTransform(FVector2D(Pos.X + Size.X - 18.f, TrackTop))), White, ESlateDrawEffect::None, Gold.CopyWithNewOpacity(0.2f));
		FSlateDrawElement::MakeBox(Out, Layer + 4, Geometry.ToPaintGeometry(FVector2D(6.f, ThumbH), FSlateLayoutTransform(FVector2D(Pos.X + Size.X - 18.f, ThumbY))), White, ESlateDrawEffect::None, Gold);
	}
	for (int32 r = Scroll; r < Scroll + BuildingRowsShown; ++r)
	{
		const int32 i = Rows[r];
		const FCampaign1851SiteModule& Def = Types[i];
		const FVector2D Row = Pos + FVector2D(0.f, 44.f + (r - Scroll) * RowHeight);
		// The picture and the name: click for the building's card.
		Buttons.Add({ Row + FVector2D(14.f, 0.f), Row + FVector2D(250.f, RowHeight - 4.f), EButton::BuildingInfo, i });
		AddTip(Row + FVector2D(14.f, 0.f), FVector2D(236.f, RowHeight - 4.f), TEXT("Klik for bygningens kort: hvad den gør, hvad den kræver og koster"));
		TArray<FVector2D> Rule = { Row + FVector2D(18.f, -2.f), Row + FVector2D(Size.X - 18.f, -2.f) };
		FSlateDrawElement::MakeLines(Out, Layer + 2, Geometry.ToPaintGeometry(), Rule, ESlateDrawEffect::None, Gold.CopyWithNewOpacity(0.25f), true, 1.f);
		if (TownBrushes.IsValidIndex(i) && TownBrushes[i]->GetResourceObject())
		{
			FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(42.f, 42.f), FSlateLayoutTransform(Row + FVector2D(18.f, 3.f))), TownBrushes[i].Get());
		}
		PaintText(Geometry, Out, Layer + 2, Def.Name, Row + FVector2D(70.f, 15.f), Serif(14), Ink, 0.f, false);
		const ACampaign1851ConstructionSite* Site = Map->FindBuilding(SelectedCity, Def.Key);
		const FVector2D ButtonPos = Row + FVector2D(Size.X - 96.f, 10.f), ButtonSize(78.f, 26.f);
		if (Site)
		{
			const bool bDone = Site->IsModuleDone(0);
			const int32 Code = SelectedCity * 100 + i;
			const FString State = Site->IsDemolishing() ? FString::Printf(TEXT("Rives ned  ·  %.0f %%"), 100.f * Site->GetDemolishDone() / FMath::Max(Site->GetDemolishDays(), 1.f))
				: DemolishArmed == Code ? Map->DemolishText(Site) : Site->IsHistoric() ? FString(TEXT("Står fra før 1851"))
				: (bDone ? FString(TEXT("Færdig")) : ProgressLine(Site, 0)) + (Site->IsPrivate() ? TEXT("  ·  privat") : TEXT(""));
			PaintTextFit(Geometry, Out, Layer + 2, State, Row + FVector2D(70.f, 34.f), Serif(11, EFace::Italic), DemolishArmed == Code ? Gold : bDone ? Gold : Ink, Size.X - 70.f - 190.f);
			PaintButton(Geometry, Out, Layer + 2, ButtonPos, ButtonSize, TEXT("VIS"), EButton::ShowSite, i);
			PaintButton(Geometry, Out, Layer + 2, ButtonPos - FVector2D(94.f, 0.f), FVector2D(88.f, 26.f), DemolishArmed == Code ? TEXT("BEKRÆFT") : TEXT("NEDRIV"), EButton::Demolish, Code,
				DemolishArmed == Code, Site->IsDemolishing());
			continue;
		}
		const FString Why = Map->BuildingBlockReason(SelectedCity, Def.Key);
		if (!Why.IsEmpty())
		{
			PaintText(Geometry, Out, Layer + 2, Why, Row + FVector2D(70.f, 34.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
			continue;
		}
		const bool bAfford = Map->CanAfford(Def.Cost());
		FString Effect;
		if (const FCampaign1851CivilEffect* E = Campaign1851Nations::CivilEffect(Def.Key))
		{
			Effect = FString::Printf(TEXT("  ·  %d job  ·  +%s/år"), E->Jobs, *Thousands(E->TotalRd()));
		}
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s rd.  ·  %d d%s%s"), *Thousands(Def.Cost()), int32(Def.Days()), *Effect, bAfford ? TEXT("") : TEXT("  ·  ikke råd")),
			Row + FVector2D(70.f, 34.f), Serif(11, EFace::Italic), MutedInk, Size.X - 70.f - 110.f);
		PaintButton(Geometry, Out, Layer + 2, ButtonPos, ButtonSize, TEXT("BYG"), EButton::BuildTown, i, false, !bAfford);
	}
	if (Types.IsValidIndex(BuildingInfo))
	{
		// The card is painted on top of everything (Paint), beside this list.
		// To the right of the list; if there is no room there, to its left.
		BuildingCardAnchor = FVector2D(Pos.X + Size.X + 10.f + 430.f > Geometry.GetLocalSize().X - 10.f ? Pos.X - 430.f - 10.f : Pos.X + Size.X + 10.f, Pos.Y);
	}
}

void SCampaign1851Overlay::PaintBuildingCard(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& At, int32 TypeIndex) const
{
	const FCampaign1851SiteModule& Def = ACampaign1851ConstructionSite::TownBuildings()[TypeIndex];
	const FCampaign1851BuildingDef* Data = Campaign1851Buildings::Find(Def.Key);
	const FVector2D Size(430.f, 600.f);
	const FVector2D Pos(FMath::Min(At.X, Geometry.GetLocalSize().X - Size.X - 10.f), FMath::Max(70.f, FMath::Min(At.Y, Geometry.GetLocalSize().Y - 190.f - Size.Y)));
	// In the front: opaque, and it takes the clicks.
	Buttons.Add({ Pos, Pos + Size, EButton::Block, 0 });
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(0.045f, 0.035f, 0.025f, 1.f));
	PaintPanel(Geometry, Out, Layer + 1, Pos, Size);
	PaintText(Geometry, Out, Layer + 2, Def.Name, Pos + FVector2D(22.f, 30.f), Serif(20), Ink, 0.f, false);
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(Size.X - 46.f, 12.f), FVector2D(28.f, 24.f), TEXT("X"), EButton::BuildingInfo, TypeIndex);
	float Y = Pos.Y + 52.f;
	if (Data)
	{
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s  ·  %s  ·  %s  ·  størrelse %s"), *Data->Category, *Data->Owner, *Data->Type, *Data->Size),
			FVector2D(Pos.X + 22.f, Y), Serif(11, EFace::Italic), Gold, Size.X - 44.f);
	}
	Y += 18.f;
	if (TownBrushes.IsValidIndex(TypeIndex) && TownBrushes[TypeIndex]->GetResourceObject())
	{
		FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2D(190.f, 190.f), FSlateLayoutTransform(FVector2D(Pos.X + (Size.X - 190.f) * 0.5f, Y))), TownBrushes[TypeIndex].Get());
	}
	else
	{
		PaintText(Geometry, Out, Layer + 2, TEXT("(billede kommer)"), FVector2D(Pos.X + Size.X * 0.5f, Y + 95.f), Serif(11, EFace::Italic), MutedInk, 0.5f, false);
	}
	Y += 200.f;
	const float ValueX = 130.f;
	auto Line = [&](const FString& Label, const FString& Value)
	{
		PaintText(Geometry, Out, Layer + 2, Label, FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
		// Long values wrap over two lines.
		FString A = Value, B;
		if (Measure(Value, Serif(12)).X > Size.X - ValueX - 22.f)
		{
			int32 Cut = Value.Len() / 2;
			while (Cut < Value.Len() && Value[Cut] != TEXT(' ')) { ++Cut; }
			A = Value.Left(Cut);
			B = Value.Mid(Cut + 1);
		}
		PaintTextFit(Geometry, Out, Layer + 2, A, FVector2D(Pos.X + ValueX, Y), Serif(12), Ink, Size.X - ValueX - 22.f);
		if (!B.IsEmpty())
		{
			Y += 18.f;
			PaintTextFit(Geometry, Out, Layer + 2, B, FVector2D(Pos.X + ValueX, Y), Serif(12), Ink, Size.X - ValueX - 22.f);
		}
		Y += 24.f;
	};
	if (Data)
	{
		Line(TEXT("Giver"), Data->Provides.IsEmpty() ? FString(TEXT("-")) : Data->Provides);
		Line(TEXT("Kræver"), Data->Requires.IsEmpty() ? FString(TEXT("intet særligt")) : Data->Requires);
	}
	if (const FCampaign1851CivilEffect* E = Campaign1851Nations::CivilEffect(Def.Key))
	{
		Line(TEXT("Virkning"), FString::Printf(TEXT("byens vækst +%.2f %% om året"), E->UrbanGrowth));
		Line(TEXT("Jobs"), FString::Printf(TEXT("%d arbejdspladser (arbejderne betaler %s rd./år i skat)"), E->Jobs, *Thousands(E->Jobs * FCampaign1851CivilEffect::JobTaxRd)));
		Line(TEXT("Handel"), E->TradeRd > 0 ? FString::Printf(TEXT("+%s rd./år"), *Thousands(E->TradeRd)) : FString(TEXT("-")));
		Line(TEXT("Afgifter"), E->IncomeRd > 0 ? FString::Printf(TEXT("+%s rd./år"), *Thousands(E->IncomeRd)) : FString(TEXT("-")));
		Line(TEXT("I alt til staten"), FString::Printf(TEXT("%s rd./år"), *Thousands(E->TotalRd())));
	}
	Line(TEXT("Pris"), FString::Printf(TEXT("%s rd.  ·  %d dage"), *Thousands(Def.Cost()), int32(Def.Days())));
	Line(TEXT("Vedligehold"), FString::Printf(TEXT("%s rd. om året"), *Thousands(Def.Upkeep())));
	if (Def.MinPopulation > 0 || Def.FromYear > 0 || Def.bNeedsCoast)
	{
		Line(TEXT("Betingelser"), FString::Printf(TEXT("%s%s%s"), Def.MinPopulation > 0 ? *FString::Printf(TEXT("mindst %s indb.  "), *Thousands(Def.MinPopulation)) : TEXT(""),
			Def.FromYear > 0 ? *FString::Printf(TEXT("fra %d  "), Def.FromYear) : TEXT(""), Def.bNeedsCoast ? TEXT("ved kysten") : TEXT("")));
	}
	if (Map->GetCities().IsValidIndex(SelectedCity))
	{
		const ACampaign1851ConstructionSite* Site = Map->FindBuilding(SelectedCity, Def.Key);
		const FString Why = Map->BuildingBlockReason(SelectedCity, Def.Key);
		Line(TEXT("Her"), Site ? (Site->IsModuleDone(0) ? FString(TEXT("står færdig")) : FString(TEXT("under opførelse"))) : Why.IsEmpty() ? FString(TEXT("kan bygges")) : Why);
	}
}
