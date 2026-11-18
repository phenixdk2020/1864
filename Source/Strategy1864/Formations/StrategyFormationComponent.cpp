#include "StrategyFormationComponent.h"
#include "../Units/StrategyUnit.h"
#include "../AI/StrategyFieldOfficerComponent.h"

namespace
{
    // A line that loses men keeps its depth (design paper 2026-10-16): a loss is replaced inside its own file by the man behind
    // (StepUpFiles), and when the ranks close sideways the line gets shorter, not thinner. So the rank count is always the nominal one
    // (2 or 3) and the front shrinks by whole files; only the last file may be short.
    int32 LineRanksFor(const AActor* /*Owner*/, int32 RankCount, int32 /*Strength*/)
    {
        return FMath::Max(1, RankCount);
    }
}

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

    if (AStrategyUnit* CoverFormationUnit = Cast<AStrategyUnit>(GetOwner()))
        if (CoverFormationUnit->FieldOfficerComponent) CoverFormationUnit->FieldOfficerComponent->LeaveAutomaticFireCover();
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
            EffectiveColumnWidth = 4;   // the cavalry marches four abreast
        }
        else if (CurrentFormation == EStrategyFormationType::DefileColumn)
        {
            EffectiveColumnWidth = 2;
        }

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
        // Each face as deep as the line (user 2026-11-05: "tre geledder"; it was one man deep and some 35 m a side): the front
        // rank outermost, the others one rank spacing further in, each a little shorter, so the faces meet at the corners.
        const int32 SideCount = 4;
        const int32 SquareRanks = LineRanksFor(GetOwner(), RankCount, Strength);
        const int32 PerSide = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Strength) / SideCount));
        const int32 FilesPerSide = FMath::Max(1, FMath::CeilToInt(static_cast<float>(PerSide) / SquareRanks));
        const float OuterHalf = FMath::Max(300.0f + (SquareRanks - 1) * SoldierRankSpacingCm, FilesPerSide * SoldierLateralSpacingCm * 0.5f);

        for (int32 Index = 0; Index < Strength; ++Index)
        {
            // Round-robin gives each side floor(N/4) or ceil(N/4) men; on a side they fill file by file, front rank first.
            const int32 Side = Index % SideCount;
            const int32 OnSide = Index / SideCount;
            const int32 MenOnSide = Strength / SideCount + (Side < Strength % SideCount ? 1 : 0);
            const int32 SquareRank = OnSide % SquareRanks;
            const int32 Along = OnSide / SquareRanks;
            const int32 SideFiles = FMath::Max(1, FMath::CeilToInt(static_cast<float>(MenOnSide) / SquareRanks));
            const float HalfExtent = OuterHalf - SquareRank * SoldierRankSpacingCm;
            // Each side runs from its own corner to one place short of the next: the four corners are held once. (Corner to
            // corner put two men on the same spot at every corner; QA 2026-11-05, Saved/QA/2026-11-05-karre.)
            const float Alpha =
                SideFiles <= 1
                ? 0.5f
                : static_cast<float>(Along) / static_cast<float>(SideFiles);
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

            // The square's front face is the line's front rank (the line's slots have their front rank at X = 0): the middle files
            // stand fast as the front, the wings fold back into the sides and the rear (user 2026-11-05, "karré-eksercits"; centred
            // on the line the whole company walked forward and piled up).
            LocalOffset -= Forward * OuterHalf;

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
        : (CurrentFormation == EStrategyFormationType::Line ? LineRanksFor(GetOwner(), RankCount, Strength) : FMath::Max(1, RankCount));
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
        return 2.0f * FMath::Max(300.0f, (PerSide - 1) * SoldierLateralSpacingCm * 0.5f);
    }

    const int32 EffectiveRanks =
        CurrentFormation == EStrategyFormationType::CavalryLine
        ? 4
        : (CurrentFormation == EStrategyFormationType::Line ? LineRanksFor(GetOwner(), RankCount, Strength) : FMath::Max(1, RankCount));
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
