#include "Inventory/DISInventory.h"
#include "Inventory/DISInventoryDefinition.h"
#include "Inventory/DISInventorySlot.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemDefinition.h"
#include "Item/DISItemPlacementRule.h"
#include "Item/DISItemStack.h"

UDISInventory::UDISInventory() :
	InventoryDefinition(nullptr),
	MaxWeight(0.0f),
	Filters(),
	Slots()
{
}

void UDISInventory::InitializeInventory(const UDISInventoryDefinition* InDefinition, int32 InitialSlotCount)
{
	InventoryDefinition = InDefinition;

	// 최대 허용 무게 기본값 동기화
	if (InventoryDefinition != nullptr)
	{
		MaxWeight = InventoryDefinition->DefaultMaxWeight;
	}
	else
	{
		MaxWeight = 0.0f;
	}

	// 정적 인벤토리 정의 에셋의 기본 필터 목록 복사 초기화
	if (InventoryDefinition != nullptr)
	{
		Filters = InventoryDefinition->DefaultFilters;
	}
	else
	{
		Filters.Empty();
	}

	int32 TargetCapacity = 0;
	if (InitialSlotCount > 0)
	{
		TargetCapacity = InitialSlotCount;
	}
	else if (InventoryDefinition != nullptr)
	{
		TargetCapacity = InventoryDefinition->DefaultCapacity;
	}

	// 최대 허용 용량 제한 적용
	if (InventoryDefinition != nullptr && InventoryDefinition->MaxExpandableCapacity > 0)
	{
		TargetCapacity = FMath::Min(TargetCapacity, InventoryDefinition->MaxExpandableCapacity);
	}

	// TInstancedStruct 슬롯 배열 동적 생성 및 슬롯 인덱스 초기화
	Slots.Empty(TargetCapacity);
	for (int32 Index = 0; Index < TargetCapacity; ++Index)
	{
		Slots.Add(CreateSlotInstance(Index));
	}
}

const UDISInventoryDefinition* UDISInventory::GetInventoryDefinition() const
{
	return InventoryDefinition.Get();
}

float UDISInventory::GetCurrentWeight() const
{
	// 정적 정의에서 무게 시스템을 사용하지 않는 경우 0.0 반환
	if (InventoryDefinition != nullptr && InventoryDefinition->bUseWeightSystem == false)
	{
		return 0.0f;
	}

	float TotalWeight = 0.0f;
	TSet<const UDISItemStack*> CountedStacks;

	for (const TObjectPtr<UDISInventorySlot>& Slot : Slots)
	{
		if (Slot != nullptr && Slot->IsEmpty() == false)
		{
			const UDISItemStack* Stack = Slot->GetItemStack();
			if (Stack != nullptr)
			{
				// 다중 슬롯 점유 아이템의 중복 무게 합산 방지
				if (CountedStacks.Contains(Stack) == false)
				{
					CountedStacks.Add(Stack);
					TotalWeight += Stack->GetTotalWeight();
				}
			}
		}
	}

	return TotalWeight;
}

float UDISInventory::GetMaxWeight() const
{
	return MaxWeight;
}

void UDISInventory::SetMaxWeight(float NewMaxWeight)
{
	MaxWeight = FMath::Max(0.0f, NewMaxWeight);
}

void UDISInventory::ResetMaxWeight()
{
	if (InventoryDefinition != nullptr)
	{
		MaxWeight = InventoryDefinition->DefaultMaxWeight;
	}
	else
	{
		MaxWeight = 0.0f;
	}
}

void UDISInventory::AddFilter(const TInstancedStruct<FDISInventoryFilter>& InFilter)
{
	if (InFilter.IsValid() == true)
	{
		Filters.Add(InFilter);
	}
}

void UDISInventory::ClearFilters()
{
	Filters.Empty();
}

const TArray<TInstancedStruct<FDISInventoryFilter>>& UDISInventory::GetFilters() const
{
	return Filters;
}

TArray<TInstancedStruct<FDISInventoryFilter>>& UDISInventory::GetMutableFilters()
{
	return Filters;
}

bool UDISInventory::CanAcceptItemStack(const UDISItemStack* InItemStack) const
{
	if (InItemStack == nullptr || InItemStack->IsEmpty() == true)
	{
		return false;
	}

	// 무게 시스템 검증: 추가 시 최대 적재 무게 초과 여부 검사
	if (InventoryDefinition != nullptr && InventoryDefinition->bUseWeightSystem == true)
	{
		float BaseWeight = GetCurrentWeight();

		// 이미 이 인벤토리의 슬롯에 등록되어 있던 스택이면 기존 등록 무게 차감 (순증분만 평가)
		for (const TObjectPtr<UDISInventorySlot>& SlotPtr : Slots)
		{
			if (SlotPtr != nullptr && SlotPtr->GetItemStack() == InItemStack)
			{
				BaseWeight -= InItemStack->GetTotalWeight();
				break;
			}
		}

		if (BaseWeight + InItemStack->GetTotalWeight() > GetMaxWeight())
		{
			return false;
		}
	}

	// 인벤토리 레벨에 등록된 모든 필터 검사
	for (const TInstancedStruct<FDISInventoryFilter>& Filter : Filters)
	{
		if (Filter.IsValid() == true)
		{
			const FDISInventoryFilter& FilterRef = Filter.Get<FDISInventoryFilter>();
			if (FilterRef.PassesFilter(InItemStack) == false)
			{
				return false;
			}
		}
	}

	return true;
}

