#include "StrategyFireControlComponent.h"

#include "StrategyVisibilityComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Formations/StrategyFormationComponent.h"
#include "StrategyContactComponent.h"
#include "../Visual/StrategyInfantryVisualComponent.h"
#include "DrawDebugHelpers.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyDragoonComponent.h"

UStrategyFireControlComponent::UStrategyFireControlComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyFireControlComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (OwnerUnit)
    {
        OwnerUnit->MaximumFireRangeCm = LongRangeCm;
    }
}

void UStrategyFireControlComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bDrawQARangeCones || !OwnerUnit)
    {
        return;
    }

    const bool bShouldDraw =
        OwnerUnit->bSelected ||
        (bShowPrussianQARangesFromStartup && OwnerUnit->Side == EStrategySide::Prussia);

    if (bShouldDraw)
    {
        DrawQARangeCones();
    }
}

void UStrategyFireControlComponent::SetFirePolicy(EStrategyFirePolicy NewPolicy)
{
    FirePolicy = NewPolicy;
}

float UStrategyFireControlComponent::GetActiveRangeCm() const
{
    switch (FirePolicy)
    {
        case EStrategyFirePolicy::Close:
            return CloseRangeCm;

        case EStrategyFirePolicy::Medium:
            return MediumRangeCm;

        case EStrategyFirePolicy::Long:
            return LongRangeCm;

        case EStrategyFirePolicy::Hold:
        default:
            return 0.0f;
    }
}

FString UStrategyFireControlComponent::GetActiveRangeLabel() const
{
    const TCHAR* Label = TEXT("Hold ild");
    switch (FirePolicy)
    {
        case EStrategyFirePolicy::Close: Label = TEXT("Kort"); break;
        case EStrategyFirePolicy::Medium: Label = TEXT("Mellem"); break;
        case EStrategyFirePolicy::Long: Label = TEXT("Lang"); break;
        default: return Label;
    }
    return FString::Printf(TEXT("%s (%.0f m)"), Label, GetActiveRangeCm() / 100.0f);
}

bool UStrategyFireControlComponent::IsInsideFireCone(const AStrategyUnit* Target) const
{
    return IsValid(Target) && IsLocationInsideFireField(Target->GetActorLocation(), MAX_flt);
}

void UStrategyFireControlComponent::GetFireFront(FVector& Left, FVector& Right, int32 FaceIndex) const
{
    const AStrategyUnit* Unit = OwnerUnit ? OwnerUnit.Get() : Cast<AStrategyUnit>(GetOwner());
    Left = Right = FVector::ZeroVector;
    if (!Unit) return;

    FBox Bounds(ForceInit);
    // Gameplay geometry must not depend on graphics quality or animated figure bounds.
    if (Unit->FormationComponent && Unit->CurrentStrength > 0)
    {
        for (const FStrategyFormationSlot& SimulatedSlot :
             Unit->FormationComponent->GenerateSoldierSlots(FVector::ZeroVector, 0.0f, Unit->CurrentStrength))
        {
            Bounds += SimulatedSlot.WorldLocation;
        }
        Bounds = Bounds.ExpandBy(35.0f);
    }
    if (!Bounds.IsValid) Bounds = FBox(FVector(-300.0f, -130.0f, 0.0f), FVector(300.0f, 130.0f, 140.0f));

    const FVector Forward = FRotator(0.0f, FaceIndex * 90.0f, 0.0f).Vector();
    const FVector Lateral(-Forward.Y, Forward.X, 0.0f);
    float Front = -MAX_flt;
    float MinLateral = MAX_flt;
    float MaxLateral = -MAX_flt;
    for (int32 X = 0; X < 2; ++X)
    {
        for (int32 Y = 0; Y < 2; ++Y)
        {
            const FVector Corner(X ? Bounds.Max.X : Bounds.Min.X, Y ? Bounds.Max.Y : Bounds.Min.Y, 0.0f);
            Front = FMath::Max(Front, FVector::DotProduct(Corner, Forward));
            const float Across = FVector::DotProduct(Corner, Lateral);
            MinLateral = FMath::Min(MinLateral, Across);
            MaxLateral = FMath::Max(MaxLateral, Across);
        }
    }
    Left = Unit->GetActorTransform().TransformPosition(Forward * Front + Lateral * MinLateral);
    Right = Unit->GetActorTransform().TransformPosition(Forward * Front + Lateral * MaxLateral);
}

