#include "Item/DISItemPlacementRule.h"
#include "Inventory/DISInventory.h"
#include "Item/DISItemEntry.h"

FDISItemPlacementContext::FDISItemPlacementContext() :
	Inventory(nullptr),
	TargetSlotIndex(INDEX_NONE),
	TargetCoord(FIntPoint::NoneValue),
	InventoryOwner(nullptr)
{
}

bool FDISItemPlacementRule::CanPlaceItem(const UDISItemEntry* ItemEntry, const FDISItemPlacementContext& Context) const
{
	// 기본 구현은 제약 없이 배치를 허용함
	return true;
}

bool FDISCornerPlacementRule::CanPlaceItem(const UDISItemEntry* ItemEntry, const FDISItemPlacementContext& Context) const
{
	if (Context.Inventory.IsValid() == false)
	{
		return false;
	}

	// 인벤토리의 모서리 판정 API를 통해 검증
	return Context.Inventory->IsSlotCorner(Context.TargetSlotIndex);
}

bool FDISCenterPlacementRule::CanPlaceItem(const UDISItemEntry* ItemEntry, const FDISItemPlacementContext& Context) const
{
	if (Context.Inventory.IsValid() == false)
	{
		return false;
	}

	// 인벤토리의 중앙 판정 API를 통해 검증
	return Context.Inventory->IsSlotCenter(Context.TargetSlotIndex);
}
