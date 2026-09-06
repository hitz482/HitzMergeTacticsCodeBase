using UnrealBuildTool;

public class HMT_GASBridge : ModuleRules
{
	public HMT_GASBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"GameplayAbilities",
			"GameplayTasks",
			"HMT_Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});
	}
}
