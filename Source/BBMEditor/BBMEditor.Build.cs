// Copyright (c) 2026 codeGlutton. All Rights Reserved.

using UnrealBuildTool;

public class BBMEditor : ModuleRules
{
	public BBMEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				/* 기본 엔진 모듈 */
				"Core",
				"CoreUObject",
				"Engine",

				/* 호스트 게임 런타임 모듈 */
				"BBM"
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
				"PropertyEditor",
				"EditorStyle"
			}
		);
	}
}

