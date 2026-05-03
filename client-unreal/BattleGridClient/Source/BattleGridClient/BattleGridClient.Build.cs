// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BattleGridClient : ModuleRules
{
	public BattleGridClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore",
			"WebSockets",
			"Json",
			"JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"BattleGridClient",
			"BattleGridClient/Variant_Strategy",
			"BattleGridClient/Variant_Strategy/UI",
			"BattleGridClient/Variant_TwinStick",
			"BattleGridClient/Variant_TwinStick/AI",
			"BattleGridClient/Variant_TwinStick/Gameplay",
			"BattleGridClient/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
