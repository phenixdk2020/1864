#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StrategyAnimationManifestLibrary.generated.h"

UCLASS()
class STRATEGY1864_API UStrategyAnimationManifestLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Animation")
    static TArray<FName> GetCoreHumanAnimationNames();

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Animation")
    static TArray<FName> GetMountedAnimationNames();

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Animation")
    static TArray<FName> GetHorseAnimationNames();

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Animation")
    static TArray<FName> GetArtilleryCrewAnimationNames();
};
