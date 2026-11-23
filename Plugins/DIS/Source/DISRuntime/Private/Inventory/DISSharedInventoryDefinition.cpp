#include "Inventory/DISSharedInventoryDefinition.h"
#include "Inventory/DISSharedInventorySlot.h"

UDISSharedInventoryDefinition::UDISSharedInventoryDefinition() :
	DefaultCapacity(4),
	DefaultSlotClass(nullptr),
	DefaultFilters()
{
}

FPrimaryAssetId UDISSharedInventoryDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(DISPrimaryAssetTypes::SharedInventory, GetFName());
}