bool UDISInventory::CanAcceptItemStackAtSlot(const UDISItemStack* InItemStack, int32 TargetSlotIndex) const
{
	if (InventoryDefinition == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UDISInventory::CanAcceptItemStackAtSlot - InventoryDefinition is null."));
		return false;
	}

	if (InItemStack == nullptr || InItemStack->IsEmpty() == true)
	{
		return false;
	}

	// 먼저 인벤토리 전역 규칙(무게 및 전역 필터) 통과 여부 검증
	if (CanAcceptItemStack(InItemStack) == false)
	{
		return false;
	}

	if (Slots.IsValidIndex(TargetSlotIndex) == false)
	{
		return false;
	}

	const UDISItemEntry* TopEntry = InItemStack->GetTopEntry();
	if (TopEntry == nullptr)
	{
		return false;
	}

	// 점유 타일 상대 오프셋 목록 획득 (형상이 비어있으면 데이터 이상으로 판단하여 에러 로그 출력 및 거부)
	const TArray<FIntPoint> OccupiedPoints = InItemStack->GetOccupiedPoints();
	if (OccupiedPoints.IsEmpty() == true)
	{
		UE_LOG(LogTemp, Error, TEXT("UDISInventory::CanAcceptItemStackAtSlot - OccupiedPoints is empty for ItemStack. TargetSlotIndex: %d"), TargetSlotIndex);
		return false;
	}

	const FIntPoint BaseCoord = InventoryDefinition->SlotIndexToCoord(TargetSlotIndex);
	if (BaseCoord == FIntPoint::NoneValue)
	{
		return false;
	}

	// 점유 타일들 중 이미 다른 아이템 스택이 차지하고 있는 기존 스택 수집 (중복 제외)
	TSet<const UDISItemStack*> OverlappingStacks;

	// 점유할 모든 타일에 대해 경계/블록 여부, 슬롯 유효성, 슬롯 필터, 기존 아이템 점유 충돌 검증
	for (const FIntPoint& Offset : OccupiedPoints)
	{
		const FIntPoint TileCoord = BaseCoord + Offset;

		// 대상 타일이 인벤토리 경계 밖이거나 블록된 타일인지 검사
		if (IsTileBlockedOrOutOfBounds(TileCoord) == true)
		{
			return false;
		}

		const int32 CurrentSlotIndex = InventoryDefinition->CoordToSlotIndex(TileCoord);
		if (Slots.IsValidIndex(CurrentSlotIndex) == false)
		{
			return false;
		}

		const UDISInventorySlot* CurrentSlot = Slots[CurrentSlotIndex].Get();
		if (CurrentSlot == nullptr)
		{
			return false;
		}

		// 슬롯 자체 필터 검증
		for (const TInstancedStruct<FDISInventoryFilter>& Filter : CurrentSlot->GetFilters())
		{
			if (Filter.IsValid() == true)
			{
				const FDISInventoryFilter& FilterRef = Filter.Get<FDISInventoryFilter>();
				if (FilterRef.PassesFilter(InItemStack) == false)
				{
					return false;
				}
			}
		}

		// 기존 점유 아이템 충돌 수집 (자기 자신이 아닌 다른 스택이 이미 차지하고 있는 경우)
		const UDISItemStack* ExistingStack = CurrentSlot->GetItemStack();
		if (ExistingStack != nullptr && ExistingStack->IsEmpty() == false && ExistingStack != InItemStack)
		{
			OverlappingStacks.Add(ExistingStack);
		}
	}

	// 케이스 1: 겹치는 기존 스택이 있는 경우 (충돌 발생 시 스태킹 가능 여부 검증)
	if (OverlappingStacks.IsEmpty() == false)
	{
		// 겹친 기존 스택들 중 스태킹 가능하고 여유 공간(StackCount < MaxStackCount)이 남아있는 후보가 최소 하나라도 있으면 수용 허용
		for (const UDISItemStack* Stack : OverlappingStacks)
		{
			if (Stack != nullptr && Stack->CanStackWith(TopEntry) == true)
			{
				if (Stack->GetStackCount() < Stack->GetMaxStackCount())
				{
					return true;
				}
			}
		}

		// 스태킹 가능한 스택이 없거나, 후보들이 모두 이미 최대 수량에 도달해 있다면 거부
		return false;
	}

	// 케이스 2: 겹치는 기존 스택이 전혀 없는 경우: 신규 빈 슬롯 배치이므로 아이템 자체의 슬롯 배치 제한 규칙(Placement Rules) 검증
	FDISItemPlacementContext Context;
	Context.Inventory = this;
	Context.TargetSlotIndex = TargetSlotIndex;
	Context.TargetCoord = BaseCoord;
	Context.InventoryOwner = nullptr;

	if (TopEntry->CanPlaceAtSlot(Context) == false)
	{
		return false;
	}

	return true;
}

