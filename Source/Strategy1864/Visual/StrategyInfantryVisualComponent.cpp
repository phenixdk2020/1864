#include "StrategyInfantryVisualComponent.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Units/StrategyCompanyUnit.h"
#include "StrategyEquipmentVisualComponent.h"
#include "StrategyHumanAnimationStateComponent.h"

UStrategyInfantryVisualComponent::UStrategyInfantryVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = RefreshIntervalSeconds;

    SoldierMeshAsset = TSoftObjectPtr<USkeletalMesh>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864.SK_DK_Livgarden_1864")));

    RifleMeshAsset = TSoftObjectPtr<UStaticMesh>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Weapons/SM_Rifle_1.SM_Rifle_1")));

    RifleBayonetMeshAsset = TSoftObjectPtr<UStaticMesh>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Weapons/SM_Rifle_Bayonet_1.SM_Rifle_Bayonet_1")));

    IdleStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Idle.A_Rifle_Idle")));

    WalkStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Walking_with_rifle.A_Walking_with_rifle")));

    RunStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Running.A_Running")));

    AimStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Aiming_Idle.A_Rifle_Aiming_Idle")));

    FireStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Fire_Rifle.A_Fire_Rifle")));

    IdleKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Kneel_Idle.A_Rifle_Kneel_Idle")));

    AimKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Idle_Crouching_Aiming.A_Idle_Crouching_Aiming")));

    FireKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Firing_Rifle.A_Firing_Rifle")));

    ReloadKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Reload_sitting.A_Reload_sitting")));

    IdleProneAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Prone_Idle.A_Prone_Idle")));

    CrawlProneAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Crawl_Forward.A_Crawl_Forward")));

    FireProneAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Prone_Firing_Rifle.A_Prone_Firing_Rifle")));

    ReloadProneAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Prone_Reloading.A_Prone_Reloading")));

    BayonetChargeAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Charge.A_Charge")));

    BayonetThrustAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Bayonet_Stab.A_Bayonet_Stab")));

    DeathAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Death_From_The_Front.A_Death_From_The_Front")));
}

void UStrategyInfantryVisualComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCompany = Cast<AStrategyCompanyUnit>(GetOwner());

    if (OwnerCompany &&
        OwnerCompany->CombatComponent)
    {
        OwnerCompany->CombatComponent->OnVolleyVisualEvent.AddDynamic(
            this,
            &UStrategyInfantryVisualComponent::HandleVolleyVisualEvent);
    }

    PrimaryComponentTick.TickInterval =
        FMath::Max(0.02f, RefreshIntervalSeconds);

    if (bEnabled)
    {
        RefreshVisuals();
    }
}

void UStrategyInfantryVisualComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (OwnerCompany &&
        OwnerCompany->CombatComponent)
    {
        OwnerCompany->CombatComponent->OnVolleyVisualEvent.RemoveDynamic(
            this,
            &UStrategyInfantryVisualComponent::HandleVolleyVisualEvent);
    }

    DestroyVisualComponents();

    Super::EndPlay(EndPlayReason);
}

void UStrategyInfantryVisualComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);

    if (!bEnabled ||
        !OwnerCompany ||
        !LoadedSoldierMesh)
    {
        return;
    }

    const int32 CurrentStrength =
        FMath::Max(0, OwnerCompany->CurrentStrength);

    const uint8 CurrentFormationValue =
        OwnerCompany->FormationComponent
        ? static_cast<uint8>(
            OwnerCompany->FormationComponent->CurrentFormation)
        : 255;

    if (CachedStrength != CurrentStrength)
    {
        EnsureVisualCount(GetDesiredVisualCount());
        RebuildFormation();
        CachedStrength = CurrentStrength;
    }

    if (CachedFormationValue != CurrentFormationValue)
    {
        RebuildFormation();
        CachedFormationValue = CurrentFormationValue;
    }

    const bool bBayonetFixed =
        OwnerCompany->EquipmentVisualComponent &&
        OwnerCompany->EquipmentVisualComponent->bBayonetFixed;

    if (bCachedBayonetFixed != bBayonetFixed)
    {
        RefreshWeaponMeshes();
        bCachedBayonetFixed = bBayonetFixed;
    }

    RefreshAnimation(false);
}

