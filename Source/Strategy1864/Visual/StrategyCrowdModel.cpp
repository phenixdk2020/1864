#include "StrategyCrowdModel.h"
#include "StrategyInfantryVisualComponent.h"
#include "StrategyUniformAppearanceComponent.h"
#include "../Player/StrategyBattlePerformance.h"
#include "../Units/StrategyCompanyUnit.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "CoreGlobals.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshResources.h"
#include "UObject/Package.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#endif

namespace
{
    // Full silhouette for near VAT figures; no runtime mesh construction.
    constexpr int32 CrowdLOD = 0; // near figures need the full silhouette
    const TCHAR* CrowdMaterialPath = TEXT("/Game/Battle/Materials/M_CrowdVAT_All.M_CrowdVAT_All");

    UTexture* FirstTexture(const UMaterialInterface* Material)
    {
        TArray<UTexture*> Textures;
        if (Material)
        {
            Material->GetUsedTextures(Textures);
        }
        for (UTexture* T : Textures)
        {
            if (T)
            {
                return T;
            }
        }
        return nullptr;
    }

    /** A skinning matrix as three texels: the columns of the 4x4 (row vectors: p' = (p,1) . column). */
    void WriteMatrix(TArray<FVector4f>& Texels, int32 Index, const FMatrix44f& M)
    {
        for (int32 c = 0; c < 3; ++c)
        {
            Texels[Index + c] = FVector4f(M.M[0][c], M.M[1][c], M.M[2][c], M.M[3][c]);
        }
    }
}

UMaterialInterface* UStrategyCrowdModel::GetMaterial(int32 Index) const
{
    return Materials.IsValidIndex(Index) ? Materials[Index].Get() : nullptr;
}

FString UStrategyCrowdModel::AssetPath(const USkeletalMesh* Soldier, const UStaticMesh* Rifle)
{
    const FString CrowdName = TEXT("VAT_") + Soldier->GetName() + TEXT("_") + (Rifle ? Rifle->GetName() : TEXT("Unarmed"));
    return TEXT("/Game/Battle/VAT/") + CrowdName + TEXT(".") + CrowdName;
}

void UStrategyCrowdModel::BindMaterials()
{
    Materials.Reset();
    UMaterialInterface* CrowdParent = LoadObject<UMaterialInterface>(nullptr, CrowdMaterialPath);
    if (!CrowdParent || !Mesh || !BoneTexture) return;
    for (int32 CrowdSlot = 0; CrowdSlot < DiffuseTextures.Num(); ++CrowdSlot)
    {
        UMaterialInstanceDynamic* CrowdMID = UMaterialInstanceDynamic::Create(CrowdParent, this);
        CrowdMID->SetTextureParameterValue(TEXT("BoneTex"), BoneTexture);
        if (DiffuseTextures[CrowdSlot]) CrowdMID->SetTextureParameterValue(TEXT("Diffuse"), DiffuseTextures[CrowdSlot]);
        Materials.Add(CrowdMID);
    }
}

UStrategyCrowdModel* UStrategyCrowdModel::BakeAsset(UWorld* World, USkeletalMesh* Soldier, UStaticMesh* Rifle, const TArray<UAnimSequence*>& Animations)
{
#if WITH_EDITOR
    if (!World || !Soldier) return nullptr;
    const FString CrowdPath = AssetPath(Soldier, Rifle);
    FString CrowdPackageName, CrowdObjectName;
    CrowdPath.Split(TEXT("."), &CrowdPackageName, &CrowdObjectName);
    UStrategyCrowdModel* CrowdAsset = LoadObject<UStrategyCrowdModel>(nullptr, *CrowdPath);
    UPackage* CrowdPackage = CrowdAsset ? CrowdAsset->GetOutermost() : CreatePackage(*CrowdPackageName);
    const bool bCrowdNewAsset = !CrowdAsset;
    if (!CrowdAsset) CrowdAsset = NewObject<UStrategyCrowdModel>(CrowdPackage, FName(*CrowdObjectName), RF_Public | RF_Standalone);
    FStrategyCrowdRifleGrip CrowdGrip;
    CrowdGrip.RightHand = TEXT("RightHand");
    CrowdGrip.LeftHand = TEXT("LeftHand");
    CrowdGrip.HandTransform = FTransform(FQuat(FRotator(-8.f, 90.f, 0.f)) * FQuat(FVector::XAxisVector, UE_PI) * FQuat(FVector::YAxisVector, UE_PI), FVector(-4.f, 0.f, 10.f));
    if (!CrowdAsset->Bake(World, Soldier, Rifle, Animations, CrowdGrip)) return nullptr;
    // Subobjects travel in the model's package; no transient MID may enter the saved mesh.
    for (FStaticMaterial& CrowdSlot : CrowdAsset->Mesh->GetStaticMaterials())
        CrowdSlot.MaterialInterface = LoadObject<UMaterialInterface>(nullptr, CrowdMaterialPath);
    CrowdAsset->BakeVersion = 3;
    CrowdPackage->MarkPackageDirty();
    if (bCrowdNewAsset) FAssetRegistryModule::AssetCreated(CrowdAsset);
    return CrowdAsset;
#else
    return nullptr;
#endif
}

