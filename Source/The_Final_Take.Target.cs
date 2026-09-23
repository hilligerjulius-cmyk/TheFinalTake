using UnrealBuildTool;

public class The_Final_TakeTarget : TargetRules
{
	public The_Final_TakeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("The_Final_Take");
	}
}
