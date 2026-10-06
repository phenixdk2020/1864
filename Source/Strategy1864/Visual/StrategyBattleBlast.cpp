#include "StrategyBattleBlast.h"

#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
    // Powder smoke is a warm white; earth a dark brown; the dust of a dry field a grey brown.
    const FLinearColor PowderSmoke(1.35f, 1.33f, 1.28f);   // above 1: the material is lit, and the smoke must stay white in the shade
    const FLinearColor Earth(0.16f, 0.11f, 0.07f);
    const FLinearColor Dust(0.42f, 0.36f, 0.28f);
    const FLinearColor BlackSmoke(0.10f, 0.09f, 0.085f);
    const FLinearColor Fire(6.0f, 2.6f, 0.6f);

    UStaticMesh* SphereMesh()
    {
        static TWeakObjectPtr<UStaticMesh> Cached;
        if (!Cached.IsValid())
        {
            Cached = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        }
        return Cached.Get();
    }

    UStaticMesh* DiscMesh()
    {
        static TWeakObjectPtr<UStaticMesh> Cached;
        if (!Cached.IsValid())
        {
            Cached = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        }
        return Cached.Get();
    }

    UMaterialInterface* TranslucentMaterial()
    {
        static TWeakObjectPtr<UMaterialInterface> Cached;
        if (!Cached.IsValid())
        {
            Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent"));
        }
        return Cached.Get();
    }

    FVector RandomCone(const FVector& Axis, float HalfAngleDeg)
    {
        return FMath::VRandCone(Axis.GetSafeNormal(), FMath::DegreesToRadians(HalfAngleDeg));
    }
}

AStrategyBattleBlast::AStrategyBattleBlast()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    SetActorEnableCollision(false);
}