TArray<UDISItemStack*> UDISInventory::GetStackableCandidatesAtSlot(const UDISItemStack* InItemStack, int32 TargetSlotIndex) const
{
	TArray<UDISItemStack*> Candidates;
	if (InventoryDefinition == nullptr || InItemStack == nullptr || InItemStack->IsEmpty() == true)
	{
		return Candidates;
	}

	if (Slots.IsValidIndex(TargetSlotIndex) == false)
	{
		return Candidates;
	}

	const UDISItemEntry* TopEntry = InItemStack->GetTopEntry();
	if (TopEntry == nullptr)
	{
		return Candidates;
	}

	const TArray<FIntPoint> OccupiedPoints = InItemStack->GetOccupiedPoints();
	if (OccupiedPoints.IsEmpty() == true)
	{
		return Candidates;
	}

	const FIntPoint BaseCoord = InventoryDefinition->SlotIndexToCoord(TargetSlotIndex);
	if (BaseCoord == FIntPoint::NoneValue)
	{
		return Candidates;
	}

	TSet<UDISItemStack*> UniqueCandidates;
	for (const FIntPoint& Offset : OccupiedPoints)
	{
		const FIntPoint TileCoord = BaseCoord + Offset;
		if (IsTileBlockedOrOutOfBounds(TileCoord) == true)
		{
			continue;
		}

		const int32 CurrentSlotIndex = InventoryDefinition->CoordToSlotIndex(TileCoord);
		if (Slots.IsValidIndex(CurrentSlotIndex) == true)
		{
			const UDISInventorySlot* Slot = Slots[CurrentSlotIndex].Get();
			if (Slot != nullptr)
			{
				UDISItemStack* ExistingStack = Slot->GetItemStack();
				if (ExistingStack != nullptr && ExistingStack->IsEmpty() == false && ExistingStack != InItemStack)
				{
					if (ExistingStack->CanStackWith(TopEntry) == true && ExistingStack->GetStackCount() < ExistingStack->GetMaxStackCount())
					{
						UniqueCandidates.Add(ExistingStack);
					}
				}
			}
		}
	}

	return UniqueCandidates.Array();
}

int32 UDISInventory::GetSlotCount() const
{
	return Slots.Num();
}

UDISInventorySlot* UDISInventory::GetSlot(int32 SlotIndex)
{
	if (Slots.IsValidIndex(SlotIndex) == true)
	{
		return Slots[SlotIndex].Get();
	}

	return nullptr;
}

const UDISInventorySlot* UDISInventory::GetSlot(int32 SlotIndex) const
{
	if (Slots.IsValidIndex(SlotIndex) == true)
	{
		return Slots[SlotIndex].Get();
	}

	return nullptr;
}

const TArray<TObjectPtr<UDISInventorySlot>>& UDISInventory::GetSlots() const
{
	return Slots;
}

TArray<TObjectPtr<UDISInventorySlot>>& UDISInventory::GetMutableSlots()
{
	return Slots;
}

bool UDISInventory::ResizeSlots(int32 NewSlotCount)
{
	// 0 미만 슬롯 크기는 허용하지 않음
	if (NewSlotCount < 0)
	{
		return false;
	}

	// 최대 확장 가능 용량 한도 초과 검증
	if (InventoryDefinition != nullptr && InventoryDefinition->MaxExpandableCapacity > 0)
	{
		if (NewSlotCount > InventoryDefinition->MaxExpandableCapacity)
		{
			return false;
		}
	}

	const int32 CurrentCapacity = Slots.Num();
	if (NewSlotCount == CurrentCapacity)
	{
		return true;
	}

	// 1. 축소 시 잘려나갈 슬롯에 아이템이 보관되어 있으면 퇴출을 허용하지 않는 안전 모드이므로 실패 처리
	if (NewSlotCount < CurrentCapacity)
	{
		for (int32 Index = NewSlotCount; Index < CurrentCapacity; ++Index)
		{
			if (Slots[Index] != nullptr && Slots[Index]->IsEmpty() == false)
			{
				return false;
			}
		}
	}

	// 2. 크기 변경 후, 잔여 슬롯의 아이템들이 새 그리드 규격에서 PlacementRule을 만족하는지 가상 사전 검증
	// 인벤토리 공간 판정 함수(IsTileBlockedOrOutOfBounds 등)가 Slots.Num()을 참조하므로 일시적 슬롯 크기 조정 후 검사
	TArray<TObjectPtr<UDISInventorySlot>> OriginalSlots = Slots;
	if (NewSlotCount > CurrentCapacity)
	{
		Slots.SetNumZeroed(NewSlotCount);
		for (int32 Index = 0; Index < CurrentCapacity; ++Index)
		{
			Slots[Index] = OriginalSlots[Index];
		}
	}
	else
	{
		Slots.SetNum(NewSlotCount);
	}

	bool bAllItemsValid = true;
	const int32 CheckCount = FMath::Min(CurrentCapacity, NewSlotCount);
	TSet<const UDISItemStack*> CheckedStacks;
	for (int32 Index = 0; Index < CheckCount; ++Index)
	{
		const UDISInventorySlot* Slot = Slots[Index].Get();
		if (Slot != nullptr && Slot->IsEmpty() == false)
		{
			const UDISItemStack* Stack = Slot->GetItemStack();
			if (Stack != nullptr && CheckedStacks.Contains(Stack) == false)
			{
				CheckedStacks.Add(Stack);
				if (CanAcceptItemStackAtSlot(Stack, Index) == false)
				{
					bAllItemsValid = false;
					break;
				}
			}
		}
	}

	// 원래 상태로 복원
	Slots = MoveTemp(OriginalSlots);

	// 규칙 위반으로 퇴출되어야 하는 아이템이 발생하면 실패 처리
	if (bAllItemsValid == false)
	{
		return false;
	}

	// 3. 실제 슬롯 배열 적용
	if (NewSlotCount > CurrentCapacity)
	{
		Slots.Reserve(NewSlotCount);
		for (int32 Index = CurrentCapacity; Index < NewSlotCount; ++Index)
		{
			Slots.Add(CreateSlotInstance(Index));
		}
	}
	else
	{
		Slots.SetNum(NewSlotCount);
	}

	return true;
}

bool UDISInventory::AddSlots(int32 AdditionalSlots)
{
	if (AdditionalSlots <= 0)
	{
		return false;
	}

	return ResizeSlots(Slots.Num() + AdditionalSlots);
}

