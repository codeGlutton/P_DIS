/*****************************************************************//**
 * @file   BBMGameFeatureCoreRuntimeModule.h
 * @brief  BBM(비트 배낭 상인) 코어 게임 피처 런타임 모듈 선언
 * @date   2026-10-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBBMGameFeatureCoreRuntimeModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
