#include "StrategyHQVisualComponent.h"

#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "../Units/StrategyHQUnit.h"
#include "../AI/StrategyHQFollowComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "StrategyCrowdModel.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

namespace
{
    // The fallback: the old static horse and a seated soldier.
    const TCHAR* StaffHorsePath = TEXT("/Game/Units/Items/SM_Horse_Static.SM_Horse_Static");
    const TCHAR* StaffRiderPath = TEXT("/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864.SK_DK_Livgarden_1864");
    const TCHAR* StaffSeatPath = TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Reload_sitting.A_Reload_sitting");
    // The cavalry's rigged horse and the rider clips (StrategyCavalryVisualComponent), ridden by the officer (Final_v5).
    const TCHAR* StaffHorseSkPath = TEXT("/Game/Units/Horse/SK_Horse_1864.SK_Horse_1864");
    const TCHAR* StaffHorseClipPaths[5] = {
        TEXT("/Game/Units/Horse/A_Horse_Stand.A_Horse_Stand"), TEXT("/Game/Units/Horse/A_Horse_M2M_Walk.A_Horse_M2M_Walk"),
        TEXT("/Game/Units/Horse/A_Horse_M2M_Trot.A_Horse_M2M_Trot"), TEXT("/Game/Units/Horse/A_Horse_Canter.A_Horse_Canter"),
        TEXT("/Game/Units/Horse/A_Horse_Gallop.A_Horse_Gallop")};
    const TCHAR* StaffRiderClipPaths[5] = {   // index 0 is the held seat (StaffSeatPath)
        TEXT(""), TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavWalk.A_Combat_CavWalk"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavTrotRise.A_Combat_CavTrotRise"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavCanter.A_Combat_CavCanter"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavGallop.A_Combat_CavGallop")};
    const TCHAR* StaffScabbardPath = TEXT("/Game/Units/Human/Officer/SM_Officer_Scabbard.SM_Officer_Scabbard");
    const TCHAR* StaffSabrePath = TEXT("/Game/Units/Human/Officer/SM_Officer_Saber.SM_Officer_Saber");
    const FName StaffSaddleBone(TEXT("saddle_root"));
    // As the cavalry: the horse's length, the gait thresholds (cm/s), each horse clip's own ground speed and the play rate range.
    const float StaffHorseLengthCm = 219.0f;
    // The gait changes where the clip's stride still covers the ground (no sliding hooves, 2026-11-08).
    const float StaffGaitThresholds[4] = {80.f, 190.f, 400.f, 640.f};
    const float StaffClipSpeedCmS[5] = {0.f, 132.f, 250.f, 357.f, 757.f};
    const float StaffRateMin[5] = {1.f, 0.45f, 0.75f, 0.85f, 0.8f};
    const float StaffRateMax[5] = {1.f, 1.6f, 1.6f, 1.8f, 1.6f};
    const float StaffRiderSeatCm = 48.0f;   // the seated pose's pelvis above its root
    const float StaffSaddleLiftCm = 6.0f;   // the pelvis above the saddle_root bone
    // The sabre and the scabbard on the officer's hips (Final_v5 notes.md: the sabre sheathed shares the scabbard's transform).
    const FTransform StaffScabbardOnHips(FRotator(-20.730171f, 84.857239f, -90.000003f), FVector(33.243320f, -9.561045f, -19.432434f), FVector(1.106196f));
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

bool UStrategyHQVisualComponent::GetDrawnCenter(FVector& OutTrainCenter) const
{
    if (!bBuilt) return false;
    OutTrainCenter = ShownLocation;
    return true;
}

int32 UStrategyHQVisualComponent::RiderCount() const
{
    if (!OwnerHQ) { return 2; }
    return (OwnerHQ->HQLevel == EStrategyHQLevel::Brigade || OwnerHQ->HQLevel == EStrategyHQLevel::Division) ? 3 : 2;
}

