/*****************************************************************//**
 * @file   ExtensionWidgetModule.h
 * @brief  ExtensionWidget 런타임 플러그인 모듈 인터페이스 및 로그 카테고리 선언
 * @details Slate/UMG 확장 위젯(RichScrollBar, RichListView 등) 모듈의 진입점을 정의함.
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogExtensionWidget, Log, All);

class EXTENSIONWIDGET_API FExtensionWidgetModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
