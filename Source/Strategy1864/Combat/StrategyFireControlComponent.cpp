#include "StrategyFireControlComponent.h"

#include "StrategyVisibilityComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Formations/StrategyFormationComponent.h"
#include "StrategyContactComponent.h"
#include "DrawDebugHelpers.h"

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

bool UStrategyFireControlComponent::IsInsideFireCone(const AStrategyUnit* Target) const
{
    const AStrategyUnit* Unit = OwnerUnit ? OwnerUnit.Get() : Cast<AStrategyUnit>(GetOwner());
    if (!Unit || !IsValid(Target))
    {
        return false;
    }

    FVector ToTarget = Target->GetActorLocation() - Unit->GetActorLocation();
    ToTarget.Z = 0.0f;

    if (ToTarget.IsNearlyZero())
    {
        return true;
    }

    const FVector Direction = ToTarget.GetSafeNormal();

    if (Unit->FormationComponent &&
        Unit->FormationComponent->CurrentFormation == EStrategyFormationType::Square)
    {
        const float BaseYaw = Unit->GetActorRotation().Yaw;

        for (int32 FaceIndex = 0; FaceIndex < 4; ++FaceIndex)
        {
            const FVector FaceForward =
                FRotator(0.0f, BaseYaw + FaceIndex * 90.0f, 0.0f)
                .Vector()
                .GetSafeNormal2D();

            const float FaceDot = FMath::Clamp(
                FVector::DotProduct(FaceForward, Direction),
                -1.0f,
                1.0f);

            const float FaceAngleDegrees =
                FMath::RadiansToDegrees(FMath::Acos(FaceDot));

            if (FaceAngleDegrees <= 45.0f)
            {
                return true;
            }
        }

        return false;
    }

    const FVector Forward = Unit->GetActorForwardVector().GetSafeNormal2D();

    const float Dot = FMath::Clamp(
        FVector::DotProduct(Forward, Direction),
        -1.0f,
        1.0f);

    const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));
    return AngleDegrees <= FireConeHalfAngleDegrees;
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

    const float DistanceCm = FVector::Dist2D(
        Unit->GetActorLocation(),
        Target->GetActorLocation());

    if (DistanceCm > GetActiveRangeCm() || !IsInsideFireCone(Target))
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
        Unit->VisibilityComponent->HasLineOfSightTo(Target);
}


void UStrategyFireControlComponent::DrawQARangeCones() const
{
    DrawRangeArc(
        CloseRangeCm,
        FirePolicy == EStrategyFirePolicy::Close,
        FColor(255, 110, 90));

    DrawRangeArc(
        MediumRangeCm,
        FirePolicy == EStrategyFirePolicy::Medium,
        FColor(255, 190, 80));

    DrawRangeArc(
        LongRangeCm,
        FirePolicy == EStrategyFirePolicy::Long,
        FColor(245, 245, 245));
}

void UStrategyFireControlComponent::DrawRangeArc(
    float RangeCm,
    bool bActive,
    const FColor& Color) const
{
    if (!OwnerUnit || !GetWorld() || RangeCm <= 0.0f)
    {
        return;
    }

    const FVector Origin = OwnerUnit->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
    const float BaseYaw = OwnerUnit->GetActorRotation().Yaw;
    const int32 Segments = 12;
    const float Thickness = bActive ? 4.0f : 1.0f;

    FVector PreviousPoint = Origin;
    FVector FirstPoint = Origin;
    FVector LastPoint = Origin;

    for (int32 Index = 0; Index <= Segments; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / static_cast<float>(Segments);
        const float Angle =
            FMath::Lerp(-FireConeHalfAngleDegrees, FireConeHalfAngleDegrees, Alpha);
        const FVector Direction =
            FRotator(0.0f, BaseYaw + Angle, 0.0f).Vector();
        const FVector Point = Origin + Direction * RangeCm;

        if (Index == 0)
        {
            FirstPoint = Point;
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

    DrawDebugLine(GetWorld(), Origin, FirstPoint, Color, false, 0.0f, 0, Thickness);
    DrawDebugLine(GetWorld(), Origin, LastPoint, Color, false, 0.0f, 0, Thickness);
}
