#include "AssetLinter.h"

#define LOCTEXT_NAMESPACE "FAssetLinterModule"

void FAssetLinterModule::StartupModule()
{
	// 에디터 모듈 초기화 (DataValidation 서브시스템이 UEditorValidatorBase 파생 클래스를 자동 등록함)
}

void FAssetLinterModule::ShutdownModule()
{
	// 에디터 모듈 종료 처리
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssetLinterModule, AssetLinter)
