#include "StrategyLimberVisualComponent.h"

#include "StrategyArtilleryVisualComponent.h"
#include "StrategyVisualFormationPath.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryDeploymentComponent.h"
#include "../Artillery/StrategyMortarBatteryUnit.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

namespace
{
    // The cavalry's rigged horse and gaits, the riders' clips (as StrategyHQVisualComponent).
    const TCHAR* LimberHorseClipPaths[5] = {
        TEXT("/Game/Units/Horse/A_Horse_Stand.A_Horse_Stand"), TEXT("/Game/Units/Horse/A_Horse_M2M_Walk.A_Horse_M2M_Walk"),
        TEXT("/Game/Units/Horse/A_Horse_M2M_Trot.A_Horse_M2M_Trot"), TEXT("/Game/Units/Horse/A_Horse_Canter.A_Horse_Canter"),
        TEXT("/Game/Units/Horse/A_Horse_Gallop.A_Horse_Gallop")};
    const TCHAR* LimberDriverClipPaths[5] = {
        TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Reload_sitting.A_Reload_sitting"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavWalk.A_Combat_CavWalk"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavTrotRise.A_Combat_CavTrotRise"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavCanter.A_Combat_CavCanter"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_CavGallop.A_Combat_CavGallop")};
    const FName LimberSaddleBone(TEXT("saddle_root"));
    const float LimberHorseLengthCm = 219.0f;
    // As the cavalry: the gait changes where the clip's stride still covers the ground (no sliding hooves).
    const float LimberGaitThresholds[4] = {60.f, 190.f, 400.f, 640.f};
    const float LimberClipSpeedCmS[5] = {0.f, 132.f, 250.f, 357.f, 757.f};
    const float LimberRateMin[5] = {1.f, 0.45f, 0.75f, 0.85f, 0.8f};
    const float LimberRateMax[5] = {1.f, 1.6f, 1.6f, 1.8f, 1.6f};
    const float LimberRiderSeatCm = 48.0f;
    const float LimberSaddleLiftCm = 6.0f;
    // The team's layout in its own frame (X ahead, Y right, cm; an estimate after a six-horse field-gun team): the limber 210 cm
    // long, the wheel pair's middle 265 cm ahead of the limber's, each pair 260 cm ahead of the next, the horses 65 cm either side
    // of the pole; the drivers ride the near-side (left) horses. Hooked, the limber's middle is 350 cm ahead of the gun's.
    const float LimberLengthCm = 210.0f;
    const float LimberWheelPairCm = 265.0f;
    const float LimberPairSpacingCm = 260.0f;
    const float LimberHorseSideCm = 65.0f;
    const float LimberHookCm = 350.0f;
    // In action the limber stands 13 m behind its gun with the pole to the rear (the team ready to drive off), as the drill of the day.
    const float LimberPostCm = 1300.0f;
    // Driving: at most a brisk trot.
    const float LimberTopSpeedCmS = 550.0f;
}

UStrategyLimberVisualComponent::UStrategyLimberVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UStrategyLimberVisualComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
    if (Cast<AStrategyMortarBatteryUnit>(GetOwner())) { OwnerBattery = nullptr; SetComponentTickEnabled(false); return; }
    if (OwnerBattery && OwnerBattery->ArtilleryVisualComponent) AddTickPrerequisiteComponent(OwnerBattery->ArtilleryVisualComponent);
}

