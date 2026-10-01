#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "StrategyHUD.generated.h"

UCLASS()
class STRATEGY1864_API AStrategyHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    void BeginSelectionBox(const FVector2D& ScreenPoint);
    void UpdateSelectionBox(const FVector2D& ScreenPoint);
    void EndSelectionBox();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Selection")
    FLinearColor SelectionFillColor = FLinearColor(0.12f, 0.42f, 1.0f, 0.16f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Selection")
    FLinearColor SelectionBorderColor = FLinearColor(0.25f, 0.65f, 1.0f, 0.95f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    bool bDrawQABuildMarker = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    bool bDrawSelectedUnitQA = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    FString BuildMarker = TEXT("PROJECT 1864 | UNREAL PORT | v00.02.79-dev");

private:
    bool bSelectionBoxActive = false;
    FVector2D SelectionStart = FVector2D::ZeroVector;
    FVector2D SelectionEnd = FVector2D::ZeroVector;
};
