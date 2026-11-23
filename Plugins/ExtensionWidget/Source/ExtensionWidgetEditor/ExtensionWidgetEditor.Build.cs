// Copyright (c) 2026 codeGlutton. All Rights Reserved.

using UnrealBuildTool;

public class ExtensionWidgetEditor : ModuleRules
{
	public ExtensionWidgetEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				/* 기본 엔진 모듈 */
				"Core",
				"CoreUObject",
				"Engine",

				/* 런타임 확장 위젯 모듈 */
				"ExtensionWidget",

				/* UI 및 에디터 확장 인터페이스 */
				"UMG",
				"UMGEditor"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				/* UI 및 프로퍼티 디테일 커스터마이제이션 */
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"PropertyEditor",

				/* MVVM 블루프린트 조회 모듈 */
				"ModelViewViewModel",
				"ModelViewViewModelBlueprint"
			}
		);
	}
}
