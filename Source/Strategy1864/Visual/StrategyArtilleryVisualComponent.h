#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyVisualFormationPath.h"
#include "StrategyArtilleryVisualComponent.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * A battery's guns on the field (PROJECT 1864): the field gun model (SM_Cannon_1864) or the siege mortar
 * (SM_Mortar_1864) once for every piece, in line at fifteen metres when unlimbered and in column behind each
 * other on the march; a disabled piece stands askew, a destroyed one lies tipped over where it stood. Replaces
 * the QA box when the models are imported.
 */
UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryVisualComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Metres between the pieces in line, and on the march. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Artillery")
    float LineIntervalCm = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Artillery")
    float ColumnIntervalCm = 900.0f;

private:
    void Rebuild();
    FStrategyVisualFormationPath ArtilleryVisualPath;
    TArray<FTransform> GunGoals;
    TArray<FVector> GunVelocities;
    TArray<float> GunTurnVelocities;

    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> Guns;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> GunMesh;

    bool bMortar = false;
    int32 BuiltTotal = -1;
    int32 BuiltWorking = -1;
    int32 BuiltDestroyed = -1;
    bool bBuiltLimbered = false;
};
