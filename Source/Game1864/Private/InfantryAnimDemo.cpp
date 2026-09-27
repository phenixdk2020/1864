#include "InfantryAnimDemo.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UAnimSequence* LoadAnim(const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/Game/Units/Infantry/Animations/A_Infantry_%s.A_Infantry_%s"), Name, Name);
		return LoadObject<UAnimSequence>(nullptr, *Path);
	}

	FInfantryDemoStep Step(const TCHAR* Anim, float Duration, EMusketHold Hold)
	{
		FInfantryDemoStep S;
		S.Animation = LoadAnim(Anim);
		S.Duration = Duration;
		S.Musket = Hold;
		return S;
	}
}

AInfantryAnimDemo::AInfantryAnimDemo()
{
	PrimaryActorTick.bCanEverTick = true;
	// Also run in Simulate / editor preview worlds.
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	// The imported figure faces +Y; turn it so the actor's forward (+X) is the soldier's front.
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	Musket = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Musket"));
	Musket->SetupAttachment(Root);
	Musket->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMesh(TEXT("/Game/Units/Infantry/Infantry_Rigged/SkeletalMeshes/Infantry_Rigged.Infantry_Rigged"));
	if (BodyMesh.Succeeded())
	{
		Body->SetSkeletalMesh(BodyMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MusketMesh(TEXT("/Game/Units/Infantry/Musket/SM_Musket/StaticMeshes/SM_Musket.SM_Musket"));
	if (MusketMesh.Succeeded())
	{
		Musket->SetStaticMesh(MusketMesh.Object);
	}
}

void AInfantryAnimDemo::BeginPlay()
{
	Super::BeginPlay();

	if (Steps.Num() == 0)
	{
		// Walk forward (Mixamo root motion: 150 cm per 0.97 s loop), stand, load, aim, fire twice, fall.
		FInfantryDemoStep Walk = Step(TEXT("Walking"), 2.9f, EMusketHold::Upright);
		Walk.RootMotionPerLoop = 150.f;
		Steps.Add(Walk);
		Steps.Add(Step(TEXT("Idle"), 2.5f, EMusketHold::Upright));
		Steps.Add(Step(TEXT("Reloading"), 3.3f, EMusketHold::TwoHanded));
		Steps.Add(Step(TEXT("RifleAimingIdle"), 1.0f, EMusketHold::TwoHanded));
		Steps.Add(Step(TEXT("FiringRifle"), 0.54f, EMusketHold::TwoHanded));
		Steps.Add(Step(TEXT("RifleAimingIdle"), 0.8f, EMusketHold::TwoHanded));
		// Walking To Dying: skip the first 1.15 s of walking; the fall starts there with the hips 118 cm forward.
		FInfantryDemoStep Die = Step(TEXT("WalkingToDying"), 1.75f, EMusketHold::LockedToHand);
		Die.StartTime = 1.15f;
		Die.RootOffsetAtStart = 118.f;
		Die.bHoldLastFrame = true;
		Steps.Add(Die);
	}

	RightHand = FindBone(TEXT("RightHand"));
	LeftHand = FindBone(TEXT("LeftHand"));
	StartLocation = GetActorLocation();
	StartStep(0);
}

FName AInfantryAnimDemo::FindBone(const TCHAR* Suffix) const
{
	// Bone names may keep Mixamo's "mixamorig:" prefix or have it converted; match on the ending.
	const int32 Num = Body->GetNumBones();
	for (int32 i = 0; i < Num; ++i)
	{
		const FName Name = Body->GetBoneName(i);
		if (Name.ToString().EndsWith(Suffix))
		{
			return Name;
		}
	}
	return NAME_None;
}

void AInfantryAnimDemo::StartStep(int32 Index)
{
	if (!Steps.IsValidIndex(Index))
	{
		return;
	}
	StepIndex = Index;
	StepTime = 0.f;

	const FInfantryDemoStep& S = Steps[Index];
	Travelled -= S.RootOffsetAtStart;
	if (S.Animation)
	{
		Body->PlayAnimation(S.Animation, !S.bHoldLastFrame);
		Body->SetPosition(S.StartTime, false);
	}
	UpdateRootMotion();

	if (S.Musket == EMusketHold::LockedToHand && RightHand != NAME_None)
	{
		MusketInHand = Musket->GetComponentTransform().GetRelativeTransform(Body->GetSocketTransform(RightHand));
	}
}

void AInfantryAnimDemo::UpdateRootMotion()
{
	SetActorLocation(StartLocation + GetActorForwardVector() * Travelled);
}

void AInfantryAnimDemo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Steps.IsValidIndex(StepIndex))
	{
		return;
	}

	const FInfantryDemoStep& S = Steps[StepIndex];
	const float Length = S.Animation ? S.Animation->GetPlayLength() : 0.f;
	const float Before = StepTime;
	StepTime += DeltaSeconds;

	// Carry the clip's forward travel over each loop boundary.
	if (Length > 0.f && S.RootMotionPerLoop != 0.f && !S.bHoldLastFrame)
	{
		const int32 LoopsBefore = FMath::FloorToInt((S.StartTime + Before) / Length);
		const int32 LoopsNow = FMath::FloorToInt((S.StartTime + StepTime) / Length);
		if (LoopsNow > LoopsBefore)
		{
			Travelled += S.RootMotionPerLoop * (LoopsNow - LoopsBefore);
			UpdateRootMotion();
		}
	}

	PlaceMusket();

	const bool bLast = StepIndex == Steps.Num() - 1;
	if (StepTime >= S.Duration + (bLast ? RestartDelay : 0.f))
	{
		if (bLast)
		{
			Travelled = 0.f;
			StartStep(0);
		}
		else
		{
			// Leaving a looping walk: bank the partial loop too, so the next clip starts where he stands.
			if (Length > 0.f && S.RootMotionPerLoop != 0.f)
			{
				const float Frac = FMath::Fmod(S.StartTime + StepTime, Length) / Length;
				Travelled += S.RootMotionPerLoop * Frac;
			}
			StartStep(StepIndex + 1);
		}
	}
}

void AInfantryAnimDemo::PlaceMusket()
{
	if (!Musket->GetStaticMesh() || RightHand == NAME_None)
	{
		return;
	}

	const FVector Right = Body->GetBoneLocation(RightHand);
	const EMusketHold Hold = Steps[StepIndex].Musket;

	if (Hold == EMusketHold::LockedToHand)
	{
		Musket->SetWorldTransform(MusketInHand * Body->GetSocketTransform(RightHand));
		return;
	}

	// The musket mesh runs along its local +Z from the butt (origin) to the bayonet tip.
	FVector Along = FVector::UpVector;
	if (Hold == EMusketHold::TwoHanded && LeftHand != NAME_None)
	{
		const FVector Left = Body->GetBoneLocation(LeftHand);
		const FVector Dir = Left - Right;
		if (Dir.SizeSquared() > 25.f)
		{
			Along = Dir.GetSafeNormal();
		}
	}
	const FRotator Rot = FRotationMatrix::MakeFromZX(Along, GetActorRightVector()).Rotator();
	Musket->SetWorldLocationAndRotation(Right - Along * GripFromButt, Rot);
}
