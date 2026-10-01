#pragma once

#include "CoreMinimal.h"
#include "StrategyHumanVisualTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategyHumanAnimationAction : uint8
{
    None,
    Idle,
    Walk,
    Run,
    Aim,
    Fire,
    Reload,
    Die,
    Mount,
    Dismount,
    Kneel,
    Prone,
    BayonetReady,
    BayonetCharge,
    RoutedRun,
    MountedIdle,
    MountedWalk,
    MountedTrot,
    MountedCanter,
    MountedGallop,
    ArtilleryWork
};

UENUM(BlueprintType)
enum class EStrategyHorseGait : uint8
{
    Idle,
    Walk,
    Trot,
    Canter,
    Gallop
};

UENUM(BlueprintType)
enum class EStrategyEquipmentSlot : uint8
{
    PrimaryWeapon,
    Bayonet,
    Sabre,
    Sidearm,
    Tool,
    Backpack,
    CartridgeBox,
    Headgear
};

UENUM(BlueprintType)
enum class EStrategyArtilleryCrewRole : uint8
{
    Gunner,
    Loader,
    Rammer,
    Sponger,
    Ammunition,
    WheelLeft,
    WheelRight,
    Driver,
    HorseHandler,
    Reserve
};

UENUM(BlueprintType)
enum class EStrategyArtilleryCrewActivity : uint8
{
    Idle,
    PushGun,
    PullGun,
    TraverseLeft,
    TraverseRight,
    Sponge,
    LoadCharge,
    LoadProjectile,
    Ram,
    Prime,
    ClearGun,
    Fire,
    RecoilReact,
    ReturnToBattery,
    Limber,
    Unlimber,
    Repair,
    CarryAmmunition
};

USTRUCT(BlueprintType)
struct FStrategyUniformColors
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Coat = FLinearColor(0.20f, 0.20f, 0.20f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Trousers = FLinearColor(0.18f, 0.18f, 0.18f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Facings = FLinearColor(0.35f, 0.05f, 0.05f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor HeadgearDetail = FLinearColor(0.08f, 0.08f, 0.08f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Leather = FLinearColor(0.08f, 0.04f, 0.02f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Accent = FLinearColor(0.60f, 0.60f, 0.60f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Metal = FLinearColor(0.55f, 0.50f, 0.35f, 1.0f);
};

USTRUCT(BlueprintType)
struct FStrategyUniformPreset
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName PresetId = TEXT("UNIFORM_DEFAULT");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName NationId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName UnitTypeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RegimentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FStrategyUniformColors Colors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bLockHistoricalPalette = false;
};

USTRUCT(BlueprintType)
struct FStrategyUniformOverrides
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOverrideCoat = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Coat = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOverrideTrousers = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Trousers = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOverrideFacings = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Facings = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOverrideHeadgearDetail = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor HeadgearDetail = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOverrideLeather = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Leather = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOverrideAccent = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Accent = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOverrideMetal = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Metal = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct FStrategyHumanVisualProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ProfileId = TEXT("HUMAN_DEFAULT");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SharedSkeletonId = TEXT("SK_Human_1864");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SharedAnimationSetId = TEXT("ABP_Human_1864");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName BodyMeshId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName UniformPresetId = TEXT("UNIFORM_DEFAULT");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EquipmentPresetId = NAME_None;
};

USTRUCT(BlueprintType)
struct FStrategyEquipmentSocketMap
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName PrimaryWeaponSocket = TEXT("hand_r_socket");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName BayonetSocket = TEXT("bayonet_socket");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SabreSocket = TEXT("sabre_socket");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ToolSocket = TEXT("tool_socket");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName BackpackSocket = TEXT("back_socket");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CartridgeBoxSocket = TEXT("belt_socket");
};

USTRUCT(BlueprintType)
struct FStrategyArtilleryCrewStation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GunIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StationIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EStrategyArtilleryCrewRole Role = EStrategyArtilleryCrewRole::Reserve;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EStrategyArtilleryCrewActivity Activity =
        EStrategyArtilleryCrewActivity::Idle;
};
