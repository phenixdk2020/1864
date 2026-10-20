#pragma once

#include "StrategyVisualFormationPath.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Shared planar centreline; built once, sampled without world/path queries.
struct FStrategyColumnDrill
{
    using FSample = FStrategyVisualFormationPath::FSample;
    static bool Enabled()
    {
        int32 DrillSetting = 1;
        FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Drill="), DrillSetting);
        return DrillSetting != 0;
    }
    static float Radius(float DrillRankSpacing, float /*DrillFileSpacing*/)
    {
        return FMath::Max(300.f, 3.f * DrillRankSpacing);
    }
    static FVector Right(const FVector& DrillDirection) { return FVector(-DrillDirection.Y, DrillDirection.X, 0.f); }
    static float Turn(const FVector& DrillA, const FVector& DrillB)
    {
        return FMath::FindDeltaAngleDegrees(DrillA.Rotation().Yaw, DrillB.Rotation().Yaw);
    }
    static FVector ArcDelta(const FVector& DrillA, const FVector& DrillB, float DrillRadius)
    {
        const float DrillAngle = FMath::DegreesToRadians(Turn(DrillA, DrillB));
        const float DrillSign = DrillAngle < 0.f ? -1.f : 1.f;
        return DrillA * (DrillRadius * FMath::Sin(FMath::Abs(DrillAngle))) +
            Right(DrillA) * (DrillSign * DrillRadius * (1.f - FMath::Cos(DrillAngle)));
    }
    static TArray<FSample> BuildPath(const FVector& DrillStart, const FVector& DrillA, float DrillStraightA,
        float DrillRadius, const FVector& DrillB, float DrillStraightB)
    {
        TArray<FSample> DrillPath;
        const float DrillTurn = Turn(DrillA, DrillB);
        const float DrillArc = FMath::Abs(FMath::DegreesToRadians(DrillTurn)) * DrillRadius;
        const float DrillLength = DrillStraightA + DrillArc + DrillStraightB;
        const int32 DrillSteps = FMath::Max(1, FMath::CeilToInt(DrillLength / 10.f));
        TArray<float> DrillDistances;
        for (int32 DrillIndex = 0; DrillIndex <= DrillSteps; ++DrillIndex)
            DrillDistances.Add(FMath::Min(DrillIndex * 10.f, DrillLength));
        // Preserve exact positions AND tangent yaws at both straight/arc junctions.
        DrillDistances.Add(DrillStraightA);
        DrillDistances.Add(DrillStraightA + DrillArc);
        DrillDistances.Sort();
        for (const float DrillS : DrillDistances)
        {
            FVector DrillPosition;
            float DrillYaw = DrillA.Rotation().Yaw;
            if (DrillS <= DrillStraightA) DrillPosition = DrillStart + DrillA * DrillS;
            else if (DrillS < DrillStraightA + DrillArc)
            {
                const float DrillFraction = (DrillS - DrillStraightA) / FMath::Max(0.01f, DrillArc);
                DrillYaw += DrillTurn * DrillFraction;
                DrillPosition = DrillStart + DrillA * DrillStraightA +
                    ArcDelta(DrillA, FRotator(0.f, DrillYaw, 0.f).Vector(), DrillRadius);
            }
            else
            {
                DrillYaw += DrillTurn;
                DrillPosition = DrillStart + DrillA * DrillStraightA + ArcDelta(DrillA, DrillB, DrillRadius) +
                    DrillB * (DrillS - DrillStraightA - DrillArc);
            }
            // Analytic arc length avoids a sample-size-dependent phase/end-point error.
            DrillPath.Add({DrillPosition, DrillYaw, DrillS});
        }
        return DrillPath;
    }
    static FVector Sample(const TArray<FSample>& DrillPath, float DrillS, float DrillLateral, float& DrillOutYaw)
    {
        int32 DrillLow = 0, DrillHigh = DrillPath.Num() - 1;
        while (DrillHigh - DrillLow > 1)
        {
            const int32 DrillMid = (DrillLow + DrillHigh) / 2;
            if (DrillPath[DrillMid].Distance <= DrillS) DrillLow = DrillMid; else DrillHigh = DrillMid;
        }
        const FSample& DrillA = DrillPath[DrillLow];
        const FSample& DrillB = DrillPath[DrillHigh];
        const float DrillAlpha = FMath::Clamp((DrillS - DrillA.Distance) / FMath::Max(0.01f, DrillB.Distance - DrillA.Distance), 0.f, 1.f);
        DrillOutYaw = DrillA.Yaw + FMath::FindDeltaAngleDegrees(DrillA.Yaw, DrillB.Yaw) * DrillAlpha;
        return FMath::Lerp(DrillA.Position, DrillB.Position, DrillAlpha) +
            Right(FRotator(0.f, DrillOutYaw, 0.f).Vector()) * DrillLateral;
    }
    // Appends a straight piece from the path's end (DrillFrom must equal the last sample's position).
    static void AppendStraight(TArray<FSample>& DrillPath, const FVector& DrillFrom, const FVector& DrillDirection, float DrillLength)
    {
        const float DrillBase = DrillPath.IsEmpty() ? 0.f : DrillPath.Last().Distance;
        const float DrillYaw = DrillDirection.Rotation().Yaw;
        const int32 DrillSteps = FMath::Max(1, FMath::CeilToInt(DrillLength / 10.f));
        for (int32 DrillIndex = DrillPath.IsEmpty() ? 0 : 1; DrillIndex <= DrillSteps; ++DrillIndex)
        {
            const float DrillS = DrillLength * DrillIndex / DrillSteps;
            DrillPath.Add({DrillFrom + DrillDirection * DrillS, DrillYaw, DrillBase + DrillS});
        }
    }
    // Appends a smooth curve (cubic Hermite) from a position/heading to a position/heading; arc-length distances.
    static void AppendHermite(TArray<FSample>& DrillPath, const FVector& DrillP0, const FVector& DrillT0,
        const FVector& DrillP1, const FVector& DrillT1)
    {
        const float DrillChord = FVector::Dist2D(DrillP0, DrillP1);
        if (DrillChord < 5.f) return;
        const float DrillTangent = FMath::Max(300.f, DrillChord * 1.1f);
        const FVector DrillM0 = DrillT0.GetSafeNormal2D() * DrillTangent;
        const FVector DrillM1 = DrillT1.GetSafeNormal2D() * DrillTangent;
        const int32 DrillSteps = FMath::Clamp(FMath::CeilToInt(DrillChord * 3.f / 10.f), 16, 2000);
        float DrillS = DrillPath.IsEmpty() ? 0.f : DrillPath.Last().Distance;
        FVector DrillPrevious = DrillP0;
        for (int32 DrillIndex = 1; DrillIndex <= DrillSteps; ++DrillIndex)
        {
            const float DrillU = float(DrillIndex) / DrillSteps;
            const float DrillU2 = DrillU * DrillU, DrillU3 = DrillU2 * DrillU;
            const FVector DrillPosition = DrillP0 * (2.f * DrillU3 - 3.f * DrillU2 + 1.f) + DrillM0 * (DrillU3 - 2.f * DrillU2 + DrillU) +
                DrillP1 * (-2.f * DrillU3 + 3.f * DrillU2) + DrillM1 * (DrillU3 - DrillU2);
            const FVector DrillDerivative = DrillP0 * (6.f * DrillU2 - 6.f * DrillU) + DrillM0 * (3.f * DrillU2 - 4.f * DrillU + 1.f) +
                DrillP1 * (-6.f * DrillU2 + 6.f * DrillU) + DrillM1 * (3.f * DrillU2 - 2.f * DrillU);
            DrillS += float(FVector::Dist2D(DrillPrevious, DrillPosition));
            DrillPath.Add({DrillPosition, float(DrillDerivative.Rotation().Yaw), DrillS});
            DrillPrevious = DrillPosition;
        }
        DrillPath.Last().Position = DrillP1;
        DrillPath.Last().Yaw = DrillT1.Rotation().Yaw;
    }
    static float Flank(const FVector& DrillFacing, const FVector& DrillMarch)
    {
        float DrillFlank = FVector::DotProduct(DrillMarch, Right(DrillFacing)) >= 0.f ? 1.f : -1.f;
        if (FMath::Abs(Turn(DrillMarch, Right(DrillFacing) * DrillFlank)) > 135.f) DrillFlank *= -1.f;
        return DrillFlank;
    }
    // DrillLineCenter is the front-rank origin, not the depth-centred line centroid.
    // Returns the physical column centre. Generated column slots are head-anchored;
    // the caller adds MarchDir * ColumnLength/2 to obtain the actor/slot origin.
    static FVector ColumnApproachGoal(const FVector& DrillLineCenter, const FVector& DrillFacing,
        const FVector& DrillMarch, float DrillWidth, float DrillLength, float DrillRadius, float DrillRankSpacing)
    {
        const FVector DrillAxis = Right(DrillFacing) * Flank(DrillFacing, DrillMarch);
        const FVector DrillNear = DrillLineCenter - DrillFacing * DrillRankSpacing - DrillAxis * (DrillWidth * 0.5f);
        const float DrillArc = FMath::Abs(FMath::DegreesToRadians(Turn(DrillMarch, DrillAxis))) * DrillRadius;
        const float DrillRamp = FMath::Max(300.f, DrillArc);
        return DrillNear - ArcDelta(DrillMarch, DrillAxis, DrillRadius) -
            DrillAxis * (DrillRamp - DrillArc) - DrillMarch * (DrillLength * 0.5f);
    }
};
