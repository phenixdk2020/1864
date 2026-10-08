#include "StrategyHQVisualComponent.h"

#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "../Units/StrategyHQUnit.h"
#include "../AI/StrategyHQFollowComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

namespace
{
    // The same models as the squadrons and the couriers.
    const TCHAR* StaffHorsePath = TEXT("/Game/Units/Items/SM_Horse_Static.SM_Horse_Static");
    const TCHAR* StaffRiderPath = TEXT("/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864.SK_DK_Livgarden_1864");
    const TCHAR* StaffSeatPath = TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Reload_sitting.A_Reload_sitting");
}

UStrategyHQVisualComponent::UStrategyHQVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UStrategyHQVisualComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerHQ = Cast<AStrategyHQUnit>(GetOwner());
    if (OwnerHQ)
    {
        if (UStrategyHQFollowComponent* StaffFollow = OwnerHQ->FindComponentByClass<UStrategyHQFollowComponent>())
            AddTickPrerequisiteComponent(StaffFollow);
        LastLocation = OwnerHQ->GetActorLocation();
        ShownLocation = LastLocation;
        Yaw = OwnerHQ->GetActorRotation().Yaw;
    }
}

int32 UStrategyHQVisualComponent::RiderCount() const
{
    if (!OwnerHQ) { return 2; }
    return (OwnerHQ->HQLevel == EStrategyHQLevel::Brigade || OwnerHQ->HQLevel == EStrategyHQLevel::Division) ? 3 : 2;
}

bool UStrategyHQVisualComponent::Build()
{
    HorseModel = LoadObject<UStaticMesh>(nullptr, StaffHorsePath);
    RiderModel = LoadObject<USkeletalMesh>(nullptr, StaffRiderPath);
    SeatClip = LoadObject<UAnimSequence>(nullptr, StaffSeatPath);
    if (!HorseModel || !OwnerHQ)
    {
        return false;
    }
    const FBox B = HorseModel->GetBoundingBox();
    const FVector S = B.GetSize();
    const bool bAlongX = S.X >= S.Y;
    HorseScale = 250.0f / FMath::Max(1.0f, bAlongX ? S.X : S.Y);
    HorseYaw = bAlongX ? 0.0f : -90.0f;
    HorseLift = -B.Min.Z * HorseScale;
    SaddleHeightCm = S.Z * HorseScale * 0.72f;

    // The pivot keeps its own heading and ground height; the horsemen are placed round it.
    Pivot = NewObject<USceneComponent>(OwnerHQ);
    Pivot->SetupAttachment(OwnerHQ->GetRootComponent());
    Pivot->SetUsingAbsoluteLocation(true);
    Pivot->SetUsingAbsoluteRotation(true);
    Pivot->RegisterComponent();

    const int32 Count = RiderCount();
    for (int32 i = 0; i < Count; ++i)
    {
        FStaffRider R;
        // A pair side by side, or a wedge of three with the commander's aide in front.
        R.Offset = Count == 2 ? FVector(0.0f, (i == 0 ? -1.0f : 1.0f) * 130.0f, 0.0f)
            : (i == 0 ? FVector(0.0f, 0.0f, 0.0f) : FVector(-260.0f, (i == 1 ? -1.0f : 1.0f) * 230.0f, 0.0f));
        R.Phase = 0.37f * i;
        R.Horse = NewObject<UStaticMeshComponent>(OwnerHQ);
        R.Horse->SetupAttachment(Pivot);
        R.Horse->SetStaticMesh(HorseModel);
        R.Horse->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        R.Horse->bVisibleInRayTracing = false;
        R.Horse->SetRelativeScale3D(FVector(HorseScale));
        R.Horse->RegisterComponent();
        if (RiderModel)
        {
            R.Rider = NewObject<USkeletalMeshComponent>(OwnerHQ);
            R.Rider->SetupAttachment(Pivot);
            R.Rider->SetSkeletalMesh(RiderModel);
            R.Rider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            R.Rider->bVisibleInRayTracing = false;
            R.Rider->RegisterComponent();
            if (SeatClip)
            {
                R.Rider->PlayAnimation(SeatClip, false);
                R.Rider->SetPosition(0.05f, false);
                R.Rider->bPauseAnims = true;
            }
        }
        Riders.Add(R);
    }
    return true;
}

void UStrategyHQVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!OwnerHQ || !GetWorld())
    {
        return;
    }
    if (!bBuilt)
    {
        bBuilt = true;
        Build();
    }
    // The grey block is not drawn: the staff is the horsemen.
    if (OwnerHQ->QAPlaceholderMesh && OwnerHQ->QAPlaceholderMesh->IsVisible())
    {
        OwnerHQ->QAPlaceholderMesh->SetVisibility(false);
    }
    if (!Pivot || DeltaTime <= KINDA_SMALL_NUMBER)
    {
        return;
    }
    FVector Here = OwnerHQ->GetActorLocation();
    Here.Z = UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, Here);
    FStrategyVisualFormationPath::SmoothPosition(ShownLocation, ShownVelocity, Here, DeltaTime, 24.f);
    Here = ShownLocation;
    FVector Moved = Here - LastLocation;
    Moved.Z = 0.0f;
    const float Speed = Moved.Size() / DeltaTime;
    LastLocation = Here;
    if (Speed > 30.0f)
    {
        // Turn with the way it goes, at a horse's pace (not at once).
        Yaw = FStrategyVisualFormationPath::SmoothFacing(Yaw, YawVelocity, Moved.Rotation().Yaw, DeltaTime, 90.f);
    }
    Pace = FMath::Fmod(Pace + FMath::Clamp(Speed / 600.0f, 0.0f, 1.5f) * 1.6f * DeltaTime, 1.0f);
    const float Walk = FMath::Clamp(Speed / 30.f, 0.f, 1.f);
    Pivot->SetWorldLocation(Here);
    Pivot->SetWorldRotation(FRotator(0.0f, Yaw, 0.0f));
    for (FStaffRider& R : Riders)
    {
        const float Wave = FMath::Sin((Pace + R.Phase) * 2.0f * PI);
        const float Rise = Walk * 5.0f * (0.5f + 0.5f * Wave);
        if (R.Horse)
        {
            R.Horse->SetRelativeLocation(R.Offset + FVector(0.0f, 0.0f, HorseLift + Rise));
            R.Horse->SetRelativeRotation(FRotator(Walk * 1.5f * Wave, HorseYaw, 0.0f));
        }
        if (R.Rider)
        {
            R.Rider->SetRelativeLocation(R.Offset + FVector(-10.0f, 0.0f, SaddleHeightCm - 48.0f + Rise));
            R.Rider->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        }
    }
}
