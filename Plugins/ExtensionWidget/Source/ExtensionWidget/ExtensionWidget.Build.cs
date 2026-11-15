// Copyright (c) 2026 codeGlutton. All Rights Reserved.

using UnrealBuildTool;

public class ExtensionWidget : ModuleRules
{
	public ExtensionWidget(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				/* 기본 엔진 모듈 */
				"Core",
				"CoreUObject",
				"Engine",

				/* UI 모듈 */
				"Slate",
				"SlateCore",
				"UMG",
				"InputCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
