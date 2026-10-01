#pragma once

#include "CoreMinimal.h"
#include "StrategyArtilleryTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategyArtilleryMobilityState : uint8
{
    Limbered,
    Deploying,
    Deployed,
    Limbering,
    Manhandling,
    Abandoned,
    Disabled
};

UENUM(BlueprintType)
enum class EStrategyArtilleryFireMode : uint8
{
    ManualTarget,
    AutoTarget,
    HoldFire
};

UENUM(BlueprintType)
enum class EStrategyArtilleryAmmoType : uint8
{
    RoundShot,
    Shell,
    Shrapnel,
    Canister
};

UENUM(BlueprintType)
enum class EStrategyArtilleryTargetPriority : uint8
{
    Balanced,
    CounterBattery,
    Infantry,
    ClosestThreat,
    ConserveAmmo
};

UENUM(BlueprintType)
enum class EStrategyArtilleryOwnershipState : uint8
{
    Operational,
    Abandoned,
    Captured
};

USTRUCT(BlueprintType)
struct FStrategyArtilleryGunProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ProfileId = TEXT("FIELD_GUN_GENERIC");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CalibreMm = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AmmunitionFamilyTag = TEXT("FIELD_ARTILLERY_GENERIC");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinimumRangeCm = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaximumRangeCm = 180000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TraverseHalfAngleDegrees = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ReloadSeconds = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CrewRequiredPerGun = 8;
};
