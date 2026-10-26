/*****************************************************************//**
 * @file   DISInventory.h
 * @brief  DIS 슬롯 배열을 소유하고 관리하는 런타임 인벤토리 클래스 정의
 * @details TInstancedStruct 기반 슬롯 및 필터 컬렉션을 총괄하는 핵심 인벤토리 객체
 * @date   2026-10-14
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Inventory/DISInventoryFilter.h"
#include "DISInventory.generated.h"

class UDISInventoryDefinition;
class UDISItemEntry;
class UDISItemStack;
class UDISInventorySlot;

/** 슬롯 아이템 배치 및 스태킹 조작 결과 상태 */
UENUM(BlueprintType)
enum class EDISItemPlacementResult : uint8
{
	/** 실패 (배치 공간 없음, 슬롯 규칙 위반, 또는 모든 스택이 꽉 참) */
	Failed UMETA(DisplayName = "Failed"),

	/** 신규 빈 슬롯 공간에 온전히 배치됨 */
	PlacedNew UMETA(DisplayName = "Placed New"),

	/** 기존 스택에 100% 완전히 흡수 병합됨 (진입 스택 소진, 잔여물 없음) */
	FullyMerged UMETA(DisplayName = "Fully Merged"),

	/** 기존 스택에 일부만 흡수 병합됨 (진입 스택에 잔여물 남아있음) */
	PartiallyMerged UMETA(DisplayName = "Partially Merged"),

	/** 스태킹 가능한 대상이 2개 이상이어서 사용자 팝업 선택이 필요함 */
	RequiresSelection UMETA(DisplayName = "Requires Selection")
};

/** 슬롯 아이템 배치 및 스태킹 상세 결과 데이터 구조체 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISItemPlacementResult
{
	GENERATED_BODY()

	/* 함수 먼저 선언 */
public:
	/** C++ 조건식 if (Result) 및 TestTrue 편의용 암시적 bool 변환 (조작 성공 여부) */
	operator bool() const
	{
		return bSuccess;
	}

	/* 멤버 변수 */
