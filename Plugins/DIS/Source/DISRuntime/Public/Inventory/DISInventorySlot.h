/*****************************************************************//**
 * @file   DISInventorySlot.h
 * @brief  DIS 인벤토리의 개별 슬롯(한 칸) 런타임 UObject 클래스 정의
 * @details IDISEnhanceableInterface를 구현하여 슬롯 자체 강화를 지원하고, FDISItemStack으로 아이템 묶음을 관리함
 * @date   2026-10-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Inventory/DISInventoryFilter.h"
#include "Enhancement/DISEnhanceableInterface.h"
#include "Item/DISItemStack.h"
#include "DISInventorySlot.generated.h"

class UDISItemEntry;
class UDISItemStack;
class UDISEnhancementContainer;

/**
 * 인벤토리 내 개별 한 칸 슬롯의 런타임 상태, 자체 강화, 필터링 및 아이템 스택을 관리하는 UObject 객체
 */
UCLASS(BlueprintType, meta = (DisplayName = "DIS Inventory Slot"))
class DISRUNTIME_API UDISInventorySlot : public UObject, public IDISEnhanceableInterface
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UDISInventorySlot();

	/* 초기화 */
public:
	/* 슬롯 고유 인덱스를 기반으로 슬롯 인스턴스 초기화 */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable)
	void InitializeSlot(int32 InSlotIndex);

	/* IDISEnhanceableInterface 인터페이스 구현 (슬롯 자체 강화) */
public:
	/* 슬롯 강화를 전담 관리하는 컨테이너 객체 포인터 반환 */
	virtual UDISEnhancementContainer* GetEnhancementContainer() const override;

	/* 슬롯 식별 및 상태 조회 */
public:
	/* 인벤토리 내 슬롯 고유 인덱스 반환 */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable, BlueprintPure)
	int32 GetSlotIndex() const;

	/* 슬롯이 비어있는지 여부 반환 */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable, BlueprintPure)
	bool IsEmpty() const;

	/* 현재 슬롯에 보관된 아이템 스택 수량 반환 */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable, BlueprintPure)
	int32 GetStackCount() const;

	/* 슬롯의 최상단(사용 우선순위 최고 / 최저 내구도) 아이템 엔트리 반환 */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable, BlueprintPure)
	UDISItemEntry* GetItemEntry() const;

	/* 슬롯이 참조하고 있는 아이템 스택 인스턴스 반환 (없으면 nullptr) */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable, BlueprintPure)
	UDISItemStack* GetItemStack() const;

	/* 슬롯에 아이템 스택 인스턴스 직접 바인딩 (공간 점유 다중 슬롯 공유 시 활용) */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable)
	void SetItemStack(UDISItemStack* InItemStack);

	/* 아이템 상태 조작 */
public:
	/* 슬롯에 단일 아이템 엔트리 배치 (기존 아이템을 비우고 새로 설정) */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable)
	bool SetItemEntry(UDISItemEntry* InItemEntry);

	/* 슬롯에 아이템 엔트리를 추가 (비어있으면 신규 배치, 이미 있으면 스택 결합) */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable)
	bool PushItemEntry(UDISItemEntry* InItemEntry);

	/* 슬롯의 최상단 아이템 1개를 꺼내고 반환 */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable)
	UDISItemEntry* PopItemEntry();

	/* 슬롯을 완전히 비우고 최상단 아이템 엔트리 반환 */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable)
	UDISItemEntry* ClearSlot();

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
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable)
	void ClearFilters();

	/* 슬롯에 등록된 모든 필터 목록 반환 */
	const TArray<TInstancedStruct<FDISInventoryFilter>>& GetFilters() const;

	/* 슬롯에 등록된 모든 필터 목록 가변 참조 반환 */
	TArray<TInstancedStruct<FDISInventoryFilter>>& GetMutableFilters();

	/* 가상 필터링 평가 */
public:
	/* 지정된 아이템이 슬롯에 진입 가능한지 검증 (필터 검사 및 스택 결합 가능 여부 확인) */
	UFUNCTION(Category = "DIS|Slot", BlueprintCallable, BlueprintPure)
	virtual bool CanAcceptItem(const UDISItemEntry* InItemEntry) const;

	/* 내부 강화 대상 바인딩 헬퍼 */
protected:
	/* 특정 아이템 엔트리를 슬롯 강화 컨테이너의 EffectTarget으로 등록 */
	void BindItemEntryEnhancement(UDISItemEntry* InItemEntry);

	/* 특정 아이템 엔트리를 슬롯 강화 컨테이너의 EffectTarget에서 해제 */
	void UnbindItemEntryEnhancement(UDISItemEntry* InItemEntry);

	/* 특정 아이템 스택의 모든 엔트리들을 슬롯 강화 컨테이너의 EffectTargets로 일괄 등록 */
	void BindItemStackEnhancement(UDISItemStack* InItemStack);

	/* 특정 아이템 스택의 모든 엔트리들을 슬롯 강화 컨테이너의 EffectTargets에서 일괄 해제 */
	void UnbindItemStackEnhancement(UDISItemStack* InItemStack);

	/* 런타임 멤버 상태 프로퍼티 */
protected:
	/* 인벤토리 내 슬롯 고유 인덱스 */
	UPROPERTY(Category = "Inventory Slot", VisibleAnywhere, BlueprintReadOnly)
	int32 SlotIndex;

	/* 슬롯이 참조하고 있는 아이템 스택 객체 (복수 슬롯이 동일 스택 인스턴스를 공유 가능) */
	UPROPERTY(Category = "Inventory Slot", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDISItemStack> ItemStack;

	/* 슬롯 자체 강화 컨테이너 */
	UPROPERTY(Category = "Inventory Slot", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDISEnhancementContainer> EnhancementContainer;

	/* 슬롯 전용 등록 필터 목록 (TInstancedStruct 다형성 지원) */
	UPROPERTY(Category = "Inventory Slot", VisibleAnywhere, BlueprintReadOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FDISInventoryFilter>> Filters;
};
