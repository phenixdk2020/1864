#include "StrategyCavalryVisualComponent.h"

#include "StrategyBattleBlast.h"
#include "StrategyHorseAnimationStateComponent.h"
#include "StrategyHumanVisualTypes.h"
#include "../Units/CavalryUnit.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Combat/StrategyCavalryChargeComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Player/StrategyBattleQuality.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "Animation/AnimSequence.h"
#include "StrategyCrowdModel.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    const TCHAR* HorsePath = TEXT("/Game/Units/Items/SM_Horse_Static.SM_Horse_Static");
    const TCHAR* SabrePath = TEXT("/Game/Units/Items/SM_Saber.SM_Saber");
    const TCHAR* RiderPath = TEXT("/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864.SK_DK_Livgarden_1864");
    const TCHAR* SeatPath = TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Reload_sitting.A_Reload_sitting");
    const TCHAR* FallPath = TEXT("/Game/Units/Human/CombatAnimations/A_Combat_DeathBack.A_Combat_DeathBack");
    // The skeletal horse and its seven clips (Content/Python/import_horse_rigged.py) and the rider clips (Tools/Animation/build_cavalry_rider_set.py).
    const TCHAR* CavHorseSkPath = TEXT("/Game/Units/Horse/SK_Horse_1864.SK_Horse_1864");
    const TCHAR* CavHorseClipPaths[5] = {
        TEXT("/Game/Units/Horse/A_Horse_Stand.A_Horse_Stand"), TEXT("/Game/Units/Horse/A_Horse_Walk.A_Horse_Walk"),
        TEXT("/Game/Units/Horse/A_Horse_Trot.A_Horse_Trot"), TEXT("/Game/Units/Horse/A_Horse_Canter.A_Horse_Canter"),
        TEXT("/Game/Units/Horse/A_Horse_Gallop.A_Horse_Gallop")};
    const TCHAR* CavHorseRearPath = TEXT("/Game/Units/Horse/A_Horse_Rear.A_Horse_Rear");
    const TCHAR* CavHorseFallPath = TEXT("/Game/Units/Horse/A_Horse_Fall.A_Horse_Fall");
    const TCHAR* CavRiderClipPaths[5] = {   // index 0 is the held seat (SeatPath)
        TEXT(""), TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavWalk.A_Combat_CavWalk"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavTrotRise.A_Combat_CavTrotRise"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavCanter.A_Combat_CavCanter"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavGallop.A_Combat_CavGallop")};
    const TCHAR* CavRiderChargePath = TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavCharge.A_Combat_CavCharge");
    const TCHAR* CavRiderDrawPath = TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavSabreDraw.A_Combat_CavSabreDraw");
    const TCHAR* CavRiderSlashPath = TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavSabreSlash.A_Combat_CavSabreSlash");
    const TCHAR* CavRiderFallPath = TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavFall.A_Combat_CavFall");
    const FName CavSaddleBone(TEXT("saddle_root"));
    // Foot speed of each horse clip on the ground, measured on the clips (Tools/Animation/export_horse_mesh_b1.py): Stand, Walk, Trot, Canter, Gallop.
    const float CavClipSpeedCmS[5] = {0.f, 60.f, 179.f, 357.f, 757.f};
    const float CavRateMin[5] = {1.f, 0.9f, 0.9f, 0.85f, 0.9f};
    const float CavRateMax[5] = {1.f, 2.2f, 1.8f, 1.5f, 1.4f};
    const float CavRiderRootAboveGroundWhenFallenCm = 81.3f;   // the ground the CavFall clip was built for (saddle 118 cm * 1.1 - pelvis 48.7 cm)
    const float CavSlashSeconds = 1.4f;                         // the first swing of the combo clip

    float Flag(const TCHAR* Name, float Default)
    {
        float V = Default;
        FParse::Value(FCommandLine::Get(), Name, V);
        return V;
    }
}

UStrategyCavalryVisualComponent::UStrategyCavalryVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.f;
}

void UStrategyCavalryVisualComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCavalry = Cast<ACavalryUnit>(GetOwner());
    if (!OwnerCavalry || Flag(TEXT("Strategy1864Horsemen="), 1.0f) == 0.0f || !LoadAssets())
    {
        SetComponentTickEnabled(false);
        return;
    }
    // The single horse of the old placeholder goes; the squadron shows its horsemen instead.
    if (OwnerCavalry->HorseMesh) { OwnerCavalry->HorseMesh->SetVisibility(false, true); }
    if (OwnerCavalry->RiderMesh)
    {
        // Off the empty horse's socket (asked for every frame, and missing: the log fills with warnings).
        OwnerCavalry->RiderMesh->AttachToComponent(OwnerCavalry->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        OwnerCavalry->RiderMesh->SetVisibility(false, true);
    }
    if (OwnerCavalry->MovementExecutor) AddTickPrerequisiteComponent(OwnerCavalry->MovementExecutor);
    LastLocation = OwnerCavalry->GetActorLocation();
    MenPerHorseman = Strategy1864BattleQuality::GetFigureDivisor();
    bReady = true;
}

void UStrategyCavalryVisualComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    for (FHorseman& H : Horsemen)
    {
        if (H.Horse) { H.Horse->DestroyComponent(); }
        if (H.Rider) { UStrategyCrowdSubsystem::RemoveAuxiliary(H.Rider); H.Rider->DestroyComponent(); }
        if (H.Sabre) { H.Sabre->DestroyComponent(); }
    }
    for (FFallen& CrowdFallen : Fallen)
    {
        if (CrowdFallen.Rider) { UStrategyCrowdSubsystem::RemoveAuxiliary(CrowdFallen.Rider); CrowdFallen.Rider->DestroyComponent(); }
        if (CrowdFallen.Horse) CrowdFallen.Horse->DestroyComponent();
        if (CrowdFallen.Sabre) CrowdFallen.Sabre->DestroyComponent();
    }
    Fallen.Reset();
    Horsemen.Reset();
    CentroidFigureCount = 0;
    bReady = false;
    Super::EndPlay(Reason);
}

bool UStrategyCavalryVisualComponent::LoadAssets()
{
    HorseModel = LoadObject<UStaticMesh>(nullptr, HorsePath);
    RiderModel = LoadObject<USkeletalMesh>(nullptr, RiderPath);
    SabreModel = LoadObject<UStaticMesh>(nullptr, SabrePath);
    SeatClip = LoadObject<UAnimSequence>(nullptr, SeatPath);
    FallClip = LoadObject<UAnimSequence>(nullptr, FallPath);
    // Skeletal horse: all five gaits are required; rear, fall and the rider clips are optional (the old assets stand in).
    bSkeletalHorse = false;
    if (Flag(TEXT("Strategy1864StaticHorse="), 0.0f) == 0.0f)
    {
        HorseSkeletal = LoadObject<USkeletalMesh>(nullptr, CavHorseSkPath);
        HorseGaitClips.Reset();
        for (int32 CavClipIndex = 0; CavClipIndex < 5; ++CavClipIndex)
        {
            HorseGaitClips.Add(LoadObject<UAnimSequence>(nullptr, CavHorseClipPaths[CavClipIndex]));
        }
        bSkeletalHorse = HorseSkeletal && !HorseGaitClips.ContainsByPredicate([](const TObjectPtr<UAnimSequence>& CavGaitClip) { return !CavGaitClip; });
    }
    HorseRearClip = LoadObject<UAnimSequence>(nullptr, CavHorseRearPath);
    HorseFallClip = LoadObject<UAnimSequence>(nullptr, CavHorseFallPath);
    RiderGaitClips.Reset();
    RiderGaitClips.Add(SeatClip);
    for (int32 CavClipIndex = 1; CavClipIndex < 5; ++CavClipIndex)
    {
        RiderGaitClips.Add(LoadObject<UAnimSequence>(nullptr, CavRiderClipPaths[CavClipIndex]));
    }
    RiderChargeClip = LoadObject<UAnimSequence>(nullptr, CavRiderChargePath);
    RiderDrawClip = LoadObject<UAnimSequence>(nullptr, CavRiderDrawPath);
    RiderSlashClip = LoadObject<UAnimSequence>(nullptr, CavRiderSlashPath);
    RiderFallClip = LoadObject<UAnimSequence>(nullptr, CavRiderFallPath);
    RiderSeatCm = Flag(TEXT("Strategy1864Seat="), 48.0f);
    SaddleLiftCm = Flag(TEXT("Strategy1864SaddleLift="), 6.0f);
    if (bSkeletalHorse)
    {
        // Scaled to a horse's length (its long side is the forward axis), its feet on the ground. The forward sign comes
        // from the rest pose (pelvis to head), so no import axis is assumed.
        const FBoxSphereBounds CavBounds = HorseSkeletal->GetBounds();
        const FVector CavSize = CavBounds.BoxExtent * 2.0;
        HorseScale = SkeletalHorseLengthCm / FMath::Max(1.0f, float(FMath::Max(CavSize.X, CavSize.Y)));
        HorseLift = -(CavBounds.Origin.Z - CavBounds.BoxExtent.Z) * HorseScale;
        const FReferenceSkeleton& CavRef = HorseSkeletal->GetRefSkeleton();
        float CavAutoYaw = 0.0f;
        if (CavRef.FindBoneIndex(TEXT("head")) != INDEX_NONE && CavRef.FindBoneIndex(TEXT("pelvis")) != INDEX_NONE)
        {
            const FVector CavForward = HorseSkeletal->GetComposedRefPoseMatrix(FName(TEXT("head"))).GetOrigin() -
                HorseSkeletal->GetComposedRefPoseMatrix(FName(TEXT("pelvis"))).GetOrigin();
            CavAutoYaw = -FMath::RadiansToDegrees(FMath::Atan2(CavForward.Y, CavForward.X));
        }
        HorseYaw = Flag(TEXT("Strategy1864HorseYaw="), CavAutoYaw);
        bSaddleBone = CavRef.FindBoneIndex(CavSaddleBone) != INDEX_NONE;
        SaddleHeightCm = bSaddleBone ? HorseSkeletal->GetComposedRefPoseMatrix(CavSaddleBone).GetOrigin().Z * HorseScale : CavSize.Z * HorseScale * 0.72f;
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-CAV: skeletal horse %s scale %.3f yaw %.1f saddle %.1f cm (bone %d); rider clips walk %d trot %d canter %d gallop %d charge %d draw %d slash %d fall %d"),
            *HorseSkeletal->GetName(), HorseScale, HorseYaw, SaddleHeightCm, bSaddleBone ? 1 : 0, RiderGaitClips[1] != nullptr, RiderGaitClips[2] != nullptr,
            RiderGaitClips[3] != nullptr, RiderGaitClips[4] != nullptr, RiderChargeClip != nullptr, RiderDrawClip != nullptr, RiderSlashClip != nullptr, RiderFallClip != nullptr);
        return true;
    }
    if (!HorseModel)
    {
        return false;
    }
    // The model's size: scaled to a horse's length, its long side is its forward axis, its feet on the ground.
    const FBox B = HorseModel->GetBoundingBox();
    const FVector S = B.GetSize();
    const bool bAlongX = S.X >= S.Y;
    HorseScale = HorseLengthCm / FMath::Max(1.0f, bAlongX ? S.X : S.Y);
    HorseYaw = Flag(TEXT("Strategy1864HorseYaw="), bAlongX ? 0.0f : -90.0f);
    HorseLift = -B.Min.Z * HorseScale;
    SaddleHeightCm = Flag(TEXT("Strategy1864Saddle="), S.Z * HorseScale * 0.72f);
    return true;
}

