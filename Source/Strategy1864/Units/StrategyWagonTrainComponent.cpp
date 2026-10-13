#include "StrategyWagonTrainComponent.h"

#include "StrategyHQUnit.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyMortarBatteryUnit.h"
#include "../Artillery/StrategyMortarDeploymentComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyVisibilityComponent.h"
#include "../Visual/StrategyArtilleryVisualComponent.h"
#include "../Visual/StrategyHQVisualComponent.h"
#include "../Visual/StrategyInfantryVisualComponent.h"
#include "../Visual/StrategyCavalryVisualComponent.h"
#include "GameFramework/PlayerController.h"
#include "../Logistics/StrategySupplyCargoComponent.h"
#include "../Logistics/StrategySupplyComponent.h"
#include "../Logistics/StrategySupplyCaptureComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AStrategyWagonTrain::AStrategyWagonTrain()
{
    WagonCount = 1;
    DriverStrength = 2;
    HorseStrength = HorsesRequiredForFullMobility = 2;
    bOfficerAIEnabled = false;
}

void AStrategyWagonTrain::RecordMortarLoss(bool bDestroyed)
{
    if (!bMortarLoad || bMortarLossRecorded || BattleLedger.bFrozen) return;
    bMortarLossRecorded = true;
    if (AStrategyMortarBatteryUnit* TrainMortar = Cast<AStrategyMortarBatteryUnit>(GetOwner()))
        ++TrainMortar->DestroyedGunCount; // Removed from the original battery, including captured pieces.
    if (bDestroyed)
    {
        if (BattleLedger.bEquipmentAbandoned) BattleLedger.Abandoned.Mortars = 0;
        else ++BattleLedger.Lost.Mortars;
    }
}

void AStrategyWagonTrain::BeginPlay()
{
    Super::BeginPlay();
    // The parent train updates all wagons together. Retain existing capture and visibility logic.
    for (UActorComponent* TrainComponent : GetComponents())
    {
        if (TrainComponent != SupplyCaptureComponent && TrainComponent != VisibilityComponent)
            TrainComponent->SetComponentTickEnabled(false);
    }
    SupplyCaptureComponent->SetComponentTickInterval(1.f);
    SupplyComponent->bActsAsSupplySource = false; // Batched, finite-cargo transfer below.
    QAPlaceholderMesh->SetVisibility(false);
    DebugLabel->SetVisibility(false);
    EnsureBattleLedger();
}

UStrategyWagonTrainComponent::UStrategyWagonTrainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.05f;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UStrategyWagonTrainComponent::Configure(int32 InWagons, int32 InHorses)
{
    if (bBuilt) return; // Scenario calls this before the first tick.
    WagonCount = FMath::Clamp(InWagons, 0, 128);
    HorseCount = FMath::Clamp(InHorses, 0, 1000000); // Reserve riding mounts before limiting the drawn teams.
}

