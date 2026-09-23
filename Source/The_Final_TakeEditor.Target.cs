using UnrealBuildTool;

public class The_Final_TakeEditorTarget : TargetRules
{
	public The_Final_TakeEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("The_Final_Take");
	}
}
