#include "StrategyGunCrewVisualComponent.h"

#include "StrategyArtilleryVisualComponent.h"
#include "StrategyHumanVisualTypes.h"
#include "StrategyUniformAppearanceComponent.h"
#include "StrategyVisualFormationPath.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryCrewAnimationComponent.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    // The crew clips (Tools/Animation/build_gun_crew_clips_1864.py, Content/Python/import_gun_crew_1864.py) and the soldiers' own.
    enum EGunCrewClip : int32 { GunClipStandBy, GunClipSponge, GunClipLoad, GunClipRam, GunClipPrime, GunClipAim, GunClipPush, GunClipWalk, GunClipIdle, GunClipDeath, GunClipCount };
    const TCHAR* GunCrewClipPaths[GunClipCount] = {
        TEXT("/Game/Units/Artillery/Crew/A_Gun_StandBy.A_Gun_StandBy"),
        TEXT("/Game/Units/Artillery/Crew/A_Gun_Sponge.A_Gun_Sponge"),
        TEXT("/Game/Units/Artillery/Crew/A_Gun_Load.A_Gun_Load"),
        TEXT("/Game/Units/Artillery/Crew/A_Gun_Ram.A_Gun_Ram"),
        TEXT("/Game/Units/Artillery/Crew/A_Gun_Prime.A_Gun_Prime"),
        TEXT("/Game/Units/Artillery/Crew/A_Gun_Aim.A_Gun_Aim"),
        TEXT("/Game/Units/Artillery/Crew/A_Gun_Push_Loop.A_Gun_Push_Loop"),
        // Walking and standing without a rifle (Allan 2026-11-15: "han går også rundt som om han har et gevær"): the officer's walk with
        // the sabre sheathed, on the same skeleton - the right arm swings free, the left hand rests at the hip. The soldiers' walk
        // carries a rifle that the gunners do not have.
        TEXT("/Game/Units/Human/Officer/A_Officer_WalkSheathed.A_Officer_WalkSheathed"),
        TEXT("/Game/Units/Human/Officer/A_Officer_Idle.A_Officer_Idle"),
        TEXT("/Game/Units/Human/CombatAnimations/A_Combat_DeathFront.A_Combat_DeathFront")};
    constexpr float GunCrewWalkClipCmS = 76.99816f;   // the walk clip's ground speed (the officer's walk has the soldiers' legs and root)
    constexpr float GunCrewMeshYaw = -90.0f;          // the soldier mesh faces +Y
}

UStrategyGunCrewVisualComponent::UStrategyGunCrewVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UStrategyGunCrewVisualComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
    if (OwnerBattery && OwnerBattery->ArtilleryVisualComponent) AddTickPrerequisiteComponent(OwnerBattery->ArtilleryVisualComponent);
}

