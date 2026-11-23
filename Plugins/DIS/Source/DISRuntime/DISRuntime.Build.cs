// Copyright (c) 2026 codeGlutton. All Rights Reserved.

using UnrealBuildTool;

public class DISRuntime : ModuleRules
{
	public DISRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				/* 기본 엔진 모듈 */
				"Core",
				"CoreUObject",
				"Engine",

				/* 환경설정 시스템 */
				"DeveloperSettings",

				/* 게임플레이 태그 시스템 */
				"GameplayTags",

				/* UI 시스템 */
				"UMG",
				"Slate",
				"SlateCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
