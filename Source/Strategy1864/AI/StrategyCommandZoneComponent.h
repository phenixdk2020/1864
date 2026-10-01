#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyCommandZoneComponent.generated.h"

class AStrategyHQUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyCommandZoneComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyCommandZoneComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Command Zone")
    bool bDrawWhenSelected = true;

private:
    UPROPERTY()
    TObjectPtr<AStrategyHQUnit> OwnerHQ;
};
