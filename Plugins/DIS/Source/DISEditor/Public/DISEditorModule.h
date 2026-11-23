/*****************************************************************//**
 * @file   DISEditorModule.h
 * @brief  DIS 플러그인 에디터 전용 확장 모듈 인터페이스 정의
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * DIS 에디터 전용 확장 모듈 인터페이스
 */
class DISEDITOR_API FDISEditorModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