bool UDISInventory::RemoveSlots(int32 CountToRemove)
{
	if (CountToRemove <= 0)
	{
		return false;
	}

	return ResizeSlots(Slots.Num() - CountToRemove);
}

bool UDISInventory::ResetSlotsToDefault()
{
	if (InventoryDefinition == nullptr)
	{
		return false;
	}

	return ResizeSlots(InventoryDefinition->DefaultCapacity);
}

bool UDISInventory::ExpandCapacity(int32 AdditionalSlots)
{
	return AddSlots(AdditionalSlots);
}

bool UDISInventory::ForceResizeSlots(int32 NewSlotCount, TArray<UDISItemStack*>& OutEvictedStacks)
{
	// 0 미만 슬롯 크기는 허용하지 않음
	if (NewSlotCount < 0)
	{
		return false;
	}

	// 최대 확장 가능 용량 한도 초과 검증
	if (InventoryDefinition != nullptr && InventoryDefinition->MaxExpandableCapacity > 0)
	{
		if (NewSlotCount > InventoryDefinition->MaxExpandableCapacity)
		{
			return false;
		}
	}

	const int32 CurrentCapacity = Slots.Num();
	TSet<UDISItemStack*> EvictedStackSet;

	// 1. 슬롯 축소로 인해 잘려나가는 슬롯들의 모든 아이템 스택을 퇴출 목록으로 수집하고 슬롯 비우기
	if (NewSlotCount < CurrentCapacity)
	{
		for (int32 Index = NewSlotCount; Index < CurrentCapacity; ++Index)
		{
			UDISInventorySlot* Slot = Slots[Index].Get();
			if (Slot != nullptr && Slot->IsEmpty() == false)
			{
				UDISItemStack* Stack = Slot->GetItemStack();
				if (Stack != nullptr)
				{
					EvictedStackSet.Add(Stack);
					ClearSlot(Index);
				}
			}
		}
		Slots.SetNum(NewSlotCount);
	}
	else if (NewSlotCount > CurrentCapacity)
	{
		Slots.Reserve(NewSlotCount);
		for (int32 Index = CurrentCapacity; Index < NewSlotCount; ++Index)
		{
			Slots.Add(CreateSlotInstance(Index));
		}
	}

	// 2. 새로운 그리드 규격에서 각 슬롯의 PlacementRule 및 영역 유효성 검사
	// (슬롯 추가나 삭제로 인해 그리드 외곽선/중앙 판정이 바뀌어 더 이상 해당 슬롯에 머무를 수 없는 아이템 퇴출)
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		UDISInventorySlot* Slot = Slots[Index].Get();
		if (Slot != nullptr && Slot->IsEmpty() == false)
		{
			UDISItemStack* Stack = Slot->GetItemStack();
			if (Stack != nullptr)
			{
				if (CanAcceptItemStackAtSlot(Stack, Index) == false)
				{
					EvictedStackSet.Add(Stack);
					ClearSlot(Index);
				}
			}
		}
	}

	OutEvictedStacks = EvictedStackSet.Array();
	return true;
}

bool UDISInventory::ForceAddSlots(int32 AdditionalSlots, TArray<UDISItemStack*>& OutEvictedStacks)
{
	if (AdditionalSlots <= 0)
	{
		return false;
	}

	return ForceResizeSlots(Slots.Num() + AdditionalSlots, OutEvictedStacks);
}

bool UDISInventory::ForceRemoveSlots(int32 CountToRemove, TArray<UDISItemStack*>& OutEvictedStacks)
{
	if (CountToRemove <= 0)
	{
		return false;
	}

	return ForceResizeSlots(Slots.Num() - CountToRemove, OutEvictedStacks);
}

bool UDISInventory::ForceResetSlotsToDefault(TArray<UDISItemStack*>& OutEvictedStacks)
{
	if (InventoryDefinition == nullptr)
	{
		return false;
	}

	return ForceResizeSlots(InventoryDefinition->DefaultCapacity, OutEvictedStacks);
}

UDISItemStack* UDISInventory::GetSlotItemStack(int32 SlotIndex) const
{
	const UDISInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->GetItemStack();
	}

	return nullptr;
}

int32 UDISInventory::GetSlotStackCount(int32 SlotIndex) const
{
	const UDISInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->GetStackCount();
	}

	return 0;
}

bool UDISInventory::IsSlotEmpty(int32 SlotIndex) const
{
	const UDISInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->IsEmpty();
	}

	return true;
}

