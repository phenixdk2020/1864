#include "StrategyCrowdModel.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
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

namespace
{
    // The reduced LOD for the far men (about 2,500 vertices; Content/Python/make_crowd_material.py makes the LODs).
    constexpr int32 CrowdLOD = 3;
    const TCHAR* CrowdMaterialPath = TEXT("/Game/Battle/Materials/M_CrowdVAT.M_CrowdVAT");

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
        return *Found;
    }
    if (Sub->Failed.Contains(Key))
    {
        return nullptr;
    }
    UStrategyCrowdModel* Model = NewObject<UStrategyCrowdModel>(Sub);
    const double Start = FPlatformTime::Seconds();
    if (!Model->Bake(World, Soldier, Rifle, Clips, Grip))
    {
        Sub->Failed.Add(Key);
        UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-CROWD: cannot bake %s (the far men stay fully animated)"), *Key);
        return nullptr;
    }
    Sub->Models.Add(Key, Model);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-CROWD: baked %s: %d clips in %.0f ms"), *Key, Model->Clips.Num(), (FPlatformTime::Seconds() - Start) * 1000.0);
    return Model;
}

bool UStrategyCrowdModel::MakeCustomData(const UAnimSequence* Clip, float StartTime, float PlayRate, bool bLoop, float Out[CustomDataFloats], float Now, float HeldPosition) const
{
    const FStrategyCrowdClip* C = Clip ? Clips.Find(Clip) : nullptr;
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

bool UStrategyCrowdModel::Bake(UWorld* World, USkeletalMesh* Soldier, UStaticMesh* Rifle, const TArray<UAnimSequence*>& InClips, const FStrategyCrowdRifleGrip& Grip)
{
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
    TPolygonGroupAttributesRef<FName> SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();

    auto AddVertex = [&](const FVector3f& P, const FVector3f& N, const FVector3f& T, const FVector2f& UV, const int32 Bone[4], const float W[4])
    {
        const FVertexID V = Description.CreateVertex();
        VertexPositions[V] = P;
        const FVertexInstanceID I = Description.CreateVertexInstance(V);
        Normals[I] = N;
        Tangents[I] = T;
        Signs[I] = 1.0f;
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
    if (Rifle && Rifle->GetRenderData() && Rifle->GetRenderData()->LODResources.Num() > 0)
    {
        const FStaticMeshLODResources& R = Rifle->GetRenderData()->LODResources[0];
        TArray<uint32> RifleIndices;
        R.IndexBuffer.GetCopy(RifleIndices);
        if (R.VertexBuffers.PositionVertexBuffer.GetVertexData() && R.VertexBuffers.StaticMeshVertexBuffer.GetTangentData() && RifleIndices.Num() > 0)
        {
            RifleBox = Rifle->GetBoundingBox();
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
    bool bMoved = false;
    FVector FirstHand = FVector::ZeroVector;
    for (UAnimSequence* Clip : Unique)
    {
        const FStrategyCrowdClip& C = Clips[Clip];
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
    BoneTexture->UpdateResource();

    // ---- the materials (one per group: the original's texture, the bones) and the mesh
    Mesh = NewObject<UStaticMesh>(this, NAME_None, RF_Transient);
    for (int32 g = 0; g < GroupSources.Num(); ++g)
    {
        UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, this);
        MID->SetTextureParameterValue(TEXT("BoneTex"), BoneTexture);
        if (UTexture* Diffuse = FirstTexture(GroupSources[g]))
        {
            MID->SetTextureParameterValue(TEXT("Diffuse"), Diffuse);
        }
        Materials.Add(MID);
        const FName Slot(*FString::Printf(TEXT("Crowd_%d"), g));
        Mesh->GetStaticMaterials().Add(FStaticMaterial(MID, Slot, Slot));
    }
    Mesh->bSupportRayTracing = false;
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bFastBuild = true;
    Params.bMarkPackageDirty = false;
    Params.bCommitMeshDescription = false;
    Mesh->BuildFromMeshDescriptions({ &Description }, Params);
    // The poses reach beyond the reference pose (a man falling, a rifle raised; the rifle sits at its own origin).
    Mesh->SetPositiveBoundsExtension(FVector(220.0f, 220.0f, 120.0f));
    Mesh->SetNegativeBoundsExtension(FVector(220.0f, 220.0f, 60.0f));
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-CROWD: %s LOD %d, %d vertices, %d bones, %d rows, %d materials"), *Soldier->GetName(), LODIndex,
        Description.Vertices().Num(), NumBones, Rows, Materials.Num());
    return true;
}
