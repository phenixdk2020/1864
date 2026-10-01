using UnrealBuildTool;

public class Strategy1864 : ModuleRules
{
    public Strategy1864(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "EnhancedInput",
                "AIModule",
                "NavigationSystem",
                "UMG",
                "ProceduralMeshComponent",
                "Json",
                "ImageWrapper",
                "RenderCore",
                "Game1864"   // the campaign: the scenery pieces for the battlefield from the campaign
            });
    }
}
