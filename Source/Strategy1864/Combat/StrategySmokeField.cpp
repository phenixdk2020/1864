#include "StrategySmokeField.h"
#include "Components/SceneComponent.h"

AStrategySmokeField::AStrategySmokeField()
{
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    PrimaryActorTick.bCanEverTick = true;
}

void AStrategySmokeField::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    AgeSeconds += DeltaTime;
    if (AgeSeconds >= LifetimeSeconds)
    {
        Destroy();
    }
}

float AStrategySmokeField::GetCurrentDensity() const
{
    const float Alpha =
        LifetimeSeconds > KINDA_SMALL_NUMBER
        ? FMath::Clamp(AgeSeconds / LifetimeSeconds, 0.0f, 1.0f)
        : 1.0f;

    return InitialDensity * (1.0f - Alpha);
}

bool AStrategySmokeField::IntersectsSightSegment(
    const FVector& Start,
    const FVector& End) const
{
    const FVector Segment = End - Start;
    const float LengthSq = Segment.SizeSquared();

    if (LengthSq <= KINDA_SMALL_NUMBER)
    {
        return FVector::Dist(Start, GetActorLocation()) <= RadiusCm;
    }

    const float T =
        FMath::Clamp(
            FVector::DotProduct(GetActorLocation() - Start, Segment) / LengthSq,
            0.0f,
            1.0f);

    const FVector Closest = Start + Segment * T;
    return FVector::Dist(Closest, GetActorLocation()) <= RadiusCm;
}
