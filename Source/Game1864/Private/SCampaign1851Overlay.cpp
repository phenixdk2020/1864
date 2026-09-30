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
	PaintScale = Geometry.Scale;
	Layer = PaintLabels(Geometry, Out, Layer, DistanceKm) + 2;
	PaintProjects(Geometry, Out, Layer);
	PaintLinkWorks(Geometry, Out, Layer);
	PaintArmy(Geometry, Out, Layer);
	Layer += 6;
	PaintTitle(Geometry, Out, Layer);
	PaintLegend(Geometry, Out, Layer);
	PaintCompass(Geometry, Out, Layer, Camera ? Camera->GetYaw() : 0.f);
	PaintScaleBar(Geometry, Out, Layer);
	PaintBornholm(Geometry, Out, Layer);
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
		PaintMenu(Geometry, Out, Layer + 8);
	}

	const FVector2D Size = Geometry.GetLocalSize();
	PaintText(Geometry, Out, Layer, TEXT("Klik: by eller regiment  ·  Højreklik: march  ·  Hjul: zoom  ·  Træk/WASD: panorer  ·  Q/E: drej  ·  Mellemrum: pause  ·  1-5: fart  ·  M: menu  ·  F5/F9"),
		FVector2D(Size.X * 0.5f, Size.Y - 42.f), Serif(12), MutedInk, 0.5f);
	PaintText(Geometry, Out, Layer, TEXT("v00.00.53 LANDE OG MORTERER — UNREAL"), FVector2D(Size.X * 0.5f, Size.Y - 20.f), Serif(9), MutedInk.CopyWithNewOpacity(0.5f), 0.5f);
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
	PaintText(Geometry, Out, Layer + 2, TEXT("1851"), FVector2D(Cx, Pos.Y + 112.f), Serif(29), Ink, 0.5f);
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
			*Thousands(FMath::RoundToInt(C.Population * ACampaign1851Map::UrbanTaxPerHead)), *Thousands(FMath::FloorToInt(Map->GetManpower(AmtIndex))),
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
			Before += FVector2D::Distance(Corps[j].Km, C.Km) < 3.0 ? 1 : 0;
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
				continue;   // riding a train: the train stands for it
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
	Height += bSingle ? 3 * 21.f + 8.f : 0.f;                   // training bars (the table shows them for a stack)
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
	const FVector2D Pos(28.f, Geometry.GetLocalSize().Y - 190.f - Size.Y);
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

	PaintTextFit(Geometry, Out, Layer + 2, bSingle ? First.Name : FString::Printf(TEXT("Kolonne: %d enheder"), Sel.Num()), FVector2D(Pos.X + 22.f, Y), Serif(22), Ink, Inner);
	Y += 30.f;
	PaintTextFit(Geometry, Out, Layer + 2, bSingle ? FString::Printf(TEXT("%s  ·  garnison %s"), Campaign1851Army::ArmName(First.Arm), *Cities[First.Home].Name) : FString(TEXT("Marcherer samlet i den langsomstes tempo")),
		FVector2D(Pos.X + 22.f, Y), Serif(13, EFace::Italic), Gold, Inner);
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