FDISItemPlacementResult UDISInventory::SetSlotItemStack(int32 SlotIndex, UDISItemStack* InItemStack, UDISItemStack* PreferredTargetStack)
{
	FDISItemPlacementResult Result;
	Result.ResultType = EDISItemPlacementResult::Failed;
	Result.bSuccess = false;

	if (InItemStack == nullptr)
	{
		ClearSlot(SlotIndex);
		Result.bSuccess = true;
		Result.ResultType = EDISItemPlacementResult::PlacedNew;
		return Result;
	}

	// 배치 가능 여부(영역, 블록 타일, 슬롯 필터, PlacementRule, 스태킹 가능성 및 충돌) 검증
	if (CanAcceptItemStackAtSlot(InItemStack, SlotIndex) == false)
	{
		return Result;
	}

	if (InventoryDefinition == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UDISInventory::SetSlotItemStack - InventoryDefinition is null."));
		return Result;
	}

	// 점유 타일 상대 오프셋 목록 획득 (형상이 비어있는 경우 배치 거부)
	const TArray<FIntPoint> OccupiedPoints = InItemStack->GetOccupiedPoints();
	if (OccupiedPoints.IsEmpty() == true)
	{
		UE_LOG(LogTemp, Error, TEXT("UDISInventory::SetSlotItemStack - OccupiedPoints is empty for ItemStack. SlotIndex: %d"), SlotIndex);
		return Result;
	}

	// 스태킹 가능한 기존 스택 후보 목록 조회 (여유 공간이 있는 스택들)
	const TArray<UDISItemStack*> Candidates = GetStackableCandidatesAtSlot(InItemStack, SlotIndex);

	UDISItemStack* TargetStackToMerge = nullptr;

	// 스태킹 처리 분기:
	// 1) 명시적 선호 타깃 스택(PreferredTargetStack)이 전달되었고 후보에 포함되어 있는 경우
	if (PreferredTargetStack != nullptr && Candidates.Contains(PreferredTargetStack) == true)
	{
		TargetStackToMerge = PreferredTargetStack;
	}
	// 2) 후보가 정확히 1개인 경우 -> 단일 대상 자동 스태킹 병합 실행
	else if (Candidates.Num() == 1)
	{
		TargetStackToMerge = Candidates[0];
	}
	// 3) 후보가 2개 이상인데 PreferredTargetStack이 지정되지 않은 경우: 유저 선택 팝업 필요
	else if (Candidates.Num() > 1)
	{
		Result.ResultType = EDISItemPlacementResult::RequiresSelection;
		Result.bSuccess = false;
		Result.CandidateStacks = Candidates;
		UE_LOG(LogTemp, Log, TEXT("UDISInventory::SetSlotItemStack - Multiple stackable candidates found (%d). User selection required via popup."), Candidates.Num());
		return Result;
	}

	// 스태킹 병합 대상이 결정된 경우
	if (TargetStackToMerge != nullptr)
	{
		const int32 MergedCount = MergeItemStackInto(InItemStack, TargetStackToMerge);

		Result.TargetStack = TargetStackToMerge;
		Result.MergedCount = MergedCount;
		Result.RemainingCount = InItemStack->GetStackCount();
		Result.CandidateStacks = Candidates;

		if (MergedCount > 0)
		{
			Result.bSuccess = true;
			if (InItemStack->IsEmpty() == true)
			{
				Result.ResultType = EDISItemPlacementResult::FullyMerged;
				// 완전 병합되어 비워진 경우, 이전 소유 인벤토리가 있다면 이전 인벤토리에서 완전 해제
				UDISInventory* PrevInventory = InItemStack->GetOwningInventory();
				if (PrevInventory != nullptr && PrevInventory != this)
				{
					PrevInventory->ClearItemStack(InItemStack);
				}
			}
			else
			{
				Result.ResultType = EDISItemPlacementResult::PartiallyMerged;
				// 부분 병합된 경우: 이전 인벤토리에 잔여 스택이 그대로 유지됨
			}
		}
		else
		{
			Result.bSuccess = false;
			Result.ResultType = EDISItemPlacementResult::Failed;
		}

		return Result;
	}

	// 케이스 3: 겹치는 스태킹 후보가 없는 경우 -> 신규 슬롯 배치 처리
	// 이전 소유 인벤토리가 존재하고 서로 다른 인벤토리인 경우 이전 인벤토리에서 먼저 원자적 언바인딩
	UDISInventory* PrevInventory = InItemStack->GetOwningInventory();
	if (PrevInventory != nullptr && PrevInventory != this)
	{
		PrevInventory->ClearItemStack(InItemStack);
	}
	else
	{
		// 동일한 인벤토리 내부 이동인 경우 기존 슬롯들에서 먼저 언바인딩
		for (const TObjectPtr<UDISInventorySlot>& SlotPtr : Slots)
		{
			if (SlotPtr != nullptr && SlotPtr->GetItemStack() == InItemStack)
			{
				SlotPtr->SetItemStack(nullptr);
			}
		}
	}

	const FIntPoint BaseCoord = InventoryDefinition->SlotIndexToCoord(SlotIndex);
	for (const FIntPoint& Offset : OccupiedPoints)
	{
		const FIntPoint TileCoord = BaseCoord + Offset;
		const int32 TargetSlotIndex = InventoryDefinition->CoordToSlotIndex(TileCoord);
		UDISInventorySlot* Slot = GetSlot(TargetSlotIndex);
		if (Slot != nullptr)
		{
			Slot->SetItemStack(InItemStack);
		}
	}

	// 신규 소유 인벤토리 등록
	InItemStack->SetOwningInventory(this);

	Result.bSuccess = true;
	Result.ResultType = EDISItemPlacementResult::PlacedNew;
	Result.MergedCount = 0;
	Result.RemainingCount = InItemStack->GetStackCount();
	Result.TargetStack = InItemStack;

	return Result;
}

