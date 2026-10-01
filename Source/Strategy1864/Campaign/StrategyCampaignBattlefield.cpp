#include "StrategyCampaignBattlefield.h"

#include "Campaign1851Scenery.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    const TCHAR* FieldMaterialPath = TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery");

    float FieldHash01(uint32 A, uint32 B)
    {
        uint32 H = HashCombine(A * 2246822519u + 3266489917u, B);
        H ^= H >> 13; H *= 0x5bd1e995u; H ^= H >> 15;
        return (H & 0xffffff) / float(0xffffff);
    }

    TArray<FVector2D> ReadPoints(const TArray<TSharedPtr<FJsonValue>>& Flat)
    {
        TArray<FVector2D> Out;
        for (int32 i = 0; i + 1 < Flat.Num(); i += 2)
        {
            Out.Add(FVector2D(Flat[i]->AsNumber(), Flat[i + 1]->AsNumber()));
        }
        return Out;
    }
}

AStrategyCampaignBattlefield::AStrategyCampaignBattlefield()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Ground = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Ground"));
    Ground->SetupAttachment(Root);
    Ground->bUseAsyncCooking = false;
    Ground->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Ground->SetCollisionObjectType(ECC_WorldStatic);
    Ground->SetCastShadow(false);
}

float AStrategyCampaignBattlefield::HeightAtM(double XM, double YM) const
{
    if (HeightM.Num() != Grid * Grid)
    {
        return 0.0f;
    }
    const double SizeM = SizeCm / 100.0;
    const double Cell = SizeM / Grid;
    const double Gx = FMath::Clamp(XM / Cell - 0.5, 0.0, Grid - 1.001), Gy = FMath::Clamp(YM / Cell - 0.5, 0.0, Grid - 1.001);
    const int32 I = int32(Gx), J = int32(Gy);
    const float Ax = float(Gx - I), Ay = float(Gy - J);
    auto H = [&](int32 x, int32 y) { return HeightM[FMath::Min(y, Grid - 1) * Grid + FMath::Min(x, Grid - 1)]; };
    return FMath::Lerp(FMath::Lerp(H(I, J), H(I + 1, J), Ax), FMath::Lerp(H(I, J + 1), H(I + 1, J + 1), Ax), Ay) - MinHeightM;
}

FVector AStrategyCampaignBattlefield::FieldToWorld(double XM, double YM, double LiftCm) const
{
    const double Half = SizeCm * 0.5;
    return GetActorLocation() + FVector(YM * 100.0 - Half, XM * 100.0 - Half, HeightAtM(XM, YM) * 100.0 + LiftCm);
}

float AStrategyCampaignBattlefield::GroundZ(const FVector& World) const
{
    const FVector L = World - GetActorLocation();
    const double Half = SizeCm * 0.5;
    return GetActorLocation().Z + HeightAtM((L.Y + Half) / 100.0, (L.X + Half) / 100.0) * 100.0f;
}

namespace
{
    float MeadowNoise(uint32 Seed, double X, double Y, double Lattice)
    {
        const double Fx = X / Lattice, Fy = Y / Lattice;
        const int32 Ix = FMath::FloorToInt(Fx), Iy = FMath::FloorToInt(Fy);
        const float Tx = float(Fx - Ix), Ty = float(Fy - Iy);
        auto L = [&](int32 A, int32 B) { return FieldHash01(Seed, uint32(A * 73856093) ^ uint32(B * 19349663)); };
        const float Sx = Tx * Tx * (3.0f - 2.0f * Tx), Sy = Ty * Ty * (3.0f - 2.0f * Ty);
        return FMath::Lerp(FMath::Lerp(L(Ix, Iy), L(Ix + 1, Iy), Sx), FMath::Lerp(L(Ix, Iy + 1), L(Ix + 1, Iy + 1), Sx), Sy);
    }

    FColor Mix(const FColor& A, const FColor& B, float T)
    {
        T = FMath::Clamp(T, 0.0f, 1.0f);
        return FColor(uint8(FMath::Lerp(float(A.R), float(B.R), T)), uint8(FMath::Lerp(float(A.G), float(B.G), T)), uint8(FMath::Lerp(float(A.B), float(B.B), T)), 255);
    }
}

