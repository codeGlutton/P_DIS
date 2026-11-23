/*****************************************************************//**
 * @file   BBMInventoryDefinition.h
 * @brief  BBM(비트 배낭 상인) 배낭 인벤토리 정의 클래스
 * @details 아이템 꺼내는 방향(추출 순서) 및 슬롯 칸 강화(Slot Enhancement) 속성을 보유함
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Inventory/DISInventoryDefinition.h"
#include "Inventory/BBMInventoryTypes.h"
#include "BBMInventoryDefinition.generated.h"

class UDISInventory;
class UDISItemStack;

/**
 * BBM 전용 배낭 인벤토리 불변 정적 정의(PrimaryDataAsset) 클래스
 * 기본 2D 공간 규격 외에 턴/비트 진행 시 아이템을 꺼내는 방향 우선순위와 슬롯 칸 강화 설정을 관리함
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Inventory Definition"))
class BBM_API UBBMInventoryDefinition : public UDISInventoryDefinition
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMInventoryDefinition();

	/* 슬롯 강화 조회 헬퍼 */
public:
	/* 특정 2D 그리드 좌표에 할당된 슬롯 강화 효과 명세 목록 조회 (없으면 nullptr) */
	const FBBMSlotEffectSpecList* FindSlotEnhancements(const FIntPoint& Coord) const;

	/* 특정 1차원 슬롯 인덱스에 할당된 슬롯 강화 설정 존재 여부 조회 */
	UFUNCTION(Category = "BBM|Inventory Definition", BlueprintCallable, BlueprintPure)
	bool HasSlotEnhancement(int32 SlotIndex) const;

	/* 프로퍼티 */
public:
	/* 2D 배낭 그리드에서 턴/비트 진행 시 아이템을 꺼내는 우선순위 방향 */
	UPROPERTY(Category = "BBM|Extraction", EditDefaultsOnly, BlueprintReadOnly)
	EBBMItemExtractionOrder ExtractionOrder;

	/* 2D 타일 좌표별 슬롯에 적용할 강화 효과 명세 목록 (X, Y 좌표 -> 강화 효과 명세 목록) */
	UPROPERTY(Category = "BBM|Slot Enhancement", EditDefaultsOnly, BlueprintReadOnly)
	TMap<FIntPoint, FBBMSlotEffectSpecList> SlotEnhancements;
};

