/*****************************************************************//**
 * @file   BBMGameFeatureCoreRuntimeModule.cpp
 * @brief  BBM(비트 배낭 상인) 코어 게임 피처 런타임 모듈 구현
 * @date   2026-10-28
 *********************************************************************/

#include "BBMGameFeatureCoreRuntimeModule.h"

#define LOCTEXT_NAMESPACE "FBBMGameFeatureCoreRuntimeModule"

void FBBMGameFeatureCoreRuntimeModule::StartupModule()
{
	/* 모듈 로드 시 초기화 로직 수행 */
}

void FBBMGameFeatureCoreRuntimeModule::ShutdownModule()
{
	/* 모듈 언로드 시 정리 로직 수행 */
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBBMGameFeatureCoreRuntimeModule, BBMGameFeatureCoreRuntime)
