/*****************************************************************//**
 * @file   BBMSettingsWidget.h
 * @brief  BBM(비트 배낭 상인) 환경설정 팝업 위젯 C++ 베이스 클래스 정의
 * @details PersistentViewport 수명 주기 및 Popup(30) ZOrder 레이어를 사용하며 UBBMSettingsViewModel 접근을 지원함
 * @date   2026-11-13
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/BBMUserWidgetBase.h"
#include "BBMSettingsWidget.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMSettingsViewModel;

/**
 * BBM 환경설정 화면 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Settings Widget"))
class BBM_API UBBMSettingsWidget : public UBBMUserWidgetBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMSettingsWidget(const FObjectInitializer& ObjectInitializer);

	/* 뷰모델 헬퍼 */
public:
	/* 현재 바인딩된 UBBMSettingsViewModel 반환 */
	UFUNCTION(Category = "BBM|UI|Settings", BlueprintPure)
	UBBMSettingsViewModel* GetSettingsViewModel() const;
};
