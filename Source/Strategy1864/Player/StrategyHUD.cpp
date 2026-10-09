#include "StrategyHUD.h"
#include "../AI/StrategyOfficerProfileComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../AI/StrategyFieldOfficerComponent.h"
#include "../Combat/StrategyContactComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyFireDisciplineComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Formations/StrategyFormationTransitionComponent.h"
#include "../Combat/StrategyThreatReactionComponent.h"
#include "../Combat/StrategyVisibilityComponent.h"
#include "../Campaign/StrategyCampaignBattlefield.h"
#include "StrategyBattleQuality.h"
#include "../AI/StrategyAITelemetryComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"

#include "StrategyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "../AI/StrategyDoctrineComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyFireDrillComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Formations/StrategyFormationPolicyComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "../Units/StrategyUnit.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "TextureResource.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "../Units/StrategyCompanyUnit.h"
#include "../Visual/StrategyInfantryVisualComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyDragoonComponent.h"
#include "../Combat/StrategyStanceComponent.h"
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

    FString BaseOrderLabel(const AStrategyUnit* Unit)
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
            case EStrategyOrderType::Disengage: return TEXT("AFBRYDER");
            case EStrategyOrderType::Withdraw: return TEXT("TILBAGE");
            case EStrategyOrderType::Assemble: return TEXT("SAML");
            case EStrategyOrderType::ScoutHere: return TEXT("SPEJD");
            case EStrategyOrderType::Charge: return TEXT("CHARGE");
            default: return TEXT("SKYD");
        }
    }

    FString OrderLabel(const AStrategyUnit* Unit)
    {
        FString RouteLabel = BaseOrderLabel(Unit);
        if (Unit->OrderComponent)
        {
            const AStrategyPlayerController* LabelPC = Cast<AStrategyPlayerController>(Unit->GetWorld()->GetFirstPlayerController());
            const FStrategyOrder LabelOrder = LabelPC ? LabelPC->GetRequestedRoute(Unit) : Unit->OrderComponent->GetLatestRequestedOrder();
            const int32 RemainingWaypoints = FMath::Max(0, LabelOrder.Waypoints.Num() - LabelOrder.NextWaypointIndex);
            if (RemainingWaypoints > 0) RouteLabel += FString::Printf(TEXT(" (%d vejpunkter)"), RemainingWaypoints);
        }
        return RouteLabel;
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
    if (bCommandStyle)
    {
        // Pills: dark with a grey outline; the chosen one outlined and softly filled in green; coloured buttons tinted by their colour.
        const FLinearColor Bright(0.30f, 0.88f, 0.50f, 1.f);
        if (bActive && !Colour) { DrawRounded(X, Y, W, H, FLinearColor(0.04f, 0.24f, 0.13f, 1.f), &Bright); }
        else if (Colour)
        {
            const FLinearColor Tint(Colour->R * 0.38f, Colour->G * 0.38f, Colour->B * 0.38f, 1.f);
            const FLinearColor Edge(FMath::Min(1.f, Colour->R * 1.5f + 0.1f), FMath::Min(1.f, Colour->G * 1.5f + 0.1f), FMath::Min(1.f, Colour->B * 1.5f + 0.1f), 0.95f);
            DrawRounded(X, Y, W, H, bActive ? FLinearColor(Colour->R * 0.6f, Colour->G * 0.6f, Colour->B * 0.6f, 1.f) : Tint, &Edge, H > 30.f ? 9.f : -1.f);
        }
        else { DrawRounded(X, Y, W, H, FLinearColor(0.030f, 0.045f, 0.070f, 1.f)); }
    }
    else { DrawRect(Colour ? *Colour : bActive ? ActiveGreen : OrderRed, X, Y, W, H); }
    float TW = 0.0f, TH = 0.0f;
    GetTextSize(Label, TW, TH, nullptr, 1.0f);
    const float ButtonScale = bCommandStyle ? FMath::Min(0.95f, (W - 8.f) / FMath::Max(1.f, TW)) : 1.f;
    Text(Label, X + (W - TW * ButtonScale) * 0.5f, Y + (H - TH * ButtonScale) * 0.5f, bCommandStyle && Action == EAction::None ? Muted : Ink, ButtonScale);
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
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864NoHud")))
    {
        return;   // clean screenshots (the start menu's background)
    }
    if (FigureDivisor < 0)
    {
        FigureDivisor = Strategy1864BattleQuality::GetFigureDivisor();
        BattleQualityPreset = Strategy1864BattleQuality::GetPreset();
    }
    {
        static double LastShadowCheck = 0.0;
        const double Now = FPlatformTime::Seconds();
        if (Now - LastShadowCheck > 2.0)
        {
            LastShadowCheck = Now;
            Strategy1864BattleQuality::ApplyShadows(GetWorld());
        }
    }
    Buttons.Reset();
    Panels.Reset();

    if (bDrawQABuildMarker)
    {
        DrawRect(PanelColour, 6.0f, 6.0f, 330.0f, 22.0f);
        Text(BuildMarker, 12.0f, 9.0f, Ink);
    }
    DrawButton(342.0f, 6.0f, 130.0f, 22.0f, TEXT("INDSTILLINGER"), EAction::SettingsToggle, 0, bSettingsOpen, nullptr, bSettingsOpen ? nullptr : &ButtonDark);

    if (const AStrategyPlayerController* RoutePC = Cast<AStrategyPlayerController>(GetOwningPlayerController()))
    {
        const TArray<AStrategyUnit*> RouteSelection = RoutePC->GetSelectedUnits();
        for (TActorIterator<AStrategyUnit> RouteIt(GetWorld()); RouteIt; ++RouteIt)
            if (RouteIt->bPlayerControllable) DrawMovementRoute(*RouteIt, RouteSelection.Contains(*RouteIt));
    }

    // Fire cones under the panels: the selected units', and the duel's two companies.
    {
        TSet<const AStrategyUnit*> Drawn;
        bool bLegend = true;
        if (const AStrategyPlayerController* ConePC = Cast<AStrategyPlayerController>(GetOwningPlayerController()))
        {
            for (const AStrategyUnit* Unit : ConePC->GetSelectedUnits())
            {
                if (IsValid(Unit) && Unit->FireControlComponent && !IsCommandHQ(Unit) && !Drawn.Contains(Unit) && Unit->Side == EStrategySide::Denmark)
                {
                    DrawFireCone(Unit, bLegend);
                    bLegend = false;
                    Drawn.Add(Unit);
                }
            }
        }
        for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
        {
            for (const AStrategyCompanyUnit* Company : It->GetDuelCompanies())
            {
                if (IsValid(Company) && Company->IsCombatEffective() && !Drawn.Contains(Company) && (Company->Side == EStrategySide::Denmark || ShowEnemyRange()))
                {
                    DrawFireCone(Company, bLegend);
                    bLegend = false;
                    Drawn.Add(Company);
                }
            }
        }
        // The enemy's reach: hidden in a real battle (the commander does not see it), shown for testing.
        if (ShowEnemyRange())
        {
            for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
            {
                const AStrategyUnit* Unit = *It;
                if (IsValid(Unit) && Unit->Side != EStrategySide::Denmark && Unit->Side != EStrategySide::Neutral && Unit->FireControlComponent &&
                    !IsCommandHQ(Unit) && Unit->IsCombatEffective() && !Drawn.Contains(Unit) && Unit->Echelon == EStrategyEchelon::Company)
                {
                    DrawFireCone(Unit, false);
                    Drawn.Add(Unit);
                }
            }
        }
    }

    DrawObjectiveMarkers();
    DrawNotices();
    DrawOOB();
    DrawMinimap();
    DrawSettings();
    // The battle's standing (the men each side has fighting) and, once decided, the outcome.
    for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
    {
        if (!It->IsSkirmish() && !It->IsCampaignBattle())
        {
            break;
        }
        int32 DS, DN, ES, EN, DB, EB;
        It->GetBattleScore(DS, DN, ES, EN, DB, EB);
        int32 DanishPoints = 0, EnemyPoints = 0;
        It->GetObjectivePoints(DanishPoints, EnemyPoints);
        const bool bObjectives = It->GetObjectives().Num() > 0;
        const float BW = bObjectives ? 700.0f : 420.0f, BX = (Canvas->ClipX - BW) * 0.5f;
        DrawPanel(BX, 6.0f, BW, 30.0f);
        // Time: pause and the speeds (keys: pause, 1-3), always within reach on the battlefield.
        {
            const AStrategyPlayerController* TimePC = Cast<AStrategyPlayerController>(GetOwningPlayerController());
            const bool bPaused = UGameplayStatics::IsGamePaused(this);
            const float Speed = TimePC ? TimePC->SimulationSpeed : 1.0f;
            const float TX = BX + BW + 10.0f;
            DrawButton(TX, 8.0f, 74.0f, 26.0f, bPaused ? TEXT("FORTSÆT") : TEXT("PAUSE"), EAction::TimeControl, 0, bPaused);
            const int32 Speeds[] = { -1, 1, 2, 3, 5, 10 };   // -1 is half speed
            for (int32 i = 0; i < 6; ++i)
            {
                const float Value = Speeds[i] < 0 ? 0.5f : float(Speeds[i]);
                DrawButton(TX + 82.0f + i * 42.0f, 8.0f, 38.0f, 26.0f, Speeds[i] < 0 ? FString(TEXT("x½")) : FString::Printf(TEXT("x%d"), Speeds[i]), EAction::TimeControl, Speeds[i], !bPaused && FMath::IsNearlyEqual(Speed, Value));
            }
        }
        Text(FString::Printf(TEXT("DANSKE  %d / %d   ·   FJENDEN  %d / %d"), DN, DS, EN, ES), BX + 14.0f, 13.0f, Gold);
        if (bObjectives)
        {
            const int32 Clock = int32(It->GetBattleClock()), Day = FMath::Min(3, Clock / 86400 + 1);
            const float Hour = It->GetBattleHour();
            Text(FString::Printf(TEXT("MÅL  %d : %d   ·   Dag %d af 3  ·  kl. %02d:%02d"), DanishPoints, EnemyPoints, Day, int32(Hour), int32((Hour - int32(Hour)) * 60.0f)), BX + 440.0f, 13.0f, Gold);
        }
        const FString& Outcome = It->GetBattleOutcome();
        if (!Outcome.IsEmpty())
        {
            float TW = 0.0f, TH = 0.0f;
            GetTextSize(Outcome, TW, TH, nullptr, 1.6f);
            const float W = TW + 60.0f, X = (Canvas->ClipX - W) * 0.5f, Y = Canvas->ClipY * 0.28f;
            DrawRect(It->IsDanishVictory() ? FLinearColor(0.10f, 0.22f, 0.10f, 0.88f) : FLinearColor(0.28f, 0.08f, 0.06f, 0.88f), X, Y, W, TH + 30.0f);
            DrawText(Outcome, FLinearColor(0.98f, 0.92f, 0.70f), X + 30.0f, Y + 15.0f, nullptr, 1.6f, false);
        }
        break;
    }
    // The way out of the battle: back to the campaign for a campaign battle, to the start menu for a test battle.
    {
        bool bCampaignBattle = false;
        for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
        {
            bCampaignBattle = It->IsCampaignBattle();
            break;
        }
        const float BW = 300.0f, BX = Canvas->ClipX - BW - 8.0f;
        DrawPanel(BX - 6.0f, 40.0f, BW + 12.0f, 44.0f);
        DrawButton(BX, 46.0f, BW, 32.0f, bCampaignBattle ? TEXT("AFSLUT SLAGET  →  KAMPAGNEN") : TEXT("FORLAD SLAGET  →  STARTMENU"), EAction::FinishBattle, 0, false, nullptr, &ExecutingBlue);
    }

    if (const AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetOwningPlayerController()))
    {
        const TArray<AStrategyUnit*> Selected = PC->GetSelectedUnits();
        DrawCommandPanel(Selected.Num() > 0 && IsValid(Selected[0]) ? Selected[0] : nullptr);
    }

    DrawUnitHover();

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
    if (!IsValid(Unit) || Guard > 12 || Y > Canvas->ClipY - CommandHeight() - 38.0f)
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

