#include "Inventory/BBMInventory.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Inventory/BBMInventorySlot.h"
#include "Item/DISItemStack.h"
#include "Enhancement/DISEnhancementContainer.h"

UBBMInventory::UBBMInventory()
{
}

TArray<UDISItemStack*> UBBMInventory::GetItemStacksInExtractionOrder() const
{
	TArray<UDISItemStack*> Result;

	// 1. 인벤토리 내 고유 아이템 스택 목록 수집
	TSet<UDISItemStack*> UniqueStackSet;
	const int32 TotalSlots = GetSlotCount();

	for (int32 SlotIndex = 0; SlotIndex < TotalSlots; ++SlotIndex)
	{
		if (UDISItemStack* Stack = GetSlotItemStack(SlotIndex))
		{
			if (Stack->IsEmpty() == false)
			{
				UniqueStackSet.Add(Stack);
			}
		}
	}

	if (UniqueStackSet.Num() == 0)
	{
		return Result;
	}

	Result = UniqueStackSet.Array();

	const UBBMInventoryDefinition* BBMDef = Cast<UBBMInventoryDefinition>(InventoryDefinition.Get());
	const EBBMItemExtractionOrder Order = (BBMDef != nullptr)
		? BBMDef->ExtractionOrder
		: EBBMItemExtractionOrder::TopToBottom_LeftToRight;

	// 2. 각 스택의 좌상단(Root) 2D 그리드 좌표를 기준으로 ExtractionOrder에 따라 정렬
	Result.Sort([this, Order](const UDISItemStack& A, const UDISItemStack& B)
	{
		const FIntPoint CoordA = GetItemStackBaseCoord(&A);
		const FIntPoint CoordB = GetItemStackBaseCoord(&B);

		switch (Order)
		{
		case EBBMItemExtractionOrder::TopToBottom_LeftToRight:
			// 상단(Y 오름차순) -> 좌측(X 오름차순)
			if (CoordA.Y != CoordB.Y)
			{
				return CoordA.Y < CoordB.Y;
			}
			return CoordA.X < CoordB.X;

		case EBBMItemExtractionOrder::TopToBottom_RightToLeft:
			// 상단(Y 오름차순) -> 우측(X 내림차순)
			if (CoordA.Y != CoordB.Y)
			{
				return CoordA.Y < CoordB.Y;
			}
			return CoordA.X > CoordB.X;

		case EBBMItemExtractionOrder::LeftToRight_TopToBottom:
			// 좌측(X 오름차순) -> 상단(Y 오름차순)
			if (CoordA.X != CoordB.X)
			{
				return CoordA.X < CoordB.X;
			}
			return CoordA.Y < CoordB.Y;

		case EBBMItemExtractionOrder::RightToLeft_TopToBottom:
			// 우측(X 내림차순) -> 상단(Y 오름차순)
			if (CoordA.X != CoordB.X)
			{
				return CoordA.X > CoordB.X;
			}
			return CoordA.Y < CoordB.Y;

		case EBBMItemExtractionOrder::BottomToTop_LeftToRight:
			// 하단(Y 내림차순) -> 좌측(X 오름차순)
			if (CoordA.Y != CoordB.Y)
			{
				return CoordA.Y > CoordB.Y;
			}
			return CoordA.X < CoordB.X;

		case EBBMItemExtractionOrder::BottomToTop_RightToLeft:
			// 하단(Y 내림차순) -> 우측(X 내림차순)
			if (CoordA.Y != CoordB.Y)
			{
				return CoordA.Y > CoordB.Y;
			}
			return CoordA.X > CoordB.X;

		default:
			return CoordA.Y < CoordB.Y;
		}
	});

	return Result;
}

UDISInventorySlot* UBBMInventory::CreateSlotInstance(int32 SlotIndex)
{
	UBBMInventorySlot* NewSlot = NewObject<UBBMInventorySlot>(this, UBBMInventorySlot::StaticClass());
	NewSlot->InitializeSlot(SlotIndex);

	if (InventoryDefinition != nullptr)
	{
		const FIntPoint Coord = InventoryDefinition->SlotIndexToCoord(SlotIndex);

		if (const UBBMInventoryDefinition* BBMDef = Cast<UBBMInventoryDefinition>(InventoryDefinition.Get()))
		{
			if (const FBBMSlotEffectSpecList* SpecList = BBMDef->FindSlotEnhancements(Coord))
			{
				if (UDISEnhancementContainer* SlotContainer = NewSlot->GetEnhancementContainer())
				{
					for (const FDISEnhancementEffectSpec& Spec : SpecList->Effects)
					{
						SlotContainer->ApplyEnhancementEffectSpec(Spec);
					}
				}
			}
		}
	}

	return NewSlot;
}
