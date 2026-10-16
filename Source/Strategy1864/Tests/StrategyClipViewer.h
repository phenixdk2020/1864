#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyClipViewer.generated.h"

class USkeletalMeshComponent;
class UAnimSequence;

/** QA tool (-Strategy1864ClipViewer): one baked (crowd) figure per animation clip in a grid, each looping at its own length with its name above.
 *  It draws through the same crowd path as the battle figures, so what is seen here is what the baked men do. */
UCLASS()
class STRATEGY1864_API AStrategyClipViewer : public AActor
{
    GENERATED_BODY()

public:
    AStrategyClipViewer();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    struct FViewerFigure
    {
        TObjectPtr<USkeletalMeshComponent> Figure;
        TObjectPtr<UAnimSequence> Clip;
        FString Label;
    };
    TArray<FViewerFigure> Figures;
    TObjectPtr<USkeletalMesh> Mesh;
    bool bPlaced = false;
    static constexpr int32 ClipViewerColumns = 6;   // figures per row
    float ClipViewerSpacing = 600.f;                // cm between figures in a row (rows are 1.2x this)
};
