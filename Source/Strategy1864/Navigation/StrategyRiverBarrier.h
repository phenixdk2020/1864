#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyRiverBarrier.generated.h"

class AStrategyUnit;

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyRiverBarrier : public AActor
{
    GENERATED_BODY()

public:
    AStrategyRiverBarrier();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|River")
    FVector RiverAxisDirection = FVector(1.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|River")
    float RiverHalfWidthCm = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|River")
    FVector BankAApproachOffset = FVector(0.0f, -1800.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|River")
    FVector BankBApproachOffset = FVector(0.0f, 1800.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|River")
    float ExitClearanceCm = 3600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|River")
    int32 MaxConcurrentCrossers = 1;

    UFUNCTION(BlueprintCallable, Category="Strategy|River")
    bool TryAcquireCrossing(AStrategyUnit* Unit);

    UFUNCTION(BlueprintCallable, Category="Strategy|River")
    void ReleaseCrossing(AStrategyUnit* Unit);

    UFUNCTION(BlueprintPure, Category="Strategy|River")
    bool IsCrossingOwner(const AStrategyUnit* Unit) const;

    UFUNCTION(BlueprintPure, Category="Strategy|River")
    float SignedBankDistance(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|River")
    int32 GetBankSide(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|River")
    FVector GetBridgeApproachForSide(int32 Side) const;

    UFUNCTION(BlueprintPure, Category="Strategy|River")
    FVector GetBridgeExitForSide(int32 Side) const;

    UFUNCTION(BlueprintPure, Category="Strategy|River")
    bool IsInsideRiver(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|River")
    bool RequiresBankChange(const FVector& Start, const FVector& End) const;

    UFUNCTION(BlueprintPure, Category="Strategy|River")
    bool SameBankChordIntersectsRiver(const FVector& Start, const FVector& End) const;

private:
    FVector GetAcrossRiverNormal() const;

    UPROPERTY()
    TArray<TObjectPtr<AStrategyUnit>> ActiveCrossers;
};
