#include "StrategyCampaignBattlefield.h"

#include "Campaign1851Scenery.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    const TCHAR* FieldMaterialPath = TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery");

    // The grass tiles: 25 m squares within 115 m of the camera, while the camera is under 130 m above the ground.
    constexpr float GrassTileCm = 2500.0f;
    constexpr float GrassRadiusCm = 11500.0f;
    constexpr float GrassMaxHeightCm = 13000.0f;
    constexpr float GrassPerSquareMetre = 2.8f;
    constexpr double NoGrassCellM = 2.0;

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

    UStaticMesh* BattleMesh(const TCHAR* Name)
    {
        return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Battle/Foliage/%s.%s"), Name, Name));
    }

    UMaterialInterface* BattleMaterial(const TCHAR* Name)
    {
        return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Battle/Materials/%s.%s"), Name, Name));
    }
}

AStrategyCampaignBattlefield::AStrategyCampaignBattlefield()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f;
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

bool AStrategyCampaignBattlefield::IsOpenGround(const FVector& World) const
{
    const FVector L = World - GetActorLocation();
    const double Half = SizeCm * 0.5;
    const double SizeM = SizeCm / 100.0;
    const double XM = (L.Y + Half) / 100.0, YM = (L.X + Half) / 100.0;
    if (XM < 0.0 || YM < 0.0 || XM >= SizeM || YM >= SizeM)
    {
        return false;
    }
    if (KindsGrid.Len() == Grid * Grid)
    {
        const int32 I = FMath::Clamp(int32(XM / SizeM * Grid), 0, Grid - 1), J = FMath::Clamp(int32(YM / SizeM * Grid), 0, Grid - 1);
        const TCHAR K = KindsGrid[J * Grid + I];
        if (K == TCHAR('~') || K == TCHAR('w') || K == TCHAR('t') || K == TCHAR('o'))
        {
            return false;
        }
    }
    return GrassAt(XM, YM) > 0.2f;
}

bool AStrategyCampaignBattlefield::IsWater(const FVector& World, float* OutWidthM) const
{
    const FVector L = World - GetActorLocation();
    const double Half = SizeCm * 0.5;
    const FVector2D P((L.Y + Half) / 100.0, (L.X + Half) / 100.0);
    for (const FRiver& R : Rivers)
    {
        for (int32 s = 0; s + 1 < R.Points.Num(); ++s)
        {
            const FVector2D A = R.Points[s], D = R.Points[s + 1] - A;
            const double T = FMath::Clamp(FVector2D::DotProduct(P - A, D) / FMath::Max(D.SizeSquared(), 1e-6), 0.0, 1.0);
            if (FVector2D::Distance(P, A + D * T) <= R.WidthM * 0.5)
            {
                if (OutWidthM) { *OutWidthM = R.WidthM; }
                return true;
            }
        }
    }
    if (OutWidthM) { *OutWidthM = 0.0f; }
    const double SizeM = SizeCm / 100.0;
    if (KindsGrid.Len() == Grid * Grid && P.X >= 0.0 && P.Y >= 0.0 && P.X < SizeM && P.Y < SizeM)
    {
        const TCHAR K = KindsGrid[FMath::Clamp(int32(P.Y / SizeM * Grid), 0, Grid - 1) * Grid + FMath::Clamp(int32(P.X / SizeM * Grid), 0, Grid - 1)];
        return K == TCHAR('~') || K == TCHAR('o');
    }
    return false;
}

FVector2D AStrategyCampaignBattlefield::ToField(const FVector& World) const
{
    const FVector L = World - GetActorLocation();
    const double Half = SizeCm * 0.5;
    return FVector2D((L.Y + Half) / 100.0, (L.X + Half) / 100.0);
}

int32 AStrategyCampaignBattlefield::NearestRiver(const FVector2D& P, double& OutDistanceM, FVector2D& OutAcross) const
{
    int32 Best = INDEX_NONE;
    OutDistanceM = 1e12;
    for (int32 r = 0; r < Rivers.Num(); ++r)
    {
        const TArray<FVector2D>& Pts = Rivers[r].Points;
        for (int32 s = 0; s + 1 < Pts.Num(); ++s)
        {
            const FVector2D A = Pts[s], D = Pts[s + 1] - A;
            const double T = FMath::Clamp(FVector2D::DotProduct(P - A, D) / FMath::Max(D.SizeSquared(), 1e-6), 0.0, 1.0);
            const double Dist = FVector2D::Distance(P, A + D * T);
            if (Dist < OutDistanceM)
            {
                OutDistanceM = Dist;
                Best = r;
                const FVector2D Dir = D.GetSafeNormal();
                OutAcross = FVector2D(-Dir.Y, Dir.X);
            }
        }
    }
    return Best;
}

bool AStrategyCampaignBattlefield::RouteAcrossRivers(const FVector& Start, const FVector& End, TArray<FVector>& OutVia) const
{
    OutVia.Reset();
    FVector2D Cur = ToField(Start);
    const FVector2D Goal = ToField(End);
    // Where the way Cur -> Goal first meets a broad river (the parameter along the way), crossing by crossing.
    for (int32 Guard = 0; Guard < 4; ++Guard)
    {
        double FirstT = 2.0;
        int32 FirstRiver = INDEX_NONE;
        const FVector2D W = Goal - Cur;
        for (int32 r = 0; r < Rivers.Num(); ++r)
        {
            if (Rivers[r].WidthM < BroadRiverM)
            {
                continue;
            }
            const TArray<FVector2D>& Pts = Rivers[r].Points;
            for (int32 s = 0; s + 1 < Pts.Num(); ++s)
            {
                const FVector2D A = Pts[s], D = Pts[s + 1] - A;
                const double Den = W.X * D.Y - W.Y * D.X;
                if (FMath::Abs(Den) < 1e-9)
                {
                    continue;
                }
                const FVector2D AC = A - Cur;
                const double T = (AC.X * D.Y - AC.Y * D.X) / Den;
                const double U = (AC.X * W.Y - AC.Y * W.X) / Den;
                if (T > 1e-4 && T <= 1.0 && U >= 0.0 && U <= 1.0 && T < FirstT)
                {
                    FirstT = T;
                    FirstRiver = r;
                }
            }
        }
        if (FirstRiver == INDEX_NONE)
        {
            break;
        }
        // The bridge on that river giving the shortest way.
        const FBridge* Best = nullptr;
        double BestLength = 1e12;
        for (const FBridge& B : Bridges)
        {
            if (B.River == FirstRiver)
            {
                const double Length = FVector2D::Distance(Cur, B.Centre) + FVector2D::Distance(B.Centre, Goal);
                if (Length < BestLength)
                {
                    BestLength = Length;
                    Best = &B;
                }
            }
        }
        if (!Best)
        {
            return false;
        }
        const double Side = FVector2D::DotProduct(Cur - Best->Centre, Best->Across) >= 0.0 ? 1.0 : -1.0;
        const double Reach = Best->LengthM * 0.5 + 25.0;
        const FVector2D Near = Best->Centre + Best->Across * (Side * Reach), Far = Best->Centre - Best->Across * (Side * Reach);
        OutVia.Add(FieldToWorld(Near.X, Near.Y));
        OutVia.Add(FieldToWorld(Far.X, Far.Y));
        Cur = Far;
    }
    return true;
}