bool UStrategyLimberVisualComponent::BuildTeams()
{
    if (!OwnerBattery || !OwnerBattery->ArtilleryVisualComponent) return false;
    const int32 LimberGuns = OwnerBattery->ArtilleryVisualComponent->GetDrawnGunCount();
    if (LimberGuns <= 0) return false;
    HorseMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Units/Horse/SK_Horse_1864.SK_Horse_1864"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    UStaticMesh* LimberMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Units/Items/SM_Wagon_Empty.SM_Wagon_Empty"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    UStaticMesh* LimberPoleMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    HorseClips.Reset();
    DriverClips.Reset();
    for (int32 LimberClip = 0; LimberClip < 5; ++LimberClip)
    {
        HorseClips.Add(LoadObject<UAnimSequence>(nullptr, LimberHorseClipPaths[LimberClip], nullptr, LOAD_NoWarn | LOAD_Quiet));
        DriverClips.Add(LoadObject<UAnimSequence>(nullptr, LimberDriverClipPaths[LimberClip], nullptr, LOAD_NoWarn | LOAD_Quiet));
    }
    if (!HorseMesh || !LimberMesh || HorseClips.Contains(nullptr)) return false;
    HorseStandExtras.Reset();
    for (const TCHAR* LimberExtraPath : { TEXT("/Game/Units/Horse/A_Horse_Idle2.A_Horse_Idle2"), TEXT("/Game/Units/Horse/A_Horse_Eat.A_Horse_Eat") })
        if (UAnimSequence* LimberExtra = LoadObject<UAnimSequence>(nullptr, LimberExtraPath, nullptr, LOAD_NoWarn | LOAD_Quiet)) HorseStandExtras.Add(LimberExtra);
    // The drivers are the battery's own gunners (their nation's mesh), else none.
    const FString LimberNation = OwnerBattery->GetNationCodeOrDefault();
    DriverMesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Units/Artillery/Crew/SK_Artillery_1864_%s.SK_Artillery_1864_%s"), *LimberNation, *LimberNation),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    // As the cavalry and the staff: scaled to a horse's length, feet on the ground, forward from the rest pose (pelvis to head).
    const FBoxSphereBounds LimberBounds = HorseMesh->GetBounds();
    const FVector LimberSize = LimberBounds.BoxExtent * 2.0;
    HorseScale = LimberHorseLengthCm / FMath::Max(1.0f, float(FMath::Max(LimberSize.X, LimberSize.Y)));
    HorseLift = -(LimberBounds.Origin.Z - LimberBounds.BoxExtent.Z) * HorseScale;
    const FReferenceSkeleton& LimberRef = HorseMesh->GetRefSkeleton();
    if (LimberRef.FindBoneIndex(TEXT("head")) != INDEX_NONE && LimberRef.FindBoneIndex(TEXT("pelvis")) != INDEX_NONE)
    {
        const FVector LimberForward = HorseMesh->GetComposedRefPoseMatrix(FName(TEXT("head"))).GetOrigin() -
            HorseMesh->GetComposedRefPoseMatrix(FName(TEXT("pelvis"))).GetOrigin();
        HorseYaw = -FMath::RadiansToDegrees(FMath::Atan2(LimberForward.Y, LimberForward.X));
    }
    bSaddleBone = LimberRef.FindBoneIndex(LimberSaddleBone) != INDEX_NONE;
    SaddleHeightCm = bSaddleBone ? HorseMesh->GetComposedRefPoseMatrix(LimberSaddleBone).GetOrigin().Z * HorseScale : LimberSize.Z * HorseScale * 0.72f;

    // Six horses a gun at most, as many as the battery has (HorseStrength over its guns).
    const int32 LimberHorsesPerGun = FMath::Clamp(FMath::RoundToInt(float(OwnerBattery->HorseStrength) / float(LimberGuns)), 0, 6) / 2 * 2;
    const auto LimberMakeSkeletal = [&](USkeletalMesh* LimberMeshAsset)
    {
        USkeletalMeshComponent* LimberSk = NewObject<USkeletalMeshComponent>(OwnerBattery, NAME_None, RF_Transient);
        LimberSk->SetSkeletalMeshAsset(LimberMeshAsset);
        LimberSk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        LimberSk->SetGenerateOverlapEvents(false);
        LimberSk->bVisibleInRayTracing = false;
        LimberSk->SetupAttachment(OwnerBattery->GetRootComponent());
        LimberSk->SetUsingAbsoluteLocation(true);
        LimberSk->SetUsingAbsoluteRotation(true);
        LimberSk->SetUsingAbsoluteScale(true);
        LimberSk->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        LimberSk->RegisterComponent();
        return LimberSk;
    };
    const auto LimberMakeStatic = [&](UStaticMesh* LimberMeshAsset)
    {
        UStaticMeshComponent* LimberSm = NewObject<UStaticMeshComponent>(OwnerBattery, NAME_None, RF_Transient);
        LimberSm->SetStaticMesh(LimberMeshAsset);
        LimberSm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        LimberSm->SetGenerateOverlapEvents(false);
        LimberSm->bVisibleInRayTracing = false;
        LimberSm->SetupAttachment(OwnerBattery->GetRootComponent());
        LimberSm->SetUsingAbsoluteLocation(true);
        LimberSm->SetUsingAbsoluteRotation(true);
        LimberSm->SetUsingAbsoluteScale(true);
        LimberSm->RegisterComponent();
        return LimberSm;
    };
    for (int32 LimberGun = 0; LimberGun < LimberGuns; ++LimberGun)
    {
        FLimberTeam LimberTeam;
        LimberTeam.Limber = LimberMakeStatic(LimberMesh);
        if (LimberPoleMesh) { LimberTeam.Pole = LimberMakeStatic(LimberPoleMesh); LimberTeam.Pole->SetCastShadow(false); }
        for (int32 LimberHorseIndex = 0; LimberHorseIndex < LimberHorsesPerGun; ++LimberHorseIndex)
        {
            FLimberHorse LimberHorse;
            LimberHorse.Horse = LimberMakeSkeletal(HorseMesh);
            LimberHorse.GaitPhase = 0.23f * LimberHorseIndex + 0.11f * LimberGun;
            PlayHorseClip(LimberHorse, HorseClips[0], 1.f);
            // A driver on each near-side (left, the even index) horse.
            if (DriverMesh && LimberHorseIndex % 2 == 0) LimberHorse.Driver = LimberMakeSkeletal(DriverMesh);
            LimberTeam.Horses.Add(LimberHorse);
        }
        Teams.Add(LimberTeam);
    }
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-LIMBER %s: %d limbers, %d horses each, drivers %s, rider clips %d%d%d%d%d, saddle %.0f cm"),
        *OwnerBattery->StableUnitId.ToString(), Teams.Num(), LimberHorsesPerGun, DriverMesh ? *DriverMesh->GetName() : TEXT("-"),
        DriverClips[0] != nullptr, DriverClips[1] != nullptr, DriverClips[2] != nullptr, DriverClips[3] != nullptr, DriverClips[4] != nullptr, SaddleHeightCm);
    return true;
}