int32 UDISInventory::MergeItemStackInto(UDISItemStack* SourceStack, UDISItemStack* TargetStack)
{
	if (SourceStack == nullptr || SourceStack->IsEmpty() == true || TargetStack == nullptr)
	{
		return 0;
	}

	if (SourceStack == TargetStack)
	{
		return 0;
	}

	int32 MergedCount = 0;
	while (SourceStack->IsEmpty() == false && TargetStack->CanStackWith(SourceStack->GetTopEntry()) == true)
	{
		UDISItemEntry* Entry = SourceStack->PopEntry();
		if (Entry != nullptr)
		{
			const bool bPushed = TargetStack->PushEntry(Entry);
			if (bPushed == true)
			{
				// TargetStack을 참조하고 있는 모든 슬롯의 강화 컨테이너에 신규 엔트리 바인딩 동기화
				for (const TObjectPtr<UDISInventorySlot>& SlotPtr : Slots)
				{
					if (SlotPtr != nullptr && SlotPtr->GetItemStack() == TargetStack)
					{
						SlotPtr->BindItemEntryEnhancement(Entry);
					}
				}
				++MergedCount;
			}
			else
			{
				// Push 실패 시 다시 복구하고 중단
				SourceStack->PushEntry(Entry);
				break;
			}
		}
	}

	// SourceStack이 완전히 비워졌다면, 이전 슬롯에 걸려있던 바인딩 정리
	if (SourceStack->IsEmpty() == true)
	{
		if (SourceStack->GetOwningInventory() != nullptr)
		{
			SourceStack->GetOwningInventory()->ClearItemStack(SourceStack);
		}
		else
		{
			for (const TObjectPtr<UDISInventorySlot>& SlotPtr : Slots)
			{
				if (SlotPtr != nullptr && SlotPtr->GetItemStack() == SourceStack)
				{
					SlotPtr->SetItemStack(nullptr);
				}
			}
		}
	}

	return MergedCount;
}

UDISItemStack* UDISInventory::ClearSlot(int32 SlotIndex)
{
	UDISInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot == nullptr)
	{
		return nullptr;
	}

	UDISItemStack* TargetStack = Slot->GetItemStack();
	if (TargetStack == nullptr)
	{
		return nullptr;
	}

	ClearItemStack(TargetStack);
	return TargetStack;
}

bool UDISInventory::ClearItemStack(UDISItemStack* InItemStack)
{
	if (InItemStack == nullptr)
	{
		return false;
	}

	bool bFound = false;
	for (const TObjectPtr<UDISInventorySlot>& SlotPtr : Slots)
	{
		if (SlotPtr != nullptr && SlotPtr->GetItemStack() == InItemStack)
		{
			SlotPtr->SetItemStack(nullptr);
			bFound = true;
		}
	}

	if (bFound == true)
	{
		if (InItemStack->GetOwningInventory() == this)
		{
			InItemStack->SetOwningInventory(nullptr);
		}
		return true;
	}

	return false;
}

bool UDISInventory::IsSlotBlocked(int32 SlotIndex) const
{
	if (InventoryDefinition != nullptr)
	{
		return InventoryDefinition->IsSlotBlocked(SlotIndex);
	}

	return false;
}

FIntPoint UDISInventory::GetItemStackBaseCoord(const UDISItemStack* InItemStack) const
{
	if (InItemStack == nullptr || InventoryDefinition == nullptr)
	{
		return FIntPoint::NoneValue;
	}

	int32 MinX = MAX_int32;
	int32 MinY = MAX_int32;
	bool bFound = false;

	for (const TObjectPtr<UDISInventorySlot>& SlotPtr : Slots)
	{
		if (SlotPtr != nullptr && SlotPtr->GetItemStack() == InItemStack)
		{
			const FIntPoint Coord = InventoryDefinition->SlotIndexToCoord(SlotPtr->GetSlotIndex());
			if (Coord != FIntPoint::NoneValue)
			{
				MinX = FMath::Min(MinX, Coord.X);
				MinY = FMath::Min(MinY, Coord.Y);
				bFound = true;
			}
		}
	}

	if (bFound == true)
	{
		return FIntPoint(MinX, MinY);
	}

	return FIntPoint::NoneValue;
}

int32 UDISInventory::GetItemStackRootSlot(const UDISItemStack* InItemStack) const
{
	if (InventoryDefinition == nullptr)
	{
		return INDEX_NONE;
	}

	const FIntPoint BaseCoord = GetItemStackBaseCoord(InItemStack);
	if (BaseCoord == FIntPoint::NoneValue)
	{
		return INDEX_NONE;
	}

	return InventoryDefinition->CoordToSlotIndex(BaseCoord);
}

TArray<int32> UDISInventory::GetItemStackOccupiedSlots(const UDISItemStack* InItemStack) const
{
	TArray<int32> OccupiedSlots;
	if (InItemStack == nullptr)
	{
		return OccupiedSlots;
	}

	for (const TObjectPtr<UDISInventorySlot>& SlotPtr : Slots)
	{
		if (SlotPtr != nullptr && SlotPtr->GetItemStack() == InItemStack)
		{
			OccupiedSlots.Add(SlotPtr->GetSlotIndex());
		}
	}

	return OccupiedSlots;
}