void UStrategyInfantryVisualComponent::SetEnabled(
    bool bNewEnabled)
{
    if (bEnabled == bNewEnabled &&
        (bEnabled ? SoldierComponents.Num() > 0 : true))
    {
        return;
    }

    bEnabled = bNewEnabled;

    if (!bEnabled)
    {
        DestroyVisualComponents();

        if (OwnerCompany &&
            OwnerCompany->QAPlaceholderMesh)
        {
            OwnerCompany->QAPlaceholderMesh->SetVisibility(true, true);
        }

        return;
    }

    bLoadAttempted = false;
    RefreshVisuals();
}

void UStrategyInfantryVisualComponent::SetVisualScaleDivisor(
    int32 NewDivisor)
{
    const int32 NormalizedDivisor =
        NewDivisor <= 1
        ? 1
        : NewDivisor <= 2
            ? 2
            : NewDivisor <= 5
                ? 5
                : 10;

    if (VisualScaleDivisor == NormalizedDivisor)
    {
        return;
    }

    VisualScaleDivisor = NormalizedDivisor;

    if (bEnabled)
    {
        EnsureVisualCount(GetDesiredVisualCount());
        RebuildFormation();
        RefreshAnimation(true);
    }
}

void UStrategyInfantryVisualComponent::RefreshVisuals()
{
    if (!bEnabled)
    {
        return;
    }

    if (!OwnerCompany)
    {
        OwnerCompany = Cast<AStrategyCompanyUnit>(GetOwner());
    }

    if (!LoadedSoldierMesh)
    {
        bLoadAttempted = false;
    }

    if (!OwnerCompany ||
        !EnsureAssetsLoaded())
    {
        return;
    }

    EnsureVisualCount(GetDesiredVisualCount());
    RebuildFormation();
    RefreshWeaponMeshes();
    RefreshAnimation(true);

    CachedStrength =
        FMath::Max(0, OwnerCompany->CurrentStrength);

    CachedFormationValue =
        OwnerCompany->FormationComponent
        ? static_cast<uint8>(
            OwnerCompany->FormationComponent->CurrentFormation)
        : 255;

    bCachedBayonetFixed =
        OwnerCompany->EquipmentVisualComponent &&
        OwnerCompany->EquipmentVisualComponent->bBayonetFixed;

    if (OwnerCompany->QAPlaceholderMesh)
    {
        OwnerCompany->QAPlaceholderMesh->SetVisibility(false, true);
    }
}

void UStrategyInfantryVisualComponent::HandleVolleyVisualEvent(
    FVector Origin,
    FVector Direction,
    int32 Shots,
    int32 Hits)
{
    if (!bEnabled ||
        !OwnerCompany ||
        !OwnerCompany->HumanAnimationStateComponent)
    {
        return;
    }

    OwnerCompany->HumanAnimationStateComponent->RequestAction(
        EStrategyHumanAnimationAction::Fire,
        0.65f);

    RefreshAnimation(true);
}

bool UStrategyInfantryVisualComponent::EnsureAssetsLoaded()
{
    if (LoadedSoldierMesh)
    {
        return true;
    }

    if (bLoadAttempted)
    {
        return false;
    }

    bLoadAttempted = true;

    LoadedSoldierMesh = SoldierMeshAsset.LoadSynchronous();
    LoadedRifleMesh = RifleMeshAsset.LoadSynchronous();
    LoadedRifleBayonetMesh = RifleBayonetMeshAsset.LoadSynchronous();

    if (!LoadedSoldierMesh)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("PROJECT 1864: Livgarden visual mesh not imported yet: %s"),
            *SoldierMeshAsset.ToSoftObjectPath().ToString());

        return false;
    }

    return true;
}

int32 UStrategyInfantryVisualComponent::GetDesiredVisualCount() const
{
    if (!OwnerCompany)
    {
        return 0;
    }

    const int32 Strength =
        FMath::Max(0, OwnerCompany->CurrentStrength);

    if (Strength <= 0)
    {
        return 0;
    }

    const int32 Divisor =
        VisualScaleDivisor <= 1
        ? 1
        : VisualScaleDivisor <= 2
            ? 2
            : VisualScaleDivisor <= 5
                ? 5
                : 10;

    int32 Desired =
        FMath::CeilToInt(
            static_cast<float>(Strength) /
            static_cast<float>(Divisor));

    if (MaxVisualSoldiers > 0)
    {
        Desired = FMath::Min(
            Desired,
            MaxVisualSoldiers);
    }

    return Desired;
}