// ------------------------------------------------------------------ fire cone

void AStrategyHUD::DashedPolyline(const TArray<FVector>& WorldPoints, const FLinearColor& Colour, float Thickness, float Dash, float Gap)
{
    // In screen space, so the dashes keep their size at every zoom.
    float Phase = 0.0f;
    for (int32 i = 0; i + 1 < WorldPoints.Num(); ++i)
    {
        const FVector A3 = Project(WorldPoints[i], false), B3 = Project(WorldPoints[i + 1], false);
        if (A3.Z <= 0.0f || B3.Z <= 0.0f)
        {
            continue;   // behind the camera
        }
        const FVector2D A(A3.X, A3.Y), B(B3.X, B3.Y);
        const float Len = FVector2D::Distance(A, B);
        // A point just in front of the camera projects far off the screen: such a segment is skipped (otherwise
        // millions of dashes, and the memory runs out).
        const float Far = 4.0f * FMath::Max(Canvas->ClipX, Canvas->ClipY);
        if (Len > Far || FMath::Abs(A.X) > Far || FMath::Abs(A.Y) > Far || FMath::Abs(B.X) > Far || FMath::Abs(B.Y) > Far)
        {
            Phase += FMath::Min(Len, Far);
            continue;
        }
        float T = 0.0f;
        int32 Guard = 0;
        while (T < Len && ++Guard < 2000)
        {
            const float Period = Dash + Gap;
            const float InPeriod = FMath::Fmod(Phase + T, Period);
            if (InPeriod < Dash)
            {
                const float Step = FMath::Min(Dash - InPeriod, Len - T);
                const FVector2D P = A + (B - A) * (T / Len), Q = A + (B - A) * ((T + Step) / Len);
                DrawLine(P.X, P.Y, Q.X, Q.Y, Colour, Thickness);
                T += Step;
            }
            else
            {
                T += FMath::Min(Period - InPeriod, Len - T);
            }
        }
        Phase += Len;
    }
}

void AStrategyHUD::DrawMovementRoute(const AStrategyUnit* Unit, bool bSelected)
{
    if (!Unit || !Unit->OrderComponent) return;
    const AStrategyPlayerController* RoutePC = Cast<AStrategyPlayerController>(GetOwningPlayerController());
    const FStrategyOrder RouteOrder = RoutePC ? RoutePC->GetRequestedRoute(Unit) : Unit->OrderComponent->GetCurrentOrder();
    const FStrategyOrder ExecutingRoute = Unit->OrderComponent->GetCurrentOrder();
    const bool bRequestedRoute = RouteOrder.OrderSerial != ExecutingRoute.OrderSerial ||
        RouteOrder.WaypointRouteId != ExecutingRoute.WaypointRouteId || Unit->OrderComponent->HasDelayedOrder();
    const bool bRouteActive = (bRequestedRoute || Unit->OrderComponent->IsPhysicallyExecuting()) &&
        (RouteOrder.Type == EStrategyOrderType::Move || RouteOrder.Type == EStrategyOrderType::Advance ||
         RouteOrder.Type == EStrategyOrderType::AttackHere || !RouteOrder.Waypoints.IsEmpty());
    if (!bRouteActive) return;
    const FLinearColor RouteColour = bSelected ? Gold : FLinearColor(1.0f, 1.0f, 1.0f, 0.22f);
    const float RouteYaw = RouteOrder.bHasFacing ? RouteOrder.FacingYaw : Unit->GetActorRotation().Yaw;
    const FVector RouteGoal = RouteOrder.TargetLocation;
    const FVector RouteForward = FRotator(0.0f, RouteYaw, 0.0f).Vector();
    const FVector RouteRight(-RouteForward.Y, RouteForward.X, 0.0f);
    auto RouteOnGround = [&](const FVector& RoutePoint)
    {
        return UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, RoutePoint) + FVector(0, 0, 30);
    };
    // Sample every segment so lines follow hills rather than joining only projected endpoints.
    auto RouteLine = [&](const TArray<FVector>& RouteVertices)
    {
        TArray<FVector> GroundVertices;
        for (int32 RouteVertex = 1; RouteVertex < RouteVertices.Num(); ++RouteVertex)
        {
            const int32 RouteSamples = FMath::Clamp(FMath::CeilToInt(FVector::Dist2D(RouteVertices[RouteVertex - 1], RouteVertices[RouteVertex]) / 500.0f), 1, 256);
            for (int32 RouteSample = 0; RouteSample < RouteSamples; ++RouteSample)
                GroundVertices.Add(RouteOnGround(FMath::Lerp(RouteVertices[RouteVertex - 1], RouteVertices[RouteVertex], float(RouteSample) / RouteSamples)));
        }
        if (!RouteVertices.IsEmpty()) GroundVertices.Add(RouteOnGround(RouteVertices.Last()));
        DashedPolyline(GroundVertices, RouteColour, bSelected ? 2.0f : 1.0f, 10.0f, 6.0f);
    };
    float RouteHalfWidth = 250.0f, RouteHalfDepth = 150.0f;
    if (Unit->FormationComponent)
    {
        const UStrategyFormationComponent* RouteFormation = Unit->FormationComponent;
        const EStrategyFormationType RouteBattleFormation = Unit->FormationPolicy ?
            Unit->FormationPolicy->GetDestinationFormation() : RouteFormation->CurrentFormation;
        if (RouteBattleFormation == EStrategyFormationType::Square)
        {
            const int32 RouteMenPerSide = FMath::Max(1, FMath::CeilToInt(Unit->CurrentStrength / 4.0f));
            RouteHalfWidth = RouteHalfDepth = FMath::Max(300.0f,
                (RouteMenPerSide - 1) * RouteFormation->SoldierLateralSpacingCm * 0.5f);
        }
        else
        {
            // Match the battle slot layout, including the cavalry line's four ranks.
            const int32 RouteRanks = RouteBattleFormation == EStrategyFormationType::CavalryLine ?
                4 : FMath::Max(1, RouteFormation->RankCount);
            const int32 RouteFiles = FMath::Max(1, FMath::CeilToInt(float(Unit->CurrentStrength) / RouteRanks));
            RouteHalfWidth = FMath::Max(1, RouteFiles - 1) * RouteFormation->SoldierLateralSpacingCm * 0.5f;
            RouteHalfDepth = FMath::Max(1, FMath::Min(RouteRanks, Unit->CurrentStrength) - 1) *
                RouteFormation->SoldierRankSpacingCm * 0.5f;
        }
    }
    const FVector RouteFront = RouteForward * RouteHalfDepth, RouteSide = RouteRight * RouteHalfWidth;
    RouteLine({ RouteGoal + RouteFront + RouteSide, RouteGoal + RouteFront - RouteSide,
                RouteGoal - RouteFront - RouteSide, RouteGoal - RouteFront + RouteSide, RouteGoal + RouteFront + RouteSide });
    const FVector RouteArrowBase = RouteGoal + RouteFront;
    const FVector RouteArrowTip = RouteArrowBase + RouteForward * 700.0f;
    RouteLine({ RouteArrowBase, RouteArrowTip });
    RouteLine({ RouteArrowTip - RouteForward * 250.0f + RouteRight * 180.0f, RouteArrowTip,
                RouteArrowTip - RouteForward * 250.0f - RouteRight * 180.0f });
    const FVector RouteLabelScreen = Project(RouteOnGround(RouteGoal), false);
    if (RouteLabelScreen.Z > 0)
    {
        const FString RouteUnitLabel = Unit->DisplayName.ToString();
        float RouteLabelWidth = 0.0f, RouteLabelHeight = 0.0f;
        GetTextSize(RouteUnitLabel, RouteLabelWidth, RouteLabelHeight, nullptr, 0.85f);
        Text(RouteUnitLabel, RouteLabelScreen.X - RouteLabelWidth * 0.5f,
            RouteLabelScreen.Y - RouteLabelHeight * 0.5f, RouteColour, 0.85f);
    }
    TArray<FVector> RouteVertices { Unit->GetActorLocation() };
    if (Unit->MovementExecutor && Unit->MovementExecutor->HasMovementGoal() &&
        (!bRequestedRoute || RouteOrder.WaypointRouteId == ExecutingRoute.WaypointRouteId))
        RouteVertices.Append(Unit->MovementExecutor->GetRemainingRoutePoints());
    for (int32 RouteWaypoint = RouteOrder.NextWaypointIndex; RouteWaypoint < RouteOrder.Waypoints.Num(); ++RouteWaypoint)
    {
        const FVector RoutePoint = RouteOrder.Waypoints[RouteWaypoint];
        if (!RouteVertices.Last().Equals(RoutePoint, 1.0f)) RouteVertices.Add(RoutePoint);
        TArray<FVector> RouteMarker;
        for (int32 RouteCorner = 0; RouteCorner <= 12; ++RouteCorner)
        {
            const float RouteAngle = RouteCorner * 2.0f * PI / 12.0f;
            RouteMarker.Add(RoutePoint + FVector(FMath::Cos(RouteAngle), FMath::Sin(RouteAngle), 0) * 180.0f);
        }
        RouteLine(RouteMarker);
        const FVector RouteScreen = Project(RouteOnGround(RoutePoint), false);
        if (RouteScreen.Z > 0) Text(FString::FromInt(RouteWaypoint + 1), RouteScreen.X + 5, RouteScreen.Y - 12, RouteColour, 0.85f);
    }
    if (!RouteVertices.Last().Equals(RouteGoal, 1.0f)) RouteVertices.Add(RouteGoal);
    RouteLine(RouteVertices);
}

