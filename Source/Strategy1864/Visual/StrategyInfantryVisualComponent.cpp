#include "StrategyInfantryVisualComponent.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Units/StrategyCompanyUnit.h"
#include "StrategyEquipmentVisualComponent.h"
#include "StrategyHumanAnimationStateComponent.h"
#include "StrategyMuzzleSmokePuff.h"
#include "Engine/World.h"

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
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Firing_Rifle.A_Firing_Rifle")));

    IdleKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Kneel_Idle.A_Rifle_Kneel_Idle")));

    AimKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Idle_Crouching_Aiming.A_Idle_Crouching_Aiming")));

    FireKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Fire_Rifle.A_Fire_Rifle")));

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
    DeathAsset2 = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Death_From_Front_Headshot.A_Death_From_Front_Headshot")));
    DeathAsset3 = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Falling_Back_Death.A_Falling_Back_Death")));
    RaiseToAimAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Down_To_Aim.A_Rifle_Down_To_Aim")));
    DeathWalkingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Walking_To_Dying.A_Walking_To_Dying")));
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
        const int32 Desired = GetDesiredVisualCount();
        if (bLeaveCorpses && CachedStrength != INDEX_NONE && Desired < SoldierComponents.Num())
        {
            KillSoldiers(SoldierComponents.Num() - Desired);
        }
        EnsureVisualCount(Desired);
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
    UpdatePersonalActions();
    AlignWeapons();
    UpdateFormationBounds();
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
        UpdateFormationBounds();
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
    UpdateFormationBounds();

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

    // Every soldier of the volley fires his own shot, a little apart from the next: raise, fire, smoke.
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const int32 Count = SoldierComponents.Num();
    SoldierFireAt.SetNumZeroed(Count);
    SoldierBusyUntil.SetNumZeroed(Count);
    SoldierFirePhase.SetNumZeroed(Count);
    const int32 Firing = FMath::Clamp(Shots / FMath::Max(1, VisualScaleDivisor), 1, Count);
    // Those who can bear on the target first (their own place, angle and range); the rest only to make up the number.
    TArray<int32> Order, Others;
    const AStrategyUnit* Target = OwnerCompany->CombatComponent ? OwnerCompany->CombatComponent->LastVolleyTarget.Get() : nullptr;
    const UStrategyFireControlComponent* Fire = OwnerCompany->FireControlComponent;
    for (int32 i = 0; i < Count; ++i)
    {
        const bool bBears = !Target || !Fire || !SoldierComponents[i] ||
            Fire->CanPointBearOn(SoldierComponents[i]->GetComponentLocation(), OwnerCompany->GetActorForwardVector(), Target, Fire->GetActiveRangeCm());
        (bBears ? Order : Others).Add(i);
    }
    for (int32 i = Order.Num() - 1; i > 0; --i) { Order.Swap(i, FMath::RandRange(0, i)); }
    for (int32 k = 0; k < Firing && k < Order.Num(); ++k)
    {
        const int32 i = Order[k];
        if (SoldierFirePhase[i] == 0)
        {
            SoldierFireAt[i] = Now + FMath::FRandRange(0.0f, 0.6f);
        }
    }
}

void UStrategyInfantryVisualComponent::PlayOnSoldier(USkeletalMeshComponent* Soldier, UAnimSequence* Sequence, bool bLooping, bool bRandomStart)
{
    if (!Soldier || !Sequence)
    {
        return;
    }
    Soldier->bPauseAnims = false;
    Soldier->PlayAnimation(Sequence, bLooping);
    // Not all in step: each soldier starts somewhere in a looping animation and at his own pace.
    Soldier->SetPosition(bRandomStart ? FMath::FRandRange(0.0f, Sequence->GetPlayLength()) : 0.0f, false);
    Soldier->SetPlayRate(bRandomStart ? FMath::FRandRange(0.9f, 1.1f) : 1.0f);
}

