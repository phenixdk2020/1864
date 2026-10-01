#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyPlayerController.generated.h"

class AStrategyHUD;
class AStrategyUnit;

UCLASS()
class STRATEGY1864_API AStrategyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AStrategyPlayerController();

    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category="Strategy|Selection")
    void ClearSelection();

    UFUNCTION(BlueprintPure, Category="Strategy|Selection")
    TArray<AStrategyUnit*> GetSelectedUnits() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|OOB")
    void SelectUnitFromOOB(AStrategyUnit* Unit, bool bFocusCamera);

    UFUNCTION(BlueprintCallable, Category="Strategy|OOB")
    bool RequestTacticalAttachment(AStrategyUnit* Unit, AStrategyUnit* NewParent);

    UFUNCTION(BlueprintCallable, Category="Strategy|OOB")
    bool RestoreOrganicAttachment(AStrategyUnit* Unit);

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    void BeginOrderPlacement(EStrategyOrderType OrderType);

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    void CancelOrderPlacement();

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    bool CommitPendingOrderUnderCursor();

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    bool IssueOrderToSelection(
        EStrategyOrderType OrderType,
        const FVector& TargetLocation,
        float FacingYaw,
        bool bHasFacing);

    UFUNCTION(BlueprintCallable, Category="Strategy|Orders")
    void IssueHoldToSelection();

    UFUNCTION(BlueprintPure, Category="Strategy|Orders")
    bool HasPendingOrderPlacement() const { return bOrderPlacementPending; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Selection")
    float BoxSelectionThresholdPixels = 6.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Time")
    float SimulationSpeed = 1.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Time")
    void TogglePauseSimulation();

    UFUNCTION(BlueprintCallable, Category="Strategy|Time")
    void SetSimulationSpeed(float NewSpeed);

private:
    void SelectionPressed();
    void SelectionReleased();
    void SetSpeed1x();
    void SetSpeed2x();
    void SetSpeed3x();
    void ResetQAScenario();
    void ToggleSelectedOfficerAI();
    void SetEnemyDifficultyEasy();
    void SetEnemyDifficultyNormal();
    void SetEnemyDifficultyHard();
    void SetEnemyDifficultyByValue(uint8 DifficultyValue);
    void ToggleFollowLatestProjectile();
    void ToggleProjectileTrajectoryDebug();
    void SelectSingleUnderCursor();
    void SelectUnitsInScreenRectangle(const FVector2D& Start, const FVector2D& End);
    void ApplySelection(AStrategyUnit* Unit, bool bAdd, bool bRemove);
    void ReadModifierState(bool& bOutAdd, bool& bOutRemove) const;
    AStrategyHUD* GetStrategyHUD() const;
    bool ResolveGroundPointUnderCursor(FVector& OutWorldPoint) const;

    bool bOrderPlacementPending = false;
    bool bOrderFacingDragActive = false;
    EStrategyOrderType PendingOrderType = EStrategyOrderType::None;
    FVector PendingOrderTarget = FVector::ZeroVector;
    bool bSelectionInputDown = false;
    FVector2D SelectionStart = FVector2D::ZeroVector;
    FVector2D SelectionCurrent = FVector2D::ZeroVector;

    UPROPERTY()
    TArray<TObjectPtr<AStrategyUnit>> SelectedUnitObjects;
};
