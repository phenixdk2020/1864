#include "StrategyArtilleryVisualComponent.h"

#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyMortarBatteryUnit.h"
#include "../Artillery/StrategyMortarDeploymentComponent.h"
#include "../Units/StrategyWagonTrainComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/World.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
    // The models as imported (Content/Python/import_units_1864.py): about 2 m long. The gun's muzzle points
    // along +Y, the mortar's along -X; a field gun with its trail is some 4.5 m.
    constexpr float CannonScale = 2.2f;
    constexpr float CannonYaw = -90.0f;
    constexpr float MortarScale = 1.1f;
    constexpr float MortarYaw = 180.0f;
}

UStrategyArtilleryVisualComponent::UStrategyArtilleryVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.f;
}

void UStrategyArtilleryVisualComponent::BeginPlay()
{
    Super::BeginPlay();
    AStrategyArtilleryBatteryUnit* Battery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
    bMortar = Cast<AStrategyMortarBatteryUnit>(Battery) != nullptr;
    if (Battery && Battery->MovementExecutor) AddTickPrerequisiteComponent(Battery->MovementExecutor);
    GunMesh = LoadObject<UStaticMesh>(nullptr, bMortar ? TEXT("/Game/Units/Items/SM_Mortar_1864.SM_Mortar_1864") : TEXT("/Game/Units/Items/SM_Cannon_1864.SM_Cannon_1864"));
    if (!Battery || !GunMesh)
    {
        SetComponentTickEnabled(false);
        return;
    }
    Guns = NewObject<UInstancedStaticMeshComponent>(Battery, TEXT("BatteryGuns"));
    Guns->SetStaticMesh(GunMesh);
    Guns->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Guns->SetCastShadow(true);
    Guns->bVisibleInRayTracing = false;
    Guns->SetupAttachment(Battery->GetRootComponent());
    Guns->RegisterComponent();
    if (Battery->QAPlaceholderMesh)
    {
        Battery->QAPlaceholderMesh->SetVisibility(false, true);
    }
    Rebuild();
    if (Battery) ArtilleryVisualPath.Reset(Battery->GetActorTransform());
}

void UStrategyArtilleryVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    Rebuild();
    if (!Guns || !GetOwner() || DeltaTime <= 0.f) return;
    ArtilleryVisualPath.Advance(GetOwner()->GetActorTransform(), DeltaTime, false, 160.f);
    Guns->SetWorldLocation(ArtilleryVisualPath.Center);
    Guns->SetWorldRotation(FRotator(0.f, ArtilleryVisualPath.Facing, 0.f));
    for (int32 GunIndex = 0; GunIndex < GunGoals.Num(); ++GunIndex)
    {
        FTransform GunShown;
        if (!Guns->GetInstanceTransform(GunIndex, GunShown, false)) continue;
        FVector GunPosition = GunShown.GetLocation();
        FStrategyVisualFormationPath::SmoothTravel(GunPosition, GunVelocities[GunIndex], GunGoals[GunIndex].GetLocation(), DeltaTime, 420.f);
        FRotator GunRotation = GunShown.Rotator();
        GunRotation.Yaw = FStrategyVisualFormationPath::SmoothFacing(GunRotation.Yaw, GunTurnVelocities[GunIndex], GunGoals[GunIndex].Rotator().Yaw, DeltaTime, 120.f);
        const float GunAlpha = 1.f - FMath::Exp(-12.f * DeltaTime);
        GunRotation.Roll = FMath::Lerp(GunRotation.Roll, GunGoals[GunIndex].Rotator().Roll, GunAlpha);
        GunShown.SetLocation(GunPosition);
        GunShown.SetRotation(GunRotation.Quaternion());
        GunShown.SetScale3D(FMath::Lerp(GunShown.GetScale3D(), GunGoals[GunIndex].GetScale3D(), GunAlpha));
        Guns->UpdateInstanceTransform(GunIndex, GunShown, false, false, false);
    }
    if (const AStrategyMortarBatteryUnit* TransportMortar = Cast<AStrategyMortarBatteryUnit>(GetOwner()))
    {
        if (TransportMortar->WagonTrainComponent)
        {
            const int32 TransportCount = TransportMortar->WagonTrainComponent->GetTransportedMortarCount();
            for (int32 TransportIndex = 0; TransportIndex < FMath::Min(TransportCount, Guns->GetInstanceCount()); ++TransportIndex)
            {
                FTransform TransportShown;
                if (Guns->GetInstanceTransform(TransportIndex, TransportShown))
                {
                    TransportShown.SetScale3D(FVector::ZeroVector);
                    Guns->UpdateInstanceTransform(TransportIndex, TransportShown, false, false, false);
                }
            }
        }
    }
    UpdateTransport();
    Guns->MarkRenderStateDirty();
    ArtilleryVisualPath.PreviousUnit = GetOwner()->GetActorTransform();
}

