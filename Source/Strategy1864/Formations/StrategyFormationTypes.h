#pragma once

#include "CoreMinimal.h"
#include "StrategyFormationTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategyFormationType : uint8
{
    Line,
    MarchColumn,
    Square,
    CavalryLine,
    CavalryColumn,
    DefileColumn
};

USTRUCT(BlueprintType)
struct FStrategyFormationSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float FacingYaw = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    int32 SlotIndex = INDEX_NONE;
};
