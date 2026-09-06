using UnrealBuildTool;

public class HMT_Core : ModuleRules
{
	public HMT_Core(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"NetCore",
			// AHMT_RulerActor's OverheadWidgetClass/OverheadWidgetComponent (UWidgetComponent,
			// UUserWidget) are UPROPERTYs on a public header — UHT-generated reflection data
			// references UMG's Z_Construct_UClass_* symbols directly, so this must be a public
			// (linked) dependency, not just an include.
			"UMG",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Procedural fallback tile shapes (HMT_TileMeshUtils) — engine-shipped plugin,
			// declared in HitzMergeTactics.uplugin's Plugins array. Private + forward-declared
			// in the public header, so it never leaks into buyer include paths.
			"ProceduralMeshComponent",
		});
	}
}
