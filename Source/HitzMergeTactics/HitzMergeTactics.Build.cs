// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HitzMergeTactics : ModuleRules
{
	public HitzMergeTactics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "HMT_Core", "UMG", "ProceduralMeshComponent" });

		PrivateDependencyModuleNames.AddRange(new string[] { "GameplayTags" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Online subsystem for session management (Phase 14.5 main menu)
		PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
