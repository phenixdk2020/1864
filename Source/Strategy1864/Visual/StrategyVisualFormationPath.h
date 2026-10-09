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
    FVector CenterVelocity = FVector::ZeroVector;
    FBox SlotBounds = FBox(ForceInit);
    float Facing = 0.f;
    float FacingVelocity = 0.f;
    float TrailDistance = 0.f;
    bool bInitialized = false;
    bool bTurning = false;
    bool bMovingVisuals = false;
    FVector HeadPos = FVector::ZeroVector; // column head: a vehicle with a turn rate, so corners are rounded
    float HeadYaw = 0.f;

    static void SmoothPosition(FVector& Position, FVector& Velocity, const FVector& Target,
                               float Dt, float Frequency)
    {
        if (Dt <= 0.f) return;
        const FVector Error = Position - Target;
        const FVector Term = Velocity + Error * Frequency;
        const float Decay = FMath::Exp(-Frequency * Dt);
        Position = Target + (Error + Term * Dt) * Decay;
        Velocity = (Velocity - Term * (Frequency * Dt)) * Decay;
    }

    static void SmoothTravel(FVector& Position, FVector& Velocity, const FVector& Target, float Dt, float MaxSpeed)
    {
        for (float TravelTimeLeft = Dt; TravelTimeLeft > SMALL_NUMBER; )
        {
            const float TravelDt = FMath::Min(TravelTimeLeft, 1.f / 120.f);
            const FVector TravelError = Target - Position;
            const FVector DesiredVelocity = TravelError.GetSafeNormal() * FMath::Min(MaxSpeed, TravelError.Size() * 12.f);
            const float TravelDecay = FMath::Exp(-12.f * TravelDt);
            Position += DesiredVelocity * TravelDt + (Velocity - DesiredVelocity) * ((1.f - TravelDecay) / 12.f);
            Velocity = DesiredVelocity + (Velocity - DesiredVelocity) * TravelDecay;
            TravelTimeLeft -= TravelDt;
        }
    }

    static float SmoothFacing(float Yaw, float& AngularVelocity, float Target, float Dt, float MaxRate)
    {
        // Small presentation-only integration steps keep x10/hitches from overshooting.
        for (float Left = Dt; Left > SMALL_NUMBER; )
        {
            const float StepTime = FMath::Min(Left, 1.f / 120.f);
            const float DesiredRate = FMath::Clamp(FMath::FindDeltaAngleDegrees(Yaw, Target) * 12.f, -MaxRate, MaxRate);
            const float Decay = FMath::Exp(-12.f * StepTime);
            Yaw += DesiredRate * StepTime + (AngularVelocity - DesiredRate) * ((1.f - Decay) / 12.f);
            AngularVelocity = DesiredRate + (AngularVelocity - DesiredRate) * Decay;
            Left -= StepTime;
        }
        return FRotator::NormalizeAxis(Yaw);
    }

    void Reset(const FTransform& Unit)
    {
        PreviousUnit = Unit;
        Center = Unit.GetLocation();
        CenterVelocity = FVector::ZeroVector;
        Facing = Unit.Rotator().Yaw;
        FacingVelocity = 0.f;
        TrailDistance = 0.f;
        Trail.Reset();
        bInitialized = true;
    }

    void Advance(const FTransform& Unit, float Dt, bool bColumn, float WalkSpeed)
    {
        if (!bInitialized) { Reset(Unit); }
        // Exact critically damped presentation spring: never inherit an actor snap.
        // Velocity survives starts, stops, route corners and speed changes.
        SmoothPosition(Center, CenterVelocity, Unit.GetLocation(), Dt, 24.f);
        const float Remaining = FMath::FindDeltaAngleDegrees(Facing, Unit.Rotator().Yaw);
        bTurning = FMath::Abs(Remaining) > 0.01f;
        const float Radius = SlotBounds.IsValid ? SlotBounds.GetSize().Size2D() : 1.f;
        // The wheel: the outer end of the line steps out at a brisk walk (about 4 m/s at most), so the line turns
        // as a line, not as a bent snake. Half the line's length is the radius.
        const float OuterSpeed = FMath::Max(WalkSpeed * 2.f, 400.f);
        const float TurnRate = FMath::Clamp(FMath::RadiansToDegrees(OuterSpeed / FMath::Max(1.f, Radius * 0.5f)), 5.f, 36.f);
        const float NewFacing = SmoothFacing(Facing, FacingVelocity, Unit.Rotator().Yaw, Dt, TurnRate);
        Facing = NewFacing;
        bMovingVisuals = bTurning || FMath::Abs(FacingVelocity) > 0.01f || CenterVelocity.SizeSquared() > 0.01f || !Center.Equals(Unit.GetLocation(), 0.1f);
        if (bColumn)
        {
            const float Front = SlotBounds.IsValid ? SlotBounds.Max.X : 0.f;
            const float Length = SlotBounds.IsValid ? SlotBounds.GetSize().X + 1000.f : 1000.f;
            const FVector HeadTarget = Unit.GetLocation() + FRotator(0.f, Unit.Rotator().Yaw, 0.f).RotateVector(FVector(Front, 0.f, 0.f));
            if (Trail.IsEmpty())
            {
                HeadPos = HeadTarget;
                HeadYaw = Unit.Rotator().Yaw;
            }
            // The head walks towards its place with a limited turn rate (about 3 m turning radius), so the column
            // bends round a corner instead of folding at it.
            {
                const FVector To = HeadTarget - HeadPos;
                const float Dist = To.Size2D();
                if (Dist > 30.f)
                {
                    const float Want = To.Rotation().Yaw;
                    const float MaxTurn = 50.f * Dt;
                    HeadYaw = FRotator::NormalizeAxis(HeadYaw + FMath::Clamp(FMath::FindDeltaAngleDegrees(HeadYaw, Want), -MaxTurn, MaxTurn));
                }
                const float Speed = FMath::Min(Dist * 4.f, FMath::Max(WalkSpeed * 2.f, 300.f));
                HeadPos += FRotator(0.f, HeadYaw, 0.f).Vector() * (Speed * Dt);
                HeadPos.Z = HeadTarget.Z;
            }
            Facing = HeadYaw;
            const FVector Lead = HeadPos;
            if (Trail.IsEmpty())
            {
                TrailDistance = 0.f; // A new column starts a new distance coordinate.
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
