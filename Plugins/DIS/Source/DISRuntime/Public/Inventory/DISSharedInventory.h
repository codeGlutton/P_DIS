/*****************************************************************//**
 * @file   DISSharedInventory.h
 * @brief  DIS 1차원 선형 공유/단축(Shared/QuickSlot) 인벤토리 런타임 클래스 정의
 * @details 고정 슬롯 용량, 무충돌 단일 칸 등록, 슬롯 스왑 및 필터링을 지원하는 경량 인벤토리 객체
 * @date   2026-10-26
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Inventory/DISInventoryFilter.h"
#include "DISSharedInventory.generated.h"

class UDISSharedInventoryDefinition;
class UDISSharedInventorySlot;
class UDISItemStack;

/**
 * 퀵슬롯 및 단축창 등 1차원 선형 고정 슬롯을 관리하고, 외부 아이템 스택을 공유/참조하는 경량 인벤토리 객체
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "DIS Shared Inventory"))
class DISRUNTIME_API UDISSharedInventory : public UObject
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UDISSharedInventory();

	/* 초기화 */
public:
	/* 정적 공유 인벤토리 정의 에셋을 기반으로 고정 슬롯 인벤토리 인스턴스 초기화 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable)
	void InitializeInventory(const UDISSharedInventoryDefinition* InDefinition);

	/* 연결된 정적 공유 인벤토리 정의(DataAsset) 반환 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable, BlueprintPure)
	const UDISSharedInventoryDefinition* GetInventoryDefinition() const;

	/* 필터 관리 */
public:
	/* 인벤토리에 등록 가능한 TInstancedStruct 전역 필터 추가 */
	void AddFilter(const TInstancedStruct<FDISInventoryFilter>& InFilter);

	/* 템플릿 기반 구체 필터 생성 및 추가 */
	template <typename TFilterType, typename... TArgs>
	TFilterType& EmplaceFilter(TArgs&&... Args)
	{
		TInstancedStruct<FDISInventoryFilter> NewFilter = TInstancedStruct<FDISInventoryFilter>::Make<TFilterType>(Forward<TArgs>(Args)...);
		Filters.Add(NewFilter);
		return NewFilter.GetMutable<TFilterType>();
	}

	/* 인벤토리의 모든 필터 제거 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable)
	void ClearFilters();

	/* 인벤토리에 등록된 모든 필터 목록 반환 */
	const TArray<TInstancedStruct<FDISInventoryFilter>>& GetFilters() const;

	/* 인벤토리에 등록된 모든 필터 목록 가변 참조 반환 */
	TArray<TInstancedStruct<FDISInventoryFilter>>& GetMutableFilters();

	/* 가상 필터링 및 수용 평가 */
public:
	/* 아이템 스택이 인벤토리 전역 필터를 통과하는지 검증 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable, BlueprintPure)
	virtual bool CanAcceptItemStack(const UDISItemStack* InItemStack) const;

	/* 아이템 스택이 인벤토리 전역 필터 및 특정 슬롯 필터를 모두 통과하는지 검증 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable, BlueprintPure)
	virtual bool CanAcceptItemStackAtSlot(const UDISItemStack* InItemStack, int32 TargetSlotIndex) const;

	/* 슬롯 직접 접근 및 관리 */
public:
	/* 현재 소유 중인 총 고정 슬롯 개수 반환 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable, BlueprintPure)
	int32 GetSlotCount() const;

	/* 특정 인덱스의 슬롯 객체 포인터 반환 (유효하지 않은 인덱스 시 nullptr) */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable, BlueprintPure)
	UDISSharedInventorySlot* GetSlot(int32 SlotIndex);

	/* 특정 인덱스의 슬롯 객체 상수 포인터 반환 (유효하지 않은 인덱스 시 nullptr) */
	const UDISSharedInventorySlot* GetSlot(int32 SlotIndex) const;

	/* 소유 중인 전체 슬롯 객체 인스턴스 배열 반환 */
	const TArray<TObjectPtr<UDISSharedInventorySlot>>& GetSlots() const;

	/* 소유 중인 전체 슬롯 객체 인스턴스 배열 가변 참조 반환 */
	TArray<TObjectPtr<UDISSharedInventorySlot>>& GetMutableSlots();

	/* 블루프린트 편의 슬롯 조작 래퍼 API */
public:
	/* 특정 슬롯에 보관된 아이템 스택 반환 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable, BlueprintPure)
	UDISItemStack* GetSlotItemStack(int32 SlotIndex) const;

	/* 특정 슬롯의 현재 스택 수량 반환 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable, BlueprintPure)
	int32 GetSlotStackCount(int32 SlotIndex) const;

	/* 특정 슬롯이 비어있는지 여부 반환 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable, BlueprintPure)
	bool IsSlotEmpty(int32 SlotIndex) const;

	/* 특정 슬롯에 아이템 스택 등록 (다중 타일 아이템도 단일 칸에 등록되며, 동일 스택이 이미 등록된 슬롯이 있다면 자동 해제되어 이동) */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable)
	bool SetSlotItemStack(int32 SlotIndex, UDISItemStack* InItemStack);

	/* 특정 슬롯을 비우고 보관 중이던 아이템 스택 반환 */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable)
	UDISItemStack* ClearSlot(int32 SlotIndex);

	/* 두 슬롯 간 아이템 스택 위치 맞교환 (필터 통과 검증 후 원자적 교환) */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable)
	bool SwapSlots(int32 SlotIndexA, int32 SlotIndexB);

	/* 특정 아이템 스택이 현재 인벤토리의 몇 번 슬롯에 등록되어 있는지 검색 (미등록 시 INDEX_NONE 반환) */
	UFUNCTION(Category = "DIS|Shared Inventory", BlueprintCallable, BlueprintPure)
	int32 FindSlotWithItemStack(const UDISItemStack* InItemStack) const;

	/* 슬롯 생성 가상 팩토리 메서드 */
protected:
	/* 지정된 슬롯 인덱스에 해당하는 슬롯 인스턴스 생성 */
	virtual UDISSharedInventorySlot* CreateSlotInstance(int32 SlotIndex);

	/* 런타임 멤버 상태 프로퍼티 */
protected:
	/* 공유 인벤토리 정적 정의 에셋 참조 (PrimaryDataAsset) */
	UPROPERTY(Category = "Shared Inventory", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<const UDISSharedInventoryDefinition> InventoryDefinition;

	/* 공유 인벤토리 레벨 전역 등록 필터 목록 (TInstancedStruct 다형성 지원) */
	UPROPERTY(Category = "Shared Inventory", VisibleAnywhere, BlueprintReadOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FDISInventoryFilter>> Filters;

	/* 소유 중인 개별 한 칸 슬롯 UObject 배열 */
	UPROPERTY(Category = "Shared Inventory", VisibleAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UDISSharedInventorySlot>> Slots;
};