bool UDISInventory::CanRotateItemStack(const UDISItemStack* InItemStack, bool bClockwise) const
{
	if (InItemStack == nullptr || InItemStack->IsEmpty() == true || InventoryDefinition == nullptr)
	{
		return false;
	}

	// 1. 아이템 정의에서 회전 허용 여부 검증
	if (InItemStack->CanRotate() == false)
	{
		return false;
	}

	// 2. 현재 인벤토리 내 좌상단 기준 좌표 조회
	const FIntPoint BaseCoord = GetItemStackBaseCoord(InItemStack);
	if (BaseCoord == FIntPoint::NoneValue)
	{
		return false;
	}

	const int32 BaseSlotIndex = InventoryDefinition->CoordToSlotIndex(BaseCoord);
	if (Slots.IsValidIndex(BaseSlotIndex) == false)
	{
		return false;
	}

	const UDISItemEntry* TopEntry = InItemStack->GetTopEntry();
	if (TopEntry == nullptr || TopEntry->GetItemDefinition() == nullptr)
	{
		return false;
	}

	// 3. 목표 회전 상태 및 회전된 형상의 상대 타일 좌표 목록 획득
	const EDISItemRotation CurrentRotation = InItemStack->GetRotation();
	const EDISItemRotation TargetRotation = (bClockwise == true)
		? DISItemRotation::GetNextClockwise(CurrentRotation)
		: DISItemRotation::GetNextCounterClockwise(CurrentRotation);

	const TArray<FIntPoint> NewOccupiedPoints = TopEntry->GetItemDefinition()->Shape.GetRotatedPoints(TargetRotation);
	if (NewOccupiedPoints.IsEmpty() == true)
	{
		return false;
	}

	// 4. 새 회전 좌표들의 경계 초과, 블록 타일, 슬롯 필터, 타 아이템과의 충돌 검증
	for (const FIntPoint& Offset : NewOccupiedPoints)
	{
		const FIntPoint TileCoord = BaseCoord + Offset;

		// 인벤토리 경계 밖 또는 블록된 타일 검사
		if (IsTileBlockedOrOutOfBounds(TileCoord) == true)
		{
			return false;
		}

		const int32 CurrentSlotIndex = InventoryDefinition->CoordToSlotIndex(TileCoord);
		if (Slots.IsValidIndex(CurrentSlotIndex) == false)
		{
			return false;
		}

		const UDISInventorySlot* CurrentSlot = Slots[CurrentSlotIndex].Get();
		if (CurrentSlot == nullptr)
		{
			return false;
		}

		// 슬롯 자체 필터 검증
		for (const TInstancedStruct<FDISInventoryFilter>& Filter : CurrentSlot->GetFilters())
		{
			if (Filter.IsValid() == true)
			{
				const FDISInventoryFilter& FilterRef = Filter.Get<FDISInventoryFilter>();
				if (FilterRef.PassesFilter(InItemStack) == false)
				{
					return false;
				}
			}
		}

		// 점유 충돌 검증: 자기 자신이 아닌 다른 아이템 스택이 이미 차지하고 있다면 회전 불가
		const UDISItemStack* ExistingStack = CurrentSlot->GetItemStack();
		if (ExistingStack != nullptr && ExistingStack->IsEmpty() == false && ExistingStack != InItemStack)
		{
			return false;
		}
	}

	// 5. 슬롯 배치 규칙(Placement Rules) 검증 (모서리, 중앙 등)
	FDISItemPlacementContext Context;
	Context.Inventory = this;
	Context.TargetSlotIndex = BaseSlotIndex;
	Context.TargetCoord = BaseCoord;
	Context.InventoryOwner = nullptr;

	if (TopEntry->CanPlaceAtSlot(Context) == false)
	{
		return false;
	}

	return true;
}

bool UDISInventory::RotateItemStack(UDISItemStack* InItemStack, bool bClockwise)
{
	if (CanRotateItemStack(InItemStack, bClockwise) == false)
	{
		return false;
	}

	const FIntPoint BaseCoord = GetItemStackBaseCoord(InItemStack);
	if (BaseCoord == FIntPoint::NoneValue || InventoryDefinition == nullptr)
	{
		return false;
	}

	const UDISItemEntry* TopEntry = InItemStack->GetTopEntry();
	if (TopEntry == nullptr || TopEntry->GetItemDefinition() == nullptr)
	{
		return false;
	}

	const EDISItemRotation CurrentRotation = InItemStack->GetRotation();
	const EDISItemRotation TargetRotation = (bClockwise == true)
		? DISItemRotation::GetNextClockwise(CurrentRotation)
		: DISItemRotation::GetNextCounterClockwise(CurrentRotation);

	const TArray<FIntPoint> NewOccupiedPoints = TopEntry->GetItemDefinition()->Shape.GetRotatedPoints(TargetRotation);

	// 1. 기존 점유 슬롯들에서 InItemStack 언바인딩
	for (const TObjectPtr<UDISInventorySlot>& SlotPtr : Slots)
	{
		if (SlotPtr != nullptr && SlotPtr->GetItemStack() == InItemStack)
		{
			SlotPtr->SetItemStack(nullptr);
		}
	}

	// 2. 스택의 회전값 갱신
	InItemStack->SetRotation(TargetRotation);

	// 3. 회전된 새 좌표의 슬롯들에 InItemStack 바인딩
	for (const FIntPoint& Offset : NewOccupiedPoints)
	{
		const FIntPoint TileCoord = BaseCoord + Offset;
		const int32 TargetSlotIndex = InventoryDefinition->CoordToSlotIndex(TileCoord);
		UDISInventorySlot* Slot = GetSlot(TargetSlotIndex);
		if (Slot != nullptr)
		{
			Slot->SetItemStack(InItemStack);
		}
	}

	return true;
}

bool UDISInventory::RotateItemStackAtSlot(int32 SlotIndex, bool bClockwise)
{
	UDISItemStack* Stack = GetSlotItemStack(SlotIndex);
	if (Stack == nullptr)
	{
		return false;
	}

	return RotateItemStack(Stack, bClockwise);
}

