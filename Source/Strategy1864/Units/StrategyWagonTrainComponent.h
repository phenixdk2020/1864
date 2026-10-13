#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Logistics/StrategySupplyWagonUnit.h"
#include "StrategyWagonTrainComponent.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

/** One target per wagon; rendering and movement are batched by the owning train. */
UCLASS()
class STRATEGY1864_API AStrategyWagonTrain : public AStrategySupplyWagonUnit
{
    GENERATED_BODY()
public:
    AStrategyWagonTrain();
    bool bAmmunitionWagon = true;
    bool bMortarLoad = false;
    bool bEmptyCargoWagon = false;
    void RecordMortarLoss(bool bDestroyed);
    bool bMortarLossRecorded = false;
protected:
    virtual void BeginPlay() override;
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyWagonTrainComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStrategyWagonTrainComponent();
    // -1 means test defaults (only enabled by -Strategy1864Train); explicit zero means no train.
    UPROPERTY(EditAnywhere, Category="Strategy|Train")
    int32 WagonCount = -1;
    UPROPERTY(EditAnywhere, Category="Strategy|Train")
    int32 HorseCount = -1;
    void Configure(int32 InWagons, int32 InHorses);
    int32 GetTransportedMortarCount() const;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    struct FTrainSample { FVector Position; float Distance; };
    TArray<FTrainSample> TrainTrail;
    float TrainDistance = 0.f, TrailingCm = 6000.f, ScanSeconds = 0.f, ResupplySeconds = 0.f;
    bool bBuilt = false;
    TWeakObjectPtr<AStrategyUnit> TrainUnit;
    UPROPERTY(Transient) TArray<TObjectPtr<AStrategyWagonTrain>> TrainTargets;
    UPROPERTY(Transient) TArray<TObjectPtr<UInstancedStaticMeshComponent>> TrainMeshes;
    void BuildTrain();
    FVector SampleTrail(float Distance, float& Yaw) const;
    void ScanTrain();
    UInstancedStaticMeshComponent* MakeInstances(const TCHAR* Asset, const TCHAR* Name, bool bShadow);
    FTransform MeshTransform(UStaticMesh* Mesh, const FVector& Ground, const FRotator& Facing, float LengthCm, float WidthCm = 0.f) const;
};
