/*****************************************************************//**
 * @file   BBMInventorySlot.h
 * @brief  BBM(비트 배낭 상인) 배낭 인벤토리 개별 슬롯 클래스 정의
 * @details 슬롯 자체의 EnhancementContainer 활성화 및 배치된 아이템으로의 UDISEnhancementEffect 전파/회수 총괄
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Inventory/DISInventorySlot.h"
#include "BBMInventorySlot.generated.h"

/**
 * BBM 전용 인벤토리 개별 슬롯 클래스
 * DIS 코어의 UDISInventorySlot을 상속받아 배낭 인벤토리 슬롯 기능을 수행함
 */
UCLASS(BlueprintType, meta = (DisplayName = "BBM Inventory Slot"))
class BBM_API UBBMInventorySlot : public UDISInventorySlot
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMInventorySlot();

	/* UDISInventorySlot 가상함수 오버라이드 */
public:
	virtual void InitializeSlot(int32 InSlotIndex) override;
};