AStrategyBattleBlast* AStrategyBattleBlast::Spawn(UWorld* World, EStrategyBlastKind Kind, const FVector& Location, const FVector& Direction, float Scale, float RangeCm)
{
    if (!World || World->IsNetMode(NM_DedicatedServer))
    {
        return nullptr;
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AStrategyBattleBlast* Blast = World->SpawnActor<AStrategyBattleBlast>(AStrategyBattleBlast::StaticClass(), Location, FRotator::ZeroRotator, Params);
    if (Blast)
    {
        Blast->Build(Kind, Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal(), FMath::Max(0.1f, Scale), RangeCm);
    }
    return Blast;
}

float AStrategyBattleBlast::GroundZAt(const FVector& At) const
{
    return UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(const_cast<AStrategyBattleBlast*>(this), At);
}

AStrategyBattleBlast::FPiece AStrategyBattleBlast::Smoke(const FVector& Offset, const FVector& Velocity, float D0, float D1, float Life, float Opacity, const FLinearColor& Colour) const
{
    FPiece P;
    P.Position = Offset;
    P.Velocity = Velocity;
    P.StartDiameter = D0;
    P.EndDiameter = D1;
    P.Life = Life * FMath::FRandRange(0.85f, 1.2f);
    P.Opacity = Opacity;
    P.Colour = Colour;
    P.Drag = 1.6f;
    return P;
}

void AStrategyBattleBlast::AddEarth(int32 Count, float UpMin, float UpMax, float Side, float Scale)
{
    for (int32 i = 0; i < Count; ++i)
    {
        FPiece P;
        const FVector Out = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
        P.Velocity = Out * FMath::FRandRange(0.3f, 1.f) * Side + FVector(0.f, 0.f, FMath::FRandRange(UpMin, UpMax));
        P.Position = Out * 30.f * Scale;
        P.StartDiameter = FMath::FRandRange(18.f, 45.f) * Scale;
        P.EndDiameter = P.StartDiameter * 0.8f;
        P.Life = FMath::FRandRange(1.1f, 1.8f);
        P.Opacity = 0.95f;
        P.Colour = Earth * FMath::FRandRange(0.8f, 1.3f);
        P.bGravity = true;
        AddPiece(P);
    }
}

void AStrategyBattleBlast::AddFlash(float Intensity, float RadiusCm, float Seconds)
{
    Flash = NewObject<UPointLightComponent>(this);
    Flash->SetupAttachment(Root);
    Flash->SetIntensityUnits(ELightUnits::Candelas);
    Flash->SetIntensity(Intensity);
    Flash->SetAttenuationRadius(RadiusCm);
    Flash->SetLightColor(FLinearColor(1.0f, 0.62f, 0.28f));
    Flash->SetCastShadows(false);
    Flash->RegisterComponent();
    FlashIntensity = Intensity;
    FlashSeconds = Seconds;
}

void AStrategyBattleBlast::AddPiece(const FPiece& In)
{
    UStaticMesh* Mesh = In.bFlat ? DiscMesh() : SphereMesh();
    if (!Mesh)
    {
        return;
    }
    FPiece P = In;
    UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
    C->SetupAttachment(Root);
    C->SetStaticMesh(Mesh);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCastShadow(false);
    C->bVisibleInRayTracing = false;
    C->SetVisibility(P.Delay <= 0.0f);
    C->RegisterComponent();
    if (UMaterialInterface* Base = TranslucentMaterial())
    {
        P.Material = UMaterialInstanceDynamic::Create(Base, this);
        C->SetMaterial(0, P.Material);
    }
    P.Mesh = C;
    Meshes.Add(C);
    Pieces.Add(P);
}

void AStrategyBattleBlast::Build(EStrategyBlastKind Kind, const FVector& Dir, float Scale, float RangeCm)
{
    const FVector Flat = FVector(Dir.X, Dir.Y, 0.f).GetSafeNormal();
    const FVector Right(-Flat.Y, Flat.X, 0.f);
    switch (Kind)
    {
    case EStrategyBlastKind::CannonMuzzle:
    case EStrategyBlastKind::MortarMuzzle:
    {
        const bool bMortar = Kind == EStrategyBlastKind::MortarMuzzle;
        const FVector Out = bMortar ? (Flat * 0.35f + FVector::UpVector).GetSafeNormal() : Flat;
        AddFlash(bMortar ? 30000.f : 45000.f, 3500.f * Scale, 0.12f);
        FPiece Core = Smoke(Out * 60.f * Scale, Out * 300.f, 40.f * Scale, 260.f * Scale, 0.14f, 1.0f, Fire);
        Core.bGlow = true;
        Core.Drag = 6.f;
        AddPiece(Core);
        // The bank of smoke: thrown forward fast, then hanging and spreading; it hides the gun for a while.
        for (int32 i = 0; i < 7; ++i)
        {
            const float Speed = FMath::FRandRange(500.f, 1500.f) * Scale;
            AddPiece(Smoke(Out * FMath::FRandRange(40.f, 160.f) * Scale, (RandomCone(Out, 18.f) * Speed) + FVector(0.f, 0.f, 40.f),
                FMath::FRandRange(90.f, 160.f) * Scale, FMath::FRandRange(700.f, 1100.f) * Scale, FMath::FRandRange(7.f, 11.f), 0.78f, PowderSmoke));
        }
        // The blast blows the dust up off the ground in front of (or around) the piece.
        for (int32 i = 0; i < 4; ++i)
        {
            const FVector Side = (bMortar ? FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal() : (Flat + Right * FMath::FRandRange(-0.8f, 0.8f)).GetSafeNormal());
            FPiece D = Smoke(Side * 150.f * Scale + FVector(0.f, 0.f, -60.f), Side * FMath::FRandRange(300.f, 700.f), 60.f * Scale, 300.f * Scale, 2.5f, 0.22f, Dust);
            AddPiece(D);
        }
        break;
    }
    case EStrategyBlastKind::GroundImpact:
    {
        AddEarth(10, 500.f, 1100.f, 380.f, Scale);
        for (int32 i = 0; i < 3; ++i)
        {
            AddPiece(Smoke(FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), 40.f), FVector(0.f, 0.f, FMath::FRandRange(150.f, 300.f)) + Flat * 200.f,
                90.f * Scale, FMath::FRandRange(380.f, 520.f) * Scale, 3.2f, 0.6f, Dust));
        }
        FPiece Scar;
        Scar.bFlat = true;
        Scar.Position = FVector(0.f, 0.f, -GetActorLocation().Z + GroundZAt(GetActorLocation()) + 3.f);
        Scar.StartDiameter = 160.f * Scale;
        Scar.EndDiameter = 170.f * Scale;
        Scar.Life = 50.f;
        Scar.Opacity = 0.35f;
        Scar.Colour = Earth * 1.6f;
        AddPiece(Scar);
        break;
    }
    case EStrategyBlastKind::ShellBurst:
    {
        AddFlash(60000.f, 4000.f * Scale, 0.16f);
        FPiece Ball = Smoke(FVector(0.f, 0.f, 80.f), FVector(0.f, 0.f, 200.f), 80.f * Scale, 420.f * Scale, 0.22f, 1.0f, Fire);
        Ball.bGlow = true;
        Ball.Drag = 4.f;
        AddPiece(Ball);
        AddEarth(16, 700.f, 1500.f, 600.f, Scale * 1.2f);
        for (int32 i = 0; i < 5; ++i)
        {
            AddPiece(Smoke(FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), 100.f), FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(250.f, 500.f)),
                120.f * Scale, FMath::FRandRange(550.f, 800.f) * Scale, 5.5f, 0.8f, i < 3 ? BlackSmoke : Dust));
        }
        FPiece Crater;
        Crater.bFlat = true;
        Crater.Position = FVector(0.f, 0.f, -GetActorLocation().Z + GroundZAt(GetActorLocation()) + 3.f);
        Crater.StartDiameter = 260.f * Scale;
        Crater.EndDiameter = 280.f * Scale;
        Crater.Life = 80.f;
        Crater.Opacity = 0.5f;
        Crater.Colour = Earth;
        AddPiece(Crater);
        break;
    }
    case EStrategyBlastKind::AirBurst:
    {
        AddFlash(25000.f, 2500.f * Scale, 0.1f);
        FPiece Spark = Smoke(FVector::ZeroVector, FVector::ZeroVector, 40.f * Scale, 160.f * Scale, 0.1f, 1.f, Fire);
        Spark.bGlow = true;
        AddPiece(Spark);
        for (int32 i = 0; i < 3; ++i)
        {
            AddPiece(Smoke(FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-20.f, 40.f)), Flat * 250.f + FVector(0.f, 0.f, 30.f),
                70.f * Scale, FMath::FRandRange(380.f, 480.f) * Scale, 5.f, 0.85f, PowderSmoke));
        }
        // The balls go on in a cone and kick up the dust where they strike, a moment later.
        const float Ground = GroundZAt(GetActorLocation()) - GetActorLocation().Z;
        for (int32 i = 0; i < 9; ++i)
        {
            const FVector At = Flat * FMath::FRandRange(300.f, 1800.f) * Scale + Right * FMath::FRandRange(-500.f, 500.f) * Scale;
            FPiece Kick = Smoke(FVector(At.X, At.Y, Ground + 20.f), FVector(0.f, 0.f, 120.f), 30.f, FMath::FRandRange(120.f, 200.f), 1.6f, 0.55f, Dust);
            Kick.Delay = FMath::FRandRange(0.08f, 0.25f);
            AddPiece(Kick);
        }
        break;
    }
    case EStrategyBlastKind::Canister:
    {
        const float Range = FMath::Max(RangeCm, 3000.f);
        const float Ground = GroundZAt(GetActorLocation()) - GetActorLocation().Z;
        for (int32 i = 0; i < 16; ++i)
        {
            const float T = FMath::FRandRange(0.25f, 1.f);
            const FVector At = Flat * Range * T + Right * Range * T * FMath::FRandRange(-0.17f, 0.17f);
            FPiece Kick = Smoke(FVector(At.X, At.Y, Ground + 15.f), FVector(0.f, 0.f, 150.f), 30.f, FMath::FRandRange(130.f, 220.f), 1.5f, 0.6f, Dust);
            Kick.Delay = T * Range / 40000.f;   // ~400 m/s
            AddPiece(Kick);
        }
        break;
    }
    case EStrategyBlastKind::HoofDust:
    {
        AddPiece(Smoke(FVector::ZeroVector, FVector(0.f, 0.f, 60.f) - Flat * 120.f, 60.f * Scale, 260.f * Scale, 1.6f, 0.32f, Dust));
        break;
    }
    }
}

