#include "Campaign1851Map.h"

#include "Campaign1851Camera.h"
#include "Campaign1851Json.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshAttributes.h"

namespace Campaign1851Tiles
{
    constexpr int32 HeightSide = 257;
    constexpr int32 ForestSide = 65;
    constexpr int32 ResidentLimit = 24;

    bool ReadBounds(const FJsonObject& TileObject, FCampaign1851Extent& TileBounds)
    {
        return TileObject.TryGetNumberField(TEXT("xMin"), TileBounds.XMin) && TileObject.TryGetNumberField(TEXT("xMax"), TileBounds.XMax)
            && TileObject.TryGetNumberField(TEXT("yMin"), TileBounds.YMin) && TileObject.TryGetNumberField(TEXT("yMax"), TileBounds.YMax)
            && FMath::IsFinite(TileBounds.XMin) && FMath::IsFinite(TileBounds.XMax) && FMath::IsFinite(TileBounds.YMin) && FMath::IsFinite(TileBounds.YMax)
            && TileBounds.XMax > TileBounds.XMin && TileBounds.YMax > TileBounds.YMin;
    }
}

bool ACampaign1851Map::LoadMapTiles()
{
    TileStartSeconds = FPlatformTime::Seconds();
    FString TileText;
    TSharedPtr<FJsonObject> TileJson;
    const FString TileDataDir = FPaths::ProjectDir() / TEXT("Data/Campaign1851");
    if (!FFileHelper::LoadFileToString(TileText, *(TileDataDir / TEXT("MapTiles.json")))) { return false; }
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(TileText), TileJson) || !TileJson.IsValid()) { return false; }
    // Import is optional: never replace the working Danish map with a blank material.
    TileMasterMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Campaign1851/M_Campaign1851Tile.M_Campaign1851Tile"));
    TileOverviewMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Campaign1851/M_Campaign1851TileOverview.M_Campaign1851TileOverview"));
    if (!TileMasterMaterial || !TileOverviewMaterial)
    {
        UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-TILES|import mangler; bruger oprindeligt kort"));
        return false;
    }
    const TArray<TSharedPtr<FJsonValue>>* TileRows = nullptr;
    if (!TileJson->TryGetArrayField(TEXT("tiles"), TileRows) || TileRows->Num() == 0) { return false; }
    int32 TilePixelSide = 0;
    const TSharedPtr<FJsonObject> TileOriginJson = Campaign1851Json::Object(*TileJson, TEXT("originKm"));
    const TSharedPtr<FJsonObject> TilePixelJson = Campaign1851Json::Object(*TileJson, TEXT("kmPerPixel"));
    if (!TileJson->TryGetNumberField(TEXT("tileSizePx"), TilePixelSide) || TilePixelSide != 1024
        || !Campaign1851Tiles::ReadBounds(*Campaign1851Json::Object(*TileJson, TEXT("extentKm")), TileExtent)
        || !TileOriginJson->TryGetNumberField(TEXT("x"), TileOrigin.X) || !TileOriginJson->TryGetNumberField(TEXT("y"), TileOrigin.Y)
        || !TilePixelJson->TryGetNumberField(TEXT("x"), TileSpan.X) || !TilePixelJson->TryGetNumberField(TEXT("y"), TileSpan.Y)
        || !FMath::IsFinite(TileOrigin.X) || !FMath::IsFinite(TileOrigin.Y) || !FMath::IsFinite(TileSpan.X) || !FMath::IsFinite(TileSpan.Y)
        || TileSpan.X <= 0 || TileSpan.Y <= 0) { return false; }
    TileSpan *= TilePixelSide;
    for (const TSharedPtr<FJsonValue>& TileValue : *TileRows)
    {
        const TSharedPtr<FJsonObject> TileRow = Campaign1851Json::Row(TileValue);
        FCampaign1851RuntimeTile TileEntry;
        if (!TileRow->TryGetNumberField(TEXT("x"), TileEntry.Key.X) || !TileRow->TryGetNumberField(TEXT("y"), TileEntry.Key.Y)
            || !Campaign1851Tiles::ReadBounds(*Campaign1851Json::Object(*TileRow, TEXT("boundsKm")), TileEntry.Bounds)
            || RuntimeTileLookup.Contains(TileEntry.Key)
            || !FMath::IsNearlyEqual(TileEntry.Bounds.XMin, TileOrigin.X + TileEntry.Key.X * TileSpan.X, 0.000001)
            || !FMath::IsNearlyEqual(TileEntry.Bounds.XMax, TileOrigin.X + (double(TileEntry.Key.X) + 1.0) * TileSpan.X, 0.000001)
            || !FMath::IsNearlyEqual(TileEntry.Bounds.YMax, TileOrigin.Y - TileEntry.Key.Y * TileSpan.Y, 0.000001)
            || !FMath::IsNearlyEqual(TileEntry.Bounds.YMin, TileOrigin.Y - (double(TileEntry.Key.Y) + 1.0) * TileSpan.Y, 0.000001))
        { RuntimeTiles.Reset(); RuntimeTileLookup.Reset(); return false; }
        TArray<uint8> TileBytes;
        const FString TileFile = FString::Printf(TEXT("MapTileHeights/Tile_%d_%d.bin"), TileEntry.Key.X, TileEntry.Key.Y);
        constexpr int32 TileHeightCount = Campaign1851Tiles::HeightSide * Campaign1851Tiles::HeightSide;
        if (!FFileHelper::LoadFileToArray(TileBytes, *(TileDataDir / TileFile)) || TileBytes.Num() != TileHeightCount * 2 + Campaign1851Tiles::ForestSide * Campaign1851Tiles::ForestSide)
        {
            RuntimeTiles.Reset(); RuntimeTileLookup.Reset();
            UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-TILES|ugyldige runtimehøjder: %s; oprindeligt kort"), *TileFile);
            return false;
        }
        TileEntry.Heights.SetNumUninitialized(TileHeightCount);
        for (int32 TileSample = 0; TileSample < TileHeightCount; ++TileSample)
        { TileEntry.Heights[TileSample] = uint16(TileBytes[TileSample * 2]) | (uint16(TileBytes[TileSample * 2 + 1]) << 8); }
        TileEntry.Forest.Append(TileBytes.GetData() + TileHeightCount * 2, Campaign1851Tiles::ForestSide * Campaign1851Tiles::ForestSide);
        RuntimeTileLookup.Add(TileEntry.Key, RuntimeTiles.Num());
        RuntimeTiles.Add(MoveTemp(TileEntry));
    }
    RuntimeTileMeshes.SetNum(RuntimeTiles.Num());
    RuntimeTileHandles.SetNum(RuntimeTiles.Num());
    TileLegacyDetailUvYScale = MapTextureYScale;
    MapTextureYScale = 1.f; // No north texture stretch or flat north materials in the tile path.
    bUseMapTiles = true;
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-TILES|fundet=%d|cache=%d|LOD=257/129/65|oversigt klar"), RuntimeTiles.Num(), Campaign1851Tiles::ResidentLimit);
    return true;
}