UStrategyCrowdModel* UStrategyCrowdModel::Find(UWorld* World, USkeletalMesh* Soldier, UStaticMesh* Rifle, const TArray<UAnimSequence*>& Clips, const FStrategyCrowdRifleGrip& Grip)
{
    if (!World || !Soldier)
    {
        return nullptr;
    }
    UStrategyCrowdSubsystem* Sub = World->GetSubsystem<UStrategyCrowdSubsystem>();
    if (!Sub)
    {
        return nullptr;
    }
    const FString Key = Soldier->GetPathName() + TEXT("|") + (Rifle ? Rifle->GetPathName() : FString(TEXT("-")));
    if (const TObjectPtr<UStrategyCrowdModel>* Found = Sub->Models.Find(Key))
    {
        if (Clips.ContainsByPredicate([Found](UAnimSequence* CrowdClip) { return CrowdClip && !(*Found)->Clips.Contains(CrowdClip); }))
        {
            const FString CrowdMissingKey = Key + TEXT("|clips");
            if (!Sub->Failed.Contains(CrowdMissingKey))
                UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-CROWD: cached asset lacks requested clips: %s; skeletal fallback"), *Key);
            Sub->Failed.Add(CrowdMissingKey);
            return nullptr;
        }
        return Found->Get();
    }
    if (Sub->Failed.Contains(Key))
    {
        return nullptr;
    }
    UStrategyCrowdModel* Model = LoadObject<UStrategyCrowdModel>(nullptr, *AssetPath(Soldier, Rifle));
    if (!Model || Model->BakeVersion != 3 || !Model->Mesh || !Model->BoneTexture ||
        Clips.ContainsByPredicate([Model](UAnimSequence* CrowdClip) { return CrowdClip && !Model->Clips.Contains(CrowdClip); }))
    {
        Sub->Failed.Add(Key);
        UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-CROWD: missing/incomplete baked asset %s; skeletal fallback. Run Content/Python/bake_all_vat.py."), *Key);
        return nullptr;
    }
    Model->BindMaterials();
    if (Model->Materials.IsEmpty())
    {
        Sub->Failed.Add(Key);
        UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-CROWD: VAT material missing for %s; skeletal fallback"), *Key);
        return nullptr;
    }
    Sub->Models.Add(Key, Model);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-CROWD: loaded %s: %d clips"), *Key, Model->Clips.Num());
    return Model;
}

bool UStrategyCrowdModel::MakeCustomData(const UAnimSequence* Clip, float StartTime, float PlayRate, bool bLoop, float Out[CustomDataFloats], float Now, float HeldPosition) const
{
    FMemory::Memzero(Out, sizeof(float) * CustomDataFloats);
    const FStrategyCrowdClip* C = Clip ? Clips.Find(const_cast<UAnimSequence*>(Clip)) : nullptr;
    if (!C)
    {
        return false;
    }
    // Existing VAT shader interpolates rows. A very slow, rebased non-looping clock
    // preserves an arbitrary frozen phase without requiring a material asset rebuild.
    if (PlayRate <= 0.f && HeldPosition > 0.f)
    {
        StartTime = Now - FMath::Clamp(HeldPosition, 0.f, C->Length) / 0.001f;
        PlayRate = 0.001f;
        bLoop = false;
    }
    if (PlayRate <= 0.f)
    {
        Out[0] = float(C->StartRow);
        Out[1] = 1.f;
        Out[2] = Now;
        Out[3] = 0.f;
        return true;
    }
    const float Fps = (C->Frames - 1) / FMath::Max(0.001f, C->Length) * FMath::Max(0.0f, PlayRate);
    Out[0] = float(C->StartRow);
    Out[1] = float(bLoop ? FMath::Max(1, C->Frames - 1) : C->Frames); // endpoint is not an extra looping frame
    Out[2] = StartTime;
    Out[3] = bLoop ? Fps : -FMath::Max(Fps, 0.0001f);
    return true;
}

void UStrategyCrowdSubsystem::RemoveAuxiliary(USkeletalMeshComponent* Figure)
{
    if (!Figure || !Figure->GetWorld()) return;
    UStrategyCrowdSubsystem* CrowdSub = Figure->GetWorld()->GetSubsystem<UStrategyCrowdSubsystem>();
    if (!CrowdSub) return;
    if (const FStrategyCrowdAuxFigure* CrowdAux = CrowdSub->AuxiliaryFigures.Find(Figure))
        if (CrowdAux->NearBlend > 0.f) CrowdSub->NearGeometryCount = FMath::Max(0, CrowdSub->NearGeometryCount - 1);
    CrowdSub->AuxiliaryFigures.Remove(Figure);
    FTransform CrowdHidden = FTransform::Identity;
    CrowdHidden.SetScale3D(FVector::ZeroVector);
    for (auto& CrowdGroupPair : CrowdSub->AuxiliaryGroups)
    {
        FStrategyCrowdAuxGroup& CrowdGroup = CrowdGroupPair.Value;
        if (const int32* CrowdSlot = CrowdGroup.Slots.Find(Figure))
        {
            if (IsValid(CrowdGroup.Instances)) CrowdGroup.Instances->UpdateInstanceTransform(*CrowdSlot, CrowdHidden, true, true);
            CrowdGroup.FreeSlots.Add(*CrowdSlot);
            CrowdGroup.Slots.Remove(Figure);
        }
    }
}

