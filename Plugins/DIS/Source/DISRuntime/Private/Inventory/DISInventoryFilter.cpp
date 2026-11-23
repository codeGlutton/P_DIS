#include "Inventory/DISInventoryFilter.h"
#include "Item/DISItemStack.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemDefinition.h"

FDISInventoryFilter::FDISInventoryFilter()
{
}

bool FDISInventoryFilter::PassesFilter(const UDISItemStack* ItemStack) const
{
	// 기본 필터는 유효하며 비어있지 않은 아이템 스택인 경우 통과시킴
	if (ItemStack == nullptr || ItemStack->IsEmpty() == true)
	{
		return false;
	}

	return true;
}

FDISTagInventoryFilter::FDISTagInventoryFilter() :
	RequiredTags(),
	DeniedTags()
{
}

bool FDISTagInventoryFilter::PassesFilter(const UDISItemStack* ItemStack) const
{
	if (ItemStack == nullptr || ItemStack->IsEmpty() == true)
	{
		return false;
	}

	const UDISItemEntry* TopEntry = ItemStack->GetTopEntry();
	if (TopEntry == nullptr)
	{
		return false;
	}

	const UDISItemDefinition* ItemDef = TopEntry->GetItemDefinition();
	if (ItemDef == nullptr)
	{
		return false;
	}

	// 아이템 정의에서 부여된 태그 수집
	FGameplayTagContainer ItemTags;
	if (ItemDef->ItemTypeTag.IsValid() == true)
	{
		ItemTags.AddTag(ItemDef->ItemTypeTag);
	}

	if (ItemDef->RarityTag.IsValid() == true)
	{
		ItemTags.AddTag(ItemDef->RarityTag);
	}

	// 필수 태그 검증: 지정된 필수 태그가 하나라도 누락되면 거부
	if (RequiredTags.IsEmpty() == false)
	{
		if (ItemTags.HasAll(RequiredTags) == false)
		{
			return false;
		}
	}

	// 거부 태그 검증: 거부 태그 중 하나라도 포함되어 있으면 거부
	if (DeniedTags.IsEmpty() == false)
	{
		if (ItemTags.HasAny(DeniedTags) == true)
		{
			return false;
		}
	}

	return true;
}
