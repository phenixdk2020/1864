#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyAIDifficultyComponent.generated.h"

UENUM(BlueprintType)
enum class EStrategyAIDifficulty : uint8
{
    Easy,
    Normal,
    Hard
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyAIDifficultyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyAIDifficultyComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|AI Difficulty")
    EStrategyAIDifficulty Difficulty = EStrategyAIDifficulty::Normal;

    UFUNCTION(BlueprintPure, Category="Strategy|AI Difficulty")
    float GetReactionTimeMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|AI Difficulty")
    float GetDecisionNoiseAmplitude() const;
};
