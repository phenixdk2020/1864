#include "StrategyHUD.h"

#include "StrategyPlayerController.h"
#include "../AI/StrategyDoctrineComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "../Units/StrategyUnit.h"
#include "Engine/Canvas.h"
#include "../Tests/StrategyOOBTestScenario.h"
#include "StrategyCameraPawn.h"
#include "EngineUtils.h"

namespace
{
    // The Unity prototype's palette: dark panels, gold headings, red orders (blue while executing, green chosen).
    const FLinearColor PanelColour(0.018f, 0.022f, 0.026f, 0.90f);
    const FLinearColor RowColour(0.035f, 0.042f, 0.048f, 0.85f);
    const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(232, 182, 72));
    const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(226, 226, 222));
    const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(150, 152, 150));
    const FLinearColor SymbolBlue = FLinearColor::FromSRGBColor(FColor(90, 150, 255));
    const FLinearColor OrderRed = FLinearColor::FromSRGBColor(FColor(122, 26, 28));
    const FLinearColor ActiveGreen = FLinearColor::FromSRGBColor(FColor(36, 132, 58));
    const FLinearColor ExecutingBlue = FLinearColor::FromSRGBColor(FColor(36, 82, 196));
    const FLinearColor ButtonDark = FLinearColor::FromSRGBColor(FColor(52, 32, 30));

    const TCHAR* EchelonMark(const AStrategyUnit* Unit)
    {
        switch (Unit->Echelon)
        {
            case EStrategyEchelon::Division: return TEXT("XX");
            case EStrategyEchelon::Brigade: return TEXT("X");
            case EStrategyEchelon::Regiment: return TEXT("III");
            case EStrategyEchelon::Battalion: return TEXT("II");
            default: return TEXT("I");
        }
    }

    bool IsCommandHQ(const AStrategyUnit* Unit)
    {
        return Unit->Echelon == EStrategyEchelon::Division || Unit->Echelon == EStrategyEchelon::Brigade ||
            Unit->Echelon == EStrategyEchelon::Regiment || Unit->Echelon == EStrategyEchelon::Battalion ||
            Unit->Echelon == EStrategyEchelon::Headquarters;
    }

    FString OrderLabel(const AStrategyUnit* Unit)
    {
        if (!Unit->OrderComponent)
        {
            return FString();
        }
        switch (Unit->OrderComponent->GetCurrentOrder().Type)
        {
            case EStrategyOrderType::None: return FString();
            case EStrategyOrderType::Move: return TEXT("MARCH");
            case EStrategyOrderType::AttackHere: return TEXT("ANGRIB");
            case EStrategyOrderType::DefendHere: return TEXT("FORSVAR");
            case EStrategyOrderType::Hold: return TEXT("HOLD");
            case EStrategyOrderType::Advance: return TEXT("RYK FREM");
            case EStrategyOrderType::Withdraw: return TEXT("TILBAGE");
            case EStrategyOrderType::Assemble: return TEXT("SAML");
            case EStrategyOrderType::ScoutHere: return TEXT("SPEJD");
            case EStrategyOrderType::Charge: return TEXT("CHARGE");
            default: return TEXT("SKYD");
        }
    }

    TArray<AStrategyUnit*> Subordinates(const AStrategyUnit* Unit);

    /** An HQ shows the men under it (as the Unity order of battle), a unit its own strength. */
    int32 MenUnder(const AStrategyUnit* Unit, int32 Guard = 0)
    {
        const TArray<AStrategyUnit*> Subs = Subordinates(Unit);
        if (Subs.Num() == 0 || Guard > 12)
        {
            return Unit->CurrentStrength;
        }
        int32 Men = 0;
        for (const AStrategyUnit* Sub : Subs) { Men += MenUnder(Sub, Guard + 1); }
        return Men;
    }

    TArray<AStrategyUnit*> Subordinates(const AStrategyUnit* Unit)
    {
        TArray<AStrategyUnit*> Out;
        if (Unit->CommandComponent)
        {
            for (AStrategyUnit* Sub : Unit->CommandComponent->CurrentSubordinates)
            {
                if (IsValid(Sub))
                {
                    Out.Add(Sub);
                }
            }
        }
        return Out;
    }
}

