#include "Inventory/BBMSlotEnhancement.h"
#include "Inventory/BBMInventorySlot.h"
#include "Item/BBMBeatEnhancementEffect.h"
#include "Item/DISItemEntry.h"
#include "Enhancement/DISEnhancementContainer.h"

UBBMSlotEnhancement::UBBMSlotEnhancement() :
	GrantedBeatEffectClass(nullptr)
{
}

void UBBMSlotEnhancement::OnEnhancementActivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const
{
	Super::OnEnhancementActivated_Implementation(Target, ActiveEnhancement);

	UDISItemEntry* ItemEntry = Cast<UDISItemEntry>(Target);
	if (ItemEntry == nullptr || GrantedBeatEffectClass == nullptr || ActiveEnhancement.GetLevel() <= 0)
	{
		return;
	}

	UBBMInventorySlot* Slot = nullptr;
	if (const UDISEnhancementContainer* Container = ActiveEnhancement.GetContainer())
	{
		Slot = Cast<UBBMInventorySlot>(Container->GetOwnerInterface().GetObject());
	}

	if (Slot == nullptr)
	{
		return;
	}

	if (UDISEnhancementContainer* ItemContainer = ItemEntry->GetEnhancementContainer())
	{
		ItemContainer->ApplyEnhancementEffectSpec(
			FDISEnhancementEffectSpec(GrantedBeatEffectClass, static_cast<float>(ActiveEnhancement.GetLevel()), Slot)
		);
	}
}

void UBBMSlotEnhancement::OnEnhancementDeactivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const
{
	Super::OnEnhancementDeactivated_Implementation(Target, ActiveEnhancement);

	UDISItemEntry* ItemEntry = Cast<UDISItemEntry>(Target);
	if (ItemEntry == nullptr)
	{
		return;
	}

	UBBMInventorySlot* Slot = nullptr;
	if (const UDISEnhancementContainer* Container = ActiveEnhancement.GetContainer())
	{
		Slot = Cast<UBBMInventorySlot>(Container->GetOwnerInterface().GetObject());
	}

	if (Slot == nullptr)
	{
		return;
	}

	if (UDISEnhancementContainer* ItemContainer = ItemEntry->GetEnhancementContainer())
	{
		ItemContainer->RemoveEnhancementEffectsByInstigator(Slot, GrantedBeatEffectClass);
	}
}

void UBBMSlotEnhancement::OnEnhancementLevelChanged_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement, int32 OldLevel, int32 NewLevel) const
{
	Super::OnEnhancementLevelChanged_Implementation(Target, ActiveEnhancement, OldLevel, NewLevel);

	if (OldLevel == NewLevel)
	{
		return;
	}

	UDISItemEntry* ItemEntry = Cast<UDISItemEntry>(Target);
	if (ItemEntry == nullptr || GrantedBeatEffectClass == nullptr)
	{
		return;
	}

	UBBMInventorySlot* Slot = nullptr;
	if (const UDISEnhancementContainer* Container = ActiveEnhancement.GetContainer())
	{
		Slot = Cast<UBBMInventorySlot>(Container->GetOwnerInterface().GetObject());
	}

	if (Slot == nullptr)
	{
		return;
	}

	if (UDISEnhancementContainer* ItemContainer = ItemEntry->GetEnhancementContainer())
	{
		// 기존 효과 회수 (해당 슬롯(Instigator)이 부여했던 효과만 특정 회수)
		ItemContainer->RemoveEnhancementEffectsByInstigator(Slot, GrantedBeatEffectClass);

		// 신규 레벨 > 0 이면 재적용
		if (NewLevel > 0)
		{
			ItemContainer->ApplyEnhancementEffectSpec(
				FDISEnhancementEffectSpec(GrantedBeatEffectClass, static_cast<float>(NewLevel), Slot)
			);
		}
	}
}

TSubclassOf<UDISEnhancementEffect> UBBMSlotEnhancement::GetGrantedBeatEffectClass() const
{
	return GrantedBeatEffectClass;
}

UBBMSlotEffectRangeEnhancement::UBBMSlotEffectRangeEnhancement()
{
	// 슬롯에 적용 시 소유 아이템에게 부여할 비트 강화 효과 클래스 지정
	GrantedBeatEffectClass = UBBMEffectRangeBeatEnhancementEffect::StaticClass();
}

UBBMSlotConditionalActionEnhancement::UBBMSlotConditionalActionEnhancement()
{
	// 슬롯에 적용 시 소유 아이템에게 부여할 비트 강화 효과 클래스 지정
	GrantedBeatEffectClass = UBBMConditionalActionBeatEnhancementEffect::StaticClass();
}