FVector2D ACampaign1851Map::GetNavigationSizeKm() const
{
    return bUseMapTiles ? FVector2D(TileExtent.XMax - TileExtent.XMin, TileExtent.YMax - TileExtent.YMin) : SizeKm;
}

FVector2D ACampaign1851Map::GetNavigationCentre() const
{
    return bUseMapTiles ? FVector2D(((TileExtent.XMin + TileExtent.XMax) - (Extent.XMin + Extent.XMax)) * 0.5 * KmToUnits,
        -((TileExtent.YMin + TileExtent.YMax) - (Extent.YMin + Extent.YMax)) * 0.5 * KmToUnits) : FVector2D::ZeroVector;
}

int32 ACampaign1851Map::FindMapTile(const FVector2D& TileKm) const
{
    const FIntPoint TileKey(FMath::FloorToInt((TileKm.X - TileOrigin.X) / TileSpan.X), FMath::FloorToInt((TileOrigin.Y - TileKm.Y) / TileSpan.Y));
    const int32* TileFound = RuntimeTileLookup.Find(TileKey);
    return TileFound ? *TileFound : INDEX_NONE;
}

float ACampaign1851Map::TileHeightAtKm(const FVector2D& TileKm) const
{
    const int32 TileIndex = FindMapTile(TileKm);
    if (TileIndex == INDEX_NONE) { return 0.f; }
    const FCampaign1851RuntimeTile& TileEntry = RuntimeTiles[TileIndex];
    const double TileGx = FMath::Clamp((TileKm.X - TileEntry.Bounds.XMin) / TileSpan.X, 0.0, 1.0) * 256.0;
    const double TileGy = FMath::Clamp((TileEntry.Bounds.YMax - TileKm.Y) / TileSpan.Y, 0.0, 1.0) * 256.0;
    const int32 TileX = FMath::Min(int32(TileGx), 255), TileY = FMath::Min(int32(TileGy), 255);
    const float TileFx = float(TileGx - TileX), TileFy = float(TileGy - TileY);
    const int32 TileBase = TileY * 257 + TileX;
    const float TileA = TileEntry.Heights[TileBase], TileB = TileEntry.Heights[TileBase + 1], TileC = TileEntry.Heights[TileBase + 257], TileD = TileEntry.Heights[TileBase + 258];
    // Identical triangle split to LOD0. CPU height never depends on cache residency.
    const float TileH = TileFx + TileFy <= 1.f ? TileA + TileFx * (TileB - TileA) + TileFy * (TileC - TileA)
        : TileD + (1.f - TileFx) * (TileC - TileD) + (1.f - TileFy) * (TileB - TileD);
    return TileH / 65535.f * HeightRangeKm * float(KmToUnits);
}

