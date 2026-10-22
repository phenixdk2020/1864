#include "StrategyColourFlag.h"
#include "StrategyInfantryVisualComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"

AStrategyColourFlag::AStrategyColourFlag()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Flag"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->bVisibleInRayTracing = false;
}

void AStrategyColourFlag::Setup(AActor* InFollow, const FString& Nation, const FVector& Offset)
{
    Follow = InFollow;
    LocalOffset = Offset;
    if (InFollow)
    {
        AddTickPrerequisiteActor(InFollow);
        if (UStrategyInfantryVisualComponent* ColourVisual = InFollow->FindComponentByClass<UStrategyInfantryVisualComponent>())
        {
            AddTickPrerequisiteComponent(ColourVisual);
            ColourVisual->RegisterColours();   // the company gets a colour bearer who carries this flag
        }
    }
    // The model: 2 m as imported, 2.8 m on the field; the cloth streams to +X from the pole, turned to -X here.
    if (UStaticMesh* Standard = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Units/Items/SM_Flag_Standard.SM_Flag_Standard")))
    {
        Model = NewObject<UStaticMeshComponent>(this, TEXT("Model"));
        Model->SetStaticMesh(Standard);
        Model->SetupAttachment(Mesh);
        Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Model->bVisibleInRayTracing = false;
        Model->SetCastShadow(true);
        Model->SetRelativeScale3D(FVector(1.4f));
        // The pole stands 23.3 cm from the model's origin (Flag_Standard_Split.fbx, measured in Blender 2026-10-22: pole centre
        // x = -0.233 m, y = 0.014 m): shifted so its axis runs through the actor's origin, which the bearer's hand defines
        // (it stood 33 cm beside his hand).
        Model->SetRelativeLocationAndRotation(FVector(-23.3f * 1.4f, -1.4f * 1.4f, 140.0f), FRotator(0.0f, 180.0f, 0.0f));
        Model->RegisterComponent();
        UTexture* Flag = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Units/Flags/T_Flag_%s.T_Flag_%s"), *Nation, *Nation));
        if (Flag && SetFlagTexture(Flag))
        {
            return;
        }
        Model->DestroyComponent();
        Model = nullptr;
    }
    TArray<FVector> V;
    TArray<int32> T;
    TArray<FColor> C;
    // A quad (both faces) in the XZ plane: the flag hangs to -X from the pole.
    auto Quad = [&](const FVector& A, const FVector& B, const FVector& Cc, const FVector& D, const FColor& Colour)
    {
        const int32 I = V.Num();
        V.Append({ A, B, Cc, D });
        C.Append({ Colour, Colour, Colour, Colour });
        T.Append({ I, I + 1, I + 2, I, I + 2, I + 3, I, I + 2, I + 1, I, I + 3, I + 2 });
    };
    auto Box = [&](const FVector& Min, const FVector& Max, const FColor& Colour)
    {
        Quad(FVector(Min.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Max.Z), FVector(Min.X, Min.Y, Max.Z), Colour);
        Quad(FVector(Min.X, Max.Y, Min.Z), FVector(Max.X, Max.Y, Min.Z), FVector(Max.X, Max.Y, Max.Z), FVector(Min.X, Max.Y, Max.Z), Colour);
        Quad(FVector(Min.X, Min.Y, Min.Z), FVector(Min.X, Max.Y, Min.Z), FVector(Min.X, Max.Y, Max.Z), FVector(Min.X, Min.Y, Max.Z), Colour);
        Quad(FVector(Max.X, Min.Y, Min.Z), FVector(Max.X, Max.Y, Min.Z), FVector(Max.X, Max.Y, Max.Z), FVector(Max.X, Min.Y, Max.Z), Colour);
    };
    const FColor PoleColour(70, 52, 34), FinialColour(214, 180, 70);
    Box(FVector(-3.0f, -3.0f, 0.0f), FVector(3.0f, 3.0f, 300.0f), PoleColour);
    Box(FVector(-5.0f, -5.0f, 300.0f), FVector(5.0f, 5.0f, 312.0f), FinialColour);
    // The cloth: 120 x 90 cm from 200 to 290 cm, the cross off-centre towards the pole (a Nordic cross).
    const bool bDanish = Nation == TEXT("DK");
    const bool bSwedish = Nation == TEXT("SE");
    const FColor Field = bDanish ? FColor(198, 12, 48) : bSwedish ? FColor(0, 82, 147) : Nation == TEXT("PR") ? FColor(240, 240, 240) : FColor(240, 240, 240);
    const FColor Cross = bDanish ? FColor(250, 250, 250) : bSwedish ? FColor(254, 204, 0) : FColor(20, 20, 20);
    const float L = -120.0f, Bottom = 200.0f, Top = 290.0f, Y = 0.0f;
    Quad(FVector(0.0f, Y, Bottom), FVector(L, Y, Bottom), FVector(L, Y, Top), FVector(0.0f, Y, Top), Field);
    // The cross: a vertical bar a third in from the pole, a horizontal bar through the middle (a hair in front).
    const float V0 = -32.0f, V1 = -46.0f, H0 = 238.0f, H1 = 252.0f, F = 0.6f;
    Quad(FVector(V0, Y - F, Bottom), FVector(V1, Y - F, Bottom), FVector(V1, Y - F, Top), FVector(V0, Y - F, Top), Cross);
    Quad(FVector(0.0f, Y - F, H0), FVector(L, Y - F, H0), FVector(L, Y - F, H1), FVector(0.0f, Y - F, H1), Cross);
    Quad(FVector(V0, Y + F, Bottom), FVector(V1, Y + F, Bottom), FVector(V1, Y + F, Top), FVector(V0, Y + F, Top), Cross);
    Quad(FVector(0.0f, Y + F, H0), FVector(L, Y + F, H0), FVector(L, Y + F, H1), FVector(0.0f, Y + F, H1), Cross);
    Mesh->CreateMeshSection(0, V, T, TArray<FVector>(), TArray<FVector2D>(), C, TArray<FProcMeshTangent>(), false);
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery")))
    {
        Mesh->SetMaterial(0, Material);
    }
}

bool AStrategyColourFlag::SetFlagTexture(UTexture* Flag)
{
    const int32 Slot = Model ? Model->GetMaterialIndex(TEXT("Cloth")) : INDEX_NONE;
    if (!Flag || Slot == INDEX_NONE)
    {
        return false;
    }
    UMaterialInstanceDynamic* Cloth = Model->CreateDynamicMaterialInstance(Slot);
    if (!Cloth)
    {
        return false;
    }
    Cloth->SetTextureParameterValue(TEXT("Flag"), Flag);
    return true;
}

void AStrategyColourFlag::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AActor* Target = Follow.Get();
    if (!Target)
    {
        Destroy();
        return;
    }
    Age += DeltaSeconds;
    FVector P = Target->GetActorTransform().TransformPosition(LocalOffset);
    float ColourBearerYaw = 0.f;
    bool bColoursDown = false;
    bool bCarried = false;
    if (const UStrategyInfantryVisualComponent* ColourVisual = Target->FindComponentByClass<UStrategyInfantryVisualComponent>())
    {
        bCarried = ColourVisual->GetColourTransform(P, ColourBearerYaw, bColoursDown);
        if (!bCarried) ColourVisual->GetDrawnColourPosition(LocalOffset, P);
    }
    const FVector Ground = UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, P);
    if (bCarried && bColoursDown)
    {
        // Fallen with the bearer: the colours lie on the ground beside him.
        SetActorLocationAndRotation(Ground + FVector(0.f, 0.f, 6.f), FRotator(0.0f, ColourBearerYaw + 70.f, 84.f));
        if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864ColourDiag")) && Model)
        {
            static double ColourDownDiagNext = 0.0;
            if (FPlatformTime::Seconds() >= ColourDownDiagNext)
            {
                ColourDownDiagNext = FPlatformTime::Seconds() + 1.0;
                UE_LOG(LogTemp, Display, TEXT("Strategy1864ColourDiag down actor=%s model-bounds=%s..%s visible=%d"), *GetActorLocation().ToString(),
                    *Model->Bounds.GetBox().Min.ToString(), *Model->Bounds.GetBox().Max.ToString(), Model->IsVisible() ? 1 : 0);
            }
        }
        return;
    }
    if (bCarried)
    {
        // Carried: the foot of the pole at knee height, the pole leaning back, the cloth streaming behind the bearer.
        SetActorLocationAndRotation(P,
            FRotator(UStrategyInfantryVisualComponent::ColourCarryLeanDeg, ColourBearerYaw + 6.0f * FMath::Sin(Age * 1.3f), 0.0f));
        return;
    }
    // The flag streams with the wind (from the west), swinging a little.
    SetActorLocationAndRotation(Ground, FRotator(0.0f, 90.0f + 12.0f * FMath::Sin(Age * 1.3f), 0.0f));
}