void AStrategyCampaignBattlefield::BuildMeadow(float InSizeM, int32 Seed)
{
    const uint32 S = uint32(Seed);
    Place = TEXT("Testmark");
    SizeCm = InSizeM * 100.0f;
    const double SizeM = InSizeM;
    // Heights: long low swells, a couple of metres; the middle (where the QA units stand) kept nearly flat.
    Grid = 200;
    HeightM.SetNumUninitialized(Grid * Grid);
    const double Cell = SizeM / Grid;
    for (int32 j = 0; j < Grid; ++j)
    {
        for (int32 i = 0; i < Grid; ++i)
        {
            const double X = (i + 0.5) * Cell, Y = (j + 0.5) * Cell;
            const double R = FVector2D(X - SizeM * 0.5, Y - SizeM * 0.5).Size() / (SizeM * 0.5);
            const float Swell = 3.5f * MeadowNoise(S, X, Y, 260.0) + 1.5f * MeadowNoise(S + 1u, X, Y, 90.0);
            HeightM[j * Grid + i] = Swell * float(FMath::Clamp((R - 0.25) / 0.5, 0.15, 1.0));
        }
    }
    MinHeightM = TNumericLimits<float>::Max();
    for (float H : HeightM) { MinHeightM = FMath::Min(MinHeightM, H); }
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, FieldMaterialPath);

    // The track: a gentle curve across the field (dirt), kept for the colours and a ribbon.
    TArray<FVector2D> Track;
    for (int32 k = 0; k <= 40; ++k)
    {
        const double T = k / 40.0;
        Track.Add(FVector2D(SizeM * (0.08 + 0.84 * T), SizeM * (0.18 + 0.1 * FMath::Sin(T * 3.0) + 0.05 * T)));
    }
    auto TrackDistance = [&](double X, double Y)
    {
        double Best = 1e9;
        for (int32 k = 0; k + 1 < Track.Num(); ++k)
        {
            const FVector2D A = Track[k], D = Track[k + 1] - A;
            const double T = FMath::Clamp(FVector2D::DotProduct(FVector2D(X, Y) - A, D) / D.SizeSquared(), 0.0, 1.0);
            Best = FMath::Min(Best, FVector2D::Distance(FVector2D(X, Y), A + D * T));
        }
        return Best;
    };

    // ---- the ground: 2.5 m a vertex, grass in patches, hillshade from the north-west baked in
    {
        const int32 N = FMath::Clamp(int32(SizeM / 2.5), 64, 520);
        const double Step = SizeM / N;
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FColor> Colours;
        const FColor Fresh(98, 132, 58), Lush(76, 112, 46), Dry(146, 146, 82), Dark(60, 90, 40), Dirt(138, 112, 78), Flowers(150, 148, 96);
        for (int32 j = 0; j <= N; ++j)
        {
            for (int32 i = 0; i <= N; ++i)
            {
                const double X = i * Step, Y = j * Step;
                Vertices.Add(FieldToWorld(X, Y) - GetActorLocation());
                FColor C = Mix(Fresh, Lush, MeadowNoise(S + 3u, X, Y, 40.0));
                C = Mix(C, Dry, FMath::Max(0.0f, MeadowNoise(S + 4u, X, Y, 110.0) - 0.55f) * 2.2f);
                C = Mix(C, Dark, FMath::Max(0.0f, MeadowNoise(S + 5u, X, Y, 18.0) - 0.62f) * 2.0f);
                C = Mix(C, Flowers, FMath::Max(0.0f, MeadowNoise(S + 6u, X, Y, 7.0) - 0.8f) * 2.5f);
                // Fine grain: blades and tufts.
                const float Grain = 0.92f + 0.16f * FieldHash01(S + 7u, uint32(i * 7919 + j));
                C = FColor(uint8(FMath::Min(255.0f, C.R * Grain)), uint8(FMath::Min(255.0f, C.G * Grain)), uint8(FMath::Min(255.0f, C.B * Grain)), 255);
                const double TD = TrackDistance(X, Y);
                if (TD < 3.5)
                {
                    C = Mix(C, Dirt, float(1.0 - TD / 3.5) * 1.4f);
                }
                // Light from the north-west.
                const float Dx = HeightAtM(X + Step, Y) - HeightAtM(X - Step, Y), Dy = HeightAtM(X, Y + Step) - HeightAtM(X, Y - Step);
                const float Shade = FMath::Clamp(1.0f + (-Dx + Dy) / float(2.0 * Step) * 3.0f, 0.75f, 1.2f);
                C = FColor(uint8(FMath::Min(255.0f, C.R * Shade)), uint8(FMath::Min(255.0f, C.G * Shade)), uint8(FMath::Min(255.0f, C.B * Shade)), 255);
                Colours.Add(C);
                UVs.Add(FVector2D(double(i) / N, double(j) / N));
                Normals.Add(FVector(-Dy, -Dx, 2.0 * Step).GetSafeNormal());
            }
        }
        for (int32 j = 0; j < N; ++j)
        {
            for (int32 i = 0; i < N; ++i)
            {
                const int32 A = j * (N + 1) + i, B = A + 1, C = A + N + 1, D = C + 1;
                Triangles.Append({ A, B, C, B, D, C });
            }
        }
        Ground->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colours, TArray<FProcMeshTangent>(), false);
        if (Material)
        {
            Ground->SetMaterial(0, Material);
        }
        TArray<FVector> CV;
        TArray<int32> CT;
        for (int32 j = 0; j <= Grid; ++j)
        {
            for (int32 i = 0; i <= Grid; ++i)
            {
                CV.Add(FieldToWorld(i * Cell, j * Cell) - GetActorLocation());
            }
        }
        for (int32 j = 0; j < Grid; ++j)
        {
            for (int32 i = 0; i < Grid; ++i)
            {
                const int32 A = j * (Grid + 1) + i, B = A + 1, C = A + Grid + 1, D = C + 1;
                CT.Append({ A, B, C, B, D, C });
            }
        }
        Ground->CreateMeshSection(1, CV, CT, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
        Ground->SetMeshSectionVisible(1, false);
    }
    if (!Material)
    {
        return;
    }
    // ---- the track as a slightly raised ribbon
    {
        TArray<TArray<FVector>> Pts;
        TArray<FVector>& Out = Pts.AddDefaulted_GetRef();
        for (const FVector2D& P : Track) { Out.Add(FieldToWorld(P.X, P.Y, 8.0) - GetActorLocation()); }
        UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, TEXT("MeadowTrack"));
        C->SetupAttachment(Root);
        C->SetStaticMesh(Campaign1851Scenery::BuildRibbons(Pts, 180.0f, FLinearColor::FromSRGBColor(FColor(150, 124, 88)), Material, TEXT("SM_Meadow_Track")));
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->RegisterComponent();
        Parts.Add(C);
    }
    // ---- trees: a wood behind each side's start, copses along the flanks, a few single trees; hedges
    using Campaign1851Scenery::EPiece;
    TMap<int32, TArray<FTransform>> Items;
    const FVector Actor = GetActorLocation();
    int32 Key = 0;
    auto Tree = [&](double X, double Y, float Size)
    {
        const uint32 H = uint32(++Key);
        const float Pick = FieldHash01(S + 21u, H);
        const EPiece Piece = Pick < 0.35f ? EPiece::Conifer : Pick < 0.6f ? EPiece::Oak : EPiece::Broadleaf;
        Items.FindOrAdd(int32(Piece)).Emplace(FRotator(0.0f, 360.0f * FieldHash01(S + 22u, H), 0.0f), FieldToWorld(X, Y) - Actor,
            FVector(Size * 100.0f * (0.8f + 0.5f * FieldHash01(S + 23u, H))));
    };
    for (int32 t = 0; t < 2600; ++t)
    {
        const double X = FieldHash01(S + 30u, t) * SizeM, Y = FieldHash01(S + 31u, t) * SizeM;
        const double Cx = X - SizeM * 0.5, Cy = Y - SizeM * 0.5;
        // Woods beyond 330 m north and south of the middle and at the flanks beyond 380 m; thin in between.
        const bool bWood = FMath::Abs(Cy) > 330.0 || FMath::Abs(Cx) > 380.0;
        const float Clump = MeadowNoise(S + 32u, X, Y, 70.0);
        if ((bWood && Clump > 0.38f) || (!bWood && Clump > 0.86f && FMath::Abs(Cy) > 150.0) || TrackDistance(X, Y) < 6.0)
        {
            if (TrackDistance(X, Y) >= 6.0) { Tree(X, Y, 1.8f); }
        }
    }
    // Hedges along two field edges (east and west of the middle, north to south).
    for (const double Cx : { -260.0, 260.0 })
    {
        for (double Y = SizeM * 0.15; Y < SizeM * 0.85; Y += 4.0)
        {
            const double X = SizeM * 0.5 + Cx + 4.0 * FMath::Sin(Y / 40.0);
            if (FieldHash01(S + 40u, uint32(Y)) < 0.12f) { continue; }   // gaps
            Items.FindOrAdd(int32(EPiece::Knick)).Emplace(FRotator(0.0f, 0.0f, 0.0f), FieldToWorld(X, Y) - Actor, FVector(100.0f, 140.0f, 140.0f));
        }
    }
    for (const TPair<int32, TArray<FTransform>>& It : Items)
    {
        UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("MeadowPieces")));
        C->SetupAttachment(Root);
        C->SetStaticMesh(Campaign1851Scenery::Build(EPiece(It.Key), Material));
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(true);
        C->RegisterComponent();
        C->AddInstances(It.Value, false, false);
        Parts.Add(C);
    }
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: meadow %.0f m square, %d parts"), SizeM, Parts.Num());
}

