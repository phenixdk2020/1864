#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "StrategyHUD.generated.h"

class AStrategyUnit;

/**
 * The battle's screen (as the Unity prototype F30X): the order of battle top left (a tree that folds, a
 * click selects a unit, a double click puts the camera on it) and the command panel at the bottom for the
 * selected unit (an HQ: AI, doctrine and the six orders; a company: fire policy, formation, charge and
 * stop; the subordinates' status on the right). Clicks on the panels do not reach the battlefield.
 */
UCLASS()
class STRATEGY1864_API AStrategyHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    void BeginSelectionBox(const FVector2D& ScreenPoint);
    void UpdateSelectionBox(const FVector2D& ScreenPoint);
    void EndSelectionBox();

    /** A click on a panel: carried out here; true when it was on the screen's panels (not the battlefield). */
    bool HandleScroll(const FVector2D& ScreenPoint, float Delta);

    bool HandleClick(const FVector2D& ScreenPoint);

    /** Is the point over one of the panels. */
    bool IsOverPanel(const FVector2D& ScreenPoint) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Selection")
    FLinearColor SelectionFillColor = FLinearColor(0.12f, 0.42f, 1.0f, 0.16f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Selection")
    FLinearColor SelectionBorderColor = FLinearColor(0.25f, 0.65f, 1.0f, 0.95f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    bool bDrawQABuildMarker = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    FString BuildMarker = TEXT("PROJECT 1864 | GAME1864 | SLAG v00.02.80-dev");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|UI")
    bool bOOBOpen = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|UI")
    bool bSettingsOpen = false;
    /** The enemy's fire cones (normally hidden; for testing the march and the deployment). Saved with the settings. */
    static bool ShowEnemyRange();
    static void SetShowEnemyRange(bool bShow);

private:
    enum class EAction : uint8
    {
        None, Minimap, FinishBattle, SettingsToggle, CameraSpeed, OOBToggle, OOBRow, OOBFold, AIToggle, Doctrine, Order, FirePolicy, Formation, Charge, Stop, FireDrill, Pontoon, EnemyRange, EnemyPosture, EnemyFire, Couriers, Stance, Dismount, FigureScale, BattleQuality, TimeControl, Shadows
    };

    struct FButton
    {
        FBox2D Box = FBox2D(ForceInit);
        EAction Action = EAction::None;
        int32 Value = 0;
        TWeakObjectPtr<AStrategyUnit> Unit;
    };

    void DrawOOB();
    void DrawOOBRow(AStrategyUnit* Unit, int32 Depth, float& Y, int32 Guard);
    float CommandHeight() const;
    void DrawRounded(float X, float Y, float W, float H, const FLinearColor& Fill);
    void DrawHeading(const FString& Label, float X, float Y, float W);
    void DrawStatBar(const FString& Label, const FString& Value, float Fraction, float X, float Y, float W);
    bool bCommandStyle = false;
    FBox2D SubordinateRect = FBox2D(ForceInit);
    TWeakObjectPtr<AStrategyUnit> ScrollUnit;
    int32 SubordinateOffset = 0;
    int32 SubordinateMaxOffset = 0;
    void DrawCommandPanel(AStrategyUnit* Unit);
    void DrawMinimap();
    void DrawObjectiveMarkers();
    int32 FigureDivisor = -1;
    int32 BattleQualityPreset = -1;
    void DrawNotices();
    void DrawUnitHover();
    TArray<TPair<FString, float>> Notices;
    void DrawSettings();
    /** The fire cone of a unit on the ground (as the QA design): from the formation's front corners, the
     *  sides dashed at the half angle, the close, medium and long ranges as dashed arcs following the front,
     *  the chosen range strong and orange with the band up to it filled; the labels in metres and degrees. */
    void DrawMovementRoute(const AStrategyUnit* Unit, bool bSelected);
    void DrawFireCone(const AStrategyUnit* Unit, bool bWithLegend);
    void DashedPolyline(const TArray<FVector>& WorldPoints, const FLinearColor& Colour, float Thickness, float Dash, float Gap);
    void DrawButton(float X, float Y, float W, float H, const FString& Label, EAction Action, int32 Value, bool bActive,
        AStrategyUnit* Unit = nullptr, const FLinearColor* Colour = nullptr);
    void DrawPanel(float X, float Y, float W, float H);
public:
    /** A short message to the player (shown some seconds under the score panel). */
    void AddNotice(const FString& Text);
private:
    void Text(const FString& S, float X, float Y, const FLinearColor& Colour, float Scale = 1.0f);

    TArray<FButton> Buttons;
    TArray<FBox2D> Panels;
    TSet<TWeakObjectPtr<AStrategyUnit>> Folded;
    TWeakObjectPtr<AStrategyUnit> LastClickedUnit;
    double LastClickTime = 0.0;
    FBox2D MinimapRect = FBox2D(ForceInit);
    FVector2D MinimapCentre = FVector2D::ZeroVector;
    float MinimapHalfWidth = 30000.0f;

    bool bSelectionBoxActive = false;
    FVector2D SelectionStart = FVector2D::ZeroVector;
    FVector2D SelectionEnd = FVector2D::ZeroVector;
};
