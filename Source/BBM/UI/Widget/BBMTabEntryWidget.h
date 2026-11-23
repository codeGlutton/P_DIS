/*****************************************************************//**
 * @file   BBMTabEntryWidget.h
 * @brief  BBM(비트 배낭 상인) 범용 탭 버튼 엔트리 위젯 C++ 베이스 클래스 정의
 * @details UUserWidget 파생 및 IUserObjectListEntry 구현을 통해 탭 목록/패널 엔트리 위젯으로 사용 가능한 최소 껍데기를 제공함
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "BBMTabEntryWidget.generated.h"

/**
 * BBM 범용 탭 엔트리용 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Tab Entry Widget"))
class BBM_API UBBMTabEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
};