bool UStrategyGunCrewVisualComponent::BuildCrew()
{
    if (!OwnerBattery || !OwnerBattery->CrewAnimationComponent || !OwnerBattery->ArtilleryVisualComponent) return false;
    const TArray<FStrategyArtilleryCrewStation>& BuildStations = OwnerBattery->CrewAnimationComponent->CrewStations;
    if (BuildStations.IsEmpty()) return false;
    // The gunners of the battery's nation (SourceAssets/Units1864/Artillery/Final_v1, rigged to the soldiers' skeleton); without
    // them the infantry stand in, in the battery's colours.
    const FString CrewNation = OwnerBattery->GetNationCodeOrDefault();
    CrewMesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Units/Artillery/Crew/SK_Artillery_1864_%s.SK_Artillery_1864_%s"), *CrewNation, *CrewNation),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    const bool bCrewGunners = CrewMesh != nullptr;
    if (!CrewMesh) CrewMesh = LoadObject<USkeletalMesh>(nullptr, CrewNation == TEXT("DK")
        ? TEXT("/Game/Units/Danish/Infantry1864/Mesh/SK_DK_Infantry_1864.SK_DK_Infantry_1864")
        : TEXT("/Game/Units/Swedish/Infantry1864/Mesh/SK_SE_Infantry_1864.SK_SE_Infantry_1864"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!CrewMesh) CrewMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864.SK_DK_Livgarden_1864"));
    if (!CrewMesh) return false;
    CrewClips.Reset();
    int32 CrewOwnClips = 0;
    for (int32 CrewClipIndex = 0; CrewClipIndex < GunClipCount; ++CrewClipIndex)
    {
        CrewClips.Add(LoadObject<UAnimSequence>(nullptr, GunCrewClipPaths[CrewClipIndex], nullptr, LOAD_NoWarn | LOAD_Quiet));
        CrewOwnClips += CrewClipIndex < GunClipWalk && CrewClips.Last() ? 1 : 0;
    }
    UStaticMesh* CrewSponge = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Units/Artillery/Crew/SM_Sponge.SM_Sponge"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    UStaticMesh* CrewRammer = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Units/Artillery/Crew/SM_Rammer.SM_Rammer"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    for (int32 CrewStation = 0; CrewStation < BuildStations.Num(); ++CrewStation)
    {
        FGunCrewFigure CrewFigure;
        CrewFigure.Station = CrewStation;
        CrewFigure.Man = NewObject<USkeletalMeshComponent>(OwnerBattery, NAME_None, RF_Transient);
        CrewFigure.Man->SetSkeletalMeshAsset(CrewMesh);
        CrewFigure.Man->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CrewFigure.Man->SetGenerateOverlapEvents(false);
        CrewFigure.Man->SetCastShadow(true);
        CrewFigure.Man->bVisibleInRayTracing = false;
        CrewFigure.Man->SetupAttachment(OwnerBattery->GetRootComponent());
        CrewFigure.Man->SetUsingAbsoluteLocation(true);
        CrewFigure.Man->SetUsingAbsoluteRotation(true);
        CrewFigure.Man->SetUsingAbsoluteScale(true);
        CrewFigure.Man->RegisterComponent();
        CrewFigure.Man->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        if (!bCrewGunners && OwnerBattery->UniformAppearanceComponent) OwnerBattery->UniformAppearanceComponent->ApplyAppearanceToMesh(CrewFigure.Man);
        // The sponger carries the sponge, the rammer the rammer (in the right hand; the grip is set from the clips' notes).
        const EStrategyArtilleryCrewRole CrewRole = BuildStations[CrewStation].Role;
        // The staff of the day was sponge at one end and rammer at the other: the sponger sponges and rams with it (2026-11-08: the
        // rammer's own staff stood idle across the wheel), so only he carries one.
        UStaticMesh* CrewPropMesh = CrewRole == EStrategyArtilleryCrewRole::Sponger ? CrewSponge : nullptr;
        if (CrewPropMesh)
        {
            CrewFigure.Prop = NewObject<UStaticMeshComponent>(OwnerBattery, NAME_None, RF_Transient);
            CrewFigure.Prop->SetStaticMesh(CrewPropMesh);
            CrewFigure.Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            CrewFigure.Prop->SetCastShadow(false);
            CrewFigure.Prop->bVisibleInRayTracing = false;
            CrewFigure.Prop->SetupAttachment(CrewFigure.Man, TEXT("RightHand"));
            // The grip measured in Blender (SourceAssets/Artillery/Crew/notes.md): the staff held 45 cm up from the hand bone.
            CrewFigure.Prop->SetRelativeTransform(FTransform(FRotator(0.f, 90.f, 180.f), FVector(3.2f, -6.0f, 45.0f)));
            CrewFigure.Prop->RegisterComponent();
        }
        Figures.Add(CrewFigure);
    }
    InitialCrewStrength = FMath::Max(1, OwnerBattery->CrewStrength + OwnerBattery->DriverStrength);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-GUNCREW %s: %d men at %d guns (%s), crew clips %d of 7, sponge %d rammer %d"),
        *OwnerBattery->StableUnitId.ToString(), Figures.Num(), OwnerBattery->ArtilleryVisualComponent->GetDrawnGunCount(), *CrewMesh->GetName(),
        CrewOwnClips, CrewSponge ? 1 : 0, CrewRammer ? 1 : 0);
    return true;
}