UInstancedStaticMeshComponent* UStrategyWagonTrainComponent::MakeInstances(const TCHAR* Asset, const TCHAR* Name, bool bShadow)
{
    UStaticMesh* TrainMesh = LoadObject<UStaticMesh>(nullptr, Asset);
    if (!TrainMesh) { UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-TRAIN mangler %s"), Asset); return nullptr; }
    UInstancedStaticMeshComponent* TrainISM = NewObject<UInstancedStaticMeshComponent>(GetOwner(), FName(Name));
    TrainISM->SetStaticMesh(TrainMesh);
    TrainISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TrainISM->SetCastShadow(bShadow);
    TrainISM->bVisibleInRayTracing = false;
    TrainISM->SetupAttachment(GetOwner()->GetRootComponent());
    TrainISM->RegisterComponent();
    // World instances must not inherit the owner's QA scale or movement.
    TrainISM->SetAbsolute(true, true, true);
    TrainISM->SetWorldTransform(FTransform::Identity);
    return TrainISM;
}

FTransform UStrategyWagonTrainComponent::MeshTransform(UStaticMesh* Mesh, const FVector& Ground,
    const FRotator& Facing, float LengthCm, float WidthCm) const
{
    const FBox TrainBounds = Mesh->GetBoundingBox();
    const FVector TrainSize = TrainBounds.GetSize();
    const bool bTrainAlongX = TrainSize.X >= TrainSize.Y;
    const float TrainScale = LengthCm / FMath::Max(1.f, float(bTrainAlongX ? TrainSize.X : TrainSize.Y));
    FVector TrainScale3D(TrainScale);
    // Wagons: length and track width are calibrated independently; preserve vertical wheel proportions.
    if (WidthCm > 0.f)
    {
        if (bTrainAlongX) TrainScale3D.Y = WidthCm / FMath::Max(1.f, float(TrainSize.Y));
        else TrainScale3D.X = WidthCm / FMath::Max(1.f, float(TrainSize.X));
    }
    const FRotator TrainRotation = Facing + FRotator(0.f, bTrainAlongX ? 0.f : -90.f, 0.f);
    const FVector TrainPivot(TrainBounds.GetCenter().X, TrainBounds.GetCenter().Y, TrainBounds.Min.Z);
    return FTransform(TrainRotation, Ground - TrainRotation.RotateVector(TrainPivot * TrainScale3D), TrainScale3D);
}

void UStrategyWagonTrainComponent::BuildTrain()
{
    bBuilt = true;
    TrainUnit = Cast<AStrategyUnit>(GetOwner());
    AStrategyUnit* TrainOwner = TrainUnit.Get();
    if (!TrainOwner || Cast<AStrategySupplyWagonUnit>(TrainOwner)) { SetComponentTickEnabled(false); return; }
    AStrategyHQUnit* TrainHQ = Cast<AStrategyHQUnit>(TrainOwner);
    AStrategyArtilleryBatteryUnit* TrainBattery = Cast<AStrategyArtilleryBatteryUnit>(TrainOwner);
    AStrategyMortarBatteryUnit* TrainMortar = Cast<AStrategyMortarBatteryUnit>(TrainOwner);
    if (WagonCount < 0)
    {
        if (!FParse::Param(FCommandLine::Get(), TEXT("Strategy1864Train"))) { SetComponentTickEnabled(false); return; }
        WagonCount = TrainMortar ? TrainMortar->MortarPieceCount : TrainBattery ? (TrainBattery->GunCount + 1) / 2
            : TrainHQ ? (TrainHQ->HQLevel >= EStrategyHQLevel::Brigade ? 4 : 2)
            : TrainOwner->Echelon == EStrategyEchelon::Battalion ? 2 : 0;
    }
    WagonCount = FMath::Clamp(WagonCount, 0, 128);
    if (WagonCount == 0) { SetComponentTickEnabled(false); return; }
    if (HorseCount < 0) HorseCount = WagonCount * 2;
    else if (TrainOwner->Echelon == EStrategyEchelon::Cavalry)
        HorseCount = FMath::Max(0, HorseCount - TrainOwner->CurrentStrength); // Riding mounts are not draught horses.
    HorseCount = FMath::Min(HorseCount, WagonCount * 2);
    TrailingCm = TrainBattery ? 2500.f : TrainHQ && TrainHQ->HQLevel >= EStrategyHQLevel::Brigade ? 25000.f : 6000.f;
    const TCHAR* TrainBodyAsset = TrainMortar ? TEXT("/Game/Units/Items/SM_Wagon_Mortar.SM_Wagon_Mortar")
        : TEXT("/Game/Units/Items/SM_Wagon_Supply.SM_Wagon_Supply");
    TrainMeshes.Add(MakeInstances(TrainBodyAsset, TEXT("TrainNearWagons"), true));
    TrainMeshes.Add(MakeInstances(TrainBodyAsset, TEXT("TrainFarWagons"), false));
    TrainMeshes.Add(MakeInstances(TEXT("/Game/Units/Items/SM_Horse_Static.SM_Horse_Static"), TEXT("TrainHorses"), false));
    TrainMeshes.Add(MakeInstances(TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("TrainHarness"), false));
    TrainMeshes.Add(MakeInstances(TEXT("/Game/Units/Items/SM_Wagon_Empty.SM_Wagon_Empty"), TEXT("TrainAmmoWagons"), false));
    TrainMeshes.Add(MakeInstances(TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("TrainAmmoCrates"), false));
    TrainMeshes.Add(MakeInstances(TEXT("/Game/Units/Items/SM_Wagon_Empty.SM_Wagon_Empty"), TEXT("TrainNearAmmoWagons"), true));
    if (!TrainMeshes[0] || !TrainMeshes[1] || !TrainMeshes[2] || !TrainMeshes[3] || !TrainMeshes[4] || !TrainMeshes[5] || !TrainMeshes[6]) { SetComponentTickEnabled(false); return; }
    if (TrainOwner->MovementExecutor) AddTickPrerequisiteComponent(TrainOwner->MovementExecutor);
    // Sample the rendered formation after its visual component has advanced.
    for (UActorComponent* TrainVisual : TrainOwner->GetComponents())
    {
        if (TrainVisual->IsA<UStrategyArtilleryVisualComponent>() ||
            TrainVisual->IsA<UStrategyInfantryVisualComponent>() || TrainVisual->IsA<UStrategyCavalryVisualComponent>() || TrainVisual->IsA<UStrategyHQVisualComponent>())
            AddTickPrerequisiteComponent(TrainVisual);
    }
    const FVector TrainHead = TrainOwner->GetVisualCentroid();
    const float TrainLength = TrailingCm + WagonCount * 900.f + 1000.f;
    TrainTrail.Add({TrainHead - TrainOwner->GetActorForwardVector() * TrainLength, -TrainLength});
    TrainTrail.Add({TrainHead, 0.f});
    for (int32 TrainIndex = 0; TrainIndex < WagonCount; ++TrainIndex)
    {
        float TrainYaw = 0.f;
        const FVector TrainPlace = UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, SampleTrail(-TrailingCm - TrainIndex * 900.f, TrainYaw));
        const FTransform TrainSpawn(FRotator(0.f, TrainYaw, 0.f), TrainPlace);
        AStrategyWagonTrain* TrainTarget = GetWorld()->SpawnActorDeferred<AStrategyWagonTrain>(
            AStrategyWagonTrain::StaticClass(), TrainSpawn, TrainOwner, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!TrainTarget) break;
        TrainTarget->Side = TrainOwner->Side;
        TrainTarget->StableUnitId = FName(*FString::Printf(TEXT("%s-TRAIN-%d"), *TrainOwner->StableUnitId.ToString(), TrainIndex + 1));
        TrainTarget->DisplayName = FText::FromString(FString::Printf(TEXT("%s - vogn %d"), *TrainOwner->DisplayName.ToString(), TrainIndex + 1));
        TrainTarget->HorseStrength = FMath::Clamp(HorseCount - TrainIndex * 2, 0, 2);
        if (TrainBattery) TrainTarget->HorseStrength = FMath::Min(TrainTarget->HorseStrength, TrainBattery->HorseStrength);
        TrainTarget->bEmptyCargoWagon = TrainMortar && TrainIndex >= TrainMortar->MortarPieceCount;
        TrainTarget->bMortarLoad = !TrainTarget->bEmptyCargoWagon && TrainMortar && TrainMortar->MortarDeploymentComponent && TrainMortar->MortarDeploymentComponent->CanMoveNormally();
        TrainTarget->bAmmunitionWagon = !TrainMortar && (!TrainHQ || TrainHQ->HQLevel < EStrategyHQLevel::Brigade) && (TrainBattery || TrainIndex % 2 == 0);
        TrainTarget->CargoComponent->SmallArmsRounds = TrainTarget->bAmmunitionWagon ? 12000 : 0;
        TrainTarget->CargoComponent->ArtilleryRounds = TrainTarget->bAmmunitionWagon && TrainBattery ? 320 : 0;
        if (TrainBattery) TrainBattery->HorseStrength = FMath::Max(0, TrainBattery->HorseStrength - TrainTarget->HorseStrength);
        TrainTarget->FinishSpawning(TrainSpawn);
        TrainTargets.Add(TrainTarget);
        TrainMeshes[TrainIndex < 2 ? 0 : 1]->AddInstance(FTransform::Identity, true);
        TrainMeshes[TrainIndex < 2 ? 6 : 4]->AddInstance(FTransform::Identity, true);
        for (int32 TrainCrate = 0; TrainCrate < 3; ++TrainCrate) TrainMeshes[5]->AddInstance(FTransform::Identity, true);
        for (int32 TrainHorse = 0; TrainHorse < 2; ++TrainHorse)
        {
            TrainMeshes[2]->AddInstance(FTransform::Identity, true);
            TrainMeshes[3]->AddInstance(FTransform::Identity, true);
        }
    }
    UE_LOG(LogTemp, Log, TEXT("PROJECT1864-TRAIN %s: %d vogne, %d heste; afstand %.0f m"),
        *TrainOwner->StableUnitId.ToString(), TrainTargets.Num(), HorseCount, TrailingCm / 100.f);
}

