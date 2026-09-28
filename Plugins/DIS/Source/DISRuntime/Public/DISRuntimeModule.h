/*****************************************************************//**
 * @file   DISRuntimeModule.h
 * @brief  DIS 플러그인 런타임 코어 모듈 인터페이스 정의
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * DIS 런타임 코어 모듈 인터페이스
 */
class DISRUNTIME_API FDISRuntimeModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
