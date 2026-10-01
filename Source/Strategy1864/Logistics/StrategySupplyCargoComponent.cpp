#include "StrategySupplyCargoComponent.h"

UStrategySupplyCargoComponent::UStrategySupplyCargoComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

int32 UStrategySupplyCargoComponent::ConsumeSmallArmsRounds(int32 Requested)
{
    const int32 Consumed =
        FMath::Min(
            FMath::Max(0, Requested),
            FMath::Max(0, SmallArmsRounds));

    SmallArmsRounds -= Consumed;
    return Consumed;
}

int32 UStrategySupplyCargoComponent::ConsumeArtilleryRounds(int32 Requested)
{
    const int32 Consumed =
        FMath::Min(
            FMath::Max(0, Requested),
            FMath::Max(0, ArtilleryRounds));

    ArtilleryRounds -= Consumed;
    return Consumed;
}

void UStrategySupplyCargoComponent::ApplyCargoLossFraction(float Fraction)
{
    const float Clamped = FMath::Clamp(Fraction, 0.0f, 1.0f);

    SmallArmsRounds =
        FMath::Max(
            0,
            FMath::RoundToInt(
                static_cast<float>(SmallArmsRounds) * (1.0f - Clamped)));

    ArtilleryRounds =
        FMath::Max(
            0,
            FMath::RoundToInt(
                static_cast<float>(ArtilleryRounds) * (1.0f - Clamped)));

    FoodUnits =
        FMath::Max(
            0,
            FMath::RoundToInt(
                static_cast<float>(FoodUnits) * (1.0f - Clamped)));

    MedicalUnits =
        FMath::Max(
            0,
            FMath::RoundToInt(
                static_cast<float>(MedicalUnits) * (1.0f - Clamped)));

    CargoIntegrity =
        FMath::Clamp(
            CargoIntegrity - Clamped * 100.0f,
            0.0f,
            100.0f);
}