void UStrategyInfantryVisualComponent::UpdatePersonalActions()
{
    UWorld* World = GetWorld();
    if (!World || SoldierFireAt.Num() != SoldierComponents.Num())
    {
        return;
    }
    const float Now = World->GetTimeSeconds();
    const EStrategyStance Stance = OwnerCompany && OwnerCompany->StanceComponent ? OwnerCompany->StanceComponent->Stance : EStrategyStance::Standing;
    for (int32 i = 0; i < SoldierComponents.Num(); ++i)
    {
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        if (!Soldier)
        {
            continue;
        }
        if (SoldierFirePhase[i] == 0 && SoldierFireAt[i] > 0.0f && Now >= SoldierFireAt[i])
        {
            // Raise the rifle (standing), then the shot.
            UAnimSequence* Raise = Stance == EStrategyStance::Standing ? RaiseToAimAsset.LoadSynchronous() : nullptr;
            if (Raise)
            {
                PlayOnSoldier(Soldier, Raise, false, false);
                SoldierFirePhase[i] = 1;
                SoldierBusyUntil[i] = Now + Raise->GetPlayLength() * 0.85f;
            }
            else
            {
                SoldierFirePhase[i] = 1;
                SoldierBusyUntil[i] = Now;
            }
            SoldierFireAt[i] = 0.0f;
        }
        else if (SoldierFirePhase[i] == 1 && Now >= SoldierBusyUntil[i])
        {
            UAnimSequence* Fire = (Stance == EStrategyStance::Prone ? FireProneAsset : Stance == EStrategyStance::Kneeling ? FireKneelingAsset : FireStandingAsset).LoadSynchronous();
            PlayOnSoldier(Soldier, Fire, false, false);
            SoldierFirePhase[i] = 2;
            SoldierBusyUntil[i] = Now + (Fire ? Fire->GetPlayLength() : 0.6f);
            SpawnMuzzleSmoke(Soldier, i);
        }
        else if (SoldierFirePhase[i] == 2 && Now >= SoldierBusyUntil[i])
        {
            // Back to what the company does (reloading, aiming, marching).
            SoldierFirePhase[i] = 0;
            bool bLooping = true;
            PlayOnSoldier(Soldier, ResolveAnimation(bLooping), bLooping, true);
        }
    }
}

void UStrategyInfantryVisualComponent::SpawnMuzzleSmoke(const USkeletalMeshComponent* Soldier, int32 Index)
{
    UWorld* World = GetWorld();
    if (!bMuzzleSmoke || !World || !Soldier)
    {
        return;
    }
    // At the muzzle: the far end of the rifle, else ahead of the soldier at shoulder height.
    FVector Muzzle = Soldier->GetComponentLocation() + Soldier->GetRightVector() * -70.0f + FVector(0.0f, 0.0f, 150.0f);
    FVector Forward = OwnerCompany ? OwnerCompany->GetActorForwardVector() : FVector::ForwardVector;
    if (WeaponComponents.IsValidIndex(Index) && WeaponComponents[Index] && WeaponComponents[Index]->GetStaticMesh())
    {
        const UStaticMeshComponent* Weapon = WeaponComponents[Index];
        const FBox Box = Weapon->GetStaticMesh()->GetBoundingBox();
        const float Tip = bRifleBarrelAlongNegativeX ? Box.Min.X : Box.Max.X;
        Muzzle = Weapon->GetComponentTransform().TransformPosition(FVector(Tip, 0.0f, 0.0f));
        Forward = Weapon->GetForwardVector() * (bRifleBarrelAlongNegativeX ? -1.0f : 1.0f);
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AStrategyMuzzleSmokePuff* Puff = World->SpawnActor<AStrategyMuzzleSmokePuff>(AStrategyMuzzleSmokePuff::StaticClass(), Muzzle + Forward * 40.0f, FRotator::ZeroRotator, Params))
    {
        Puff->Drift = Forward * 60.0f + FVector(FMath::FRandRange(-15.0f, 15.0f), FMath::FRandRange(-15.0f, 15.0f), 18.0f);
    }
}

