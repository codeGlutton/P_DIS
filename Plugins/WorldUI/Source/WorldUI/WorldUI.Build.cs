// Copyright (c) 2026 codeGlutton. All Rights Reserved.

using UnrealBuildTool;

public class WorldUI : ModuleRules
{
	public WorldUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				/* 기본 엔진 모듈 */
				"Core",
				"CoreUObject",
				"Engine",

				/* UI 및 MVVM 모듈 */
				"Slate",
				"SlateCore",
				"UMG",
				"ModelViewViewModel"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				/* 애니메이션 에셋 조회 */
				"MovieScene"
			}
		);
	}
}