void UStrategyLimberVisualComponent::PlayHorseClip(FLimberHorse& LimberHorse, UAnimSequence* LimberClip, float LimberRate)
{
    if (!LimberHorse.Horse || !LimberClip) return;
    // A change of gait keeps the stride's fraction.
    LimberHorse.Horse->PlayAnimation(LimberClip, true);
    LimberHorse.Horse->SetPlayRate(LimberRate);
    LimberHorse.Horse->SetPosition(FMath::Clamp(LimberHorse.GaitPhase, 0.f, 0.999f) * LimberClip->GetPlayLength(), false);
    LimberHorse.ClipNow = LimberClip;
}

void UStrategyLimberVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!OwnerBattery || !OwnerBattery->ArtilleryVisualComponent || !GetWorld() || DeltaTime <= KINDA_SMALL_NUMBER) return;
    if (!bBuildTried)
    {
        if (OwnerBattery->ArtilleryVisualComponent->GetDrawnGunCount() <= 0) return;
        bBuildTried = true;
        if (!BuildTeams()) { UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-LIMBER %s: no rigged horse or limber mesh, the old team stays"), *OwnerBattery->StableUnitId.ToString()); return; }
    }
    if (Teams.IsEmpty()) return;
    const bool bLimberHidden = OwnerBattery->bOutOfPlay || OwnerBattery->IsHidden();
    const int32 LimberWorking = OwnerBattery->GetOperationalGunCount();
    const bool bLimberProbe = GetWorld()->GetTimeSeconds() - LimberProbeLast >= 2.0;
    if (bLimberProbe) LimberProbeLast = GetWorld()->GetTimeSeconds();
    for (int32 LimberGun = 0; LimberGun < Teams.Num(); ++LimberGun)
    {
        FLimberTeam& Team = Teams[LimberGun];
        FVector LimberGunAt, LimberMuzzle;
        bool bLimberInAction = false;
        const bool bLimberHasGun = OwnerBattery->ArtilleryVisualComponent->GetGunPlacement(LimberGun, LimberGunAt, LimberMuzzle, bLimberInAction, true);
        // At the post or at the trail by the battery's own state, not the gun's pose (2026-11-08: the gun re-laid for the march
        // read as "in action" for a frame and the team let go of the trail): limbering up, the team drives to the trail at once
        // (the gun still faces the enemy: the trail is behind it); unlimbering, it drives off to its post.
        if (const UStrategyArtilleryDeploymentComponent* LimberDeploy = OwnerBattery->DeploymentComponent.Get())
            bLimberInAction = !(LimberDeploy->IsLimbered() || LimberDeploy->IsLimbering());
        const bool bLimberShown = !bLimberHidden && bLimberHasGun && LimberGun < LimberWorking;
        Team.Limber->SetVisibility(bLimberShown);
        if (Team.Pole) Team.Pole->SetVisibility(bLimberShown);
        for (FLimberHorse& LimberHorse : Team.Horses)
        {
            LimberHorse.Horse->SetVisibility(bLimberShown);
            if (LimberHorse.Driver) LimberHorse.Driver->SetVisibility(bLimberShown);
        }
        if (!bLimberShown) { Team.bPlaced = false; continue; }

        // Where the team belongs: hooked to the trail on the march (the muzzle points to the rear), or at its post behind the gun.
        const FVector LimberTrailward = (-LimberMuzzle).GetSafeNormal2D();
        FVector LimberGoal;
        float LimberGoalYaw;
        if (bLimberInAction)
        {
            // Behind the battery's front, not along the muzzle: a gun laid a few degrees aside moved the post a metre and the team
            // drove in circles (2026-11-08 test).
            const FVector LimberBatteryRear = (-OwnerBattery->GetActorForwardVector()).GetSafeNormal2D();
            LimberGoal = LimberGunAt + LimberBatteryRear * LimberPostCm;
            LimberGoalYaw = LimberBatteryRear.Rotation().Yaw;
        }
        else
        {
            LimberGoal = LimberGunAt + LimberTrailward * LimberHookCm;
            LimberGoalYaw = LimberTrailward.Rotation().Yaw;
        }
        LimberGoal.Z = LimberGunAt.Z;
        const FVector LimberBefore = Team.Place;
        if (!Team.bPlaced)
        {
            Team.Place = LimberGoal;
            Team.Yaw = LimberGoalYaw;
            Team.bPlaced = true;
            Team.bHooked = !bLimberInAction;
            Team.bParked = bLimberInAction;
        }
        if (bLimberInAction) Team.bHooked = false; else Team.bParked = false;
        // Parked at its post, the team stays while the gun recoils and is laid; it drives again only when the gun has moved off.
        if (Team.bParked && FVector::Dist2D(Team.Place, LimberGoal) > 400.f) Team.bParked = false;
        if (Team.bParked)
        {
            Team.Speed = 0.f;
        }
        else if (Team.bHooked)
        {
            // Hooked: the limber goes where the trail goes.
            Team.Place = LimberGoal;
            Team.Yaw = LimberGoalYaw;
        }
        else
        {
            // Driving: a smooth curve from the team's place and heading to the goal and its heading (Hermite, the tangents as
            // long as the way, so a goal behind the team is reached in a loop), walked at a team's pace by its length; planned
            // anew when the goal moves off (2026-11-08: steering at a turning circle spiralled round the post and arrived late).
            const FVector LimberGoalAhead = FRotator(0.f, LimberGoalYaw, 0.f).Vector();
            if (!Team.bDriving || FVector::Dist2D(Team.DriveTo, LimberGoal) > 300.f)
            {
                Team.DriveFrom = Team.Place;
                Team.DriveFromAhead = FRotator(0.f, Team.Yaw, 0.f).Vector();
                Team.DriveTo = LimberGoal;
                Team.DriveToAhead = LimberGoalAhead;
                Team.DriveTangent = FMath::Max(600.f, float(FVector::Dist2D(Team.DriveFrom, Team.DriveTo))) * 1.2f;
                Team.DriveLength = 0.f;
                FVector LimberPrev = Team.DriveFrom;
                for (int32 LimberStep = 1; LimberStep <= 24; ++LimberStep)
                {
                    const FVector LimberPoint = FMath::CubicInterp(Team.DriveFrom, Team.DriveFromAhead * Team.DriveTangent, Team.DriveTo, Team.DriveToAhead * Team.DriveTangent, LimberStep / 24.f);
                    Team.DriveLength += float(FVector::Dist2D(LimberPrev, LimberPoint));
                    LimberPrev = LimberPoint;
                }
                Team.DriveDone = 0.f;
                if (!Team.bDriving) Team.DriveSpeed = 0.f;
                Team.bDriving = true;
            }
            const float LimberLeft = FMath::Max(0.f, Team.DriveLength - Team.DriveDone);
            // A walk to start and to stop, a trot on the way (about 1.5 m/s squared).
            const float LimberWanted = FMath::Min(LimberTopSpeedCmS, FMath::Max(40.f, FMath::Sqrt(2.f * 150.f * LimberLeft)));
            Team.DriveSpeed = FMath::FInterpConstantTo(Team.DriveSpeed, LimberWanted, DeltaTime, 150.f);
            Team.DriveDone = FMath::Min(Team.DriveLength, Team.DriveDone + Team.DriveSpeed * DeltaTime);
            const float LimberU = Team.DriveLength > 1.f ? Team.DriveDone / Team.DriveLength : 1.f;
            const FVector LimberOnCurve = FMath::CubicInterp(Team.DriveFrom, Team.DriveFromAhead * Team.DriveTangent, Team.DriveTo, Team.DriveToAhead * Team.DriveTangent, LimberU);
            const FVector LimberCurveAhead = FMath::CubicInterpDerivative(Team.DriveFrom, Team.DriveFromAhead * Team.DriveTangent, Team.DriveTo, Team.DriveToAhead * Team.DriveTangent, LimberU);
            Team.Place = FVector(LimberOnCurve.X, LimberOnCurve.Y, Team.Place.Z);
            if (LimberCurveAhead.SizeSquared2D() > 1.f) Team.Yaw = FRotator(0.f, LimberCurveAhead.Rotation().Yaw, 0.f).Yaw;
            if (LimberU >= 1.f)
            {
                // At the end of the curve; the goal may have drifted a little since (under 3 m): the last step is short.
                Team.Place = FVector(LimberGoal.X, LimberGoal.Y, Team.Place.Z);
                Team.Yaw = LimberGoalYaw;
                Team.bDriving = false;
                if (bLimberInAction) Team.bParked = true; else Team.bHooked = true;
            }
        }
        Team.Place.Z = UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, Team.Place);
        FVector LimberMoved = Team.Place - LimberBefore;
        LimberMoved.Z = 0.f;
        const float LimberSpeed = LimberMoved.Size2D() / DeltaTime;
        Team.Speed = FMath::Lerp(float(Team.Speed), FMath::Min(LimberSpeed, 1200.f), 1.f - FMath::Exp(-6.f * DeltaTime));
        if (LimberMoved.Size2D() > 600.f) Team.Speed = 0.f;   // a jump (the battery placed anew): no gallop

        // The limber (the wagon's long side along the team), the pole from it to the leaders' pair.
        const FRotator LimberFacing(0.f, Team.Yaw, 0.f);
        if (UStaticMesh* LimberMeshNow = Team.Limber->GetStaticMesh())
        {
            const FBox LimberBox = LimberMeshNow->GetBoundingBox();
            const FVector LimberBoxSize = LimberBox.GetSize();
            const bool bLimberAlongX = LimberBoxSize.X >= LimberBoxSize.Y;
            const float LimberScale = LimberLengthCm / FMath::Max(1.f, float(bLimberAlongX ? LimberBoxSize.X : LimberBoxSize.Y));
            const FRotator LimberRotation = LimberFacing + FRotator(0.f, bLimberAlongX ? 0.f : -90.f, 0.f);
            const FVector LimberPivot(LimberBox.GetCenter().X, LimberBox.GetCenter().Y, LimberBox.Min.Z);
            Team.Limber->SetWorldTransform(FTransform(LimberRotation, Team.Place - LimberRotation.RotateVector(LimberPivot * LimberScale), FVector(LimberScale)));
        }
        const int32 LimberPairs = Team.Horses.Num() / 2;
        if (Team.Pole)
        {
            const float LimberPoleLength = LimberWheelPairCm + FMath::Max(0, LimberPairs - 1) * LimberPairSpacingCm - LimberLengthCm * 0.5f;
            const FVector LimberPoleMiddle = Team.Place + LimberFacing.RotateVector(FVector(LimberLengthCm * 0.5f + LimberPoleLength * 0.5f, 0.f, 95.f));
            Team.Pole->SetWorldTransform(FTransform(LimberFacing, LimberPoleMiddle, FVector(LimberPairs > 0 ? LimberPoleLength / 100.f : 0.f, 0.05f, 0.05f)));
        }
        // The horses in the team's gait, the drivers in the saddle.
        int32 LimberGait = 0;
        const int32 LimberGaitBefore = Team.Horses.Num() > 0 ? Team.Horses[0].Gait : 0;
        for (int32 LimberThreshold = 0; LimberThreshold < 4; ++LimberThreshold)
            if (Team.Speed >= LimberGaitThresholds[LimberThreshold] * (LimberGaitBefore > LimberThreshold ? 0.9f : 1.f)) LimberGait = LimberThreshold + 1;
        const float LimberRate = LimberGait == 0 ? 1.f : FMath::Clamp(float(Team.Speed) / LimberClipSpeedCmS[LimberGait], LimberRateMin[LimberGait], LimberRateMax[LimberGait]);
        for (int32 LimberHorseIndex = 0; LimberHorseIndex < Team.Horses.Num(); ++LimberHorseIndex)
        {
            FLimberHorse& LimberHorse = Team.Horses[LimberHorseIndex];
            const int32 LimberPair = LimberHorseIndex / 2;
            const float LimberSide = LimberHorseIndex % 2 == 0 ? -1.f : 1.f;
            FVector LimberHorseAt = Team.Place + LimberFacing.RotateVector(FVector(LimberWheelPairCm + LimberPair * LimberPairSpacingCm, LimberSide * LimberHorseSideCm, 0.f));
            LimberHorseAt.Z = UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, LimberHorseAt);
            LimberHorse.Gait = LimberGait;
            UAnimSequence* LimberHorseClip = HorseClips[LimberGait].Get();
            // Standing, the horses do not all stand alike: some shift their weight, some graze (Mesh2Motion, CC0, 2026-11-08).
            if (LimberGait == 0 && HorseStandExtras.Num() > 0)
            {
                const int32 LimberStandPick = (LimberHorseIndex + LimberGun) % (HorseStandExtras.Num() + 1);
                if (LimberStandPick > 0 && HorseStandExtras[LimberStandPick - 1]) LimberHorseClip = HorseStandExtras[LimberStandPick - 1].Get();
            }
            if (LimberHorse.ClipNow == LimberHorseClip)
            {
                LimberHorse.GaitPhase = FMath::Clamp(LimberHorse.Horse->GetPosition() / FMath::Max(0.1f, LimberHorseClip->GetPlayLength()), 0.f, 0.999f);
                LimberHorse.Horse->SetPlayRate(LimberRate);
            }
            else
            {
                PlayHorseClip(LimberHorse, LimberHorseClip, LimberRate);
            }
            LimberHorse.Horse->SetWorldLocationAndRotation(LimberHorseAt + FVector(0.f, 0.f, HorseLift), FRotator(0.f, Team.Yaw + HorseYaw, 0.f));
            LimberHorse.Horse->SetWorldScale3D(FVector(HorseScale));
            if (LimberHorse.Driver)
            {
                FVector LimberSeat = LimberHorseAt + FVector(0.f, 0.f, HorseLift + SaddleHeightCm - LimberRiderSeatCm + LimberSaddleLiftCm);
                if (bSaddleBone && LimberHorse.Horse->WasRecentlyRendered(0.3f))
                    LimberSeat = LimberHorse.Horse->GetSocketLocation(LimberSaddleBone) - FVector(0.f, 0.f, LimberRiderSeatCm - LimberSaddleLiftCm);
                LimberHorse.Driver->SetWorldLocationAndRotation(LimberSeat, FRotator(0.f, Team.Yaw - 90.f, 0.f));
                UAnimSequence* LimberDriverClip = DriverClips.IsValidIndex(LimberGait) && DriverClips[LimberGait] ? DriverClips[LimberGait].Get() : DriverClips[0].Get();
                const float LimberDriverPose = LimberGait > 0 ? LimberHorse.GaitPhase * (LimberDriverClip ? LimberDriverClip->GetPlayLength() : 0.f) : 0.05f;
                if (LimberDriverClip && LimberHorse.DriverClipNow != LimberDriverClip)
                {
                    LimberHorse.Driver->PlayAnimation(LimberDriverClip, false);
                    LimberHorse.Driver->bPauseAnims = true;
                    LimberHorse.DriverClipNow = LimberDriverClip;
                }
                if (LimberDriverClip) LimberHorse.Driver->SetPosition(FMath::Min(LimberDriverPose, LimberDriverClip->GetPlayLength()), false);
            }
        }
        if (bLimberProbe && LimberGun == 0)
        {
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-LIMBERPROBE %s gun0 mobility=%d action=%d hooked=%d to-goal=%.0f cm speed=%.0f gait=%d yaw-off=%.0f"),
                *OwnerBattery->StableUnitId.ToString(), OwnerBattery->DeploymentComponent ? int32(OwnerBattery->DeploymentComponent->MobilityState) : -1,
                bLimberInAction ? 1 : 0, Team.bHooked ? 1 : 0, float(FVector::Dist2D(Team.Place, LimberGoal)),
                float(Team.Speed), LimberGait, FMath::FindDeltaAngleDegrees(Team.Yaw, LimberGoalYaw));
        }
    }
}