void AStrategyHUD::BeginSelectionBox(const FVector2D& ScreenPoint)
{
    bSelectionBoxActive = true;
    SelectionStart = ScreenPoint;
    SelectionEnd = ScreenPoint;
}

void AStrategyHUD::UpdateSelectionBox(const FVector2D& ScreenPoint)
{
    SelectionEnd = ScreenPoint;
}

void AStrategyHUD::EndSelectionBox()
{
    bSelectionBoxActive = false;
}

void AStrategyHUD::Text(const FString& S, float X, float Y, const FLinearColor& Colour, float Scale)
{
    DrawText(S, Colour, X, Y, nullptr, Scale, false);
}

void AStrategyHUD::DrawPanel(float X, float Y, float W, float H)
{
    DrawRect(PanelColour, X, Y, W, H);
    Panels.Add(FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
}

void AStrategyHUD::DrawButton(float X, float Y, float W, float H, const FString& Label, EAction Action, int32 Value, bool bActive,
    AStrategyUnit* Unit, const FLinearColor* Colour)
{
    DrawRect(Colour ? *Colour : bActive ? ActiveGreen : OrderRed, X, Y, W, H);
    float TW = 0.0f, TH = 0.0f;
    GetTextSize(Label, TW, TH, nullptr, 1.0f);
    Text(Label, X + (W - TW) * 0.5f, Y + (H - TH) * 0.5f, Ink);
    FButton B;
    B.Box = FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H));
    B.Action = Action;
    B.Value = Value;
    B.Unit = Unit;
    Buttons.Add(B);
}

void AStrategyHUD::DrawHUD()
{
    Super::DrawHUD();
    Buttons.Reset();
    Panels.Reset();

    if (bDrawQABuildMarker)
    {
        DrawRect(PanelColour, 6.0f, 6.0f, 330.0f, 22.0f);
        Text(BuildMarker, 12.0f, 9.0f, Ink);
    }
    DrawButton(342.0f, 6.0f, 130.0f, 22.0f, TEXT("INDSTILLINGER"), EAction::SettingsToggle, 0, bSettingsOpen, nullptr, bSettingsOpen ? nullptr : &ButtonDark);

    DrawOOB();
    DrawMinimap();
    DrawSettings();
    // The battle from the campaign: its end and the way back.
    for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
    {
        if (It->IsCampaignBattle())
        {
            const float BW = 300.0f, BX = Canvas->ClipX - BW - 8.0f;
            DrawPanel(BX - 6.0f, 40.0f, BW + 12.0f, 44.0f);
            DrawButton(BX, 46.0f, BW, 32.0f, TEXT("AFSLUT SLAGET  →  KAMPAGNEN"), EAction::FinishBattle, 0, false, nullptr, &ExecutingBlue);
            break;
        }
    }

    if (const AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetOwningPlayerController()))
    {
        const TArray<AStrategyUnit*> Selected = PC->GetSelectedUnits();
        if (Selected.Num() > 0 && IsValid(Selected[0]))
        {
            DrawCommandPanel(Selected[0]);
        }
    }

    if (!bSelectionBoxActive)
    {
        return;
    }
    const float Left = FMath::Min(SelectionStart.X, SelectionEnd.X);
    const float Top = FMath::Min(SelectionStart.Y, SelectionEnd.Y);
    const float Right = FMath::Max(SelectionStart.X, SelectionEnd.X);
    const float Bottom = FMath::Max(SelectionStart.Y, SelectionEnd.Y);
    DrawRect(SelectionFillColor, Left, Top, Right - Left, Bottom - Top);
    DrawLine(Left, Top, Right, Top, SelectionBorderColor, 1.5f);
    DrawLine(Right, Top, Right, Bottom, SelectionBorderColor, 1.5f);
    DrawLine(Right, Bottom, Left, Bottom, SelectionBorderColor, 1.5f);
    DrawLine(Left, Bottom, Left, Top, SelectionBorderColor, 1.5f);
}