void AStrategyBattleBlast::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Age += DeltaTime;
    if (Flash)
    {
        const float T = FlashSeconds > 0.f ? Age / FlashSeconds : 1.f;
        if (T >= 1.f)
        {
            Flash->DestroyComponent();
            Flash = nullptr;
        }
        else
        {
            Flash->SetIntensity(FlashIntensity * FMath::Square(1.f - T));
        }
    }
    bool bAny = Flash != nullptr;
    const float GroundZ = GetActorLocation().Z;
    for (FPiece& P : Pieces)
    {
        if (!P.Mesh)
        {
            continue;
        }
        if (P.Delay > 0.f)
        {
            P.Delay -= DeltaTime;
            bAny = true;
            if (P.Delay > 0.f)
            {
                continue;
            }
            P.Mesh->SetVisibility(true);
        }
        P.Life -= DeltaTime;
        if (P.Life <= 0.f)
        {
            P.Mesh->DestroyComponent();
            P.Mesh = nullptr;
            continue;
        }
        bAny = true;
        const float Lived = FMath::Max(0.f, Age - FMath::Max(0.f, P.Delay));
        if (P.bGravity)
        {
            P.Velocity.Z -= 980.f * DeltaTime;
        }
        P.Velocity *= FMath::Exp(-P.Drag * DeltaTime);
        P.Position += P.Velocity * DeltaTime;
        // Earth lands and stops; the smoke rises slowly when the push is spent.
        if (P.bGravity)
        {
            const float Floor = GroundZAt(GetActorLocation() + P.Position) - GroundZ;
            if (P.Position.Z < Floor)
            {
                P.Position.Z = Floor;
                P.Velocity = FVector::ZeroVector;
            }
        }
        else if (!P.bFlat && !P.bGlow)
        {
            P.Position.Z += 25.f * DeltaTime;
        }
        const float Total = P.Life + Lived;
        const float T = FMath::Clamp(Lived / FMath::Max(Total, 0.01f), 0.f, 1.f);
        const float D = FMath::Lerp(P.StartDiameter, P.EndDiameter, P.bFlat ? FMath::Min(1.f, T * 20.f) : FMath::Sqrt(T)) / 100.f;
        P.Mesh->SetRelativeLocation(P.Position);
        P.Mesh->SetRelativeScale3D(P.bFlat ? FVector(D, D, 0.02f) : FVector(D, D, D * 0.85f));
        if (P.Material)
        {
            const float Fade = P.bFlat ? FMath::Clamp(P.Life / 8.f, 0.f, 1.f) : P.bGravity ? FMath::Clamp(P.Life / 0.4f, 0.f, 1.f) : FMath::Pow(1.f - T, 1.5f);
            P.Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(P.Colour.R, P.Colour.G, P.Colour.B, P.Opacity * Fade));
        }
    }
    if (!bAny)
    {
        Destroy();
    }
}

