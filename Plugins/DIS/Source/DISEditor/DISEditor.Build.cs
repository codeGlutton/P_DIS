// Copyright (c) 2026 codeGlutton. All Rights Reserved.

using UnrealBuildTool;

public class DISEditor : ModuleRules
{
	public DISEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				/* 기본 엔진 모듈 */
				"Core",
				"CoreUObject",
				"Engine",

				/* 프로젝트 런타임 모듈 */
				"DISRuntime"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				/* UI 및 에디터 툴 모듈 */
				"Slate",
				"SlateCore",
				"UnrealEd",
				"ToolMenus",
				"PropertyEditor"
			}
		);
	}
}
