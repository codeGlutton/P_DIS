/*****************************************************************//**
 * @file   BBMGlobalDefinition.h
 * @brief  BBM(비트 배낭 상인) 전역 정의 에셋 클래스
 * @date   2026-10-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Global/DISGlobalDefinition.h"
#include "Item/BBMItemGroupTypes.h"
#include "BBMGlobalDefinition.generated.h"

/**
 * BBM(비트 배낭 상인) 전역 정의 에셋
 * 기본 아이템/희귀도 프로파일 외에 아이템 그룹 프로파일 매핑 테이블을 보유함
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Global Definition"))
class DIS_API UBBMGlobalDefinition : public UDISGlobalDefinition
{
	GENERATED_BODY()

public:
	UBBMGlobalDefinition();

	/* 프로퍼티 */
public:
	/* 전역 아이템 그룹 프로파일 매핑 맵 (GroupTag -> FBBMItemGroupProfile) */
	UPROPERTY(Category = "Item Groups", EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FBBMItemGroupProfile> ItemGroupProfiles;
};
