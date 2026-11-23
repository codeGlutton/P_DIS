/*****************************************************************//**
 * @file   ExtensionWidgetEditorModule.h
 * @brief  ExtensionWidget 에디터 전용 모듈 인터페이스 및 커스터마이징 등록부
 * @details UMG 디자이너에서 URichComboBoxKey의 Viewmodel Extension 토글 및 Entry ViewModel 선택 UI를 등록/해제함.
 * @date   2026-11-18
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class FRichComboBoxKeyCustomizationExtender;

/**
 * ExtensionWidget 에디터 모듈
 */
class EXTENSIONWIDGETEDITOR_API FExtensionWidgetEditorModule : public IModuleInterface
{
	/* 모듈 라이프사이클 */
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/* 등록된 UMG 디자이너 위젯 확장기 */
private:
	TSharedPtr<FRichComboBoxKeyCustomizationExtender> ComboBoxKeyCustomizationExtender;
};