void AStrategyHUD::DrawFireCone(const AStrategyUnit* Unit, bool bWithLegend)
{
    const UStrategyFireControlComponent* Fire = Unit->FireControlComponent;
    if (!Fire || !Unit->IsCombatEffective())
    {
        return;
    }
    bool bTagMounted = false;
    // Horsemen have a fire cone only when they fight on foot (dismounted dragoons).
    if (const ACavalryUnit* Horse = Cast<ACavalryUnit>(Unit))
    {
        if (!Horse->DragoonComponent || Horse->DragoonComponent->MountedState == EStrategyMountedState::Mounted)
        {
            bTagMounted = true;
        }
    }
    FVector Left, Right;
    // Presentation follows the compact drawn formation; simulation uses full strength.
    const UStrategyInfantryVisualComponent* ConeVisual = Unit->FindComponentByClass<UStrategyInfantryVisualComponent>();
    if (!ConeVisual || !ConeVisual->GetDrawnFireFront(Left, Right))
    {
        Fire->GetFireFront(Left, Right, 0);
    }
    const FVector Lateral = (Right - Left).GetSafeNormal2D();
    const FVector Forward(Lateral.Y, -Lateral.X, 0.0f);
    const float Half = FMath::DegreesToRadians(Fire->FireConeHalfAngleDegrees);
    auto Dir = [&](float Angle) { return Forward * FMath::Cos(Angle) + Lateral * FMath::Sin(Angle); };   // < 0: to the left
    auto OnGround = [&](const FVector& P) { return UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, P) + FVector(0.0f, 0.0f, 30.0f); };
    // A range's outline: around the left corner, straight across the front, around the right corner.
    auto Outline = [&](float Range, int32 Steps)
    {
        TArray<FVector> Line;
        for (int32 s = 0; s <= Steps; ++s) { Line.Add(OnGround(Left + Dir(-Half + Half * s / Steps) * Range)); }
        const int32 Across = FMath::Clamp(int32(FVector::Dist2D(Left, Right) / 600.0f), 1, 12);
        for (int32 s = 1; s < Across; ++s) { Line.Add(OnGround(FMath::Lerp(Left, Right, float(s) / Across) + Forward * Range)); }
        for (int32 s = 0; s <= Steps; ++s) { Line.Add(OnGround(Right + Dir(Half * s / Steps) * Range)); }
        return Line;
    };
    const FLinearColor White(1.0f, 1.0f, 1.0f, 0.85f);
    const FLinearColor Faint(1.0f, 1.0f, 1.0f, 0.55f);
    const bool bDanish = Unit->Side == EStrategySide::Denmark;
    const FLinearColor Active = bDanish ? FLinearColor::FromSRGBColor(FColor(255, 186, 40)) : FLinearColor::FromSRGBColor(FColor(255, 96, 60));
    const float ActiveRange = Fire->GetActiveRangeCm();
    const bool bHold = Fire->FirePolicy == EStrategyFirePolicy::Hold;

    struct FRange { float Cm; const TCHAR* Name; };
    const FRange Ranges[] = { { Fire->CloseRangeCm, TEXT("CLOSE") }, { Fire->MediumRangeCm, TEXT("MEDIUM") }, { Fire->LongRangeCm, TEXT("LONG") } };
    if (!bTagMounted && Fire->IsBattleFormationReady())
    {
        // The chosen band filled (front to the chosen range), translucent; the enemy's cone only in outline (its fill
        // would lie over our own line).
        if (!bHold && bDanish)
        {
            const TArray<FVector> Edge = Outline(ActiveRange, 10);
            TArray<FVector2D> Screen;
            for (const FVector& P : Edge)
            {
                const FVector S = Project(P, false);
                if (S.Z <= 0.0f) { Screen.Reset(); break; }
                Screen.Add(FVector2D(S.X, S.Y));
            }
            const FVector LS = Project(OnGround(Left), false), RS = Project(OnGround(Right), false);
            if (Screen.Num() > 2 && LS.Z > 0.0f && RS.Z > 0.0f)
            {
                // A fan from the middle of the front (the band is convex).
                const FVector2D Centre((LS.X + RS.X) * 0.5f, (LS.Y + RS.Y) * 0.5f);
                Screen.Insert(FVector2D(LS.X, LS.Y), 0);
                Screen.Add(FVector2D(RS.X, RS.Y));
                FLinearColor Fill = Active;
                Fill.A = 0.26f;
                for (int32 i = 0; i + 1 < Screen.Num(); ++i)
                {
                    FCanvasTriangleItem Tri(Centre, Screen[i], Screen[i + 1], GWhiteTexture);
                    Tri.SetColor(Fill);
                    Tri.BlendMode = SE_BLEND_Translucent;
                    Canvas->DrawItem(Tri);
                }
            }
        }
        // The ranges: the chosen one strong and orange, the others white and faint.
        for (const FRange& R : Ranges)
        {
            const bool bActive = !bHold && FMath::IsNearlyEqual(R.Cm, ActiveRange, 1.0f);
            DashedPolyline(Outline(R.Cm, 12), bActive ? Active : Faint, bActive ? 3.5f : 1.6f, bActive ? 16.0f : 10.0f, bActive ? 8.0f : 8.0f);
            // Its distance on the middle of the arc.
            const FVector Label = Project(OnGround((Left + Right) * 0.5f + Forward * R.Cm) + FVector(0.0f, 0.0f, 120.0f), false);
            if (Label.Z > 0.0f)
            {
                const FString Text = FString::Printf(TEXT("%.0f m"), R.Cm / 100.0f);
                float TW = 0.0f, TH = 0.0f;
                GetTextSize(Text, TW, TH, nullptr, bActive ? 1.25f : 1.05f);
                DrawText(Text, FLinearColor(0.0f, 0.0f, 0.0f, 0.7f), Label.X - TW * 0.5f + 1.5f, Label.Y - TH + 1.5f, nullptr, bActive ? 1.25f : 1.05f, false);
                DrawText(Text, bActive ? Active : White, Label.X - TW * 0.5f, Label.Y - TH, nullptr, bActive ? 1.25f : 1.05f, false);
            }
        }
        // The sides from the front corners out to the long range, dashed white, with their angles.
        const float Long = Fire->LongRangeCm;
        for (const float Sign : { -1.0f, 1.0f })
        {
            const FVector Corner = Sign < 0.0f ? Left : Right;
            TArray<FVector> Side;
            for (int32 s = 0; s <= 8; ++s) { Side.Add(OnGround(Corner + Dir(Sign * Half) * Long * s / 8.0f)); }
            DashedPolyline(Side, White, 2.2f, 12.0f, 7.0f);
        }
        // The front itself.
        DashedPolyline({ OnGround(Left), OnGround(Right) }, White, 1.6f, 6.0f, 6.0f);

    } // battle formation ready

    // The unit's tag behind it (as the QA design): its name, its men, formation and fire policy.
    {
        const FVector Behind = Project(OnGround(Unit->GetVisualCentroid() - Forward * 650.0f), false);   // offset from the living drawn centre
        if (Behind.Z > 0.0f)
        {
            const TCHAR* Formation = !Unit->FormationComponent ? TEXT("") :
                Unit->FormationComponent->CurrentFormation == EStrategyFormationType::Line ? TEXT("Linie") :
                Unit->FormationComponent->CurrentFormation == EStrategyFormationType::Square ? TEXT("Karré") : TEXT("Kolonne");
            const FString Name = Unit->DisplayName.ToString();
            FString Info = FString::Printf(TEXT("%d mand | %s | Ild: %s"), Unit->CurrentStrength, Formation, bHold ? TEXT("HOLD") : *Fire->GetActiveRangeLabel().ToUpper());
            if (Unit->FieldOfficerComponent && Unit->FieldOfficerComponent->IsTakingFireCover()) Info += TEXT(" | ligger ned / spredt orden");
            else if (Unit->FieldOfficerComponent && Unit->FieldOfficerComponent->IsStandingUpFromFireCover()) Info += TEXT(" | rejser sig");
            const bool bTagReforming = Unit->UnitState == EStrategyUnitState::Reforming ||
                (Unit->FormationTransition && Unit->FormationTransition->IsReforming());
            const bool bTagMoving = Unit->MovementExecutor && !Unit->MovementExecutor->GetExecutedVelocity().IsNearlyZero();
            const bool bTagFiring = Fire->IsBattleFormationReady() && Unit->CombatComponent &&
                Unit->CombatComponent->AmmunitionRounds > 0 && Unit->CombatComponent->FindBestTarget() &&
                (!Unit->FireDisciplineComponent || Unit->FireDisciplineComponent->AllowsAutomaticFire()) &&
                (!Unit->MovementExecutor || !Unit->MovementExecutor->HasMovementGoal() || Unit->MovementExecutor->IsHoldingForFire());
            const bool bTagAdvance = Unit->OrderComponent &&
                (Unit->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::Advance ||
                 Unit->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::AttackHere);
            const bool bTagDisengage = Unit->OrderComponent && Unit->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::Disengage;
            const TCHAR* TagStatus = bTagDisengage ? TEXT("AFBRYDER") : bTagReforming ? TEXT("FORMERER") : bTagFiring ? TEXT("SKYDER") :
                bTagMoving ? (bTagAdvance ? TEXT("RYKKER FREM") : TEXT("MARCHERER")) : TEXT("HOLDER");
            const FLinearColor TagColour = bTagReforming ? FLinearColor(1.f, 0.8f, 0.1f) :
                bTagFiring ? FLinearColor(1.f, 0.15f, 0.1f) : bTagMoving ? FLinearColor(0.2f, 0.5f, 1.f) : FLinearColor(0.6f, 0.6f, 0.6f);
            Info = FString(TagStatus) + TEXT(" | ") + Info;
            float NW = 0.0f, NH = 0.0f, IW = 0.0f, IH = 0.0f;
            GetTextSize(Name, NW, NH, nullptr, 1.15f);
            GetTextSize(Info, IW, IH, nullptr, 1.0f);
            const float BoxW = FMath::Max(NW, IW) + 20.0f;
            DrawRect(FLinearColor(0.02f, 0.025f, 0.03f, 0.7f), Behind.X - BoxW * 0.5f, Behind.Y, BoxW, NH + IH + 10.0f);
            DrawRect(TagColour, Behind.X - BoxW * 0.5f + 4.f, Behind.Y + 5.f, 6.f, 6.f);
            // A NATO infantry mark above the tag.
            const float MX = Behind.X - 16.0f, MY = Behind.Y - 26.0f;
            DrawRect(bDanish ? FLinearColor(0.1f, 0.3f, 0.75f, 0.95f) : FLinearColor(0.75f, 0.15f, 0.1f, 0.95f), MX, MY, 32.0f, 22.0f);
            DrawLine(MX, MY, MX + 32.0f, MY + 22.0f, White, 1.5f);
            DrawLine(MX + 32.0f, MY, MX, MY + 22.0f, White, 1.5f);
            DrawText(Name, White, Behind.X - NW * 0.5f, Behind.Y + 3.0f, nullptr, 1.15f, false);
            DrawText(Info, White, Behind.X - IW * 0.5f, Behind.Y + NH + 5.0f, nullptr, 1.0f, false);
        }
    }

    // The legend for the first cone (top right, under the battle's end button).
    if (bWithLegend && !bTagMounted && Fire->IsBattleFormationReady())
    {
        const float W = 330.0f, X = Canvas->ClipX - W - 8.0f, Y = 92.0f;
        DrawPanel(X, Y, W, 136.0f);
        Text(FString::Printf(TEXT("FIRE POLICY (AKTIV: %s)"), bHold ? TEXT("HOLD") : *Fire->GetActiveRangeLabel().ToUpper()), X + 12.0f, Y + 10.0f, Ink);
        for (int32 i = 0; i < 3; ++i)
        {
            const bool bActive = !bHold && FMath::IsNearlyEqual(Ranges[i].Cm, ActiveRange, 1.0f);
            const float RY = Y + 36.0f + i * 22.0f;
            DrawRect(bActive ? Active : FLinearColor(0.45f, 0.45f, 0.45f, 1.0f), X + 14.0f, RY + 3.0f, 12.0f, 12.0f);
            Text(FString::Printf(TEXT("%-7s  %.0f m  %s"), Ranges[i].Name, Ranges[i].Cm / 100.0f, bActive ? TEXT("(aktiv, kraftig)") : TEXT("(svag visning)")),
                X + 36.0f, RY, bActive ? Active : Ink);
        }
        Text(FString::Printf(TEXT("Ildkegle: ±%.0f° (%.0f° i alt) fra frontens hjørner"), Fire->FireConeHalfAngleDegrees, Fire->FireConeHalfAngleDegrees * 2.0f),
            X + 14.0f, Y + 108.0f, Muted, 0.9f);
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
    const float X = 342.0f, Y = 32.0f, W = 420.0f, H = 380.0f;
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
    Text(TEXT("Fjendens skudvidde (til test)"), X + 12.0f, Y + 100.0f, Ink);
    DrawButton(X + 240.0f, Y + 96.0f, 120.0f, 24.0f, ShowEnemyRange() ? TEXT("VIST") : TEXT("SKJULT"), EAction::EnemyRange, 0, ShowEnemyRange(), nullptr, ShowEnemyRange() ? nullptr : &ButtonDark);
    for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
    {
        const bool bAttack = It->IsEnemyAttacking();
        Text(TEXT("Fjenden (til test)"), X + 12.0f, Y + 134.0f, Ink);
        DrawButton(X + 180.0f, Y + 130.0f, 110.0f, 24.0f, TEXT("ANGRIBER"), EAction::EnemyPosture, 1, bAttack, nullptr, bAttack ? nullptr : &ButtonDark);
        DrawButton(X + 296.0f, Y + 130.0f, 110.0f, 24.0f, TEXT("FORSVARER"), EAction::EnemyPosture, 0, !bAttack, nullptr, !bAttack ? nullptr : &ButtonDark);
        const bool bFire = It->IsEnemyFiring();
        Text(TEXT("Fjenden skyder (til test)"), X + 12.0f, Y + 170.0f, Ink);
        DrawButton(X + 240.0f, Y + 166.0f, 80.0f, 24.0f, TEXT("TIL"), EAction::EnemyFire, 1, bFire, nullptr, bFire ? nullptr : &ButtonDark);
        DrawButton(X + 326.0f, Y + 166.0f, 80.0f, 24.0f, TEXT("FRA"), EAction::EnemyFire, 0, !bFire, nullptr, !bFire ? nullptr : &ButtonDark);
        break;
    }
    const bool bRiders = AStrategyPlayerController::AreCouriersOn();
    Text(TEXT("Ordonnanser (ordrer tager tid)"), X + 12.0f, Y + 206.0f, Ink);
    DrawButton(X + 240.0f, Y + 202.0f, 80.0f, 24.0f, TEXT("TIL"), EAction::Couriers, 1, bRiders, nullptr, bRiders ? nullptr : &ButtonDark);
    DrawButton(X + 326.0f, Y + 202.0f, 80.0f, 24.0f, TEXT("FRA"), EAction::Couriers, 0, !bRiders, nullptr, !bRiders ? nullptr : &ButtonDark);
    // How many men are drawn: a figure for each man, for every second or for every fifth (the GPU's load).
    Text(TEXT("Mænd vist (en figur for ...)"), X + 12.0f, Y + 242.0f, Ink);
    Text(TEXT("Grafikkvalitet"), X + 12.0f, Y + 278.0f, Ink);
    const TCHAR* QualityLabels[] = { TEXT("LAV"), TEXT("MIDDEL"), TEXT("HØJ") };
    for (int32 i = 0; i < 3; ++i)
    {
        DrawButton(X + 180.0f + i * 76.0f, Y + 274.0f, 72.0f, 24.0f, QualityLabels[i], EAction::BattleQuality, i, BattleQualityPreset == i, nullptr, BattleQualityPreset == i ? nullptr : &ButtonDark);
    }
    Text(TEXT("70 / 85 / 100 % opløsning. Kvalitet gemmes."), X + 12.0f, Y + 308.0f, Muted, 0.85f);
    // All shadows on or off (the lights' shadow casting): the cheapest way to save the GPU.
    Text(TEXT("Skygger"), X + 12.0f, Y + 342.0f, Ink);
    {
        const bool bShadows = Strategy1864BattleQuality::GetShadowsOn();
        DrawButton(X + 180.0f, Y + 338.0f, 72.0f, 24.0f, TEXT("TIL"), EAction::Shadows, 1, bShadows, nullptr, bShadows ? nullptr : &ButtonDark);
        DrawButton(X + 256.0f, Y + 338.0f, 72.0f, 24.0f, TEXT("FRA"), EAction::Shadows, 0, !bShadows, nullptr, !bShadows ? nullptr : &ButtonDark);
    }
    const int32 Divisors[] = { 1, 2, 5 };
    for (int32 i = 0; i < 3; ++i)
    {
        DrawButton(X + 240.0f + i * 58.0f, Y + 238.0f, 54.0f, 24.0f, *FString::Printf(TEXT("%d"), Divisors[i]), EAction::FigureScale, Divisors[i], FigureDivisor == Divisors[i], nullptr, FigureDivisor == Divisors[i] ? nullptr : &ButtonDark);
    }
}

