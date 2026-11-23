/*****************************************************************//**
 * @file   BBMLoadingWidget.h
 * @brief  BBM(비트 배낭 상인) 로딩 화면 위젯 C++ 베이스 클래스 정의
 * @details UWorldUserWidgetBase를 상속하며 레벨/지역 전환 로딩 위젯 블루프린트의 공통 부모 클래스로 활용됨
 * @date   2026-11-19
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widget/WorldUserWidgetBase.h"
#include "BBMLoadingWidget.generated.h"

/**
 * BBM 로딩 화면 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Loading Widget"))
class BBM_API UBBMLoadingWidget : public UWorldUserWidgetBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMLoadingWidget(const FObjectInitializer& ObjectInitializer);
};
