#include "StrategyAIDifficultyComponent.h"

UStrategyAIDifficultyComponent::UStrategyAIDifficultyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

float UStrategyAIDifficultyComponent::GetReactionTimeMultiplier() const
{
    switch (Difficulty)
    {
        case EStrategyAIDifficulty::Easy:
            return 1.40f;
        case EStrategyAIDifficulty::Hard:
            return 0.75f;
        case EStrategyAIDifficulty::Normal:
        default:
            return 1.0f;
    }
}

float UStrategyAIDifficultyComponent::GetDecisionNoiseAmplitude() const
{
    switch (Difficulty)
    {
        case EStrategyAIDifficulty::Easy:
            return 0.15f;
        case EStrategyAIDifficulty::Hard:
            return 0.03f;
        case EStrategyAIDifficulty::Normal:
        default:
            return 0.08f;
    }
}
