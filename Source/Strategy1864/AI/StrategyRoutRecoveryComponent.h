#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyRoutRecoveryComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyRoutRecoveryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyRoutRecoveryComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Rout")
    float FallbackDistanceCm = 12000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Rout")
    float SafeEnemyDistanceCm = 18000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Rout")
    float MoraleRecoveryPerSecond = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Rout")
    float CohesionRecoveryPerSecond = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Rout")
    float RallyMoraleThreshold = 35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Rout")
    float RallyCohesionThreshold = 30.0f;

private:
    AStrategyUnit* FindNearestEnemy(float& OutDistanceCm) const;
    void StartFallback(AStrategyUnit* Enemy);
    void TryRally(float DeltaTime, float NearestEnemyDistanceCm);

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    bool bFallbackIssued = false;
};
