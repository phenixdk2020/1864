#include "StrategyCavalryVisualComponent.h"

#include "StrategyBattleBlast.h"
#include "../Units/CavalryUnit.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Combat/StrategyCavalryChargeComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Player/StrategyBattleQuality.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "Animation/AnimSequence.h"
#include "StrategyCrowdModel.h"
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
    RiderSeatCm = Flag(TEXT("Strategy1864Seat="), 48.0f);
    return true;
}

UStrategyCavalryVisualComponent::FHorseman UStrategyCavalryVisualComponent::MakeHorseman()
{
    FHorseman H;
    USceneComponent* Root = OwnerCavalry->GetRootComponent();
    H.Horse = NewObject<UStaticMeshComponent>(OwnerCavalry);
    H.Horse->SetupAttachment(Root);
    H.Horse->SetStaticMesh(HorseModel);
    H.Horse->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    H.Horse->bVisibleInRayTracing = false;
    H.Horse->SetRelativeScale3D(FVector(HorseScale));
    H.Horse->RegisterComponent();
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
    // Off the moving unit: they stay where they fell.
    if (F.Horse) { F.Horse->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform); F.From = F.Horse->GetComponentRotation(); }
    if (F.Rider)
    {
        F.Rider->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        if (FallClip)
        {
            F.Rider->bPauseAnims = false;
            F.Rider->PlayAnimation(FallClip, false);
        }
    }
    if (UStrategyCrowdSubsystem::CountFallen(GetWorld()) >= 150)
    {
        if (F.Rider) { UStrategyCrowdSubsystem::RemoveAuxiliary(F.Rider); F.Rider->DestroyComponent(); }
        if (F.Horse) F.Horse->DestroyComponent();
        if (F.Sabre) F.Sabre->DestroyComponent();
        return;
    }
    if (F.Rider) UStrategyCrowdSubsystem::DrawAuxiliary(F.Rider, RiderModel, FallClip ? FallClip.Get() : SeatClip.Get(), 0.f, false);
    Fallen.Add(F);
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
        if (F.Horse)
        {
            const float T = FMath::Clamp(F.Age / 0.7f, 0.f, 1.f);
            const float Ease = T * T * (3.f - 2.f * T);
            F.Horse->SetWorldRotation(F.From + FRotator(0.f, 0.f, 82.f * F.Side * Ease));
        }
        if (F.Rider) UStrategyCrowdSubsystem::DrawAuxiliary(F.Rider, RiderModel, FallClip ? FallClip.Get() : SeatClip.Get(),
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
        const FVector Local(H.Shown.X, H.Shown.Y, Ground + HorseLift + FigureRise * (0.5f + 0.5f * Wave));
        H.Horse->SetRelativeLocation(Local);
        H.Horse->SetRelativeRotation(FRotator(Pitch * Wave, HorseYaw + RelativeFacing, 0.f));
        if (H.Rider)
        {
            // In the saddle, rising a little behind the horse's motion; leaning forward at the gallop.
            const float RiderWave = FMath::Sin((H.Phase - 0.12f) * 2.f * PI);
            H.Rider->SetRelativeLocation(FVector(H.Shown.X, H.Shown.Y, Ground + SaddleHeightCm - RiderSeatCm + FigureRise * (0.55f + 0.45f * RiderWave)) + FRotator(0.f, RelativeFacing, 0.f).RotateVector(FVector(-10.f, 0.f, 0.f)));
            H.Rider->SetRelativeRotation(FRotator(-6.f * FMath::Min(Gait, 1.f), -90.f + RelativeFacing, 0.f));
            // Keep the sabre's held hand pose before releasing skeletal geometry. It remains a static prop.
            if (H.Sabre && H.Sabre->GetAttachSocketName() != NAME_None)
            {
                H.Rider->TickAnimation(0.f, false);
                H.Rider->RefreshBoneTransforms();
                H.Sabre->AttachToComponent(H.Rider, FAttachmentTransformRules::KeepWorldTransform);
            }
            UStrategyCrowdSubsystem::DrawAuxiliary(H.Rider, RiderModel, SeatClip, 0.05f);
        }
        FigureLocalCentroid += H.Rider ? H.Rider->GetRelativeLocation() : Local;
        ++CentroidFigureCount;
        if (H.Sabre)
        {
            H.Sabre->SetVisibility(bCharge);
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
