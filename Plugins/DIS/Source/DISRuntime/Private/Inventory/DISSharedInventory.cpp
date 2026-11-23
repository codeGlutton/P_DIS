#include "Inventory/DISSharedInventory.h"
#include "Inventory/DISSharedInventoryDefinition.h"
#include "Inventory/DISSharedInventorySlot.h"
#include "Item/DISItemStack.h"

UDISSharedInventory::UDISSharedInventory() :
	InventoryDefinition(nullptr),
	Filters(),
	Slots()
{
}

void UDISSharedInventory::InitializeInventory(const UDISSharedInventoryDefinition* InDefinition)
{
	InventoryDefinition = InDefinition;

	if (InventoryDefinition != nullptr)
	{
		Filters = InventoryDefinition->DefaultFilters;
	}
	else
	{
		Filters.Empty();
	}

	int32 TargetCapacity = 0;
	if (InventoryDefinition != nullptr)
	{
		TargetCapacity = InventoryDefinition->DefaultCapacity;
	}

	TargetCapacity = FMath::Max(0, TargetCapacity);

	Slots.Empty(TargetCapacity);
	for (int32 Index = 0; Index < TargetCapacity; ++Index)
	{
		Slots.Add(CreateSlotInstance(Index));
	}
}

const UDISSharedInventoryDefinition* UDISSharedInventory::GetInventoryDefinition() const
{
	return InventoryDefinition.Get();
}

void UDISSharedInventory::AddFilter(const TInstancedStruct<FDISInventoryFilter>& InFilter)
{
	if (InFilter.IsValid() == true)
	{
		Filters.Add(InFilter);
	}
}

void UDISSharedInventory::ClearFilters()
{
	Filters.Empty();
}

const TArray<TInstancedStruct<FDISInventoryFilter>>& UDISSharedInventory::GetFilters() const
{
	return Filters;
}

TArray<TInstancedStruct<FDISInventoryFilter>>& UDISSharedInventory::GetMutableFilters()
{
	return Filters;
}

bool UDISSharedInventory::CanAcceptItemStack(const UDISItemStack* InItemStack) const
{
	if (InItemStack == nullptr || InItemStack->IsEmpty() == true)
	{
		return false;
	}

	// 인벤토리 레벨 전역 필터 검사
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

bool UDISSharedInventory::CanAcceptItemStackAtSlot(const UDISItemStack* InItemStack, int32 TargetSlotIndex) const
{
	if (InItemStack == nullptr || InItemStack->IsEmpty() == true)
	{
		return false;
	}

	if (CanAcceptItemStack(InItemStack) == false)
	{
		return false;
	}

	if (Slots.IsValidIndex(TargetSlotIndex) == false)
	{
		return false;
	}

	const UDISSharedInventorySlot* Slot = Slots[TargetSlotIndex].Get();
	if (Slot == nullptr)
	{
		return false;
	}

	return Slot->CanAcceptItemStack(InItemStack);
}

int32 UDISSharedInventory::GetSlotCount() const
{
	return Slots.Num();
}

UDISSharedInventorySlot* UDISSharedInventory::GetSlot(int32 SlotIndex)
{
	if (Slots.IsValidIndex(SlotIndex) == true)
	{
		return Slots[SlotIndex].Get();
	}

	return nullptr;
}

const UDISSharedInventorySlot* UDISSharedInventory::GetSlot(int32 SlotIndex) const
{
	if (Slots.IsValidIndex(SlotIndex) == true)
	{
		return Slots[SlotIndex].Get();
	}

	return nullptr;
}

const TArray<TObjectPtr<UDISSharedInventorySlot>>& UDISSharedInventory::GetSlots() const
{
	return Slots;
}

TArray<TObjectPtr<UDISSharedInventorySlot>>& UDISSharedInventory::GetMutableSlots()
{
	return Slots;
}

UDISItemStack* UDISSharedInventory::GetSlotItemStack(int32 SlotIndex) const
{
	const UDISSharedInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->GetItemStack();
	}

	return nullptr;
}