bool UStrategyArtilleryVisualComponent::GetDrawnCenter(FVector& OutTrainCenter) const
{
    if (!Guns || Guns->GetInstanceCount() == 0) return false;
    OutTrainCenter = ArtilleryVisualPath.Center;
    return true;
}

void UStrategyArtilleryVisualComponent::Rebuild()
{
    const AStrategyArtilleryBatteryUnit* Battery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
    if (!Battery || !Guns)
    {
        return;
    }
    // The QA box comes back when the unit refreshes its label: keep it hidden while the guns stand there.
    if (Battery->QAPlaceholderMesh && Battery->QAPlaceholderMesh->IsVisible())
    {
        Battery->QAPlaceholderMesh->SetVisibility(false, true);
    }
    const AStrategyMortarBatteryUnit* Mortars = Cast<AStrategyMortarBatteryUnit>(Battery);
    const int32 Total = Mortars ? FMath::Max(1, Mortars->MortarPieceCount) : FMath::Max(1, Battery->GunCount);
    const int32 Destroyed = FMath::Clamp(Battery->DestroyedGunCount, 0, Total);
    const int32 Working = Mortars ? FMath::Clamp(Mortars->GetOperationalMortarCount(), 0, Total) : FMath::Clamp(Battery->GetOperationalGunCount(), 0, Total);
    const bool bLimbered = Mortars && Mortars->MortarDeploymentComponent ? Mortars->MortarDeploymentComponent->CanMoveNormally() : Battery->CanNormalMove();
    if (Total == BuiltTotal && Working == BuiltWorking && Destroyed == BuiltDestroyed && bLimbered == bBuiltLimbered)
    {
        return;
    }
    BuiltTotal = Total;
    BuiltWorking = Working;
    BuiltDestroyed = Destroyed;
    bBuiltLimbered = bLimbered;
    // The QA box is the unit's own scale; the guns stand at world size.
    const FVector OwnerScale = Battery->GetActorScale3D();
    const FVector Unscale(1.0f / FMath::Max(0.01f, OwnerScale.X), 1.0f / FMath::Max(0.01f, OwnerScale.Y), 1.0f / FMath::Max(0.01f, OwnerScale.Z));
    const float Scale = bMortar ? MortarScale : CannonScale;
    const float BaseYaw = bMortar ? MortarYaw : CannonYaw;
    TArray<FTransform> Instances;
    for (int32 g = 0; g < Total; ++g)
    {
        const bool bDestroyed = g >= Total - Destroyed;
        const bool bDisabled = !bDestroyed && g >= Working;
        FVector Place;
        float Yaw = BaseYaw;
        if (bLimbered && !bDestroyed)
        {
            // On the march: one behind the other, the muzzle to the rear (the limber pulls by the trail).
            Place = FVector(-g * ColumnIntervalCm, 0.0f, 0.0f);
            Yaw += 180.0f;
        }
        else
        {
            Place = FVector(0.0f, (g - (Total - 1) * 0.5f) * LineIntervalCm, 0.0f);
        }
        FRotator Rotation(0.0f, Yaw + (bDisabled ? 14.0f : 0.0f), bDestroyed ? 38.0f : 0.0f);
        Instances.Emplace(Rotation, Place * Unscale, Unscale * Scale);
    }
    GunGoals = Instances;
    GunVelocities.SetNumZeroed(Instances.Num());
    GunTurnVelocities.SetNumZeroed(Instances.Num());
    if (Guns->GetInstanceCount() != Instances.Num())
    {
        Guns->ClearInstances();
        Guns->AddInstances(Instances, false, false);
    }
}

