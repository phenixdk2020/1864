"""UE 5.8-headerkontrol og Python-syntaks for Codex 21; ingen build/import."""
import ast
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ENGINE = Path("I:/Spil/Epic Games/UE_5.8/Engine")
CHECKS = {
    "Runtime/Core/Public/Math/UnrealMathUtility.h": ["IsNearlyEqual", "GetMappedRangeValueClamped"],
    "Runtime/Core/Public/GenericPlatform/GenericPlatformMath.h": ["IsFinite"],
    "Runtime/Core/Public/Containers/Map.h.inl": ["Contains("],
    "Runtime/Core/Public/Math/Vector2D.h": ["DistSquared", "Distance"],
    "Runtime/Engine/Classes/Engine/AssetManager.h": ["GetStreamableManager"],
    "Runtime/Engine/Classes/Engine/StreamableManager.h": ["RequestAsyncLoad", "FStreamableDelegate", "CancelHandle"],
    "Runtime/Engine/Classes/Engine/StaticMesh.h": ["BuildFromMeshDescriptions", "FBuildMeshDescriptionsParams", "GetStaticMaterials", "bSupportRayTracing"],
    "Runtime/Engine/Public/Materials/MaterialInstanceDynamic.h": ["Create(", "SetScalarParameterValue", "SetTextureParameterValue"],
    "Runtime/Engine/Classes/Components/ActorComponent.h": ["DestroyComponent", "RegisterComponent", "IsRegistered"],
    "Runtime/Engine/Classes/Components/SceneComponent.h": ["SetupAttachment", "SetRelativeLocation", "SetRelativeScale3D", "SetVisibility"],
    "Runtime/Engine/Classes/Components/PrimitiveComponent.h": ["SetCollisionEnabled", "SetCastShadow"],
    "Runtime/Engine/Classes/Components/StaticMeshComponent.h": ["SetStaticMesh"],
    "Runtime/Engine/Classes/Components/MeshComponent.h": ["SetMaterial"],
    "Runtime/Engine/Public/EngineUtils.h": ["TActorIterator"],
    "Runtime/CoreUObject/Public/UObject/SoftObjectPath.h": ["ResolveObject"],
    "Runtime/Core/Public/Misc/Paths.h": ["FileExists", "ProjectDir"],
    "Runtime/Core/Public/Misc/FileHelper.h": ["LoadFileToArray", "LoadFileToString"],
    "Runtime/Core/Public/Misc/CommandLine.h": ["Get()"],
    "Runtime/Core/Public/Misc/Parse.h": ["Param("],
    "Runtime/Core/Public/Windows/WindowsPlatformTime.h": ["Seconds()"],
    "Runtime/Json/Public/Dom/JsonObject.h": ["TryGetNumberField", "TryGetArrayField", "TryGetObjectField"],
    "Runtime/Json/Public/Serialization/JsonSerializer.h": ["Deserialize"],
    "Runtime/Json/Public/Serialization/JsonReader.h": ["Create("],
    "Runtime/MeshDescription/Public/MeshDescription.h": ["CreateVertex", "CreateVertexInstance", "GetVertexInstanceVertex", "CreateTriangle", "CreatePolygonGroup"],
    "Runtime/MeshDescription/Public/MeshAttributeArray.h": ["Get(", "Set(", "SetNumChannels"],
    "Runtime/MeshDescription/Public/MeshAttributes.h": ["GetVertexPositions"],
    "Runtime/StaticMeshDescription/Public/StaticMeshAttributes.h": ["Register", "GetVertexInstanceUVs", "GetVertexInstanceNormals", "GetVertexInstanceTangents", "GetVertexInstanceBinormalSigns", "GetPolygonGroupMaterialSlotNames"],
    "Editor/UnrealEd/Classes/Factories/TextureFactory.h": ["bCreateMaterial"],
    "Editor/UnrealEd/Public/AssetImportTask.h": ["DestinationName", "Factory", "bAutomated", "bSave", "bReplaceExisting", "Filename", "DestinationPath"],
    "Runtime/Engine/Classes/Engine/Texture.h": ["VirtualTextureStreaming", "SRGB", "MipGenSettings", "PowerOfTwoMode", "CompressionSettings"],
    "Runtime/Engine/Classes/Engine/TextureDefines.h": ["TMGS_SimpleAverage", "TC_Masks", "PadToPowerOfTwo", "TA_Clamp"],
    "Runtime/Engine/Classes/Engine/Texture2D.h": ["AddressX", "AddressY"],
    "Editor/MaterialEditor/Public/MaterialEditingLibrary.h": ["DeleteAllMaterialExpressions", "CreateMaterialExpression", "ConnectMaterialExpressions", "ConnectMaterialProperty", "SetMaterialInstanceParent", "SetMaterialInstanceTextureParameterValue", "SetMaterialInstanceScalarParameterValue", "UpdateMaterialInstance", "RecompileMaterial"],
    "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h": ["GetConsoleVariableIntValue"],
    "Developer/AssetTools/Public/IAssetTools.h": ["ImportAssetTasks", "CreateAsset", "GetAssetTools"],
    "Editor/UnrealEd/Public/Subsystems/EditorAssetSubsystem.h": ["LoadAsset", "DoesAssetExist", "SaveLoadedAsset"],
    "Runtime/Engine/Public/Materials/MaterialExpressionTextureCoordinate.h": ["CoordinateIndex", "UTiling", "VTiling"],
    "Runtime/Engine/Public/Materials/MaterialExpressionTextureSampleParameter.h": ["ParameterName"],
    "Runtime/Engine/Public/Materials/MaterialExpressionTextureBase.h": ["SamplerType", "Texture"],
    "Runtime/Engine/Public/Materials/MaterialExpressionScalarParameter.h": ["DefaultValue"],
    "Runtime/Engine/Public/Materials/MaterialExpressionCustom.h": ["CMOT_Float3", "Code", "OutputType", "Description", "Inputs", "InputName"],
    "Runtime/Engine/Classes/Engine/EngineTypes.h": ["SAMPLERTYPE_VirtualColor", "SAMPLERTYPE_VirtualMasks", "MSM_Unlit"],
    "Editor/UnrealEd/Classes/Factories/MaterialFactoryNew.h": ["UMaterialFactoryNew"],
    "Editor/UnrealEd/Classes/Factories/MaterialInstanceConstantFactoryNew.h": ["UMaterialInstanceConstantFactoryNew"],
    "Runtime/Engine/Classes/Engine/PrimaryAssetLabel.h": ["bLabelAssetsInMyDirectory", "ExplicitAssets", "Rules"],
    "Runtime/Engine/Classes/Engine/AssetManagerTypes.h": ["AlwaysCook", "CookRule", "Priority", "bApplyRecursively"],
    "Editor/UnrealEd/Classes/Factories/DataAssetFactory.h": ["DataAssetClass"],
}