void UStrategyGunCrewVisualComponent::StationPost(uint8 StationRole, bool bInAction, int32 StationOrder, uint8 StationActivity, FVector& OutOffset, float& OutFacingYaw)
{
    const EStrategyArtilleryCrewActivity PostActivity = static_cast<EStrategyArtilleryCrewActivity>(StationActivity);
    // The gun commander goes to the vent while the charge is rammed home, so that he primes in time (QA 2026-11-08: the walk ate
    // the whole priming phase and the shot came with nobody at the vent).
    const bool bStationPriming = PostActivity == EStrategyArtilleryCrewActivity::Prime || PostActivity == EStrategyArtilleryCrewActivity::Ram;
    // The sponger and the loader stand clear of the muzzle and the wheels except while they work at it (QA 2026-11-08: the sponger
    // stood in the muzzle blast all the cycle).
    const bool bStationAtMuzzle = PostActivity == EStrategyArtilleryCrewActivity::Sponge || PostActivity == EStrategyArtilleryCrewActivity::Ram ||
        PostActivity == EStrategyArtilleryCrewActivity::LoadCharge || PostActivity == EStrategyArtilleryCrewActivity::LoadProjectile;
    // In action, by the piece: the posts the crew clips were fitted to at the gun's scale 1.6 (SourceAssets/Artillery/Crew_scale1.6/
    // notes.md, there in the gun mesh's frame: X = its left, Y = along the muzzle; an estimate after the drill of a light field gun).
    // The sponger also rams (No. 1 of the drill); the gun commander lays the gun from behind the trail and steps to the vent to
    // fire it; the ammunition man and the reserve stand some metres to the rear. X runs along the muzzle, Y to its right (cm).
    if (bInAction)
    {
        switch (static_cast<EStrategyArtilleryCrewRole>(StationRole))
        {
            case EStrategyArtilleryCrewRole::Gunner:
                if (bStationPriming) { OutOffset = FVector(-6.f, 42.f, 0.f); OutFacingYaw = -100.f; }
                // Behind the trail only while he lays the gun (ClearGun, the piece run forward and loaded). Else beside the trail's
                // end, clear of the recoil: the trail runs back 1.3 m to -290 along the middle (Allan 2026-11-15: "manden i midten får
                // pludselig kanonen i hovedet da den skyder, så den står midt i manden" - he stood at -205 on the middle line).
                else if (PostActivity == EStrategyArtilleryCrewActivity::ClearGun) { OutOffset = FVector(-205.f, 0.f, 0.f); OutFacingYaw = 0.f; }
                else                 { OutOffset = FVector(-215.f, -85.f, 0.f); OutFacingYaw = 20.f; }
                return;
            case EStrategyArtilleryCrewRole::Sponger:
                if (bStationAtMuzzle) { OutOffset = FVector(211.f, 25.f, 0.f); OutFacingYaw = -75.f; }
                else                  { OutOffset = FVector(75.f, 130.f, 0.f); OutFacingYaw = -90.f; }
                return;
            case EStrategyArtilleryCrewRole::Loader:
                if (bStationAtMuzzle) { OutOffset = FVector(130.f, -38.f, 0.f); OutFacingYaw = 0.f; }
                else                  { OutOffset = FVector(75.f, -130.f, 0.f); OutFacingYaw = 90.f; }
                return;
            // Clear of the gun commander's way to the vent (QA 2026-11-08: it passed 29 cm from him).
            case EStrategyArtilleryCrewRole::Rammer:       OutOffset = FVector(-110.f, 90.f, 0.f);   OutFacingYaw = -90.f; return;
            case EStrategyArtilleryCrewRole::WheelLeft:    OutOffset = FVector(-27.f, -121.f, 0.f);  OutFacingYaw = 60.f;  return;
            case EStrategyArtilleryCrewRole::WheelRight:   OutOffset = FVector(-27.f, 121.f, 0.f);   OutFacingYaw = -60.f; return;
            case EStrategyArtilleryCrewRole::Ammunition:   OutOffset = FVector(-650.f, -120.f, 0.f); OutFacingYaw = 0.f;   return;
            // By the limber's front (it stands 13 m back with its team beyond, StrategyLimberVisualComponent).
            case EStrategyArtilleryCrewRole::Driver:       OutOffset = FVector(-1050.f, -150.f, 0.f); OutFacingYaw = 0.f;  return;
            case EStrategyArtilleryCrewRole::HorseHandler: OutOffset = FVector(-1050.f, 150.f, 0.f);  OutFacingYaw = 0.f;  return;
            default:                                       OutOffset = FVector(-800.f, 150.f, 0.f);  OutFacingYaw = 0.f;   return;
        }
    }
    // On the march the muzzle points to the rear: the men walk in pairs beside the piece, facing the way it goes.
    OutOffset = FVector(150.f + (StationOrder / 2) * 140.f, (StationOrder % 2 == 0 ? 1.f : -1.f) * 150.f, 0.f);
    OutFacingYaw = 180.f;
}

