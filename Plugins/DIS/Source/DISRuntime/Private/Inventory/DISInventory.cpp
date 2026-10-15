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
	for (const TObjectPtr<UDISInventorySlot>& Slot : Slots)
	{
		if (Slot != nullptr && Slot->IsEmpty() == false)
		{
			if (const UDISItemStack* Stack = Slot->GetItemStack())
			{
				for (const TObjectPtr<UDISItemEntry>& Entry : Stack->GetEntries())
				{
					if (Entry != nullptr && Entry->GetItemDefinition() != nullptr)
					{
						TotalWeight += Entry->GetItemDefinition()->Weight;
					}
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

bool UDISInventory::CanAcceptItem(const UDISItemEntry* InItemEntry) const
{
	if (InItemEntry == nullptr)
	{
		return false;
	}

	// 무게 시스템 검증: 추가 시 최대 적재 무게 초과 여부 검사
	if (InventoryDefinition != nullptr && InventoryDefinition->bUseWeightSystem == true)
	{
		const UDISItemDefinition* ItemDef = InItemEntry->GetItemDefinition();
		const float ItemWeight = (ItemDef != nullptr) ? ItemDef->Weight : 0.0f;
		if (GetCurrentWeight() + ItemWeight > GetMaxWeight())
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
			if (FilterRef.PassesFilter(InItemEntry) == false)
			{
				return false;
			}
		}
	}

	return true;
}

bool UDISInventory::CanAcceptItemAtSlot(const UDISItemEntry* InItemEntry, int32 TargetSlotIndex) const
{
	// 먼저 인벤토리 전역 규칙(무게 및 전역 필터) 통과 여부 검증
	if (CanAcceptItem(InItemEntry) == false)
	{
		return false;
	}

	// 대상 슬롯이 블록된(막힌) 칸인지 검증
	if (IsSlotBlocked(TargetSlotIndex) == true)
	{
		return false;
	}

	// 대상 슬롯 유효성 및 슬롯 자체 필터 검증
	const UDISInventorySlot* TargetSlot = GetSlot(TargetSlotIndex);
	if (TargetSlot == nullptr)
	{
		return false;
	}

	if (TargetSlot->CanAcceptItem(InItemEntry) == false)
	{
		return false;
	}

	// 아이템 자체의 슬롯 배치 제한 규칙(Placement Rules) 검증
	FDISItemPlacementContext Context;
	Context.Inventory = this;
	Context.TargetSlotIndex = TargetSlotIndex;
	if (InventoryDefinition != nullptr)
	{
		Context.TargetCoord = InventoryDefinition->SlotIndexToCoord(TargetSlotIndex);
	}
	Context.InventoryOwner = nullptr;

	if (InItemEntry->CanPlaceAtSlot(Context) == false)
	{
		return false;
	}

	return true;
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
	for (int32 Index = 0; Index < CheckCount; ++Index)
	{
		const UDISInventorySlot* Slot = Slots[Index].Get();
		if (Slot != nullptr && Slot->IsEmpty() == false)
		{
			if (CanAcceptItemAtSlot(Slot->GetItemEntry(), Index) == false)
			{
				bAllItemsValid = false;
				break;
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

bool UDISInventory::ForceResizeSlots(int32 NewSlotCount, TArray<UDISItemEntry*>& OutEvictedItems)
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

	// 1. 슬롯 축소로 인해 잘려나가는 슬롯들의 모든 아이템을 퇴출 목록으로 수집하고 슬롯 비우기
	if (NewSlotCount < CurrentCapacity)
	{
		for (int32 Index = NewSlotCount; Index < CurrentCapacity; ++Index)
		{
			UDISInventorySlot* Slot = Slots[Index].Get();
			if (Slot != nullptr && Slot->IsEmpty() == false)
			{
				if (UDISItemStack* Stack = Slot->GetItemStack())
				{
					for (const TObjectPtr<UDISItemEntry>& Entry : Stack->GetEntries())
					{
						if (Entry != nullptr)
						{
							OutEvictedItems.Add(Entry.Get());
						}
					}
				}
				Slot->ClearSlot();
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

	// 2. 새로운 그리드 규격에서 각 슬롯의 PlacementRule 위반 여부 검사
	// (슬롯 추가나 삭제로 인해 그리드 외곽선/중앙 판정이 바뀌어 더 이상 해당 슬롯에 머무를 수 없는 아이템 퇴출)
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		UDISInventorySlot* Slot = Slots[Index].Get();
		if (Slot != nullptr && Slot->IsEmpty() == false)
		{
			const UDISItemEntry* TopEntry = Slot->GetItemEntry();
			if (CanAcceptItemAtSlot(TopEntry, Index) == false)
			{
				if (UDISItemStack* Stack = Slot->GetItemStack())
				{
					for (const TObjectPtr<UDISItemEntry>& Entry : Stack->GetEntries())
					{
						if (Entry != nullptr)
						{
							OutEvictedItems.Add(Entry.Get());
						}
					}
				}
				Slot->ClearSlot();
			}
		}
	}

	return true;
}

bool UDISInventory::ForceAddSlots(int32 AdditionalSlots, TArray<UDISItemEntry*>& OutEvictedItems)
{
	if (AdditionalSlots <= 0)
	{
		return false;
	}

	return ForceResizeSlots(Slots.Num() + AdditionalSlots, OutEvictedItems);
}

bool UDISInventory::ForceRemoveSlots(int32 CountToRemove, TArray<UDISItemEntry*>& OutEvictedItems)
{
	if (CountToRemove <= 0)
	{
		return false;
	}

	return ForceResizeSlots(Slots.Num() - CountToRemove, OutEvictedItems);
}

bool UDISInventory::ForceResetSlotsToDefault(TArray<UDISItemEntry*>& OutEvictedItems)
{
	if (InventoryDefinition == nullptr)
	{
		return false;
	}

	return ForceResizeSlots(InventoryDefinition->DefaultCapacity, OutEvictedItems);
}

UDISItemEntry* UDISInventory::GetSlotItemEntry(int32 SlotIndex) const
{
	const UDISInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->GetItemEntry();
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

bool UDISInventory::SetSlotItemEntry(int32 SlotIndex, UDISItemEntry* InItemEntry)
{
	UDISInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->SetItemEntry(InItemEntry);
	}

	return false;
}

UDISItemEntry* UDISInventory::ClearSlot(int32 SlotIndex)
{
	UDISInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->ClearSlot();
	}

	return nullptr;
}

bool UDISInventory::IsSlotBlocked(int32 SlotIndex) const
{
	if (InventoryDefinition != nullptr)
	{
		return InventoryDefinition->IsSlotBlocked(SlotIndex);
	}

	return false;
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