// ------------------------------------------------------------------ order of battle

void AStrategyHUD::DrawOOB()
{
    const float X = 8.0f, W = 560.0f;
    float Y = 34.0f;
    // The player's side: the tops of the command trees (no current parent).
    TArray<AStrategyUnit*> Roots;
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Unit = *It;
        if (IsValid(Unit) && Unit->Side == EStrategySide::Denmark &&
            (!Unit->CommandComponent || !IsValid(Unit->CommandComponent->CurrentCommandParent)))
        {
            Roots.Add(Unit);
        }
    }

    // First pass to measure the height: draw the header, then the rows.
    const float HeaderH = 30.0f;
    DrawRect(PanelColour, X, Y, W, HeaderH);
    Panels.Add(FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + HeaderH)));
    Text(TEXT("ORDER OF BATTLE   [O]"), X + 10.0f, Y + 8.0f, Gold);
    DrawButton(X + W - 34.0f, Y + 5.0f, 26.0f, 20.0f, bOOBOpen ? TEXT("-") : TEXT("+"), EAction::OOBToggle, 0, false, nullptr, &ButtonDark);
    Y += HeaderH;
    if (!bOOBOpen)
    {
        return;
    }
    const float Top = Y;
    DrawRect(PanelColour, X, Y, W, 24.0f);
    Text(TEXT("MÆND"), X + 350.0f, Y + 6.0f, Muted, 0.85f);
    Text(TEXT("STATUS"), X + 405.0f, Y + 6.0f, Muted, 0.85f);
    Text(TEXT("AI"), X + 478.0f, Y + 6.0f, Muted, 0.85f);
    Text(TEXT("TILK"), X + 515.0f, Y + 6.0f, Muted, 0.85f);
    Y += 24.0f;
    for (AStrategyUnit* Root : Roots)
    {
        DrawOOBRow(Root, 0, Y, 0);
    }
    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), X, Top, W, Y - Top);
    Panels.Add(FBox2D(FVector2D(X, Top), FVector2D(X + W, Y + 6.0f)));
}

void AStrategyHUD::DrawOOBRow(AStrategyUnit* Unit, int32 Depth, float& Y, int32 Guard)
{
    if (!IsValid(Unit) || Guard > 12 || Y > Canvas->ClipY - 190.0f)
    {
        return;
    }
    const float X = 8.0f, W = 560.0f, H = 23.0f;
    const bool bSel = Unit->bSelected;
    DrawRect(bSel ? FLinearColor(0.20f, 0.16f, 0.05f, 0.95f) : RowColour, X + 4.0f, Y, W - 8.0f, H - 2.0f);
    FButton Row;
    Row.Box = FBox2D(FVector2D(X + 4.0f, Y), FVector2D(X + W - 4.0f, Y + H - 2.0f));
    Row.Action = EAction::OOBRow;
    Row.Unit = Unit;
    Buttons.Add(Row);

    const TArray<AStrategyUnit*> Subs = Subordinates(Unit);
    const float Indent = X + 14.0f + Depth * 22.0f;
    if (Subs.Num() > 0)
    {
        DrawButton(Indent - 4.0f, Y + 3.0f, 16.0f, 16.0f, Folded.Contains(Unit) ? TEXT("+") : TEXT("-"), EAction::OOBFold, 0, false, Unit, &ButtonDark);
    }
    const bool bAttached = Unit->CommandComponent && Unit->CommandComponent->CurrentCommandParent != Unit->CommandComponent->OrganicParent;
    Text(EchelonMark(Unit), Indent + 18.0f, Y + 4.0f, SymbolBlue);
    const FString Upper = Unit->DisplayName.ToString().ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å"));
    Text(FString::Printf(TEXT("%s%s"), bAttached ? TEXT("↳ ") : TEXT(""), *Upper), Indent + 46.0f, Y + 4.0f, bSel ? Gold : Ink);
    Text(FString::FromInt(MenUnder(Unit)), X + 350.0f, Y + 4.0f, Ink, 0.9f);
    Text(OrderLabel(Unit), X + 405.0f, Y + 4.0f, Muted, 0.9f);
    Text(Unit->bOfficerAIEnabled ? TEXT("ON") : TEXT("OFF"), X + 478.0f, Y + 4.0f, Unit->bOfficerAIEnabled ? ActiveGreen : Muted, 0.9f);
    if (bAttached)
    {
        Text(TEXT("ATT"), X + 515.0f, Y + 4.0f, Muted, 0.9f);
    }
    Y += H;
    if (!Folded.Contains(Unit))
    {
        for (AStrategyUnit* Sub : Subs)
        {
            DrawOOBRow(Sub, Depth + 1, Y, Guard + 1);
        }
    }
}

