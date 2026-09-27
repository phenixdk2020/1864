#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TacticalPlayerController.generated.h"

class AInfantryCompany;

/**
 * Battle input: WASD / screen-edge pan, wheel zoom, Q/E rotate, left click selects a company.
 * Keys are polled directly so the prototype runs without input assets.
 */
UCLASS()
class GAME1864_API ATacticalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATacticalPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Selection")
	AInfantryCompany* GetSelectedCompany() const { return SelectedCompany.Get(); }

	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bEdgePan = true;

	/** Edge-pan band in pixels. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float EdgePanMargin = 12.f;

protected:
	virtual void BeginPlay() override;

private:
	void UpdateCamera(float DeltaTime);
	void UpdateSelection();

	TWeakObjectPtr<AInfantryCompany> SelectedCompany;
};
