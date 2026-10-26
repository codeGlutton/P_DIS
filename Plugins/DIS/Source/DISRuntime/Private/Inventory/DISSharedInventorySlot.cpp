#include "Inventory/DISSharedInventorySlot.h"
#include "Item/DISItemStack.h"
#include "Inventory/DISInventory.h"

UDISSharedInventorySlot::UDISSharedInventorySlot() :
	SlotIndex(INDEX_NONE),
	ItemStack(nullptr),
	Filters(),
	RegisteredOwningInventory(nullptr)
{
}

void UDISSharedInventorySlot::InitializeSlot(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
}

int32 UDISSharedInventorySlot::GetSlotIndex() const
{
	return SlotIndex;
}

bool UDISSharedInventorySlot::IsEmpty() const
{
	return ItemStack == nullptr || ItemStack->IsEmpty() == true;
}

int32 UDISSharedInventorySlot::GetStackCount() const
{
	return (ItemStack != nullptr) ? ItemStack->GetStackCount() : 0;
}

UDISItemStack* UDISSharedInventorySlot::GetItemStack() const
{
	return ItemStack.Get();
}

bool UDISSharedInventorySlot::SetItemStack(UDISItemStack* InItemStack)
{
	if (InItemStack == nullptr)
	{
		ClearSlot();
		return true;
	}

	if (CanAcceptItemStack(InItemStack) == false && ItemStack != InItemStack)
	{
		return false;
	}

	if (ItemStack.Get() == InItemStack)
	{
		return true;
	}

	if (ItemStack != nullptr)
	{
		ItemStack->OnOwningInventoryChanged.RemoveDynamic(this, &UDISSharedInventorySlot::HandleObservedStackOwnershipChanged);
	}

	ItemStack = InItemStack;
	RegisteredOwningInventory = (InItemStack != nullptr) ? InItemStack->GetOwningInventory() : nullptr;

	if (ItemStack != nullptr)
	{
		ItemStack->OnOwningInventoryChanged.AddDynamic(this, &UDISSharedInventorySlot::HandleObservedStackOwnershipChanged);
	}

	return true;
}

UDISItemStack* UDISSharedInventorySlot::ClearSlot()
{
	UDISItemStack* RemovedStack = ItemStack.Get();
	if (RemovedStack != nullptr)
	{
		RemovedStack->OnOwningInventoryChanged.RemoveDynamic(this, &UDISSharedInventorySlot::HandleObservedStackOwnershipChanged);
	}

	RegisteredOwningInventory = nullptr;
	ItemStack = nullptr;
	return RemovedStack;
}

void UDISSharedInventorySlot::AddFilter(const TInstancedStruct<FDISInventoryFilter>& InFilter)
{
	if (InFilter.IsValid() == true)
	{
		Filters.Add(InFilter);
	}
}

void UDISSharedInventorySlot::ClearFilters()
{
	Filters.Empty();
}

const TArray<TInstancedStruct<FDISInventoryFilter>>& UDISSharedInventorySlot::GetFilters() const
{
	return Filters;
}

TArray<TInstancedStruct<FDISInventoryFilter>>& UDISSharedInventorySlot::GetMutableFilters()
{
	return Filters;
}

bool UDISSharedInventorySlot::CanAcceptItemStack(const UDISItemStack* InItemStack) const
{
	if (InItemStack == nullptr || InItemStack->IsEmpty() == true)
	{
		return false;
	}

	// 슬롯에 등록된 모든 필터 검사
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

void UDISSharedInventorySlot::HandleObservedStackOwnershipChanged(UDISItemStack* ChangedStack, UDISInventory* NewOwningInventory)
{
	if (ChangedStack == nullptr || ChangedStack != ItemStack.Get())
	{
		return;
	}

	// 미소유 상태에서 처음으로 특정 인벤토리에 안착한 경우 등록 인벤토리로 바인딩 갱신
	if (RegisteredOwningInventory.IsValid() == false && NewOwningInventory != nullptr)
	{
		RegisteredOwningInventory = NewOwningInventory;
		return;
	}

	// 소유 인벤토리가 해제되었거나(nullptr), 등록된 원래 인벤토리와 다른 인벤토리로 이전된 경우 슬롯 자동 해제
	if (NewOwningInventory == nullptr || NewOwningInventory != RegisteredOwningInventory.Get())
	{
		ClearSlot();
	}
}