int32 UStrategyCrowdSubsystem::CountFallen(UWorld* World)
{
    if (!World) return 0;
    int32 CrowdBodies = 0;
    for (const TWeakObjectPtr<AStrategyUnit>& CrowdEntry : Strategy1864Performance::VisualUnits(World))
        if (const AStrategyCompanyUnit* CrowdCompany = Cast<AStrategyCompanyUnit>(CrowdEntry.Get()))
            if (CrowdCompany->InfantryVisualComponent) CrowdBodies += CrowdCompany->InfantryVisualComponent->GetCorpseCount();
    if (UStrategyCrowdSubsystem* CrowdSub = World->GetSubsystem<UStrategyCrowdSubsystem>())
        for (const auto& CrowdPair : CrowdSub->AuxiliaryFigures)
            if (CrowdPair.Key.IsValid() && !CrowdPair.Value.bLiving) ++CrowdBodies;
    return CrowdBodies;
}

void UStrategyCrowdSubsystem::SweepAuxiliary()
{
    FTransform CrowdHidden = FTransform::Identity;
    CrowdHidden.SetScale3D(FVector::ZeroVector);
    for (auto CrowdGroupIterator = AuxiliaryGroups.CreateIterator(); CrowdGroupIterator; ++CrowdGroupIterator)
    {
        FStrategyCrowdAuxGroup& CrowdGroup = CrowdGroupIterator.Value();
        if (!IsValid(CrowdGroup.Instances) || !CrowdGroup.Instances->IsRegistered())
        {
            CrowdGroupIterator.RemoveCurrent();
            continue;
        }
        for (auto CrowdSlotIterator = CrowdGroup.Slots.CreateIterator(); CrowdSlotIterator; ++CrowdSlotIterator)
            if (!CrowdSlotIterator.Key().IsValid() || !CrowdSlotIterator.Key()->IsRegistered())
            {
                CrowdGroup.Instances->UpdateInstanceTransform(CrowdSlotIterator.Value(), CrowdHidden, true, true);
                CrowdGroup.FreeSlots.Add(CrowdSlotIterator.Value());
                CrowdSlotIterator.RemoveCurrent();
            }
    }
}

