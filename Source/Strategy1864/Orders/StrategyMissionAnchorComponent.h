#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyOrderTypes.h"
#include "StrategyMissionAnchorComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyMissionAnchorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyMissionAnchorComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mission")
    FVector MissionAnchor = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mission")
    float MissionFacingYaw = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mission")
    bool bHasMissionFacing = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mission")
    EStrategyOrderType AnchoredMissionType = EStrategyOrderType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mission")
    float DefendReassertToleranceCm = 100.0f;

private:
    UFUNCTION()
    void HandleOrderChanged(const FStrategyOrder& NewOrder);

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