namespace
{
    int32 GEnemyRangeShown = -1;   // -1: not read from the settings yet
}

bool AStrategyHUD::ShowEnemyRange()
{
    if (GEnemyRangeShown < 0)
    {
        bool bShow = false;
        if (GConfig)
        {
            GConfig->GetBool(TEXT("/Script/Strategy1864.Settings"), TEXT("ShowEnemyRange"), bShow, GGameUserSettingsIni);
        }
        GEnemyRangeShown = bShow || FParse::Param(FCommandLine::Get(), TEXT("Strategy1864ShowEnemyRange")) ? 1 : 0;
    }
    return GEnemyRangeShown == 1;
}

void AStrategyHUD::SetShowEnemyRange(bool bShow)
{
    GEnemyRangeShown = bShow ? 1 : 0;
    if (GConfig)
    {
        GConfig->SetBool(TEXT("/Script/Strategy1864.Settings"), TEXT("ShowEnemyRange"), bShow, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
}

// ------------------------------------------------------------------ minimap

void AStrategyHUD::DrawMinimap()
{
    // The battlefield from above, bottom right above the command panel: every unit, the camera's place.
    const float W = 300.0f, H = 200.0f;
    const float X = Canvas->ClipX - W - 8.0f, Y = Canvas->ClipY - CommandHeight() - H - 36.0f;
    DrawPanel(X, Y, W, H + 28.0f);
    Text(TEXT("TAKTISK KORT / KAMERA  [M]"), X + 10.0f, Y + 6.0f, Gold, 0.9f);
    const float MX = X + 8.0f, MY = Y + 26.0f, MW = W - 16.0f, MH = H - 6.0f;
    DrawRect(FLinearColor::FromSRGBColor(FColor(52, 78, 44, 240)), MX, MY, MW, MH);
    TSet<FName> ForestVisibleEnemies;
    for (TActorIterator<AStrategyUnit> ForestObserver(GetWorld()); ForestObserver; ++ForestObserver)
    {
        if (!IsValid(*ForestObserver) || ForestObserver->Side != EStrategySide::Denmark || !ForestObserver->IsCombatEffective() || !ForestObserver->ContactComponent) continue;
        for (const FStrategyContactRecord& ForestContact : ForestObserver->ContactComponent->GetKnownContacts())
            if (ForestContact.bCurrentlyVisible) ForestVisibleEnemies.Add(ForestContact.StableUnitId);
    }
    // Hidden enemies must not affect the map bounds either.
    FBox Bounds(ForceInit);
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It) && (It->Side == EStrategySide::Denmark || ForestVisibleEnemies.Contains(It->StableUnitId))) { Bounds += It->GetActorLocation(); }
    }
    if (!Bounds.IsValid) { Bounds = FBox(FVector(-30000.0f), FVector(30000.0f)); }
    FVector Centre = Bounds.GetCenter();
    const float Half = FMath::Max3(30000.0f, float(Bounds.GetExtent().X) + 15000.0f, (float(Bounds.GetExtent().Y) + 15000.0f) * MW / MH);
    MinimapCentre = FVector2D(Centre.X, Centre.Y);
    MinimapHalfWidth = Half;
    MinimapRect = FBox2D(FVector2D(MX, MY), FVector2D(MX + MW, MY + MH));
    const float Scale = MW / (2.0f * Half);
    // World +X is north (up the map), +Y east (right).
    // Fixed raster budget, direct cached-grid lookups; no forest-grid scan per frame.
    for (int32 ForestRow = 0; ForestRow < 24; ++ForestRow)
    for (int32 ForestColumn = 0; ForestColumn < 32; ++ForestColumn)
    {
        const FVector ForestWorld(Centre.X + Half * (1.0 - 2.0 * (ForestRow + 0.5) / 24.0) * MH / MW,
            Centre.Y + Half * (2.0 * (ForestColumn + 0.5) / 32.0 - 1.0), 0.0);
        const float ForestDensity = UStrategyTerrainQueryLibrary::GetForestDensityAt(this, ForestWorld);
        if (ForestDensity > 0.0f) DrawRect(FLinearColor(0.015f, 0.10f, 0.025f, ForestDensity), MX + ForestColumn * MW / 32.0f, MY + ForestRow * MH / 24.0f, MW / 32.0f, MH / 24.0f);
    }
    auto ToMap = [&](const FVector& P) { return FVector2D(MX + MW * 0.5f + (P.Y - Centre.Y) * Scale, MY + MH * 0.5f - (P.X - Centre.X) * Scale); };
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        const AStrategyUnit* Unit = *It;
        if (!IsValid(Unit) || Unit->CurrentStrength <= 0 ||
            (Unit->Side != EStrategySide::Denmark && !ForestVisibleEnemies.Contains(Unit->StableUnitId)))
        {
            continue;
        }
        const FVector2D P = ToMap(Unit->GetActorLocation());
        if (!MinimapRect.IsInside(P)) { continue; }
        const float S = IsCommandHQ(Unit) ? 5.0f : 4.0f;
        const FLinearColor C = Unit->bSelected ? Gold : Unit->Side == EStrategySide::Denmark ? FLinearColor::FromSRGBColor(FColor(70, 220, 255)) : FLinearColor::FromSRGBColor(FColor(235, 50, 40));
        DrawRect(C, P.X - S, P.Y - S * 0.6f, S * 2.0f, S * 1.2f);
        if (Unit->Side == EStrategySide::Denmark && Unit->VisibilityComponent && Unit->VisibilityComponent->IsConcealedInForest(Unit))
            Text(TEXT("skjult"), P.X + S + 2.0f, P.Y - 5.0f, Ink, 0.6f);
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

