// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BBM : ModuleRules
{
	public BBM(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);
	
		PublicDependencyModuleNames.AddRange(new string[] {
			/* 기본 엔진 모듈 */
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 

			/* 게임플레이 및 모듈러 기능 모듈 */
			"GameplayTags",
			"ModularGameplay",
			"GameFeatures",
			"GameplayAbilities",
			"GameplayTasks",
			"AIModule",
			"DeveloperSettings",

			/* 인벤토리 플러그인 런타임 모듈 */
			"DISRuntime" 
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			 });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