bool UStrategyCrowdSubsystem::DrawAuxiliary(USkeletalMeshComponent* Figure, USkeletalMesh* SourceMesh,
    UAnimSequence* Clip, float Position, bool bLiving, float Opacity)
{
    if (!Figure || !SourceMesh || !Clip || !Figure->GetWorld()) return false;
    int32 CrowdEnabled = 1;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Crowd="), CrowdEnabled);
    UWorld* CrowdWorld = Figure->GetWorld();
    UStrategyCrowdSubsystem* CrowdSub = CrowdWorld->GetSubsystem<UStrategyCrowdSubsystem>();
    if (!CrowdSub) return false;
    UStrategyCrowdModel* CrowdModel = CrowdEnabled ? UStrategyCrowdModel::Find(CrowdWorld, SourceMesh, nullptr, {Clip}, FStrategyCrowdRifleGrip()) : nullptr;
    if (!CrowdModel)
    {
        RemoveAuxiliary(Figure);
        CrowdSub->AuxiliaryFigures.FindOrAdd(Figure).bLiving = bLiving;
        if (Figure->GetSkeletalMeshAsset() != SourceMesh) Figure->SetSkeletalMeshAsset(SourceMesh);
        Figure->SetVisibility(true);
        Figure->PlayAnimation(Clip, false);
        Figure->SetPosition(Position, false);
        Figure->bPauseAnims = true;
        Figure->TickAnimation(0.f, false);
        Figure->RefreshBoneTransforms();
        for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < Figure->GetNumMaterials(); ++CrowdMaterialIndex)
            if (UMaterialInstanceDynamic* CrowdMID = Cast<UMaterialInstanceDynamic>(Figure->GetMaterial(CrowdMaterialIndex)))
                CrowdMID->SetScalarParameterValue(TEXT("CrowdOpacity"), 1.f);
        return false;
    }
    const FString CrowdGroupKey = Figure->GetOwner()->GetPathName() + TEXT("|") + CrowdModel->GetPathName() + (bLiving ? TEXT("|living") : TEXT("|fallen"));
    FStrategyCrowdAuxGroup& CrowdGroup = CrowdSub->AuxiliaryGroups.FindOrAdd(CrowdGroupKey);
    if (!IsValid(CrowdGroup.Instances))
    {
        CrowdGroup = FStrategyCrowdAuxGroup();
        CrowdGroup.Instances = NewObject<UInstancedStaticMeshComponent>(Figure->GetOwner(), NAME_None, RF_Transient);
        CrowdGroup.Instances->SetStaticMesh(CrowdModel->GetMesh());
        CrowdGroup.Instances->SetNumCustomDataFloats(UStrategyCrowdModel::CustomDataFloats);
        CrowdGroup.Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CrowdGroup.Instances->SetCastShadow(false);
        CrowdGroup.Instances->bVisibleInRayTracing = false;
        for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < CrowdModel->GetNumMaterials(); ++CrowdMaterialIndex)
            CrowdGroup.Instances->SetMaterial(CrowdMaterialIndex, CrowdModel->GetMaterial(CrowdMaterialIndex));
        CrowdGroup.Instances->RegisterComponent(); // unattached, world transforms also keep fallen at impact
    }
    // A casualty moves between groups once; survivor slots are never reordered.
    for (auto& CrowdOtherPair : CrowdSub->AuxiliaryGroups)
    {
        if (CrowdOtherPair.Key == CrowdGroupKey) continue;
        FStrategyCrowdAuxGroup& CrowdOther = CrowdOtherPair.Value;
        if (const int32* CrowdOldSlot = CrowdOther.Slots.Find(Figure))
        {
            FTransform CrowdHidden = FTransform::Identity;
            CrowdHidden.SetScale3D(FVector::ZeroVector);
            if (IsValid(CrowdOther.Instances)) CrowdOther.Instances->UpdateInstanceTransform(*CrowdOldSlot, CrowdHidden, true, true);
            CrowdOther.FreeSlots.Add(*CrowdOldSlot);
            CrowdOther.Slots.Remove(Figure);
        }
    }
    FStrategyCrowdAuxFigure& CrowdAux = CrowdSub->AuxiliaryFigures.FindOrAdd(Figure);
    CrowdAux.bLiving = bLiving;
    CrowdAux.bBaked = true;
    if (APlayerController* CrowdPC = CrowdWorld->GetFirstPlayerController())
        if (CrowdPC->PlayerCameraManager)
            UStrategyInfantryVisualComponent::SelectNearestFigures(CrowdWorld, CrowdPC->PlayerCameraManager->GetCameraLocation());
    const int32 CrowdBudget = CrowdSub->NearBudget;
    bool CrowdTargetNear = bLiving && CrowdSub->NearFigures.Contains(Figure);
    const bool CrowdWasNear = CrowdAux.NearBlend > 0.f;
    if (!CrowdWasNear && CrowdSub->NearGeometryCount >= FMath::Max(0, CrowdBudget)) CrowdTargetNear = false;
    if (CrowdAux.BlendFrame != GFrameCounter)
    {
        CrowdAux.BlendFrame = GFrameCounter;
        CrowdAux.NearBlend = FMath::FInterpConstantTo(CrowdAux.NearBlend, CrowdTargetNear ? 1.f : 0.f, CrowdWorld->GetDeltaSeconds(), 5.f);
    }
    const bool CrowdIsNear = CrowdAux.NearBlend > 0.f;
    if (CrowdWasNear != CrowdIsNear) CrowdSub->NearGeometryCount += CrowdIsNear ? 1 : -1;
    if (CrowdIsNear)
    {
        if (!Figure->GetSkeletalMeshAsset()) Figure->SetSkeletalMeshAsset(SourceMesh);
        Figure->PlayAnimation(Clip, false);
        Figure->SetPosition(Position, false);
        Figure->bPauseAnims = true;
        Figure->TickAnimation(0.f, false);
        Figure->RefreshBoneTransforms();
        for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < Figure->GetNumMaterials(); ++CrowdMaterialIndex)
        {
            UMaterialInstanceDynamic* CrowdMID = Cast<UMaterialInstanceDynamic>(Figure->GetMaterial(CrowdMaterialIndex));
            if (!CrowdMID) CrowdMID = Figure->CreateAndSetMaterialInstanceDynamic(CrowdMaterialIndex);
            if (CrowdMID) CrowdMID->SetScalarParameterValue(TEXT("CrowdOpacity"), CrowdAux.NearBlend);
        }
    }
    else if (Figure->GetSkeletalMeshAsset()) Figure->SetSkeletalMeshAsset(nullptr);
    Figure->SetVisibility(CrowdIsNear);
    Figure->SetComponentTickEnabled(false);
    float CrowdData[UStrategyCrowdModel::CustomDataFloats];
    CrowdModel->MakeCustomData(Clip, CrowdWorld->GetTimeSeconds(), 0.f, false, CrowdData, CrowdWorld->GetTimeSeconds(), Position);
    if (UStrategyUniformAppearanceComponent* CrowdAppearance = Figure->GetOwner()->FindComponentByClass<UStrategyUniformAppearanceComponent>())
    {
        if (CrowdIsNear) CrowdAppearance->ApplyAppearanceToMesh(Figure);
        const FStrategyUniformColors CrowdColours = CrowdAppearance->GetResolvedColors();
        const FLinearColor CrowdParts[] = {CrowdColours.Coat, CrowdColours.Trousers, CrowdColours.HeadgearDetail};
        for (int32 CrowdPart = 0; CrowdPart < 3; ++CrowdPart)
        {
            CrowdData[4 + CrowdPart * 3] = CrowdParts[CrowdPart].R;
            CrowdData[5 + CrowdPart * 3] = CrowdParts[CrowdPart].G;
            CrowdData[6 + CrowdPart * 3] = CrowdParts[CrowdPart].B;
        }
        const bool CrowdPaletteAllowed = CrowdAppearance->CanOverrideHistoricalPalette();
        CrowdData[13] = CrowdPaletteAllowed && CrowdAppearance->Overrides.bOverrideCoat ? 1.f : 0.f;
        CrowdData[14] = CrowdPaletteAllowed && CrowdAppearance->Overrides.bOverrideTrousers ? 1.f : 0.f;
        CrowdData[15] = CrowdPaletteAllowed && CrowdAppearance->Overrides.bOverrideHeadgearDetail ? 1.f : 0.f;
    }
    CrowdData[16] = (1.f - CrowdAux.NearBlend) * Opacity;
    const int32* CrowdExistingSlot = CrowdGroup.Slots.Find(Figure);
    const int32 CrowdSlot = CrowdExistingSlot ? *CrowdExistingSlot : !CrowdGroup.FreeSlots.IsEmpty() ? CrowdGroup.FreeSlots.Pop(EAllowShrinking::No) : CrowdGroup.Instances->AddInstance(Figure->GetComponentTransform(), true);
    if (!CrowdExistingSlot) CrowdGroup.Slots.Add(Figure, CrowdSlot);
    FTransform CrowdInstancePose = Figure->GetComponentTransform();
    if (CrowdAux.NearBlend >= 1.f || Opacity <= 0.f) CrowdInstancePose.SetScale3D(FVector::ZeroVector);
    CrowdGroup.Instances->UpdateInstanceTransform(CrowdSlot, CrowdInstancePose, true, false);
    CrowdGroup.Instances->SetCustomData(CrowdSlot, TArrayView<const float>(CrowdData, UStrategyCrowdModel::CustomDataFloats), true);
    return true;
}

