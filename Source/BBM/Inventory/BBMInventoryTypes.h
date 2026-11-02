/*****************************************************************//**
 * @file   BBMInventoryTypes.h
 * @brief  BBM(비트 배낭 상인) 인벤토리 전용 열거형 및 슬롯 강화 이펙트 명세 목록 구조체 정의
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Enhancement/DISEnhancementEffectSpec.h"
#include "BBMInventoryTypes.generated.h"

/**
 * 2D 그리드 배낭 인벤토리에서 아이템을 순회/추출(Extraction)하는 우선순위 방향 열거형
 */
UENUM(BlueprintType)
enum class EBBMItemExtractionOrder : uint8
{
	/* 상단 우선 -> 동일 행 내 좌측 우선 (행 우선, Top-Left to Bottom-Right) */
	TopToBottom_LeftToRight = 0 UMETA(DisplayName = "상단 우선 -> 동일 행 좌측 우선 (행 우선)"),

	/* 상단 우선 -> 동일 행 내 우측 우선 (Top-Right to Bottom-Left) */
	TopToBottom_RightToLeft = 1 UMETA(DisplayName = "상단 우선 -> 동일 행 우측 우선"),

	/* 좌측 우선 -> 동일 열 내 상단 우선 (열 우선, Left-to-Right, Top-to-Bottom) */
	LeftToRight_TopToBottom = 2 UMETA(DisplayName = "좌측 우선 -> 동일 열 상단 우선 (열 우선)"),

	/* 우측 우선 -> 동일 열 내 상단 우선 (Right-to-Left, Top-to-Bottom) */
	RightToLeft_TopToBottom = 3 UMETA(DisplayName = "우측 우선 -> 동일 열 상단 우선"),

	/* 하단 우선 -> 동일 행 내 좌측 우선 (Bottom-to-Top, Left-to-Right) */
	BottomToTop_LeftToRight = 4 UMETA(DisplayName = "하단 우선 -> 동일 행 좌측 우선"),

	/* 하단 우선 -> 동일 행 내 우측 우선 (Bottom-to-Top, Right-to-Left) */
	BottomToTop_RightToLeft = 5 UMETA(DisplayName = "하단 우선 -> 동일 행 우측 우선")
};

/**
 * 특정 2D 그리드 슬롯 칸에 적용할 UDISEnhancementEffect 명세(Spec) 목록
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMSlotEffectSpecList
{
	GENERATED_BODY()

	FBBMSlotEffectSpecList()
	{
	}

	explicit FBBMSlotEffectSpecList(const TArray<FDISEnhancementEffectSpec>& InEffects) :
		Effects(InEffects)
	{
	}

	/* 프로퍼티 */
public:
	/* 해당 슬롯 자체에 적용할 강화 효과 명세 목록 (UDISEnhancementEffect 파생 클래스 및 배율) */
	UPROPERTY(Category = "Enhancement", EditAnywhere, BlueprintReadWrite)
	TArray<FDISEnhancementEffectSpec> Effects;
};