// ------------------------------------------------------------------ settings

void AStrategyHUD::DrawSettings()
{
    if (!bSettingsOpen)
    {
        return;
    }
    // A small window under the button: the camera's speed on the keys.
    const float X = 342.0f, Y = 32.0f, W = 420.0f, H = 96.0f;
    DrawPanel(X, Y, W, H);
    Text(TEXT("INDSTILLINGER"), X + 12.0f, Y + 8.0f, Gold);
    const float Factor = AStrategyCameraPawn::GetKeySpeedFactor();
    Text(TEXT("Kamerafart på tasterne (WASD)"), X + 12.0f, Y + 38.0f, Ink);
    static const float Steps[] = { 1.0f, 2.0f, 3.0f, 5.0f, 8.0f, 10.0f, 15.0f, 20.0f };
    int32 At = 0;
    for (int32 i = 0; i < UE_ARRAY_COUNT(Steps); ++i) { if (Steps[i] <= Factor + 0.01f) { At = i; } }
    DrawButton(X + 240.0f, Y + 34.0f, 30.0f, 24.0f, TEXT("-"), EAction::CameraSpeed, FMath::Max(0, At - 1), false, nullptr, &ButtonDark);
    Text(FString::Printf(TEXT("x %g"), Factor), X + 282.0f, Y + 38.0f, Gold);
    DrawButton(X + 330.0f, Y + 34.0f, 30.0f, 24.0f, TEXT("+"), EAction::CameraSpeed, FMath::Min(int32(UE_ARRAY_COUNT(Steps)) - 1, At + 1), false, nullptr, &ButtonDark);
    Text(TEXT("Shift giver tre gange så hurtigt. Gemmes til næste gang."), X + 12.0f, Y + 68.0f, Muted, 0.85f);
}

// ------------------------------------------------------------------ minimap