float AStrategyCampaignBattlefield::WadingFactor(const FVector& World) const
{
    const FVector2D P = ToField(World);
    for (const FBridge& B : Bridges)
    {
        const FVector2D D = P - B.Centre;
        const FVector2D Along(-B.Across.Y, B.Across.X);
        if (FMath::Abs(FVector2D::DotProduct(D, B.Across)) <= B.LengthM * 0.5 + 5.0 && FMath::Abs(FVector2D::DotProduct(D, Along)) <= 8.0)
        {
            return 1.0f;   // on the bridge
        }
    }
    float WidthM = 0.0f;
    if (!IsWater(World, &WidthM))
    {
        return 1.0f;
    }
    return WidthM > 0.0f && WidthM < BroadRiverM ? 0.35f : 0.15f;
}

void AStrategyCampaignBattlefield::AddBridgeMesh(const FBridge& Bridge, UMaterialInterface* Material)
{
    if (!Material)
    {
        return;
    }
    // The deck at the higher bank plus a metre, from bank to bank.
    const FVector2D A = Bridge.Centre - Bridge.Across * (Bridge.LengthM * 0.5), B = Bridge.Centre + Bridge.Across * (Bridge.LengthM * 0.5);
    const FVector Actor = GetActorLocation();
    const FVector WA = FieldToWorld(A.X, A.Y), WB = FieldToWorld(B.X, B.Y);
    const float Deck = FMath::Max(WA.Z, WB.Z) + 100.0f - Actor.Z;
    TArray<TArray<FVector>> Pts;
    TArray<FVector>& Line = Pts.AddDefaulted_GetRef();
    for (int32 k = 0; k <= 8; ++k)
    {
        const FVector W = FMath::Lerp(WA, WB, k / 8.0f) - Actor;
        Line.Add(FVector(W.X, W.Y, Deck));
    }
    UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("Bridge")));
    C->SetupAttachment(Root);
    C->SetStaticMesh(Campaign1851Scenery::BuildRibbons(Pts, Bridge.bPontoon ? 260.0f : 380.0f,
        FLinearColor::FromSRGBColor(Bridge.bPontoon ? FColor(92, 72, 52) : FColor(132, 112, 88)), Material, TEXT("SM_Field_Bridge")));
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCastShadow(true);
    C->bVisibleInRayTracing = false;
    C->RegisterComponent();
    Parts.Add(C);
}

bool AStrategyCampaignBattlefield::LayPontoonBridge(const FVector& Near, float MaxDistanceCm)
{
    // The broad river nearest the point; the bridge where it comes closest.
    const FVector2D P = ToField(Near);
    double Best = 1e12;
    FVector2D BestPoint = FVector2D::ZeroVector, BestAcross = FVector2D(1.0, 0.0);
    int32 BestRiver = INDEX_NONE;
    for (int32 r = 0; r < Rivers.Num(); ++r)
    {
        if (Rivers[r].WidthM < BroadRiverM)
        {
            continue;
        }
        const TArray<FVector2D>& Pts = Rivers[r].Points;
        for (int32 s = 0; s + 1 < Pts.Num(); ++s)
        {
            const FVector2D A = Pts[s], D = Pts[s + 1] - A;
            const double T = FMath::Clamp(FVector2D::DotProduct(P - A, D) / FMath::Max(D.SizeSquared(), 1e-6), 0.0, 1.0);
            const FVector2D Q = A + D * T;
            const double Dist = FVector2D::Distance(P, Q);
            if (Dist < Best)
            {
                Best = Dist;
                BestPoint = Q;
                const FVector2D Dir = D.GetSafeNormal();
                BestAcross = FVector2D(-Dir.Y, Dir.X);
                BestRiver = r;
            }
        }
    }
    if (BestRiver == INDEX_NONE || Best * 100.0 > MaxDistanceCm)
    {
        return false;
    }
    FBridge Pontoon;
    Pontoon.Centre = BestPoint;
    Pontoon.Across = BestAcross;
    Pontoon.LengthM = Rivers[BestRiver].WidthM + 10.0f;
    Pontoon.River = BestRiver;
    Pontoon.bPontoon = true;
    Bridges.Add(Pontoon);
    AddBridgeMesh(Pontoon, SceneryMaterial(LoadObject<UMaterialInterface>(nullptr, FieldMaterialPath)));
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: a pontoon bridge laid over the %.0f m river at (%.0f, %.0f) m"), Rivers[BestRiver].WidthM, BestPoint.X, BestPoint.Y);
    return true;
}

float AStrategyCampaignBattlefield::GroundZ(const FVector& World) const
{
    const FVector L = World - GetActorLocation();
    const double Half = SizeCm * 0.5;
    return GetActorLocation().Z + HeightAtM((L.Y + Half) / 100.0, (L.X + Half) / 100.0) * 100.0f;
}

// ------------------------------------------------------------------------------------------------ the look

UMaterialInterface* AStrategyCampaignBattlefield::GroundMaterial() const
{
    if (UMaterialInterface* M = BattleMaterial(TEXT("M_BattleGround")))
    {
        return M;
    }
    return LoadObject<UMaterialInterface>(nullptr, FieldMaterialPath);
}