bool AStrategyCampaignBattlefield::BuildFromFile(const FString& FileName)
{
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Battle");
    const FString Path = FPaths::FileExists(FileName) ? FileName : Dir / FileName;
    FString Text;
    TSharedPtr<FJsonObject> Json;
    if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("PROJECT1864-FIELD: cannot read %s"), *Path);
        return false;
    }
    Place = Json->GetStringField(TEXT("place"));
    const double SizeM = Json->GetNumberField(TEXT("sizeM"));
    Grid = int32(Json->GetNumberField(TEXT("grid")));
    SizeCm = float(SizeM * 100.0);
    HeightM.Reset();
    for (const TSharedPtr<FJsonValue>& V : Json->GetArrayField(TEXT("heightDm")))
    {
        HeightM.Add(float(V->AsNumber()) / 10.0f);
    }
    if (HeightM.Num() != Grid * Grid)
    {
        UE_LOG(LogTemp, Error, TEXT("PROJECT1864-FIELD: %s has %d heights for a %d grid"), *Path, HeightM.Num(), Grid);
        return false;
    }
    MinHeightM = TNumericLimits<float>::Max();
    for (float H : HeightM) { MinHeightM = FMath::Min(MinHeightM, H); }

    // The picture (row 0 north): the ground's colours.
    TArray<FColor> Pixels;
    int32 ImageW = 0, ImageH = 0;
    {
        TArray<uint8> Png;
        // The ground alone when the campaign wrote it (newer files), else the full picture.
        FString ImagePath = FPaths::GetPath(Path) / Json->GetStringField(TEXT("image"));
        const FString GroundPath = FPaths::GetPath(Path) / (FPaths::GetBaseFilename(ImagePath) + TEXT("_ground.png"));
        if (FPaths::FileExists(GroundPath))
        {
            ImagePath = GroundPath;
        }
        IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
        TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
        TArray<uint8> Raw;
        if (FFileHelper::LoadFileToArray(Png, *ImagePath) && Wrapper.IsValid() && Wrapper->SetCompressed(Png.GetData(), Png.Num()) &&
            Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
        {
            ImageW = int32(Wrapper->GetWidth());
            ImageH = int32(Wrapper->GetHeight());
            Pixels.SetNumUninitialized(ImageW * ImageH);
            FMemory::Memcpy(Pixels.GetData(), Raw.GetData(), Pixels.Num() * sizeof(FColor));
        }
    }
    auto PixelAt = [&](double XM, double YM)
    {
        if (Pixels.Num() == 0)
        {
            return FColor(140, 150, 90);
        }
        const int32 Px = FMath::Clamp(int32(XM / SizeM * ImageW), 0, ImageW - 1), Py = FMath::Clamp(int32((SizeM - YM) / SizeM * ImageH), 0, ImageH - 1);
        return Pixels[Py * ImageW + Px];
    };

    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, FieldMaterialPath);

    // ---- the ground: a vertex per picture pixel for the look, the height grid for the collision
    {
        const int32 N = FMath::Clamp(ImageW > 0 ? ImageW : Grid * 2, 64, 512);
        const double Step = SizeM / N;
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FColor> Colours;
        TArray<FProcMeshTangent> Tangents;
        Vertices.Reserve((N + 1) * (N + 1));
        for (int32 j = 0; j <= N; ++j)
        {
            for (int32 i = 0; i <= N; ++i)
            {
                const double X = i * Step, Y = j * Step;
                Vertices.Add(FieldToWorld(X, Y) - GetActorLocation());
                Colours.Add(PixelAt(FMath::Min(X + Step * 0.5, SizeM), FMath::Min(Y + Step * 0.5, SizeM)));
                UVs.Add(FVector2D(double(i) / N, double(j) / N));
                // A normal from the slope.
                const float Dx = HeightAtM(X + Step, Y) - HeightAtM(X - Step, Y), Dy = HeightAtM(X, Y + Step) - HeightAtM(X, Y - Step);
                Normals.Add(FVector(-Dy, -Dx, 2.0 * Step).GetSafeNormal());
            }
        }
        for (int32 j = 0; j < N; ++j)
        {
            for (int32 i = 0; i < N; ++i)
            {
                const int32 A = j * (N + 1) + i, B = A + 1, C = A + N + 1, D = C + 1;
                // World X is north (j), Y east (i): wind the triangles to face up.
                Triangles.Append({ A, B, C, B, D, C });
            }
        }
        Ground->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colours, Tangents, false);
        if (Material)
        {
            Ground->SetMaterial(0, Material);
        }

        // Collision on the height grid.
        TArray<FVector> CV;
        TArray<int32> CT;
        const double CStep = SizeM / Grid;
        for (int32 j = 0; j <= Grid; ++j)
        {
            for (int32 i = 0; i <= Grid; ++i)
            {
                CV.Add(FieldToWorld(i * CStep, j * CStep) - GetActorLocation());
            }
        }
        for (int32 j = 0; j < Grid; ++j)
        {
            for (int32 i = 0; i < Grid; ++i)
            {
                const int32 A = j * (Grid + 1) + i, B = A + 1, C = A + Grid + 1, D = C + 1;
                CT.Append({ A, B, C, B, D, C });
            }
        }
        Ground->CreateMeshSection(1, CV, CT, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
        Ground->SetMeshSectionVisible(1, false);
    }

    auto AddMesh = [&](UStaticMesh* Mesh, const TCHAR* Name)
    {
        UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Name));
        C->SetupAttachment(Root);
        C->SetStaticMesh(Mesh);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(false);
        C->RegisterComponent();
        Parts.Add(C);
    };

    // ---- ribbons: rivers, ways (world positions relative to the actor, a little above the ground)
    auto Ribbons = [&](const TArray<TArray<FVector2D>>& Lines, double WidthM, const FLinearColor& Colour, double LiftCm, const TCHAR* Name)
    {
        if (!Material || Lines.Num() == 0)
        {
            return;
        }
        TArray<TArray<FVector>> Pts;
        for (const TArray<FVector2D>& L : Lines)
        {
            TArray<FVector>& Out = Pts.AddDefaulted_GetRef();
            for (int32 i = 0; i + 1 < L.Num(); ++i)
            {
                const double Len = FVector2D::Distance(L[i], L[i + 1]);
                const int32 Steps = FMath::Max(1, FMath::CeilToInt(Len / 10.0));
                for (int32 s = 0; s < Steps; ++s)
                {
                    const FVector2D P = FMath::Lerp(L[i], L[i + 1], double(s) / Steps);
                    Out.Add(FieldToWorld(P.X, P.Y, LiftCm) - GetActorLocation());
                }
            }
            if (L.Num() > 0)
            {
                Out.Add(FieldToWorld(L.Last().X, L.Last().Y, LiftCm) - GetActorLocation());
            }
        }
        AddMesh(Campaign1851Scenery::BuildRibbons(Pts, float(WidthM * 50.0), Colour, Material, *FString::Printf(TEXT("SM_Field_%s"), Name)), Name);
    };
    auto LinesOf = [&](const TCHAR* Field)
    {
        TArray<TArray<FVector2D>> Out;
        const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
        if (Json->TryGetArrayField(Field, Array))
        {
            for (const TSharedPtr<FJsonValue>& V : *Array)
            {
                Out.Add(ReadPoints(V->AsArray()));
            }
        }
        return Out;
    };
    {
        TArray<TArray<FVector2D>> Water;
        const TArray<TSharedPtr<FJsonValue>>* Rivers = nullptr;
        if (Json->TryGetArrayField(TEXT("rivers"), Rivers))
        {
            for (const TSharedPtr<FJsonValue>& V : *Rivers)
            {
                Water.Add(ReadPoints(V->AsObject()->GetArrayField(TEXT("points"))));
            }
        }
        Ribbons(Water, 12.0, FLinearColor::FromSRGBColor(FColor(74, 112, 140)), 30.0, TEXT("Rivers"));
    }
    Ribbons(LinesOf(TEXT("tracks")), 3.0, FLinearColor::FromSRGBColor(FColor(150, 128, 92)), 35.0, TEXT("Tracks"));
    Ribbons(LinesOf(TEXT("lanes")), 5.0, FLinearColor::FromSRGBColor(FColor(196, 172, 128)), 40.0, TEXT("Lanes"));
    Ribbons(LinesOf(TEXT("roads")), 8.0, FLinearColor::FromSRGBColor(FColor(222, 202, 156)), 45.0, TEXT("Roads"));
    Ribbons(LinesOf(TEXT("chaussees")), 10.0, FLinearColor::FromSRGBColor(FColor(240, 232, 206)), 50.0, TEXT("Chaussees"));
    Ribbons(LinesOf(TEXT("railways")), 5.0, FLinearColor::FromSRGBColor(FColor(52, 46, 42)), 55.0, TEXT("Rails"));

    // ---- instanced pieces: buildings, woods, field boundaries
    if (Material)
    {
        using Campaign1851Scenery::EPiece;
        TMap<int32, TArray<FTransform>> Items;
        const FVector ActorLocation = GetActorLocation();
        auto Local = [&](double X, double Y) { return FieldToWorld(X, Y) - ActorLocation; };
        // Battlefield yaw (degrees north of east) to world yaw (world +X north, +Y east).
        auto Yaw = [](double DegNorthOfEast) { return float(90.0 - DegNorthOfEast); };
        // Buildings and a clear ring round them for the trees.
        TArray<FVector2D> Built;
        const TArray<TSharedPtr<FJsonValue>>* Buildings = nullptr;
        if (Json->TryGetArrayField(TEXT("buildings"), Buildings))
        {
            for (const TSharedPtr<FJsonValue>& V : *Buildings)
            {
                const TSharedPtr<FJsonObject> B = V->AsObject();
                const FString Kind = B->GetStringField(TEXT("kind"));
                const double X = B->GetNumberField(TEXT("x")), Y = B->GetNumberField(TEXT("y"));
                const double W = B->GetNumberField(TEXT("w")), D = B->GetNumberField(TEXT("d"));
                const EPiece Piece = Kind == TEXT("church") ? EPiece::Church : Kind == TEXT("farm") ? EPiece::Cottage : Kind == TEXT("house") ? EPiece::TownHouse : EPiece::MerchantHouse;
                const double PieceW = Piece == EPiece::Church ? 14.0 : Piece == EPiece::Cottage ? 5.5 : 6.0;
                const double PieceD = Piece == EPiece::Church ? 6.0 : Piece == EPiece::Cottage ? 3.4 : 4.0;
                const double Sx = W * 100.0 / PieceW, Sy = D * 100.0 / PieceD;
                Items.FindOrAdd(int32(Piece)).Emplace(FRotator(0.0f, Yaw(B->GetNumberField(TEXT("yaw"))), 0.0f), Local(X, Y) - FVector(0.0, 0.0, 30.0), FVector(Sx, Sy, (Sx + Sy) * 0.5));
                Built.Add(FVector2D(X, Y));
            }
        }
        // Woods: a tree in about every 18 m square of the wood cells, not on the ways.
        const FString Kinds = Json->GetStringField(TEXT("kinds"));
        const FString Woods = Json->GetStringField(TEXT("woodDensity"));
        const double Cell = SizeM / Grid;
        const uint32 Seed = GetTypeHash(Place);
        for (int32 k = 0; k < Kinds.Len() && k < Grid * Grid; ++k)
        {
            if (Kinds[k] != TCHAR('w'))
            {
                continue;
            }
            const int32 i = k % Grid, j = k / Grid;
            const float Density = Woods.IsValidIndex(k) ? (Woods[k] - TCHAR('0')) / 9.0f : 0.6f;
            const int32 Per = FMath::Clamp(int32(Cell * Cell / 320.0 * FMath::Max(0.3f, Density)), 1, 8);
            for (int32 t = 0; t < Per; ++t)
            {
                const double X = (i + FieldHash01(Seed, k * 16 + t)) * Cell, Y = (j + FieldHash01(Seed + 7u, k * 16 + t)) * Cell;
                const float Size = 1.6f + 0.8f * FieldHash01(Seed + 3u, k * 16 + t);
                const EPiece Tree = FieldHash01(Seed + 5u, k * 16 + t) < 0.25f ? EPiece::Conifer : FieldHash01(Seed + 9u, k) < 0.3f ? EPiece::Oak : EPiece::Broadleaf;
                Items.FindOrAdd(int32(Tree)).Emplace(FRotator(0.0f, 360.0f * FieldHash01(Seed + 11u, k * 16 + t), 0.0f), Local(X, Y), FVector(Size * 100.0f));
            }
        }
        // Field boundaries.
        const TArray<TSharedPtr<FJsonValue>>* Hedges = nullptr;
        if (Json->TryGetArrayField(TEXT("hedges"), Hedges))
        {
            for (const TSharedPtr<FJsonValue>& V : *Hedges)
            {
                const TSharedPtr<FJsonObject> H = V->AsObject();
                const FString Kind = H->GetStringField(TEXT("kind"));
                const EPiece Piece = Kind == TEXT("knick") ? EPiece::Knick : Kind == TEXT("dike") ? EPiece::StoneDike : EPiece::Ditch;
                const TArray<FVector2D> P = ReadPoints(H->GetArrayField(TEXT("points")));
                for (int32 s = 0; s + 1 < P.Num(); ++s)
                {
                    const double Len = FVector2D::Distance(P[s], P[s + 1]);
                    if (Len < 1.0)
                    {
                        continue;
                    }
                    const FVector2D Mid = (P[s] + P[s + 1]) * 0.5;
                    const double Deg = FMath::RadiansToDegrees(FMath::Atan2(P[s + 1].Y - P[s].Y, P[s + 1].X - P[s].X));
                    Items.FindOrAdd(int32(Piece)).Emplace(FRotator(0.0f, Yaw(Deg), 0.0f), Local(Mid.X, Mid.Y), FVector(Len * 100.0 / 4.0, 140.0, 140.0));
                }
            }
        }
        for (const TPair<int32, TArray<FTransform>>& It : Items)
        {
            UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("FieldPieces")));
            C->SetupAttachment(Root);
            C->SetStaticMesh(Campaign1851Scenery::Build(EPiece(It.Key), Material));
            C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            C->SetCastShadow(true);
            C->RegisterComponent();
            C->AddInstances(It.Value, false, false);
            Parts.Add(C);
        }
    }
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: %s, %.0f m square, grid %d, %d parts, picture %dx%d"), *Place, SizeM, Grid, Parts.Num(), ImageW, ImageH);
    return true;
}
