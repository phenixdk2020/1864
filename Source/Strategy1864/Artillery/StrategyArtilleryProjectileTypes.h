#pragma once

#include "CoreMinimal.h"
#include "../Artillery/StrategyArtilleryTypes.h"
#include "StrategyArtilleryProjectileTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategyProjectilePresentationStyle : uint8
{
    RoundShot,
    Shell,
    Shrapnel,
    Canister
};

USTRUCT(BlueprintType)
struct FStrategyArtilleryProjectileSpec
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 ShotSerial = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 GunIndex = 0;

    UPROPERTY(BlueprintReadOnly)
    EStrategyArtilleryAmmoType AmmoType =
        EStrategyArtilleryAmmoType::RoundShot;

    UPROPERTY(BlueprintReadOnly)
    EStrategyProjectilePresentationStyle Style =
        EStrategyProjectilePresentationStyle::RoundShot;

    UPROPERTY(BlueprintReadOnly)
    FVector LaunchLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector AimLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector PrimaryImpactLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    bool bAuthoritativeHit = false;

    UPROPERTY(BlueprintReadOnly)
    int32 ResolvedCasualties = 0;

    UPROPERTY(BlueprintReadOnly)
    float FlightSeconds = 1.0f;
};

USTRUCT(BlueprintType)
struct FStrategyArtilleryProjectileHistoryRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 ShotSerial = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 GunIndex = 0;

    UPROPERTY(BlueprintReadOnly)
    EStrategyArtilleryAmmoType AmmoType =
        EStrategyArtilleryAmmoType::RoundShot;

    UPROPERTY(BlueprintReadOnly)
    FVector LaunchLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector AimLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector ImpactLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    bool bAuthoritativeHit = false;

    UPROPERTY(BlueprintReadOnly)
    int32 ResolvedCasualties = 0;

    UPROPERTY(BlueprintReadOnly)
    float WorldTimeSeconds = 0.0f;
};