TArray<FVector> UStrategyFireControlComponent::GetTargetSamplePoints(const AStrategyUnit* Target) const
{
    TArray<FVector> Points;
    if (!IsValid(Target))
    {
        return Points;
    }
    Points.Add(Target->GetActorLocation());
    FBox Bounds(ForceInit);
    if (Target->FormationComponent && Target->CurrentStrength > 0)
    {
        for (const FStrategyFormationSlot& Slot : Target->FormationComponent->GenerateSoldierSlots(FVector::ZeroVector, 0.0f, Target->CurrentStrength))
        {
            Bounds += Slot.WorldLocation;
        }
    }
    if (!Bounds.IsValid)
    {
        return Points;
    }
    // Seven points across the front and the rear, the centre line between.
    const FTransform& T = Target->GetActorTransform();
    for (int32 i = 0; i <= 6; ++i)
    {
        const float Y = FMath::Lerp(Bounds.Min.Y, Bounds.Max.Y, i / 6.0f);
        Points.Add(T.TransformPosition(FVector(Bounds.Max.X, Y, 0.0f)));
        Points.Add(T.TransformPosition(FVector(Bounds.Min.X, Y, 0.0f)));
        Points.Add(T.TransformPosition(FVector((Bounds.Min.X + Bounds.Max.X) * 0.5f, Y, 0.0f)));
    }
    return Points;
}

bool UStrategyFireControlComponent::CanPointBearOn(const FVector& From, const FVector& Forward, const AStrategyUnit* Target, float RangeCm) const
{
    const float TanHalf = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FireConeHalfAngleDegrees, 0.0f, 89.9f)));
    const FVector F = Forward.GetSafeNormal2D();
    const FVector Lateral(-F.Y, F.X, 0.0f);
    for (const FVector& P : GetTargetSamplePoints(Target))
    {
        FVector Offset = P - From;
        Offset.Z = 0.0f;
        const float Ahead = FVector::DotProduct(Offset, F);
        if (Ahead > 0.0f && Offset.Size2D() <= RangeCm && FMath::Abs(FVector::DotProduct(Offset, Lateral)) <= Ahead * TanHalf)
        {
            return true;
        }
    }
    return false;
}

float UStrategyFireControlComponent::GetBearingFraction(const AStrategyUnit* Target, int32* OutBearing, int32* OutTotal) const
{
    const AStrategyUnit* Unit = OwnerUnit ? OwnerUnit.Get() : Cast<AStrategyUnit>(GetOwner());
    if (OutBearing) { *OutBearing = 0; }
    if (OutTotal) { *OutTotal = 0; }
    if (!Unit || !IsValid(Target) || !Unit->FormationComponent || Unit->CurrentStrength <= 0)
    {
        return 1.0f;
    }
    if (Unit->FormationComponent->CurrentFormation == EStrategyFormationType::Square)
    {
        int32 SquareFaces = 0;
        const FVector SquareOffset = (Target->GetActorLocation() - Unit->GetActorLocation());
        const float SquareYaw = SquareOffset.Rotation().Yaw;
        if (SquareOffset.Size2D() <= GetActiveRangeCm())
        {
            for (int32 SquareFace = 0; SquareFace < 4; ++SquareFace)
            {
                if (FMath::Abs(FMath::FindDeltaAngleDegrees(Unit->GetActorRotation().Yaw + SquareFace * 90.0f, SquareYaw)) <= 45.0f + KINDA_SMALL_NUMBER)
                { ++SquareFaces; }
            }
        }
        const float SquareFraction = FMath::Clamp(SquareFaces * SquareFaceFireShare, 0.0f, 1.0f);
        if (OutBearing) { *OutBearing = FMath::RoundToInt(Unit->CurrentStrength * SquareFraction); }
        if (OutTotal) { *OutTotal = Unit->CurrentStrength; }
        return SquareFraction;
    }
    const TArray<FStrategyFormationSlot> Slots = Unit->FormationComponent->GenerateSoldierSlots(FVector::ZeroVector, 0.0f, Unit->CurrentStrength);
    if (Slots.Num() == 0)
    {
        return 1.0f;
    }
    // The target's points once, then each man's place against them.
    const TArray<FVector> Points = GetTargetSamplePoints(Target);
    const float RangeCm = GetActiveRangeCm();
    const float TanHalf = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FireConeHalfAngleDegrees, 0.0f, 89.9f)));
    const FTransform& T = Unit->GetActorTransform();
    const FVector F = Unit->GetActorForwardVector().GetSafeNormal2D();
    const FVector Lateral(-F.Y, F.X, 0.0f);
    int32 Bearing = 0;
    for (const FStrategyFormationSlot& Slot : Slots)
    {
        const FVector From = T.TransformPosition(Slot.WorldLocation);
        for (const FVector& P : Points)
        {
            FVector Offset = P - From;
            Offset.Z = 0.0f;
            const float Ahead = FVector::DotProduct(Offset, F);
            if (Ahead > 0.0f && Offset.Size2D() <= RangeCm && FMath::Abs(FVector::DotProduct(Offset, Lateral)) <= Ahead * TanHalf)
            {
                ++Bearing;
                break;
            }
        }
    }
    if (OutBearing) { *OutBearing = Bearing; }
    if (OutTotal) { *OutTotal = Slots.Num(); }
    return static_cast<float>(Bearing) / static_cast<float>(Slots.Num());
}