UMaterialInterface* AStrategyCampaignBattlefield::SceneryMaterial(UMaterialInterface* Fallback) const
{
    UMaterialInterface* M = BattleMaterial(TEXT("M_BattleScenery"));
    return M ? M : Fallback;
}

UMaterialInterface* AStrategyCampaignBattlefield::WaterMaterial(UMaterialInterface* Fallback) const
{
    UMaterialInterface* M = BattleMaterial(TEXT("M_BattleWater"));
    return M ? M : Fallback;
}

void AStrategyCampaignBattlefield::ResetNoGrass()
{
    NoGrassN = FMath::Clamp(int32(SizeCm / 100.0 / NoGrassCellM), 16, 4096);
    NoGrass.Init(false, NoGrassN * NoGrassN);
}

void AStrategyCampaignBattlefield::MarkNoGrass(const TArray<FVector2D>& Line, double HalfWidthM)
{
    if (NoGrassN == 0)
    {
        return;
    }
    const double Cell = SizeCm / 100.0 / NoGrassN;
    const double Reach = HalfWidthM + Cell;
    for (int32 s = 0; s + 1 < Line.Num(); ++s)
    {
        const FVector2D A = Line[s], B = Line[s + 1];
        const int32 I0 = FMath::Clamp(FMath::FloorToInt((FMath::Min(A.X, B.X) - Reach) / Cell), 0, NoGrassN - 1);
        const int32 I1 = FMath::Clamp(FMath::FloorToInt((FMath::Max(A.X, B.X) + Reach) / Cell), 0, NoGrassN - 1);
        const int32 J0 = FMath::Clamp(FMath::FloorToInt((FMath::Min(A.Y, B.Y) - Reach) / Cell), 0, NoGrassN - 1);
        const int32 J1 = FMath::Clamp(FMath::FloorToInt((FMath::Max(A.Y, B.Y) + Reach) / Cell), 0, NoGrassN - 1);
        const FVector2D D = B - A;
        const double LenSq = FMath::Max(D.SizeSquared(), 1e-6);
        for (int32 j = J0; j <= J1; ++j)
        {
            for (int32 i = I0; i <= I1; ++i)
            {
                const FVector2D P((i + 0.5) * Cell, (j + 0.5) * Cell);
                const double T = FMath::Clamp(FVector2D::DotProduct(P - A, D) / LenSq, 0.0, 1.0);
                if (FVector2D::Distance(P, A + D * T) <= Reach)
                {
                    NoGrass[j * NoGrassN + i] = true;
                }
            }
        }
    }
}

void AStrategyCampaignBattlefield::MarkNoGrassDisc(const FVector2D& Centre, double RadiusM)
{
    MarkNoGrass({ Centre, Centre + FVector2D(0.01, 0.0) }, RadiusM);
}

float AStrategyCampaignBattlefield::GrassAt(double XM, double YM) const
{
    const double SizeM = SizeCm / 100.0;
    if (XM < 0.0 || YM < 0.0 || XM >= SizeM || YM >= SizeM)
    {
        return 0.0f;
    }
    if (NoGrassN > 0)
    {
        const int32 I = FMath::Clamp(int32(XM / SizeM * NoGrassN), 0, NoGrassN - 1), J = FMath::Clamp(int32(YM / SizeM * NoGrassN), 0, NoGrassN - 1);
        if (NoGrass[J * NoGrassN + I])
        {
            return 0.0f;
        }
    }
    if (ColourN == 0 || GroundColours.Num() != (ColourN + 1) * (ColourN + 1))
    {
        return 1.0f;
    }
    const int32 I = FMath::Clamp(FMath::RoundToInt(XM / SizeM * ColourN), 0, ColourN), J = FMath::Clamp(FMath::RoundToInt(YM / SizeM * ColourN), 0, ColourN);
    const FColor C = GroundColours[J * (ColourN + 1) + I];
    // Green over blue (not water), and not brown (not dirt, not the town's ground), and not the ochre of a grain field.
    const float Green = FMath::Clamp((float(C.G) - float(C.B)) / 60.0f, 0.0f, 1.0f);
    const float Dirt = FMath::Clamp((float(C.R) - float(C.G) + 4.0f) / 16.0f, 0.0f, 1.0f);
    return Green * (1.0f - Dirt);
}

float AStrategyCampaignBattlefield::CropAt(double XM, double YM) const
{
    const double SizeM = SizeCm / 100.0;
    if (XM < 0.0 || YM < 0.0 || XM >= SizeM || YM >= SizeM || ColourN == 0 || GroundColours.Num() != (ColourN + 1) * (ColourN + 1))
    {
        return 0.0f;
    }
    if (NoGrassN > 0)
    {
        const int32 I = FMath::Clamp(int32(XM / SizeM * NoGrassN), 0, NoGrassN - 1), J = FMath::Clamp(int32(YM / SizeM * NoGrassN), 0, NoGrassN - 1);
        if (NoGrass[J * NoGrassN + I])
        {
            return 0.0f;
        }
    }
    const int32 I = FMath::Clamp(FMath::RoundToInt(XM / SizeM * ColourN), 0, ColourN), J = FMath::Clamp(FMath::RoundToInt(YM / SizeM * ColourN), 0, ColourN);
    const FColor C = GroundColours[J * (ColourN + 1) + I];
    // A ripe grain field is yellow: red a little over green and little blue (the greyer browns are ploughed land).
    const float RG = float(C.R) - float(C.G);
    const float BlueToRed = float(C.B) / FMath::Max(float(C.R), 1.0f);
    return FMath::Clamp((RG - 4.0f) / 6.0f, 0.0f, 1.0f) * (1.0f - FMath::Clamp((RG - 26.0f) / 6.0f, 0.0f, 1.0f))
        * FMath::Clamp((0.60f - BlueToRed) / 0.06f, 0.0f, 1.0f) * FMath::Clamp((float(C.R) - 120.0f) / 30.0f, 0.0f, 1.0f);
}

UInstancedStaticMeshComponent* AStrategyCampaignBattlefield::AddInstanced(UStaticMesh* Mesh, const TArray<FTransform>& Instances, bool bShadows, float CullCm, const TCHAR* Name)
{
    if (!Mesh || Instances.Num() == 0)
    {
        return nullptr;
    }
    UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), Name));
    C->SetupAttachment(Root);
    C->SetStaticMesh(Mesh);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCastShadow(bShadows);
    C->bVisibleInRayTracing = false;
    C->bAffectDistanceFieldLighting = bShadows;
    if (CullCm > 0.0f)
    {
        C->SetCullDistances(0, int32(CullCm));
    }
    C->RegisterComponent();
    C->AddInstances(Instances, false, false);
    Parts.Add(C);
    return C;
}