public:
	/** 조작 결과 상태 유형 */
	UPROPERTY(Category = "DIS|Inventory", VisibleAnywhere, BlueprintReadOnly)
	EDISItemPlacementResult ResultType = EDISItemPlacementResult::Failed;

	/** 조작 성공 여부 (PlacedNew, FullyMerged, PartiallyMerged인 경우 true) */
	UPROPERTY(Category = "DIS|Inventory", VisibleAnywhere, BlueprintReadOnly)
	bool bSuccess = false;

	/** 대상 스택으로 이전/병합된 아이템 수량 */
	UPROPERTY(Category = "DIS|Inventory", VisibleAnywhere, BlueprintReadOnly)
	int32 MergedCount = 0;

	/** 진입 스택(InItemStack)에 남아있는 잔여 수량 (PartiallyMerged 시 > 0) */
	UPROPERTY(Category = "DIS|Inventory", VisibleAnywhere, BlueprintReadOnly)
	int32 RemainingCount = 0;

	/** 병합 대상이 된 기존 스택 (스태킹된 경우) */
	UPROPERTY(Category = "DIS|Inventory", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDISItemStack> TargetStack = nullptr;

	/** 다중 충돌 시 선택 가능한 스태킹 후보 목록 (RequiresSelection 상태일 때 채워짐) */
	UPROPERTY(Category = "DIS|Inventory", VisibleAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UDISItemStack>> CandidateStacks;
};

/**
 * 슬롯 1개 이상을 소유하고 인벤토리 전반의 런타임 상태 및 규칙을 관리하는 객체
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "DIS Inventory"))
class DISRUNTIME_API UDISInventory : public UObject
{
	GENERATED_BODY()

public:
	UDISInventory();

	/* 초기화 */
public:
	/* 정적 인벤토리 정의 및 초기 슬롯 수량을 기반으로 인벤토리 인스턴스 초기화 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	void InitializeInventory(const UDISInventoryDefinition* InDefinition, int32 InitialSlotCount = -1);

	/* 연결된 정적 인벤토리 정의(DataAsset) 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	const UDISInventoryDefinition* GetInventoryDefinition() const;

	/* 무게 시스템 관리 */
public:
	/* 인벤토리에 보관된 모든 아이템의 현재 총 적재 무게 계산 및 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	float GetCurrentWeight() const;

	/* 적용 중인 최종 최대 적재 허용 무게 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	float GetMaxWeight() const;

	/* 런타임 최대 허용 무게 설정 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	void SetMaxWeight(float NewMaxWeight);

	/* 런타임 최대 허용 무게 초기화 (정적 정의 기본값으로 복귀) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	void ResetMaxWeight();

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
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	void ClearFilters();

	/* 인벤토리에 등록된 모든 필터 목록 반환 */
	const TArray<TInstancedStruct<FDISInventoryFilter>>& GetFilters() const;

	/* 인벤토리에 등록된 모든 필터 목록 가변 참조 반환 */
	TArray<TInstancedStruct<FDISInventoryFilter>>& GetMutableFilters();

	/* 가상 필터링 및 배치 평가 */
public:
	/* 아이템 스택이 인벤토리 전역 규칙(무게 제한, 인벤토리 필터)을 통과하는지 검증 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	virtual bool CanAcceptItemStack(const UDISItemStack* InItemStack) const;

	/* 아이템 스택이 인벤토리 전역 규칙, 슬롯 필터, 블록 타일 및 점유 영역 충돌을 모두 통과하는지 검증 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	virtual bool CanAcceptItemStackAtSlot(const UDISItemStack* InItemStack, int32 TargetSlotIndex) const;

	/* 특정 슬롯에 아이템 스택 배치 시도 시 점유 영역과 겹치며 스태킹 병합이 가능한 기존 스택 목록 반환 (UI 팝업 선택 지원) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	TArray<UDISItemStack*> GetStackableCandidatesAtSlot(const UDISItemStack* InItemStack, int32 TargetSlotIndex) const;

	/* 슬롯 직접 접근 및 관리 */
public:
	/* 현재 소유 중인 총 슬롯 개수 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	int32 GetSlotCount() const;

	/* 특정 인덱스의 슬롯 객체 포인터 반환 (유효하지 않은 인덱스 시 nullptr) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	UDISInventorySlot* GetSlot(int32 SlotIndex);

	/* 특정 인덱스의 슬롯 객체 상수 포인터 반환 (유효하지 않은 인덱스 시 nullptr) */
	const UDISInventorySlot* GetSlot(int32 SlotIndex) const;

	/* 소유 중인 전체 슬롯 객체 인스턴스 배열 반환 */
	const TArray<TObjectPtr<UDISInventorySlot>>& GetSlots() const;

	/* 소유 중인 전체 슬롯 객체 인스턴스 배열 가변 참조 반환 */
	TArray<TObjectPtr<UDISInventorySlot>>& GetMutableSlots();

	/* 슬롯 용량 동적 조절 (안전 버전: 퇴출 발생 시 실패) */
public:
	/* 슬롯 총 개수를 NewSlotCount로 리사이즈 (슬롯 삭제나 크기 변화로 아이템이 퇴출되어야 하는 경우 실패) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool ResizeSlots(int32 NewSlotCount);

	/* 슬롯 개수를 AdditionalSlots만큼 추가 확장 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool AddSlots(int32 AdditionalSlots);

	/* 슬롯 개수를 뒤에서부터 CountToRemove만큼 축소 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool RemoveSlots(int32 CountToRemove);

	/* 슬롯 총 개수를 인벤토리 정적 정의의 기본 용량(DefaultCapacity)으로 복구 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool ResetSlotsToDefault();

	/* 슬롯 용량 확장 (AddSlots 위임 래퍼 API) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool ExpandCapacity(int32 AdditionalSlots);

	/* 슬롯 용량 강제 조절 (강제 버전: 부적합 아이템 스택 퇴출 후 반환) */
public:
	/* 슬롯 총 개수를 NewSlotCount로 강제 리사이즈하고, 공간 부족 또는 PlacementRule 불일치로 퇴출된 아이템 스택 목록 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool ForceResizeSlots(int32 NewSlotCount, TArray<UDISItemStack*>& OutEvictedStacks);

	/* 슬롯 개수를 AdditionalSlots만큼 강제 확장하고, PlacementRule 불일치로 퇴출된 아이템 스택 목록 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool ForceAddSlots(int32 AdditionalSlots, TArray<UDISItemStack*>& OutEvictedStacks);

	/* 슬롯 개수를 뒤에서부터 CountToRemove만큼 강제 축소하고, 퇴출된 아이템 스택 목록 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool ForceRemoveSlots(int32 CountToRemove, TArray<UDISItemStack*>& OutEvictedStacks);

	/* 슬롯 총 개수를 인벤토리 정적 정의의 기본 용량(DefaultCapacity)으로 강제 복구하고 퇴출된 아이템 스택 목록 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool ForceResetSlotsToDefault(TArray<UDISItemStack*>& OutEvictedStacks);

	/* 블루프린트 편의 슬롯 조작 래퍼 API */
public:
	/* 특정 슬롯에 보관된 아이템 스택 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	UDISItemStack* GetSlotItemStack(int32 SlotIndex) const;

	/* 특정 슬롯의 현재 스택 수량 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	int32 GetSlotStackCount(int32 SlotIndex) const;

	/* 특정 슬롯이 비어있는지 여부 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	bool IsSlotEmpty(int32 SlotIndex) const;

	/* 특정 슬롯에 아이템 스택 배치 또는 스태킹 병합 시도 후 상세 결과 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	FDISItemPlacementResult SetSlotItemStack(int32 SlotIndex, UDISItemStack* InItemStack, UDISItemStack* PreferredTargetStack = nullptr);

	/* 원본 아이템 스택(SourceStack)의 수량을 대상 아이템 스택(TargetStack)으로 이전 병합하고 슬롯 강화를 동기화 (병합된 수량 반환) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	int32 MergeItemStackInto(UDISItemStack* SourceStack, UDISItemStack* TargetStack);

	/* 특정 슬롯을 비우고 보관 중이던 아이템 스택 반환 (다중 슬롯 점유 시 모든 점유 타일에서 일괄 분리) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	UDISItemStack* ClearSlot(int32 SlotIndex);

	/* 특정 아이템 스택이 점유하고 있는 모든 슬롯을 일괄 해제하고 소유권 분리 (해제 성공 시 true 반환) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool ClearItemStack(UDISItemStack* InItemStack);

	/* 특정 슬롯이 블록된(막힌) 칸인지 여부 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	bool IsSlotBlocked(int32 SlotIndex) const;

	/* 아이템 스택 회전 조작 */
public:
	/* 특정 아이템 스택의 인벤토리 내 좌상단(Top-Left) 2D 그리드 기준 좌표 반환 (미배치 시 FIntPoint::NoneValue) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	FIntPoint GetItemStackBaseCoord(const UDISItemStack* InItemStack) const;

	/* 특정 아이템 스택의 인벤토리 내 좌상단(Top-Left) 루트 슬롯 인덱스 반환 (미배치 시 INDEX_NONE) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	int32 GetItemStackRootSlot(const UDISItemStack* InItemStack) const;

	/* 특정 아이템 스택이 현재 인벤토리에서 점유하고 있는 모든 슬롯 인덱스 배열 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	TArray<int32> GetItemStackOccupiedSlots(const UDISItemStack* InItemStack) const;

	/* 특정 아이템 스택을 현재 좌상단 슬롯 위치를 기준으로 회전시킬 수 있는지 사전 검증 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	bool CanRotateItemStack(const UDISItemStack* InItemStack, bool bClockwise = true) const;

	/* 특정 아이템 스택을 현재 좌상단 슬롯 위치를 기준으로 회전 명령 (성공 시 true, 불가 시 false) */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool RotateItemStack(UDISItemStack* InItemStack, bool bClockwise = true);

	/* 특정 슬롯에 보관된 아이템 스택을 현재 좌상단 슬롯 위치를 기준으로 회전 명령 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable)
	bool RotateItemStackAtSlot(int32 SlotIndex, bool bClockwise = true);

	/* 공간 위치 판별 및 검증 */
public:
	/* 특정 2D 그리드 좌표가 인벤토리 경계 밖이거나 블록된 타일인지 여부 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	bool IsTileBlockedOrOutOfBounds(const FIntPoint& Coord) const;

	/* 특정 슬롯이 인벤토리 모서리(외곽 꼭짓점)이거나 BlockTile과 인접하여 구석을 형성한 칸인지 여부 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	bool IsSlotCorner(int32 SlotIndex) const;

	/* 특정 슬롯이 인벤토리 외곽 가장자리이거나 BlockTile과 1면 이상 맞닿은 경계 칸인지 여부 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	bool IsSlotEdge(int32 SlotIndex) const;

	/* 특정 슬롯이 인벤토리 경계선 및 BlockTile과 맞닿지 않은 내부 중앙 영역 칸인지 여부 반환 */
	UFUNCTION(Category = "DIS|Inventory", BlueprintCallable, BlueprintPure)
	bool IsSlotCenter(int32 SlotIndex) const;

	/* 슬롯 생성 가상 팩토리 메서드 */
protected:
	/* 지정된 슬롯 인덱스에 해당하는 슬롯 인스턴스 생성 (인벤토리 정의의 DefaultSlotClass 또는 서브클래스 오버라이드 지원) */
	virtual UDISInventorySlot* CreateSlotInstance(int32 SlotIndex);

	/* 런타임 멤버 상태 프로퍼티 */
protected:
	/* 인벤토리 정적 정의 에셋 참조 (PrimaryDataAsset) */
	UPROPERTY(Category = "Inventory", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<const UDISInventoryDefinition> InventoryDefinition;

	/* 인벤토리의 현재 최대 적재 허용 무게 */
	UPROPERTY(Category = "Inventory", VisibleAnywhere, BlueprintReadOnly)
	float MaxWeight;

	/* 인벤토리 레벨 전역 등록 필터 목록 (TInstancedStruct 다형성 지원) */
	UPROPERTY(Category = "Inventory", VisibleAnywhere, BlueprintReadOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FDISInventoryFilter>> Filters;

	/* 소유 중인 개별 한 칸 슬롯 UObject 배열 */
	UPROPERTY(Category = "Inventory", VisibleAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UDISInventorySlot>> Slots;
};
