/*****************************************************************//**
 * @file   BBMKeyMappingCategoryEntryWidget.h
 * @brief  BBM(비트 배낭 상인) 키 매핑 카테고리 ListView 단일 섹션 위젯 C++ 베이스 클래스 정의
 * @details UUserWidget 파생 및 IUserObjectListEntry 구현을 통해 카테고리 단위 ListView 엔트리 위젯으로 사용 가능한 최소 껍데기를 제공함
 * @date   2026-11-17
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "BBMKeyMappingCategoryEntryWidget.generated.h"

/**
 * BBM 키 매핑 카테고리 ListView 엔트리용 UMG C++ 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Key Mapping Category Entry Widget"))
class BBM_API UBBMKeyMappingCategoryEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
};
