#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyHQVisualComponent.generated.h"

class AStrategyHQUnit;
class UAnimSequence;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A headquarters on the field (PROJECT 1864): the commander's staff shown as two or three horsemen (a pair for a battalion
 * or regiment, a wedge of three for a brigade or division) instead of the grey block. They ride the cavalry's own models
 * and follow the headquarters, facing the way it moves.
 */
UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyHQVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyHQVisualComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;

private:
    bool Build();
    int32 RiderCount() const;

    struct FStaffRider
    {
        TObjectPtr<UStaticMeshComponent> Horse;
        TObjectPtr<USkeletalMeshComponent> Rider;
        FVector Offset = FVector::ZeroVector;
        float Phase = 0.0f;
    };

    UPROPERTY() TObjectPtr<AStrategyHQUnit> OwnerHQ;
    UPROPERTY() TObjectPtr<USceneComponent> Pivot;
    UPROPERTY() TObjectPtr<UStaticMesh> HorseModel;
    UPROPERTY() TObjectPtr<USkeletalMesh> RiderModel;
    UPROPERTY() TObjectPtr<UAnimSequence> SeatClip;
    TArray<FStaffRider> Riders;
    FVector LastLocation = FVector::ZeroVector;
    float Yaw = 0.0f;
    float Pace = 0.0f;
    float HorseScale = 1.0f;
    float HorseYaw = 0.0f;
    float HorseLift = 0.0f;
    float SaddleHeightCm = 120.0f;
    bool bBuilt = false;
};
