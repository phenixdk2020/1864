#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InfantryAnimDemo.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UAnimSequence;

/** How the musket is held during a demo step. */
UENUM(BlueprintType)
enum class EMusketHold : uint8
{
	/** Upright in the right hand (walking, standing). */
	Upright,
	/** Along the line from the right hand to the left hand (aiming, firing, loading). */
	TwoHanded,
	/** Frozen relative to the right hand from the moment the step starts (falling). */
	LockedToHand
};

USTRUCT(BlueprintType)
struct FInfantryDemoStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	TObjectPtr<UAnimSequence> Animation;

	/** Where in the clip to start, seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float StartTime = 0.f;

	/** How long the step lasts, seconds. Longer than the clip = the clip loops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float Duration = 1.f;

	/** Forward root motion baked into one loop of the clip, cm; carried over at each loop so the walk does not snap back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float RootMotionPerLoop = 0.f;

	/** Forward hip offset at StartTime, cm; subtracted so starting mid-clip does not pop the figure forward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float RootOffsetAtStart = 0.f;

	/** Stop on the last frame instead of looping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	bool bHoldLastFrame = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	EMusketHold Musket = EMusketHold::Upright;
};

/**
 * Plays the rigged infantryman through a fixed sequence (walk, stand, load, aim, fire, die) and
 * loops. Place it in a level and press Simulate or Play to watch it. The musket is a separate mesh
 * placed from the hand bones every frame, since the Mixamo clips carry no weapon.
 */
UCLASS()
class GAME1864_API AInfantryAnimDemo : public AActor
{
	GENERATED_BODY()

public:
	AInfantryAnimDemo();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	TArray<FInfantryDemoStep> Steps;

	/** Pause on the final pose before starting over, seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float RestartDelay = 3.f;

	/** Distance from the butt to the right-hand grip along the musket, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float GripFromButt = 42.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Musket;

private:
	void StartStep(int32 Index);
	void UpdateRootMotion();
	void PlaceMusket();
	FName FindBone(const TCHAR* Suffix) const;

	int32 StepIndex = INDEX_NONE;
	float StepTime = 0.f;
	float Travelled = 0.f;
	FVector StartLocation = FVector::ZeroVector;
	FName RightHand, LeftHand;
	FTransform MusketInHand = FTransform::Identity;
};
