/*****************************************************************//**
 * @file   BBMItemGroupTypes.h
 * @brief  BBM(비트 배낭 상인) 아이템 그룹 시스템 타입 및 프로파일 정의
 * @date   2026-10-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BBMItemGroupTypes.generated.h"

/**
 * BBM(비트 배낭 상인) 아이템 그룹의 표시 및 메타 정보를 정의하는 프로파일 구조체
 */
USTRUCT(BlueprintType)
struct DIS_API FBBMItemGroupProfile
{
	GENERATED_BODY()

	/* 프로퍼티 */
public:
	/* 그룹 표시 이름 */
	UPROPERTY(Category = "Display", EditAnywhere, BlueprintReadWrite)
	FText DisplayName;
};