UAnimSequence* UStrategyGunCrewVisualComponent::ClipForStation(int32 StationIndex, bool bInAction, bool bWalking) const
{
    const UStrategyArtilleryCrewAnimationComponent* ClipDrill = OwnerBattery ? OwnerBattery->CrewAnimationComponent.Get() : nullptr;
    int32 ClipIndex = GunClipStandBy;
    if (bWalking) ClipIndex = GunClipWalk;
    else if (ClipDrill && ClipDrill->CrewStations.IsValidIndex(StationIndex) && bInAction)
    {
        const EStrategyArtilleryCrewRole ClipRole = ClipDrill->CrewStations[StationIndex].Role;
        const bool bClipWheel = ClipRole == EStrategyArtilleryCrewRole::WheelLeft || ClipRole == EStrategyArtilleryCrewRole::WheelRight;
        // The sponger rams too (No. 1 at the muzzle, the post the Ram clip is fitted to); the rammer's station waits meanwhile.
        const EStrategyArtilleryCrewActivity ClipActivity = ClipDrill->CurrentBatteryActivity == EStrategyArtilleryCrewActivity::Ram
            ? (ClipRole == EStrategyArtilleryCrewRole::Sponger ? EStrategyArtilleryCrewActivity::Ram : EStrategyArtilleryCrewActivity::Idle)
            : ClipDrill->GetActivityForStation(StationIndex);
        switch (ClipActivity)
        {
            case EStrategyArtilleryCrewActivity::Sponge:         ClipIndex = GunClipSponge; break;
            case EStrategyArtilleryCrewActivity::LoadCharge:
            case EStrategyArtilleryCrewActivity::LoadProjectile: ClipIndex = ClipRole == EStrategyArtilleryCrewRole::Loader ? GunClipLoad : GunClipStandBy; break;
            case EStrategyArtilleryCrewActivity::Ram:            ClipIndex = GunClipRam; break;
            case EStrategyArtilleryCrewActivity::Prime:          ClipIndex = GunClipPrime; break;
            case EStrategyArtilleryCrewActivity::ClearGun:       ClipIndex = GunClipAim; break;
            case EStrategyArtilleryCrewActivity::RecoilReact:   // the wheel men run the piece forward after the shot
            case EStrategyArtilleryCrewActivity::PushGun:
            case EStrategyArtilleryCrewActivity::TraverseLeft:
            case EStrategyArtilleryCrewActivity::TraverseRight:
            case EStrategyArtilleryCrewActivity::Limber:
            case EStrategyArtilleryCrewActivity::Unlimber:       ClipIndex = bClipWheel ? GunClipPush : ClipRole == EStrategyArtilleryCrewRole::Gunner ? GunClipAim : GunClipStandBy; break;
            default:                                             ClipIndex = ClipRole == EStrategyArtilleryCrewRole::Gunner ? GunClipAim : GunClipStandBy; break;
        }
    }
    if (CrewClips.IsValidIndex(ClipIndex) && CrewClips[ClipIndex]) return CrewClips[ClipIndex].Get();
    return CrewClips.IsValidIndex(GunClipIdle) ? CrewClips[GunClipIdle].Get() : nullptr;
}

void UStrategyGunCrewVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!OwnerBattery || DeltaTime <= 0.f || !GetWorld()) return;
    if (Figures.IsEmpty())
    {
        if (bBuildTried) return;
        bBuildTried = true;
        if (!BuildCrew()) return;
    }
    const UStrategyArtilleryCrewAnimationComponent* TickDrill = OwnerBattery->CrewAnimationComponent;
    const UStrategyArtilleryVisualComponent* TickGuns = OwnerBattery->ArtilleryVisualComponent;
    if (!TickDrill || !TickGuns) return;
    const bool bTickHidden = OwnerBattery->bOutOfPlay || OwnerBattery->IsHidden();
    // As the battery loses men they fall: the reserve and the men at the rear first, the gun commanders last.
    const int32 TickAlive = FMath::Clamp(FMath::CeilToInt(float(Figures.Num()) * float(OwnerBattery->CrewStrength + OwnerBattery->DriverStrength) / float(InitialCrewStrength)), 0, Figures.Num());
    int32 TickStanding = 0;
    for (const FGunCrewFigure& TickCount : Figures) TickStanding += TickCount.bFallen ? 0 : 1;
    static const EStrategyArtilleryCrewRole TickFallOrder[] = {
        EStrategyArtilleryCrewRole::Reserve, EStrategyArtilleryCrewRole::HorseHandler, EStrategyArtilleryCrewRole::Driver,
        EStrategyArtilleryCrewRole::Ammunition, EStrategyArtilleryCrewRole::WheelRight, EStrategyArtilleryCrewRole::WheelLeft,
        EStrategyArtilleryCrewRole::Rammer, EStrategyArtilleryCrewRole::Loader, EStrategyArtilleryCrewRole::Sponger, EStrategyArtilleryCrewRole::Gunner};
    for (int32 TickRoleIndex = 0; TickRoleIndex < int32(UE_ARRAY_COUNT(TickFallOrder)) && TickStanding > TickAlive; ++TickRoleIndex)
        for (int32 TickFigureIndex = Figures.Num() - 1; TickFigureIndex >= 0 && TickStanding > TickAlive; --TickFigureIndex)
        {
            FGunCrewFigure& TickFalling = Figures[TickFigureIndex];
            if (TickFalling.bFallen || !TickDrill->CrewStations.IsValidIndex(TickFalling.Station) ||
                TickDrill->CrewStations[TickFalling.Station].Role != TickFallOrder[TickRoleIndex]) continue;
            TickFalling.bFallen = true;
            --TickStanding;
            if (UAnimSequence* TickDeath = CrewClips.IsValidIndex(GunClipDeath) ? CrewClips[GunClipDeath].Get() : nullptr)
            {
                TickFalling.Man->PlayAnimation(TickDeath, false);
                TickFalling.Man->SetPlayRate(FMath::FRandRange(0.9f, 1.1f));
            }
            if (TickFalling.Prop) TickFalling.Prop->SetVisibility(false);
        }
    static const bool bCrewProbe = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864GroundProbe"));
    const bool bProbeNow = bCrewProbe && GetWorld()->GetTimeSeconds() - CrewProbeLast > 3.0;
    if (bProbeNow) CrewProbeLast = GetWorld()->GetTimeSeconds();
    TArray<int32> TickOrderAtGun;
    TickOrderAtGun.SetNumZeroed(FMath::Max(1, TickGuns->GetDrawnGunCount()));
    for (FGunCrewFigure& Figure : Figures)
    {
        Figure.Man->SetVisibility(!bTickHidden);
        if (Figure.Prop) Figure.Prop->SetVisibility(!bTickHidden && !Figure.bFallen);
        if (Figure.bFallen || bTickHidden || !TickDrill->CrewStations.IsValidIndex(Figure.Station)) continue;
        const FStrategyArtilleryCrewStation& Station = TickDrill->CrewStations[Figure.Station];
        FVector GunAt, GunMuzzle;
        bool bInAction = false;
        // The posts stand by the piece in battery, not by the recoiling gun (QA 2026-11-08: the whole crew, the ammunition man and
        // the driver too, walked 1.3 m back and forth with every shot); only the men at the wheels go with it, pushing it back.
        const bool bTickWheel = Station.Role == EStrategyArtilleryCrewRole::WheelLeft || Station.Role == EStrategyArtilleryCrewRole::WheelRight;
        // The piece is run forward again right after the shot (RecoilReact), before it is sponged and loaded: see the recoil in
        // StrategyArtilleryVisualComponent. (It stood back all through the loading, 1.3 m from the posts the crew worked at.)
        const bool bTickReturning = TickDrill->CurrentBatteryActivity == EStrategyArtilleryCrewActivity::RecoilReact;
        if (!TickGuns->GetGunPlacement(Station.GunIndex, GunAt, GunMuzzle, bInAction, !(bTickWheel && bTickReturning))) continue;
        const int32 StationOrder = TickOrderAtGun.IsValidIndex(Station.GunIndex) ? TickOrderAtGun[Station.GunIndex]++ : 0;
        FVector PostOffset;
        float PostYaw = 0.f;
        StationPost(uint8(Station.Role), bInAction, StationOrder, uint8(TickDrill->CurrentBatteryActivity), PostOffset, PostYaw);
        const float GunYaw = GunMuzzle.Rotation().Yaw;
        FVector PostGoal = GunAt + FRotator(0.f, GunYaw, 0.f).RotateVector(PostOffset);
        PostGoal.Z = UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, PostGoal);
        // He walks to his post from where he stood (never jumps with the unit's actor), and runs to catch up when left behind.
        if (!Figure.bPlaced) { Figure.WorldPlace = PostGoal; Figure.Yaw = GunYaw + PostYaw; Figure.bPlaced = true; }
        const FVector PostTo = PostGoal - Figure.WorldPlace;
        const float PostDistance = PostTo.Size2D();
        const float PostSpeed = FMath::Min(PostDistance * 3.f, PostDistance > 300.f ? FMath::Max(450.f, PostDistance * 1.5f) : 160.f);
        const float PostStep = FMath::Min(PostDistance, PostSpeed * DeltaTime);
        if (PostDistance > 1.f) Figure.WorldPlace += PostTo.GetSafeNormal2D() * PostStep;
        Figure.WorldPlace.Z = UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, Figure.WorldPlace);
        // A short step is taken standing (no walk for a few centimetres); the wheel men push the gun forward, not walk.
        const bool bPushing = bTickWheel && bTickReturning && bInAction;
        const bool bWalking = !bPushing && PostStep / DeltaTime > 25.f && PostDistance > 40.f;
        Figure.Yaw = FStrategyVisualFormationPath::SmoothFacing(Figure.Yaw, Figure.YawVelocity, bWalking ? PostTo.Rotation().Yaw : GunYaw + PostYaw, DeltaTime, 200.f);
        Figure.Man->SetWorldLocationAndRotation(Figure.WorldPlace, FRotator(0.f, Figure.Yaw + GunCrewMeshYaw, 0.f));
        UAnimSequence* FigureClip = ClipForStation(Figure.Station, bInAction, bWalking);
        if (FigureClip && FigureClip != Figure.ClipNow)
        {
            // The drill's actions (sponge, load, ram, prime) are done once a phase; standing by, laying, pushing and walking loop.
            const bool bFigureLoops = FigureClip != CrewClips[GunClipSponge] && FigureClip != CrewClips[GunClipLoad] &&
                FigureClip != CrewClips[GunClipRam] && FigureClip != CrewClips[GunClipPrime];
            Figure.Man->PlayAnimation(FigureClip, bFigureLoops);
            Figure.ClipNow = FigureClip;
        }
        // The drill's actions fill their phase of the reload (QA 2026-11-08: the sponge clip is 3.0 s for a 1.9 s phase, prime 2.0 s
        // for 1.2 s - the actions were cut off and the crew seemed to stand idle); the walk follows the ground speed.
        float FigureRate = 1.f;
        if (bWalking && FigureClip == CrewClips[GunClipWalk]) FigureRate = FMath::Clamp((PostStep / DeltaTime) / GunCrewWalkClipCmS, 0.3f, 2.5f);
        else if (FigureClip)
        {
            const EStrategyArtilleryCrewActivity RateActivity = TickDrill->CurrentBatteryActivity;
            const float RatePhase = RateActivity == EStrategyArtilleryCrewActivity::Sponge ? 0.16f
                : (RateActivity == EStrategyArtilleryCrewActivity::LoadCharge || RateActivity == EStrategyArtilleryCrewActivity::LoadProjectile) ? 0.26f
                : RateActivity == EStrategyArtilleryCrewActivity::Ram ? 0.18f : RateActivity == EStrategyArtilleryCrewActivity::Prime ? 0.10f : 0.f;
            const bool bRateAction = FigureClip == CrewClips[GunClipSponge] || FigureClip == CrewClips[GunClipLoad] || FigureClip == CrewClips[GunClipRam] || FigureClip == CrewClips[GunClipPrime];
            if (bRateAction && RatePhase > 0.f && TickDrill->DrillDurationSeconds > 0.f)
                FigureRate = FMath::Clamp(FigureClip->GetPlayLength() / (RatePhase * TickDrill->DrillDurationSeconds), 0.6f, 2.2f);
        }
        Figure.Man->SetPlayRate(FigureRate);
        // The staff is in his hands only while he sponges and rams; else it rests (not carried across the wheel, QA 2026-11-08).
        if (Figure.Prop) Figure.Prop->SetVisibility(!bTickHidden && (FigureClip == CrewClips[GunClipSponge] || FigureClip == CrewClips[GunClipRam]));
        if (bProbeNow && Station.GunIndex == 0)
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-GUNCREWPROBE %s gun 0 role %d activity %d clip %s in-action %d walking %d post %.0f cm off"),
                *OwnerBattery->StableUnitId.ToString(), int32(Station.Role), int32(TickDrill->GetActivityForStation(Figure.Station)),
                FigureClip ? *FigureClip->GetName() : TEXT("-"), bInAction ? 1 : 0, bWalking ? 1 : 0, PostDistance);
    }
}