float ACampaign1851Map::TileForestAtKm(const FVector2D& TileKm) const
{
    const int32 TileIndex = FindMapTile(TileKm);
    if (TileIndex == INDEX_NONE) { return 0.f; }
    const FCampaign1851RuntimeTile& TileEntry = RuntimeTiles[TileIndex];
    const int32 TileX = FMath::Clamp(FMath::RoundToInt((TileKm.X - TileEntry.Bounds.XMin) / TileSpan.X * 64), 0, 64);
    const int32 TileY = FMath::Clamp(FMath::RoundToInt((TileEntry.Bounds.YMax - TileKm.Y) / TileSpan.Y * 64), 0, 64);
    return TileEntry.Forest[TileY * 65 + TileX] / 255.f;
}

void ACampaign1851Map::UpdateMapTiles()
{
    FVector TileFocus = Project(55.55, 10.45);
    for (TActorIterator<ACampaign1851Camera> TileCamera(GetWorld()); TileCamera; ++TileCamera) { TileFocus = TileCamera->GetTarget(); break; }
    TileFocus = GetActorTransform().InverseTransformPosition(TileFocus);
    const FVector2D TileFocusKm(TileFocus.X / KmToUnits + (Extent.XMin + Extent.XMax) * 0.5, -TileFocus.Y / KmToUnits + (Extent.YMin + Extent.YMax) * 0.5);
    TArray<int32> TileOrder;
    for (int32 TileIndex = 0; TileIndex < RuntimeTiles.Num(); ++TileIndex) { TileOrder.Add(TileIndex); }
    TileOrder.Sort([&](int32 TileA, int32 TileB)
    {
        const FCampaign1851Extent& TileBA = RuntimeTiles[TileA].Bounds;
        const FCampaign1851Extent& TileBB = RuntimeTiles[TileB].Bounds;
        return FVector2D::DistSquared(TileFocusKm, FVector2D((TileBA.XMin + TileBA.XMax) * 0.5, (TileBA.YMin + TileBA.YMax) * 0.5))
            < FVector2D::DistSquared(TileFocusKm, FVector2D((TileBB.XMin + TileBB.XMax) * 0.5, (TileBB.YMin + TileBB.YMax) * 0.5));
    });
    const int32 TileKeep = FMath::Min(Campaign1851Tiles::ResidentLimit, TileOrder.Num());
    for (int32 TileRank = TileKeep; TileRank < TileOrder.Num(); ++TileRank)
    {
        TObjectPtr<UStaticMeshComponent>& TileComponent = RuntimeTileMeshes[TileOrder[TileRank]];
        if (TileComponent)
        {
            if (RuntimeTileHandles[TileOrder[TileRank]]) { RuntimeTileHandles[TileOrder[TileRank]]->CancelHandle(); RuntimeTileHandles[TileOrder[TileRank]].Reset(); }
            TileComponent->DestroyComponent(); TileComponent = nullptr;
        }
    }
    for (int32 TileRank = 0; TileRank < TileKeep; ++TileRank)
    {
        if (!RuntimeTileMeshes[TileOrder[TileRank]])
        {
            // One mesh (three LODs) per tick, no all-map startup mesh. The four tiles nearest the camera load their
            // textures at once, so the first close view shows the fields and not the darker overview sheet.
            BuildMapTile(TileOrder[TileRank], TileRank < 4);
            if (!bTileFirstFrameLogged && RuntimeTileMeshes[TileOrder[TileRank]])
            {
                bTileFirstFrameLogged = true;
                UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-TILES|første nærflise indsendt=%.3f s|fundet=%d|bygget=1"), FPlatformTime::Seconds() - TileStartSeconds, RuntimeTiles.Num());
            }
            break;
        }
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("CampaignTileStats")) && GFrameCounter % 300 == 0)
    {
        int32 TileBuilt = 0;
        for (const TObjectPtr<UStaticMeshComponent>& TileComponent : RuntimeTileMeshes) { TileBuilt += TileComponent != nullptr; }
        UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-TILES|fundet=%d|bygget=%d|sek=%.3f"), RuntimeTiles.Num(), TileBuilt, FPlatformTime::Seconds() - TileStartSeconds);
    }
}