void SCampaign1851Overlay::PaintOrderDialog(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const FOrderDialog& D = OrderDialog;
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const int32 Rows = FMath::Min(D.Units.Num(), 12);
	const float RowH = 28.f;
	const FVector2D Size(880.f, 150.f + Rows * RowH + D.Columns.Num() * 24.f + 70.f);
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
	PaintText(Geometry, Out, Layer + 2, TEXT("Hver vej bliver sin egen kolonne  ·  OPDEL: udskiller enheden, den holder stand  ·  Shift+højreklik: straks"), FVector2D(Pos.X + Size.X - 22.f, Pos.Y + Size.Y - 33.f), Serif(10, EFace::Italic), MutedInk, 1.f, false);
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
		Action(Pos.X + 22.f, EBridgeAction::Build, TEXT("LÆG PONTONBRO  (25.000)"));
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
	const float Cols[] = { 0.f, 170.f, 250.f, 350.f, 450.f, 540.f, 610.f };
	const TCHAR* Heads[] = { TEXT("Land"), TEXT("Styres af"), TEXT("Befolkning"), TEXT("Udv.-budget"), TEXT("Hær"), TEXT("Bane km"), TEXT("Vækst") };
	for (int32 c = 0; c < 7; ++c)
	{
		PaintText(Geometry, Out, Layer + 1, Heads[c], FVector2D(X + Cols[c], Y), Serif(10, EFace::Italic), MutedInk, 0.f, false);
	}
	Y += 20.f;
	TArray<FString> Small;
	for (int32 n = 0; n < Nations.Num(); ++n)
	{
		const FCampaign1851Nation& N = Nations[n];
		const FCampaign1851NationFigures F = Map->NationFigures(n);
		if (!N.bActive)
		{
			continue;
		}
		if (!N.IsPlayer() && n != Map->GetPlayerNation() && N.Group != TEXT("stormagt") && N.Id != TEXT("SE"))
		{
			Small.Add(N.Name);
			continue;
		}
		const FLinearColor C = N.IsPlayer() ? Gold : Ink;
		PaintTextFit(Geometry, Out, Layer + 1, N.Name + (N.bOnMap ? TEXT("") : TEXT(" *")), FVector2D(X + Cols[0], Y), Serif(13), C, 160.f);
		PaintText(Geometry, Out, Layer + 1, N.IsPlayer() ? TEXT("Spilleren") : TEXT("AI"), FVector2D(X + Cols[1], Y), Serif(12), C, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Thousands(int32(F.Population)), FVector2D(X + Cols[2], Y), Serif(12), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Thousands(int32(F.YearlyBudget)), FVector2D(X + Cols[3], Y), Serif(12), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Thousands(int32(F.ArmyMen)), FVector2D(X + Cols[4], Y), Serif(12), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, Thousands(int32(F.RailKm)), FVector2D(X + Cols[5], Y), Serif(12), Ink, 0.f, false);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%.1f %%"), F.Growth), FVector2D(X + Cols[6], Y), Serif(12), Ink, 0.f, false);
		Y += 21.f;
	}
	PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("* abstrakt model (skøn)  ·  mindre stater: %s (se UDENRIGS)"), *FString::Join(Small, TEXT(", "))), FVector2D(X, Y), Serif(10, EFace::Italic), MutedInk, LeftW - 10.f);
	Y += 28.f;
		// The tension with the German Confederation, and war.
		PaintText(Geometry, Out, Layer + 1, Map->IsAtWar() ? TEXT("KRIG med Preussen og Østrig") : TEXT("Spænding med Det tyske forbund"), FVector2D(X, Y), Serif(13), Map->IsAtWar() ? FLinearColor(0.95f, 0.4f, 0.35f) : Ink, 0.f, false);
		PaintBar(Geometry, Out, Layer + 1, FVector2D(X + 260.f, Y - 5.f), 260.f, Map->GetTension() / 100.f);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%.0f / 100  (krig ved 80)"), Map->GetTension()), FVector2D(X + 530.f, Y), Serif(11), Ink, 0.f, false);
		Y += 30.f;
	// The government and opinion.
	{
		const ECampaign1851Current Gov = Map->GetGovernment();
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Regeringen %s  ·  %s  ·  siden %s"), *Map->GetPrimeMinister(), Campaign1851Politics::CurrentName(Gov),
			*ACampaign1851Map::FormatDate(ACampaign1851Map::StartDate() + FTimespan::FromDays(Map->GetGovernmentSince()), true)), FVector2D(X, Y), Serif(13), Gold, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 1, Campaign1851Politics::CurrentEffect(Gov), FVector2D(X, Y + 17.f), Serif(10, EFace::Italic), MutedInk, LeftW - 10.f);
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
			PaintTextFit(Geometry, Out, Layer + 1, Min.Name, FVector2D(X + 158.f, Y), Serif(12), Gold, 150.f);
			PaintTextFit(Geometry, Out, Layer + 1, FString::Printf(TEXT("dygtig %d · sparsom %d · forsigtig %d"), Min.Skill, Min.Thrift, Min.Caution), FVector2D(X + 158.f, Y + 15.f), Serif(9, EFace::Italic), MutedInk, 205.f);
			const int32 Mode = int32(Me.Modes[p]);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 400.f, Y - 6.f), FVector2D(130.f, 24.f), Campaign1851Nations::DelegationName(ECampaign1851Delegation(Mode)),
				EButton::Delegate, p * 3 + (Mode + 1) % 3, Mode == 2);
			PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 540.f, Y - 6.f), FVector2D(110.f, 24.f), TEXT("UDSKIFT"), EButton::MinisterDismiss, p, MinisterPick == p);
			Y += 36.f;
		}
		Y += 6.f;
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("Mindste kassebeholdning: %s rd."), *Thousands(int32(Me.Reserve))), FVector2D(X, Y), Serif(13), Ink, 0.f, false);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 330.f, Y - 12.f), FVector2D(60.f, 24.f), TEXT("-"), EButton::Reserve, 0);
		PaintButton(Geometry, Out, Layer + 1, FVector2D(X + 396.f, Y - 12.f), FVector2D(60.f, 24.f), TEXT("+"), EButton::Reserve, 1);
		PaintText(Geometry, Out, Layer + 1, TEXT("ministerierne bruger kun penge over denne grænse"), FVector2D(X, Y + 20.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
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
	PaintText(Geometry, Out, Layer + 1, TEXT("B E S L U T N I N G E R   O G   A N B E F A L I N G E R"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
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
	Y += 24.f;
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
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Krigsministeriet udvikler ét projekt ad gangen, betalt måned for måned; doktrinen bestemmer, hvordan hæren kæmper"),
		Pos + FVector2D(24.f, 64.f), Serif(12, EFace::Italic), Gold, Size.X - 170.f);
	const float X = Pos.X + 24.f;
	float Y = Pos.Y + 106.f;
	const float LeftW = FMath::Min(1040.f, Size.X * 0.66f);
	PaintText(Geometry, Out, Layer + 1, TEXT("F O R S K N I N G"), FVector2D(X, Y), Serif(11), Gold, 0.f, false);
	Y += 24.f;
	// Columns by branch, rows by year (1852 at the top), a box per topic; click a box to start it.
	const TArray<FCampaign1851ResearchTopic>& Topics = Campaign1851Research::Topics();
	const float YearW = 46.f;
	const float ColW = (LeftW - YearW) / Campaign1851Research::Branches;
	const float Top = Y + 84.f;   // the headings, then the branch symbols
	int32 Years = 1;   // the number of levels
	for (int32 t = 0; t < Topics.Num(); ++t)
	{
		Years = FMath::Max(Years, Campaign1851Research::Tier(t) + 1);
	}
	const float RowH = FMath::Min(120.f, (Pos.Y + Size.Y - 30.f - Top) / Years);
	const FVector2D Box(ColW - 22.f, RowH - 14.f);
	for (int32 c = 0; c < Campaign1851Research::Branches; ++c)
	{
		PaintBranchSymbol(Geometry, Out, Layer + 1, c, FVector2D(X + YearW + c * ColW + ColW * 0.5f, Y + 44.f), 22.f);
		PaintTextFit(Geometry, Out, Layer + 1, FString(Campaign1851Research::BranchName(c)).ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å")), FVector2D(X + YearW + c * ColW + ColW * 0.5f - Box.X * 0.5f, Y), Serif(10), Gold, Box.X);
	}
	for (int32 r = 0; r < Years; ++r)
	{
		const float RY = Top + r * RowH;
		PaintText(Geometry, Out, Layer + 1, Campaign1851Research::Roman(r), FVector2D(X + 8.f, RY + Box.Y * 0.5f), Serif(16), Gold, 0.f, false);
		DrawLines(Geometry, Out, Layer, { FVector2D(X + YearW - 6.f, RY - 7.f), FVector2D(X + LeftW, RY - 7.f) }, Gold.CopyWithNewOpacity(0.12f), 1.f);
	}
	auto BoxPos = [&](const FCampaign1851ResearchTopic& T)
	{
		return FVector2D(X + YearW + T.Branch * ColW + (ColW - Box.X) * 0.5f, Top + Campaign1851Research::Tier(Campaign1851Research::FindTopic(T.Id)) * RowH);
	};
	// The lines first, under the boxes.
	for (const FCampaign1851ResearchTopic& T : Topics)
	{
		const int32 Need = T.Needs ? Campaign1851Research::FindTopic(T.Needs) : INDEX_NONE;
		if (Topics.IsValidIndex(Need))
		{
			const FVector2D From = BoxPos(Topics[Need]) + FVector2D(Box.X * 0.5f, Box.Y);
			const FVector2D To = BoxPos(T) + FVector2D(Box.X * 0.5f, 0.f);
			const float MidY = To.Y - 7.f;
			const FLinearColor C = Map->HasResearch(Topics[Need].Id) ? Gold : MutedInk.CopyWithNewOpacity(0.6f);
			DrawLines(Geometry, Out, Layer + 1, { From, FVector2D(From.X, MidY), FVector2D(To.X, MidY), To }, C, 2.f);
		}
	}
	const FLinearColor Dark = FLinearColor::FromSRGBColor(FColor(30, 22, 12));
	for (int32 t = 0; t < Topics.Num(); ++t)
	{
		const FCampaign1851ResearchTopic& T = Topics[t];
		const FString Why = Map->ResearchBlockReason(t);
		const bool bDone = Why == TEXT("færdig");
		const bool bBusy = Map->GetResearching() == t;
		const FVector2D P = BoxPos(T);
		PaintButton(Geometry, Out, Layer + 2, P, Box, FString(), EButton::ResearchPick, t, bDone || ResearchPick == t);
		const FLinearColor Main = bDone ? Dark : Why.IsEmpty() || bBusy ? Ink : MutedInk;
		PaintTextFit(Geometry, Out, Layer + 4, T.Name, P + FVector2D(8.f, 13.f), Serif(12), Main, Box.X - 16.f);
		const FString State = bDone ? FString(TEXT("færdig"))
			: bBusy ? FString::Printf(TEXT("i gang  ·  %d af %d md."), Map->GetResearchMonths(), T.Months)
			: Why.IsEmpty() ? FString::Printf(TEXT("%d md.  ·  %s rd./md."), T.Months, *Thousands(int32(T.CostPerMonth)))
			: Why;
		PaintTextFit(Geometry, Out, Layer + 4, State, P + FVector2D(8.f, 30.f), Serif(10, EFace::Italic), bDone ? Dark : bBusy ? Gold : MutedInk, Box.X - 16.f);
		if (Box.Y > 56.f)
		{
			PaintTextFit(Geometry, Out, Layer + 4, T.Effect, P + FVector2D(8.f, 46.f), Serif(9, EFace::Italic), bDone ? Dark : MutedInk, Box.X - 16.f);
		}
		if (bBusy)
		{
			PaintBar(Geometry, Out, Layer + 4, P + FVector2D(8.f, Box.Y - 9.f), Box.X - 16.f, float(Map->GetResearchMonths()) / float(T.Months));
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
		PaintText(Geometry, Out, Layer + 8, FString::Printf(TEXT("%s  ·  niveau %s"), Campaign1851Research::BranchName(T.Branch), Campaign1851Research::Roman(Campaign1851Research::Tier(ResearchPick))),
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
		const bool bBusy = Map->GetResearching() == ResearchPick;
		const FString Label = Why == TEXT("færdig") ? FString(TEXT("FÆRDIG")) : bBusy ? FString::Printf(TEXT("I GANG  ·  %d AF %d MD."), Map->GetResearchMonths(), T.Months)
			: Why.IsEmpty() ? (Map->GetResearching() != INDEX_NONE ? FString(TEXT("START (AFBRYDER DET NUVÆRENDE)")) : FString(TEXT("START"))) : Why;
		PaintButton(Geometry, Out, Layer + 8, FVector2D(BP.X + 24.f, BP.Y + BoxSize.Y - 54.f), FVector2D(330.f, 32.f), Label, EButton::ResearchStart, ResearchPick, false, !Why.IsEmpty());
		PaintButton(Geometry, Out, Layer + 8, FVector2D(BP.X + BoxSize.X - 144.f, BP.Y + BoxSize.Y - 54.f), FVector2D(120.f, 32.f), TEXT("LUK"), EButton::ResearchPick, -1);
	}
	// The doctrines.
	const float RX = X + LeftW + 30.f, RW = Size.X - LeftW - 90.f;
	float RY = Pos.Y + 106.f;
	DrawLines(Geometry, Out, Layer + 1, { FVector2D(RX - 14.f, RY - 10.f), FVector2D(RX - 14.f, Pos.Y + Size.Y - 24.f) }, Gold.CopyWithNewOpacity(0.3f), 1.f);
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
	DrawLines(Geometry, Out, Layer + 1, { FVector2D(RX - 14.f, RY - 10.f), FVector2D(RX - 14.f, Pos.Y + Size.Y - 24.f) }, Gold.CopyWithNewOpacity(0.3f), 1.f);
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
	// ---- New unit.
	const float RX = X + LeftW + 30.f, RW = Size.X - LeftW - 90.f;
	float RY = Pos.Y + 110.f;
	DrawLines(Geometry, Out, Layer + 1, { FVector2D(RX - 14.f, RY - 10.f), FVector2D(RX - 14.f, Pos.Y + Size.Y - 24.f) }, Gold.CopyWithNewOpacity(0.3f), 1.f);
	PaintText(Geometry, Out, Layer + 1, TEXT("I N D K A L D   E N   N Y   E N H E D"), FVector2D(RX, RY), Serif(11), Gold, 0.f, false);
	RY += 22.f;
	const float TW = (RW - 5.f * 6.f) / 6.f;
	for (int32 t = 0; t < Campaign1851Resources::UnitTypes; ++t)
	{
		const TCHAR* Short[] = { TEXT("BATAILLON"), TEXT("JÆGERE"), TEXT("DRAGONER"), TEXT("BATTERI"), TEXT("RID. BATTERI"), TEXT("MORTERER") };
		PaintButton(Geometry, Out, Layer + 1, FVector2D(RX + t * (TW + 6.f), RY), FVector2D(TW, 28.f), Short[t], EButton::UnitType, t, RaiseType == t);
	}
	RY += 48.f;
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
	Chooser(TEXT("Garnison"), Town != INDEX_NONE ? Map->GetCities()[Town].Name : FString(TEXT("ingen by med kaserne")), EButton::UnitTown);
	Chooser(TEXT("Hører under"), Command != INDEX_NONE ? Commands[Command].Name : FString(TEXT("-")), EButton::UnitCommand);
	Chooser(TEXT("Øvelser"), Campaign1851Army::ProgramName(ECampaign1851Program(FMath::Clamp(RaiseProgram, 0, int32(ECampaign1851Program::Count) - 1))), EButton::UnitProgram);
	RY += 8.f;
	// What it takes, and what there is.
	const Campaign1851Resources::FUnitType& T = Campaign1851Resources::Type(RaiseType);
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
	Need(TEXT("Penge (rd.)"), float(Map->UnitCost(RaiseType)), float(Map->GetTreasury()), TEXT("udrustning og sold"));
	RY += 14.f;
	const FString Why = Map->UnitBlockReason(RaiseType, Town);
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
	PaintTextFit(Geometry, Out, Layer + 1, TEXT("Midt på kortets udsnit. Et slag bygger sin egen slagmark (SE SLAGMARKEN i slagpanelet)."), FVector2D(RX, Y + 46.f), Serif(10, EFace::Italic), MutedInk, Pos.X + Size.X - RX - 30.f);
	// Legend.
	Y += 80.f;
	const TPair<const TCHAR*, FLinearColor> Legend[] = {
		{ TEXT("Mark"), FLinearColor::FromSRGBColor(FColor(172, 160, 104)) }, { TEXT("Eng og strand"), FLinearColor::FromSRGBColor(FColor(118, 146, 104)) },
		{ TEXT("Skov"), FLinearColor::FromSRGBColor(FColor(52, 84, 46)) }, { TEXT("By"), FLinearColor::FromSRGBColor(FColor(158, 130, 104)) },
		{ TEXT("Hav"), FLinearColor::FromSRGBColor(FColor(62, 96, 124)) }, { TEXT("Chaussé / landevej"), FLinearColor::FromSRGBColor(FColor(224, 206, 160)) },
		{ TEXT("Jernbane"), FLinearColor::FromSRGBColor(FColor(52, 46, 42)) }, { TEXT("Huse og gårde"), FLinearColor::FromSRGBColor(FColor(170, 64, 48)) },
		{ TEXT("Skanse og løbegrav"), FLinearColor::FromSRGBColor(FColor(150, 128, 84)) } };
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	for (const TPair<const TCHAR*, FLinearColor>& L : Legend)
	{
		FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(FVector2D(18.f, 12.f), FSlateLayoutTransform(FVector2D(RX, Y - 6.f))), White, ESlateDrawEffect::None, L.Value);
		PaintText(Geometry, Out, Layer + 1, L.Key, FVector2D(RX + 28.f, Y), Serif(12), Ink, 0.f, false);
		Y += 20.f;
	}
	PaintText(Geometry, Out, Layer + 1, TEXT("Højdekurver for hver 5 m; skyggen falder fra nordvest."), FVector2D(RX, Y + 6.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
}

void SCampaign1851Overlay::PaintOOBChart(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const
{
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const TArray<FCampaign1851Formation>& Forms = Map->GetFormations();
	const TArray<FCampaign1851Officer>& Officers = Map->GetOfficers();
	const TArray<FCampaign1851Command>& Commands = Map->GetCommands();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const FLinearColor Dark = FLinearColor::FromSRGBColor(FColor(30, 22, 12));
	const bool bDraggingUnit = bDragging && TreeKind(DragKey) == ETreeKind::Regiment;
	auto OfficerText = [&](int32 O) { return Officers.IsValidIndex(O) ? FString::Printf(TEXT("%s %s %s"), *Officers[O].Rank, *Officers[O].Name, *FString::ChrN(Campaign1851Army::Stars(Officers[O].Experience), TEXT('*'))) : FString(TEXT("ubesat")); };

	// ---------------------------------------------------------------- left: the units in garrison, to drag in
	const float PoolW = 260.f;
	const int32 GarrisonKey = TreeKey(ETreeKind::Garrisons, 0);
	PaintButton(Geometry, Out, Layer + 2, Pos, FVector2D(PoolW, 26.f), TEXT("I GARNISON"), EButton::TreeRow, GarrisonKey, bDragging && HoverKey == GarrisonKey);
	{
		float Y = Pos.Y + 42.f;
		const float Bottom = Pos.Y + Size.Y - 12.f;
		bool bFull = false;
		for (int32 c = -1; c < Commands.Num() && !bFull; ++c)
		{
			TArray<int32> Units;
			for (int32 i = 0; i < Regs.Num(); ++i)
			{
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
				PaintTextFit(Geometry, Out, Layer + 3, FString::Printf(TEXT("%s [%s]"), *Regs[i].Name, Campaign1851Army::ArmMark(Regs[i].Arm)), FVector2D(Pos.X + 14.f, Y), Serif(10), bSel ? Dark : Ink, PoolW - 70.f);
				PaintText(Geometry, Out, Layer + 3, FString::FromInt(Regs[i].Men), FVector2D(Pos.X + PoolW - 6.f, Y), Serif(10), bSel ? Dark : MutedInk, 1.f, false);
				Y += 16.5f;
			}
			Y += 3.f;
		}
	}

	// ---------------------------------------------------------------- right: the field army as an org chart
	const FVector2D Area(Pos.X + PoolW + 18.f, Pos.Y);
	const FVector2D AreaSize(Size.X - PoolW - 18.f, Size.Y - 18.f);
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
	auto Slot = [&](const FVector2D& Min, const FVector2D& BoxSize, int32 Parent, const FString& A, const FString& B, bool bAlways)
	{
		const int32 Key = TreeKey(ETreeKind::NewFormation, Parent);
		const bool bHover = bDragging && HoverKey == Key;
		if (!bAlways && !bDraggingUnit)
		{
			return;
		}
		FSlateDrawElement::MakeBox(Out, Layer + 5, Geometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(Min)), White, ESlateDrawEffect::None,
			bHover ? Gold.CopyWithNewOpacity(0.85f) : FLinearColor(0.05f, 0.04f, 0.03f, 0.9f));
		Dashed(Min, BoxSize, Gold);
		PaintTextFit(Geometry, Out, Layer + 7, A, FVector2D(Min.X + BoxSize.X * 0.5f, Min.Y + BoxSize.Y * 0.5f - 8.f), Serif(BoxSize.Y > 100.f ? 16 : 11, EFace::Bold), bHover ? Dark : Ink, BoxSize.X - 12.f, 0.5f);
		PaintTextFit(Geometry, Out, Layer + 7, B, FVector2D(Min.X + BoxSize.X * 0.5f, Min.Y + BoxSize.Y * 0.5f + 10.f), Serif(BoxSize.Y > 100.f ? 12 : 9, EFace::Italic), bHover ? Dark : MutedInk, BoxSize.X - 12.f, 0.5f);
		Buttons.Add({ Min, Min + BoxSize, EButton::TreeRow, Key });
	};

	if (Forms.Num() == 0)
	{
		// Nothing in the field yet: the whole right side is the drop zone for the first division.
		const FVector2D ZoneSize(FMath::Min(520.f, AreaSize.X - 60.f), 170.f);
		Slot(Area + (AreaSize - ZoneSize) * 0.5f, ZoneSize, 0, TEXT("Træk en enhed herover"), TEXT("for at lave en ny enhed (1. Division)"), true);
		PaintTextFit(Geometry, Out, Layer + 3, TEXT("Felthæren er tom: hele hæren står i garnison. Træk bataljoner, eskadroner og batterier fra listen til venstre."),
			FVector2D(Area.X + AreaSize.X * 0.5f, Area.Y + AreaSize.Y * 0.5f + 120.f), Serif(11, EFace::Italic), MutedInk, AreaSize.X - 40.f, 0.5f);
		return;
	}

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
			if (F.Parent == Id)
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
	Build(0);
	const float BoxW = 160.f, Gap = 14.f, VGap = 24.f;
	const float HQH = 90.f, UnitH = 78.f, CompH = 42.f;
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
	MeasureNode(0);
	const float MaxScroll = FMath::Max(0.f, Nodes[0].SubW + 200.f - AreaSize.X);
	ChartScroll = FMath::Clamp(ChartScroll, 0.f, MaxScroll);
	ChartScrollY = FMath::Clamp(ChartScrollY, 0.f, 1200.f);

	Out.PushClip(FSlateClippingZone(Geometry.ToPaintGeometry(AreaSize, FSlateLayoutTransform(Area))));
	auto Line = [&](const FVector2D& A, const FVector2D& B) { DrawLines(Geometry, Out, Layer + 2, { A, B }, Gold.CopyWithNewOpacity(0.8f), 1.5f); };
	auto Visible = [&](const FVector2D& Min, const FVector2D& Max) { return Max.X > Area.X && Min.X < Area.X + AreaSize.X; };
	// A box: 0 = headquarters, 1 = unit, 2 = company, 3 = ammunition wagons (no unit of its own yet).
	auto Box = [&](const FVector2D& Min, const FVector2D& BoxSize, int32 Key, int32 Style, const TArray<FString>& Lines, bool bSelected)
	{
		const bool bTarget = bDragging && Key != 0 && HoverKey == Key && Key != DragKey;
		const bool bLit = bSelected || bTarget;
		const FLinearColor Fill = bLit ? Gold.CopyWithNewOpacity(0.9f)
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
			Buttons.Add({ FVector2D(FMath::Max(Min.X, Area.X), Min.Y), FVector2D(FMath::Min(Max.X, Area.X + AreaSize.X), Max.Y), EButton::TreeRow, Key });
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
		ECampaign1851Echelon Echelon = ECampaign1851Echelon::Army;
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
			Echelon = F.Echelon;
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
			// Click an officer line to fill (or change) that post.
			const EButton Posts[] = { EButton::FormationChief, EButton::FormationDeputy, EButton::FormationStaff };
			for (int32 Post = 0; Post < 3; ++Post)
			{
				const float LineY = Min.Y + 9.f + (2 + Post) * 13.5f;
				if (Visible(Min, Min + FVector2D(BoxW, HQH)))
				{
					Buttons.Add({ FVector2D(FMath::Max(Min.X + 2.f, Area.X), LineY - 6.f), FVector2D(FMath::Min(Min.X + BoxW - 20.f, Area.X + AreaSize.X), LineY + 6.f), Posts[Post], F.Id });
				}
			}
		}
		Toggle(Min + FVector2D(BoxW - 17.f, HQH - 17.f), HQKey, Node.bOpen);
		// While a unit is dragged: a slot beside the box for a new formation one level down.
		if (Echelon != ECampaign1851Echelon::Regiment && Echelon != ECampaign1851Echelon::Detachment)
		{
			const ECampaign1851Echelon Next = Echelon == ECampaign1851Echelon::Army ? ECampaign1851Echelon::Division : Echelon == ECampaign1851Echelon::Division ? ECampaign1851Echelon::Brigade : ECampaign1851Echelon::Regiment;
			Slot(Min + FVector2D(BoxW + 6.f, 4.f), FVector2D(118.f, HQH - 8.f), Node.Id, FString::Printf(TEXT("+ ny %s"), *FString(Campaign1851Army::EchelonName(Next)).ToLower()), TEXT("slip her"), false);
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
			Box(FVector2D(X, RowTop), FVector2D(BoxW, UnitH), TreeKey(ETreeKind::Regiment, i), 1, { FString::Printf(TEXT("%s [%s]"), *R.Name, Campaign1851Army::ArmMark(R.Arm)),
				Campaign1851Army::UnitRole(R.Arm), Officers.IsValidIndex(R.Chief) ? OfficerText(R.Chief) : FString::Printf(TEXT("fungerende: %s"), *OfficerText(Senior)),
				FString::Printf(TEXT("NK: %s"), *OfficerText(Senior)), FString::Printf(TEXT("%d/%d mand"), R.Men, R.MaxMen) }, SelectedRegiments.Contains(i));
			// Its companies, stacked under it.
			float Y = RowTop + UnitH + 10.f;
			const float Spine = X + 8.f;
			for (int32 k = 0; k < R.Captains.Num(); ++k)
			{
				Line(FVector2D(Spine, Y - 10.f), FVector2D(Spine, Y + CompH * 0.5f));
				Line(FVector2D(Spine, Y + CompH * 0.5f), FVector2D(X + 16.f, Y + CompH * 0.5f));
				Box(FVector2D(X + 16.f, Y), FVector2D(BoxW - 16.f, CompH), TreeKey(ETreeKind::Company, i * 10 + k), 2,
					{ FString::Printf(TEXT("%d. Kompagni [I]"), Map->CompanyNumber(i, k)), OfficerText(R.Captains[k]),
					  FString::Printf(TEXT("%d/%d mand"), Map->CompanyMen(i, k), R.MaxMen / FMath::Max(1, R.Captains.Num())) }, false);
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
				if (s > 0)
				{
					Line(FVector2D(X + BoxW * 0.5f, Y - 10.f), FVector2D(X + BoxW * 0.5f, Y));
				}
				Box(FVector2D(X, Y), FVector2D(BoxW, UnitH), TreeKey(ETreeKind::Regiment, Node.Support[s]), 1, { FString::Printf(TEXT("%s [%s]"), *R.Name, Campaign1851Army::ArmMark(R.Arm)),
					Campaign1851Army::UnitRole(R.Arm), OfficerText(R.Chief), R.Guns > 0 ? FString::Printf(TEXT("%d mand  ·  %d kanoner"), R.Men, R.Guns) : FString::Printf(TEXT("%d/%d mand  ·  %d heste"), R.Men, R.MaxMen, R.Horses) },
					SelectedRegiments.Contains(Node.Support[s]));
				Y += UnitH + 10.f;
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
	Place(0, Area.X + 12.f + FMath::Max(0.f, (AreaSize.X - 150.f - Nodes[0].SubW) * 0.5f) - ChartScroll, Area.Y + 4.f - ChartScrollY);
	Out.PopClip();

	PaintText(Geometry, Out, Layer + 3, FString::Printf(TEXT("Klik: vælg  ·  klik på en officer i et HQ: udnævn  ·  træk en kasse hen på en anden: flyt  ·  træk en enhed hen på \"+ ny\": ny formation  ·  -/+: fold  ·  hjul: rul til siden, Shift+hjul: op/ned%s"),
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
	auto RegimentRow = [&](int32 i, int32 Depth)
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
		Row.bOpen = Collapsed.Contains(Row.Key);
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
	Field.Info = Forms.Num() > 0 ? FString::Printf(TEXT("%s mand"), *Thousands(FieldMen)) : FString(TEXT("ingen formationer: træk enheder hertil efter NY BRIGADE"));
	Field.bHasChildren = Forms.Num() > 0;
	Field.bOpen = !Collapsed.Contains(Field.Key);
	Rows.Add(Field);
	if (Field.bOpen)
	{
		for (const FCampaign1851Formation& F : Forms)
		{
			if (F.Parent == 0)
			{
				AddFormation(F.Id, 1);
			}
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
	switch (ETreeKind(Key / 100000))
	{
	case ETreeKind::Regiment: return Map->GetRegiments().IsValidIndex(Id) ? Map->GetRegiments()[Id].Name : FString();
	case ETreeKind::Formation: return Map->FormationIndex(Id) != INDEX_NONE ? Map->GetFormations()[Map->FormationIndex(Id)].Name : FString();
	case ETreeKind::Company: return FString::Printf(TEXT("%d. Kompagni"), Map->CompanyNumber(Id / 10, Id % 10));
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
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(190.f, 12.f), FVector2D(116.f, 26.f), TEXT("NY DIVISION"), EButton::TreeNew, int32(ECampaign1851Echelon::Division));
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(312.f, 12.f), FVector2D(116.f, 26.f), TEXT("NY BRIGADE"), EButton::TreeNew, int32(ECampaign1851Echelon::Brigade));
	PaintButton(Geometry, Out, Layer + 2, Pos + FVector2D(434.f, 12.f), FVector2D(116.f, 26.f), TEXT("NYT REGIMENT"), EButton::TreeNew, int32(ECampaign1851Echelon::Regiment));
	PaintText(Geometry, Out, Layer + 2, TEXT("Træk enheder og formationer på plads  ·  klik: vælg  ·  hjul: rul"), Pos + FVector2D(22.f, 56.f), Serif(10, EFace::Italic), MutedInk, 0.f, false);
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
	const TArray<int32> Pool = Map->OfficerPool(bGenerals);
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
	const float RowHeight = 34.f;
	const FVector2D Size(460.f, 110.f + (CardStats - (O.bGeneral ? 0 : 1)) * RowHeight + 70.f);
	const FVector2D Pos(BottomLeft.X, BottomLeft.Y - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseOfficerCard);
	float Y = Pos.Y + 32.f;
	PaintTextFit(Geometry, Out, Layer + 2, O.Name, FVector2D(Pos.X + 22.f, Y), Serif(20), Ink, Size.X - 80.f);
	Y += 28.f;
	const int32 Age = Map->GetDate().GetYear() - O.Born;
	const FString Post = Map->OfficerRole(InspectedOfficer);
	PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s  ·  %d år  ·  %s%s"), *O.Rank, Age, *Post, O.bRecruited ? TEXT("  ·  ansat under felttoget") : TEXT("")),
		FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, Size.X - 44.f);
	Y += 26.f;
	PaintText(Geometry, Out, Layer + 2, TEXT("Erfaring"), FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
	PaintBar(Geometry, Out, Layer + 2, FVector2D(Pos.X + 150.f, Y - 3.f), 200.f, O.Experience / 100.f);
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%.0f  %s"), O.Experience, *FString::ChrN(Campaign1851Army::Stars(O.Experience), TEXT('*'))), FVector2D(Pos.X + 362.f, Y), Serif(12), Ink, 0.f, false);
	Y += 24.f;
	for (int32 s = 0; s < CardStats; ++s)
	{
		if (s == int32(ECampaign1851OfficerStat::Political) && !O.bGeneral)
		{
			continue;   // only generals have political weight
		}
		PaintText(Geometry, Out, Layer + 2, Campaign1851Army::StatName(ECampaign1851OfficerStat(s)), FVector2D(Pos.X + 22.f, Y), Serif(12), Ink, 0.f, false);
		PaintBar(Geometry, Out, Layer + 2, FVector2D(Pos.X + 150.f, Y - 3.f), 200.f, O.Stats[s] / 10.f);
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%d"), O.Stats[s]), FVector2D(Pos.X + 362.f, Y), Serif(12), Ink, 0.f, false);
		PaintTextFit(Geometry, Out, Layer + 2, Meaning[s], FVector2D(Pos.X + 150.f, Y + 14.f), Serif(9, EFace::Italic), MutedInk, 290.f);
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
	const TCHAR* Labels[] = { TEXT("HÆREN"), TEXT("OFFICERER"), TEXT("KASSEN"), TEXT("BYER"), TEXT("TOG"), TEXT("KAMPORDEN"), TEXT("STATSRÅD"), TEXT("FORSYNING"), TEXT("UDENRIGS"), TEXT("FORSKNING"), TEXT("FLÅDEN") };
	// Small square buttons: a drawn symbol above, the name small below.
	const float W = 74.f, H = 46.f, Gap = 3.f;
	const float Total = 11.f * W + 10.f * Gap;
	const float X0 = FMath::Max((Geometry.GetLocalSize().X - Total) * 0.5f, 484.f);
	const FLinearColor Dark = FLinearColor::FromSRGBColor(FColor(30, 22, 12));
	for (int32 i = 0; i < 11; ++i)
	{
		const bool bOn = int32(Window) == i + 1;
		const FVector2D P(X0 + i * (W + Gap), 90.f);
		PaintButton(Geometry, Out, Layer, P, FVector2D(W, H), FString(), EButton::MainMenu, i + 1, bOn);
		PaintMenuIcon(Geometry, Out, Layer + 2, i, P + FVector2D(W * 0.5f, 17.f), 11.f, bOn ? Dark : Ink);
		PaintTextFit(Geometry, Out, Layer + 2, Labels[i], FVector2D(P.X + W * 0.5f - FMath::Min(Measure(Labels[i], Serif(8)).X, W - 6.f) * 0.5f, P.Y + H - 9.f), Serif(8), bOn ? Dark : Gold, W - 6.f);
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
	const bool bBig = Window == EWindow::Chart;
	const FVector2D Size(FMath::Min(bBig ? 1860.f : 1460.f, Screen.X - (bBig ? 40.f : 80.f)), FMath::Min(bBig ? 930.f : 820.f, Screen.Y - (bBig ? 140.f : 200.f)));
	const FVector2D Pos((Screen.X - Size.X) * 0.5f, 132.f);
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
		Y += 18.f;
		PaintButton(Geometry, Out, Layer + 3, FVector2D(LX, Y), FVector2D(170.f, 26.f), TEXT("LÅN 100.000"), EButton::Loan, 0, false, !Map->LoanBlockReason(100000.0).IsEmpty());
		PaintButton(Geometry, Out, Layer + 3, FVector2D(LX + 180.f, Y), FVector2D(170.f, 26.f), TEXT("LÅN 250.000"), EButton::Loan, 1, false, !Map->LoanBlockReason(250000.0).IsEmpty());
		PaintButton(Geometry, Out, Layer + 3, FVector2D(LX + 360.f, Y), FVector2D(170.f, 26.f), TEXT("AFDRAG 100.000"), EButton::Loan, 2, false, Map->GetDebt() < 1.0 || Map->GetTreasury() < 100000.0);

		const float RX = Pos.X + 640.f, RW = Size.X - 640.f - 24.f;
		const TArray<FCampaign1851Transaction>& Ledger = Map->GetLedger();
		PaintText(Geometry, Out, Layer + 3, TEXT("R E G N S K A B"), FVector2D(RX, Pos.Y + 110.f), Serif(11), Gold, 0.f, false);
		const int32 Shown = FMath::Min(Ledger.Num(), VisibleRows);
		for (int32 k = 0; k < Shown; ++k)
		{
			const FCampaign1851Transaction& T = Ledger[Ledger.Num() - 1 - k];
			const float TY = Pos.Y + 140.f + k * 23.f;
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
		PaintText(Geometry, Out, Layer + 3, TEXT("T O G E N E"), FVector2D(Pos.X + 24.f, Y), Serif(11), Gold, 0.f, false);
		Y += 30.f;
		const TArray<FCampaign1851TroopTrain>& List = Map->GetTroopTrainList();
		for (int32 t = 0; t < List.Num() && Y < Pos.Y + Size.Y - 260.f; ++t)
		{
			Line(FString::Printf(TEXT("Tog %d"), List[t].Id), Map->DescribeTrain(t));
		}
		Y += 16.f;
		PaintText(Geometry, Out, Layer + 3, TEXT("B E S T I L T"), FVector2D(Pos.X + 24.f, Y), Serif(11), Gold, 0.f, false);
		Y += 30.f;
		if (Map->GetTrainOrders().Num() == 0)
		{
			Line(TEXT("Ingen tog i bestilling"), FString());
		}
		for (const FVector2D& O : Map->GetTrainOrders())
		{
			const FDateTime Arrive = ACampaign1851Map::StartDate() + FTimespan::FromDays(O.Y);
			Line(FString::Printf(TEXT("%d togsæt fra England"), int32(O.X)), FString::Printf(TEXT("leveres ca. %s"), *ACampaign1851Map::FormatDate(Arrive, true)));
		}
		Y += 24.f;
		PaintButton(Geometry, Out, Layer + 3, FVector2D(Pos.X + 24.f, Y), FVector2D(420.f, 32.f),
			FString::Printf(TEXT("BESTIL TOGSÆT  (%s rd.  ·  levering %.0f dage)"), *Thousands(ACampaign1851Map::TroopTrainCost), ACampaign1851Map::TroopTrainDeliveryDays),
			EButton::TrainOrder, 0, false, Map->GetTreasury() < ACampaign1851Map::TroopTrainCost);
		PaintText(Geometry, Out, Layer + 3, TEXT("Lokomotiv og vogne bygges i England og skibes til Danmark. Uden ledige tog marcherer kolonnen i stedet."),
			FVector2D(Pos.X + 24.f, Y + 56.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
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
	else if (Window == EWindow::Chart)
	{
		Title(TEXT("Kamporden"), TEXT("Felthæren som organisation: hovedkvarterer og deres enheder"));
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
				Thousands(FMath::RoundToInt(C.Population * ACampaign1851Map::UrbanTaxPerHead)), Thousands(FMath::RoundToInt(C.Population * 0.09)),
				FString::FromInt(Units), Thousands(Men), FString::FromInt(Built), Map->HasStation(c) ? FString(TEXT("ja")) : FString(TEXT("-")) };
			Row.Keys = { Text, Text, Text, double(C.Population), C.Population * ACampaign1851Map::UrbanTaxPerHead, C.Population * 0.09, double(Units), double(Men), double(Built), Text };
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
	const FVector2D Size(720.f, 120.f + MenuSlots.Num() * RowHeight + 60.f);
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
	const FVector2D Size(660.f, 56.f);
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
	const FString Weather = FString::Printf(TEXT("%s %.0f°"), Campaign1851Weather::Name(Map->GetWeather()), Map->GetTemperature());
	const float SeasonW = FMath::Max(Measure(Season, SeasonFont).X, Measure(Weather, SeasonFont).X);
	const float Room = X - 12.f - TextX - SeasonW - 12.f;
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
	const FVector2D Size(420.f, 318.f);
	const FVector2D Pos(28.f, Geometry.GetLocalSize().Y - 190.f - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintCloseX(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X, 0.f), CloseSelection);
	float Y = Pos.Y + 32.f;
	auto Line = [&](const FString& Label, const FString& Value)
	{
		PaintText(Geometry, Out, Layer + 2, Label, FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
		PaintText(Geometry, Out, Layer + 2, Value, FVector2D(Pos.X + 158.f, Y), Serif(13), Ink, 0.f, false);
		Y += 24.f;
	};
	PaintText(Geometry, Out, Layer + 2, A->Name, FVector2D(Pos.X + 22.f, Y), Serif(24), Ink, 0.f);
	Y += 32.f;
	PaintText(Geometry, Out, Layer + 2, ACampaign1851Map::RegionName(A->Region), FVector2D(Pos.X + 22.f, Y), Serif(14, EFace::Italic), Gold, 0.f, false);
	Y += 32.f;
	Line(TEXT("Amtsby"), A->Seat);
	Line(TEXT("Befolkning"), FString::Printf(TEXT("ca. %s  ·  by %s"), *Thousands(A->Population), *Thousands(A->Urban)));
	Line(TEXT("Areal"), FString::Printf(TEXT("%s km²  ·  %d indb./km²"), *Thousands(FMath::RoundToInt(A->AreaKm2)), FMath::RoundToInt(A->Population / FMath::Max(A->AreaKm2, 1.f))));
	Line(TEXT("Købstæder"), A->Towns.Num() > 0 ? FString::Join(A->Towns, TEXT(", ")).Left(36) : TEXT("ingen"));
	Line(TEXT("Skat til anlæg"), FString::Printf(TEXT("%s rd./år"), *Thousands(FMath::RoundToInt(ACampaign1851Map::AmtYearlyTax(*A)))));
	Line(TEXT("Våbenføre mænd"), FString::Printf(TEXT("ca. %s (skøn)"), *Thousands(FMath::RoundToInt(A->Population * 0.09))));
	Line(TEXT("Garnisoner"), Garrisons.Num() > 0 ? FString::Join(Garrisons, TEXT(", ")).Left(36) : TEXT("ingen"));
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
	const FVector2D Size(470.f, 48.f + FMath::Max(Rows.Num(), 1) * RowHeight + 8.f);
	const FVector2D Pos(28.f + 440.f + 10.f, FMath::Max(130.f, Geometry.GetLocalSize().Y - 190.f - Size.Y));
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
	for (int32 r = 0; r < Rows.Num(); ++r)
	{
		const int32 i = Rows[r];
		const FCampaign1851SiteModule& Def = Types[i];
		const FVector2D Row = Pos + FVector2D(0.f, 44.f + r * RowHeight);
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
			Effect = FString::Printf(TEXT("  ·  +%.2f %% vækst%s"), E->UrbanGrowth, E->IncomeRd > 0 ? *FString::Printf(TEXT(", +%s rd./år"), *Thousands(E->IncomeRd)) : TEXT(""));
		}
		PaintTextFit(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s rd.  ·  %d dage%s%s"), *Thousands(Def.Cost()), int32(Def.Days()), *Effect, bAfford ? TEXT("") : TEXT("  ·  ikke råd")),
			Row + FVector2D(70.f, 34.f), Serif(11, EFace::Italic), MutedInk, Size.X - 70.f - 110.f);
		PaintButton(Geometry, Out, Layer + 2, ButtonPos, ButtonSize, TEXT("BYG"), EButton::BuildTown, i, false, !bAfford);
	}
}