void UStrategyArtilleryVisualComponent::UpdateTransport()
{
    AStrategyArtilleryBatteryUnit* TransportBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
    if (!TransportBattery || bMortar || !Guns) return;
    if (TransportBattery->bOutOfPlay || TransportBattery->IsHidden())
    {
        if (BatteryLimbers) BatteryLimbers->SetVisibility(false);
        if (BatteryTeamHorses) BatteryTeamHorses->SetVisibility(false);
        if (BatteryHarness) BatteryHarness->SetVisibility(false);
        return;
    }
    const bool bTransportEnabled = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864Train")) ||
        (TransportBattery->WagonTrainComponent && TransportBattery->WagonTrainComponent->WagonCount >= 0);
    if (!bTransportEnabled) return;
    if (!bTransportAssetsAttempted)
    {
        bTransportAssetsAttempted = true;
        auto TransportMakeISM = [&](const TCHAR* TransportAsset, const TCHAR* TransportName)
        {
            UStaticMesh* TransportMesh = LoadObject<UStaticMesh>(nullptr, TransportAsset);
            if (!TransportMesh) return static_cast<UInstancedStaticMeshComponent*>(nullptr);
            UInstancedStaticMeshComponent* TransportISM = NewObject<UInstancedStaticMeshComponent>(TransportBattery, FName(TransportName));
            TransportISM->SetStaticMesh(TransportMesh);
            TransportISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            TransportISM->SetCastShadow(false);
            TransportISM->bVisibleInRayTracing = false;
            TransportISM->SetupAttachment(TransportBattery->GetRootComponent());
            TransportISM->RegisterComponent();
            TransportISM->SetAbsolute(true, true, true);
            TransportISM->SetWorldTransform(FTransform::Identity);
            return TransportISM;
        };
        BatteryLimbers = TransportMakeISM(TEXT("/Game/Units/Items/SM_Wagon_Empty.SM_Wagon_Empty"), TEXT("BatteryLimbers"));
        BatteryTeamHorses = TransportMakeISM(TEXT("/Game/Units/Items/SM_Horse_Static.SM_Horse_Static"), TEXT("BatteryTeamHorses"));
        BatteryHarness = TransportMakeISM(TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("BatteryHarness"));
    }
    if (!BatteryLimbers || !BatteryTeamHorses || !BatteryHarness) return;
    const bool bTransportMoving = TransportBattery->CanNormalMove();
    BatteryLimbers->SetVisibility(bTransportMoving);
    BatteryTeamHorses->SetVisibility(bTransportMoving);
    BatteryHarness->SetVisibility(bTransportMoving);
    if (!bTransportMoving) return;
    const int32 TransportPieces = Guns->GetInstanceCount();
    if (BatteryLimbers->GetInstanceCount() != TransportPieces)
    {
        BatteryLimbers->ClearInstances(); BatteryTeamHorses->ClearInstances(); BatteryHarness->ClearInstances();
        for (int32 TransportIndex = 0; TransportIndex < TransportPieces; ++TransportIndex)
        {
            BatteryLimbers->AddInstance(FTransform::Identity);
            for (int32 TransportHorse = 0; TransportHorse < 2; ++TransportHorse)
            {
                BatteryTeamHorses->AddInstance(FTransform::Identity);
                BatteryHarness->AddInstance(FTransform::Identity);
            }
        }
    }
    auto TransportTransform = [](UStaticMesh* TransportMesh, const FVector& TransportGround, const FRotator& TransportFacing, float TransportLength)
    {
        const FBox TransportBounds = TransportMesh->GetBoundingBox();
        const FVector TransportSize = TransportBounds.GetSize();
        const bool bTransportAlongX = TransportSize.X >= TransportSize.Y;
        const float TransportScale = TransportLength / FMath::Max(1.f, float(bTransportAlongX ? TransportSize.X : TransportSize.Y));
        const FRotator TransportRotation = TransportFacing + FRotator(0.f, bTransportAlongX ? 0.f : -90.f, 0.f);
        const FVector TransportPivot(TransportBounds.GetCenter().X, TransportBounds.GetCenter().Y, TransportBounds.Min.Z);
        return FTransform(TransportRotation, TransportGround - TransportRotation.RotateVector(TransportPivot * TransportScale), FVector(TransportScale));
    };
    for (int32 TransportIndex = 0; TransportIndex < TransportPieces; ++TransportIndex)
    {
        FTransform TransportGun;
        if (!Guns->GetInstanceTransform(TransportIndex, TransportGun, true)) continue;
        const FRotator TransportFacing(0.f, TransportGun.Rotator().Yaw - CannonYaw - 180.f, 0.f);
        const bool bTransportWorking = TransportIndex < TransportBattery->GetOperationalGunCount();
        const FVector TransportGround = TransportGun.GetLocation();
        FTransform TransportLimber = TransportTransform(BatteryLimbers->GetStaticMesh(), TransportGround + TransportFacing.RotateVector(FVector(350.f, 0.f, 0.f)), TransportFacing, 210.f);
        if (!bTransportWorking) TransportLimber.SetScale3D(FVector::ZeroVector);
        BatteryLimbers->UpdateInstanceTransform(TransportIndex, TransportLimber, true, false, true);
        for (int32 TransportHorse = 0; TransportHorse < 2; ++TransportHorse)
        {
            const int32 TransportHorseIndex = TransportIndex * 2 + TransportHorse;
            const bool bTransportHasHorse = bTransportWorking && TransportHorseIndex < TransportBattery->HorseStrength;
            const float TransportWave = FMath::Sin(GetWorld()->GetTimeSeconds() * (ArtilleryVisualPath.bMovingVisuals ? 12.f : 2.f) + TransportHorseIndex);
            const FVector TransportPlace = TransportGround + TransportFacing.RotateVector(FVector(650.f, TransportHorse ? 65.f : -65.f, TransportWave * (ArtilleryVisualPath.bMovingVisuals ? 5.f : 1.f)));
            FTransform TransportHorseShown = TransportTransform(BatteryTeamHorses->GetStaticMesh(), TransportPlace,
                TransportFacing + FRotator(TransportWave * 2.f, 0.f, TransportWave * 0.7f), 240.f);
            if (!bTransportHasHorse) TransportHorseShown.SetScale3D(FVector::ZeroVector);
            BatteryTeamHorses->UpdateInstanceTransform(TransportHorseIndex, TransportHorseShown, true, false, true);
            BatteryHarness->UpdateInstanceTransform(TransportHorseIndex,
                FTransform(TransportFacing, TransportGround + TransportFacing.RotateVector(FVector(500.f, TransportHorse ? 65.f : -65.f, 95.f)),
                    bTransportHasHorse ? FVector(2.8f, 0.025f, 0.025f) : FVector::ZeroVector), true, false, true);
        }
    }
    BatteryLimbers->MarkRenderStateDirty(); BatteryTeamHorses->MarkRenderStateDirty(); BatteryHarness->MarkRenderStateDirty();
}
