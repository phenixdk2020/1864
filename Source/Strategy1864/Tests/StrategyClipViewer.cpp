#include "StrategyClipViewer.h"
#include "../Visual/StrategyCrowdModel.h"
#include "../Player/StrategyCameraPawn.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AStrategyClipViewer::AStrategyClipViewer()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void AStrategyClipViewer::BeginPlay()
{
    Super::BeginPlay();
    Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864.SK_DK_Livgarden_1864"));
    if (!Mesh) { UE_LOG(LogTemp, Warning, TEXT("CLIPVIEWER: soldier mesh missing")); return; }
    FString Filter;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864ClipViewer="), Filter);   // optional name filter, e.g. =Shoulder
    FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    Registry.Get().ScanPathsSynchronous({TEXT("/Game/Units")}, true);
    TArray<FAssetData> Assets;
    Registry.Get().GetAssetsByPath(FName("/Game/Units"), Assets, true);
    Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
    // Wide spacing so neighbouring clips (sabre swings, deaths, reloads) do not overlap; -Strategy1864ClipViewerSpacing=<cm> overrides.
    const int32 Columns = ClipViewerColumns;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864ClipViewerSpacing="), ClipViewerSpacing);
    int32 Index = 0;
    for (const FAssetData& Data : Assets)
    {
        if (Data.AssetClassPath != UAnimSequence::StaticClass()->GetClassPathName()) { continue; }
        if (!Filter.IsEmpty() && !Data.AssetName.ToString().Contains(Filter)) { continue; }
        UAnimSequence* Clip = Cast<UAnimSequence>(Data.GetAsset());
        if (!Clip || Clip->GetSkeleton() != Mesh->GetSkeleton()) { continue; }
        USkeletalMeshComponent* Figure = NewObject<USkeletalMeshComponent>(this);
        Figure->SetupAttachment(GetRootComponent());
        Figure->RegisterComponent();
        Figure->SetSkeletalMeshAsset(Mesh);
        Figure->SetRelativeLocation(FVector((Index / Columns) * ClipViewerSpacing * 1.2f, (Index % Columns) * ClipViewerSpacing, 0.f));
        Figure->SetRelativeRotation(FRotator(0.f, 150.f, 0.f));   // turned toward the camera, a little to the side
        UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
        Label->SetupAttachment(GetRootComponent());
        Label->RegisterComponent();
        Label->SetText(FText::FromString(FString::Printf(TEXT("%s\n%.1fs"), *Data.AssetName.ToString().Replace(TEXT("A_Combat_"), TEXT("")).Replace(TEXT("A_"), TEXT("")), Clip->GetPlayLength())));
        Label->SetWorldSize(24.f);
        Label->SetTextRenderColor(FColor::White);
        Label->SetHorizontalAlignment(EHTA_Center);
        Label->SetRelativeLocation(FVector((Index / Columns) * ClipViewerSpacing * 1.2f, (Index % Columns) * ClipViewerSpacing, (Index % 2) ? 265.f : 215.f));
        Label->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
        Figures.Add({Figure, Clip, Data.AssetName.ToString()});
        ++Index;
    }
    UE_LOG(LogTemp, Display, TEXT("CLIPVIEWER: %d clips"), Figures.Num());
}

void AStrategyClipViewer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UWorld* World = GetWorld();
    if (!World) { return; }
    if (!bPlaced)
    {
        // The grid hangs in front of the camera (whatever the map and camera clamps), once.
        if (APlayerController* PC = World->GetFirstPlayerController())
            if (PC->PlayerCameraManager && PC->GetPawn())
            {
                static double PresetAt = -1.0;
                if (PresetAt < 0.0)
                {
                    PresetAt = FPlatformTime::Seconds();
                    if (AStrategyCameraPawn* CameraPawn = Cast<AStrategyCameraPawn>(PC->GetPawn())) { CameraPawn->ApplyPreset(3600.f, -30.f); }
                }
                if (FPlatformTime::Seconds() - PresetAt < 1.5) { return; }   // let the camera settle, then hang the grid in front of it
                bPlaced = true;
                FVector CameraLocation; FRotator CameraRotation;
                PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
                const FRotator Facing(0.f, CameraRotation.Yaw, 0.f);
                const int32 Rows = FMath::Max(1, FMath::DivideAndRoundUp(Figures.Num(), ClipViewerColumns));
                const FVector Look = CameraRotation.Vector();
                const float ToGround = Look.Z < -0.05f ? FMath::Clamp(CameraLocation.Z / -Look.Z, 1500.f, 9000.f) : 3300.f;
                const FVector Centre = FVector(CameraLocation.X + Look.X * ToGround, CameraLocation.Y + Look.Y * ToGround, 0.f);   // the ground point the camera looks at (flat QA map at 0)
                SetActorRotation(Facing);
                SetActorLocation(Centre - Facing.RotateVector(FVector((Rows - 1) * ClipViewerSpacing * 0.6f, (ClipViewerColumns - 1) * ClipViewerSpacing * 0.5f, 0.f)));
                UE_LOG(LogTemp, Display, TEXT("CLIPVIEWER: grid at %s (%d rows), camera %s"), *GetActorLocation().ToString(), Rows, *CameraLocation.ToString());
            }
    }
    const double Now = FPlatformTime::Seconds();   // real time, so the clips run under a pause too
    // -Strategy1864ClipViewerShot: one screenshot 12 s after the camera is placed, then quit (QA of the viewer itself).
    if (bPlaced && FParse::Param(FCommandLine::Get(), TEXT("Strategy1864ClipViewerShot")))
    {
        static double ShotAt = -1.0;
        if (ShotAt < 0.0) { ShotAt = Now + 12.0; }
        static bool bShotTaken = false;
        if (!bShotTaken && Now > ShotAt)
        {
            bShotTaken = true;
            if (APlayerController* ShotPC = World->GetFirstPlayerController()) { ShotPC->ConsoleCommand(TEXT("HighResShot 1920x1080")); }
        }
        if (bShotTaken && Now > ShotAt + 4.0) { FPlatformMisc::RequestExit(false); }
    }
    for (const FViewerFigure& Entry : Figures)
    {
        if (!Entry.Figure || !Entry.Clip) { continue; }
        const float Length = FMath::Max(0.05f, Entry.Clip->GetPlayLength());
        UStrategyCrowdSubsystem::DrawAuxiliary(Entry.Figure, Mesh, Entry.Clip, float(FMath::Fmod(Now, double(Length))), true);

    }
}
