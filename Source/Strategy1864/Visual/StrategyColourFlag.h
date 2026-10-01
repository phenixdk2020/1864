#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyColourFlag.generated.h"

class UProceduralMeshComponent;
class UStaticMeshComponent;

/** A company's colour: the flag model (SM_Flag_Standard) with any flag picture on its cloth (/Game/Units/Flags/T_Flag_<Nation>,
 *  through M_FlagCloth's "Flag" parameter), following the company; a procedural pole and cross flag when the model is missing. */
UCLASS()
class STRATEGY1864_API AStrategyColourFlag : public AActor
{
    GENERATED_BODY()

public:
    AStrategyColourFlag();
    virtual void Tick(float DeltaSeconds) override;

    /** Build the flag for a nation ("DK", "SE", "PR", "AT") and follow the actor. */
    void Setup(AActor* InFollow, const FString& Nation, const FVector& Offset);

    /** Put any flag picture on the model's cloth (false without the model). */
    bool SetFlagTexture(UTexture* Flag);

private:
    UPROPERTY()
    TObjectPtr<UProceduralMeshComponent> Mesh;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Model;

    TWeakObjectPtr<AActor> Follow;
    FVector LocalOffset = FVector::ZeroVector;
    float Age = 0.0f;
};