bool UStrategyFireControlComponent::IsLocationInsideFireField(FVector Location, float RangeCm) const
{
    const AStrategyUnit* Unit = OwnerUnit ? OwnerUnit.Get() : Cast<AStrategyUnit>(GetOwner());
    if (!Unit || RangeCm <= 0.0f) return false;
    const bool bSquare = Unit->FormationComponent &&
        Unit->FormationComponent->CurrentFormation == EStrategyFormationType::Square;
    if (bSquare)
    {
        const FVector SquareFieldOffset = Location - Unit->GetActorLocation();
        if (SquareFieldOffset.Size2D() > RangeCm) { return false; }
        for (int32 SquareFieldFace = 0; SquareFieldFace < 4; ++SquareFieldFace)
        {
            if (FMath::Abs(FMath::FindDeltaAngleDegrees(Unit->GetActorRotation().Yaw + SquareFieldFace * 90.0f,
                SquareFieldOffset.Rotation().Yaw)) <= 45.0f + KINDA_SMALL_NUMBER) { return true; }
        }
        return false;
    }
    const float HalfAngle = FMath::Clamp(FireConeHalfAngleDegrees, 0.0f, 89.9f);
    for (int32 Face = 0; Face < (bSquare ? 4 : 1); ++Face)
    {
        FVector Left, Right;
        GetFireFront(Left, Right, Face);
        const FVector Lateral = (Right - Left).GetSafeNormal2D();
        const FVector Forward(Lateral.Y, -Lateral.X, 0.0f);
        const float Across = FVector::DotProduct(Location - Left, Lateral);
        const FVector Origin = Left + Lateral * FMath::Clamp(Across, 0.0f, FVector::Dist2D(Left, Right));
        FVector Offset = Location - Origin;
        Offset.Z = 0.0f;
        const float Ahead = FVector::DotProduct(Offset, Forward);
        const float Sideways = FMath::Abs(FVector::DotProduct(Offset, Lateral));
        if (Ahead >= 0.0f && Offset.Size2D() <= RangeCm &&
            Sideways <= Ahead * FMath::Tan(FMath::DegreesToRadians(HalfAngle)) + KINDA_SMALL_NUMBER)
        {
            return true;
        }
    }
    return false;
}

bool UStrategyFireControlComponent::CanEngageTarget(const AStrategyUnit* Target) const
{
    const AStrategyUnit* Unit = OwnerUnit ? OwnerUnit.Get() : Cast<AStrategyUnit>(GetOwner());
    if (!Unit ||
        !IsValid(Target) ||
        !Unit->IsCombatEffective() ||
        !Target->IsCombatEffective() ||
        Target->Side == EStrategySide::Neutral ||
        Target->Side == Unit->Side ||
        FirePolicy == EStrategyFirePolicy::Hold)
    {
        return false;
    }

    if (!IsLocationInsideFireField(Target->GetActorLocation(), GetActiveRangeCm()))
    {
        return false;
    }

    if (bRequireCurrentContact &&
        (!Unit->ContactComponent ||
         !Unit->ContactComponent->HasCurrentContact(Target)))
    {
        return false;
    }

    return Unit->VisibilityComponent &&
        Unit->VisibilityComponent->CanDetectTarget(Target, GetActiveRangeCm());
}