float AStrategyHUD::CommandHeight() const
{
    return 226.f;
}

void AStrategyHUD::DrawRounded(float X, float Y, float W, float H, const FLinearColor& Fill, const FLinearColor* BorderOverride, float RadiusOverride)
{
    // Scanline rounded rectangles need no textures or additional fonts. Short ones are pills (radius = half the height).
    const float Radius = FMath::Min(RadiusOverride >= 0.f ? RadiusOverride : (H <= 40.f ? H * 0.5f : 14.f), H * 0.5f);
    const FLinearColor Border = BorderOverride ? *BorderOverride : FLinearColor(0.20f, 0.26f, 0.33f, 0.85f);
    for (float Row = 0.f; Row < H; Row += 1.f)
    {
        const float Edge = FMath::Min(Row, H - 1.f - Row);
        const float Inset = Edge < Radius ? Radius - FMath::Sqrt(FMath::Max(0.f, Radius * Radius - FMath::Square(Radius - Edge))) : 0.f;
        DrawRect(Border, X + Inset, Y + Row, FMath::Max(0.f, W - 2.f * Inset), 1.f);
        if (Row > 0.f && Row < H - 1.f) DrawRect(Fill, X + Inset + 1.f, Y + Row, FMath::Max(0.f, W - 2.f * Inset - 2.f), 1.f);
    }
}

void AStrategyHUD::DrawHeading(const FString& Label, float X, float Y, float W, int32 Icon)
{
    const FLinearColor Light(0.78f, 0.84f, 0.92f, 1.f);
    const float CX = X + 8.f, CY = Y + 9.f;
    switch (Icon)
    {
        case 0:   // hatch: three slanted strokes
            for (int32 k = 0; k < 3; ++k) DrawLine(X + 1.f + k * 5.f, CY + 7.f, X + 8.f + k * 5.f, CY - 7.f, Light, 1.5f);
            break;
        case 1:   // crosshair
            DrawLine(CX - 8.f, CY, CX + 8.f, CY, Light, 1.2f); DrawLine(CX, CY - 8.f, CX, CY + 8.f, Light, 1.2f);
            DrawLine(CX - 5.f, CY - 5.f, CX + 5.f, CY - 5.f, Light, 1.f); DrawLine(CX + 5.f, CY - 5.f, CX + 5.f, CY + 5.f, Light, 1.f);
            DrawLine(CX + 5.f, CY + 5.f, CX - 5.f, CY + 5.f, Light, 1.f); DrawLine(CX - 5.f, CY + 5.f, CX - 5.f, CY - 5.f, Light, 1.f);
            break;
        case 2:   // double chevron
            for (int32 k = 0; k < 2; ++k) { DrawLine(X + k * 7.f, CY - 7.f, X + 7.f + k * 7.f, CY, Light, 1.8f); DrawLine(X + 7.f + k * 7.f, CY, X + k * 7.f, CY + 7.f, Light, 1.8f); }
            break;
        case 3:   // three dots in a triangle
            DrawRect(Light, CX - 2.f, CY - 8.f, 5.f, 5.f); DrawRect(Light, CX - 8.f, CY + 2.f, 5.f, 5.f); DrawRect(Light, CX + 4.f, CY + 2.f, 5.f, 5.f);
            break;
        default:  // layers
            for (int32 k = 0; k < 3; ++k)
            {
                const float LY = CY - 6.f + k * 6.f;
                DrawLine(X, LY, X + 8.f, LY - 4.f, Light, 1.3f); DrawLine(X + 8.f, LY - 4.f, X + 16.f, LY, Light, 1.3f);
                DrawLine(X + 16.f, LY, X + 8.f, LY + 4.f, Light, 1.3f); DrawLine(X + 8.f, LY + 4.f, X, LY, Light, 1.3f);
            }
            break;
    }
    float TW = 0.f, TH = 0.f;
    GetTextSize(Label, TW, TH);
    Text(Label, X + 26.f, Y + 1.f, Light, FMath::Min(1.0f, (W - 26.f) / FMath::Max(TW, 1.f)));
}

void AStrategyHUD::DrawStatBar(const FString& Label, const FString& Value, float Fraction, float X, float Y, float W)
{
    // A column: the small caption, the figure in large type, a thin rounded bar under it.
    Text(Label, X, Y, Muted, 0.72f);
    Text(Value, X, Y + 13.f, Ink, 1.25f);
    const float Level = FMath::Clamp(Fraction, 0.f, 1.f);
    const FLinearColor Track(0.10f, 0.13f, 0.17f, 1.f);
    const FLinearColor Colour = Level < 0.3f ? FLinearColor(0.88f, 0.20f, 0.18f) : Level < 0.6f ? FLinearColor(0.92f, 0.74f, 0.20f) : FLinearColor(0.20f, 0.85f, 0.42f);
    DrawRounded(X, Y + 38.f, W, 7.f, Track, &Track, 3.5f);
    if (Level > 0.02f) { DrawRounded(X, Y + 38.f, FMath::Max(7.f, W * Level), 7.f, Colour, &Colour, 3.5f); }
}

bool AStrategyHUD::HandleScroll(const FVector2D& Point, float Delta)
{
    if (!SubordinateRect.bIsValid || !SubordinateRect.IsInside(Point)) return false;
    SubordinateOffset = FMath::Clamp(SubordinateOffset + (Delta > 0.f ? -1 : 1), 0, SubordinateMaxOffset);
    return true;
}