void UStrategyInfantryVisualComponent::EnsureVisualCount(
    int32 DesiredCount)
{
    if (!OwnerCompany ||
        !LoadedSoldierMesh)
    {
        return;
    }

    DesiredCount = FMath::Max(0, DesiredCount);

    while (SoldierComponents.Num() > DesiredCount)
    {
        const int32 LastIndex =
            SoldierComponents.Num() - 1;

        if (WeaponComponents.IsValidIndex(LastIndex) &&
            WeaponComponents[LastIndex])
        {
            WeaponComponents[LastIndex]->DestroyComponent();
        }

        if (SoldierComponents[LastIndex])
        {
            SoldierComponents[LastIndex]->DestroyComponent();
        }

        if (WeaponComponents.IsValidIndex(LastIndex))
        {
            WeaponComponents.RemoveAt(LastIndex);
        }

        SoldierComponents.RemoveAt(LastIndex);
    }

    while (SoldierComponents.Num() < DesiredCount)
    {
        USkeletalMeshComponent* Soldier =
            NewObject<USkeletalMeshComponent>(
                OwnerCompany,
                NAME_None,
                RF_Transient);

        if (!Soldier)
        {
            break;
        }

        Soldier->SetSkeletalMeshAsset(LoadedSoldierMesh);
        Soldier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Soldier->SetGenerateOverlapEvents(false);
        Soldier->SetCastShadow(true);
        Soldier->SetupAttachment(OwnerCompany->SceneRoot);
        Soldier->RegisterComponent();

        UStaticMeshComponent* Weapon =
            NewObject<UStaticMeshComponent>(
                OwnerCompany,
                NAME_None,
                RF_Transient);

        if (Weapon)
        {
            Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Weapon->SetGenerateOverlapEvents(false);
            Weapon->SetCastShadow(true);
            Weapon->RegisterComponent();
            Weapon->AttachToComponent(
                Soldier,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                RightHandBoneName);
            Weapon->SetRelativeTransform(WeaponRelativeTransform);
        }

        SoldierComponents.Add(Soldier);
        WeaponComponents.Add(Weapon);
    }

    LastAnimationAsset = nullptr;
}

void UStrategyInfantryVisualComponent::RebuildFormation()
{
    if (!OwnerCompany ||
        !OwnerCompany->FormationComponent ||
        SoldierComponents.Num() == 0)
    {
        return;
    }

    const int32 Strength =
        FMath::Max(0, OwnerCompany->CurrentStrength);

    if (Strength <= 0)
    {
        return;
    }

    const TArray<FStrategyFormationSlot> FullSlots =
        OwnerCompany->FormationComponent->GenerateSoldierSlots(
            FVector::ZeroVector,
            0.0f,
            Strength);

    if (FullSlots.Num() == 0)
    {
        return;
    }

    const int32 RenderedCount =
        SoldierComponents.Num();

    for (int32 VisualIndex = 0;
         VisualIndex < RenderedCount;
         ++VisualIndex)
    {
        USkeletalMeshComponent* Soldier =
            SoldierComponents[VisualIndex];

        if (!Soldier)
        {
            continue;
        }

        const int32 FullIndex =
            RenderedCount <= 1
            ? 0
            : FMath::Clamp(
                FMath::RoundToInt(
                    static_cast<float>(VisualIndex) *
                    static_cast<float>(FullSlots.Num() - 1) /
                    static_cast<float>(RenderedCount - 1)),
                0,
                FullSlots.Num() - 1);

        const FStrategyFormationSlot& Slot =
            FullSlots[FullIndex];

        Soldier->SetRelativeLocation(
            Slot.WorldLocation);

        Soldier->SetRelativeRotation(
            FRotator(0.0f, Slot.FacingYaw, 0.0f));
    }
}

void UStrategyInfantryVisualComponent::RefreshAnimation(
    bool bForce)
{
    if (SoldierComponents.Num() == 0)
    {
        return;
    }

    bool bLooping = true;
    UAnimSequence* Sequence =
        ResolveAnimation(bLooping);

    if (!Sequence)
    {
        return;
    }

    if (!bForce &&
        LastAnimationAsset == Sequence &&
        bLastAnimationLooping == bLooping)
    {
        return;
    }

    for (USkeletalMeshComponent* Soldier :
         SoldierComponents)
    {
        if (!Soldier)
        {
            continue;
        }

        Soldier->PlayAnimation(
            Sequence,
            bLooping);
    }

    LastAnimationAsset = Sequence;
    bLastAnimationLooping = bLooping;
}

