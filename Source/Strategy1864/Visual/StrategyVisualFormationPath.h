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
    // Snake column (rigid company in march column): the head walks at march pace and turns on a fixed radius,
    // so every man turns where the head turned. 0 = the old free head (speed up to 2x walk, 50 deg/s).
    float HeadSpeedCap = 0.f;
    float HeadTurnRadiusCm = 0.f;
    bool bHeadLocked = false;   // snake head has caught the actor: it now walks exactly on the actor's own track

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

    void Advance(const FTransform& Unit, float Dt, bool bColumn, float WalkSpeed, bool bRigid = false)
    {
        if (!bInitialized) { Reset(Unit); }
        if (bRigid)
        {
            // As in the Unity prototype: the company is one rigid body. The drawn centre and facing ARE the unit's (no spring, no
            // turn-rate lag, no trail), so the men keep their places in the unit's own frame and nothing can wander or hop.
            bMovingVisuals = !Unit.Equals(PreviousUnit, 0.001f);
            bTurning = false;
            Center = Unit.GetLocation();
            CenterVelocity = FVector::ZeroVector;
            Facing = Unit.Rotator().Yaw;
            FacingVelocity = 0.f;
            Trail.Reset();
            TrailDistance = 0.f;
            return;
        }
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
            // Substepped, so the curve does not depend on the frame rate or a hitch.
            for (float HeadTimeLeft = Dt; HeadTimeLeft > SMALL_NUMBER; )
            {
                const float HeadDt = FMath::Min(HeadTimeLeft, 1.f / 60.f);
                HeadTimeLeft -= HeadDt;
                const FVector To = HeadTarget - HeadPos;
                const float Dist = To.Size2D();
                // Snake head locked on the actor (user's recording 2026-10-18: a pursuing head with a turn limit weaved from side
                // to side round a straight march). Once caught, it walks exactly where the actor walks; the turn radius only
                // shapes the first convergence from the leading flank and a jump of the actor (new route, relocation).
                // Lock only when caught AND heading the same way, so the right face still curves off the flank on the radius.
                const bool bHeadAligned = Dist > 1.f && FMath::Abs(FMath::FindDeltaAngleDegrees(HeadYaw, To.Rotation().Yaw)) < 12.f;
                if (HeadSpeedCap > 0.f && (bHeadLocked ? Dist < 300.f : (Dist < 40.f && bHeadAligned)))
                {
                    bHeadLocked = true;
                    // A halting actor that settles back a few cm must not turn the head round through its own column.
                    const bool bSettlingBack = Dist < 50.f && FVector::DotProduct(To, FRotator(0.f, HeadYaw, 0.f).Vector()) < 0.f;
                    if (!bSettlingBack)
                    {
                        if (Dist > 1.f) HeadYaw = To.Rotation().Yaw;
                        HeadPos = HeadTarget;
                    }
                    break;
                }
                bHeadLocked = false;
                float Speed = FMath::Min(Dist * 4.f, FMath::Max(WalkSpeed * 2.f, 300.f));
                if (HeadSpeedCap > 0.f) Speed = FMath::Min(Speed, HeadSpeedCap);
                // Snake head: always the same turning radius, at every distance (review 2026-10-18: aiming straight at a
                // close actor kinked the column, and freezing a head with the target behind it stalled a new march).
                // The route itself leads to the approach point, so the head does not pass the actor at a halt.
                if (Dist > 30.f)
                {
                    const float Want = To.Rotation().Yaw;
                    const float HeadTurnRate = HeadTurnRadiusCm > 0.f ?
                        FMath::Min(50.f, FMath::RadiansToDegrees(FMath::Max(Speed, 1.f) / HeadTurnRadiusCm)) : 50.f;
                    const float MaxTurn = HeadTurnRate * HeadDt;
                    HeadYaw = FRotator::NormalizeAxis(HeadYaw + FMath::Clamp(FMath::FindDeltaAngleDegrees(HeadYaw, Want), -MaxTurn, MaxTurn));
                }
                HeadPos += FRotator(0.f, HeadYaw, 0.f).Vector() * (Speed * HeadDt);
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
