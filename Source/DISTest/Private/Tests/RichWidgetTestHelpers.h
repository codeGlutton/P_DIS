/*****************************************************************//**
 * @file   RichWidgetTestHelpers.h
 * @brief  RichWidgetTests 전용 동적 델리게이트 바인딩 헬퍼 객체 정의
 * @details URichComboBoxKey::OnGenerateContentWidget 및 URichInputKeySelector::OnKeySelected 동적 대리자 검증용 UFUNCTION을 제공함.
 * @date   2026-11-19
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Spacer.h"
#include "Framework/Commands/InputChord.h"
#include "RichWidgetTestHelpers.generated.h"

/**
 * RichWidget 자동화 테스트에서 동적 델리게이트 호출 횟수와 전달 값을 추적하는 경량 헬퍼
 */
UCLASS()
class URichWidgetTestDelegateHelper : public UObject
{
	GENERATED_BODY()

	/* 테스트 대리자 핸들러 */
public:
	UFUNCTION()
	UWidget* HandleGenerateContentWidget(FName InKey)
	{
		++GenerateContentCallCount;
		LastGeneratedContentKey = InKey;
		return NewObject<USpacer>(this);
	}

	UFUNCTION()
	void HandleKeySelected(FInputChord InSelectedKey)
	{
		++KeySelectedCallCount;
		LastSelectedKeyChord = InSelectedKey;
	}

	/* 호출 추적 상태 */
public:
	int32 GenerateContentCallCount = 0;
	FName LastGeneratedContentKey = NAME_None;

	int32 KeySelectedCallCount = 0;
	FInputChord LastSelectedKeyChord;
};