UStrategyCavalryVisualComponent::FHorseman UStrategyCavalryVisualComponent::MakeHorseman()
{
    FHorseman H;
    USceneComponent* Root = OwnerCavalry->GetRootComponent();
    if (bSkeletalHorse)
    {
        H.HorseSk = NewObject<USkeletalMeshComponent>(OwnerCavalry);
        H.Horse = H.HorseSk;
        H.HorseSk->SetupAttachment(Root);
        H.HorseSk->SetSkeletalMeshAsset(HorseSkeletal);
        H.HorseSk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        H.HorseSk->bVisibleInRayTracing = false;
        H.HorseSk->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        H.HorseSk->bEnableUpdateRateOptimizations = true;
        H.HorseSk->SetRelativeScale3D(FVector(HorseScale));
        H.HorseSk->RegisterComponent();
        H.CavPhase = FMath::FRand();
        PlayHorseClip(H, HorseGaitClips[0], true, H.CavPhase, 1.f);
    }
    else
    {
        UStaticMeshComponent* CavStaticHorse = NewObject<UStaticMeshComponent>(OwnerCavalry);
        H.Horse = CavStaticHorse;
        CavStaticHorse->SetupAttachment(Root);
        CavStaticHorse->SetStaticMesh(HorseModel);
        CavStaticHorse->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CavStaticHorse->bVisibleInRayTracing = false;
        CavStaticHorse->SetRelativeScale3D(FVector(HorseScale));
        CavStaticHorse->RegisterComponent();
    }
    if (RiderModel)
    {
        H.Rider = NewObject<USkeletalMeshComponent>(OwnerCavalry);
        H.Rider->SetupAttachment(Root);
        H.Rider->SetSkeletalMesh(RiderModel);
        H.Rider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        H.Rider->bVisibleInRayTracing = false;
        H.Rider->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        H.Rider->RegisterComponent();
        if (SeatClip)
        {
            // The seated pose, held: a man in the saddle.
            H.Rider->PlayAnimation(SeatClip, false);
            H.Rider->SetPosition(0.05f, false);
            H.Rider->bPauseAnims = true;
        }
        if (SabreModel)
        {
            H.Sabre = NewObject<UStaticMeshComponent>(OwnerCavalry);
            H.Sabre->SetStaticMesh(SabreModel);
            H.Sabre->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            H.Sabre->SetupAttachment(H.Rider, TEXT("RightHand"));
            H.Sabre->RegisterComponent();
            H.Sabre->SetVisibility(false);
        }
    }
    H.Phase = FMath::FRand();
    return H;
}

