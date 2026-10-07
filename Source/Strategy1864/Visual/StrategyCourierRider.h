#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyCourierRider.generated.h"

class AStrategyUnit;
class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimSequence;

/**
 * An orderly with an order (PROJECT 1864): a single horseman who gallops from the army's staff to the unit the order is
 * for, waits for the player's controller to hand the order over, and rides back. The horse and rider are the cavalry's own
 * models; the horse rises and pitches with the gallop and throws up dust.
 */
UCLASS()
class STRATEGY1864_API AStrategyCourierRider : public AActor
{
    GENERATED_BODY()

public:
    AStrategyCourierRider();

    virtual void Tick(float DeltaSeconds) override;

    /** Ride from Home to the unit (it is followed if it moves). */
    void Send(const FVector& InHome, AStrategyUnit* To);
    /** The rider is with the unit. */
    bool HasArrived() const { return bArrived; }
    /** Seconds to go at the gallop (0 when there). */
    float SecondsLeft() const;
    /** The order is handed over: the rider goes back to the staff and is gone. */
    void Dismiss();

    static constexpr float GallopCmPerSecond = 1200.0f;

private:
    bool LoadAssets();

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Horse;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Rider;
    UPROPERTY() TObjectPtr<UStaticMesh> HorseModel;
    UPROPERTY() TObjectPtr<USkeletalMesh> RiderModel;
    UPROPERTY() TObjectPtr<UAnimSequence> SeatClip;

    TWeakObjectPtr<AStrategyUnit> Target;
    FVector Home = FVector::ZeroVector;
    bool bReady = false;
    bool bBack = false;
    bool bArrived = false;
    float Phase = 0.0f;
    float NextDust = 0.0f;
    float HorseScale = 1.0f;
    float HorseYaw = 0.0f;
    float HorseLift = 0.0f;
    float SaddleHeightCm = 120.0f;
    float RiderSeatCm = 48.0f;
};