void AStrategyHUD::DrawCommandPanel(AStrategyUnit* Unit)
{
    bCommandStyle = true;
    const float HudY = Canvas->ClipY - CommandHeight();
    const float HudWeights[] = {20.f, 26.f, 16.f, 22.f, 24.f};
    float HudX[5], HudW[5], HudCursor = 8.f;
    const TCHAR* HudTitles[] = {TEXT("E N H E D"), TEXT("LEDELSE & ILD"), TEXT("O R D R E R"), TEXT("FORMATION"), TEXT("UNDERLAGTE")};
    Panels.Add(FBox2D(FVector2D(0.f, HudY), FVector2D(Canvas->ClipX, Canvas->ClipY)));
    {
        // One dark rounded bar behind the five panels.
        const FLinearColor BarFill(0.014f, 0.026f, 0.044f, 0.97f), BarEdge(0.16f, 0.22f, 0.30f, 0.9f);
        DrawRounded(2.f, HudY + 2.f, Canvas->ClipX - 4.f, CommandHeight() - 4.f, BarFill, &BarEdge, 14.f);
    }
    for (int32 PanelIndex = 0; PanelIndex < 5; ++PanelIndex)
    {
        HudW[PanelIndex] = FMath::Max(1.f, (Canvas->ClipX - 48.f) * HudWeights[PanelIndex] / 108.f);
        HudX[PanelIndex] = HudCursor + 10.f;
        if (PanelIndex > 0) { DrawLine(HudCursor - 4.f, HudY + 14.f, HudCursor - 4.f, HudY + CommandHeight() - 16.f, FLinearColor(0.16f, 0.21f, 0.28f, 0.8f), 1.f); }
        DrawHeading(HudTitles[PanelIndex], HudX[PanelIndex], HudY + 14.f, HudW[PanelIndex] - 20.f, PanelIndex);
        DrawLine(HudX[PanelIndex], HudY + 34.f, HudX[PanelIndex] + HudW[PanelIndex] - 20.f, HudY + 34.f, FLinearColor(0.14f, 0.19f, 0.26f, 0.8f), 1.f);
        HudCursor += HudW[PanelIndex] + 8.f;
        HudW[PanelIndex] -= 20.f;
    }
    SubordinateRect = FBox2D(ForceInit);
    if (ScrollUnit.Get() != Unit) { ScrollUnit = Unit; SubordinateOffset = 0; }
    if (!Unit)
    {
        Text(TEXT("Ingen enhed valgt"), HudX[0], HudY + 58.f, Ink, 0.9f);
        bCommandStyle = false;
        return;
    }
    const bool HudHQ = IsCommandHQ(Unit);
    const FString HudRank = Unit->OfficerProfileComponent && !Unit->OfficerProfileComponent->OfficerRank.IsEmpty()
        ? Unit->OfficerProfileComponent->OfficerRank.ToUpper()
        : Unit->Echelon == EStrategyEchelon::Battalion ? TEXT("MAJOR") : Unit->Echelon == EStrategyEchelon::Regiment ? TEXT("OBERSTL\u00d8JTNANT")
        : Unit->Echelon == EStrategyEchelon::Cavalry ? TEXT("RITMESTER") : HudHQ ? TEXT("CHEF") : TEXT("KAPTAJN");
    float HudTW = 0.f, HudTH = 0.f;
    {
        // The unit: a tile with its mark, the name in large type, the rank and post in spaced capitals, then the three figures.
        const FLinearColor TileFill(0.02f, 0.04f, 0.07f, 1.f), TileEdge(0.22f, 0.30f, 0.40f, 1.f);
        DrawRounded(HudX[0], HudY + 42.f, 44.f, 44.f, TileFill, &TileEdge, 8.f);
        DrawRect(FLinearColor(0.35f, 0.65f, 1.f, 1.f), HudX[0] + 18.f, HudY + 50.f, 8.f, 28.f);
        auto Spaced = [](const FString& In) { FString Out; for (int32 k = 0; k < In.Len(); ++k) { Out.AppendChar(In[k]); Out.AppendChar(TCHAR(' ')); } return Out; };
        GetTextSize(Unit->DisplayName.ToString(), HudTW, HudTH);
        Text(Unit->DisplayName.ToString(), HudX[0] + 54.f, HudY + 42.f, Ink, FMath::Min(1.5f, (HudW[0] - 54.f) / FMath::Max(1.f, HudTW)));
        Text(Spaced(HudRank), HudX[0] + 54.f, HudY + 64.f, Muted, 0.72f);
        const FString HudPost = HudHQ ? TEXT("STABSKOMMANDO") : Unit->Echelon == EStrategyEchelon::Artillery ? TEXT("BATTERIKOMMANDO") : Unit->Echelon == EStrategyEchelon::Cavalry ? TEXT("KAVALERIKOMMANDO") : TEXT("KOMPAGNIKOMMANDO");
        GetTextSize(Spaced(HudPost), HudTW, HudTH);
        Text(Spaced(HudPost), HudX[0] + 54.f, HudY + 79.f, Muted, FMath::Min(0.66f, (HudW[0] - 54.f) / FMath::Max(1.f, HudTW)));
        DrawLine(HudX[0], HudY + 98.f, HudX[0] + HudW[0], HudY + 98.f, FLinearColor(0.14f, 0.19f, 0.26f, 0.8f), 1.f);
        const float ColW = (HudW[0] - 20.f) / 3.f;
        DrawStatBar(TEXT("STYRKE"), FString::Printf(TEXT("%d/%d"), Unit->CurrentStrength, Unit->InitialStrength), float(Unit->CurrentStrength) / FMath::Max(1, Unit->InitialStrength), HudX[0], HudY + 112.f, ColW);
        DrawStatBar(TEXT("MORAL"), FString::Printf(TEXT("%.0f"), Unit->Morale), Unit->Morale / 100.f, HudX[0] + ColW + 10.f, HudY + 112.f, ColW);
        DrawStatBar(TEXT("SAMHOLD"), FString::Printf(TEXT("%.0f"), Unit->Cohesion), Unit->Cohesion / 100.f, HudX[0] + 2.f * (ColW + 10.f), HudY + 112.f, ColW);
    }
    auto HudPills = [&](int32 Panel, float RowY, const TCHAR* const* Labels, int32 Count, EAction Action, int32 Active, bool Enabled = true)
    {
        const float PillW = (HudW[Panel] - (Count - 1) * 4.f) / Count;
        for (int32 PillIndex = 0; PillIndex < Count; ++PillIndex)
            DrawButton(HudX[Panel] + PillIndex * (PillW + 4.f), HudY + RowY, PillW, 23.f, Labels[PillIndex], Enabled ? Action : EAction::None, PillIndex, Active == PillIndex && Enabled, Unit);
    };
    Text(TEXT("AI"), HudX[1], HudY + 42.f, Muted, 0.7f);
    const TCHAR* HudAI[] = {TEXT("ON"), TEXT("OFF")};
    HudPills(1, 57.f, HudAI, 2, EAction::AIToggle, Unit->bOfficerAIEnabled ? 0 : 1);
    Text(TEXT("DOKTRIN"), HudX[1], HudY + 84.f, Muted, 0.7f);
    const TCHAR* HudDoctrine[] = {TEXT("DEF"), TEXT("BAL"), TEXT("OFF")};
    HudPills(1, 99.f, HudDoctrine, 3, EAction::Doctrine, Unit->DoctrineComponent ? int32(Unit->DoctrineComponent->Doctrine) : -1, Unit->DoctrineComponent != nullptr);
    Text(TEXT("SKYDNING"), HudX[1], HudY + 126.f, Muted, 0.7f);
    const TCHAR* HudFire[] = {TEXT("HOLD"), TEXT("CLOSE"), TEXT("MED"), TEXT("LONG")};
    HudPills(1, 141.f, HudFire, 4, EAction::FirePolicy, Unit->FireControlComponent ? int32(Unit->FireControlComponent->FirePolicy) : -1, !HudHQ && Unit->FireControlComponent);
    Text(TEXT("SALVEMETODE"), HudX[1], HudY + 168.f, Muted, 0.7f);
    const TCHAR* HudDrills[] = {TEXT("1.GLD"), TEXT("2.GLD"), TEXT("GELED"), TEXT("SALVE"), TEXT("FRI")};
    const EStrategyFireDrillMode HudModes[] = {EStrategyFireDrillMode::FrontRank, EStrategyFireDrillMode::TwoRankFire, EStrategyFireDrillMode::FireByRank, EStrategyFireDrillMode::Volley, EStrategyFireDrillMode::Independent};
    for (int32 DrillIndex = 0; DrillIndex < 5; ++DrillIndex)
    {
        const bool HudUnlocked = Cast<AStrategyCompanyUnit>(Unit) && Unit->FireDrillComponent && Unit->FireDrillComponent->IsDrillModeUnlocked(HudModes[DrillIndex]);
        const float HudPillW = (HudW[1] - 16.f) / 5.f;
        DrawButton(HudX[1] + DrillIndex * (HudPillW + 4.f), HudY + 183.f, HudPillW, 23.f, HudDrills[DrillIndex], HudUnlocked ? EAction::FireDrill : EAction::None, int32(HudModes[DrillIndex]), HudUnlocked && Unit->FireDrillComponent->DrillMode == HudModes[DrillIndex], Unit);
    }
    const EStrategyOrderType HudCurrent = Unit->OrderComponent ? Unit->OrderComponent->GetCurrentOrder().Type : EStrategyOrderType::None;
    const float HudOrderW = (HudW[2] - 5.f) / 2.f;
    const TCHAR* HudOrders[] = {TEXT("> RYK FREM"), TEXT("< TILBAGE"), TEXT("X CHARGE"), TEXT("[] STOP")};
    const EAction HudActions[] = {EAction::Order, EAction::Order, EAction::Charge, EAction::Stop};
    const EStrategyOrderType HudTypes[] = {EStrategyOrderType::Advance, EStrategyOrderType::Withdraw, EStrategyOrderType::Charge, EStrategyOrderType::Hold};
    for (int32 OrderIndex = 0; OrderIndex < 4; ++OrderIndex)
    {
        const FLinearColor* HudColour = OrderIndex == 0 ? &ExecutingBlue : OrderIndex == 2 ? &ActiveGreen : &OrderRed;
        DrawButton(HudX[2] + (OrderIndex % 2) * (HudOrderW + 5.f), HudY + 43.f + (OrderIndex / 2) * 44.f, HudOrderW, 38.f, HudOrders[OrderIndex], HudActions[OrderIndex], int32(HudTypes[OrderIndex]), HudCurrent == HudTypes[OrderIndex], Unit, HudColour);
    }
    if (!HudHQ && Cast<AStrategyCompanyUnit>(Unit))
        DrawButton(HudX[2], HudY + 132.f, HudW[2], 30.f, TEXT("AFBRYD"), EAction::Disengage, 0, HudCurrent == EStrategyOrderType::Disengage, Unit, &OrderRed);
    if (!HudHQ) Text(BaseOrderLabel(Unit), HudX[2], HudY + 185.f, Muted, 0.72f);
    if (HudHQ)
    {
        const TCHAR* HudExtra[] = {TEXT("ANGRIB"), TEXT("FORSVAR"), TEXT("SAML")};
        const EStrategyOrderType HudExtraTypes[] = {EStrategyOrderType::AttackHere, EStrategyOrderType::DefendHere, EStrategyOrderType::Assemble};
        for (int32 ExtraIndex = 0; ExtraIndex < 3; ++ExtraIndex)
            DrawButton(HudX[2], HudY + 132.f + ExtraIndex * 25.f, HudW[2], 22.f, HudExtra[ExtraIndex], EAction::Order, int32(HudExtraTypes[ExtraIndex]), HudCurrent == HudExtraTypes[ExtraIndex], Unit);
        for (TActorIterator<AStrategyOOBTestScenario> HudScenario(GetWorld()); HudScenario; ++HudScenario)
        {
            if (HudScenario->CanLayPontoonBridges()) DrawButton(HudX[3], HudY + 183.f, HudW[3], 23.f, TEXT("SL\u00c5 PONTONBRO"), EAction::Pontoon, 0, HudScenario->GetPontoonSecondsLeft() > 0.f, Unit);
            break;
        }
    }
    const TCHAR* HudFormation[] = {TEXT("LINJE"), TEXT("KOLONNE"), TEXT("KARRE")};
    const EStrategyFormationType HudFormTypes[] = {EStrategyFormationType::Line, EStrategyFormationType::MarchColumn, EStrategyFormationType::Square};
    const int32 HudFormCount = Cast<ACavalryUnit>(Unit) ? 2 : 3;
    for (int32 FormIndex = 0; FormIndex < HudFormCount; ++FormIndex)
    {
        const float HudFW = (HudW[3] - (HudFormCount - 1) * 4.f) / HudFormCount;
        DrawButton(HudX[3] + FormIndex * (HudFW + 4.f), HudY + 43.f, HudFW, 23.f, HudFormation[FormIndex], !HudHQ && Unit->FormationComponent ? EAction::Formation : EAction::None, int32(HudFormTypes[FormIndex]), !HudHQ && Unit->FormationComponent && Unit->FormationComponent->CurrentFormation == HudFormTypes[FormIndex], Unit);
    }
    Text(TEXT("STILLING"), HudX[3], HudY + 78.f, Muted, 0.7f);
    const TCHAR* HudStances[] = {TEXT("STA"), TEXT("KNAE"), TEXT("LIG")};
    const EStrategyStance HudStanceTypes[] = {EStrategyStance::Standing, EStrategyStance::Kneeling, EStrategyStance::Prone};
    for (int32 StanceIndex = 0; StanceIndex < 3; ++StanceIndex)
    {
        const float HudSW = (HudW[3] - 8.f) / 3.f;
        const bool HudCanStance = Cast<AStrategyCompanyUnit>(Unit) && Unit->StanceComponent;
        DrawButton(HudX[3] + StanceIndex * (HudSW + 4.f), HudY + 94.f, HudSW, 23.f, HudStances[StanceIndex], HudCanStance ? EAction::Stance : EAction::None, int32(HudStanceTypes[StanceIndex]), HudCanStance && Unit->StanceComponent->Stance == HudStanceTypes[StanceIndex], Unit);
    }
    if (const ACavalryUnit* HudHorse = Cast<ACavalryUnit>(Unit))
    {
        if (HudHorse->DragoonComponent && HudHorse->DragoonComponent->Role == EStrategyCavalryRole::Dragoon)
        {
            const bool HudFoot = HudHorse->DragoonComponent->MountedState != EStrategyMountedState::Mounted;
            DrawButton(HudX[3], HudY + 94.f, (HudW[3] - 4.f) / 2.f, 23.f, TEXT("SIT AF"), EAction::Dismount, 1, HudFoot, Unit);
            DrawButton(HudX[3] + (HudW[3] + 4.f) / 2.f, HudY + 94.f, (HudW[3] - 4.f) / 2.f, 23.f, TEXT("STIG P\u00c5"), EAction::Dismount, 0, !HudFoot, Unit);
        }
    }
    Text(TEXT("SKUDAFSTAND"), HudX[3], HudY + 130.f, Muted, 0.7f);
    if (!HudHQ && Unit->FireControlComponent)
    {
        const UStrategyFireControlComponent* HudFC = Unit->FireControlComponent;
        const float HudRanges[] = {HudFC->CloseRangeCm, HudFC->MediumRangeCm, HudFC->LongRangeCm};
        // A horizontal range track with the three bands; the chosen one is a yellow pill. The cone half angle stands to the right.
        const float TrackW = (HudW[3] - 24.f) * 0.62f, TrackX = HudX[3] + 12.f, TrackY = HudY + 156.f;
        const FLinearColor TrackCol(0.12f, 0.16f, 0.21f, 1.f);
        DrawRounded(TrackX, TrackY, TrackW, 8.f, TrackCol, &TrackCol, 4.f);
        for (int32 RangeIndex = 0; RangeIndex < 3; ++RangeIndex)
        {
            const float HudRX = TrackX + RangeIndex * TrackW / 2.f;
            const bool HudChosen = int32(HudFC->FirePolicy) == RangeIndex + 1;
            if (HudChosen)
            {
                const FLinearColor Yellow(0.95f, 0.78f, 0.22f, 1.f);
                DrawRounded(FMath::Clamp(HudRX - 16.f, TrackX, TrackX + TrackW - 32.f), TrackY, 32.f, 8.f, Yellow, &Yellow, 4.f);
            }
            const FString RangeText = FString::Printf(TEXT("%.0f m"), HudRanges[RangeIndex] / 100.f);
            float RTW = 0.f, RTH = 0.f;
            GetTextSize(RangeText, RTW, RTH, nullptr, 0.72f);
            Text(RangeText, FMath::Clamp(HudRX - RTW * 0.5f, TrackX - 6.f, TrackX + TrackW - RTW + 6.f), TrackY + 14.f, HudChosen ? Gold : Muted, 0.72f);
        }
        const float ConeX = TrackX + TrackW + 16.f;
        DrawLine(ConeX, HudY + 160.f, ConeX + 14.f, HudY + 152.f, Muted, 1.2f);
        DrawLine(ConeX, HudY + 160.f, ConeX + 14.f, HudY + 168.f, Muted, 1.2f);
        DrawLine(ConeX + 14.f, HudY + 152.f, ConeX + 14.f, HudY + 168.f, Muted, 1.2f);
        Text(FString::Printf(TEXT("KEGLE \u00b1%.0f\u00b0"), HudFC->FireConeHalfAngleDegrees), ConeX + 22.f, HudY + 153.f, Muted, 0.72f);
    }
    if (HudHQ && Unit->bOfficerAIEnabled && Unit->AITelemetryComponent && !Unit->AITelemetryComponent->CurrentTask.IsEmpty())
    {
        Text(TEXT("OFFICEREN"), HudX[3], HudY + 132.f, Muted, 0.7f);
        const FString HudTask = Unit->AITelemetryComponent->CurrentTask;
        GetTextSize(HudTask, HudTW, HudTH);
        Text(HudTask, HudX[3], HudY + 148.f, Ink, FMath::Min(0.72f, HudW[3] / FMath::Max(1.f, HudTW)));
        const FString HudReason = Unit->AITelemetryComponent->ReasonCode;
        GetTextSize(HudReason, HudTW, HudTH);
        Text(HudReason, HudX[3], HudY + 164.f, Muted, FMath::Min(0.66f, HudW[3] / FMath::Max(1.f, HudTW)));
    }
    const TArray<AStrategyUnit*> HudSubs = Subordinates(Unit);
    const float HudTableW = HudW[4] - 9.f;
    const float HudColumns[] = {0.f, 0.40f, 0.54f, 0.79f, 0.90f};
    const TCHAR* HudColumnNames[] = {TEXT("ENHED"), TEXT("M\u00c6ND"), TEXT("ORDRE"), TEXT("AI"), TEXT("TILK")};
    for (int32 ColumnIndex = 0; ColumnIndex < 5; ++ColumnIndex) Text(HudColumnNames[ColumnIndex], HudX[4] + HudColumns[ColumnIndex] * HudTableW, HudY + 43.f, Muted, 0.62f);
    const int32 HudVisible = 5;
    SubordinateMaxOffset = FMath::Max(0, HudSubs.Num() - HudVisible);
    SubordinateOffset = FMath::Clamp(SubordinateOffset, 0, SubordinateMaxOffset);
    SubordinateRect = FBox2D(FVector2D(HudX[4], HudY + 62.f), FVector2D(HudX[4] + HudW[4], HudY + 207.f));
    for (int32 SubIndex = SubordinateOffset; SubIndex < FMath::Min(HudSubs.Num(), SubordinateOffset + HudVisible); ++SubIndex)
    {
        AStrategyUnit* HudSub = HudSubs[SubIndex];
        const float HudRowY = HudY + 64.f + (SubIndex - SubordinateOffset) * 28.f;
        DrawButton(HudX[4], HudRowY, HudTableW, 25.f, TEXT(""), EAction::OOBRow, 0, false, HudSub);
        const bool HudAttached = HudSub->CommandComponent && HudSub->CommandComponent->CurrentCommandParent != HudSub->CommandComponent->OrganicParent;
        const FString HudCells[] = {HudSub->DisplayName.ToString(), FString::FromInt(MenUnder(HudSub)), BaseOrderLabel(HudSub).IsEmpty() ? FString(TEXT("-")) : BaseOrderLabel(HudSub), HudSub->bOfficerAIEnabled ? TEXT("ON") : TEXT("OFF"), HudAttached ? TEXT("ATT") : TEXT("-")};
        for (int32 CellIndex = 0; CellIndex < 5; ++CellIndex)
        {
            const float HudCellX = HudX[4] + HudColumns[CellIndex] * HudTableW;
            const float HudCellW = ((CellIndex == 4 ? 1.f : HudColumns[CellIndex + 1]) - HudColumns[CellIndex]) * HudTableW - 3.f;
            if (CellIndex == 2 || CellIndex == 3) DrawRounded(HudCellX, HudRowY + 3.f, HudCellW, 19.f, CellIndex == 3 && HudSub->bOfficerAIEnabled ? ActiveGreen : RowColour);
            GetTextSize(HudCells[CellIndex], HudTW, HudTH);
            Text(HudCells[CellIndex], HudCellX + 2.f, HudRowY + 6.f, Ink, FMath::Min(0.72f, (HudCellW - 4.f) / FMath::Max(1.f, HudTW)));
        }
    }
    if (HudSubs.IsEmpty()) Text(TEXT("Ingen underlagte"), HudX[4], HudY + 73.f, Muted, 0.8f);
    if (SubordinateMaxOffset > 0)
    {
        const float HudTrackH = 140.f, HudThumbH = HudTrackH * HudVisible / HudSubs.Num();
        DrawRounded(HudX[4] + HudW[4] - 5.f, HudY + 64.f, 5.f, HudTrackH, RowColour);
        DrawRounded(HudX[4] + HudW[4] - 5.f, HudY + 64.f + (HudTrackH - HudThumbH) * SubordinateOffset / SubordinateMaxOffset, 5.f, HudThumbH, Muted);
    }
    bCommandStyle = false;
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
    if (SubordinateMaxOffset > 0 && SubordinateRect.bIsValid && SubordinateRect.IsInside(P) && P.X >= SubordinateRect.Max.X - 8.f)
    {
        const float ScrollFraction = FMath::Clamp((P.Y - SubordinateRect.Min.Y) / SubordinateRect.GetSize().Y, 0.f, 1.f);
        SubordinateOffset = FMath::RoundToInt(ScrollFraction * SubordinateMaxOffset);
        return true;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugClicks")))
    {
        int32 Hit = INDEX_NONE;
        for (int32 i = Buttons.Num() - 1; i >= 0 && Hit == INDEX_NONE; --i) { if (Buttons[i].Box.IsInside(P)) { Hit = i; } }
        static bool bDumped = false;
        if (!bDumped)
        {
            bDumped = true;
            for (const FButton& Dump : Buttons)
            {
                UE_LOG(LogTemp, Display, TEXT("PROJECT1864-HUD: button action=%d value=%d box=%.0f,%.0f-%.0f,%.0f"), int32(Dump.Action), Dump.Value, Dump.Box.Min.X, Dump.Box.Min.Y, Dump.Box.Max.X, Dump.Box.Max.Y);
            }
        }
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-HUD: click %.0f,%.0f buttons=%d hit=%d action=%d"), P.X, P.Y, Buttons.Num(), Hit, Hit != INDEX_NONE ? int32(Buttons[Hit].Action) : -1);
    }
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
            case EAction::BattleQuality:
                BattleQualityPreset = B.Value;   // the graphics preset; the number of figures is its own setting
                Strategy1864BattleQuality::ApplyPreset(GetWorld(), B.Value, true);
                break;
            case EAction::Shadows:
                Strategy1864BattleQuality::SetShadowsOn(GetWorld(), B.Value != 0, true);
                break;
            case EAction::TimeControl:
                if (AStrategyPlayerController* TimePC = Cast<AStrategyPlayerController>(GetOwningPlayerController()))
                {
                    if (B.Value == 0) { TimePC->TogglePauseSimulation(); }
                    else { TimePC->SetSimulationSpeed(B.Value < 0 ? 0.5f : float(B.Value)); }
                }
                break;
            case EAction::FigureScale:
                FigureDivisor = B.Value;
                Strategy1864BattleQuality::SetFigureDivisor(B.Value);
                for (TActorIterator<AStrategyCompanyUnit> It(GetWorld()); It; ++It)
                {
                    if (IsValid(*It) && It->InfantryVisualComponent) { It->InfantryVisualComponent->SetVisualScaleDivisor(B.Value); }
                }
                break;
            case EAction::Couriers:
                AStrategyPlayerController::SetCouriersOn(B.Value == 1);
                break;
            case EAction::EnemyRange:
                SetShowEnemyRange(!ShowEnemyRange());
                break;
            case EAction::EnemyFire:
                for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
                {
                    It->SetEnemyFiring(B.Value == 1);
                    break;
                }
                break;
            case EAction::EnemyPosture:
                for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
                {
                    It->SetEnemyAttacking(B.Value == 1);
                    break;
                }
                break;
            case EAction::CameraSpeed:
            {
                static const float Steps[] = { 1.0f, 2.0f, 3.0f, 5.0f, 8.0f, 10.0f, 15.0f, 20.0f };
                AStrategyCameraPawn::SetKeySpeedFactor(Steps[FMath::Clamp(B.Value, 0, int32(UE_ARRAY_COUNT(Steps)) - 1)]);
                break;
            }
            case EAction::FinishBattle:
                {
                    bool bCampaignBattle = false;
                    for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
                    {
                        bCampaignBattle = It->IsCampaignBattle();
                        if (bCampaignBattle) { It->FinishCampaignBattle(); }
                        break;
                    }
                    if (!bCampaignBattle)
                    {
                        // A test battle (from the start menu or a launcher): the way back is the campaign's start menu.
                        UGameplayStatics::SetGamePaused(this, false);
                        UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
                        UGameplayStatics::OpenLevel(this, FName(TEXT("Campaign1851")));
                    }
                }
                break;
            case EAction::OOBToggle:
                bOOBOpen = !bOOBOpen;
                break;
            case EAction::Pontoon:
                for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
                {
                    It->OrderPontoonBridge(Unit);
                    break;
                }
                break;
            case EAction::FireDrill:
                if (Unit && Unit->FireDrillComponent)
                {
                    Unit->FireDrillComponent->SetDrillMode(EStrategyFireDrillMode(B.Value));   // refused while not drilled
                }
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
                if (Unit) { Unit->bOfficerAIEnabled = B.Value == 0; }
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
            case EAction::Disengage:
                if (PC)
                {
                    PC->CancelOrderPlacement();
                    PC->IssueOrderToSelection(EStrategyOrderType::Disengage, FVector::ZeroVector, 0.f, false);
                }
                break;
            case EAction::Stop:
                if (PC) { PC->IssueHoldToSelection(); }
                break;
            case EAction::Stance:
                if (PC)
                {
                    for (AStrategyUnit* Selected : PC->GetSelectedUnits())
                    {
                        if (IsValid(Selected) && Selected->StanceComponent) { Selected->StanceComponent->SetStance(EStrategyStance(B.Value)); }
                    }
                }
                break;
            case EAction::Dismount:
                if (PC)
                {
                    for (AStrategyUnit* Selected : PC->GetSelectedUnits())
                    {
                        if (ACavalryUnit* Horse = Cast<ACavalryUnit>(Selected))
                        {
                            if (Horse->DragoonComponent) { if (B.Value == 1) { Horse->DragoonComponent->DismountAtCurrentPosition(); } else { Horse->DragoonComponent->RequestRemount(); } }
                        }
                    }
                }
                break;
            case EAction::FirePolicy:
                if (Unit && Unit->FireControlComponent) { Unit->FireControlComponent->SetFirePolicy(EStrategyFirePolicy(B.Value)); }
                break;
            case EAction::Formation:
                if (Unit && Unit->FormationComponent)
                {
                    if (Unit->ThreatReactionComponent)
                    {
                        Unit->ThreatReactionComponent->NotifyPlayerFormationOrder(EStrategyFormationType(B.Value));
                    }
                    Unit->FormationComponent->SetFormation(EStrategyFormationType(B.Value));
                }
                break;
            default:
                break;
        }
        return true;
    }
    return IsOverPanel(P);
}

