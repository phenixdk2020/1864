#pragma once

#include "CoreMinimal.h"

// Presentation only: a speed-limited wheel and a shared, distance-sampled column trail.
// No terrain queries, slot generation, or per-figure path searches through the world.
struct FStrategyVisualFormationPath
{
    struct FSample { FVector Position; float Yaw; float Distance; };
    TArray<FSample> Trail;
    FTransform PreviousUnit = FTransform::Identity;
    FVector Center = FVector::ZeroVector;
    FBox SlotBounds = FBox(ForceInit);
    float Facing = 0.f;
    float TrailDistance = 0.f;
    bool bInitialized = false;
    bool bTurning = false;
    bool bMovingVisuals = false;

    void Reset(const FTransform& Unit)
    {
        PreviousUnit = Unit;
        Center = Unit.GetLocation();
        Facing = Unit.Rotator().Yaw;
        TrailDistance = 0.f;
        Trail.Reset();
        bInitialized = true;
    }

    void Advance(const FTransform& Unit, float Dt, bool bColumn, float WalkSpeed)
    {
        if (!bInitialized) { Reset(Unit); }
        const FVector Translation = Unit.GetLocation() - PreviousUnit.GetLocation();
        Center += Translation;
        const float Remaining = FMath::FindDeltaAngleDegrees(Facing, Unit.Rotator().Yaw);
        // The one-degree gate is shared by all figures; once started, finish smoothly.
        bTurning = FMath::Abs(Remaining) > (bTurning ? 0.01f : 1.f);
        const FVector Pivot = SlotBounds.IsValid
            ? FVector(Remaining >= 0.f ? SlotBounds.Max.X : SlotBounds.Min.X,
                      Remaining >= 0.f ? SlotBounds.Max.Y : SlotBounds.Min.Y, 0.f)
            : FVector::ZeroVector;
        const float Radius = SlotBounds.IsValid ? SlotBounds.GetSize().Size2D() : 1.f;
        const float TurnRate = FMath::Min(36.f, FMath::RadiansToDegrees(WalkSpeed / FMath::Max(1.f, Radius)));
        const float NewFacing = bTurning ? FMath::FixedTurn(Facing, Unit.Rotator().Yaw, TurnRate * Dt) : Facing;
        if (bTurning && !bColumn)
        {
            // Keep the end of the front/rear rank fixed while the rest walks its arc.
            Center += FRotator(0.f, Facing, 0.f).RotateVector(Pivot)
                    - FRotator(0.f, NewFacing, 0.f).RotateVector(Pivot);
        }
        else
        {
            Center = FMath::VInterpConstantTo(Center, Unit.GetLocation(), Dt, WalkSpeed);
        }
        Facing = NewFacing;
        bMovingVisuals = bTurning || !Center.Equals(Unit.GetLocation(), 0.1f);
        if (bColumn)
        {
            const float Front = SlotBounds.IsValid ? SlotBounds.Max.X : 0.f;
            const FVector Lead = Center + FRotator(0.f, Facing, 0.f).RotateVector(FVector(Front, 0.f, 0.f));
            const float Length = SlotBounds.IsValid ? SlotBounds.GetSize().X + 1000.f : 1000.f;
            if (Trail.IsEmpty())
            {
                Trail.Add({Lead - FRotator(0.f, Facing, 0.f).Vector() * Length, Facing, -Length});
                Trail.Add({Lead, Facing, 0.f});
            }
            const float Travel = FVector::Dist2D(Trail.Last().Position, Lead);
            if (Travel > KINDA_SMALL_NUMBER)
            {
                TrailDistance += Travel;
                Trail.Add({Lead, Facing, TrailDistance});
                // Bound memory by formation depth; retain a segment before the tail.
                int32 Drop = 0;
                while (Drop + 2 < Trail.Num() && Trail[Drop + 1].Distance < TrailDistance - Length) { ++Drop; }
                if (Drop > 0) { Trail.RemoveAt(0, Drop, EAllowShrinking::No); }
            }
        }
        else { Trail.Reset(); }
    }

    FVector Goal(const FVector& Slot, bool bColumn, float& OutYaw) const
    {
        OutYaw = Facing;
        if (bColumn && Trail.Num() >= 2)
        {
            const float Wanted = TrailDistance - (SlotBounds.Max.X - Slot.X);
            int32 Low = 0, High = Trail.Num() - 1;
            while (High - Low > 1)
            {
                const int32 Mid = (Low + High) / 2;
                if (Trail[Mid].Distance <= Wanted) { Low = Mid; } else { High = Mid; }
            }
            const FSample& A = Trail[Low];
            const FSample& B = Trail[High];
            const float Alpha = FMath::Clamp((Wanted - A.Distance) / FMath::Max(0.01f, B.Distance - A.Distance), 0.f, 1.f);
            OutYaw = A.Yaw + FMath::FindDeltaAngleDegrees(A.Yaw, B.Yaw) * Alpha;
            return FMath::Lerp(A.Position, B.Position, Alpha)
                + FRotator(0.f, OutYaw, 0.f).RotateVector(FVector(0.f, Slot.Y, Slot.Z));
        }
        return Center + FRotator(0.f, Facing, 0.f).RotateVector(Slot);
    }
};
