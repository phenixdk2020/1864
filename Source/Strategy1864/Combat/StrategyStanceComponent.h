#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyStanceComponent.generated.h"

UENUM(BlueprintType)
enum class EStrategyStance : uint8
{
    Standing = 0,
    Prone = 1,
    Kneeling = 2
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyStanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyStanceComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Stance")
    EStrategyStance Stance = EStrategyStance::Standing;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Stance")
    float KneelingMovementMultiplier = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Stance")
    float KneelingReloadMultiplier = 1.03f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Stance")
    float KneelingIncomingHitMultiplier = 0.82f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Stance")
    float ProneMovementMultiplier = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Stance")
    float ProneReloadMultiplier = 1.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Stance")
    float ProneIncomingHitMultiplier = 0.65f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Stance")
    bool SetStance(EStrategyStance NewStance);

    UFUNCTION(BlueprintPure, Category="Strategy|Stance")
    float GetMovementMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Stance")
    float GetReloadMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Stance")
    float GetIncomingHitMultiplier() const;
};