void UStrategyInfantryVisualComponent::KillSoldiers(int32 Count)
{
    // The hit: random men of the company fall where they stand and stay there (not with the company).
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const bool bMoving = OwnerCompany && OwnerCompany->HumanAnimationStateComponent &&
        (OwnerCompany->HumanAnimationStateComponent->CurrentAction == EStrategyHumanAnimationAction::Walk ||
         OwnerCompany->HumanAnimationStateComponent->CurrentAction == EStrategyHumanAnimationAction::Run);
    for (int32 k = 0; k < Count && SoldierComponents.Num() > 0; ++k)
    {
        const int32 i = FMath::RandRange(0, SoldierComponents.Num() - 1);
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        if (Soldier)
        {
            Soldier->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            const int32 Variant = FMath::RandRange(0, 2);
            const TSoftObjectPtr<UAnimSequence>& Death = bMoving && !DeathWalkingAsset.IsNull() && FMath::FRand() < 0.5f ? DeathWalkingAsset
                : Variant == 0 ? DeathAsset : Variant == 1 ? DeathAsset2 : DeathAsset3;
            Soldier->SetPlayRate(FMath::FRandRange(0.9f, 1.1f));
            Soldier->bPauseAnims = false;
            Soldier->PlayAnimation(Death.LoadSynchronous(), false);
            CorpseComponents.Add(Soldier);
        }
        SoldierComponents.RemoveAt(i);
        if (WeaponComponents.IsValidIndex(i))
        {
            WeaponComponents.RemoveAt(i);   // the rifle stays in the fallen man's hand
        }
        if (SoldierFireAt.IsValidIndex(i)) { SoldierFireAt.RemoveAt(i); }
        if (SoldierBusyUntil.IsValidIndex(i)) { SoldierBusyUntil.RemoveAt(i); }
        if (SoldierFirePhase.IsValidIndex(i)) { SoldierFirePhase.RemoveAt(i); }
    }
}

void UStrategyInfantryVisualComponent::AlignWeapons()
{
    if (!bAlignRifleBetweenHands)
    {
        return;
    }
    for (int32 i = 0; i < SoldierComponents.Num() && i < WeaponComponents.Num(); ++i)
    {
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        UStaticMeshComponent* Weapon = WeaponComponents[i];
        if (!Soldier || !Weapon || !Weapon->GetStaticMesh())
        {
            continue;
        }
        const FVector Right = Soldier->GetSocketLocation(RightHandBoneName);
        const FVector Left = Soldier->GetSocketLocation(LeftHandBoneName);
        FVector Dir = Left - Right;
        if (Dir.Size() < 12.0f)
        {
            Weapon->SetRelativeTransform(WeaponRelativeTransform);   // hands together: the hand's own grip
            continue;
        }
        Dir.Normalize();
        const FVector Barrel = bRifleBarrelAlongNegativeX ? -Dir : Dir;
        const FRotator Rotation = FRotationMatrix::MakeFromXZ(Barrel, Soldier->GetUpVector()).Rotator();
        const FBox Box = Weapon->GetStaticMesh()->GetBoundingBox();
        const float ButtX = bRifleBarrelAlongNegativeX ? Box.Max.X : Box.Min.X;
        const float GripX = ButtX + (Box.Max.X - Box.Min.X) * RifleGripFraction * (bRifleBarrelAlongNegativeX ? -1.0f : 1.0f);
        Weapon->SetWorldLocationAndRotation(Right - Rotation.RotateVector(FVector(GripX, 0.0f, 0.0f)), Rotation);
    }
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
        if (SoldierFireAt.IsValidIndex(LastIndex)) { SoldierFireAt.RemoveAt(LastIndex); }
        if (SoldierBusyUntil.IsValidIndex(LastIndex)) { SoldierBusyUntil.RemoveAt(LastIndex); }
        if (SoldierFirePhase.IsValidIndex(LastIndex)) { SoldierFirePhase.RemoveAt(LastIndex); }
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
        SoldierFireAt.Add(0.0f);
        SoldierBusyUntil.Add(0.0f);
        SoldierFirePhase.Add(0);
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
            FRotator(0.0f, Slot.FacingYaw + SoldierMeshYawOffset, 0.0f));
    }
}

