/*****************************************************************//**
 * @file   BBMInventory.h
 * @brief  BBM(비트 배낭 상인) 배낭 인벤토리 런타임 클래스 정의
 * @details 아이템 꺼내는 방향(추출 순서) 정렬 및 BBM 전용 슬롯 생성 팩토리를 전담함
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Inventory/DISInventory.h"
#include "BBMInventory.generated.h"

class UBBMInventoryDefinition;
class UBBMInventorySlot;
class UDISItemStack;

/**
 * BBM 전용 배낭 인벤토리 런타임 클래스
 * 정적 정의(UBBMInventoryDefinition)의 추출 순서에 따른 아이템 정렬 및 슬롯 팩토리를 제공함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Inventory"))
class BBM_API UBBMInventory : public UDISInventory
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMInventory();

	/* BBM 고유 아이템 추출 순서 API */
public:
	/* 인벤토리 정의의 ExtractionOrder에 따라 정렬된 아이템 스택 목록 반환 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable, BlueprintPure)
	TArray<UDISItemStack*> GetItemStacksInExtractionOrder() const;

	/* 슬롯 생성 팩토리 가상함수 오버라이드 */
protected:
	virtual UDISInventorySlot* CreateSlotInstance(int32 SlotIndex) override;
};
