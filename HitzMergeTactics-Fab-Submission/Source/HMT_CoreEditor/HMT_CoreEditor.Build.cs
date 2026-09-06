using UnrealBuildTool;

public class HMT_CoreEditor : ModuleRules
{
	public HMT_CoreEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"HMT_Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"PropertyEditor",
			"Slate",
			"SlateCore",
			"DataValidation",
			"GameplayTags",
			"AssetTools",
			"EditorFramework",
			"InputCore",
			"ApplicationCore",
			// Board-preview button builds the same procedural fallback tiles as runtime
			// (HMT_TileMeshUtils) when a layout has no TileMesh assigned.
			"ProceduralMeshComponent",
		});
	}
}