void AStrategyCampaignBattlefield::PlaceTree(TMap<UStaticMesh*, TArray<FTransform>>& Out, int32 Kind, const FVector& Local, float Scale, uint32 Hash) const
{
    using Campaign1851Scenery::EPiece;
    const float Pick = FieldHash01(Hash, 3u);
    const TCHAR* Name = nullptr;
    if (Kind == int32(EPiece::Conifer))
    {
        Name = Pick < 0.3f ? TEXT("SM_Spruce_A") : Pick < 0.55f ? TEXT("SM_Spruce_B") : Pick < 0.8f ? TEXT("SM_Pine_A") : TEXT("SM_Pine_B");
    }
    else if (Kind == int32(EPiece::Oak))
    {
        Name = TEXT("SM_Oak_A");
    }
    else
    {
        Name = Pick < 0.7f ? TEXT("SM_Broadleaf_A") : TEXT("SM_Oak_A");
    }
    if (UStaticMesh* Mesh = BattleMesh(Name))
    {
        const float S = Scale * (0.75f + 0.45f * FieldHash01(Hash, 5u));
        Out.FindOrAdd(Mesh).Emplace(FRotator(0.0f, 360.0f * FieldHash01(Hash, 7u), 0.0f), Local, FVector(S, S, S * (0.9f + 0.2f * FieldHash01(Hash, 9u))));
    }
}

void AStrategyCampaignBattlefield::PlaceKnick(TMap<UStaticMesh*, TArray<FTransform>>& Bushes, TMap<UStaticMesh*, TArray<FTransform>>& Trees, const TArray<FVector2D>& Line, uint32 Seed) const
{
    // A Holstein knick: an earth bank grown with hazel and hawthorn, an oak or a beech every forty metres or so.
    UStaticMesh* Bush = BattleMesh(TEXT("SM_Bush_A"));
    if (!Bush)
    {
        return;
    }
    const FVector Actor = GetActorLocation();
    uint32 K = Seed;
    for (int32 s = 0; s + 1 < Line.Num(); ++s)
    {
        const FVector2D A = Line[s], B = Line[s + 1];
        const double Len = FVector2D::Distance(A, B);
        const int32 Steps = FMath::Max(1, FMath::RoundToInt(Len / 3.2));
        for (int32 k = 0; k < Steps; ++k)
        {
            ++K;
            const FVector2D P = FMath::Lerp(A, B, (k + FieldHash01(K, 1u)) / Steps);
            const FVector Local = FieldToWorld(P.X, P.Y, 45.0) - Actor;
            const float S = 0.8f + 0.6f * FieldHash01(K, 2u);
            Bushes.FindOrAdd(Bush).Emplace(FRotator(0.0f, 360.0f * FieldHash01(K, 4u), 0.0f), Local, FVector(S * 1.3f, S * 1.3f, S));
            if (FieldHash01(K, 6u) < 0.075f)
            {
                PlaceTree(Trees, int32(Campaign1851Scenery::EPiece::Broadleaf), Local, 0.75f, K * 31u);
            }
        }
    }
}

void AStrategyCampaignBattlefield::PlaceFence(TArray<FTransform>& Out, const TArray<FVector2D>& Line, double SideM, float GapChance, uint32 Seed) const
{
    // Post-and-rail fence sections of 3 m beside a line (SideM to its left), with gaps.
    const FVector Actor = GetActorLocation();
    uint32 K = Seed;
    double Carry = 0.0;
    for (int32 s = 0; s + 1 < Line.Num(); ++s)
    {
        const FVector2D A = Line[s], B = Line[s + 1];
        const FVector2D D = (B - A).GetSafeNormal();
        const FVector2D Left(-D.Y, D.X);
        const double Len = FVector2D::Distance(A, B);
        for (double T = Carry; T + 3.0 <= Len; T += 3.0)
        {
            ++K;
            Carry = 0.0;
            if (FieldHash01(K, 11u) < GapChance)
            {
                continue;
            }
            const FVector2D P0 = A + D * T + Left * SideM, P1 = P0 + D * 3.0;
            const FVector W0 = FieldToWorld(P0.X, P0.Y) - Actor, W1 = FieldToWorld(P1.X, P1.Y) - Actor;
            const FVector Dir = W1 - W0;
            const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
            const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Dir.Z, Dir.Size2D()));
            Out.Emplace(FRotator(Pitch, Yaw, 0.0f), W0, FVector(Dir.Size() / 300.0, 1.0, 1.0));
        }
    }
}

// ------------------------------------------------------------------------------------------------ grass tiles

