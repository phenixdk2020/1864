#include "StrategyCourierRider.h"
#include "UObject/ConstructorHelpers.h"

#include "StrategyBattleBlast.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "../Units/StrategyUnit.h"
#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

namespace
{
    const TCHAR* CourierHorsePath = TEXT("/Game/Units/Items/SM_Horse_Static.SM_Horse_Static");
    const TCHAR* CourierRiderPath = TEXT("/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864.SK_DK_Livgarden_1864");
    const TCHAR* CourierSeatPath = TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Reload_sitting.A_Reload_sitting");
}

AStrategyCourierRider::AStrategyCourierRider()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BattleCourierHorse(CourierHorsePath);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> BattleCourierRider(CourierRiderPath);
    static ConstructorHelpers::FObjectFinder<UAnimSequence> BattleCourierSeat(CourierSeatPath);
    HorseModel = BattleCourierHorse.Object;
    RiderModel = BattleCourierRider.Object;
    SeatClip = BattleCourierSeat.Object;
    PrimaryActorTick.bCanEverTick = true;
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

bool AStrategyCourierRider::LoadAssets()
{
    if (!HorseModel)
    {
        return false;
    }
    // The same fitting as the squadron's horses: a horse's length, its long side forward, its feet on the ground.
    const FBox B = HorseModel->GetBoundingBox();
    const FVector S = B.GetSize();
    const bool bAlongX = S.X >= S.Y;
    HorseScale = 250.0f / FMath::Max(1.0f, bAlongX ? S.X : S.Y);
    HorseYaw = bAlongX ? 0.0f : -90.0f;
    HorseLift = -B.Min.Z * HorseScale;
    SaddleHeightCm = S.Z * HorseScale * 0.72f;
    Horse = NewObject<UStaticMeshComponent>(this);
    Horse->SetupAttachment(GetRootComponent());
    Horse->SetStaticMesh(HorseModel);
    Horse->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Horse->bVisibleInRayTracing = false;
    Horse->SetRelativeScale3D(FVector(HorseScale));
    Horse->RegisterComponent();
    if (RiderModel)
    {
        Rider = NewObject<USkeletalMeshComponent>(this);
        Rider->SetupAttachment(GetRootComponent());
        Rider->SetSkeletalMesh(RiderModel);
        Rider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rider->bVisibleInRayTracing = false;
        Rider->RegisterComponent();
        if (SeatClip)
        {
            Rider->PlayAnimation(SeatClip, false);
            Rider->SetPosition(0.05f, false);
            Rider->bPauseAnims = true;
        }
    }
    return true;
}

void AStrategyCourierRider::Send(const FVector& InHome, AStrategyUnit* To)
{
    Home = InHome;
    Target = To;
    bBack = false;
    bArrived = false;
    SetActorLocation(InHome);
    if (!bReady)
    {
        // Without the horse model the rider is invisible but still carries the order.
        LoadAssets();
        bReady = true;
    }
}

float AStrategyCourierRider::SecondsLeft() const
{
    const AStrategyUnit* Unit = Target.Get();
    if (bArrived || !IsValid(Unit))
    {
        return 0.0f;
    }
    return FVector::Dist2D(GetActorLocation(), Unit->GetActorLocation()) / GallopCmPerSecond;
}

void AStrategyCourierRider::Dismiss()
{
    bBack = true;
    bArrived = false;
}

void AStrategyCourierRider::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bReady)
    {
        return;
    }
    FVector Goal = bBack ? Home : FVector::ZeroVector;
    if (!bBack)
    {
        const AStrategyUnit* Unit = Target.Get();
        if (!IsValid(Unit))
        {
            Dismiss();
            Goal = Home;
        }
        else
        {
            Goal = Unit->GetActorLocation();
        }
    }
    FVector Here = GetActorLocation();
    FVector To = Goal - Here;
    To.Z = 0.0f;
    const float Distance = To.Size();
    const bool bThere = Distance < (bBack ? 400.0f : 600.0f);
    float Gallop = 1.0f;
    if (bThere)
    {
        if (bBack)
        {
            Destroy();
            return;
        }
        bArrived = true;
        Gallop = 0.0f;   // reined in at the unit, waiting for the order to be handed over
    }
    else
    {
        const FVector Dir = To / Distance;
        Here += Dir * FMath::Min(GallopCmPerSecond * DeltaSeconds, Distance);
        SetActorRotation(FRotator(0.0f, Dir.Rotation().Yaw, 0.0f));
    }
    Here.Z = UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, Here);
    SetActorLocation(Here);

    Phase = FMath::Fmod(Phase + Gallop * 2.3f * DeltaSeconds, 1.0f);
    const float Wave = FMath::Sin(Phase * 2.0f * PI);
    const float Rise = Gallop * 13.0f * (0.5f + 0.5f * Wave);
    if (Horse)
    {
        Horse->SetRelativeLocation(FVector(0.0f, 0.0f, HorseLift + Rise));
        Horse->SetRelativeRotation(FRotator(Gallop * 5.0f * Wave, HorseYaw, 0.0f));
    }
    if (Rider)
    {
        const float RiderWave = FMath::Sin((Phase - 0.12f) * 2.0f * PI);
        Rider->SetRelativeLocation(FVector(-10.0f, 0.0f, SaddleHeightCm - RiderSeatCm + Gallop * 13.0f * (0.55f + 0.45f * RiderWave)));
        Rider->SetRelativeRotation(FRotator(-6.0f * Gallop, -90.0f, 0.0f));
    }
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (Gallop > 0.0f && Now >= NextDust)
    {
        NextDust = Now + 0.22f;
        AStrategyBattleBlast::Spawn(GetWorld(), EStrategyBlastKind::HoofDust, FVector(Here.X, Here.Y, Here.Z + 20.0f), GetActorForwardVector(), 1.3f);
    }
}
