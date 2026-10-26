/*****************************************************************//**
 * @file   DISSharedInventorySlot.h
 * @brief  DIS 공유/단축 인벤토리의 개별 1차원 슬롯 런타임 UObject 클래스 정의
 * @details 슬롯 인덱스, 아이템 스택 참조 포인터 및 슬롯 자체 필터링을 관리하는 경량 슬롯 객체
 * @date   2026-10-26
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Inventory/DISInventoryFilter.h"
#include "DISSharedInventorySlot.generated.h"

class UDISItemStack;
class UDISInventory;

/**
 * 1차원 선형 공유 인벤토리 내 개별 슬롯의 런타임 상태, 참조 및 필터링을 관리하는 경량 UObject 객체
 */
UCLASS(BlueprintType, meta = (DisplayName = "DIS Shared Inventory Slot"))
class DISRUNTIME_API UDISSharedInventorySlot : public UObject
{
	GENERATED_BODY()

	friend class UDISSharedInventory;

	/* 생성자 */
public:
	UDISSharedInventorySlot();

	/* 초기화 */
public:
	/* 슬롯 고유 인덱스를 기반으로 슬롯 인스턴스 초기화 */
	UFUNCTION(Category = "DIS|Shared Slot", BlueprintCallable)
	void InitializeSlot(int32 InSlotIndex);

	/* 슬롯 식별 및 상태 조회 */
public:
	/* 인벤토리 내 슬롯 고유 인덱스 반환 */
	UFUNCTION(Category = "DIS|Shared Slot", BlueprintCallable, BlueprintPure)
	int32 GetSlotIndex() const;

	/* 슬롯이 비어있는지 여부 반환 */
	UFUNCTION(Category = "DIS|Shared Slot", BlueprintCallable, BlueprintPure)
	bool IsEmpty() const;

	/* 현재 슬롯에 보관된 아이템 스택 수량 반환 */
	UFUNCTION(Category = "DIS|Shared Slot", BlueprintCallable, BlueprintPure)
	int32 GetStackCount() const;

	/* 슬롯이 참조하고 있는 아이템 스택 인스턴스 반환 (없으면 nullptr) */
	UFUNCTION(Category = "DIS|Shared Slot", BlueprintCallable, BlueprintPure)
	UDISItemStack* GetItemStack() const;

	/* 아이템 상태 조작 */
public:
	/* 슬롯에 아이템 스택 설정 (InItemStack이 nullptr이면 ClearSlot 수행) */
	UFUNCTION(Category = "DIS|Shared Slot", BlueprintCallable)
	bool SetItemStack(UDISItemStack* InItemStack);

	/* 슬롯을 비우고 참조 중이던 아이템 스택 반환 */
	UFUNCTION(Category = "DIS|Shared Slot", BlueprintCallable)
	UDISItemStack* ClearSlot();

	/* 필터 관리 */
public:
	/* 슬롯에 등록 가능한 TInstancedStruct 필터 추가 */
	void AddFilter(const TInstancedStruct<FDISInventoryFilter>& InFilter);

	/* 템플릿 기반 구체 필터 생성 및 추가 */
	template <typename TFilterType, typename... TArgs>
	TFilterType& EmplaceFilter(TArgs&&... Args)
	{
		TInstancedStruct<FDISInventoryFilter> NewFilter = TInstancedStruct<FDISInventoryFilter>::Make<TFilterType>(Forward<TArgs>(Args)...);
		Filters.Add(NewFilter);
		return NewFilter.GetMutable<TFilterType>();
	}

	/* 슬롯의 모든 필터 제거 */
	UFUNCTION(Category = "DIS|Shared Slot", BlueprintCallable)
	void ClearFilters();

	/* 슬롯에 등록된 모든 필터 목록 반환 */
	const TArray<TInstancedStruct<FDISInventoryFilter>>& GetFilters() const;

	/* 슬롯에 등록된 모든 필터 목록 가변 참조 반환 */
	TArray<TInstancedStruct<FDISInventoryFilter>>& GetMutableFilters();

	/* 가상 필터링 평가 */
public:
	/* 지정된 아이템 스택이 슬롯 필터를 통과하는지 검증 */
	UFUNCTION(Category = "DIS|Shared Slot", BlueprintCallable, BlueprintPure)
	virtual bool CanAcceptItemStack(const UDISItemStack* InItemStack) const;

	/* 소유권 감시 이벤트 핸들러 */
protected:
	/* 참조 중인 아이템 스택의 소유 인벤토리가 변경되거나 해제될 때 호출되는 핸들러 */
	UFUNCTION()
	void HandleObservedStackOwnershipChanged(UDISItemStack* ChangedStack, UDISInventory* NewOwningInventory);

	/* 런타임 멤버 상태 프로퍼티 */
protected:
	/* 인벤토리 내 슬롯 고유 인덱스 */
	UPROPERTY(Category = "Shared Inventory Slot", VisibleAnywhere, BlueprintReadOnly)
	int32 SlotIndex;

	/* 슬롯이 참조하고 있는 아이템 스택 객체 */
	UPROPERTY(Category = "Shared Inventory Slot", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDISItemStack> ItemStack;

	/* 슬롯 전용 등록 필터 목록 (TInstancedStruct 다형성 지원) */
	UPROPERTY(Category = "Shared Inventory Slot", VisibleAnywhere, BlueprintReadOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FDISInventoryFilter>> Filters;

	/* 슬롯 등록 시점 또는 소유권 획득 시점의 소유 인벤토리 약참조 */
	UPROPERTY(Category = "Shared Inventory Slot", BlueprintReadOnly)
	TWeakObjectPtr<UDISInventory> RegisteredOwningInventory;
};
