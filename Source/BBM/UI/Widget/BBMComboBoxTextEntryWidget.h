/*****************************************************************//**
 * @file   BBMComboBoxTextEntryWidget.h
 * @brief  텍스트 기반 콤보박스 Content/Item 위젯 C++ 베이스 클래스 정의
 * @details UUserWidget 파생 최소 껍데기로 UBBMComboBoxTextEntryViewModel을 WBP에서 바인딩하기 위한 기반 클래스임
 * @date   2026-11-18
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BBMComboBoxTextEntryWidget.generated.h"

/**
 * 텍스트 콤보박스 Content/Item 엔트리용 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM ComboBox Text Entry Widget"))
class BBM_API UBBMComboBoxTextEntryWidget : public UUserWidget
{
	GENERATED_BODY()
};