void AStrategyHUD::DrawMinimap()
{
    // The battlefield from above, bottom right above the command panel: every unit, the camera's place.
    const float W = 300.0f, H = 200.0f;
    const float X = Canvas->ClipX - W - 8.0f, Y = Canvas->ClipY - 128.0f - H - 36.0f;
    DrawPanel(X, Y, W, H + 28.0f);
    Text(TEXT("TAKTISK KORT / KAMERA  [M]"), X + 10.0f, Y + 6.0f, Gold, 0.9f);
    const float MX = X + 8.0f, MY = Y + 26.0f, MW = W - 16.0f, MH = H - 6.0f;
    DrawRect(FLinearColor::FromSRGBColor(FColor(52, 78, 44, 240)), MX, MY, MW, MH);
    // What it shows: all units with a margin, at least 600 m square, kept to the frame's shape.
    FBox Bounds(ForceInit);
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It)) { Bounds += It->GetActorLocation(); }
    }
    if (!Bounds.IsValid) { Bounds = FBox(FVector(-30000.0f), FVector(30000.0f)); }
    FVector Centre = Bounds.GetCenter();
    const float Half = FMath::Max3(30000.0f, float(Bounds.GetExtent().X) + 15000.0f, (float(Bounds.GetExtent().Y) + 15000.0f) * MW / MH);
    MinimapCentre = FVector2D(Centre.X, Centre.Y);
    MinimapHalfWidth = Half;
    MinimapRect = FBox2D(FVector2D(MX, MY), FVector2D(MX + MW, MY + MH));
    const float Scale = MW / (2.0f * Half);
    // World +X is north (up the map), +Y east (right).
    auto ToMap = [&](const FVector& P) { return FVector2D(MX + MW * 0.5f + (P.Y - Centre.Y) * Scale, MY + MH * 0.5f - (P.X - Centre.X) * Scale); };
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        const AStrategyUnit* Unit = *It;
        if (!IsValid(Unit) || Unit->CurrentStrength <= 0)
        {
            continue;
        }
        const FVector2D P = ToMap(Unit->GetActorLocation());
        if (!MinimapRect.IsInside(P)) { continue; }
        const float S = IsCommandHQ(Unit) ? 5.0f : 4.0f;
        const FLinearColor C = Unit->bSelected ? Gold : Unit->Side == EStrategySide::Denmark ? FLinearColor::FromSRGBColor(FColor(70, 220, 255)) : FLinearColor::FromSRGBColor(FColor(235, 50, 40));
        DrawRect(C, P.X - S, P.Y - S * 0.6f, S * 2.0f, S * 1.2f);
    }
    if (const APlayerController* PC = GetOwningPlayerController())
    {
        if (const APawn* Pawn = PC->GetPawn())
        {
            const FVector2D P = ToMap(Pawn->GetActorLocation());
            const float S = 14.0f;
            DrawLine(P.X - S, P.Y - S * 0.6f, P.X + S, P.Y - S * 0.6f, FLinearColor::White, 1.5f);
            DrawLine(P.X + S, P.Y - S * 0.6f, P.X + S, P.Y + S * 0.6f, FLinearColor::White, 1.5f);
            DrawLine(P.X + S, P.Y + S * 0.6f, P.X - S, P.Y + S * 0.6f, FLinearColor::White, 1.5f);
            DrawLine(P.X - S, P.Y + S * 0.6f, P.X - S, P.Y - S * 0.6f, FLinearColor::White, 1.5f);
        }
    }
    FButton B;
    B.Box = MinimapRect;
    B.Action = EAction::Minimap;
    Buttons.Insert(B, 0);   // under everything else
}

// ------------------------------------------------------------------ command panel