bool UStrategyInfantryVisualComponent::GetFormationLocalBounds(FBox& OutBounds) const
{
    OutBounds = FormationLocalBounds;
    return bEnabled && OutBounds.IsValid != 0;
}

void UStrategyInfantryVisualComponent::UpdateFormationBounds()
{
    FBox& OutBounds = FormationLocalBounds;
    OutBounds = FBox(ForceInit);
    if (!bEnabled || !LoadedSoldierMesh) return;
    for (const USkeletalMeshComponent* Soldier : SoldierComponents)
    {
        if (IsValid(Soldier))
        {
            // Imported reference-pose bounds stay below ground when an animation
            // moves the hips. Use the evaluated pose so kneeling/prone also fit.
            FBox PoseBounds(ForceInit);
            for (const FTransform& Bone : Soldier->GetComponentSpaceTransforms())
            {
                PoseBounds += Bone.GetLocation();
            }
            if (PoseBounds.IsValid)
            {
                OutBounds += PoseBounds.ExpandBy(20.0f).TransformBy(Soldier->GetRelativeTransform());
            }
        }
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

    const bool bHoldPose = !bAnimateIdle &&
        (Sequence == IdleStandingAsset.Get() || Sequence == IdleKneelingAsset.Get() ||
         Sequence == IdleProneAsset.Get() || Sequence == AimStandingAsset.Get() ||
         Sequence == AimKneelingAsset.Get());

    if (!bForce &&
        LastAnimationAsset == Sequence &&
        bLastAnimationLooping == bLooping &&
        bLastHoldingPose == bHoldPose)
    {
        return;
    }

    for (int32 Index = 0; Index < SoldierComponents.Num(); ++Index)
    {
        USkeletalMeshComponent* Soldier = SoldierComponents[Index];
        if (!Soldier || (SoldierFirePhase.IsValidIndex(Index) && SoldierFirePhase[Index] != 0))
        {
            continue;
        }

        Soldier->bPauseAnims = false;
        Soldier->PlayAnimation(
            Sequence,
            bLooping);
        Soldier->SetPosition(bLooping && !bHoldPose ? FMath::FRandRange(0.0f, Sequence->GetPlayLength()) : 0.0f, false);
        Soldier->SetPlayRate(bLooping && !bHoldPose ? FMath::FRandRange(0.9f, 1.1f) : 1.0f);
        if (bHoldPose)
        {
            Soldier->TickAnimation(0.0f, false);
            Soldier->RefreshBoneTransforms();
        }
        Soldier->bPauseAnims = bHoldPose;
    }

    LastAnimationAsset = Sequence;
    bLastAnimationLooping = bLooping;
    bLastHoldingPose = bHoldPose;
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
        const uint32 Variant = GetTypeHash(OwnerCompany->StableUnitId) % 3u;
        const TSoftObjectPtr<UAnimSequence>& Chosen =
            Variant == 0 ? DeathAsset : (Variant == 1 ? DeathAsset2 : DeathAsset3);
        return Chosen.LoadSynchronous();
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
    SoldierFireAt.Reset();
    SoldierBusyUntil.Reset();
    SoldierFirePhase.Reset();

    LoadedSoldierMesh = nullptr;
    LoadedRifleMesh = nullptr;
    LoadedRifleBayonetMesh = nullptr;
    LastAnimationAsset = nullptr;

    CachedStrength = INDEX_NONE;
    CachedFormationValue = 255;
    bCachedBayonetFixed = false;
    bLastAnimationLooping = false;
    bLastHoldingPose = false;
    bLoadAttempted = false;
    FormationLocalBounds = FBox(ForceInit);
}