void UStrategyCavalryVisualComponent::EnsureCount(int32 Count)
{
    while (Horsemen.Num() < Count)
    {
        Horsemen.Add(MakeHorseman());
    }
    while (Horsemen.Num() > Count)
    {
        Fall(FMath::RandRange(0, Horsemen.Num() - 1));
    }
}

void UStrategyCavalryVisualComponent::Fall(int32 Index)
{
    FHorseman H = Horsemen[Index];
    Horsemen.RemoveAt(Index);
    FFallen F;
    F.Horse = H.Horse;
    F.Rider = H.Rider;
    F.Sabre = H.Sabre;
    F.Side = FMath::RandBool() ? 1.0f : -1.0f;
    UAnimSequence* CavRiderFall = RiderFallClip ? RiderFallClip.Get() : FallClip.Get();
    // Off the moving unit: they stay where they fell.
    if (F.Horse) { F.Horse->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform); F.From = F.Horse->GetComponentRotation(); }
    if (H.HorseSk && HorseFallClip)
    {
        // A_Horse_Fall: the horse goes down on its left side and lies there (the clip is not looped).
        H.HorseSk->PlayAnimation(HorseFallClip, false);
        H.HorseSk->SetPlayRate(1.f);
        H.HorseSk->SetPosition(0.f, false);
    }
    if (F.Rider)
    {
        F.Rider->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        if (RiderFallClip && F.Horse)
        {
            // The rider's own fall clip was built for a rider root this high above the ground.
            FVector CavFallRoot = F.Rider->GetComponentLocation();
            CavFallRoot.Z = F.Horse->GetComponentLocation().Z - HorseLift + CavRiderRootAboveGroundWhenFallenCm;
            F.Rider->SetWorldLocation(CavFallRoot);
        }
        if (CavRiderFall)
        {
            F.Rider->bPauseAnims = false;
            F.Rider->PlayAnimation(CavRiderFall, false);
        }
    }
    if (UStrategyCrowdSubsystem::CountFallen(GetWorld()) >= 150)
    {
        if (F.Rider) { UStrategyCrowdSubsystem::RemoveAuxiliary(F.Rider); F.Rider->DestroyComponent(); }
        if (F.Horse) F.Horse->DestroyComponent();
        if (F.Sabre) F.Sabre->DestroyComponent();
        return;
    }
    if (F.Rider) UStrategyCrowdSubsystem::DrawAuxiliary(F.Rider, RiderModel, CavRiderFall ? CavRiderFall : SeatClip.Get(), 0.f, false);
    Fallen.Add(F);
}

void UStrategyCavalryVisualComponent::PlayHorseClip(FHorseman& H, UAnimSequence* Clip, bool bLoop, float NormalisedPhase, float Rate)
{
    if (!H.HorseSk || !Clip)
    {
        return;
    }
    // The new clip starts at the same fraction of its cycle, so a change of gait does not restart the stride.
    H.HorseSk->PlayAnimation(Clip, bLoop);
    H.HorseSk->SetPlayRate(Rate);
    H.HorseSk->SetPosition(FMath::Clamp(NormalisedPhase, 0.f, 1.f) * Clip->GetPlayLength(), false);
    H.HorseClipNow = Clip;
}

UAnimSequence* UStrategyCavalryVisualComponent::HorseClipForGait(int32 GaitIndex) const
{
    return HorseGaitClips.IsValidIndex(GaitIndex) ? HorseGaitClips[GaitIndex].Get() : nullptr;
}

UAnimSequence* UStrategyCavalryVisualComponent::RiderClipForGait(int32 GaitIndex) const
{
    return RiderGaitClips.IsValidIndex(GaitIndex) ? RiderGaitClips[GaitIndex].Get() : nullptr;
}

