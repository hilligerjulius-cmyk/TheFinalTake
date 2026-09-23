using UnrealBuildTool;

public class The_Final_Take : ModuleRules
{
	public The_Final_Take(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "Slate", "SlateCore", "NetCore", "MeshDescription", "StaticMeshDescription"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetTools", "AssetRegistry" });
		}
	}
}
