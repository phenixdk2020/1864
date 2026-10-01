#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyWorldDebugComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyWorldDebugComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyWorldDebugComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    bool bDrawSelectedCommandTree = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    bool bDrawMissionTargets = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|QA")
    bool bDrawRoutes = true;

private:
    void DrawUnitRecursive(AStrategyUnit* Unit, int32 Depth) const;
    void DrawMissionForUnit(const AStrategyUnit* Unit) const;
    void DrawRouteForUnit(const AStrategyUnit* Unit) const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
