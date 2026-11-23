#include "DISRuntimeModule.h"

#define LOCTEXT_NAMESPACE "FDISRuntimeModule"

void FDISRuntimeModule::StartupModule()
{
	// 런타임 모듈 초기화 로직 수행
}

void FDISRuntimeModule::ShutdownModule()
{
	// 런타임 모듈 종료 및 리소스 정리 수행
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDISRuntimeModule, DISRuntime);