bool UStrategyHQVisualComponent::Build()
{
    if (!OwnerHQ) return false;
    HorseModel = LoadObject<UStaticMesh>(nullptr, StaffHorsePath);
    SeatClip = LoadObject<UAnimSequence>(nullptr, StaffSeatPath);
    // The rider: the officer of the staff's nation, else the old soldier.
    const FString StaffNation = OwnerHQ->GetNationCodeOrDefault();
    RiderModel = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Units/Human/Officer/SK_Officer_1864_%s.SK_Officer_1864_%s"), *StaffNation, *StaffNation),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!RiderModel) RiderModel = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Units/Human/Officer/SK_Officer_1864_DK.SK_Officer_1864_DK"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    bOfficerRider = RiderModel != nullptr;
    if (!RiderModel) RiderModel = LoadObject<USkeletalMesh>(nullptr, StaffRiderPath);
    // The rigged horse needs all five gaits; the rider's clips are optional (the held seat stands in).
    HorseSkeletal = LoadObject<USkeletalMesh>(nullptr, StaffHorseSkPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    HorseGaitClips.Reset();
    RiderGaitClips.Reset();
    RiderGaitClips.Add(SeatClip);
    for (int32 StaffClipIndex = 0; StaffClipIndex < 5; ++StaffClipIndex)
    {
        HorseGaitClips.Add(LoadObject<UAnimSequence>(nullptr, StaffHorseClipPaths[StaffClipIndex], nullptr, LOAD_NoWarn | LOAD_Quiet));
        if (StaffClipIndex > 0) RiderGaitClips.Add(LoadObject<UAnimSequence>(nullptr, StaffRiderClipPaths[StaffClipIndex], nullptr, LOAD_NoWarn | LOAD_Quiet));
    }
    bSkeletalHorse = HorseSkeletal && !HorseGaitClips.ContainsByPredicate([](const TObjectPtr<UAnimSequence>& StaffGaitClip) { return !StaffGaitClip; });
    if (bSkeletalHorse)
    {
        // As the cavalry: scaled to a horse's length, feet on the ground, the forward sign from the rest pose (pelvis to head).
        const FBoxSphereBounds StaffBounds = HorseSkeletal->GetBounds();
        const FVector StaffSize = StaffBounds.BoxExtent * 2.0;
        HorseScale = StaffHorseLengthCm / FMath::Max(1.0f, float(FMath::Max(StaffSize.X, StaffSize.Y)));
        HorseLift = -(StaffBounds.Origin.Z - StaffBounds.BoxExtent.Z) * HorseScale;
        const FReferenceSkeleton& StaffRef = HorseSkeletal->GetRefSkeleton();
        HorseYaw = 0.0f;
        if (StaffRef.FindBoneIndex(TEXT("head")) != INDEX_NONE && StaffRef.FindBoneIndex(TEXT("pelvis")) != INDEX_NONE)
        {
            const FVector StaffForward = HorseSkeletal->GetComposedRefPoseMatrix(FName(TEXT("head"))).GetOrigin() -
                HorseSkeletal->GetComposedRefPoseMatrix(FName(TEXT("pelvis"))).GetOrigin();
            HorseYaw = -FMath::RadiansToDegrees(FMath::Atan2(StaffForward.Y, StaffForward.X));
        }
        bSaddleBone = StaffRef.FindBoneIndex(StaffSaddleBone) != INDEX_NONE;
        SaddleHeightCm = bSaddleBone ? HorseSkeletal->GetComposedRefPoseMatrix(StaffSaddleBone).GetOrigin().Z * HorseScale : StaffSize.Z * HorseScale * 0.72f;
    }
    else if (HorseModel)
    {
        const FBox B = HorseModel->GetBoundingBox();
        const FVector S = B.GetSize();
        const bool bAlongX = S.X >= S.Y;
        HorseScale = 250.0f / FMath::Max(1.0f, bAlongX ? S.X : S.Y);
        HorseYaw = bAlongX ? 0.0f : -90.0f;
        HorseLift = -B.Min.Z * HorseScale;
        SaddleHeightCm = S.Z * HorseScale * 0.72f;
    }
    else
    {
        return false;
    }
    UStaticMesh* StaffScabbardMesh = bOfficerRider ? LoadObject<UStaticMesh>(nullptr, StaffScabbardPath, nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
    UStaticMesh* StaffSabreMesh = bOfficerRider ? LoadObject<UStaticMesh>(nullptr, StaffSabrePath, nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-STAFF %s: %s horse, rider %s, rider clips walk %d trot %d canter %d gallop %d, saddle %.0f cm"),
        *OwnerHQ->StableUnitId.ToString(), bSkeletalHorse ? TEXT("rigged") : TEXT("static"), RiderModel ? *RiderModel->GetName() : TEXT("-"),
        RiderGaitClips[1] != nullptr, RiderGaitClips[2] != nullptr, RiderGaitClips[3] != nullptr, RiderGaitClips[4] != nullptr, SaddleHeightCm);

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
        R.GaitPhase = 0.37f * i;
        if (bSkeletalHorse)
        {
            R.HorseSk = NewObject<USkeletalMeshComponent>(OwnerHQ);
            R.HorseSk->SetupAttachment(Pivot);
            R.HorseSk->SetSkeletalMeshAsset(HorseSkeletal);
            R.HorseSk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            R.HorseSk->bVisibleInRayTracing = false;
            R.HorseSk->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
            R.HorseSk->SetRelativeScale3D(FVector(HorseScale));
            R.HorseSk->RegisterComponent();
            PlayStaffHorseClip(R, HorseGaitClips[0], 1.0f);
        }
        else
        {
            R.Horse = NewObject<UStaticMeshComponent>(OwnerHQ);
            R.Horse->SetupAttachment(Pivot);
            R.Horse->SetStaticMesh(HorseModel);
            R.Horse->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            R.Horse->bVisibleInRayTracing = false;
            R.Horse->SetRelativeScale3D(FVector(HorseScale));
            R.Horse->RegisterComponent();
        }
        if (RiderModel)
        {
            R.Rider = NewObject<USkeletalMeshComponent>(OwnerHQ);
            R.Rider->SetupAttachment(Pivot);
            R.Rider->SetSkeletalMesh(RiderModel);
            R.Rider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            R.Rider->bVisibleInRayTracing = false;
            R.Rider->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
            R.Rider->RegisterComponent();
            if (SeatClip)
            {
                R.Rider->PlayAnimation(SeatClip, false);
                R.Rider->SetPosition(0.05f, false);
                R.Rider->bPauseAnims = true;
            }
            // The officer's sabre rides in its scabbard at his left hip.
            for (UStaticMesh* StaffPropMesh : { StaffScabbardMesh, StaffSabreMesh })
            {
                if (!StaffPropMesh) continue;
                UStaticMeshComponent* StaffProp = NewObject<UStaticMeshComponent>(OwnerHQ);
                StaffProp->SetStaticMesh(StaffPropMesh);
                StaffProp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                StaffProp->SetCastShadow(false);
                StaffProp->bVisibleInRayTracing = false;
                StaffProp->SetupAttachment(R.Rider, TEXT("Hips"));
                StaffProp->SetRelativeTransform(StaffScabbardOnHips);
                StaffProp->RegisterComponent();
                (StaffPropMesh == StaffScabbardMesh ? R.Scabbard : R.Sabre) = StaffProp;
            }
        }
        Riders.Add(R);
    }
    return true;
}

void UStrategyHQVisualComponent::PlayStaffHorseClip(FStaffRider& StaffRider, UAnimSequence* StaffClip, float StaffRate)
{
    if (!StaffRider.HorseSk || !StaffClip) return;
    // The new gait starts at the same fraction of its cycle, so a change of gait does not restart the stride.
    StaffRider.HorseSk->PlayAnimation(StaffClip, true);
    StaffRider.HorseSk->SetPlayRate(StaffRate);
    StaffRider.HorseSk->SetPosition(FMath::Clamp(StaffRider.GaitPhase, 0.f, 0.999f) * StaffClip->GetPlayLength(), false);
    StaffRider.HorseClipNow = StaffClip;
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
    if (Pivot && OwnerHQ->QAPlaceholderMesh && OwnerHQ->QAPlaceholderMesh->IsVisible())
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
    StaffSpeed = FMath::Lerp(StaffSpeed, Speed, 1.f - FMath::Exp(-6.f * DeltaTime));
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
        UAnimSequence* StaffRiderClip = SeatClip.Get();
        float StaffRiderPose = 0.05f;
        if (R.HorseSk)
        {
            // Gait by speed with a little hysteresis, play rate by speed, the rider's clip on the horse's stride.
            int32 StaffNewGait = 0;
            for (int32 StaffThreshold = 0; StaffThreshold < 4; ++StaffThreshold)
                if (StaffSpeed >= StaffGaitThresholds[StaffThreshold] * (R.Gait > StaffThreshold ? 0.9f : 1.0f)) StaffNewGait = StaffThreshold + 1;
            R.Gait = StaffNewGait;
            UAnimSequence* StaffHorseClip = HorseGaitClips[R.Gait].Get();
            const float StaffRate = R.Gait == 0 ? 1.f : FMath::Clamp(StaffSpeed / StaffClipSpeedCmS[R.Gait], StaffRateMin[R.Gait], StaffRateMax[R.Gait]);
            if (R.HorseClipNow == StaffHorseClip)
            {
                R.GaitPhase = FMath::Clamp(R.HorseSk->GetPosition() / FMath::Max(0.1f, StaffHorseClip->GetPlayLength()), 0.f, 0.999f);
                R.HorseSk->SetPlayRate(StaffRate);
            }
            else
            {
                PlayStaffHorseClip(R, StaffHorseClip, StaffRate);
            }
            R.HorseSk->SetRelativeLocation(R.Offset + FVector(0.0f, 0.0f, HorseLift));
            R.HorseSk->SetRelativeRotation(FRotator(0.0f, HorseYaw, 0.0f));
            if (RiderGaitClips.IsValidIndex(R.Gait) && RiderGaitClips[R.Gait] && R.Gait > 0)
            {
                StaffRiderClip = RiderGaitClips[R.Gait].Get();
                StaffRiderPose = R.GaitPhase * StaffRiderClip->GetPlayLength();
            }
        }
        else if (R.Horse)
        {
            // The static fallback bobs.
            const float Wave = FMath::Sin((Pace + R.Phase) * 2.0f * PI);
            const float Rise = Walk * 5.0f * (0.5f + 0.5f * Wave);
            R.Horse->SetRelativeLocation(R.Offset + FVector(0.0f, 0.0f, HorseLift + Rise));
            R.Horse->SetRelativeRotation(FRotator(Walk * 1.5f * Wave, HorseYaw, 0.0f));
        }
        if (R.Rider)
        {
            // In the saddle: on the horse's saddle_root bone as drawn, else at the saddle's height.
            FVector StaffRiderRoot = R.Offset + FVector(-10.0f, 0.0f, SaddleHeightCm - StaffRiderSeatCm);
            if (R.HorseSk && bSaddleBone && R.HorseSk->WasRecentlyRendered(0.3f))
                StaffRiderRoot = Pivot->GetComponentTransform().InverseTransformPosition(R.HorseSk->GetSocketLocation(StaffSaddleBone)) -
                    FVector(0.f, 0.f, StaffRiderSeatCm - StaffSaddleLiftCm);
            else if (R.HorseSk)
                StaffRiderRoot = R.Offset + FVector(0.f, 0.f, HorseLift + SaddleHeightCm - StaffRiderSeatCm + StaffSaddleLiftCm);
            R.Rider->SetRelativeLocation(StaffRiderRoot);
            R.Rider->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
            if (StaffRiderClip && R.Rider->GetSingleNodeInstance() && R.Rider->GetSingleNodeInstance()->GetAnimationAsset() != StaffRiderClip)
            {
                R.Rider->PlayAnimation(StaffRiderClip, false);
                R.Rider->bPauseAnims = true;
            }
            if (StaffRiderClip) R.Rider->SetPosition(FMath::Min(StaffRiderPose, StaffRiderClip->GetPlayLength()), false);
            // The old soldier rider is drawn baked far off with the cavalry; the officer stays a skeletal figure (a staff is two or three).
            if (!bOfficerRider) UStrategyCrowdSubsystem::DrawAuxiliary(R.Rider, RiderModel, StaffRiderClip, StaffRiderPose);
        }
    }
}
