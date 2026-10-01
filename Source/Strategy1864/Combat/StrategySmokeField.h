#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategySmokeField.generated.h"

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategySmokeField : public AActor
{
    GENERATED_BODY()

public:
    AStrategySmokeField();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Smoke")
    float RadiusCm = 1400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Smoke", meta=(ClampMin="0.0", ClampMax="1.0"))
    float InitialDensity = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Smoke")
    float LifetimeSeconds = 18.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Smoke")
    float AgeSeconds = 0.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Smoke")
    float GetCurrentDensity() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Smoke")
    bool IntersectsSightSegment(const FVector& Start, const FVector& End) const;
};
