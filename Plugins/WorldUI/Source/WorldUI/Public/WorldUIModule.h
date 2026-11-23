/*****************************************************************//**
 * @file   WorldUIModule.h
 * @brief  WorldUI 런타임 플러그인 모듈 인터페이스 및 로그 카테고리 선언
 * @details 월드 단위 레이어 UI 서브시스템, 위젯 프레젠테이션, MVVM 전환 모듈 진입점을 정의함
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWorldUI, Log, All);

/**
 * WorldUI 플러그인 런타임 모듈
 */
class WORLDUI_API FWorldUIModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};