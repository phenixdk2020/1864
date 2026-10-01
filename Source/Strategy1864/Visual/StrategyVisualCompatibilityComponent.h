#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyHumanVisualTypes.h"
#include "StrategyVisualCompatibilityComponent.generated.h"

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyVisualCompatibilityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyVisualCompatibilityComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Compatibility")
    FName RequiredSkeletonId = TEXT("SK_Human_1864");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Compatibility")
    FName RequiredAnimationSetId = TEXT("ABP_Human_1864");

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Compatibility")
    bool ValidateProfile(
        const FStrategyHumanVisualProfile& Profile,
        FString& OutFailureReason) const;
};