// ------------------------------------------------------------------ the strikes to come

TArray<FStrategyImpactRegistry::FImpact>& FStrategyImpactRegistry::Impacts()
{
    static TArray<FImpact> List;
    return List;
}

void FStrategyImpactRegistry::Add(UWorld* World, const FVector& Location, float LandTime, float RadiusCm)
{
    TArray<FImpact>& List = Impacts();
    const float Now = World ? World->GetTimeSeconds() : 0.f;
    List.RemoveAll([Now, World](const FImpact& I) { return !I.World.IsValid() || I.World.Get() != World || I.LandTime < Now - 2.f; });
    FImpact I;
    I.World = World;
    I.Location = Location;
    I.LandTime = LandTime;
    I.RadiusCm = RadiusCm;
    List.Add(I);
}

void FStrategyImpactRegistry::AddCone(UWorld* World, const FVector& Origin, const FVector& Location, float LandTime)
{
    Add(World, Location, LandTime, 1800.0f);
    FImpact& I = Impacts().Last();
    I.bCone = true;
    I.Origin = Origin;
}

bool FStrategyImpactRegistry::Find(UWorld* World, const FVector& Centre, float ReachCm, float Now, FImpact& Out)
{
    float Best = ReachCm;
    bool bFound = false;
    for (const FImpact& I : Impacts())
    {
        if (I.World.Get() != World || I.LandTime < Now - 0.5f || I.LandTime > Now + 6.f)
        {
            continue;
        }
        const float D = FVector::Dist2D(I.Location, Centre);
        if (D < Best)
        {
            Best = D;
            Out = I;
            bFound = true;
        }
    }
    return bFound;
}
