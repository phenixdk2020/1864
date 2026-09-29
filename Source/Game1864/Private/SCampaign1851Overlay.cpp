#include "SCampaign1851Overlay.h"

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
	PaintButton(Geometry, Out, Layer, FVector2D(28.f, 206.f), FVector2D(150.f, 28.f), TEXT("SPILMENU  (M)"), EButton::Menu);
	PaintCalendar(Geometry, Out, Layer);
	PaintTreasury(Geometry, Out, Layer);
	if (bLedgerOpen)
	{
		PaintLedger(Geometry, Out, Layer + 4);
	}
	PaintToast(Geometry, Out, Layer + 6);
	if (bMenuOpen)
	{
		// The menu takes the clicks: only its own buttons stay live.
		Buttons.Reset();
		PaintMenu(Geometry, Out, Layer + 8);
	}

	const FVector2D Size = Geometry.GetLocalSize();
	PaintText(Geometry, Out, Layer, TEXT("Klik: by eller regiment  ·  Højreklik: march  ·  Hjul: zoom  ·  Træk/WASD: panorer  ·  Q/E: drej  ·  Mellemrum: pause  ·  1-5: fart  ·  M: menu  ·  F5/F9"),
		FVector2D(Size.X * 0.5f, Size.Y - 42.f), Serif(12), MutedInk, 0.5f);
	PaintText(Geometry, Out, Layer, TEXT("v00.00.29 HÆREN — UNREAL"), FVector2D(Size.X * 0.5f, Size.Y - 20.f), Serif(9), MutedInk.CopyWithNewOpacity(0.5f), 0.5f);
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
		else if (L.Kind == TEXT("duchy")) { I.Text = Spaced(L.Text.ToUpper()); I.Font = Serif(13); I.Colour = Gold; I.Priority = 800000; }
		else if (L.Kind == TEXT("amt")) { if (D <= 30.f || D >= 330.f) continue; I.Text = Spaced(L.Text.ToUpper()); I.Font = Serif(10); I.Colour = FLinearColor::FromSRGBColor(FColor(232, 214, 160, 190)); I.Priority = 600; }
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
	PaintTownBuildings(Geometry, Out, Layer);
	const FVector2D Size(380.f, C.bForeign ? 150.f : 176.f);
	const ACampaign1851ConstructionSite* Site = C.bHasPlot ? Map->FindProject(SelectedCity) : nullptr;
	const int32 ModuleRows = Site && Site->IsBarracksDone() ? Site->NumModules() - 1 : 0;
	const float RowHeight = 54.f;
	const float CardHeight = 150.f + (ModuleRows > 0 ? ModuleRows * RowHeight + 10.f : 0.f);
	const FVector2D Pos(28.f, Geometry.GetLocalSize().Y - 190.f - Size.Y - (C.bHasPlot ? CardHeight - 4.f : 0.f));
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintText(Geometry, Out, Layer + 2, C.Name, Pos + FVector2D(22.f, 32.f), Serif(24), Ink, 0.f);
	PaintText(Geometry, Out, Layer + 2, C.bForeign ? TEXT("Udenlandsk by") : C.bCapital ? TEXT("Hovedstad") : TEXT("Købstad"), Pos + FVector2D(22.f, 64.f), Serif(14, EFace::Italic), Gold, 0.f, false);
	const FCampaign1851Amt* Amt = Map->FindAmt(C.AmtId);
	PaintText(Geometry, Out, Layer + 2, C.bForeign ? TEXT("Uden for monarkiet") : Amt ? FString::Printf(TEXT("%s  ·  %s"), *Amt->Name, *ACampaign1851Map::RegionName(Amt->Region)) : RegionName(C.Region),
		Pos + FVector2D(22.f, 92.f), Serif(13), Ink, 0.f, false);
	PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("ca. %s indbyggere (ca. 1850)"), *Thousands(C.Population)), Pos + FVector2D(22.f, 120.f), Serif(13), Ink, 0.f, false);
	if (!C.bForeign)
	{
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("Skat %s rd./år  ·  våbenføre ca. %s mænd"),
			*Thousands(FMath::RoundToInt(C.Population * ACampaign1851Map::UrbanTaxPerHead)), *Thousands(FMath::RoundToInt(C.Population * 0.09))),
			Pos + FVector2D(22.f, 146.f), Serif(12, EFace::Italic), Gold, 0.f, false);
	}
	if (!C.bHasPlot)
	{
		return;
	}

	// Garrison (design manual 20.16.5-7): the barracks card, then the modules around the parade ground.
	const FVector2D Card = Pos + FVector2D(0.f, Size.Y - 4.f);
	PaintPanel(Geometry, Out, Layer, Card, FVector2D(Size.X, CardHeight));
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

	for (int32 m = 1; m <= ModuleRows; ++m)
	{
		const FVector2D Row = Card + FVector2D(0.f, 150.f + (m - 1) * RowHeight);
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
	PaintText(Geometry, Out, Layer + 1, Text, Pos + Size * 0.5f, Serif(11),
		bHighlight ? FLinearColor::FromSRGBColor(FColor(30, 22, 12)) : bDisabled ? MutedInk.CopyWithNewOpacity(0.45f) : Ink, 0.5f, false);
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
		Arc(FMath::Max(Progress, 0.01f), Gold, 3.f);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%d"), FMath::FloorToInt(Progress * 100.f)), Centre, Serif(9), Ink, 0.5f, false);
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
		Arc(FMath::Max(Progress, 0.01f), Gold, 3.f);
		PaintText(Geometry, Out, Layer + 1, FString::Printf(TEXT("%d"), FMath::FloorToInt(Progress * 100.f)), P, Serif(9), Ink, 0.5f, false);
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
		else if (Arm == ECampaign1851Arm::Artillery)
		{
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
		if (ToLocal(Geometry, Cities[R.Destination()].World, End))
		{
			PaintDot(Geometry, Out, Layer + 1, End, 14.f, RouteBlue);
			PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("ankomst %s %s"), *ACampaign1851Map::FormatClock(Now + FTimespan::FromDays(R.DaysLeft())),
				*ACampaign1851Map::FormatDate(Now + FTimespan::FromDays(R.DaysLeft()), true)), End + FVector2D(12.f, 16.f), Serif(11, EFace::Italic), RouteBlue, 0.f);
		}
	}

	// Stacks: every regiment in a town together, every column on the march together.
	TMap<int64, TArray<int32>> Stacks;
	for (int32 i = 0; i < Regs.Num(); ++i)
	{
		const FCampaign1851Regiment& R = Regs[i];
		const int64 Key = !R.IsMarching() ? int64(R.Town) : R.Group ? (int64(1) << 32) + R.Group : (int64(2) << 32) + i;
		Stacks.FindOrAdd(Key).Add(i);
	}
	const FVector2D Box(46.f, 30.f);
	for (const TPair<int64, TArray<int32>>& Stack : Stacks)
	{
		const FCampaign1851Regiment& Top = Regs[Stack.Value[0]];
		FVector2D P;
		const FVector World = Top.IsMarching() ? Map->RegimentWorld(Stack.Value[0]) : Cities[Top.Town].World;
		if (!ToLocal(Geometry, World, P))
		{
			continue;
		}
		// In a town the counter stands above the dot; on the march on the column.
		const FVector2D Min = P + (Top.IsMarching() ? FVector2D(-Box.X * 0.5f, -Box.Y * 0.5f) : FVector2D(-Box.X * 0.5f, -Box.Y - 14.f));
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

void SCampaign1851Overlay::PaintArmyInfo(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
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
	const int32 Rows = FMath::Min(Sel.Num(), 8);
	const FVector2D Size(460.f, 262.f + (Sel.Num() > 1 ? Rows * 24.f + 8.f : 0.f));
	const FVector2D Pos(28.f, Geometry.GetLocalSize().Y - 190.f - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	float Y = Pos.Y + 32.f;
	int32 Men = 0, MaxMen = 0, Horses = 0, Guns = 0;
	for (const FCampaign1851Regiment* R : Sel)
	{
		Men += R->Men;
		MaxMen += R->MaxMen;
		Horses += R->Horses;
		Guns += R->Guns;
	}
	PaintText(Geometry, Out, Layer + 2, Sel.Num() == 1 ? First.Name : FString::Printf(TEXT("Kolonne: %d enheder"), Sel.Num()), FVector2D(Pos.X + 22.f, Y), Serif(22), Ink, 0.f);
	Y += 30.f;
	PaintText(Geometry, Out, Layer + 2, Sel.Num() == 1 ? FString::Printf(TEXT("%s  ·  garnison %s"), Campaign1851Army::ArmName(First.Arm), *Cities[First.Home].Name) : FString(TEXT("Marcherer samlet i den langsomstes tempo")),
		FVector2D(Pos.X + 22.f, Y), Serif(13, EFace::Italic), Gold, 0.f, false);
	Y += 30.f;
	auto Line = [&](const FString& Label, const FString& Value)
	{
		PaintText(Geometry, Out, Layer + 2, Label, FVector2D(Pos.X + 22.f, Y), Serif(12, EFace::Italic), Gold, 0.f, false);
		PaintText(Geometry, Out, Layer + 2, Value, FVector2D(Pos.X + 140.f, Y), Serif(13), Ink, 0.f, false);
		Y += 22.f;
	};
	Line(TEXT("Styrke"), FString::Printf(TEXT("%d / %d mand%s%s"), Men, MaxMen, Horses > 50 ? *FString::Printf(TEXT("  ·  %d heste"), Horses) : TEXT(""),
		Guns > 0 ? *FString::Printf(TEXT("  ·  %d kanoner"), Guns) : TEXT("")));
	FString Where;
	if (First.IsMarching())
	{
		const FCampaign1851Leg& Leg = First.Route[First.Leg];
		Where = FString::Printf(TEXT("%s %s → %s"), Leg.bRail ? TEXT("Med tog") : TEXT("På march"), *Cities[Leg.From].Name, *Cities[Leg.To].Name);
	}
	else
	{
		Where = FString::Printf(TEXT("I %s"), Cities.IsValidIndex(First.Town) ? *Cities[First.Town].Name : TEXT("?"));
	}
	Line(TEXT("Hvor"), Where);
	FString Why;
	const float Pace = Campaign1851Army::ColumnPace(Sel, &Why);
	Line(TEXT("Marchfart"), FString::Printf(TEXT("%.0f km/dag på landevej, %.0f på chaussé"), Pace, Pace * Campaign1851Network::MarchKmPerDayChaussee / Campaign1851Network::MarchKmPerDayRoad));
	if (!Why.IsEmpty())
	{
		PaintText(Geometry, Out, Layer + 2, Why, FVector2D(Pos.X + 22.f, Y), Serif(11, EFace::Italic), MutedInk, 0.f, false);
		Y += 20.f;
	}
	if (First.IsMarching())
	{
		const FDateTime Arrive = Map->GetDate() + FTimespan::FromDays(First.DaysLeft());
		Line(TEXT("Mål"), FString::Printf(TEXT("%s  ·  ankomst %s %s"), *Cities[First.Destination()].Name, *ACampaign1851Map::FormatClock(Arrive), *ACampaign1851Map::FormatDate(Arrive, true)));
	}
	if (Sel.Num() > 1)
	{
		Y += 6.f;
		for (int32 r = 0; r < Rows; ++r)
		{
			const FVector2D Row(Pos.X + 18.f, Y - 11.f);
			PaintButton(Geometry, Out, Layer + 2, Row, FVector2D(Size.X - 36.f, 21.f),
				FString::Printf(TEXT("%s  ·  %s  ·  %d mand"), *Sel[r]->Name, Campaign1851Army::ArmName(Sel[r]->Arm), Sel[r]->Men), EButton::RegimentRow, SelectedRegiments[r]);
			Y += 24.f;
		}
	}
	const FVector2D ButtonSize(150.f, 28.f);
	const float ButtonY = Pos.Y + Size.Y - 70.f;
	PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 22.f, ButtonY), ButtonSize, TEXT("TIL GARNISON"), EButton::ArmyHome);
	if (First.IsMarching())
	{
		PaintButton(Geometry, Out, Layer + 2, FVector2D(Pos.X + 32.f + ButtonSize.X, ButtonY), ButtonSize, TEXT("HOLDT"), EButton::ArmyHalt);
	}
	PaintText(Geometry, Out, Layer + 2, TEXT("Højreklik på en by: march dertil  ·  Shift-klik: vælg flere  ·  Esc: fravælg"), FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + Size.Y - 20.f),
		Serif(10, EFace::Italic), MutedInk, 0.5f, false);
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
	}
	PaintButton(Geometry, Out, Layer + 3, Pos + FVector2D(Size.X - 144.f, Size.Y - 50.f), FVector2D(120.f, 30.f), TEXT("LUK"), EButton::CloseMenu);
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
	const FVector2D Pos((Geometry.GetLocalSize().X - Size.X) * 0.5f, 100.f);   // under the date panel
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
	PaintText(Geometry, Out, Layer + 2, Date, FVector2D(TextX, Pos.Y + Size.Y * 0.5f), DateFont, Ink, 0.f);
	PaintText(Geometry, Out, Layer + 2, Map->GetSeasonName(), FVector2D(TextX + Measure(Date, DateFont).X + 12.f, Pos.Y + Size.Y * 0.5f + 2.f), Serif(13, EFace::Italic), Gold, 0.f, false);
	const float PauseWidth = 62.f, ButtonWidth = 34.f, Gap = 5.f;
	float X = Pos.X + Size.X - 16.f - PauseWidth - (ACampaign1851Map::NumSpeeds() - 1) * (ButtonWidth + Gap);
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
	const FVector2D Pos(28.f, 246.f), Size(300.f, 66.f);
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
	PaintText(Geometry, Out, Layer + 2, TEXT("Priser er statens andel  ·  20 % udbetales ved start, resten som dagløn"), FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + Size.Y - 12.f),
		Serif(9, EFace::Italic), MutedInk, 0.5f, false);
}