void UStrategyInfantryVisualComponent::RefreshWeaponMeshes()
{
    if (!OwnerCompany)
    {
        return;
    }

    const bool bUseBayonet =
        OwnerCompany->EquipmentVisualComponent &&
        OwnerCompany->EquipmentVisualComponent->bBayonetFixed;

    UStaticMesh* DesiredMesh =
        bUseBayonet &&
        LoadedRifleBayonetMesh
        ? LoadedRifleBayonetMesh
        : LoadedRifleMesh;

    for (UStaticMeshComponent* Weapon :
         WeaponComponents)
    {
        if (Weapon)
        {
            Weapon->SetStaticMesh(DesiredMesh);
            Weapon->SetRelativeTransform(
                WeaponRelativeTransform);
        }
    }
}

UAnimSequence* UStrategyInfantryVisualComponent::ResolveAnimation(
    bool& bOutLooping) const
{
    bOutLooping = true;

    if (!OwnerCompany)
    {
        return IdleStandingAsset.LoadSynchronous();
    }

    const EStrategyStance CurrentStance =
        OwnerCompany->StanceComponent
        ? OwnerCompany->StanceComponent->Stance
        : EStrategyStance::Standing;

    const EStrategyHumanAnimationAction Action =
        OwnerCompany->HumanAnimationStateComponent
        ? OwnerCompany->HumanAnimationStateComponent->CurrentAction
        : EStrategyHumanAnimationAction::Idle;

    if (OwnerCompany->UnitState ==
        EStrategyUnitState::Destroyed)
    {
        bOutLooping = false;
        return DeathAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Fire)
    {
        bOutLooping = false;

        if (CurrentStance == EStrategyStance::Prone &&
            !FireProneAsset.IsNull())
        {
            return FireProneAsset.LoadSynchronous();
        }

        if (CurrentStance == EStrategyStance::Kneeling &&
            !FireKneelingAsset.IsNull())
        {
            return FireKneelingAsset.LoadSynchronous();
        }

        return FireStandingAsset.LoadSynchronous();
    }

    if (OwnerCompany->CombatComponent &&
        OwnerCompany->CombatComponent->IsReloading())
    {
        bOutLooping = true;

        if (CurrentStance == EStrategyStance::Prone &&
            !ReloadProneAsset.IsNull())
        {
            return ReloadProneAsset.LoadSynchronous();
        }

        if (CurrentStance == EStrategyStance::Kneeling &&
            !ReloadKneelingAsset.IsNull())
        {
            return ReloadKneelingAsset.LoadSynchronous();
        }

        if (!ReloadStandingAsset.IsNull())
        {
            return ReloadStandingAsset.LoadSynchronous();
        }

        return AimStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::BayonetCharge)
    {
        return BayonetChargeAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::BayonetReady)
    {
        return AimStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Run ||
        Action ==
        EStrategyHumanAnimationAction::RoutedRun)
    {
        return RunStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Walk)
    {
        if (CurrentStance == EStrategyStance::Prone &&
            !CrawlProneAsset.IsNull())
        {
            return CrawlProneAsset.LoadSynchronous();
        }

        return WalkStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Aim)
    {
        if (CurrentStance == EStrategyStance::Prone &&
            !IdleProneAsset.IsNull())
        {
            return IdleProneAsset.LoadSynchronous();
        }

        if (CurrentStance == EStrategyStance::Kneeling &&
            !AimKneelingAsset.IsNull())
        {
            return AimKneelingAsset.LoadSynchronous();
        }

        return AimStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Prone ||
        CurrentStance ==
        EStrategyStance::Prone)
    {
        return IdleProneAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Kneel ||
        CurrentStance ==
        EStrategyStance::Kneeling)
    {
        return IdleKneelingAsset.LoadSynchronous();
    }

    return IdleStandingAsset.LoadSynchronous();
}

void UStrategyInfantryVisualComponent::DestroyVisualComponents()
{
    for (UStaticMeshComponent* Weapon :
         WeaponComponents)
    {
        if (Weapon)
        {
            Weapon->DestroyComponent();
        }
    }

    for (USkeletalMeshComponent* Soldier :
         SoldierComponents)
    {
        if (Soldier)
        {
            Soldier->DestroyComponent();
        }
    }

    WeaponComponents.Reset();
    SoldierComponents.Reset();

    LoadedSoldierMesh = nullptr;
    LoadedRifleMesh = nullptr;
    LoadedRifleBayonetMesh = nullptr;
    LastAnimationAsset = nullptr;

    CachedStrength = INDEX_NONE;
    CachedFormationValue = 255;
    bCachedBayonetFixed = false;
    bLastAnimationLooping = false;
    bLoadAttempted = false;
}
