/*****************************************************************//**
 * @file   BBMSaveWidget.h
 * @brief  BBM(비트 배낭 상인) 세이브 알림 위젯 C++ 베이스 클래스 정의
 * @details UWorldUserWidgetBase를 상속하며 저장 진행 인디케이터 위젯 블루프린트의 공통 부모 클래스로 활용됨
 * @date   2026-11-19
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widget/WorldUserWidgetBase.h"
#include "BBMSaveWidget.generated.h"

/**
 * BBM 세이브 알림 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Save Widget"))
class BBM_API UBBMSaveWidget : public UWorldUserWidgetBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMSaveWidget(const FObjectInitializer& ObjectInitializer);
};
