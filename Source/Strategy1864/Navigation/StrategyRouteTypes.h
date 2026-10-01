#pragma once

#include "CoreMinimal.h"
#include "StrategyRouteTypes.generated.h"

class AStrategyRiverBarrier;

USTRUCT(BlueprintType)
struct FStrategyRoutePlan
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    bool bValid = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    FString FailureReason;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    TArray<FVector> Points;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    bool bUsesBridge = false;

    UPROPERTY()
    TObjectPtr<AStrategyRiverBarrier> BridgeBarrier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    int32 BridgeEnterPointIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    int32 BridgeExitPointIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    bool bSameBankDetour = false;
};