int32 UDISSharedInventory::GetSlotStackCount(int32 SlotIndex) const
{
	const UDISSharedInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->GetStackCount();
	}

	return 0;
}

bool UDISSharedInventory::IsSlotEmpty(int32 SlotIndex) const
{
	const UDISSharedInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->IsEmpty();
	}

	return true;
}

bool UDISSharedInventory::SetSlotItemStack(int32 SlotIndex, UDISItemStack* InItemStack)
{
	if (Slots.IsValidIndex(SlotIndex) == false)
	{
		return false;
	}

	// InItemStack이 nullptr이면 해당 슬롯 비우기
	if (InItemStack == nullptr)
	{
		ClearSlot(SlotIndex);
		return true;
	}

	// 필터(전역 및 슬롯) 검증
	if (CanAcceptItemStackAtSlot(InItemStack, SlotIndex) == false)
	{
		return false;
	}

	// 중복 등록 원천 차단: 이미 이 인벤토리의 다른 슬롯에 등록되어 있다면 기존 위치 자동 해제
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Index != SlotIndex && Slots[Index] != nullptr && Slots[Index]->GetItemStack() == InItemStack)
		{
			Slots[Index]->ClearSlot();
		}
	}

	UDISSharedInventorySlot* Slot = Slots[SlotIndex].Get();
	if (Slot != nullptr)
	{
		return Slot->SetItemStack(InItemStack);
	}

	return false;
}

UDISItemStack* UDISSharedInventory::ClearSlot(int32 SlotIndex)
{
	UDISSharedInventorySlot* Slot = GetSlot(SlotIndex);
	if (Slot != nullptr)
	{
		return Slot->ClearSlot();
	}

	return nullptr;
}

bool UDISSharedInventory::SwapSlots(int32 SlotIndexA, int32 SlotIndexB)
{
	if (SlotIndexA == SlotIndexB)
	{
		return true;
	}

	if (Slots.IsValidIndex(SlotIndexA) == false || Slots.IsValidIndex(SlotIndexB) == false)
	{
		return false;
	}

	UDISSharedInventorySlot* SlotA = Slots[SlotIndexA].Get();
	UDISSharedInventorySlot* SlotB = Slots[SlotIndexB].Get();
	if (SlotA == nullptr || SlotB == nullptr)
	{
		return false;
	}

	UDISItemStack* StackA = SlotA->GetItemStack();
	UDISItemStack* StackB = SlotB->GetItemStack();

	// StackA가 슬롯 B의 필터를 만족하는지 검증
	if (StackA != nullptr && SlotB->CanAcceptItemStack(StackA) == false)
	{
		return false;
	}

	// StackB가 슬롯 A의 필터를 만족하는지 검증
	if (StackB != nullptr && SlotA->CanAcceptItemStack(StackB) == false)
	{
		return false;
	}

	// 원자적 포인터 교환
	SlotA->ItemStack = StackB;
	SlotB->ItemStack = StackA;

	return true;
}

int32 UDISSharedInventory::FindSlotWithItemStack(const UDISItemStack* InItemStack) const
{
	if (InItemStack == nullptr)
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index] != nullptr && Slots[Index]->GetItemStack() == InItemStack)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

UDISSharedInventorySlot* UDISSharedInventory::CreateSlotInstance(int32 SlotIndex)
{
	UClass* TargetClass = nullptr;
	if (InventoryDefinition != nullptr && InventoryDefinition->DefaultSlotClass != nullptr)
	{
		TargetClass = InventoryDefinition->DefaultSlotClass.Get();
	}

	if (TargetClass == nullptr || TargetClass->IsChildOf(UDISSharedInventorySlot::StaticClass()) == false)
	{
		TargetClass = UDISSharedInventorySlot::StaticClass();
	}

	UDISSharedInventorySlot* NewSlot = NewObject<UDISSharedInventorySlot>(this, TargetClass);
	NewSlot->InitializeSlot(SlotIndex);
	return NewSlot;
}
