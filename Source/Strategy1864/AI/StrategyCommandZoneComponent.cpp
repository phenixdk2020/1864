#include "StrategyCommandZoneComponent.h"

#include "../Units/StrategyHQUnit.h"
#include "DrawDebugHelpers.h"

UStrategyCommandZoneComponent::UStrategyCommandZoneComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyCommandZoneComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerHQ = Cast<AStrategyHQUnit>(GetOwner());
}

void UStrategyCommandZoneComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bDrawWhenSelected || !OwnerHQ || !OwnerHQ->bSelected || !GetWorld())
    {
        return;
    }

    const FVector Center = OwnerHQ->GetActorLocation() + FVector(0.0f, 0.0f, 25.0f);

    DrawDebugCircle(
        GetWorld(),
        Center,
        OwnerHQ->CommandInnerRadius,
        96,
        FColor(80, 180, 255),
        false,
        0.0f,
        0,
        2.5f,
        FVector(1,0,0),
        FVector(0,1,0),
        false);

    DrawDebugCircle(
        GetWorld(),
        Center,
        OwnerHQ->CommandOuterRadius,
        96,
        FColor(120, 120, 120),
        false,
        0.0f,
        0,
        1.25f,
        FVector(1,0,0),
        FVector(0,1,0),
        false);
}
