#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyReconComponent.generated.h"

class AStrategyUnit;

UENUM(BlueprintType)
enum class EStrategyReconState : uint8
{
    Idle,
    Seek,
    Recon,
    Contact,
    Screen,
    Report,
    LastKnown
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyReconComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyReconComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Recon")
    EStrategyReconState ReconState = EStrategyReconState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Recon")
    float ReconDwellSeconds = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Recon")
    float ContactScreenSeconds = 4.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Recon")
    void OnScoutDestinationReached();

    UFUNCTION(BlueprintPure, Category="Strategy|Recon")
    bool IsReconActive() const;

private:
    UFUNCTION()
    void HandleOrderChanged(const FStrategyOrder& NewOrder);

    void CompleteRecon();

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float StateSeconds = 0.0f;
};
