/*****************************************************************//**
 * @file   BBMSettingsTabEntryWidget.h
 * @brief  BBM(비트 배낭 상인) 환경설정 화면 전용 탭 엔트리 위젯 C++ 베이스 클래스 정의
 * @details UBBMTabEntryWidget에서 파생되어 환경설정 UI 탭 버튼 위젯 블루프린트의 부모 클래스로 사용됨
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/BBMTabEntryWidget.h"
#include "BBMSettingsTabEntryWidget.generated.h"

/**
 * BBM 환경설정 탭 엔트리용 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Settings Tab Entry Widget"))
class BBM_API UBBMSettingsTabEntryWidget : public UBBMTabEntryWidget
{
	GENERATED_BODY()
};