def validate():
    for filename, symbols in CHECKS.items():
        text = (ENGINE / "Source" / filename).read_text(encoding="utf-8",errors="replace")
        for symbol in symbols:
            assert symbol in text, (filename, symbol)
    # EditorAssetLibrary is shipped as a plugin; its replacement subsystem above
    # is in Engine/Source. Check the legacy Python wrapper as well.
    plugin_header = ENGINE / "Plugins/Editor/EditorScriptingUtilities/Source/EditorScriptingUtilities/Public/EditorAssetLibrary.h"
    text = plugin_header.read_text(encoding="utf-8",errors="replace")
    for symbol in ("LoadAsset", "SaveLoadedAsset", "DoesAssetExist"):
        assert symbol in text
    includes = []
    cpp = ROOT / "Source/Game1864/Private/Campaign1851MapTiles.cpp"
    import re
    include_roots = [ROOT / "Source/Game1864/Public", ROOT / "Source/Game1864/Private"] + [ENGINE / "Source" / p for p in ("Runtime/Core/Public", "Runtime/CoreUObject/Public", "Runtime/Engine/Classes", "Runtime/Engine/Public", "Runtime/Json/Public", "Runtime/MeshDescription/Public", "Runtime/StaticMeshDescription/Public")]
    for filename in re.findall(r'^#include "([^"]+)"', cpp.read_text(encoding="utf-8"), re.M):
        assert any((p / filename).is_file() for p in include_roots), filename
        includes.append(filename)
    assert includes[0] == "Campaign1851Map.h"
    for filename in ("Tools/Map1851/build_runtime_tiles.py", "Tools/Map1851/validate_runtime_tiles.py", "Content/Python/import_map_tiles.py", "Tools/Campaign/validate_map_tile_api.py"):
        raw = (ROOT / filename).read_bytes()
        assert b"\n" not in raw.replace(b"\r\n",b""), filename
        ast.parse(raw.decode("utf-8"),filename=filename)
    report = dict(status="OK",engine="UE 5.8",headers=len(CHECKS)+1,symbols=sum(map(len,CHECKS.values()))+3,includes=len(includes),pythonFiles=4,note="Header-/syntakskontrol; ingen C++-kompilering eller Unreal-Python-k?rsel.")
    (ROOT / "Data/Campaign1851/MapTileApi_Validation.json").write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8",newline="\r\n")
    print(json.dumps(report,ensure_ascii=False))


if __name__ == "__main__":
    validate()