void UStrategyFireControlComponent::DrawQARangeCones() const
{
    DrawRangeArc(
        0.0f,
        CloseRangeCm,
        FirePolicy == EStrategyFirePolicy::Close,
        FColor(100, 230, 130), TEXT("Kort"));

    DrawRangeArc(
        CloseRangeCm,
        MediumRangeCm,
        FirePolicy == EStrategyFirePolicy::Medium,
        FColor(255, 200, 60), TEXT("Mellem"));

    DrawRangeArc(
        MediumRangeCm,
        LongRangeCm,
        FirePolicy == EStrategyFirePolicy::Long,
        FColor(255, 110, 90), TEXT("Lang"));
}

void UStrategyFireControlComponent::DrawRangeArc(
    float InnerRangeCm,
    float RangeCm,
    bool bActive,
    const FColor& Color,
    const TCHAR* Label) const
{
    if (!OwnerUnit || !GetWorld() || RangeCm <= 0.0f)
    {
        return;
    }

    const bool bSquare = OwnerUnit->FormationComponent &&
        OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::Square;
    const float HalfAngle = FMath::Clamp(bSquare ? 45.0f : FireConeHalfAngleDegrees, 0.0f, 89.9f);
    const int32 Segments = 12;
    const float Thickness = bActive ? 12.0f : 5.0f;

    if (const ACavalryUnit* Horse = Cast<ACavalryUnit>(OwnerUnit))
    {
        if (!Horse->DragoonComponent || Horse->DragoonComponent->MountedState == EStrategyMountedState::Mounted)
        {
            return;   // mounted: no fire cone
        }
    }
    for (int32 Face = 0; Face < (bSquare ? 4 : 1); ++Face)
    {
        const float BaseYaw = OwnerUnit->GetActorRotation().Yaw + Face * 90.0f;
        FVector Left, Right;
        GetFireFront(Left, Right, Face);
        Left.Z += 20.0f;
        Right.Z += 20.0f;
        FVector PreviousPoint = Left;
        FVector FirstPoint = Left;
        FVector LastPoint = Right;
        bool bHasPrevious = false;

        for (int32 Edge = 0; Edge < 2; ++Edge)
        {
            for (int32 Index = 0; Index <= Segments; ++Index)
            {
                const float Alpha = static_cast<float>(Index) / static_cast<float>(Segments);
                const float Angle = Edge == 0 ? FMath::Lerp(-HalfAngle, 0.0f, Alpha) : FMath::Lerp(0.0f, HalfAngle, Alpha);
                const FVector Direction =
                    FRotator(0.0f, BaseYaw + Angle, 0.0f).Vector();
                const FVector Point = (Edge == 0 ? Left : Right) + Direction * RangeCm;

                if (!bHasPrevious)
                {
                    FirstPoint = Point;
                    bHasPrevious = true;
                }
                else
                {
                    DrawDebugLine(
                        GetWorld(),
                        PreviousPoint,
                        Point,
                        Color,
                        false,
                        0.0f,
                        0,
                        Thickness);
                }

                PreviousPoint = Point;
                LastPoint = Point;
            }
        }

        const FVector InnerLeft = Left + FRotator(0.0f, BaseYaw - HalfAngle, 0.0f).Vector() * InnerRangeCm;
        const FVector InnerRight = Right + FRotator(0.0f, BaseYaw + HalfAngle, 0.0f).Vector() * InnerRangeCm;
        DrawDebugLine(GetWorld(), InnerLeft, FirstPoint, Color, false, 0.0f, 0, Thickness);
        DrawDebugLine(GetWorld(), InnerRight, LastPoint, Color, false, 0.0f, 0, Thickness);
        if (InnerRangeCm <= 0.0f)
        {
            DrawDebugLine(GetWorld(), Left, Right, Color, false, 0.0f, 0, Thickness);
        }

    }
}
