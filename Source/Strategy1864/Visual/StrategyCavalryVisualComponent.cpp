#include "StrategyCavalryVisualComponent.h"

#include "StrategyBattleBlast.h"
#include "../Units/CavalryUnit.h"
#include "../Combat/StrategyCavalryChargeComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "Animation/AnimSequence.h"
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
    LastLocation = OwnerCavalry->GetActorLocation();
    bReady = true;
}

void UStrategyCavalryVisualComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    for (FHorseman& H : Horsemen)
    {
        if (H.Horse) { H.Horse->DestroyComponent(); }
        if (H.Rider) { H.Rider->DestroyComponent(); }
        if (H.Sabre) { H.Sabre->DestroyComponent(); }
    }
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
    Fallen.Add(F);
}

void UStrategyCavalryVisualComponent::Layout()
{
    // Ranks of horsemen side by side, knee to knee in line, three abreast in column.
    const bool bColumn = OwnerCavalry->FormationComponent &&
        (OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::CavalryColumn ||
         OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn ||
         OwnerCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn);
    const int32 N = Horsemen.Num();
    const int32 Ranks = bColumn ? FMath::Max(1, FMath::DivideAndRoundUp(N, 3)) : (N > 12 ? 2 : 1);   // a column of three abreast
    const int32 Files = FMath::Max(1, FMath::DivideAndRoundUp(N, Ranks));
    for (int32 i = 0; i < N; ++i)
    {
        const int32 Rank = i / Files, File = i % Files;
        // The last rank may be short: its horsemen are centred on the column, not pushed to one side.
        const int32 InRank = Rank == Ranks - 1 ? FMath::Max(1, N - (Ranks - 1) * Files) : Files;
        const float Y = (File - (InRank - 1) * 0.5f) * FileSpacingCm;
        const float X = -(Rank - (Ranks - 1) * 0.5f) * RankSpacingCm;
        // A column of three abreast stays straight (a hair of looseness only); a line may be a little ragged.
        const float JitterX = bColumn ? 6.f : 25.f, JitterY = bColumn ? 3.f : 12.f;
        Horsemen[i].Slot = FVector(X + FMath::FRandRange(-JitterX, JitterX), Y + FMath::FRandRange(-JitterY, JitterY), 0.f);
        if (Horsemen[i].Shown.IsZero())
        {
            Horsemen[i].Shown = Horsemen[i].Slot;
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
    if (Strength != CachedStrength || Formation != CachedFormation)
    {
        EnsureCount(FMath::DivideAndRoundUp(Strength, FMath::Max(1, MenPerHorseman)));
        Layout();
        CachedStrength = Strength;
        CachedFormation = Formation;
    }
    UpdatePace(DeltaTime);

    // The fallen: the horse rolls onto its side, then lies still.
    for (FFallen& F : Fallen)
    {
        if (F.Age > 1.0f || !F.Horse)
        {
            continue;
        }
        F.Age += DeltaTime;
        const float T = FMath::Clamp(F.Age / 0.7f, 0.f, 1.f);
        const float Ease = T * T * (3.f - 2.f * T);
        F.Horse->SetWorldRotation(F.From + FRotator(0.f, 0.f, 82.f * F.Side * Ease));
        if (F.Sabre) { F.Sabre->SetVisibility(false); }
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
    SpeedCmS = FMath::FInterpTo(SpeedCmS, Moved, DeltaTime, 4.f);
    const bool bCharge = OwnerCavalry->ChargeComponent && OwnerCavalry->ChargeComponent->IsChargeActive();
    // The gaits: walk ~1.6 m/s, trot ~4, gallop ~7 and more. Strides per second and the rise of the body.
    const float Gait = FMath::Clamp(SpeedCmS / 700.f, 0.f, 1.3f);
    const float Strides = SpeedCmS < 20.f ? 0.f : FMath::Lerp(1.6f, 2.3f, FMath::Min(Gait, 1.f));
    const float Rise = SpeedCmS < 20.f ? 0.f : FMath::Lerp(3.f, 13.f, FMath::Min(Gait, 1.f));
    const float Pitch = SpeedCmS < 20.f ? 0.f : FMath::Lerp(1.f, 5.f, FMath::Min(Gait, 1.f));
    const float Now = GetWorld()->GetTimeSeconds();
    const FTransform Unit = OwnerCavalry->GetActorTransform();
    const float UnitZ = Here.Z;
    FigureLocalCentroid = FVector::ZeroVector;
    CentroidFigureCount = 0;
    for (FHorseman& H : Horsemen)
    {
        if (!H.Horse)
        {
            continue;
        }
        H.Phase = FMath::Fmod(H.Phase + Strides * DeltaTime, 1.f);
        H.Shown = FMath::VInterpTo(H.Shown, H.Slot, DeltaTime, 1.5f);
        const float Wave = FMath::Sin(H.Phase * 2.f * PI);
        // Each on its own ground: the field is not flat under a squadron.
        const FVector World = Unit.TransformPosition(H.Shown);
        const float Ground = UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(OwnerCavalry, World) - UnitZ;
        const FVector Local(H.Shown.X, H.Shown.Y, Ground + HorseLift + Rise * (0.5f + 0.5f * Wave));
        H.Horse->SetRelativeLocation(Local);
        H.Horse->SetRelativeRotation(FRotator(Pitch * Wave, HorseYaw, 0.f));
        if (H.Rider)
        {
            // In the saddle, rising a little behind the horse's motion; leaning forward at the gallop.
            const float RiderWave = FMath::Sin((H.Phase - 0.12f) * 2.f * PI);
            H.Rider->SetRelativeLocation(FVector(H.Shown.X - 10.f, H.Shown.Y, Ground + SaddleHeightCm - RiderSeatCm + Rise * (0.55f + 0.45f * RiderWave)));
            H.Rider->SetRelativeRotation(FRotator(-6.f * FMath::Min(Gait, 1.f), -90.f, 0.f));
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
    if (CentroidFigureCount > 0) { FigureLocalCentroid /= CentroidFigureCount; }
}
