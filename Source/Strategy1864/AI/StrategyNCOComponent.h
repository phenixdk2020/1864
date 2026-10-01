#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyNCOComponent.generated.h"

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyNCOComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyNCOComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|NCO")
    int32 NCOStrength = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|NCO", meta=(ClampMin="0.0", ClampMax="100.0"))
    float NCOQuality = 55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|NCO")
    bool bOfficerAvailable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|NCO")
    float OfficerLossCommandFloor = 0.35f;

    UFUNCTION(BlueprintCallable, Category="Strategy|NCO")
    void SetOfficerAvailable(bool bAvailable);

    UFUNCTION(BlueprintCallable, Category="Strategy|NCO")
    int32 ApplyNCOLoss(int32 RequestedLoss);

    UFUNCTION(BlueprintPure, Category="Strategy|NCO")
    float GetCadreIntegrity() const;

    UFUNCTION(BlueprintPure, Category="Strategy|NCO")
    float GetLocalCommandContinuity() const;

    UFUNCTION(BlueprintPure, Category="Strategy|NCO")
    float GetFormationSpeedMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|NCO")
    float GetReloadDisciplineMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|NCO")
    float GetRallyMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|NCO")
    float GetResponseTimeMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|NCO")
    float GetDetachmentControlMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|NCO")
    bool CanMaintainLocalCommand() const;
};