// ------------------------------------------------------------------ objectives

void AStrategyHUD::DrawObjectiveMarkers()
{
    // The battle's objectives as flags over the field: name, worth, owner, and the capture's progress.
    for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
    {
        for (const AStrategyOOBTestScenario::FBattleObjective& O : It->GetObjectives())
        {
            const FVector Screen = Project(O.Location + FVector(0.0f, 0.0f, 2200.0f));
            if (Screen.Z <= 0.0f || Screen.X < -100.0f || Screen.X > Canvas->ClipX + 100.0f || Screen.Y < -50.0f || Screen.Y > Canvas->ClipY + 50.0f)
            {
                continue;
            }
            const FLinearColor Colour = O.Owner == 1 ? FLinearColor(0.30f, 0.50f, 0.95f) : O.Owner == 2 ? FLinearColor(0.88f, 0.30f, 0.25f) : FLinearColor(0.85f, 0.80f, 0.60f);
            const float Size = 11.0f;
            DrawLine(Screen.X, Screen.Y - Size, Screen.X + Size, Screen.Y, Colour, 2.0f);
            DrawLine(Screen.X + Size, Screen.Y, Screen.X, Screen.Y + Size, Colour, 2.0f);
            DrawLine(Screen.X, Screen.Y + Size, Screen.X - Size, Screen.Y, Colour, 2.0f);
            DrawLine(Screen.X - Size, Screen.Y, Screen.X, Screen.Y - Size, Colour, 2.0f);
            const FString Label = FString::Printf(TEXT("%s  (%d)"), *O.Name, O.Points);
            float TW = 0.0f, TH = 0.0f;
            GetTextSize(Label, TW, TH, nullptr, 1.0f);
            DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), Screen.X - TW * 0.5f - 6.0f, Screen.Y + Size + 4.0f, TW + 12.0f, TH + 14.0f);
            DrawText(Label, Colour, Screen.X - TW * 0.5f, Screen.Y + Size + 8.0f, nullptr, 1.0f, false);
            // The capture: the bar fills towards the side that is taking it.
            const float BarW = 90.0f, BarY = Screen.Y + Size + TH + 20.0f;
            DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), Screen.X - BarW * 0.5f, BarY, BarW, 6.0f);
            const float Fill = FMath::Abs(O.Progress) * BarW * 0.5f;
            const FLinearColor Taker = O.Progress < 0.0f ? FLinearColor(0.30f, 0.50f, 0.95f) : FLinearColor(0.88f, 0.30f, 0.25f);
            DrawRect(Taker, O.Progress < 0.0f ? Screen.X - Fill : Screen.X, BarY, Fill, 6.0f);
            DrawRect(FLinearColor(0.9f, 0.9f, 0.9f, 0.8f), Screen.X - 1.0f, BarY - 1.0f, 2.0f, 8.0f);
        }
        break;
    }
}