void SCampaign1851Overlay::PaintTownBuildings(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const
{
	const TArray<FCampaign1851SiteModule>& Types = ACampaign1851ConstructionSite::TownBuildings();
	TArray<int32> Rows;
	for (int32 i = 0; i < Types.Num(); ++i)
	{
		if (Map->BuildingBlockReason(SelectedCity, Types[i].Key) != TEXT("-"))
		{
			Rows.Add(i);
		}
	}
	PaintTownLinks(Geometry, Out, Layer, 28.f + 380.f + 10.f + (Rows.Num() > 0 ? 410.f + 10.f : 0.f));
	if (Rows.Num() == 0)
	{
		return;
	}
	const float RowHeight = 50.f;
	const FVector2D Size(410.f, 48.f + Rows.Num() * RowHeight + 8.f);
	const FVector2D Pos(28.f + 380.f + 10.f, Geometry.GetLocalSize().Y - 190.f - Size.Y);
	PaintPanel(Geometry, Out, Layer, Pos, Size);
	PaintText(Geometry, Out, Layer + 2, TEXT("B Y G N I N G E R"), Pos + FVector2D(22.f, 24.f), Serif(11), Gold, 0.f, false);
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
			PaintText(Geometry, Out, Layer + 2, bDone ? FString(TEXT("Færdig")) : ProgressLine(Site, 0), Row + FVector2D(70.f, 34.f), Serif(11, EFace::Italic), bDone ? Gold : Ink, 0.f, false);
			PaintButton(Geometry, Out, Layer + 2, ButtonPos, ButtonSize, TEXT("VIS"), EButton::ShowSite, i);
			continue;
		}
		const FString Why = Map->BuildingBlockReason(SelectedCity, Def.Key);
		if (!Why.IsEmpty())
		{
			PaintText(Geometry, Out, Layer + 2, Why, Row + FVector2D(70.f, 34.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
			continue;
		}
		const bool bAfford = Map->CanAfford(Def.Cost());
		PaintText(Geometry, Out, Layer + 2, FString::Printf(TEXT("%s rd.  ·  %d dage%s"), *Thousands(Def.Cost()), int32(Def.Days()), bAfford ? TEXT("") : TEXT("  ·  ikke råd")),
			Row + FVector2D(70.f, 34.f), Serif(11, EFace::Italic), MutedInk, 0.f, false);
		PaintButton(Geometry, Out, Layer + 2, ButtonPos, ButtonSize, TEXT("BYG"), EButton::BuildTown, i, false, !bAfford);
	}
}