FVector UStrategyWagonTrainComponent::SampleTrail(float Distance, float& Yaw) const
{
    int32 TrainLow = 0, TrainHigh = TrainTrail.Num() - 1;
    while (TrainHigh - TrainLow > 1)
    {
        const int32 TrainMid = (TrainLow + TrainHigh) / 2;
        if (TrainTrail[TrainMid].Distance <= Distance) TrainLow = TrainMid; else TrainHigh = TrainMid;
    }
    const FTrainSample& TrainA = TrainTrail[TrainLow];
    const FTrainSample& TrainB = TrainTrail[TrainHigh];
    Yaw = (TrainB.Position - TrainA.Position).Rotation().Yaw;
    const float TrainAlpha = FMath::Clamp((Distance - TrainA.Distance) / FMath::Max(0.01f, TrainB.Distance - TrainA.Distance), 0.f, 1.f);
    return FMath::Lerp(TrainA.Position, TrainB.Position, TrainAlpha);
}

void UStrategyWagonTrainComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bBuilt) BuildTrain();
    AStrategyUnit* TrainOwner = TrainUnit.Get();
    if (!TrainOwner || TrainTargets.IsEmpty()) return;
    const bool bTrainHidden = TrainOwner->IsHidden();
    for (UInstancedStaticMeshComponent* TrainISM : TrainMeshes) TrainISM->SetVisibility(!bTrainHidden);
    for (AStrategyWagonTrain* TrainTarget : TrainTargets)
        if (IsValid(TrainTarget)) TrainTarget->bOutOfPlay = TrainOwner->bOutOfPlay &&
            TrainTarget->OwnershipState == EStrategySupplyOwnershipState::Operational; // Captured/abandoned cargo stays on the field.
    if (bTrainHidden || TrainOwner->BattleLedger.bFrozen) return;
    const FVector TrainHead = TrainOwner->GetVisualCentroid();
    const float TrainStep = FVector::Dist2D(TrainHead, TrainTrail.Last().Position);
    if (TrainStep > 0.1f)
    {
        TrainDistance += TrainStep;
        TrainTrail.Add({TrainHead, TrainDistance});
        const float TrainKeep = TrailingCm + WagonCount * 900.f + 1000.f;
        int32 TrainDrop = 0;
        while (TrainDrop + 2 < TrainTrail.Num() && TrainTrail[TrainDrop + 1].Distance < TrainDistance - TrainKeep) ++TrainDrop;
        if (TrainDrop) TrainTrail.RemoveAt(0, TrainDrop, EAllowShrinking::No);
    }
    FVector TrainCamera = FVector::ZeroVector;
    FRotator TrainView;
    const APlayerController* TrainPC = GetWorld()->GetFirstPlayerController();
    if (TrainPC) TrainPC->GetPlayerViewPoint(TrainCamera, TrainView);
    const bool bTrainNearShadow = TrainPC && IsValid(TrainTargets[0]) && FVector::Dist(TrainCamera, TrainTargets[0]->GetActorLocation()) < 25000.f;
    TrainMeshes[0]->SetCastShadow(bTrainNearShadow);
    TrainMeshes[6]->SetCastShadow(bTrainNearShadow);
    const float TrainNow = GetWorld()->GetTimeSeconds();
    AStrategyMortarBatteryUnit* TrainMortarOwner = Cast<AStrategyMortarBatteryUnit>(TrainOwner);
    int32 TrainLoadableMortars = TrainMortarOwner ? FMath::Max(0,
        TrainMortarOwner->MortarPieceCount - TrainMortarOwner->DestroyedGunCount - TrainMortarOwner->DisabledGunCount) : 0;
    // Pieces stranded in abandoned wagons cannot also be packed onto operational wagons.
    for (const AStrategyWagonTrain* TrainStranded : TrainTargets)
        if (IsValid(TrainStranded) && TrainStranded->bMortarLoad && !TrainStranded->bMortarLossRecorded &&
            TrainStranded->OwnershipState != EStrategySupplyOwnershipState::Operational)
            TrainLoadableMortars = FMath::Max(0, TrainLoadableMortars - 1);
    for (int32 TrainIndex = 0; TrainIndex < TrainTargets.Num(); ++TrainIndex)
    {
        AStrategyWagonTrain* TrainTarget = TrainTargets[TrainIndex];
        if (!IsValid(TrainTarget)) continue;
        if (TrainMortarOwner && TrainMortarOwner->MortarDeploymentComponent && !TrainTarget->bMortarLossRecorded &&
            !TrainTarget->bEmptyCargoWagon && TrainTarget->OwnershipState == EStrategySupplyOwnershipState::Operational)
        {
            if (TrainMortarOwner->MortarDeploymentComponent->CanFire()) TrainTarget->bMortarLoad = false;
            else if (TrainMortarOwner->MortarDeploymentComponent->CanMoveNormally())
            {
                TrainTarget->bMortarLoad = TrainLoadableMortars > 0;
                if (TrainTarget->bMortarLoad) --TrainLoadableMortars;
            }
            // Keep the previous cargo ownership during emplacing/packing transitions.
        }
        const FVector TrainBefore = TrainTarget->GetActorLocation();
        float TrainYaw = TrainTarget->GetActorRotation().Yaw;
        FVector TrainPlace = TrainBefore;
        if (!TrainTarget->bOutOfPlay && TrainTarget->OwnershipState == EStrategySupplyOwnershipState::Operational && TrainTarget->CanMoveSupplyWagon())
        {
            TrainPlace = UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, SampleTrail(TrainDistance - TrailingCm - TrainIndex * 900.f, TrainYaw));
            TrainTarget->SetActorLocationAndRotation(TrainPlace, FRotator(0.f, TrainYaw, 0.f));
        }
        const float TrainSpeed = FVector::Dist2D(TrainBefore, TrainPlace) / FMath::Max(0.001f, DeltaTime);
        const float TrainWave = FMath::Sin(TrainNow * (TrainSpeed > 20.f ? 11.f : 2.f) + TrainIndex * 1.7f);
        const bool bTrainDestroyed = TrainTarget->WagonCondition <= 0.f;
        UInstancedStaticMeshComponent* TrainBodies = TrainMeshes[TrainIndex < 2 ? 0 : 1];
        FTransform TrainBody = MeshTransform(TrainBodies->GetStaticMesh(), TrainPlace + FVector(0.f, 0.f, TrainSpeed > 20.f ? TrainWave * 2.f : 0.f),
            FRotator(0.f, TrainYaw, bTrainDestroyed ? 65.f : 0.f), 420.f, 200.f);
        const bool bTrainEmptyBody = TrainTarget->bAmmunitionWagon || TrainTarget->bEmptyCargoWagon ||
            (Cast<AStrategyMortarBatteryUnit>(TrainOwner) && !TrainTarget->bMortarLoad);
        if (bTrainEmptyBody || TrainTarget->bOutOfPlay) TrainBody.SetScale3D(FVector::ZeroVector);
        FTransform TrainAmmoBody = MeshTransform(TrainMeshes[4]->GetStaticMesh(), TrainPlace, FRotator(0.f, TrainYaw, bTrainDestroyed ? 65.f : 0.f), 420.f, 200.f);
        if (!bTrainEmptyBody || TrainTarget->bOutOfPlay) TrainAmmoBody.SetScale3D(FVector::ZeroVector);
        TrainMeshes[TrainIndex < 2 ? 6 : 4]->UpdateInstanceTransform(TrainIndex < 2 ? TrainIndex : TrainIndex - 2, TrainAmmoBody, true, false, true);
        for (int32 TrainCrate = 0; TrainCrate < 3; ++TrainCrate)
        {
            const FRotator TrainCrateFacing(0.f, TrainYaw, 0.f);
            const FVector TrainCratePlace = TrainPlace + TrainCrateFacing.RotateVector(FVector((TrainCrate - 1) * 75.f, 0.f, 140.f));
            TrainMeshes[5]->UpdateInstanceTransform(TrainIndex * 3 + TrainCrate,
                FTransform(TrainCrateFacing, TrainCratePlace, TrainTarget->bAmmunitionWagon && !bTrainDestroyed && !TrainTarget->bOutOfPlay ? FVector(0.65f, 1.2f, 0.5f) : FVector::ZeroVector), true, false, true);
        }
        TrainBodies->UpdateInstanceTransform(TrainIndex < 2 ? TrainIndex : TrainIndex - 2, TrainBody, true, false, true);
        for (int32 TrainHorse = 0; TrainHorse < 2; ++TrainHorse)
        {
            const FRotator TrainFacing(0.f, TrainYaw, 0.f);
            const FVector TrainHorsePlace = TrainPlace + TrainFacing.RotateVector(FVector(350.f, TrainHorse == 0 ? -65.f : 65.f, 0.f));
            const bool bTrainHorseVisible = TrainHorse < TrainTarget->HorseStrength && !bTrainDestroyed && !TrainTarget->bOutOfPlay;
            const float TrainHorseWave = FMath::Sin(TrainNow * (TrainSpeed > 20.f ? 12.f : 2.f) + TrainIndex + TrainHorse * PI);
            FTransform TrainHorseTransform = MeshTransform(TrainMeshes[2]->GetStaticMesh(),
                TrainHorsePlace + FVector(0.f, 0.f, (TrainSpeed > 20.f ? 5.f : 1.f) * TrainHorseWave),
                FRotator(TrainHorseWave * (TrainSpeed > 20.f ? 2.f : 0.4f), TrainYaw, TrainHorseWave * 0.7f), 240.f);
            if (!bTrainHorseVisible) TrainHorseTransform.SetScale3D(FVector::ZeroVector);
            TrainMeshes[2]->UpdateInstanceTransform(TrainIndex * 2 + TrainHorse, TrainHorseTransform, true, false, true);
            FTransform TrainHarness(TrainFacing, TrainPlace + TrainFacing.RotateVector(FVector(215.f, TrainHorse == 0 ? -65.f : 65.f, 95.f)),
                bTrainHorseVisible ? FVector(2.7f, 0.025f, 0.025f) : FVector::ZeroVector);
            TrainMeshes[3]->UpdateInstanceTransform(TrainIndex * 2 + TrainHorse, TrainHarness, true, false, true);
        }
        if (TrainTarget->QAPlaceholderMesh) TrainTarget->QAPlaceholderMesh->SetVisibility(false);
        if (TrainTarget->DebugLabel) TrainTarget->DebugLabel->SetVisibility(false);
    }
    for (UInstancedStaticMeshComponent* TrainISM : TrainMeshes) TrainISM->MarkRenderStateDirty();
    ScanSeconds += DeltaTime;
    ResupplySeconds += DeltaTime;
    if (ScanSeconds >= 1.f) { ScanTrain(); ScanSeconds = 0.f; }
}

