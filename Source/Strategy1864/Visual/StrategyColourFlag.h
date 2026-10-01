#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyColourFlag.generated.h"

class UProceduralMeshComponent;

/** A company's colour: a pole with the national flag (Dannebrog, the Swedish cross flag), following the company. */
UCLASS()
class STRATEGY1864_API AStrategyColourFlag : public AActor
{
    GENERATED_BODY()

public:
    AStrategyColourFlag();
    virtual void Tick(float DeltaSeconds) override;

    /** Build the flag for a nation ("DK", "SE", "PR", "AT") and follow the actor. */
    void Setup(AActor* InFollow, const FString& Nation, const FVector& Offset);

private:
    UPROPERTY()
    TObjectPtr<UProceduralMeshComponent> Mesh;

    TWeakObjectPtr<AActor> Follow;
    FVector LocalOffset = FVector::ZeroVector;
    float Age = 0.0f;
};
