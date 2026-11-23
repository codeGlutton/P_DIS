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

UDISItemStack* UDISInventorySlot::GetItemStack() const
{
	return ItemStack.Get();
}

bool UDISInventorySlot::SetItemStack(UDISItemStack* InItemStack)
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

	if (ItemStack == InItemStack)
	{
		return true;
	}

	// 이전 ItemStack 내부 아이템들을 슬롯 강화 컨테이너 대상에서 해제
	UnbindItemStackEnhancement(ItemStack);

	ItemStack = InItemStack;

	// 신규 ItemStack 내부 아이템들을 슬롯 강화 컨테이너 대상으로 등록
	BindItemStackEnhancement(ItemStack);

	return true;
}

bool UDISInventorySlot::PushItemStack(UDISItemStack* InItemStack)
{
	if (InItemStack == nullptr || InItemStack->IsEmpty() == true)
	{
		return false;
	}

	if (CanAcceptItemStack(InItemStack) == false)
	{
		return false;
	}

	// 슬롯이 비어있는 경우 스택 직접 설정
	if (ItemStack == nullptr || ItemStack->IsEmpty() == true)
	{
		return SetItemStack(InItemStack);
	}

	// 기존 스택이 있는 경우: 합칠 수 있는지 검사
	if (ItemStack->CanStackWith(InItemStack->GetTopEntry()) == false)
	{
		return false;
	}

	// InItemStack의 엔트리들을 꺼내어 현재 슬롯 스택으로 이전
	bool bAnyPushed = false;
	while (InItemStack->IsEmpty() == false && ItemStack->CanStackWith(InItemStack->GetTopEntry()) == true)
	{
		UDISItemEntry* Entry = InItemStack->PopEntry();
		if (Entry != nullptr)
		{
			const bool bPushed = ItemStack->PushEntry(Entry);
			if (bPushed == true)
			{
				BindItemEntryEnhancement(Entry);
				bAnyPushed = true;
			}
			else
			{
				InItemStack->PushEntry(Entry);
				break;
			}
		}
	}

	return bAnyPushed;
}

UDISItemStack* UDISInventorySlot::PopItemStack(int32 SplitCount)
{
	if (ItemStack == nullptr || ItemStack->IsEmpty() == true || SplitCount <= 0)
	{
		return nullptr;
	}

	const int32 ActualCount = FMath::Min(SplitCount, ItemStack->GetStackCount());

	// 지정된 수량만큼 새 스택을 생성하여 분할 추출
	UDISItemStack* SplitStack = NewObject<UDISItemStack>(GetTransientPackage());
	SplitStack->SetRotation(ItemStack->GetRotation());

	for (int32 i = 0; i < ActualCount; ++i)
	{
		UDISItemEntry* Entry = ItemStack->PopEntry();
		if (Entry != nullptr)
		{
			UnbindItemEntryEnhancement(Entry);
			SplitStack->PushEntry(Entry);
		}
	}

	if (ItemStack->IsEmpty() == true)
	{
		ItemStack = nullptr;
	}

	return SplitStack;
}

UDISItemStack* UDISInventorySlot::ClearSlot()
{
	if (ItemStack == nullptr)
	{
		return nullptr;
	}

	// 모든 내부 아이템들을 슬롯 강화 컨테이너 대상에서 해제
	UnbindItemStackEnhancement(ItemStack);

	UDISItemStack* RemovedStack = ItemStack.Get();
	ItemStack = nullptr;
	return RemovedStack;
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

bool UDISInventorySlot::CanAcceptItemStack(const UDISItemStack* InItemStack) const
{
	if (InItemStack == nullptr || InItemStack->IsEmpty() == true)
	{
		return false;
	}

	// 1. 슬롯에 등록된 모든 필터 검사
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

	// 2. 내부 아이템 스택에 합쳐질 수 있는지 여부 검증 (스택이 없거나 비어있으면 true)
	if (ItemStack == nullptr || ItemStack->IsEmpty() == true)
	{
		return true;
	}

	return ItemStack->CanStackWith(InItemStack->GetTopEntry());
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
	if (InItemStack != nullptr)
	{
		for (const TObjectPtr<UDISItemEntry>& Entry : InItemStack->GetEntries())
		{
			if (Entry != nullptr)
			{
				BindItemEntryEnhancement(Entry.Get());
			}
		}
	}
}

void UDISInventorySlot::UnbindItemStackEnhancement(UDISItemStack* InItemStack)
{
	if (InItemStack != nullptr)
	{
		for (const TObjectPtr<UDISItemEntry>& Entry : InItemStack->GetEntries())
		{
			if (Entry != nullptr)
			{
				UnbindItemEntryEnhancement(Entry.Get());
			}
		}
	}
}