int32 UStrategyWagonTrainComponent::GetTransportedMortarCount() const
{
    int32 TrainIntact = 0;
    for (const AStrategyWagonTrain* TrainTarget : TrainTargets)
        if (IsValid(TrainTarget) && !TrainTarget->bMortarLossRecorded && TrainTarget->WagonCondition > 0.f && TrainTarget->bMortarLoad) ++TrainIntact;
    return TrainIntact;
}

void UStrategyWagonTrainComponent::ScanTrain()
{
    const bool bTrainTransfer = ResupplySeconds >= 10.f;
    if (bTrainTransfer) ResupplySeconds = 0.f; // No banking supply time outside the radius.
    for (AStrategyWagonTrain* TrainTarget : TrainTargets)
    {
        if (!IsValid(TrainTarget) || TrainTarget->BattleLedger.bFrozen || TrainTarget->bOutOfPlay || TrainTarget->WagonCondition <= 0.f) continue;
        for (TActorIterator<AStrategyUnit> TrainIt(GetWorld()); TrainIt; ++TrainIt)
        {
            AStrategyUnit* TrainOther = *TrainIt;
            if (TrainOther->bOutOfPlay || !TrainOther->IsCombatEffective() || TrainOther->Side == EStrategySide::Neutral ||
                TrainOther->Echelon == EStrategyEchelon::Supply || TrainOther->Echelon == EStrategyEchelon::Headquarters || Cast<AStrategyHQUnit>(TrainOther)) continue;
            const float TrainRange = FVector::Dist2D(TrainTarget->GetActorLocation(), TrainOther->GetVisualCentroid());
            if (TrainOther->Side != TrainTarget->Side && TrainRange < 350.f && TrainTarget->OwnershipState == EStrategySupplyOwnershipState::Operational)
            {
                TrainTarget->AbandonSupplyWagon(); // Existing guarded capture component completes and records the trophy.
                UE_LOG(LogTemp, Log, TEXT("PROJECT1864-TRAIN %s overløbet"), *TrainTarget->StableUnitId.ToString());
                break;
            }
            if (bTrainTransfer && TrainTarget->bAmmunitionWagon && TrainOther->Side == TrainTarget->Side && TrainRange <= 15000.f &&
                TrainOther->Echelon == EStrategyEchelon::Company && TrainOther->CombatComponent &&
                (TrainTarget->OwnershipState == EStrategySupplyOwnershipState::Operational || TrainTarget->OwnershipState == EStrategySupplyOwnershipState::Captured))
            {
                const int32 TrainMissing = FMath::Max(0, TrainOther->CombatComponent->MaxAmmunitionRounds - TrainOther->CombatComponent->AmmunitionRounds);
                const int32 TrainRounds = TrainTarget->CargoComponent->ConsumeSmallArmsRounds(FMath::Min(120, TrainMissing));
                if (TrainRounds > 0)
                {
                    TrainOther->CombatComponent->ResupplyAmmunition(TrainRounds);
                    UE_LOG(LogTemp, Log, TEXT("PROJECT1864-TRAIN %s -> %s: +%d patroner"), *TrainTarget->StableUnitId.ToString(), *TrainOther->StableUnitId.ToString(), TrainRounds);
                }
            }
        }
    }
}

void UStrategyWagonTrainComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    for (AStrategyWagonTrain* TrainTarget : TrainTargets) if (IsValid(TrainTarget)) TrainTarget->Destroy();
    for (UInstancedStaticMeshComponent* TrainISM : TrainMeshes) if (TrainISM) TrainISM->DestroyComponent();
    Super::EndPlay(Reason);
}
