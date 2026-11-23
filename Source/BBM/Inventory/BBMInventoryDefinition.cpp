#include "Inventory/BBMInventoryDefinition.h"
#include "Inventory/BBMInventorySlot.h"

UBBMInventoryDefinition::UBBMInventoryDefinition() :
	ExtractionOrder(EBBMItemExtractionOrder::TopToBottom_LeftToRight)
{
	// BBM 배낭 기본 규격 설정: 무게 시스템은 비활성화
	bUseWeightSystem = false;
	DefaultMaxWeight = 0.0f;

	// 기본 슬롯 클래스를 UBBMInventorySlot으로 지정
	DefaultSlotClass = UBBMInventorySlot::StaticClass();
}

const FBBMSlotEffectSpecList* UBBMInventoryDefinition::FindSlotEnhancements(const FIntPoint& Coord) const
{
	return SlotEnhancements.Find(Coord);
}

bool UBBMInventoryDefinition::HasSlotEnhancement(int32 SlotIndex) const
{
	const FIntPoint Coord = SlotIndexToCoord(SlotIndex);
	if (Coord == FIntPoint::NoneValue)
	{
		return false;
	}

	const FBBMSlotEffectSpecList* SpecList = FindSlotEnhancements(Coord);
	return SpecList != nullptr && SpecList->Effects.Num() > 0;
}
