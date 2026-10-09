#pragma once

#include "CoreMinimal.h"
#include "StrategyOrderTypes.generated.h"

class AStrategyUnit;

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
    ArtilleryFireMission UMETA(DisplayName = "Artillery fire mission"),
    Disengage UMETA(DisplayName = "Afbryd")
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

    /** Side-step: the unit keeps its front to FacingYaw while it moves (at a slower pace to the side or back) instead of
     *  turning to the way it goes. For short moves in front of the enemy. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bKeepFacing = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EStrategyOrderAuthority Authority = EStrategyOrderAuthority::InheritedAI;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 OrderSerial = 0;

    /** Player route, including the final destination. Travels with delayed orders. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> Waypoints;

    UPROPERTY()
    FGuid WaypointRouteId;

    UPROPERTY()
    int32 NextWaypointIndex = 0;

    UPROPERTY()
    FVector GroupRouteOffset = FVector::ZeroVector;

    /** Enemy reference keeps the requested fire-range destination consistent through delivery. */
    UPROPERTY()
    TWeakObjectPtr<AStrategyUnit> AttackTarget;

    /** Internal phase of a fighting withdrawal; false means halt and fire. */
    UPROPERTY()
    bool bDisengageStep = false;

    FVector ActiveDestination() const
    {
        return Waypoints.IsValidIndex(NextWaypointIndex) ? Waypoints[NextWaypointIndex] : TargetLocation;
    }

    bool IsStandingIntent() const
    {
        return Type == EStrategyOrderType::DefendHere ||
               Type == EStrategyOrderType::Hold ||
               Type == EStrategyOrderType::ArtilleryFireMission || Type == EStrategyOrderType::Disengage;
    }

    bool IsValidOrder() const
    {
        return Type != EStrategyOrderType::None;
    }
};