void AStrategyCampaignBattlefield::SetupGrass()
{
    GrassMeshes.Reset();
    // Kinds 0-2: grass; 3-4: ripe wheat (the ochre fields; only if both are there).
    for (const TCHAR* Name : { TEXT("SM_Grass_Clump_A"), TEXT("SM_Grass_Clump_C"), TEXT("SM_Grass_Clump_B"), TEXT("SM_Wheat_A"), TEXT("SM_Wheat_B") })
    {
        if (UStaticMesh* Mesh = BattleMesh(Name))
        {
            GrassMeshes.Add(Mesh);
        }
    }
    int32 Flag = 1;
    if (FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Grass="), Flag) && Flag == 0)
    {
        bStreamGrass = false;
    }
    FreeGrass.SetNum(GrassMeshes.Num());
    SetActorTickEnabled(bStreamGrass && GrassMeshes.Num() > 0);
}

void AStrategyCampaignBattlefield::BuildGrassTile(const FIntPoint& Tile, TArray<int32>& Components)
{
    const int32 Kinds = GrassMeshes.Num();
    TArray<TArray<FTransform>> Instances;
    Instances.SetNum(Kinds);
    const double SizeM = SizeCm / 100.0;
    const double TileM = GrassTileCm / 100.0;
    const int32 Count = FMath::RoundToInt(TileM * TileM * GrassPerSquareMetre);
    const uint32 Seed = HashCombine(GetTypeHash(Tile.X * 7919), GetTypeHash(Tile.Y));
    const FVector Actor = GetActorLocation();
    for (int32 k = 0; k < Count; ++k)
    {
        const double X = (Tile.X + FieldHash01(Seed, k * 4u)) * TileM, Y = (Tile.Y + FieldHash01(Seed, k * 4u + 1u)) * TileM;
        if (X < 0.0 || Y < 0.0 || X >= SizeM || Y >= SizeM)
        {
            continue;
        }
        const float Roll = FieldHash01(Seed, k * 4u + 2u);
        const float Pick = FieldHash01(Seed, k * 4u + 3u);
        // Ripe grain: a standing field of wheat.
        const float Crop = Kinds >= 5 ? CropAt(X, Y) : 0.0f;
        if (Crop > 0.0f)
        {
            if (Roll < Crop)
            {
                const float H = 0.85f + 0.3f * FieldHash01(Seed + 29u, k);
                Instances[Pick < 0.55f ? 3 : 4].Emplace(FRotator(0.0f, 360.0f * FieldHash01(Seed + 31u, k), 0.0f), FieldToWorld(X, Y, -2.0) - Actor, FVector(H, H, H * (0.9f + 0.2f * Pick)));
                // A second clump close by: the field is thick.
                const double X2 = X + (FieldHash01(Seed + 37u, k) - 0.5) * 0.8, Y2 = Y + (FieldHash01(Seed + 41u, k) - 0.5) * 0.8;
                Instances[Pick < 0.5f ? 4 : 3].Emplace(FRotator(0.0f, 360.0f * FieldHash01(Seed + 43u, k), 0.0f), FieldToWorld(X2, Y2, -2.0) - Actor, FVector(H * 0.95f, H * 0.95f, H * (0.85f + 0.25f * (1.0f - Pick))));
            }
            continue;
        }
        const float Weight = GrassAt(X, Y);
        if (Roll >= Weight)
        {
            continue;
        }
        const int32 Kind = FMath::Min(Kinds - 1, Pick < 0.55f ? 0 : Pick < 0.88f ? 1 : 2);
        const float S = 0.5f + 0.45f * FieldHash01(Seed + 17u, k);
        Instances[Kind].Emplace(FRotator(0.0f, 360.0f * Roll / FMath::Max(Weight, 0.01f), 0.0f), FieldToWorld(X, Y, -2.0) - Actor, FVector(S, S, S * (0.8f + 0.4f * Pick)));
    }
    for (int32 Kind = 0; Kind < Kinds; ++Kind)
    {
        if (Instances[Kind].Num() == 0)
        {
            continue;
        }
        int32 Index = INDEX_NONE;
        if (FreeGrass[Kind].Num() > 0)
        {
            Index = FreeGrass[Kind].Pop(EAllowShrinking::No);
        }
        else
        {
            UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(this, MakeUniqueObjectName(this, UInstancedStaticMeshComponent::StaticClass(), TEXT("Grass")));
            C->SetupAttachment(Root);
            C->SetStaticMesh(GrassMeshes[Kind]);
            C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            C->SetCastShadow(false);
            C->bVisibleInRayTracing = false;
            C->bAffectDistanceFieldLighting = false;
            C->SetCullDistances(0, int32(GrassRadiusCm));
            C->ComponentTags.Add(FName(*FString::FromInt(Kind)));
            C->RegisterComponent();
            Index = GrassPool.Add(C);
        }
        UInstancedStaticMeshComponent* C = GrassPool[Index];
        C->AddInstances(Instances[Kind], false, false);
        C->SetVisibility(true);
        Components.Add(Index);
    }
}

void AStrategyCampaignBattlefield::UpdateGrass()
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC || !PC->PlayerCameraManager)
    {
        return;
    }
    const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();
    const bool bLow = Camera.Z - GroundZ(Camera) < GrassMaxHeightCm;
    // The tiles wanted: within the radius of the camera's ground point (none when the camera is high).
    TSet<FIntPoint> Wanted;
    if (bLow)
    {
        const FVector L = Camera - GetActorLocation();
        const double Half = SizeCm * 0.5;
        const double Xcm = L.Y + Half, Ycm = L.X + Half;   // field x east, y north (cm)
        const int32 R = FMath::CeilToInt(GrassRadiusCm / GrassTileCm);
        const int32 Cx = FMath::FloorToInt(Xcm / GrassTileCm), Cy = FMath::FloorToInt(Ycm / GrassTileCm);
        for (int32 dy = -R; dy <= R; ++dy)
        {
            for (int32 dx = -R; dx <= R; ++dx)
            {
                const double Nx = FMath::Clamp(Xcm, (Cx + dx) * double(GrassTileCm), (Cx + dx + 1) * double(GrassTileCm));
                const double Ny = FMath::Clamp(Ycm, (Cy + dy) * double(GrassTileCm), (Cy + dy + 1) * double(GrassTileCm));
                if (FVector2D(Nx - Xcm, Ny - Ycm).Size() <= GrassRadiusCm)
                {
                    Wanted.Add(FIntPoint(Cx + dx, Cy + dy));
                }
            }
        }
    }
    // Drop the tiles no longer wanted (their components back to the pool).
    for (auto It = GrassTiles.CreateIterator(); It; ++It)
    {
        if (Wanted.Contains(It.Key()))
        {
            continue;
        }
        for (int32 Index : It.Value())
        {
            UInstancedStaticMeshComponent* C = GrassPool[Index];
            C->ClearInstances();
            C->SetVisibility(false);
            const int32 Kind = C->ComponentTags.Num() > 0 ? FCString::Atoi(*C->ComponentTags[0].ToString()) : 0;
            FreeGrass[FMath::Clamp(Kind, 0, FreeGrass.Num() - 1)].Add(Index);
        }
        It.RemoveCurrent();
    }
    // Build at most a few new tiles a tick (no hitch when the camera jumps).
    int32 Budget = 6;
    for (const FIntPoint& Tile : Wanted)
    {
        if (GrassTiles.Contains(Tile))
        {
            continue;
        }
        if (Budget-- <= 0)
        {
            break;
        }
        BuildGrassTile(Tile, GrassTiles.Add(Tile));
    }
}

