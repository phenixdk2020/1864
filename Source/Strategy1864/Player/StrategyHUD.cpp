#include "StrategyHUD.h"
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
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864NoHud")))
    {
        return;   // clean screenshots (the start menu's background)
    }
    if (FigureDivisor < 0)
    {
        FigureDivisor = Strategy1864BattleQuality::GetFigureDivisor();
        BattleQualityPreset = Strategy1864BattleQuality::GetPreset();
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
            const int32 Speeds[] = { -1, 1, 2, 3, 5 };   // -1 is half speed
            for (int32 i = 0; i < 5; ++i)
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
    if (!bSelected && !bRouteActive) return;
    const FLinearColor RouteColour = bSelected ? Gold : FLinearColor(1.0f, 1.0f, 1.0f, 0.22f);
    const float RouteYaw = RouteOrder.bHasFacing ? RouteOrder.FacingYaw : Unit->GetActorRotation().Yaw;
    const FVector RouteGoal = bRouteActive ? RouteOrder.TargetLocation : Unit->GetActorLocation();
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
        RouteHalfWidth = Unit->FormationComponent->EstimateFrontageCm(Unit->CurrentStrength) * 0.5f;
        const TArray<FStrategyFormationSlot> RouteSlots = Unit->FormationComponent->GenerateSoldierSlots(FVector::ZeroVector, 0.0f, Unit->CurrentStrength);
        FBox RouteBounds(ForceInit);
        for (const FStrategyFormationSlot& RouteSlot : RouteSlots) RouteBounds += RouteSlot.WorldLocation;
        if (RouteBounds.IsValid) RouteHalfDepth = FMath::Max(Unit->FormationComponent->SoldierRankSpacingCm, RouteBounds.GetSize().X) * 0.5f;
    }
    const FVector RouteFront = RouteForward * RouteHalfDepth, RouteSide = RouteRight * RouteHalfWidth;
    RouteLine({ RouteGoal + RouteFront + RouteSide, RouteGoal + RouteFront - RouteSide,
                RouteGoal - RouteFront - RouteSide, RouteGoal - RouteFront + RouteSide, RouteGoal + RouteFront + RouteSide });
    const FVector RouteArrowBase = RouteGoal + RouteFront;
    const FVector RouteArrowTip = RouteArrowBase + RouteForward * 700.0f;
    RouteLine({ RouteArrowBase, RouteArrowTip });
    RouteLine({ RouteArrowTip - RouteForward * 250.0f + RouteRight * 180.0f, RouteArrowTip,
                RouteArrowTip - RouteForward * 250.0f - RouteRight * 180.0f });
    if (!bRouteActive) return;
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
        const FVector Behind = Project(OnGround((Left + Right) * 0.5f - Forward * 650.0f), false);   // a few metres behind the rear rank
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
            const TCHAR* TagStatus = bTagReforming ? TEXT("FORMERER") : bTagFiring ? TEXT("SKYDER") :
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
    const float X = 342.0f, Y = 32.0f, W = 420.0f, H = 340.0f;
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
    const float X = Canvas->ClipX - W - 8.0f, Y = Canvas->ClipY - 128.0f - H - 36.0f;
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
    const FString Title = Unit->DisplayName.ToString().ToUpper().Replace(TEXT("æ"), TEXT("Æ")).Replace(TEXT("ø"), TEXT("Ø")).Replace(TEXT("å"), TEXT("Å"));
    Text(FString::Printf(TEXT("%s | %s"), *Title, RoleText), 10.0f, Y + 6.0f, Gold);

    Text(OrderLabel(Unit), W - 300.0f, Y + 6.0f, Muted, 0.85f);

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
    // What the officer thinks (his last decision and why).
    if (Unit->bOfficerAIEnabled && Unit->AITelemetryComponent && !Unit->AITelemetryComponent->CurrentTask.IsEmpty())
    {
        Text(FString::Printf(TEXT("OFFICEREN: %s"), *Unit->AITelemetryComponent->CurrentTask.ToUpper()).Left(40), LX, LY + 72.0f, Gold, 0.8f);
        Text(Unit->AITelemetryComponent->ReasonCode.Left(44), LX, LY + 86.0f, Ink, 0.8f);
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
        // The pioneers (Pontonnerkorpset researched): a pontoon bridge over the broad river nearest the staff.
        for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
        {
            if (It->CanLayPontoonBridges())
            {
                const float Left = It->GetPontoonSecondsLeft();
                DrawButton(MX + 546.0f, MY + 18.0f, 176.0f, 26.0f,
                    Left > 0.0f ? FString::Printf(TEXT("PONTONBRO %d:%02d"), int32(Left) / 60, int32(Left) % 60) : FString(TEXT("SLÅ PONTONBRO")),
                    EAction::Pontoon, 0, Left > 0.0f, Unit, Left > 0.0f ? &ExecutingBlue : nullptr);
            }
            break;
        }
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
        // The fire method: what the regiment has researched and drilled (the locked ones dimmed).
        if (UStrategyFireDrillComponent* Drill = Cast<AStrategyCompanyUnit>(Unit) ? Unit->FireDrillComponent.Get() : nullptr)
        {
            const TCHAR* Labels[] = { TEXT("1.GLD"), TEXT("2.GLD"), TEXT("GELED"), TEXT("SALVE"), TEXT("FRI") };
            const EStrategyFireDrillMode Modes[] = { EStrategyFireDrillMode::FrontRank, EStrategyFireDrillMode::TwoRankFire, EStrategyFireDrillMode::FireByRank,
                EStrategyFireDrillMode::Volley, EStrategyFireDrillMode::Independent };
            for (int32 i = 0; i < 5; ++i)
            {
                const bool bOpen = Drill->IsDrillModeUnlocked(Modes[i]);
                DrawButton(MX + i * 56.0f, MY + 66.0f, 52.0f, 24.0f, Labels[i], EAction::FireDrill, int32(Modes[i]), Drill->DrillMode == Modes[i], Unit,
                    bOpen ? nullptr : &ButtonDark);
            }
        }
        const float OX = MX + 280.0f;
        Text(TEXT("ORDRER / BEVÆGELSE"), OX, MY, Gold, 0.85f);
        DrawButton(OX, MY + 18.0f, 110.0f, 24.0f, TEXT("RYK FREM"), EAction::Order, int32(EStrategyOrderType::Advance), Current == EStrategyOrderType::Advance, Unit);
        DrawButton(OX + 116.0f, MY + 18.0f, 110.0f, 24.0f, TEXT("TILBAGE"), EAction::Order, int32(EStrategyOrderType::Withdraw), Current == EStrategyOrderType::Withdraw, Unit);
        DrawButton(OX + 232.0f, MY + 18.0f, 110.0f, 24.0f, TEXT("CHARGE"), EAction::Charge, 0, Current == EStrategyOrderType::Charge, Unit);
        DrawButton(OX + 348.0f, MY + 18.0f, 80.0f, 24.0f, TEXT("STOP"), EAction::Stop, 0, Current == EStrategyOrderType::Hold, Unit);
        // The stance (stand, kneel, lie down: a lying unit is harder to hit but loads slower and moves slowly), and for dragoons
        // dismounting to fight on foot and mounting again.
        if (Cast<AStrategyCompanyUnit>(Unit) && Unit->StanceComponent)
        {
            Text(TEXT("STILLING"), OX + 348.0f, MY + 50.0f, Gold, 0.85f);
            const TCHAR* StanceLabels[] = { TEXT("STÅ"), TEXT("KNÆ"), TEXT("LIG") };
            const EStrategyStance Stances[] = { EStrategyStance::Standing, EStrategyStance::Kneeling, EStrategyStance::Prone };
            for (int32 i = 0; i < 3; ++i)
            {
                DrawButton(OX + 348.0f + i * 58.0f, MY + 66.0f, 54.0f, 24.0f, StanceLabels[i], EAction::Stance, int32(Stances[i]), Unit->StanceComponent->Stance == Stances[i], Unit);
            }
        }
        else if (const ACavalryUnit* Horse = Cast<ACavalryUnit>(Unit))
        {
            if (Horse->DragoonComponent && Horse->DragoonComponent->Role == EStrategyCavalryRole::Dragoon)
            {
                const bool bFoot = Horse->DragoonComponent->MountedState != EStrategyMountedState::Mounted;
                Text(TEXT("DRAGONER"), OX + 348.0f, MY + 50.0f, Gold, 0.85f);
                DrawButton(OX + 348.0f, MY + 66.0f, 84.0f, 24.0f, TEXT("SIT AF"), EAction::Dismount, 1, bFoot, Unit);
                DrawButton(OX + 438.0f, MY + 66.0f, 84.0f, 24.0f, TEXT("STIG PÅ"), EAction::Dismount, 0, !bFoot, Unit);
            }
        }
        if (UStrategyFormationComponent* Formation = Unit->FormationComponent)
        {
            Text(TEXT("FORMATION"), OX, MY + 50.0f, Gold, 0.85f);
            const TCHAR* Labels[] = { TEXT("LINIE"), TEXT("KOLONNE"), TEXT("KARRÉ") };
            const EStrategyFormationType Types[] = { EStrategyFormationType::Line, EStrategyFormationType::MarchColumn, EStrategyFormationType::Square };
            // Horse cannot form square: line and column only.
            const int32 FormationButtons = Cast<ACavalryUnit>(Unit) ? 2 : 3;
            for (int32 i = 0; i < FormationButtons; ++i)
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
            case EAction::BattleQuality:
                BattleQualityPreset = B.Value;   // the graphics preset; the number of figures is its own setting
                Strategy1864BattleQuality::ApplyPreset(GetWorld(), B.Value, true);
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
