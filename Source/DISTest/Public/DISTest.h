/*****************************************************************//**
 * @file   DISTest.h
 * @brief  DIS 플러그인 전용 자동화 및 단위 테스트 모듈 정의
 * @date   2026-10-07
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * DIS 플러그인의 코어 기능 및 확장 요소를 검증하기 위한 테스트 전용 모듈
 */
class FDISTestModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
