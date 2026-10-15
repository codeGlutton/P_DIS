#include "Inventory/DISInventorySlot.h"
#include "Item/DISItemEntry.h"
#include "Enhancement/DISEnhancementContainer.h"

UDISInventorySlot::UDISInventorySlot() :
	SlotIndex(INDEX_NONE),
	ItemStack(nullptr),
	EnhancementContainer(nullptr),
	Filters()
{
}

void UDISInventorySlot::InitializeSlot(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	// 슬롯 자체 강화를 전담할 컨테이너 생성 및 초기화
	if (EnhancementContainer == nullptr)
	{
		EnhancementContainer = NewObject<UDISEnhancementContainer>(this);

		TScriptInterface<IDISEnhanceableInterface> OwnerInterface;
		OwnerInterface.SetObject(this);
		OwnerInterface.SetInterface(Cast<IDISEnhanceableInterface>(this));

		EnhancementContainer->Initialize(OwnerInterface, nullptr);

		// 이미 장착된 ItemStack이 존재하는 경우 내부 엔트리들을 슬롯 강화 대상으로 동기화
		BindItemStackEnhancement(ItemStack);
	}
}

UDISEnhancementContainer* UDISInventorySlot::GetEnhancementContainer() const
{
	return EnhancementContainer.Get();
}

int32 UDISInventorySlot::GetSlotIndex() const
{
	return SlotIndex;
}

bool UDISInventorySlot::IsEmpty() const
{
	return ItemStack == nullptr || ItemStack->IsEmpty() == true;
}

int32 UDISInventorySlot::GetStackCount() const
{
	return ItemStack != nullptr ? ItemStack->GetStackCount() : 0;
}

UDISItemEntry* UDISInventorySlot::GetItemEntry() const
{
	return ItemStack != nullptr ? ItemStack->GetTopEntry() : nullptr;
}

UDISItemStack* UDISInventorySlot::GetItemStack() const
{
	return ItemStack.Get();
}

void UDISInventorySlot::SetItemStack(UDISItemStack* InItemStack)
{
	if (ItemStack == InItemStack)
	{
		return;
	}

	// 이전 ItemStack 내부 아이템들을 슬롯 강화 컨테이너 대상에서 해제
	UnbindItemStackEnhancement(ItemStack);

	ItemStack = InItemStack;

	// 신규 ItemStack 내부 아이템들을 슬롯 강화 컨테이너 대상으로 등록
	BindItemStackEnhancement(ItemStack);
}

bool UDISInventorySlot::SetItemEntry(UDISItemEntry* InItemEntry)
{
	if (InItemEntry != nullptr)
	{
		if (CanAcceptItem(InItemEntry) == false)
		{
			return false;
		}

		// 기존 스택 아이템들을 슬롯 강화 컨테이너 대상에서 해제
		UnbindItemStackEnhancement(ItemStack);

		if (ItemStack == nullptr)
		{
			ItemStack = NewObject<UDISItemStack>(this);
		}
		else
		{
			ItemStack->Clear();
		}

		const bool bPushed = ItemStack->PushEntry(InItemEntry);
		if (bPushed == true)
		{
			BindItemEntryEnhancement(InItemEntry);
		}
		return bPushed;
	}

	ClearSlot();
	return true;
}

bool UDISInventorySlot::PushItemEntry(UDISItemEntry* InItemEntry)
{
	if (InItemEntry == nullptr)
	{
		return false;
	}

	if (CanAcceptItem(InItemEntry) == false)
	{
		return false;
	}

	if (ItemStack == nullptr)
	{
		ItemStack = NewObject<UDISItemStack>(this);
	}

	const bool bPushed = ItemStack->PushEntry(InItemEntry);
	if (bPushed == true)
	{
		BindItemEntryEnhancement(InItemEntry);
	}
	return bPushed;
}

UDISItemEntry* UDISInventorySlot::PopItemEntry()
{
	if (ItemStack == nullptr)
	{
		return nullptr;
	}

	UDISItemEntry* Top = ItemStack->PopEntry();
	if (Top != nullptr)
	{
		UnbindItemEntryEnhancement(Top);
	}

	if (ItemStack->IsEmpty() == true)
	{
		ItemStack = nullptr;
	}
	return Top;
}

UDISItemEntry* UDISInventorySlot::ClearSlot()
{
	if (ItemStack == nullptr)
	{
		return nullptr;
	}

	// 모든 내부 아이템들을 슬롯 강화 컨테이너 대상에서 해제
	UnbindItemStackEnhancement(ItemStack);

	UDISItemEntry* Top = ItemStack->GetTopEntry();
	ItemStack->Clear();
	ItemStack = nullptr;
	return Top;
}

void UDISInventorySlot::AddFilter(const TInstancedStruct<FDISInventoryFilter>& InFilter)
{
	if (InFilter.IsValid() == true)
	{
		Filters.Add(InFilter);
	}
}

void UDISInventorySlot::ClearFilters()
{
	Filters.Empty();
}

const TArray<TInstancedStruct<FDISInventoryFilter>>& UDISInventorySlot::GetFilters() const
{
	return Filters;
}

TArray<TInstancedStruct<FDISInventoryFilter>>& UDISInventorySlot::GetMutableFilters()
{
	return Filters;
}

bool UDISInventorySlot::CanAcceptItem(const UDISItemEntry* InItemEntry) const
{
	if (InItemEntry == nullptr)
	{
		return false;
	}

	// 1. 슬롯에 등록된 모든 필터 검사
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

	// 2. 내부 아이템 스택에 합쳐질 수 있는지 여부 검증 (스택이 없거나 비어있으면 true)
	if (ItemStack == nullptr || ItemStack->IsEmpty() == true)
	{
		return true;
	}

	return ItemStack->CanStackWith(InItemEntry);
}

void UDISInventorySlot::BindItemEntryEnhancement(UDISItemEntry* InItemEntry)
{
	if (InItemEntry != nullptr && EnhancementContainer != nullptr)
	{
		EnhancementContainer->AddEffectTarget(InItemEntry);
	}
}

void UDISInventorySlot::UnbindItemEntryEnhancement(UDISItemEntry* InItemEntry)
{
	if (InItemEntry != nullptr && EnhancementContainer != nullptr)
	{
		EnhancementContainer->RemoveEffectTarget(InItemEntry);
	}
}

void UDISInventorySlot::BindItemStackEnhancement(UDISItemStack* InItemStack)
{
	if (InItemStack != nullptr && EnhancementContainer != nullptr)
	{
		for (const TObjectPtr<UDISItemEntry>& Entry : InItemStack->GetEntries())
		{
			if (Entry != nullptr)
			{
				EnhancementContainer->AddEffectTarget(Entry.Get());
			}
		}
	}
}

void UDISInventorySlot::UnbindItemStackEnhancement(UDISItemStack* InItemStack)
{
	if (InItemStack != nullptr && EnhancementContainer != nullptr)
	{
		for (const TObjectPtr<UDISItemEntry>& Entry : InItemStack->GetEntries())
		{
			if (Entry != nullptr)
			{
				EnhancementContainer->RemoveEffectTarget(Entry.Get());
			}
		}
	}
}
