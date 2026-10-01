#include "StrategyFormationComponent.h"

UStrategyFormationComponent::UStrategyFormationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyFormationComponent::SetFormation(EStrategyFormationType NewFormation)
{
    if (CurrentFormation == NewFormation)
    {
        return;
    }

    const EStrategyFormationType OldFormation = CurrentFormation;
    CurrentFormation = NewFormation;
    OnFormationChanged.Broadcast(OldFormation, CurrentFormation);
}

TArray<FStrategyFormationSlot> UStrategyFormationComponent::GenerateSoldierSlots(
    const FVector& FormationCenter,
    float FacingYaw,
    int32 Strength) const
{
    TArray<FStrategyFormationSlot> Result;
    if (Strength <= 0)
    {
        return Result;
    }

    Result.Reserve(Strength);

    const FRotator FacingRotation(0.0f, FacingYaw, 0.0f);
    const FVector Forward = FacingRotation.Vector();
    const FVector Right = FRotationMatrix(FacingRotation).GetScaledAxis(EAxis::Y);

    if (CurrentFormation == EStrategyFormationType::MarchColumn ||
        CurrentFormation == EStrategyFormationType::CavalryColumn ||
        CurrentFormation == EStrategyFormationType::DefileColumn)
    {
        int32 EffectiveColumnWidth = FMath::Max(1, ColumnWidth);

        if (CurrentFormation == EStrategyFormationType::CavalryColumn)
        {
            EffectiveColumnWidth = 4;
        }
        else if (CurrentFormation == EStrategyFormationType::DefileColumn)
        {
            EffectiveColumnWidth = 2;
        }
        const int32 RowCount = FMath::CeilToInt(static_cast<float>(Strength) / EffectiveColumnWidth);

        for (int32 Index = 0; Index < Strength; ++Index)
        {
            const int32 Column = Index % EffectiveColumnWidth;
            const int32 Row = Index / EffectiveColumnWidth;

            const float Lateral =
                (static_cast<float>(Column) - (EffectiveColumnWidth - 1) * 0.5f) * SoldierLateralSpacingCm;
            const float Longitudinal =
                -static_cast<float>(Row) * SoldierRankSpacingCm;

            FStrategyFormationSlot Slot;
            Slot.WorldLocation = FormationCenter + Right * Lateral + Forward * Longitudinal;
            Slot.FacingYaw = FacingYaw;
            Slot.SlotIndex = Index;
            Result.Add(Slot);
        }

        return Result;
    }

    if (CurrentFormation == EStrategyFormationType::Square)
    {
        const int32 SideCount = 4;
        const int32 PerSide = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Strength) / SideCount));
        const float HalfExtent = FMath::Max(300.0f, (PerSide - 1) * SoldierLateralSpacingCm * 0.5f);

        for (int32 Index = 0; Index < Strength; ++Index)
        {
            const int32 Side = FMath::Min(3, Index / PerSide);
            const int32 Along = Index % PerSide;
            const float Alpha =
                PerSide <= 1
                ? 0.5f
                : static_cast<float>(Along) / static_cast<float>(PerSide - 1);
            const float Offset = FMath::Lerp(-HalfExtent, HalfExtent, Alpha);

            FVector LocalOffset = FVector::ZeroVector;
            float SlotYaw = FacingYaw;

            switch (Side)
            {
                case 0:
                    LocalOffset = Forward * HalfExtent + Right * Offset;
                    SlotYaw = FacingYaw;
                    break;

                case 1:
                    LocalOffset = Right * HalfExtent - Forward * Offset;
                    SlotYaw = FacingYaw + 90.0f;
                    break;

                case 2:
                    LocalOffset = -Forward * HalfExtent - Right * Offset;
                    SlotYaw = FacingYaw + 180.0f;
                    break;

                case 3:
                default:
                    LocalOffset = -Right * HalfExtent + Forward * Offset;
                    SlotYaw = FacingYaw - 90.0f;
                    break;
            }

            FStrategyFormationSlot Slot;
            Slot.WorldLocation = FormationCenter + LocalOffset;
            Slot.FacingYaw = SlotYaw;
            Slot.SlotIndex = Index;
            Result.Add(Slot);
        }

        return Result;
    }

    const int32 EffectiveRanks =
        CurrentFormation == EStrategyFormationType::CavalryLine
        ? 4
        : FMath::Max(1, RankCount);
    const int32 Files = FMath::CeilToInt(static_cast<float>(Strength) / EffectiveRanks);

    for (int32 Index = 0; Index < Strength; ++Index)
    {
        const int32 Rank = Index % EffectiveRanks;
        const int32 File = Index / EffectiveRanks;

        const float Lateral =
            (static_cast<float>(File) - (Files - 1) * 0.5f) * SoldierLateralSpacingCm;
        const float Longitudinal =
            -static_cast<float>(Rank) * SoldierRankSpacingCm;

        FStrategyFormationSlot Slot;
        Slot.WorldLocation = FormationCenter + Right * Lateral + Forward * Longitudinal;
        Slot.FacingYaw = FacingYaw;
        Slot.SlotIndex = Index;
        Result.Add(Slot);
    }

    return Result;
}

float UStrategyFormationComponent::EstimateFrontageCm(int32 Strength) const
{
    if (Strength <= 0)
    {
        return 0.0f;
    }

    if (CurrentFormation == EStrategyFormationType::MarchColumn ||
        CurrentFormation == EStrategyFormationType::CavalryColumn ||
        CurrentFormation == EStrategyFormationType::DefileColumn)
    {
        int32 Width = FMath::Max(1, ColumnWidth);
        if (CurrentFormation == EStrategyFormationType::CavalryColumn)
        {
            Width = 4;
        }
        else if (CurrentFormation == EStrategyFormationType::DefileColumn)
        {
            Width = 2;
        }
        return Width * SoldierLateralSpacingCm;
    }

    if (CurrentFormation == EStrategyFormationType::Square)
    {
        const int32 PerSide = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Strength) / 4.0f));
        return FMath::Max(0, PerSide - 1) * SoldierLateralSpacingCm;
    }

    const int32 EffectiveRanks =
        CurrentFormation == EStrategyFormationType::CavalryLine
        ? 4
        : FMath::Max(1, RankCount);
    const int32 Files = FMath::CeilToInt(static_cast<float>(Strength) / EffectiveRanks);
    return FMath::Max(0, Files - 1) * SoldierLateralSpacingCm;
}


bool UStrategyFormationComponent::IsFullCompanyFrontageWithinBaseline(
    int32 Strength) const
{
    const float FrontageCm = EstimateFrontageCm(Strength);
    return FMath::Abs(FrontageCm - FullCompanyTargetFrontageCm) <=
        FullCompanyFrontageToleranceCm;
}