void AStrategyCampaignBattlefield::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    GrassTimer -= DeltaSeconds;
    if (GrassTimer <= 0.0f)
    {
        GrassTimer = 0.1f;
        UpdateGrass();
    }
}

// ------------------------------------------------------------------------------------------------ the meadow

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
    UMaterialInterface* GroundMat = GroundMaterial();
    ResetNoGrass();

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
    MarkNoGrass(Track, 2.0);

    // ---- the ground: 2.5 m a vertex, a late-summer meadow in patches (olive, lush, straw, darker hollows)
    {
        const int32 N = FMath::Clamp(int32(SizeM / 2.5), 64, 520);
        const double Step = SizeM / N;
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FColor> Colours;
        const FColor Fresh(96, 116, 54), Lush(72, 98, 42), Dry(148, 140, 84), Dark(56, 78, 36), Dirt(132, 108, 76), Flowers(154, 148, 100);
        for (int32 j = 0; j <= N; ++j)
        {
            for (int32 i = 0; i <= N; ++i)
            {
                const double X = i * Step, Y = j * Step;
                Vertices.Add(FieldToWorld(X, Y) - GetActorLocation());
                FColor C = Mix(Fresh, Lush, MeadowNoise(S + 3u, X, Y, 40.0));
                C = Mix(C, Dry, FMath::Max(0.0f, MeadowNoise(S + 4u, X, Y, 110.0) - 0.5f) * 1.8f);
                C = Mix(C, Dark, FMath::Max(0.0f, MeadowNoise(S + 5u, X, Y, 18.0) - 0.62f) * 2.0f);
                C = Mix(C, Flowers, FMath::Max(0.0f, MeadowNoise(S + 6u, X, Y, 7.0) - 0.8f) * 2.0f);
                const double TD = TrackDistance(X, Y);
                if (TD < 3.5)
                {
                    C = Mix(C, Dirt, float(1.0 - TD / 3.5) * 1.4f);
                }
                Colours.Add(C);
                UVs.Add(FVector2D(double(i) / N, double(j) / N));
                const float Dx = HeightAtM(X + Step, Y) - HeightAtM(X - Step, Y), Dy = HeightAtM(X, Y + Step) - HeightAtM(X, Y - Step);
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
        GroundColours = Colours;
        ColourN = N;
        Ground->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colours, TArray<FProcMeshTangent>(), false);
        if (GroundMat)
        {
            Ground->SetMaterial(0, GroundMat);
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
        for (const FVector2D& P : Track) { Out.Add(FieldToWorld(P.X, P.Y, 6.0) - GetActorLocation()); }
        UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, TEXT("MeadowTrack"));
        C->SetupAttachment(Root);
        C->SetStaticMesh(Campaign1851Scenery::BuildRibbons(Pts, 180.0f, FLinearColor::FromSRGBColor(FColor(140, 114, 80)), GroundMat ? GroundMat : Material, TEXT("SM_Meadow_Track")));
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(false);
        C->RegisterComponent();
        Parts.Add(C);
    }
    // ---- trees: a wood behind each side's start, copses along the flanks, a few single trees; knicks; fences
    using Campaign1851Scenery::EPiece;
    const bool bBattleFoliage = BattleMesh(TEXT("SM_Spruce_A")) != nullptr;
    TMap<int32, TArray<FTransform>> Items;
    TMap<UStaticMesh*, TArray<FTransform>> Trees, Bushes;
    const FVector Actor = GetActorLocation();
    int32 Key = 0;
    auto Tree = [&](double X, double Y, float Size)
    {
        const uint32 H = uint32(++Key);
        const float Pick = FieldHash01(S + 21u, H);
        const EPiece Piece = Pick < 0.5f ? EPiece::Conifer : Pick < 0.7f ? EPiece::Oak : EPiece::Broadleaf;
        if (bBattleFoliage)
        {
            PlaceTree(Trees, int32(Piece), FieldToWorld(X, Y) - Actor, 1.0f, S * 977u + H);
            return;
        }
        Items.FindOrAdd(int32(Piece)).Emplace(FRotator(0.0f, 360.0f * FieldHash01(S + 22u, H), 0.0f), FieldToWorld(X, Y) - Actor,
            FVector(Size * 100.0f * (0.8f + 0.5f * FieldHash01(S + 23u, H))));
    };
    for (int32 t = 0; t < 3400; ++t)
    {
        const double X = FieldHash01(S + 30u, t) * SizeM, Y = FieldHash01(S + 31u, t) * SizeM;
        const double Cx = X - SizeM * 0.5, Cy = Y - SizeM * 0.5;
        // Woods beyond 330 m north and south of the middle and at the flanks beyond 380 m; thin in between.
        const bool bWood = FMath::Abs(Cy) > 330.0 || FMath::Abs(Cx) > 380.0;
        const float Clump = MeadowNoise(S + 32u, X, Y, 70.0);
        if (((bWood && Clump > 0.38f) || (!bWood && Clump > 0.84f && FMath::Abs(Cy) > 150.0)) && TrackDistance(X, Y) >= 8.0)
        {
            Tree(X, Y, 1.8f);
            MarkNoGrassDisc(FVector2D(X, Y), 1.5);
        }
    }
    // Knicks along two field edges (east and west of the middle, north to south); a fence along the track and
    // round a pasture in the north-west.
    TArray<FTransform> Fences;
    for (const double Cx : { -260.0, 260.0 })
    {
        TArray<FVector2D> Line;
        for (double Y = SizeM * 0.15; Y < SizeM * 0.85; Y += 4.0)
        {
            Line.Add(FVector2D(SizeM * 0.5 + Cx + 4.0 * FMath::Sin(Y / 40.0), Y));
        }
        if (bBattleFoliage)
        {
            PlaceKnick(Bushes, Trees, Line, S + uint32(Cx + 1000.0));
            MarkNoGrass(Line, 1.5);
        }
        else
        {
            for (const FVector2D& P : Line)
            {
                if (FieldHash01(S + 40u, uint32(P.Y)) < 0.12f) { continue; }   // gaps
                Items.FindOrAdd(int32(EPiece::Knick)).Emplace(FRotator(0.0f, 0.0f, 0.0f), FieldToWorld(P.X, P.Y) - Actor, FVector(100.0f, 140.0f, 140.0f));
            }
        }
    }
    PlaceFence(Fences, Track, 4.0, 0.12f, S + 50u);
    {
        const double X0 = SizeM * 0.5 - 240.0, X1 = SizeM * 0.5 - 90.0, Y0 = SizeM * 0.5 + 160.0, Y1 = SizeM * 0.5 + 300.0;
        PlaceFence(Fences, { FVector2D(X0, Y0), FVector2D(X1, Y0), FVector2D(X1, Y1), FVector2D(X0, Y1), FVector2D(X0, Y0 + 12.0) }, 0.0, 0.05f, S + 51u);
    }
    for (const TPair<int32, TArray<FTransform>>& It : Items)
    {
        AddInstanced(Campaign1851Scenery::Build(EPiece(It.Key), SceneryMaterial(Material)), It.Value, true, 0.0f, TEXT("MeadowPieces"));
    }
    for (const TPair<UStaticMesh*, TArray<FTransform>>& It : Trees)
    {
        AddInstanced(It.Key, It.Value, true, 0.0f, TEXT("MeadowTrees"));
    }
    for (const TPair<UStaticMesh*, TArray<FTransform>>& It : Bushes)
    {
        AddInstanced(It.Key, It.Value, true, 250000.0f, TEXT("MeadowBushes"));
    }
    AddInstanced(BattleMesh(TEXT("SM_Fence_Rail")), Fences, true, 150000.0f, TEXT("MeadowFences"));
    SetupGrass();
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: meadow %.0f m square, %d parts, battle foliage %s, grass %s"), SizeM, Parts.Num(),
        bBattleFoliage ? TEXT("yes") : TEXT("no"), IsActorTickEnabled() ? TEXT("streamed") : TEXT("off"));
}