void UStrategyCavalryVisualComponent::Layout(bool bPreserveSlots)
{
    // Ranks of horsemen side by side, knee to knee in line, four abreast in column.
    const bool bColumn = OwnerCavalry->FormationComponent &&
        (OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::CavalryColumn ||
         OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn ||
         OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn);
    const int32 N = Horsemen.Num();
    const int32 VisualColumnWidth = OwnerCavalry->FormationComponent &&
        OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn ? 2 : 4;
    const int32 Ranks = bColumn ? FMath::Max(1, FMath::DivideAndRoundUp(N, VisualColumnWidth)) : (N > 12 ? 2 : 1);
    const int32 Files = bColumn ? VisualColumnWidth : FMath::Max(1, FMath::DivideAndRoundUp(N, Ranks));
    if (!bPreserveSlots) VisualPath.SlotBounds = FBox(ForceInit);
    for (int32 i = 0; i < N; ++i)
    {
        if (bPreserveSlots && Horsemen[i].bPlaced) continue;
        const int32 Rank = i / Files, File = i % Files;
        // A short column rank keeps the same files as its leaders.
        const int32 InRank = !bColumn && Rank == Ranks - 1 ? FMath::Max(1, N - (Ranks - 1) * Files) : Files;
        const float Y = (File - (InRank - 1) * 0.5f) * FileSpacingCm;
        const float X = -(Rank - (Ranks - 1) * 0.5f) * RankSpacingCm;
        // A column of four abreast stays straight (a hair of looseness only); a line may be a little ragged.
        const float JitterX = bColumn ? 0.f : 25.f, JitterY = bColumn ? 0.f : 12.f;
        Horsemen[i].Slot = FVector(X + FMath::FRandRange(-JitterX, JitterX), Y + FMath::FRandRange(-JitterY, JitterY), 0.f);
        if (bPreserveSlots)
        {
            // Count/quality changes cannot put a new horse on a survivor's place.
            bool bHorseSlotOccupied = true;
            while (bHorseSlotOccupied)
            {
                bHorseSlotOccupied = false;
                for (int32 ExistingHorseIndex = 0; ExistingHorseIndex < N; ++ExistingHorseIndex)
                {
                    if (ExistingHorseIndex != i && Horsemen[ExistingHorseIndex].bPlaced &&
                        FVector::Dist2D(Horsemen[ExistingHorseIndex].Slot, Horsemen[i].Slot) < FileSpacingCm * 0.5f)
                    {
                        Horsemen[i].Slot.X -= RankSpacingCm;
                        bHorseSlotOccupied = true;
                        break;
                    }
                }
            }
        }
        VisualPath.SlotBounds += Horsemen[i].Slot;
        if (!Horsemen[i].bPlaced)
        {
            float HorseInitialYaw = 0.f;
            Horsemen[i].Shown = VisualPath.bInitialized ? OwnerCavalry->GetActorTransform().InverseTransformPosition(
                VisualPath.Goal(Horsemen[i].Slot, bColumn, HorseInitialYaw)) : Horsemen[i].Slot;
            Horsemen[i].FacingYaw = OwnerCavalry->GetActorRotation().Yaw;
            Horsemen[i].bPlaced = true;
        }
    }
}

void UStrategyCavalryVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bReady || !OwnerCavalry)
    {
        return;
    }
    if (OwnerCavalry->HorseMesh && OwnerCavalry->HorseMesh->IsVisible()) { OwnerCavalry->HorseMesh->SetVisibility(false, true); }
    if (OwnerCavalry->QAPlaceholderMesh && OwnerCavalry->QAPlaceholderMesh->IsVisible()) { OwnerCavalry->QAPlaceholderMesh->SetVisibility(false); }
    const int32 Strength = FMath::Max(0, OwnerCavalry->CurrentStrength);
    const uint8 Formation = OwnerCavalry->FormationComponent ? uint8(OwnerCavalry->FormationComponent->CurrentFormation) : 255;
    if (Strength != CachedStrength || Formation != CachedFormation || MenPerHorseman != CachedMenPerHorseman)
    {
        EnsureCount(FMath::DivideAndRoundUp(Strength, FMath::Max(1, MenPerHorseman)));
        Layout(Formation == CachedFormation);
        CachedMenPerHorseman = MenPerHorseman;
        CachedStrength = Strength;
        CachedFormation = Formation;
    }
    if (DeltaTime <= 0.f) return;
    UpdatePace(DeltaTime);

    // The fallen: the horse rolls onto its side, then lies still.
    for (int32 CrowdFallenIndex = Fallen.Num() - 1; CrowdFallenIndex >= 0; --CrowdFallenIndex)
    {
        FFallen& F = Fallen[CrowdFallenIndex];
        F.Age += DeltaTime;
        if (F.Age >= 65.f)
        {
            if (F.Rider) { UStrategyCrowdSubsystem::RemoveAuxiliary(F.Rider); F.Rider->DestroyComponent(); }
            if (F.Horse) F.Horse->DestroyComponent();
            if (F.Sabre) F.Sabre->DestroyComponent();
            Fallen.RemoveAt(CrowdFallenIndex);
            continue;
        }
        if (F.Horse && !bSkeletalHorse)
        {
            const float T = FMath::Clamp(F.Age / 0.7f, 0.f, 1.f);
            const float Ease = T * T * (3.f - 2.f * T);
            F.Horse->SetWorldRotation(F.From + FRotator(0.f, 0.f, 82.f * F.Side * Ease));
        }
        if (F.Rider) UStrategyCrowdSubsystem::DrawAuxiliary(F.Rider, RiderModel, RiderFallClip ? RiderFallClip.Get() : (FallClip ? FallClip.Get() : SeatClip.Get()),
            F.Age, false, FMath::Clamp((65.f - F.Age) / 5.f, 0.f, 1.f));
        if (F.Sabre) F.Sabre->SetVisibility(false);
    }
}

