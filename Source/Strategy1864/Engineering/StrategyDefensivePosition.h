#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Units/StrategyUnit.h"
#include "StrategyDefensivePosition.generated.h"

class AStrategyNavigationObstacle;

UENUM(BlueprintType)
enum class EStrategyDefensivePositionType : uint8
{
    RiflePit,
    Breastwork,
    Trench,
    GunEmplacement,
    Barricade,
    AbatisObstacle,
    Redoubt
};

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyDefensivePosition : public AActor
{
    GENERATED_BODY()

public:
    AStrategyDefensivePosition();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    EStrategyDefensivePositionType PositionType =
        EStrategyDefensivePositionType::Breastwork;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    EStrategySide OwningSide = EStrategySide::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float LengthCm = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float DepthCm = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    int32 CapacityMen = 220;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float ConstructionProgress = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float Condition = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float FrontalHitMultiplier = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float RearHitMultiplier = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float ProtectedHalfAngleDegrees = 75.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Fieldworks")
    FName OccupyingUnitId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Fieldworks")
    bool bBreached = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Fieldworks")
    TObjectPtr<AStrategyNavigationObstacle> NavigationObstacle;

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    bool CanOccupy(const AStrategyUnit* Unit) const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    bool Occupy(AStrategyUnit* Unit);

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    void Vacate();

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    float ApplyStructuralDamage(float Damage);

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    float Repair(float Amount);

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    bool Capture(EStrategySide NewSide);

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    void MarkBreached(bool bNewBreached);

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    float CalculateIncomingHitMultiplier(const FVector& ShooterLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    bool SupportsArtilleryEmplacement() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    bool IsUsable() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    void RefreshNavigationObstacle();
};
