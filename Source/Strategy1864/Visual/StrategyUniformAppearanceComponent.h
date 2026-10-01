#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyHumanVisualTypes.h"
#include "StrategyUniformAppearanceComponent.generated.h"

class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyUniformAppearanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyUniformAppearanceComponent();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform")
    FStrategyHumanVisualProfile VisualProfile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform")
    FStrategyUniformPreset BasePreset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform")
    FStrategyUniformOverrides Overrides;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform")
    TArray<FName> TargetMeshComponentNames;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform|Parameters")
    FName CoatParameter = TEXT("CoatColor");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform|Parameters")
    FName TrousersParameter = TEXT("TrouserColor");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform|Parameters")
    FName FacingsParameter = TEXT("FacingColor");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform|Parameters")
    FName HeadgearParameter = TEXT("HeadgearColor");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform|Parameters")
    FName LeatherParameter = TEXT("LeatherColor");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform|Parameters")
    FName AccentParameter = TEXT("AccentColor");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Uniform|Parameters")
    FName MetalParameter = TEXT("MetalColor");

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Uniform")
    void ApplyAppearance();

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Uniform")
    void SetPreset(const FStrategyUniformPreset& NewPreset, bool bApplyNow = true);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Uniform")
    void SetOverrides(const FStrategyUniformOverrides& NewOverrides, bool bApplyNow = true);

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Uniform")
    void ClearOverrides(bool bApplyNow = true);

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Uniform")
    FStrategyUniformColors GetResolvedColors() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Uniform")
    bool CanOverrideHistoricalPalette() const
    {
        return !BasePreset.bLockHistoricalPalette;
    }

private:
    bool ShouldTargetMesh(const USkeletalMeshComponent* Mesh) const;
    void ApplyColorsToMesh(
        USkeletalMeshComponent* Mesh,
        const FStrategyUniformColors& Colors);
};
