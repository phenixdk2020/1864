#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyMortarDeploymentComponent.generated.h"

UENUM(BlueprintType)
enum class EStrategyMortarClass : uint8
{
    LightHand,
    HeavySiege
};

UENUM(BlueprintType)
enum class EStrategyMortarMobilityState : uint8
{
    Transport,
    Emplacing,
    Deployed,
    Packing,
    Disabled
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyMortarDeploymentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyMortarDeploymentComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    EStrategyMortarClass MortarClass = EStrategyMortarClass::HeavySiege;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mortar")
    EStrategyMortarMobilityState MobilityState =
        EStrategyMortarMobilityState::Transport;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float BaseEmplaceSeconds = 75.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float BasePackSeconds = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    int32 WorkingPartyStrength = 24;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    int32 HorseStrength = 36;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    int32 WagonStrength = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float PieceWeightKg = 650.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    float TerrainPreparationFactor = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mortar")
    float TransitionRemainingSeconds = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    bool RequestEmplace();

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    bool RequestPack();

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    void DisableMortar();

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    bool CanMoveNormally() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    bool CanFire() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    float GetTransportMobilityFactor() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    float GetManhandlingFactor() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    float GetWorkRate() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Mortar")
    void SetTerrainPreparationFactor(float NewFactor);

private:
    void CompleteTransition();
};
