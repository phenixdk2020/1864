#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyHumanVisualTypes.h"
#include "StrategyEquipmentVisualComponent.generated.h"

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyEquipmentVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyEquipmentVisualComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Equipment")
    FStrategyEquipmentSocketMap SocketMap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Equipment")
    FName PrimaryWeaponId = TEXT("RIFLE_1864");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Equipment")
    FName SecondaryWeaponId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Equipment")
    FName ToolId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Equipment")
    bool bBayonetFixed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Equipment")
    bool bSabreDrawn = false;

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Equipment")
    FName GetSocketForSlot(EStrategyEquipmentSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Equipment")
    void SetBayonetFixed(bool bFixed)
    {
        bBayonetFixed = bFixed;
    }

    UFUNCTION(BlueprintCallable, Category="Strategy|Visual|Equipment")
    void SetSabreDrawn(bool bDrawn)
    {
        bSabreDrawn = bDrawn;
    }
};