bool UStrategyCrowdModel::RiflePose(const UAnimSequence* Clip, float Position, bool bLoop, FTransform& Out) const
{
    const FStrategyCrowdClip* CrowdClip = Clip ? Clips.Find(const_cast<UAnimSequence*>(Clip)) : nullptr;
    if (!CrowdClip || CrowdClip->RiflePoses.IsEmpty()) return false;
    const float CrowdPosition = bLoop && CrowdClip->Length > 0.f ? FMath::Fmod(FMath::Max(0.f, Position), CrowdClip->Length) : FMath::Clamp(Position, 0.f, CrowdClip->Length);
    const float CrowdFrame = CrowdPosition / FMath::Max(.001f, CrowdClip->Length) * (CrowdClip->Frames - 1);
    const int32 CrowdFirst = FMath::Clamp(FMath::FloorToInt(CrowdFrame), 0, CrowdClip->RiflePoses.Num() - 1);
    const int32 CrowdNext = FMath::Min(CrowdFirst + 1, CrowdClip->RiflePoses.Num() - 1);
    Out.Blend(CrowdClip->RiflePoses[CrowdFirst], CrowdClip->RiflePoses[CrowdNext], CrowdFrame - CrowdFirst);
    return true;
}

bool UStrategyCrowdModel::Bake(UWorld* World, USkeletalMesh* Soldier, UStaticMesh* Rifle, const TArray<UAnimSequence*>& InClips, const FStrategyCrowdRifleGrip& Grip)
{
    BakeVersion = 0;
    Clips.Reset();
    Materials.Reset();
    DiffuseTextures.Reset();
    UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, CrowdMaterialPath);
    FSkeletalMeshRenderData* RenderData = Soldier->GetResourceForRendering();
    if (!Parent || !RenderData || RenderData->LODRenderData.Num() == 0)
    {
        return false;
    }
    const int32 LODIndex = FMath::Clamp(CrowdLOD, 0, RenderData->LODRenderData.Num() - 1);
    const FSkeletalMeshLODRenderData& LOD = RenderData->LODRenderData[LODIndex];
    const FPositionVertexBuffer& Positions = LOD.StaticVertexBuffers.PositionVertexBuffer;
    const FStaticMeshVertexBuffer& Vertices = LOD.StaticVertexBuffers.StaticMeshVertexBuffer;
    const FSkinWeightVertexBuffer* Weights = LOD.GetSkinWeightVertexBuffer();
    if (!Positions.GetVertexData() || !Vertices.GetTangentData() || !Vertices.GetTexCoordData() || !Weights ||
        !Weights->GetDataVertexBuffer() || !Weights->GetDataVertexBuffer()->GetWeightData())
    {
        UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-CROWD: %s LOD %d has no CPU data"), *Soldier->GetName(), LODIndex);
        return false;
    }
    TArray<uint32> Indices;
    LOD.MultiSizeIndexContainer.GetIndexBuffer(Indices);
    const FReferenceSkeleton& RefSkeleton = Soldier->GetRefSkeleton();
    const int32 NumBones = RefSkeleton.GetNum();
    const int32 RifleBone = NumBones;
    if (Indices.Num() == 0 || NumBones == 0)
    {
        return false;
    }

    // ---- the mesh: body vertices with their four strongest bones, the rifle on the virtual bone
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description);
    Attributes.Register();
    TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
    TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
    TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
    TVertexInstanceAttributesRef<float> Signs = Attributes.GetVertexInstanceBinormalSigns();
    TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
    UVs.SetNumChannels(5);
    TVertexInstanceAttributesRef<FVector4f> CrowdMasks = Attributes.GetVertexInstanceColors();
    const FBoxSphereBounds CrowdBounds = Soldier->GetImportedBounds();
    const float CrowdLow = CrowdBounds.Origin.Z - CrowdBounds.BoxExtent.Z;
    const float CrowdHeight = FMath::Max(1.f, 2.f * float(CrowdBounds.BoxExtent.Z));
    TPolygonGroupAttributesRef<FName> SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();

    auto AddVertex = [&](const FVector3f& P, const FVector3f& N, const FVector3f& T, const FVector2f& UV, const int32 Bone[4], const float W[4])
    {
        const FVertexID V = Description.CreateVertex();
        VertexPositions[V] = P;
        const FVertexInstanceID I = Description.CreateVertexInstance(V);
        Normals[I] = N;
        Tangents[I] = T;
        Signs[I] = 1.0f;
        const float CrowdH = (P.Z - CrowdLow) / CrowdHeight;
        CrowdMasks[I] = Bone[0] == RifleBone ? FVector4f(0, 0, 0, 1) :
            FVector4f(CrowdH >= .48f && CrowdH < .82f ? 1.f : 0.f,
                CrowdH >= .12f && CrowdH < .48f ? 1.f : 0.f, CrowdH >= .88f ? 1.f : 0.f, 1.f);
        UVs.Set(I, 0, UV);
        UVs.Set(I, 1, FVector2f(float(Bone[0]), float(Bone[1])));
        UVs.Set(I, 2, FVector2f(float(Bone[2]), float(Bone[3])));
        UVs.Set(I, 3, FVector2f(W[0], W[1]));
        UVs.Set(I, 4, FVector2f(W[2], W[3]));
        return I;
    };
    TMap<int32, FPolygonGroupID> Groups;          // key: body material index, or 1000 + rifle material index
    TArray<UMaterialInterface*> GroupSources;     // the original material of each group (for its texture)
    auto GroupFor = [&](int32 Key, UMaterialInterface* Source)
    {
        if (const FPolygonGroupID* Found = Groups.Find(Key))
        {
            return *Found;
        }
        const FPolygonGroupID G = Description.CreatePolygonGroup();
        SlotNames[G] = FName(*FString::Printf(TEXT("Crowd_%d"), GroupSources.Num()));
        GroupSources.Add(Source);
        Groups.Add(Key, G);
        return G;
    };

    const uint32 MaxInfluences = Weights->GetMaxBoneInfluences();
    TArray<FVertexInstanceID> BodyInstances;
    BodyInstances.SetNum(int32(LOD.GetNumVertices()));
    const TArray<FSkeletalMaterial>& SoldierMaterials = Soldier->GetMaterials();
    for (const FSkelMeshRenderSection& Section : LOD.RenderSections)
    {
        for (uint32 v = Section.BaseVertexIndex; v < Section.BaseVertexIndex + Section.NumVertices && v < uint32(BodyInstances.Num()); ++v)
        {
            TArray<TPair<int32, float>, TInlineAllocator<12>> Influences;
            for (uint32 k = 0; k < MaxInfluences; ++k)
            {
                const float W = Weights->GetBoneWeight(v, k) / 65535.0f;
                const uint32 Local = Weights->GetBoneIndex(v, k);
                if (W > 0.0f && Section.BoneMap.IsValidIndex(int32(Local)))
                {
                    Influences.Add(TPair<int32, float>(int32(Section.BoneMap[Local]), W));
                }
            }
            Influences.Sort([](const TPair<int32, float>& A, const TPair<int32, float>& B) { return A.Value > B.Value; });
            int32 Bone[4] = { 0, 0, 0, 0 };
            float W[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float Sum = 0.0f;
            for (int32 k = 0; k < 4 && k < Influences.Num(); ++k)
            {
                Bone[k] = Influences[k].Key;
                W[k] = Influences[k].Value;
                Sum += W[k];
            }
            if (Sum <= 0.0f)
            {
                W[0] = 1.0f;
                Sum = 1.0f;
            }
            for (float& X : W) { X /= Sum; }
            const FVector4f N = Vertices.VertexTangentZ(v);
            const FVector4f T = Vertices.VertexTangentX(v);
            BodyInstances[v] = AddVertex(Positions.VertexPosition(v), FVector3f(N.X, N.Y, N.Z), FVector3f(T.X, T.Y, T.Z), Vertices.GetVertexUV(v, 0), Bone, W);
        }
        UMaterialInterface* Source = SoldierMaterials.IsValidIndex(Section.MaterialIndex) ? SoldierMaterials[Section.MaterialIndex].MaterialInterface.Get() : nullptr;
        const FPolygonGroupID Group = GroupFor(Section.MaterialIndex, Source);
        for (uint32 t = 0; t < Section.NumTriangles; ++t)
        {
            const uint32 I = Section.BaseIndex + t * 3;
            if (I + 2 >= uint32(Indices.Num()))
            {
                break;
            }
            const FVertexInstanceID A = BodyInstances[Indices[I]], B = BodyInstances[Indices[I + 1]], C = BodyInstances[Indices[I + 2]];
            if (A != INDEX_NONE && B != INDEX_NONE && C != INDEX_NONE)
            {
                Description.CreateTriangle(Group, { A, B, C });
            }
        }
    }

    FBox RifleBox(ForceInit);
    bool bCrowdRifleAdded = false;
    if (Rifle && Rifle->GetRenderData() && Rifle->GetRenderData()->LODResources.Num() > 0)
    {
        const FStaticMeshLODResources& R = Rifle->GetRenderData()->LODResources[0];
        TArray<uint32> RifleIndices;
        R.IndexBuffer.GetCopy(RifleIndices);
        if (R.VertexBuffers.PositionVertexBuffer.GetVertexData() && R.VertexBuffers.StaticMeshVertexBuffer.GetTangentData() &&
            R.VertexBuffers.StaticMeshVertexBuffer.GetTexCoordData() && RifleIndices.Num() > 0)
        {
            RifleBox = Rifle->GetBoundingBox();
            bCrowdRifleAdded = true;
            TArray<FVertexInstanceID> RifleInstances;
            const int32 Bone[4] = { RifleBone, 0, 0, 0 };
            const float W[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
            for (uint32 v = 0; v < R.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++v)
            {
                const FVector4f N = R.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(v);
                const FVector4f T = R.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(v);
                RifleInstances.Add(AddVertex(R.VertexBuffers.PositionVertexBuffer.VertexPosition(v), FVector3f(N.X, N.Y, N.Z), FVector3f(T.X, T.Y, T.Z),
                    R.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(v, 0), Bone, W));
            }
            const TArray<FStaticMaterial>& RifleMaterials = Rifle->GetStaticMaterials();
            for (const FStaticMeshSection& Section : R.Sections)
            {
                UMaterialInterface* Source = RifleMaterials.IsValidIndex(Section.MaterialIndex) ? RifleMaterials[Section.MaterialIndex].MaterialInterface.Get() : nullptr;
                const FPolygonGroupID Group = GroupFor(1000 + Section.MaterialIndex, Source);
                for (uint32 t = 0; t < Section.NumTriangles; ++t)
                {
                    const uint32 I = Section.FirstIndex + t * 3;
                    if (I + 2 < uint32(RifleIndices.Num()))
                    {
                        Description.CreateTriangle(Group, { RifleInstances[RifleIndices[I]], RifleInstances[RifleIndices[I + 1]], RifleInstances[RifleIndices[I + 2]] });
                    }
                }
            }
        }
    }

    if (Rifle && !bCrowdRifleAdded)
    {
        UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-CROWD: rifle CPU geometry missing: %s"), *Rifle->GetPathName());
        return false;
    }
    // ---- the animations: every clip at 15 frames a second, a row of skinning matrices a frame
    TArray<UAnimSequence*> Unique;
    for (UAnimSequence* Clip : InClips)
    {
        if (Clip && !Unique.Contains(Clip))
        {
            Unique.Add(Clip);
        }
    }
    int32 Rows = 0;
    for (UAnimSequence* Clip : Unique)
    {
        const float Length = Clip->GetPlayLength();
        FStrategyCrowdClip C;
        C.StartRow = Rows;
        C.Length = Length;
        C.Frames = FMath::Max(1, FMath::CeilToInt(Length * FramesPerSecond) + 1);
        Clips.Add(Clip, C);
        Rows += C.Frames;
    }
    if (Rows == 0 || Rows > 16384)
    {
        return false;
    }
    const int32 Width = (NumBones + 1) * 3;
    TArray<FVector4f> Texels;
    Texels.SetNumZeroed(Width * Rows);

    USkeletalMeshComponent* Poser = NewObject<USkeletalMeshComponent>(this, TEXT("CrowdPoser"), RF_Transient);
    Poser->SetSkeletalMeshAsset(Soldier);
    Poser->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Poser->bEnableUpdateRateOptimizations = false;
    Poser->SetVisibility(false);
    Poser->SetComponentTickEnabled(false);
    Poser->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Poser->RegisterComponentWithWorld(World);
    const TArray<FMatrix44f>& RefInverse = Soldier->GetRefBasesInvMatrix();
    const int32 RightHand = RefSkeleton.FindBoneIndex(Grip.RightHand), LeftHand = RefSkeleton.FindBoneIndex(Grip.LeftHand);
    if (Rifle && RightHand == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-CROWD: missing rifle hand bone in %s"), *Soldier->GetPathName());
        Poser->DestroyComponent();
        return false;
    }
    bool bMoved = false;
    FVector FirstHand = FVector::ZeroVector;
    for (UAnimSequence* Clip : Unique)
    {
        FStrategyCrowdClip& C = Clips[Clip];
        Poser->PlayAnimation(Clip, false);
        for (int32 f = 0; f < C.Frames; ++f)
        {
            Poser->SetPosition(C.Length * f / FMath::Max(1, C.Frames - 1), false);
            Poser->TickAnimation(0.0f, false);
            Poser->RefreshBoneTransforms();
            const TArray<FTransform>& Space = Poser->GetComponentSpaceTransforms();
            if (Space.Num() < NumBones)
            {
                Poser->DestroyComponent();
                return false;
            }
            const int32 Row = (C.StartRow + f) * Width;
            for (int32 b = 0; b < NumBones; ++b)
            {
                const FMatrix44f Skin = (RefInverse.IsValidIndex(b) ? RefInverse[b] : FMatrix44f::Identity) * FMatrix44f(Space[b].ToMatrixWithScale());
                WriteMatrix(Texels, Row + b * 3, Skin);
            }
            // The rifle: from the right hand towards the left (as the visual component aligns it).
            FTransform RifleTransform = FTransform::Identity;
            if (RightHand != INDEX_NONE)
            {
                const FVector Right = Space[RightHand].GetLocation();
                const FVector Left = LeftHand != INDEX_NONE ? Space[LeftHand].GetLocation() : Right;
                FVector Dir = Left - Right;
                if (Dir.Size() < 12.0f || !RifleBox.IsValid)
                {
                    RifleTransform = Grip.HandTransform * Space[RightHand];
                }
                else
                {
                    Dir.Normalize();
                    const FVector Barrel = Grip.bBarrelAlongNegativeX ? -Dir : Dir;
                    const FRotator Rotation = FRotationMatrix::MakeFromXZ(Barrel, FVector::UpVector).Rotator();
                    const float ButtX = Grip.bBarrelAlongNegativeX ? RifleBox.Max.X : RifleBox.Min.X;
                    const float GripX = ButtX + (RifleBox.Max.X - RifleBox.Min.X) * Grip.GripFraction * (Grip.bBarrelAlongNegativeX ? -1.0f : 1.0f);
                    RifleTransform = FTransform(Rotation, Right - Rotation.RotateVector(FVector(GripX, 0.0f, 0.0f)));
                }
                if (f == 0 && C.StartRow == 0)
                {
                    FirstHand = Right;
                }
                else if (!bMoved && FVector::Dist(FirstHand, Right) > 1.0f)
                {
                    bMoved = true;
                }
            }
            WriteMatrix(Texels, Row + RifleBone * 3, FMatrix44f(RifleTransform.ToMatrixWithScale()));
            C.RiflePoses.Add(RifleTransform);
        }
    }
    Poser->DestroyComponent();
    if (!bMoved)
    {
        UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-CROWD: the baked poses do not move (the right hand stays put)"));
    }

    BoneTexture = UTexture2D::CreateTransient(Width, Rows, PF_A32B32G32R32F, TEXT("CrowdBones"),
        TConstArrayView64<uint8>(reinterpret_cast<const uint8*>(Texels.GetData()), int64(Texels.Num()) * int64(sizeof(FVector4f))));
    if (!BoneTexture)
    {
        return false;
    }
    BoneTexture->Filter = TF_Nearest;
    BoneTexture->SRGB = false;
    BoneTexture->AddressX = TA_Clamp;
    BoneTexture->AddressY = TA_Clamp;
    BoneTexture->NeverStream = true;
#if WITH_EDITOR
    if (!HasAnyFlags(RF_Transient))
    {
        const FName CrowdTextureName = MakeUniqueObjectName(this, UTexture2D::StaticClass(), TEXT("BoneTexture"));
        if (!BoneTexture->Rename(*CrowdTextureName.ToString(), this)) return false;
        BoneTexture->ClearFlags(RF_Transient);
        BoneTexture->PreEditChange(nullptr);
        BoneTexture->Source.Init(Width, Rows, 1, 1, TSF_RGBA32F, reinterpret_cast<const uint8*>(Texels.GetData()));
        BoneTexture->CompressionSettings = TC_HDR_F32;
        BoneTexture->MipGenSettings = TMGS_NoMipmaps;
        BoneTexture->PostEditChange();
    }
#endif
    BoneTexture->UpdateResource();

    // ---- the materials (one per group: the original's texture, the bones) and the mesh
    Mesh = NewObject<UStaticMesh>(this, MakeUniqueObjectName(this, UStaticMesh::StaticClass(), TEXT("MergedMesh")), HasAnyFlags(RF_Transient) ? RF_Transient : RF_NoFlags);
    for (int32 g = 0; g < GroupSources.Num(); ++g)
    {
        UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, this);
        MID->SetTextureParameterValue(TEXT("BoneTex"), BoneTexture);
        if (UTexture* Diffuse = FirstTexture(GroupSources[g]))
        {
            MID->SetTextureParameterValue(TEXT("Diffuse"), Diffuse);
        }
        Materials.Add(MID);
        DiffuseTextures.Add(FirstTexture(GroupSources[g]));
        const FName Slot(*FString::Printf(TEXT("Crowd_%d"), g));
        Mesh->GetStaticMaterials().Add(FStaticMaterial(MID, Slot, Slot));
    }
    Mesh->bSupportRayTracing = false;
#if WITH_EDITOR
    // UV1-4 are animation data: lightmap generation must never overwrite them on save/cook.
    FMeshBuildSettings& CrowdBuildSettings = Mesh->AddSourceModel().BuildSettings;
    CrowdBuildSettings.bGenerateLightmapUVs = false;
    CrowdBuildSettings.bRecomputeNormals = false;
    CrowdBuildSettings.bRecomputeTangents = false;
    CrowdBuildSettings.bUseFullPrecisionUVs = true;
#endif
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bFastBuild = HasAnyFlags(RF_Transient);
    Params.bMarkPackageDirty = false;
    Params.bCommitMeshDescription = !HasAnyFlags(RF_Transient);
    if (!Mesh->BuildFromMeshDescriptions({ &Description }, Params)) return false;
    // The poses reach beyond the reference pose (a man falling, a rifle raised; the rifle sits at its own origin).
    Mesh->SetPositiveBoundsExtension(FVector(220.0f, 220.0f, 120.0f));
    Mesh->SetNegativeBoundsExtension(FVector(220.0f, 220.0f, 60.0f));
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-CROWD: %s LOD %d, %d vertices, %d bones, %d rows, %d materials"), *Soldier->GetName(), LODIndex,
        Description.Vertices().Num(), NumBones, Rows, Materials.Num());
    return true;
}