// ------------------------------------------------------------------ notices and orders on their way

void AStrategyHUD::AddNotice(const FString& Text)
{
    Notices.Add(TPair<FString, float>(Text, GetWorld() ? GetWorld()->GetTimeSeconds() + 9.0f : 9.0f));
    if (Notices.Num() > 5) { Notices.RemoveAt(0); }
}

// Mouse over a unit: its name, the men left out of the men it started with, and its morale.
void AStrategyHUD::DrawUnitHover()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !Canvas) { return; }
    float MX = 0.f, MY = 0.f;
    if (!PC->GetMousePosition(MX, MY)) { return; }
    const FVector2D Mouse(MX, MY);
    for (const FBox2D& Panel : Panels)
    {
        if (Panel.IsInside(Mouse)) { return; }
    }
    const AStrategyUnit* Best = nullptr;
    float BestDist = 70.f;
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        const AStrategyUnit* Unit = *It;
        if (!IsValid(Unit) || Unit->InitialStrength <= 0 || IsCommandHQ(Unit)) { continue; }
        FVector2D Screen;
        if (!PC->ProjectWorldLocationToScreen(Unit->GetActorLocation() + FVector(0.f, 0.f, 150.f), Screen)) { continue; }
        const float Dist = FVector2D::Distance(Screen, Mouse);
        if (Dist < BestDist) { BestDist = Dist; Best = Unit; }
    }
    if (!Best) { return; }
    const int32 Start = Best->InitialStrength, Now = Best->CurrentStrength;
    const FString Name = Best->DisplayName.ToString();
    const FString Line1 = FString::Printf(TEXT("%s%s"), *Name, Best->Side == EStrategySide::Denmark ? TEXT("") : TEXT("  (fjende)"));
    const FString Line2 = FString::Printf(TEXT("Mand: %d af %d  (tab %d)"), Now, Start, Start - Now);
    const FString Line3 = FString::Printf(TEXT("Moral %.0f"), Best->Morale);
    const float W = 230.f, H = 66.f;
    const float X = FMath::Min(MX + 18.f, Canvas->ClipX - W - 6.f), Y = FMath::Min(MY + 18.f, Canvas->ClipY - H - 6.f);
    DrawRect(FLinearColor(0.02f, 0.03f, 0.05f, 0.92f), X, Y, W, H);
    Text(Line1, X + 8.f, Y + 6.f, Gold);
    Text(Line2, X + 8.f, Y + 24.f, Ink);
    Text(Line3, X + 8.f, Y + 42.f, Muted);
}

void AStrategyHUD::DrawNotices()
{
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    Notices.RemoveAll([Now](const TPair<FString, float>& N) { return N.Value < Now; });
    float Y = 44.0f;
    for (const TPair<FString, float>& N : Notices)
    {
        float TW = 0.0f, TH = 0.0f;
        GetTextSize(N.Key, TW, TH, nullptr, 1.0f);
        const float X = (Canvas->ClipX - TW) * 0.5f;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 8.0f, Y, TW + 16.0f, TH + 10.0f);
        DrawText(N.Key, FLinearColor(0.98f, 0.92f, 0.70f), X, Y + 5.0f, nullptr, 1.0f, false);
        Y += TH + 14.0f;
    }
    // The units whose order is still on its way: a tag with the time left.
    if (const AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetOwningPlayerController()))
    {
        for (const AStrategyPlayerController::FCourierInfo& C : PC->GetPendingCouriers())
        {
            const AStrategyUnit* Unit = C.Unit.Get();
            if (!IsValid(Unit)) { continue; }
            const FVector Screen = Project(Unit->GetVisualCentroid() + FVector(0.0f, 0.0f, 900.0f));
            if (Screen.Z <= 0.0f) { continue; }
            const int32 Left = int32(FMath::CeilToFloat(C.SecondsLeft));
            const FString Label = FString::Printf(TEXT("Ordre på vej  %d:%02d"), Left / 60, Left % 60);
            float TW = 0.0f, TH = 0.0f;
            GetTextSize(Label, TW, TH, nullptr, 0.9f);
            DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), Screen.X - TW * 0.5f - 5.0f, Screen.Y, TW + 10.0f, TH + 6.0f);
            DrawText(Label, FLinearColor(1.0f, 0.88f, 0.45f), Screen.X - TW * 0.5f, Screen.Y + 3.0f, nullptr, 0.9f, false);
        }
    }
}