void AStrategyHUD::DrawCommandPanel(AStrategyUnit* Unit)
{
    const float H = 128.0f;
    const float Y = Canvas->ClipY - H;
    const float W = Canvas->ClipX;
    DrawPanel(0.0f, Y, W, H);
    const bool bHQ = IsCommandHQ(Unit);
    const TCHAR* RoleText = Unit->Echelon == EStrategyEchelon::Division ? TEXT("DIVISIONSCHEF | HØJERE KOMMANDO")
        : Unit->Echelon == EStrategyEchelon::Brigade ? TEXT("BRIGADECHEF | HØJERE KOMMANDO")
        : Unit->Echelon == EStrategyEchelon::Regiment ? TEXT("OBERSTLØJTNANT | REGIMENTSKOMMANDO")
        : Unit->Echelon == EStrategyEchelon::Battalion ? TEXT("MAJOR | BATAILLONSKOMMANDO")
        : Unit->Echelon == EStrategyEchelon::Cavalry ? TEXT("RITMESTER | KAVALERIKOMMANDO")
        : Unit->Echelon == EStrategyEchelon::Artillery ? TEXT("KAPTAJN | BATTERIKOMMANDO")
        : TEXT("KAPTAJN | KOMPAGNIKOMMANDO");
    Text(FString::Printf(TEXT("%s | %s"), *Unit->DisplayName.ToString().ToUpper(), RoleText), 10.0f, Y + 6.0f, Gold);

    // Left: the state and the AI.
    const float LX = 10.0f, LY = Y + 30.0f;
    Text(TEXT("STANDSINFO / AI"), LX, LY, Gold, 0.85f);
    Text(FString::Printf(TEXT("Styrke %d/%d  ·  Moral %.0f  ·  Samhold %.0f"), Unit->CurrentStrength, Unit->InitialStrength, Unit->Morale, Unit->Cohesion), LX, LY + 18.0f, Ink, 0.9f);
    DrawButton(LX, LY + 44.0f, 66.0f, 24.0f, Unit->bOfficerAIEnabled ? TEXT("AI ON") : TEXT("AI OFF"), EAction::AIToggle, 0, Unit->bOfficerAIEnabled, Unit);
    if (Unit->DoctrineComponent)
    {
        const EStrategyDoctrine D = Unit->DoctrineComponent->Doctrine;
        DrawButton(LX + 72.0f, LY + 44.0f, 60.0f, 24.0f, TEXT("DEF"), EAction::Doctrine, int32(EStrategyDoctrine::Defensive), D == EStrategyDoctrine::Defensive, Unit);
        DrawButton(LX + 138.0f, LY + 44.0f, 60.0f, 24.0f, TEXT("BAL"), EAction::Doctrine, int32(EStrategyDoctrine::Balanced), D == EStrategyDoctrine::Balanced, Unit);
        DrawButton(LX + 204.0f, LY + 44.0f, 60.0f, 24.0f, TEXT("OFF"), EAction::Doctrine, int32(EStrategyDoctrine::Offensive), D == EStrategyDoctrine::Offensive, Unit);
    }

    // Middle: the orders (blue while it is being carried out).
    const float MX = 300.0f, MY = Y + 30.0f;
    const EStrategyOrderType Current = Unit->OrderComponent ? Unit->OrderComponent->GetCurrentOrder().Type : EStrategyOrderType::None;
    const bool bExecuting = Unit->OrderComponent && Unit->OrderComponent->GetCommandVisualState() == EStrategyCommandVisualState::Blue;
    auto OrderButton = [&](float X, float BY, const TCHAR* Label, EStrategyOrderType Type)
    {
        const FLinearColor* C = Current == Type && bExecuting ? &ExecutingBlue : nullptr;
        DrawButton(X, BY, 176.0f, 26.0f, Label, EAction::Order, int32(Type), false, Unit, C);
    };
    if (bHQ)
    {
        Text(TEXT("ORDRER — KLIK = POSITION, TRÆK = FRONT"), MX, MY, Gold, 0.85f);
        OrderButton(MX, MY + 18.0f, TEXT("ANGRIB HER"), EStrategyOrderType::AttackHere);
        OrderButton(MX + 182.0f, MY + 18.0f, TEXT("FORSVAR HER"), EStrategyOrderType::DefendHere);
        OrderButton(MX + 364.0f, MY + 18.0f, TEXT("RYK FREM"), EStrategyOrderType::Advance);
        OrderButton(MX, MY + 50.0f, TEXT("TILBAGETRÆK"), EStrategyOrderType::Withdraw);
        OrderButton(MX + 182.0f, MY + 50.0f, TEXT("SAML"), EStrategyOrderType::Assemble);
        DrawButton(MX + 364.0f, MY + 50.0f, 176.0f, 26.0f, TEXT("STOP / HOLD"), EAction::Stop, 0, false, Unit,
            Current == EStrategyOrderType::Hold && bExecuting ? &ExecutingBlue : nullptr);
    }
    else
    {
        // A company: fire policy, the order buttons and the formation.
        Text(TEXT("SKYDNING"), MX, MY, Gold, 0.85f);
        if (UStrategyFireControlComponent* Fire = Unit->FireControlComponent)
        {
            const TCHAR* Labels[] = { TEXT("HOLD"), TEXT("CLOSE"), TEXT("MED"), TEXT("LONG") };
            for (int32 i = 0; i < 4; ++i)
            {
                DrawButton(MX + i * 64.0f, MY + 18.0f, 60.0f, 24.0f, Labels[i], EAction::FirePolicy, i, int32(Fire->FirePolicy) == i, Unit);
            }
            Text(FString::Printf(TEXT("%.0f m / %.0f m / %.0f m  ·  kegle ±%.0f°"), Fire->CloseRangeCm / 100.0f, Fire->MediumRangeCm / 100.0f, Fire->LongRangeCm / 100.0f,
                Fire->FireConeHalfAngleDegrees), MX, MY + 48.0f, Muted, 0.85f);
        }
        const float OX = MX + 280.0f;
        Text(TEXT("ORDRER / BEVÆGELSE"), OX, MY, Gold, 0.85f);
        DrawButton(OX, MY + 18.0f, 110.0f, 24.0f, TEXT("RYK FREM"), EAction::Order, int32(EStrategyOrderType::Advance), Current == EStrategyOrderType::Advance, Unit);
        DrawButton(OX + 116.0f, MY + 18.0f, 110.0f, 24.0f, TEXT("TILBAGE"), EAction::Order, int32(EStrategyOrderType::Withdraw), Current == EStrategyOrderType::Withdraw, Unit);
        DrawButton(OX + 232.0f, MY + 18.0f, 110.0f, 24.0f, TEXT("CHARGE"), EAction::Charge, 0, Current == EStrategyOrderType::Charge, Unit);
        DrawButton(OX + 348.0f, MY + 18.0f, 80.0f, 24.0f, TEXT("STOP"), EAction::Stop, 0, Current == EStrategyOrderType::Hold, Unit);
        if (UStrategyFormationComponent* Formation = Unit->FormationComponent)
        {
            Text(TEXT("FORMATION"), OX, MY + 50.0f, Gold, 0.85f);
            const TCHAR* Labels[] = { TEXT("LINIE"), TEXT("KOLONNE"), TEXT("KARRÉ") };
            const EStrategyFormationType Types[] = { EStrategyFormationType::Line, EStrategyFormationType::MarchColumn, EStrategyFormationType::Square };
            for (int32 i = 0; i < 3; ++i)
            {
                DrawButton(OX + i * 116.0f, MY + 66.0f, 110.0f, 24.0f, Labels[i], EAction::Formation, int32(Types[i]), Formation->CurrentFormation == Types[i], Unit);
            }
        }
    }

    // Right: the subordinates, their status and AI.
    const float RX = FMath::Max(860.0f, W - 520.0f), RY = Y + 30.0f;
    const TArray<AStrategyUnit*> Subs = Subordinates(Unit);
    if (Subs.Num() > 0)
    {
        Text(TEXT("UNDERLAGTE — STATUS / AI / TILKNYTNING"), RX, RY, Gold, 0.85f);
        for (int32 i = 0; i < Subs.Num() && i < 5; ++i)
        {
            const AStrategyUnit* Sub = Subs[i];
            const bool bAttached = Sub->CommandComponent && Sub->CommandComponent->CurrentCommandParent != Sub->CommandComponent->OrganicParent;
            Text(FString::Printf(TEXT("%s  ·  %d  ·  %s  ·  AI %s%s"), *Sub->DisplayName.ToString(), Sub->CurrentStrength, *OrderLabel(Sub),
                Sub->bOfficerAIEnabled ? TEXT("ON") : TEXT("OFF"), bAttached ? TEXT("  ·  ATT") : TEXT("")), RX, RY + 18.0f + i * 16.0f, Ink, 0.85f);
        }
    }
    else if (const UStrategyFireControlComponent* Fire = Unit->FireControlComponent)
    {
        Text(TEXT("SKYDEAFSTAND"), RX, RY, Gold, 0.85f);
        Text(FString::Printf(TEXT("Aktiv: %s"), *Fire->GetActiveRangeLabel()), RX, RY + 18.0f, Ink, 0.9f);
    }
}

