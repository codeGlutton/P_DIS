/*****************************************************************//**
 * @file   BBMCreditEntryWidget.h
 * @brief  BBM(비트 배낭 상인) 크레딧 ListView 단일 항목 위젯 C++ 베이스 클래스 정의
 * @details UUserWidget 파생 및 IUserObjectListEntry 구현을 통해 UListView 엔트리 위젯으로 사용 가능한 최소 껍데기를 제공함
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "BBMCreditEntryWidget.generated.h"

/**
 * BBM 크레딧 ListView 엔트리용 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Credit Entry Widget"))
class BBM_API UBBMCreditEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
};
