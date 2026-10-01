#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StrategyQARuntimeSubsystem.generated.h"

class AStrategyCameraPawn;
class AStrategyOOBTestScenario;

UCLASS()
class STRATEGY1864_API UStrategyQARuntimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    AStrategyOOBTestScenario* FindOrSpawnScenario(UWorld& World);
    AStrategyCameraPawn* EnsureStrategyCamera(UWorld& World);
    void FocusCameraOnBattlefield(AStrategyCameraPawn* CameraPawn) const;
};
