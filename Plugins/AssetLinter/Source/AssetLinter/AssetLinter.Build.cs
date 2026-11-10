// Copyright (c) 2026 codeGlutton. All Rights Reserved.

using UnrealBuildTool;

public class AssetLinter : ModuleRules
{
	public AssetLinter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				/* 기본 엔진 모듈 */
				"Core",
				"CoreUObject",
				"Engine",

				/* 유효성 검증 및 리플렉션 모듈 */
				"DataValidation",
				"DeveloperSettings",
				"AssetRegistry",

				/* 에셋 툴 및 리네임 모듈 */
				"AssetTools",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				/* 에디터 UI 및 슬레이트 모듈 */
				"UnrealEd",
				"Slate",
				"SlateCore",
				"InputCore",

				/* 에셋 액션 유틸리티 모듈 */
				"Blutility",
			}
		);
	}
}