// ------------------------------------------------------------------------------------------------ the campaign's field

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
    ResetNoGrass();
    KindsGrid = Json->GetStringField(TEXT("kinds"));
    Rivers.Reset();

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
    // The picture's colours a little muted and darkened for the lit ground (it was painted for the unlit map).
    const bool bLitGround = BattleMaterial(TEXT("M_BattleGround")) != nullptr;
    auto PixelAt = [&](double XM, double YM)
    {
        if (Pixels.Num() == 0)
        {
            return FColor(118, 128, 74);
        }
        const int32 Px = FMath::Clamp(int32(XM / SizeM * ImageW), 0, ImageW - 1), Py = FMath::Clamp(int32((SizeM - YM) / SizeM * ImageH), 0, ImageH - 1);
        FColor C = Pixels[Py * ImageW + Px];
        if (bLitGround)
        {
            const float Grey = (C.R + C.G + C.B) / 3.0f;
            auto F = [&](uint8 V) { return uint8(FMath::Clamp((Grey + (V - Grey) * 0.85f) * 0.8f, 0.0f, 255.0f)); };
            C = FColor(F(C.R), F(C.G), F(C.B), 255);
        }
        return C;
    };

    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, FieldMaterialPath);
    UMaterialInterface* GroundMat = GroundMaterial();

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
        GroundColours = Colours;
        ColourN = N;
        Ground->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colours, Tangents, false);
        if (GroundMat)
        {
            Ground->SetMaterial(0, GroundMat);
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
        C->bVisibleInRayTracing = false;
        C->RegisterComponent();
        Parts.Add(C);
    };

    // ---- ribbons: rivers, ways (world positions relative to the actor, a little above the ground)
    auto Ribbons = [&](const TArray<TArray<FVector2D>>& Lines, double WidthM, const FLinearColor& Colour, double LiftCm, const TCHAR* Name, UMaterialInterface* RibbonMaterial)
    {
        if (!RibbonMaterial || Lines.Num() == 0)
        {
            return;
        }
        TArray<TArray<FVector>> Pts;
        for (const TArray<FVector2D>& L : Lines)
        {
            MarkNoGrass(L, WidthM * 0.5 + 0.5);
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
        AddMesh(Campaign1851Scenery::BuildRibbons(Pts, float(WidthM * 50.0), Colour, RibbonMaterial, *FString::Printf(TEXT("SM_Field_%s"), Name)), Name);
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
    UMaterialInterface* WayMaterial = GroundMat ? GroundMat : Material;
    {
        // Each river at its own width (the Ejder is fifty metres across, a brook a few).
        const TArray<TSharedPtr<FJsonValue>>* RiverArray = nullptr;
        if (Json->TryGetArrayField(TEXT("rivers"), RiverArray))
        {
            for (const TSharedPtr<FJsonValue>& V : *RiverArray)
            {
                double WidthM = 12.0;
                V->AsObject()->TryGetNumberField(TEXT("widthM"), WidthM);
                FRiver& River = Rivers.AddDefaulted_GetRef();
                River.Points = ReadPoints(V->AsObject()->GetArrayField(TEXT("points")));
                River.WidthM = float(FMath::Clamp(WidthM, 3.0, 120.0));
                Ribbons({ ReadPoints(V->AsObject()->GetArrayField(TEXT("points"))) }, FMath::Clamp(WidthM, 3.0, 120.0),
                    FLinearColor::FromSRGBColor(FColor(60, 86, 100)), 30.0, TEXT("Rivers"), WaterMaterial(Material));
            }
        }
    }
    const TArray<TArray<FVector2D>> Tracks = LinesOf(TEXT("tracks")), Lanes = LinesOf(TEXT("lanes"));
    Ribbons(Tracks, 3.0, FLinearColor::FromSRGBColor(FColor(140, 118, 84)), 35.0, TEXT("Tracks"), WayMaterial);
    Ribbons(Lanes, 5.0, FLinearColor::FromSRGBColor(FColor(160, 138, 100)), 40.0, TEXT("Lanes"), WayMaterial);
    Ribbons(LinesOf(TEXT("roads")), 8.0, FLinearColor::FromSRGBColor(FColor(176, 156, 118)), 45.0, TEXT("Roads"), WayMaterial);
    Ribbons(LinesOf(TEXT("chaussees")), 10.0, FLinearColor::FromSRGBColor(FColor(190, 178, 150)), 50.0, TEXT("Chaussees"), WayMaterial);
    Ribbons(LinesOf(TEXT("railways")), 5.0, FLinearColor::FromSRGBColor(FColor(52, 46, 42)), 55.0, TEXT("Rails"), SceneryMaterial(Material));

    // ---- the bridges (each on the river it crosses, across it there), drawn as decks
    Bridges.Reset();
    {
        const TArray<TSharedPtr<FJsonValue>>* BridgeArray = nullptr;
        if (Json->TryGetArrayField(TEXT("bridges"), BridgeArray))
        {
            for (const TSharedPtr<FJsonValue>& V : *BridgeArray)
            {
                const TSharedPtr<FJsonObject> B = V->AsObject();
                FString State = TEXT("intact");
                B->TryGetStringField(TEXT("state"), State);
                if (State != TEXT("intact"))
                {
                    continue;   // a blown bridge is no crossing
                }
                FBridge Bridge;
                Bridge.Centre = FVector2D(B->GetNumberField(TEXT("x")), B->GetNumberField(TEXT("y")));
                double Distance = 0.0;
                Bridge.River = NearestRiver(Bridge.Centre, Distance, Bridge.Across);
                if (Bridge.River == INDEX_NONE || Distance > Rivers[Bridge.River].WidthM * 0.5 + 60.0)
                {
                    continue;
                }
                double LengthM = 30.0;
                B->TryGetNumberField(TEXT("lengthM"), LengthM);
                Bridge.LengthM = float(FMath::Max(LengthM, double(Rivers[Bridge.River].WidthM) + 10.0));
                Bridges.Add(Bridge);
                AddBridgeMesh(Bridge, SceneryMaterial(Material));
            }
        }
    }

    // ---- instanced pieces: buildings, woods, field boundaries
    if (Material)
    {
        using Campaign1851Scenery::EPiece;
        const bool bBattleFoliage = BattleMesh(TEXT("SM_Spruce_A")) != nullptr;
        TMap<int32, TArray<FTransform>> Items;
        TMap<UStaticMesh*, TArray<FTransform>> Trees, Bushes, KnickTrees;
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
                MarkNoGrassDisc(FVector2D(X, Y), FMath::Max(W, D) * 0.6 + 1.0);
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
                if (bBattleFoliage)
                {
                    PlaceTree(Trees, int32(Tree), Local(X, Y), 1.0f, Seed * 131u + uint32(k * 16 + t));
                }
                else
                {
                    Items.FindOrAdd(int32(Tree)).Emplace(FRotator(0.0f, 360.0f * FieldHash01(Seed + 11u, k * 16 + t), 0.0f), Local(X, Y), FVector(Size * 100.0f));
                }
            }
        }
        // Field boundaries: the knicks grown with bushes and trees (the bank as the map's piece), dikes, ditches.
        const TArray<TSharedPtr<FJsonValue>>* Hedges = nullptr;
        if (Json->TryGetArrayField(TEXT("hedges"), Hedges))
        {
            uint32 HedgeSeed = Seed;
            for (const TSharedPtr<FJsonValue>& V : *Hedges)
            {
                const TSharedPtr<FJsonObject> H = V->AsObject();
                const FString Kind = H->GetStringField(TEXT("kind"));
                const EPiece Piece = Kind == TEXT("knick") ? EPiece::Knick : Kind == TEXT("dike") ? EPiece::StoneDike : EPiece::Ditch;
                const TArray<FVector2D> P = ReadPoints(H->GetArrayField(TEXT("points")));
                if (Piece == EPiece::Knick && bBattleFoliage)
                {
                    PlaceKnick(Bushes, KnickTrees, P, HedgeSeed += 7919u);
                    MarkNoGrass(P, 1.5);
                }
                for (int32 s = 0; s + 1 < P.Num(); ++s)
                {
                    const double Len = FVector2D::Distance(P[s], P[s + 1]);
                    if (Len < 1.0)
                    {
                        continue;
                    }
                    const FVector2D Mid = (P[s] + P[s + 1]) * 0.5;
                    const double Deg = FMath::RadiansToDegrees(FMath::Atan2(P[s + 1].Y - P[s].Y, P[s + 1].X - P[s].X));
                    Items.FindOrAdd(int32(Piece)).Emplace(FRotator(0.0f, Yaw(Deg), 0.0f), Local(Mid.X, Mid.Y), FVector(Len * 100.0 / 4.0, 140.0, Piece == EPiece::Knick && bBattleFoliage ? 70.0 : 140.0));
                }
            }
        }
        // Fences beside the lanes and tracks (one side, with gaps).
        TArray<FTransform> Fences;
        if (bBattleFoliage)
        {
            uint32 FenceSeed = Seed + 401u;
            for (const TArray<FVector2D>& L : Lanes) { PlaceFence(Fences, L, 3.6, 0.3f, FenceSeed += 31u); }
            for (const TArray<FVector2D>& L : Tracks) { PlaceFence(Fences, L, 2.6, 0.45f, FenceSeed += 31u); }
        }
        for (const TPair<int32, TArray<FTransform>>& It : Items)
        {
            const bool bBuilding = It.Key != int32(EPiece::Knick) && It.Key != int32(EPiece::StoneDike) && It.Key != int32(EPiece::Ditch);
            AddInstanced(Campaign1851Scenery::Build(EPiece(It.Key), SceneryMaterial(Material)), It.Value, bBuilding, 0.0f, TEXT("FieldPieces"));
        }
        for (const TPair<UStaticMesh*, TArray<FTransform>>& It : Trees)
        {
            AddInstanced(It.Key, It.Value, true, 0.0f, TEXT("FieldTrees"));
        }
        for (const TPair<UStaticMesh*, TArray<FTransform>>& It : KnickTrees)
        {
            AddInstanced(It.Key, It.Value, true, 600000.0f, TEXT("KnickTrees"));
        }
        for (const TPair<UStaticMesh*, TArray<FTransform>>& It : Bushes)
        {
            AddInstanced(It.Key, It.Value, false, 180000.0f, TEXT("KnickBushes"));
        }
        AddInstanced(BattleMesh(TEXT("SM_Fence_Rail")), Fences, true, 150000.0f, TEXT("Fences"));
    }
    SetupGrass();
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: %s, %.0f m square, grid %d, %d parts, picture %dx%d, grass %s"), *Place, SizeM, Grid, Parts.Num(), ImageW, ImageH,
        IsActorTickEnabled() ? TEXT("streamed") : TEXT("off"));
    return true;
}
