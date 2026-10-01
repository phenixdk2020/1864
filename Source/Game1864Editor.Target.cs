using UnrealBuildTool;

public class Game1864EditorTarget : TargetRules
{
	public Game1864EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Game1864");
		ExtraModuleNames.Add("Strategy1864");   // the battle (mirrored from Strategy1864, Tools/Battle/Sync-Battle.ps1)
	}
}