void ACampaign1851Map::BuildMapTile(int32 TileIndex, bool bTileTexturesNow)
{
    const FCampaign1851RuntimeTile& TileEntry = RuntimeTiles[TileIndex];
    FMeshDescription TileDescriptions[3];
    for (int32 TileLod = 0; TileLod < 3; ++TileLod)
    {
        FMeshDescription& TileMesh = TileDescriptions[TileLod];
        FStaticMeshAttributes TileAttributes(TileMesh);
        TileAttributes.Register();
        TileAttributes.GetVertexInstanceUVs().SetNumChannels(3);
        const FPolygonGroupID TileGroup = TileMesh.CreatePolygonGroup();
        TileAttributes.GetPolygonGroupMaterialSlotNames()[TileGroup] = TEXT("Map");
        auto TilePositions = TileAttributes.GetVertexPositions();
        auto TileNormals = TileAttributes.GetVertexInstanceNormals();
        auto TileTangents = TileAttributes.GetVertexInstanceTangents();
        auto TileSigns = TileAttributes.GetVertexInstanceBinormalSigns();
        auto TileUVs = TileAttributes.GetVertexInstanceUVs();
        const int32 TileStep = 1 << TileLod, TileCells = 256 / TileStep, TileSide = TileCells + 1;
        TArray<FVertexInstanceID> TileVertices;
        for (int32 TileY = 0; TileY <= TileCells; ++TileY)
        {
            for (int32 TileX = 0; TileX <= TileCells; ++TileX)
            {
                const FVector2D TileUv(double(TileX) / TileCells, double(TileY) / TileCells);
                const FVector2D TileKm(TileEntry.Bounds.XMin + TileUv.X * TileSpan.X, TileEntry.Bounds.YMax - TileUv.Y * TileSpan.Y);
                const FVertexID TileVertex = TileMesh.CreateVertex();
                FVector TileLocal((TileKm.X - (Extent.XMin + Extent.XMax) * 0.5) * KmToUnits, -(TileKm.Y - (Extent.YMin + Extent.YMax) * 0.5) * KmToUnits, 0.0);
                // Read this tile's shared-edge sample, including the outer east/south boundary.
                TileLocal.Z = TileEntry.Heights[TileY * TileStep * 257 + TileX * TileStep] / 65535.f * HeightRangeKm * KmToUnits;
                TilePositions[TileVertex] = FVector3f(TileLocal);
                const FVertexInstanceID TileInstance = TileMesh.CreateVertexInstance(TileVertex);
                TileNormals[TileInstance] = FVector3f::UpVector;
                TileTangents[TileInstance] = FVector3f::ForwardVector;
                TileSigns[TileInstance] = 1.f;
                const FVector2D TileLegacyUv = UvFromKm(TileKm);
                TileUVs.Set(TileInstance, 0, FVector2f(TileUv));
                TileUVs.Set(TileInstance, 1, FVector2f(float(TileLegacyUv.X), float(1.0 - TileLegacyUv.Y * TileLegacyDetailUvYScale)));
                TileUVs.Set(TileInstance, 2, FVector2f(float((TileKm.X - TileExtent.XMin) / (TileExtent.XMax - TileExtent.XMin)), float((TileExtent.YMax - TileKm.Y) / (TileExtent.YMax - TileExtent.YMin))));
                TileVertices.Add(TileInstance);
            }
        }
        for (int32 TileY = 0; TileY < TileCells; ++TileY)
        {
            for (int32 TileX = 0; TileX < TileCells; ++TileX)
            {
                const int32 TileA = TileY * TileSide + TileX;
                const FVertexInstanceID TileTriA[] = {TileVertices[TileA], TileVertices[TileA + TileSide], TileVertices[TileA + 1]};
                const FVertexInstanceID TileTriB[] = {TileVertices[TileA + 1], TileVertices[TileA + TileSide], TileVertices[TileA + TileSide + 1]};
                TileMesh.CreateTriangle(TileGroup, TileTriA);
                TileMesh.CreateTriangle(TileGroup, TileTriB);
            }
        }
        // Skirts close T-junctions when neighbours select different LODs. Their
        // bottom is below the overview, so the unbuilt neighbour never exposes a hole.
        TArray<int32> TileRim;
        for (int32 TileEdge = 0; TileEdge < TileCells; ++TileEdge) { TileRim.Add(TileEdge); }
        for (int32 TileEdge = 0; TileEdge < TileCells; ++TileEdge) { TileRim.Add(TileEdge * TileSide + TileCells); }
        for (int32 TileEdge = TileCells; TileEdge > 0; --TileEdge) { TileRim.Add(TileCells * TileSide + TileEdge); }
        for (int32 TileEdge = TileCells; TileEdge > 0; --TileEdge) { TileRim.Add(TileEdge * TileSide); }
        TArray<FVertexInstanceID> TileSkirt;
        for (const int32 TileEdge : TileRim)
        {
            const FVertexInstanceID TileTop = TileVertices[TileEdge];
            const FVertexID TileBottomVertex = TileMesh.CreateVertex();
            FVector3f TileBottomPosition = TilePositions[TileMesh.GetVertexInstanceVertex(TileTop)];
            TileBottomPosition.Z = -21.f;
            TilePositions[TileBottomVertex] = TileBottomPosition;
            const FVertexInstanceID TileBottom = TileMesh.CreateVertexInstance(TileBottomVertex);
            TileNormals[TileBottom] = FVector3f::UpVector;
            TileTangents[TileBottom] = FVector3f::ForwardVector;
            TileSigns[TileBottom] = 1.f;
            for (int32 TileChannel = 0; TileChannel < 3; ++TileChannel) { TileUVs.Set(TileBottom, TileChannel, TileUVs.Get(TileTop, TileChannel)); }
            TileSkirt.Add(TileBottom);
        }
        for (int32 TileEdge = 0; TileEdge < TileRim.Num(); ++TileEdge)
        {
            const int32 TileNext = (TileEdge + 1) % TileRim.Num();
            const FVertexInstanceID TileSideA[] = { TileVertices[TileRim[TileEdge]], TileVertices[TileRim[TileNext]], TileSkirt[TileEdge] };
            const FVertexInstanceID TileSideB[] = { TileVertices[TileRim[TileNext]], TileSkirt[TileNext], TileSkirt[TileEdge] };
            TileMesh.CreateTriangle(TileGroup, TileSideA);
            TileMesh.CreateTriangle(TileGroup, TileSideB);
        }
    }
    UStaticMesh* TileStatic = NewObject<UStaticMesh>(this, NAME_None, RF_Transient);
    TileStatic->GetStaticMaterials().Add(FStaticMaterial(TileMasterMaterial, TEXT("Map")));
    TileStatic->bSupportRayTracing = false;
    UStaticMesh::FBuildMeshDescriptionsParams TileParams;
    TileParams.bFastBuild = true; TileParams.bMarkPackageDirty = false; TileParams.bCommitMeshDescription = false;
    if (!TileStatic->BuildFromMeshDescriptions({&TileDescriptions[0], &TileDescriptions[1], &TileDescriptions[2]}, TileParams)) { return; }
    UStaticMeshComponent* TileComponent = NewObject<UStaticMeshComponent>(this);
    TileComponent->SetupAttachment(Root);
    TileComponent->SetStaticMesh(TileStatic);
    TileComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TileComponent->SetCastShadow(false);
    // Independent primitive bounds give normal renderer frustum/occlusion culling per tile.
    UMaterialInstanceDynamic* TileMid = UMaterialInstanceDynamic::Create(TileMasterMaterial, TileComponent);
    TileMid->SetScalarParameterValue(TEXT("TileReady"), 0.f);
    TileComponent->SetMaterial(0, TileMid);
    TileComponent->RegisterComponent();
    RuntimeTileMeshes[TileIndex] = TileComponent;
    TArray<FSoftObjectPath> TilePaths;
    for (const TCHAR* TileLayer : {TEXT("Color"), TEXT("DetailMask")})
    {
        const FString TileName = FString::Printf(TEXT("T_Tile_%d_%d_%s"), TileEntry.Key.X, TileEntry.Key.Y, TileLayer);
        TilePaths.Add(FSoftObjectPath(TEXT("/Game/Campaign1851/MapTiles/") + TileName + TEXT(".") + TileName));
    }
    if (bTileTexturesNow)
    {
        UTexture2D* TileColorNow = Cast<UTexture2D>(TilePaths[0].TryLoad());
        UTexture2D* TileMaskNow = Cast<UTexture2D>(TilePaths[1].TryLoad());
        if (TileColorNow && TileMaskNow)
        {
            TileMid->SetTextureParameterValue(TEXT("TileColor"), TileColorNow);
            TileMid->SetTextureParameterValue(TEXT("TileDetailMask"), TileMaskNow);
            TileMid->SetScalarParameterValue(TEXT("TileReady"), 1.f);
            TileColorNow->SetForceMipLevelsToBeResident(15.f);
            return;
        }
    }
    const TWeakObjectPtr<UStaticMeshComponent> TileWeakComponent(TileComponent);
    const TWeakObjectPtr<UMaterialInstanceDynamic> TileWeakMid(TileMid);
    RuntimeTileHandles[TileIndex] = UAssetManager::GetStreamableManager().RequestAsyncLoad(TilePaths, FStreamableDelegate::CreateLambda([TileWeakComponent, TileWeakMid, TilePaths]()
    {
        if (!TileWeakComponent.IsValid() || !TileWeakComponent->IsRegistered() || !TileWeakMid.IsValid()) { return; }
        UTexture2D* TileColor = Cast<UTexture2D>(TilePaths[0].ResolveObject());
        UTexture2D* TileMask = Cast<UTexture2D>(TilePaths[1].ResolveObject());
        if (TileColor && TileMask)
        {
            TileWeakMid->SetTextureParameterValue(TEXT("TileColor"), TileColor);
            TileWeakMid->SetTextureParameterValue(TEXT("TileDetailMask"), TileMask);
            TileWeakMid->SetScalarParameterValue(TEXT("TileReady"), 1.f);
        }
    }));
}
