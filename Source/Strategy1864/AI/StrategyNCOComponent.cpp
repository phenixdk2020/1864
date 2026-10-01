#include "StrategyNCOComponent.h"

UStrategyNCOComponent::UStrategyNCOComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyNCOComponent::SetOfficerAvailable(bool bAvailable)
{
    bOfficerAvailable = bAvailable;
}

int32 UStrategyNCOComponent::ApplyNCOLoss(int32 RequestedLoss)
{
    if (RequestedLoss <= 0)
    {
        return 0;
    }

    const int32 Applied = FMath::Min(RequestedLoss, FMath::Max(0, NCOStrength));
    NCOStrength -= Applied;
    return Applied;
}

float UStrategyNCOComponent::GetCadreIntegrity() const
{
    const float StrengthFactor =
        FMath::Clamp(static_cast<float>(NCOStrength) / 12.0f, 0.0f, 1.0f);
    const float QualityFactor = FMath::Clamp(NCOQuality / 100.0f, 0.0f, 1.0f);
    return StrengthFactor * QualityFactor;
}

float UStrategyNCOComponent::GetLocalCommandContinuity() const
{
    if (bOfficerAvailable)
    {
        return 1.0f;
    }

    return FMath::Clamp(
        OfficerLossCommandFloor + GetCadreIntegrity() * 0.55f,
        OfficerLossCommandFloor,
        0.90f);
}

float UStrategyNCOComponent::GetFormationSpeedMultiplier() const
{
    return FMath::Lerp(0.55f, 1.05f, GetLocalCommandContinuity());
}

float UStrategyNCOComponent::GetReloadDisciplineMultiplier() const
{
    return FMath::Lerp(1.35f, 0.92f, GetLocalCommandContinuity());
}

float UStrategyNCOComponent::GetRallyMultiplier() const
{
    return FMath::Lerp(0.45f, 1.15f, GetLocalCommandContinuity());
}

float UStrategyNCOComponent::GetResponseTimeMultiplier() const
{
    return FMath::Lerp(1.75f, 0.90f, GetLocalCommandContinuity());
}

float UStrategyNCOComponent::GetDetachmentControlMultiplier() const
{
    return FMath::Lerp(0.50f, 1.10f, GetLocalCommandContinuity());
}

bool UStrategyNCOComponent::CanMaintainLocalCommand() const
{
    return bOfficerAvailable || (NCOStrength > 0 && GetLocalCommandContinuity() >= 0.40f);
}