bool UStrategyCavalryVisualComponent::GetFigureLocalCentroid(FVector& OutCentroid) const
{
    OutCentroid = FigureLocalCentroid;
    return bReady && CentroidFigureCount > 0;
}

void UStrategyCavalryVisualComponent::UpdatePace(float DeltaTime)
{
    const FVector Here = OwnerCavalry->GetActorLocation();
    const float Moved = FVector::Dist2D(Here, LastLocation) / FMath::Max(DeltaTime, 0.001f);
    LastLocation = Here;
    SpeedCmS = FMath::Lerp(SpeedCmS, Moved, 1.f - FMath::Exp(-4.f * DeltaTime));
    const bool bCharge = OwnerCavalry->ChargeComponent && OwnerCavalry->ChargeComponent->IsChargeActive();
    // The gait thresholds are those of the horse animation state component (80 / 350 / 650 / 900 cm/s by default).
    float CavThresholds[4] = {80.f, 350.f, 650.f, 900.f};
    if (const UStrategyHorseAnimationStateComponent* CavGaitState = OwnerCavalry->HorseAnimationStateComponent)
    {
        CavThresholds[0] = CavGaitState->WalkThresholdCmPerSecond;
        CavThresholds[1] = CavGaitState->TrotThresholdCmPerSecond;
        CavThresholds[2] = CavGaitState->CanterThresholdCmPerSecond;
        CavThresholds[3] = CavGaitState->GallopThresholdCmPerSecond;
    }
    if (bCharge != bChargePrev)
    {
        // The charge begins: every rider draws his sabre (a little apart). It ends: a square repelled it (the horses rear) or it hit (the riders slash).
        const bool bCavRepulsed = OwnerCavalry->ChargeComponent && OwnerCavalry->ChargeComponent->bLastChargeRepulsed;
        for (FHorseman& CavRider : Horsemen)
        {
            if (bCharge)
            {
                CavRider.CavDrawAge = -FMath::FRandRange(0.f, 0.35f);
                CavRider.bCavSlash = false;
            }
            else if (bCavRepulsed)
            {
                CavRider.bCavRear = HorseRearClip != nullptr;
                CavRider.CavRearAge = -FMath::FRandRange(0.f, 0.3f);
            }
            else if (SpeedCmS > 250.f)
            {
                CavRider.bCavSlash = RiderSlashClip != nullptr;
                CavRider.CavSlashAge = -FMath::FRandRange(0.f, 0.3f);
            }
        }
        bChargePrev = bCharge;
    }
    // The gaits: walk ~1.6 m/s, trot ~4, gallop ~7 and more. Strides per second and the rise of the body.
    const float Gait = FMath::Clamp(SpeedCmS / 700.f, 0.f, 1.3f);
    const float Pitch = SpeedCmS < 20.f ? 0.f : FMath::Lerp(1.f, 5.f, FMath::Min(Gait, 1.f));
    const float Now = GetWorld()->GetTimeSeconds();
    const FTransform Unit = OwnerCavalry->GetActorTransform();
    const bool bVisualColumn = OwnerCavalry->FormationComponent &&
        (OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
         OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::CavalryColumn ||
         OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn);
    if (!VisualPath.bInitialized) { VisualPath.Reset(Unit); }
    VisualPath.Advance(Unit, DeltaTime, bVisualColumn, 700.f);
    const float UnitZ = Here.Z;
    FigureLocalCentroid = FVector::ZeroVector;
    CentroidFigureCount = 0;
    for (FHorseman& H : Horsemen)
    {
        if (!H.Horse)
        {
            continue;
        }
        const FVector PreviousWorld = VisualPath.PreviousUnit.TransformPosition(H.Shown);
        float VisualGoalYaw = 0.f;
        const FVector VisualGoal = VisualPath.Goal(H.Slot, bVisualColumn, VisualGoalYaw);
        FVector VisualWorld = PreviousWorld;
        FStrategyVisualFormationPath::SmoothTravel(VisualWorld, H.ShownVelocity, VisualGoal, DeltaTime, FMath::Max(700.f, SpeedCmS));
        const FVector VisualStep = VisualWorld - PreviousWorld;
        const float VisualYaw = H.ShownVelocity.SizeSquared2D() > 1.f ? H.ShownVelocity.Rotation().Yaw : VisualGoalYaw;
        H.FacingYaw = FStrategyVisualFormationPath::SmoothFacing(H.FacingYaw, H.FacingVelocity, VisualYaw, DeltaTime, 120.f);
        const float RelativeFacing = H.FacingYaw - Unit.Rotator().Yaw;
        H.Shown = Unit.InverseTransformPosition(VisualWorld);
        const float FigureSpeed = VisualStep.Size2D() / FMath::Max(0.001f, DeltaTime);
        const float FigureGait = FMath::Clamp(FigureSpeed / 700.f, 0.f, 1.3f);
        const float FigureStrides = FMath::Clamp(FigureSpeed / 20.f, 0.f, 1.f) * FMath::Lerp(1.6f, 2.3f, FMath::Min(FigureGait, 1.f));
        const float FigureRise = FMath::Clamp(FigureSpeed / 20.f, 0.f, 1.f) * FMath::Lerp(3.f, 13.f, FMath::Min(FigureGait, 1.f));
        H.Phase = FMath::Fmod(H.Phase + FigureStrides * DeltaTime, 1.f);
        const float Wave = FMath::Sin(H.Phase * 2.f * PI);
        // Each on its own ground: the field is not flat under a squadron.
        const FVector World = Unit.TransformPosition(H.Shown);
        const float CavalryGroundTarget = UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(OwnerCavalry, World);
        H.GroundZ = H.bGroundPlaced ? FMath::Lerp(H.GroundZ, CavalryGroundTarget, 1.f - FMath::Exp(-24.f * DeltaTime)) : CavalryGroundTarget;
        H.bGroundPlaced = true;
        const float Ground = H.GroundZ - UnitZ;
        // The skeletal horse: gait by speed, play rate by speed, a lean into the turn, rear and fall by clip. The static fallback bobs.
        float CavRiderPose = 0.05f;                         // the rider's clip position (seconds)
        UAnimSequence* CavRiderClip = SeatClip.Get();
        FQuat CavLeanQuat = FQuat::Identity;
        if (H.HorseSk)
        {
            H.CavSpeed = FMath::Lerp(H.CavSpeed, FigureSpeed, 1.f - FMath::Exp(-6.f * DeltaTime));
            int32 CavNewGait = 0;
            for (int32 CavThresholdIndex = 0; CavThresholdIndex < 4; ++CavThresholdIndex)
            {
                // A little hysteresis: a gait is kept until the speed has fallen 10 % below the threshold that started it.
                if (H.CavSpeed >= CavThresholds[CavThresholdIndex] * (H.CavGait > CavThresholdIndex ? 0.9f : 1.0f)) CavNewGait = CavThresholdIndex + 1;
            }
            H.CavGait = CavNewGait;
            const float CavLeanTarget = FMath::Clamp(H.FacingVelocity * FMath::Clamp(H.CavSpeed / 700.f, 0.f, 1.f) * 0.12f, -14.f, 14.f);
            H.CavLean = FMath::FInterpTo(H.CavLean, CavLeanTarget, DeltaTime, 6.f);
            CavLeanQuat = FRotator(0.f, 0.f, H.CavLean).Quaternion();
            if (H.bCavRear)
            {
                H.CavRearAge += DeltaTime;
                if (H.CavRearAge >= HorseRearClip->GetPlayLength()) { H.bCavRear = false; H.HorseClipNow = nullptr; }
            }
            if (H.bCavRear && H.CavRearAge >= 0.f)
            {
                if (H.HorseClipNow != HorseRearClip) PlayHorseClip(H, HorseRearClip, false, 0.f, 1.f);
                CavRiderClip = SeatClip.Get();
            }
            else
            {
                UAnimSequence* CavHorseClip = HorseClipForGait(H.CavGait);
                const float CavClipLength = FMath::Max(0.1f, CavHorseClip->GetPlayLength());
                const float CavRate = H.CavGait == 0 ? 1.f : FMath::Clamp(H.CavSpeed / CavClipSpeedCmS[H.CavGait], CavRateMin[H.CavGait], CavRateMax[H.CavGait]);
                if (H.HorseClipNow == CavHorseClip)
                {
                    H.CavPhase = FMath::Clamp(H.HorseSk->GetPosition() / CavClipLength, 0.f, 0.999f);   // the horse's own clock is the master
                    H.HorseSk->SetPlayRate(CavRate);
                }
                else
                {
                    PlayHorseClip(H, CavHorseClip, true, H.CavPhase, CavRate);
                }
                // The rider's clip: the sabre drawn at the start of the charge, the slash at contact, the stirrups in the charge, else the gait's own.
                H.CavDrawAge = bCharge ? H.CavDrawAge + DeltaTime : -1.f;
                if (H.bCavSlash)
                {
                    H.CavSlashAge += DeltaTime;
                    if (H.CavSlashAge >= CavSlashSeconds) H.bCavSlash = false;
                }
                if (H.bCavSlash && H.CavSlashAge >= 0.f && RiderSlashClip)
                {
                    CavRiderClip = RiderSlashClip.Get();
                    CavRiderPose = FMath::Min(H.CavSlashAge, RiderSlashClip->GetPlayLength());
                }
                else if (bCharge && H.CavDrawAge >= 0.f && RiderDrawClip && H.CavDrawAge < RiderDrawClip->GetPlayLength())
                {
                    CavRiderClip = RiderDrawClip.Get();
                    CavRiderPose = H.CavDrawAge;
                }
                else
                {
                    UAnimSequence* CavGaitRider = bCharge && H.CavGait >= 3 && RiderChargeClip ? RiderChargeClip.Get() : RiderClipForGait(H.CavGait);
                    CavRiderClip = CavGaitRider ? CavGaitRider : SeatClip.Get();
                    CavRiderPose = CavRiderClip == SeatClip ? 0.05f : FMath::Min(H.CavPhase * CavRiderClip->GetPlayLength(), CavRiderClip->GetPlayLength());
                }
            }
        }
        const FQuat CavFacingQuat = FRotator(0.f, RelativeFacing, 0.f).Quaternion();
        const float CavRise = H.HorseSk ? 0.f : FigureRise * (0.5f + 0.5f * Wave);
        const FVector Local(H.Shown.X, H.Shown.Y, Ground + HorseLift + CavRise);
        H.Horse->SetRelativeLocation(Local);
        if (H.HorseSk)
        {
            H.Horse->SetRelativeRotation(CavFacingQuat * CavLeanQuat * FRotator(0.f, HorseYaw, 0.f).Quaternion());
        }
        else
        {
            H.Horse->SetRelativeRotation(FRotator(Pitch * Wave, HorseYaw + RelativeFacing, 0.f));
        }
        if (H.Rider)
        {
            // In the saddle: the rider sits on the horse's saddle_root bone (the bone is read as last drawn; it is not updated while the horse is
            // off screen, then the saddle height stands in). The static fallback horse rises with a wave instead.
            FVector CavRiderRoot;
            if (H.HorseSk && bSaddleBone && H.HorseSk->WasRecentlyRendered(0.3f))
            {
                CavRiderRoot = Unit.InverseTransformPosition(H.HorseSk->GetSocketTransform(CavSaddleBone).GetLocation()) - FVector(0.f, 0.f, RiderSeatCm - SaddleLiftCm);
            }
            else if (H.HorseSk)
            {
                CavRiderRoot = FVector(H.Shown.X, H.Shown.Y, Ground + HorseLift + SaddleHeightCm - RiderSeatCm + SaddleLiftCm);
            }
            else
            {
                const float RiderWave = FMath::Sin((H.Phase - 0.12f) * 2.f * PI);
                CavRiderRoot = FVector(H.Shown.X, H.Shown.Y, Ground + SaddleHeightCm - RiderSeatCm + FigureRise * (0.55f + 0.45f * RiderWave)) + FRotator(0.f, RelativeFacing, 0.f).RotateVector(FVector(-10.f, 0.f, 0.f));
            }
            H.Rider->SetRelativeLocation(CavRiderRoot);
            H.Rider->SetRelativeRotation(H.HorseSk ? CavFacingQuat * CavLeanQuat * FRotator(0.f, -90.f, 0.f).Quaternion()
                                                   : FRotator(-6.f * FMath::Min(Gait, 1.f), -90.f + RelativeFacing, 0.f).Quaternion());
            // Keep the sabre's held hand pose before releasing skeletal geometry. It remains a static prop.
            if (H.Sabre && H.Sabre->GetAttachSocketName() != NAME_None)
            {
                H.Rider->TickAnimation(0.f, false);
                H.Rider->RefreshBoneTransforms();
                H.Sabre->AttachToComponent(H.Rider, FAttachmentTransformRules::KeepWorldTransform);
            }
            UStrategyCrowdSubsystem::DrawAuxiliary(H.Rider, RiderModel, CavRiderClip, CavRiderPose);
        }
        FigureLocalCentroid += H.Rider ? H.Rider->GetRelativeLocation() : Local;
        ++CentroidFigureCount;
        if (H.Sabre)
        {
            H.Sabre->SetVisibility(bCharge || H.bCavSlash);
        }
        // Dust from the hooves at the trot and gallop (a few horsemen, now and then).
        if (Gait > 0.55f && Now >= H.NextDust)
        {
            H.NextDust = Now + FMath::FRandRange(0.5f, 1.4f) / Gait;
            if (FMath::FRand() < 0.35f)
            {
                AStrategyBattleBlast::Spawn(GetWorld(), EStrategyBlastKind::HoofDust, FVector(World.X, World.Y, UnitZ + Ground + 20.f), OwnerCavalry->GetActorForwardVector(), 1.f + Gait * 0.5f);
            }
        }
    }
    VisualPath.PreviousUnit = Unit;
    if (CentroidFigureCount > 0) { FigureLocalCentroid /= CentroidFigureCount; }
}
