#include "StrategyRiverBarrier.h"
#include "Components/SceneComponent.h"
#include "../Units/StrategyUnit.h"

AStrategyRiverBarrier::AStrategyRiverBarrier()
{
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    PrimaryActorTick.bCanEverTick = false;
}

FVector AStrategyRiverBarrier::GetAcrossRiverNormal() const
{
    FVector Axis = RiverAxisDirection.GetSafeNormal2D();
    if (Axis.IsNearlyZero())
    {
        Axis = FVector::ForwardVector;
    }

    return FVector(-Axis.Y, Axis.X, 0.0f);
}

float AStrategyRiverBarrier::SignedBankDistance(const FVector& WorldLocation) const
{
    return FVector::DotProduct(
        WorldLocation - GetActorLocation(),
        GetAcrossRiverNormal());
}

int32 AStrategyRiverBarrier::GetBankSide(const FVector& WorldLocation) const
{
    const float SignedDistance = SignedBankDistance(WorldLocation);
    if (FMath::Abs(SignedDistance) <= RiverHalfWidthCm)
    {
        return 0;
    }

    return SignedDistance < 0.0f ? -1 : 1;
}

FVector AStrategyRiverBarrier::GetBridgeApproachForSide(int32 Side) const
{
    return GetActorLocation() + (Side < 0 ? BankAApproachOffset : BankBApproachOffset);
}

FVector AStrategyRiverBarrier::GetBridgeExitForSide(int32 Side) const
{
    const FVector Approach = GetBridgeApproachForSide(Side);
    const FVector Normal = GetAcrossRiverNormal();
    return Approach + Normal * (Side < 0 ? -ExitClearanceCm : ExitClearanceCm);
}

bool AStrategyRiverBarrier::IsInsideRiver(const FVector& WorldLocation) const
{
    return FMath::Abs(SignedBankDistance(WorldLocation)) <= RiverHalfWidthCm;
}

bool AStrategyRiverBarrier::RequiresBankChange(const FVector& Start, const FVector& End) const
{
    const int32 StartSide = GetBankSide(Start);
    const int32 EndSide = GetBankSide(End);

    return StartSide != 0 && EndSide != 0 && StartSide != EndSide;
}

bool AStrategyRiverBarrier::SameBankChordIntersectsRiver(const FVector& Start, const FVector& End) const
{
    const int32 StartSide = GetBankSide(Start);
    const int32 EndSide = GetBankSide(End);

    if (StartSide == 0 || StartSide != EndSide)
    {
        return false;
    }

    const FVector Midpoint = (Start + End) * 0.5f;
    return FMath::Abs(SignedBankDistance(Midpoint)) <= RiverHalfWidthCm;
}


bool AStrategyRiverBarrier::TryAcquireCrossing(AStrategyUnit* Unit)
{
    if (!IsValid(Unit))
    {
        return false;
    }

    ActiveCrossers.RemoveAll(
        [](const TObjectPtr<AStrategyUnit>& Candidate)
        {
            return !IsValid(Candidate);
        });

    if (ActiveCrossers.Contains(Unit))
    {
        return true;
    }

    if (ActiveCrossers.Num() >= FMath::Max(1, MaxConcurrentCrossers))
    {
        return false;
    }

    ActiveCrossers.Add(Unit);
    return true;
}

void AStrategyRiverBarrier::ReleaseCrossing(AStrategyUnit* Unit)
{
    ActiveCrossers.Remove(Unit);
}

bool AStrategyRiverBarrier::IsCrossingOwner(const AStrategyUnit* Unit) const
{
    return IsValid(Unit) && ActiveCrossers.Contains(Unit);
}
