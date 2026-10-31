// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DISTest : ModuleRules
{
	public DISTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		/* 공용 의존성 모듈 */
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"GameplayAbilities",
			"GameplayTasks",
			"AIModule",
			"DISRuntime",
			"BBM"
		});

		/* 비공개 의존성 모듈 (자동화 테스트 및 에디터 서브시스템) */
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AutomationController"
		});
	}
}
