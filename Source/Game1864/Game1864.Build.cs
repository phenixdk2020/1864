using UnrealBuildTool;

public class Game1864 : ModuleRules
{
	public Game1864(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"MeshDescription",
			"StaticMeshDescription",
			"Slate",
			"SlateCore",
			"ImageWrapper",
			"Json"
		});
	}
}