bool UDISInventory::IsTileBlockedOrOutOfBounds(const FIntPoint& Coord) const
{
	if (InventoryDefinition == nullptr || InventoryDefinition->LineBreakTileCount <= 0)
	{
		return true;
	}

	// 음수 좌표는 인벤토리 경계 밖으로 판정
	if (Coord.X < 0 || Coord.Y < 0)
	{
		return true;
	}

	const int32 TotalSlotCount = Slots.Num();
	if (TotalSlotCount <= 0)
	{
		return true;
	}

	const int32 LineBreak = InventoryDefinition->LineBreakTileCount;
	int32 GridWidth = 0;
	int32 GridHeight = 0;

	if (InventoryDefinition->SlotIndexingDirection == EDISSlotIndexingDirection::Horizontal)
	{
		GridWidth = LineBreak;
		GridHeight = FMath::DivideAndRoundUp(TotalSlotCount, LineBreak);
	}
	else
	{
		GridHeight = LineBreak;
		GridWidth = FMath::DivideAndRoundUp(TotalSlotCount, LineBreak);
	}

	// 2D 그리드 사각 경계 초과 검사
	if (Coord.X >= GridWidth || Coord.Y >= GridHeight)
	{
		return true;
	}

	// 슬롯 1차원 인덱스로 변환 후 실제 보유 중인 슬롯 인덱스 범위 초과 검사
	const int32 SlotIndex = InventoryDefinition->CoordToSlotIndex(Coord);
	if (SlotIndex == INDEX_NONE || SlotIndex >= TotalSlotCount)
	{
		return true;
	}

	// 정적 정의 에셋의 막힌(블록된) 타일인지 검사
	if (InventoryDefinition->IsTileBlocked(Coord) == true)
	{
		return true;
	}

	return false;
}

bool UDISInventory::IsSlotCorner(int32 SlotIndex) const
{
	if (Slots.IsValidIndex(SlotIndex) == false || InventoryDefinition == nullptr)
	{
		return false;
	}

	// 슬롯 자체가 블록된 칸인 경우 모서리로 취급하지 않음
	if (IsSlotBlocked(SlotIndex) == true)
	{
		return false;
	}

	const FIntPoint Coord = InventoryDefinition->SlotIndexToCoord(SlotIndex);
	if (Coord == FIntPoint::NoneValue)
	{
		return false;
	}

	// 상/하/좌/우 4방향 직교 이웃의 차단(경계선 밖 또는 BlockTile) 여부 검사
	const bool bUpBlocked = IsTileBlockedOrOutOfBounds(FIntPoint(Coord.X, Coord.Y - 1));
	const bool bDownBlocked = IsTileBlockedOrOutOfBounds(FIntPoint(Coord.X, Coord.Y + 1));
	const bool bLeftBlocked = IsTileBlockedOrOutOfBounds(FIntPoint(Coord.X - 1, Coord.Y));
	const bool bRightBlocked = IsTileBlockedOrOutOfBounds(FIntPoint(Coord.X + 1, Coord.Y));

	// 직교하는 두 축이 모두 막혀 구석(Corner)을 형성하는지 검사
	const bool bTopLeft = (bUpBlocked == true && bLeftBlocked == true);
	const bool bTopRight = (bUpBlocked == true && bRightBlocked == true);
	const bool bBottomLeft = (bDownBlocked == true && bLeftBlocked == true);
	const bool bBottomRight = (bDownBlocked == true && bRightBlocked == true);

	return (bTopLeft == true || bTopRight == true || bBottomLeft == true || bBottomRight == true);
}

bool UDISInventory::IsSlotEdge(int32 SlotIndex) const
{
	if (Slots.IsValidIndex(SlotIndex) == false || InventoryDefinition == nullptr)
	{
		return false;
	}

	if (IsSlotBlocked(SlotIndex) == true)
	{
		return false;
	}

	const FIntPoint Coord = InventoryDefinition->SlotIndexToCoord(SlotIndex);
	if (Coord == FIntPoint::NoneValue)
	{
		return false;
	}

	const bool bUpBlocked = IsTileBlockedOrOutOfBounds(FIntPoint(Coord.X, Coord.Y - 1));
	const bool bDownBlocked = IsTileBlockedOrOutOfBounds(FIntPoint(Coord.X, Coord.Y + 1));
	const bool bLeftBlocked = IsTileBlockedOrOutOfBounds(FIntPoint(Coord.X - 1, Coord.Y));
	const bool bRightBlocked = IsTileBlockedOrOutOfBounds(FIntPoint(Coord.X + 1, Coord.Y));

	// 상/하/좌/우 중 1면 이상이 차단/경계 밖이면 가장자리임
	return (bUpBlocked == true || bDownBlocked == true || bLeftBlocked == true || bRightBlocked == true);
}

bool UDISInventory::IsSlotCenter(int32 SlotIndex) const
{
	if (Slots.IsValidIndex(SlotIndex) == false || InventoryDefinition == nullptr)
	{
		return false;
	}

	if (IsSlotBlocked(SlotIndex) == true)
	{
		return false;
	}

	// 가장자리 또는 블록 타일과 인접해있다면 중앙이 아님
	if (IsSlotEdge(SlotIndex) == true)
	{
		return false;
	}

	// 외곽/블록 타일과 닿지 않는 내부 슬롯은 중앙 영역으로 인정함
	return true;
}

UDISInventorySlot* UDISInventory::CreateSlotInstance(int32 SlotIndex)
{
	UClass* TargetClass = nullptr;
	if (InventoryDefinition != nullptr && InventoryDefinition->DefaultSlotClass != nullptr)
	{
		TargetClass = InventoryDefinition->DefaultSlotClass.Get();
	}

	// 미지정되었거나 유효하지 않은 타입인 경우 기본 UDISInventorySlot 클래스 사용
	if (TargetClass == nullptr || TargetClass->IsChildOf(UDISInventorySlot::StaticClass()) == false)
	{
		TargetClass = UDISInventorySlot::StaticClass();
	}

	UDISInventorySlot* NewSlot = NewObject<UDISInventorySlot>(this, TargetClass);
	NewSlot->InitializeSlot(SlotIndex);
	return NewSlot;
}
