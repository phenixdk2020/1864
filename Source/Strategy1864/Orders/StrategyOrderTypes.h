#pragma once

#include "CoreMinimal.h"
#include "StrategyOrderTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategyOrderType : uint8
{
    None        UMETA(DisplayName = "None"),
    Move        UMETA(DisplayName = "Move"),
    AttackHere  UMETA(DisplayName = "Angrib her"),
    DefendHere  UMETA(DisplayName = "Forsvar her"),
    Hold        UMETA(DisplayName = "Hold"),
    Advance     UMETA(DisplayName = "Ryk frem"),
    Withdraw    UMETA(DisplayName = "Tilbagetræk"),
    Assemble    UMETA(DisplayName = "Saml"),
    ScoutHere   UMETA(DisplayName = "Spejd her"),
    Charge      UMETA(DisplayName = "Charge"),
    ArtilleryFireMission UMETA(DisplayName = "Artillery fire mission")
};

UENUM(BlueprintType)
enum class EStrategyOrderExecutionState : uint8
{
    Idle,
    PendingTarget,
    Pending,
    Executing,
    Completed,
    Failed,
    Superseded
};

UENUM(BlueprintType)
enum class EStrategyCommandVisualState : uint8
{
    Red,
    Blue,
    Green
};

UENUM(BlueprintType)
enum class EStrategyOrderAuthority : uint8
{
    InheritedAI,
    OfficerAI,
    DirectPlayer
};

USTRUCT(BlueprintType)
struct FStrategyOrder
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EStrategyOrderType Type = EStrategyOrderType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FacingYaw = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasFacing = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EStrategyOrderAuthority Authority = EStrategyOrderAuthority::InheritedAI;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 OrderSerial = 0;

    bool IsStandingIntent() const
    {
        return Type == EStrategyOrderType::DefendHere ||
               Type == EStrategyOrderType::Hold ||
               Type == EStrategyOrderType::ArtilleryFireMission;
    }

    bool IsValidOrder() const
    {
        return Type != EStrategyOrderType::None;
    }
};