// ------------------------------------------------------------------ clicks

bool AStrategyHUD::IsOverPanel(const FVector2D& P) const
{
    for (const FBox2D& Box : Panels)
    {
        if (Box.IsInside(P))
        {
            return true;
        }
    }
    for (const FButton& B : Buttons)
    {
        if (B.Box.IsInside(P))
        {
            return true;
        }
    }
    return false;
}

bool AStrategyHUD::HandleClick(const FVector2D& P)
{
    AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetOwningPlayerController());
    // The last button drawn lies on top.
    for (int32 i = Buttons.Num() - 1; i >= 0; --i)
    {
        const FButton& B = Buttons[i];
        if (!B.Box.IsInside(P))
        {
            continue;
        }
        AStrategyUnit* Unit = B.Unit.Get();
        switch (B.Action)
        {
            case EAction::Minimap:
                if (PC)
                {
                    // The camera to the clicked point (world +X up the map, +Y right).
                    const FVector2D C = MinimapRect.GetCenter();
                    const float Scale = (2.0f * MinimapHalfWidth) / MinimapRect.GetSize().X;
                    const FVector World(MinimapCentre.X - (P.Y - C.Y) * Scale, MinimapCentre.Y + (P.X - C.X) * Scale, 0.0f);
                    if (AStrategyCameraPawn* Camera = Cast<AStrategyCameraPawn>(PC->GetPawn()))
                    {
                        Camera->FocusOnWorldLocation(World);
                    }
                }
                break;
            case EAction::SettingsToggle:
                bSettingsOpen = !bSettingsOpen;
                break;
            case EAction::CameraSpeed:
            {
                static const float Steps[] = { 1.0f, 2.0f, 3.0f, 5.0f, 8.0f, 10.0f, 15.0f, 20.0f };
                AStrategyCameraPawn::SetKeySpeedFactor(Steps[FMath::Clamp(B.Value, 0, int32(UE_ARRAY_COUNT(Steps)) - 1)]);
                break;
            }
            case EAction::FinishBattle:
                for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
                {
                    It->FinishCampaignBattle();
                    break;
                }
                break;
            case EAction::OOBToggle:
                bOOBOpen = !bOOBOpen;
                break;
            case EAction::OOBFold:
                if (Unit)
                {
                    if (Folded.Contains(Unit)) { Folded.Remove(Unit); } else { Folded.Add(Unit); }
                }
                break;
            case EAction::OOBRow:
                if (Unit && PC)
                {
                    // A double click puts the camera on it.
                    const double Now = FPlatformTime::Seconds();
                    const bool bDouble = LastClickedUnit.Get() == Unit && Now - LastClickTime < 0.4;
                    PC->SelectUnitFromOOB(Unit, bDouble);
                    LastClickedUnit = Unit;
                    LastClickTime = Now;
                }
                break;
            case EAction::AIToggle:
                if (Unit) { Unit->bOfficerAIEnabled = !Unit->bOfficerAIEnabled; }
                break;
            case EAction::Doctrine:
                if (Unit && Unit->DoctrineComponent) { Unit->DoctrineComponent->Doctrine = EStrategyDoctrine(B.Value); }
                break;
            case EAction::Order:
                if (PC) { PC->BeginOrderPlacement(EStrategyOrderType(B.Value)); }
                break;
            case EAction::Charge:
                if (PC) { PC->BeginOrderPlacement(EStrategyOrderType::Charge); }
                break;
            case EAction::Stop:
                if (PC) { PC->IssueHoldToSelection(); }
                break;
            case EAction::FirePolicy:
                if (Unit && Unit->FireControlComponent) { Unit->FireControlComponent->SetFirePolicy(EStrategyFirePolicy(B.Value)); }
                break;
            case EAction::Formation:
                if (Unit && Unit->FormationComponent) { Unit->FormationComponent->SetFormation(EStrategyFormationType(B.Value)); }
                break;
            default:
                break;
        }
        return true;
    }
    return IsOverPanel(P);
}
