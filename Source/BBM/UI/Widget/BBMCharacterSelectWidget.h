/*****************************************************************//**
 * @file   BBMCharacterSelectWidget.h
 * @brief  BBM(비트 배낭 상인) 캐릭터 선택 팝업 위젯 C++ 베이스 클래스 정의
 * @details PersistentViewport 수명 주기 및 Popup ZOrder 레이어를 사용하며 UBBMCharacterSelectViewModel 접근을 지원함
 * @date   2026-11-20
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widget/WorldUserWidgetBase.h"
#include "BBMCharacterSelectWidget.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMCharacterSelectViewModel;

/**
 * BBM 캐릭터 선택 화면 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Character Select Widget"))
class BBM_API UBBMCharacterSelectWidget : public UWorldUserWidgetBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMCharacterSelectWidget(const FObjectInitializer& ObjectInitializer);

	/* 뷰모델 헬퍼 */
public:
	/* 현재 바인딩된 UBBMCharacterSelectViewModel 반환 */
	UFUNCTION(Category = "BBM|UI|CharacterSelect", BlueprintPure)
	UBBMCharacterSelectViewModel* GetCharacterSelectViewModel() const;
};
