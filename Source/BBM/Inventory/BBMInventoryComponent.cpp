/*****************************************************************//**
 * @file   BBMInventoryComponent.cpp
 * @brief  BBM(비트 배낭 상인) 배낭 인벤토리 래퍼 액터 컴포넌트 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Inventory/BBMInventoryComponent.h"
#include "Inventory/BBMInventory.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Item/DISItemStack.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemTypes.h"
#include "Item/BBMItemDefinition.h"

UBBMInventoryComponent::UBBMInventoryComponent() :
	Inventory(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBBMInventoryComponent::InitializeInventory(const UBBMInventoryDefinition* InDefinition)
{
	if (InDefinition == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent] InitializeInventory failed: InDefinition is nullptr."));
		return;
	}

	// 인벤토리 인스턴스가 없으면 새로 생성
	if (Inventory == nullptr)
	{
		Inventory = NewObject<UBBMInventory>(this, UBBMInventory::StaticClass());
	}

	// 정적 정의 에셋 기반으로 인벤토리 슬롯 및 강화 초기화 실행 (인벤토리 내부에서 정의 참조 보관)
	Inventory->InitializeInventory(InDefinition);
}

void UBBMInventoryComponent::ClearAllInventory()
{
	if (Inventory != nullptr)
	{
		const int32 TotalSlots = Inventory->GetSlotCount();
		for (int32 SlotIdx = 0; SlotIdx < TotalSlots; ++SlotIdx)
		{
			Inventory->ClearSlot(SlotIdx);
		}
	}

	UsedEquipmentStorage.Empty();
}

bool UBBMInventoryComponent::RestoreInventoryFromSaveRecords(
	const TArray<FBBMInventorySlotSaveRecord>& SlotRecords,
	const TArray<FBBMEquipmentSaveRecord>& ArchivedRecords
)
{
	if (Inventory == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent] RestoreInventoryFromSaveRecords failed: Inventory is nullptr."));
		return false;
	}

	// 1. 기존 슬롯 및 분리 보관소 초기화
	ClearAllInventory();

	// 2. 인벤토리 슬롯 아이템 복원
	for (const FBBMInventorySlotSaveRecord& Record : SlotRecords)
	{
		if (Record.SlotIndex == INDEX_NONE || Record.ItemDefinitionPath.IsNull() == true || Record.StackCount <= 0)
		{
			continue;
		}

		UBBMItemDefinition* ItemDef = Cast<UBBMItemDefinition>(Record.ItemDefinitionPath.TryLoad());
		if (ItemDef == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent] Restore failed to load ItemDefinition at path: %s"), *Record.ItemDefinitionPath.ToString());
			continue;
		}

		UDISItemStack* ItemStack = NewObject<UDISItemStack>(Inventory);
		for (int32 i = 0; i < Record.StackCount; ++i)
		{
			UDISItemEntry* Entry = NewObject<UDISItemEntry>(ItemStack);
			Entry->InitializeEntry(ItemDef);
			ItemStack->PushEntry(Entry);
		}

		ItemStack->SetRotation(Record.Rotation);

		const FDISItemPlacementResult PlacementResult = Inventory->SetSlotItemStack(Record.SlotIndex, ItemStack);
		if (PlacementResult.bSuccess == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent] Restore failed to place item at slot %d (Item: %s)"), Record.SlotIndex, *ItemDef->GetName());
		}
	}

	// 3. 분리 보관소(UsedEquipmentStorage) 장비 복원
	for (const FBBMEquipmentSaveRecord& EquipRecord : ArchivedRecords)
	{
		if (EquipRecord.ItemDefinitionPath.IsNull() == true || EquipRecord.StackCount <= 0)
		{
			continue;
		}

		UBBMItemDefinition* EquipDef = Cast<UBBMItemDefinition>(EquipRecord.ItemDefinitionPath.TryLoad());
		if (EquipDef == nullptr)
		{
			continue;
		}

		UDISItemStack* EquipStack = NewObject<UDISItemStack>(this);
		for (int32 i = 0; i < EquipRecord.StackCount; ++i)
		{
			UDISItemEntry* Entry = NewObject<UDISItemEntry>(EquipStack);
			Entry->InitializeEntry(EquipDef);
			EquipStack->PushEntry(Entry);
		}

		EquipStack->SetRotation(EquipRecord.Rotation);
		UsedEquipmentStorage.Add(EquipStack);
	}

	return true;
}

UBBMInventory* UBBMInventoryComponent::GetInventory() const
{
	return Inventory;
}

const UBBMInventoryDefinition* UBBMInventoryComponent::GetInventoryDefinition() const
{
	if (Inventory != nullptr)
	{
		return Cast<const UBBMInventoryDefinition>(Inventory->GetInventoryDefinition());
	}
	return nullptr;
}

TArray<UDISItemStack*> UBBMInventoryComponent::GetItemStacksInExtractionOrder() const
{
	if (Inventory != nullptr)
	{
		return Inventory->GetItemStacksInExtractionOrder();
	}
	return TArray<UDISItemStack*>();
}

bool UBBMInventoryComponent::HasValidInventory() const
{
	return Inventory != nullptr;
}

bool UBBMInventoryComponent::ArchiveEquipment(UDISItemStack* InItemStack)
{
	if (InItemStack == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::ArchiveEquipment] InItemStack is nullptr."));
		return false;
	}

	if (Inventory == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::ArchiveEquipment] Inventory is nullptr."));
		return false;
	}

	// 이미 보관소에 보관 중인 경우 중복 보관 방지
	if (IsEquipmentArchived(InItemStack) == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::ArchiveEquipment] ItemStack is already archived in UsedEquipmentStorage."));
		return false;
	}

	// 인벤토리 내 좌상단 루트 슬롯 인덱스 조회 (배치되어 있지 않은 경우 실패)
	const int32 RootSlotIndex = Inventory->GetItemStackRootSlot(InItemStack);
	if (RootSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::ArchiveEquipment] ItemStack is not placed in the inventory."));
		return false;
	}

	// 배낭 인벤토리 슬롯 점유 해제
	const bool bCleared = Inventory->ClearItemStack(InItemStack);
	if (bCleared == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::ArchiveEquipment] Failed to clear ItemStack from inventory."));
		return false;
	}

	// 일시 보관소 목록에 추가
	UsedEquipmentStorage.Add(InItemStack);
	return true;
}

bool UBBMInventoryComponent::ArchiveEquipmentAtSlot(int32 SlotIndex)
{
	if (Inventory == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::ArchiveEquipmentAtSlot] Inventory is nullptr."));
		return false;
	}

	UDISItemStack* StackAtSlot = Inventory->GetSlotItemStack(SlotIndex);
	if (StackAtSlot == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::ArchiveEquipmentAtSlot] No ItemStack found at slot %d."), SlotIndex);
		return false;
	}

	return ArchiveEquipment(StackAtSlot);
}

bool UBBMInventoryComponent::RestoreArchivedEquipmentToSlot(UDISItemStack* InItemStack, int32 TargetSlotIndex)
{
	if (InItemStack == nullptr || Inventory == nullptr)
	{
		return false;
	}

	const int32 StackIndex = UsedEquipmentStorage.Find(InItemStack);
	if (StackIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::RestoreArchivedEquipmentToSlot] ItemStack is not found in UsedEquipmentStorage."));
		return false;
	}

	// 유저가 선택한 슬롯에 수용 가능한지 검증
	if (Inventory->CanAcceptItemStackAtSlot(InItemStack, TargetSlotIndex) == false)
	{
		return false;
	}

	// 유저가 선택한 슬롯에 배치
	const FDISItemPlacementResult PlacementResult = Inventory->SetSlotItemStack(TargetSlotIndex, InItemStack);
	if (PlacementResult.bSuccess == true)
	{
		UsedEquipmentStorage.RemoveAt(StackIndex);
		return true;
	}

	return false;
}

UDISItemStack* UBBMInventoryComponent::TakeArchivedEquipment(UDISItemStack* InItemStack)
{
	if (InItemStack == nullptr)
	{
		return nullptr;
	}

	const int32 StackIndex = UsedEquipmentStorage.Find(InItemStack);
	if (StackIndex != INDEX_NONE)
	{
		UDISItemStack* ExtractedStack = UsedEquipmentStorage[StackIndex];
		UsedEquipmentStorage.RemoveAt(StackIndex);
		return ExtractedStack;
	}

	return nullptr;
}

bool UBBMInventoryComponent::RestoreArchivedEquipment(UDISItemStack* InItemStack, bool bTryRotateToFit)
{
	if (InItemStack == nullptr || Inventory == nullptr)
	{
		return false;
	}

	const int32 StackIndex = UsedEquipmentStorage.Find(InItemStack);
	if (StackIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::RestoreArchivedEquipment] ItemStack is not found in UsedEquipmentStorage."));
		return false;
	}

	// 비어있는 공간에 최대한 채워 배치 시도
	if (TryFitItemStackIntoEmptySpace(InItemStack, bTryRotateToFit) == true)
	{
		UsedEquipmentStorage.RemoveAt(StackIndex);
		return true;
	}

	return false;
}

int32 UBBMInventoryComponent::RestoreAllArchivedEquipment(TArray<UDISItemStack*>& OutFailedToRestoreStacks, bool bTryRotateToFit)
{
	OutFailedToRestoreStacks.Reset();

	if (Inventory == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMInventoryComponent::RestoreAllArchivedEquipment] Inventory is nullptr."));
		for (const TObjectPtr<UDISItemStack>& Stack : UsedEquipmentStorage)
		{
			if (Stack != nullptr)
			{
				OutFailedToRestoreStacks.Add(Stack);
			}
		}
		return 0;
	}

	int32 RestoredCount = 0;

	// 뒤에서부터 순회하여 성공 시 보관소에서 안전하게 제거
	for (int32 Index = UsedEquipmentStorage.Num() - 1; Index >= 0; --Index)
	{
		UDISItemStack* Stack = UsedEquipmentStorage[Index];
		if (Stack == nullptr)
		{
			UsedEquipmentStorage.RemoveAt(Index);
			continue;
		}

		// 비어있는 공간에 최대한 채워 배치 시도
		if (TryFitItemStackIntoEmptySpace(Stack, bTryRotateToFit) == true)
		{
			UsedEquipmentStorage.RemoveAt(Index);
			++RestoredCount;
		}
		else
		{
			OutFailedToRestoreStacks.Add(Stack);
		}
	}

	return RestoredCount;
}

bool UBBMInventoryComponent::IsEquipmentArchived(const UDISItemStack* InItemStack) const
{
	if (InItemStack == nullptr)
	{
		return false;
	}

	return UsedEquipmentStorage.Contains(InItemStack);
}

TArray<UDISItemStack*> UBBMInventoryComponent::GetArchivedEquipmentStacks() const
{
	TArray<UDISItemStack*> Result;
	Result.Reserve(UsedEquipmentStorage.Num());
	for (const TObjectPtr<UDISItemStack>& Stack : UsedEquipmentStorage)
	{
		if (Stack != nullptr)
		{
			Result.Add(Stack);
		}
	}
	return Result;
}

int32 UBBMInventoryComponent::GetArchivedEquipmentCount() const
{
	return UsedEquipmentStorage.Num();
}

bool UBBMInventoryComponent::HasArchivedEquipment() const
{
	return UsedEquipmentStorage.IsEmpty() == false;
}

void UBBMInventoryComponent::ClearArchivedEquipment()
{
	UsedEquipmentStorage.Reset();
}

bool UBBMInventoryComponent::TryFitItemStackIntoEmptySpace(UDISItemStack* InItemStack, bool bTryRotateToFit)
{
	if (InItemStack == nullptr || Inventory == nullptr)
	{
		return false;
	}

	const int32 TotalSlotCount = Inventory->GetSlotCount();
	if (TotalSlotCount <= 0)
	{
		return false;
	}

	const EDISItemRotation InitialRotation = InItemStack->GetRotation();

	// 시도할 회전 각도 목록 구성
	TArray<EDISItemRotation, TInlineAllocator<4>> RotationsToTry;
	RotationsToTry.Add(InitialRotation);

	if (bTryRotateToFit == true && InItemStack->CanRotate() == true)
	{
		const EDISItemRotation AllRotations[] = {
			EDISItemRotation::Rotate0,
			EDISItemRotation::Rotate90,
			EDISItemRotation::Rotate180,
			EDISItemRotation::Rotate270
		};

		for (EDISItemRotation Rot : AllRotations)
		{
			if (RotationsToTry.Contains(Rot) == false)
			{
				RotationsToTry.Add(Rot);
			}
		}
	}

	// 회전 각도별로 슬롯을 순회하여 배치 가능한 비어있는 공간 탐색
	for (EDISItemRotation Rot : RotationsToTry)
	{
		InItemStack->SetRotation(Rot);

		for (int32 SlotIdx = 0; SlotIdx < TotalSlotCount; ++SlotIdx)
		{
			if (Inventory->CanAcceptItemStackAtSlot(InItemStack, SlotIdx) == true)
			{
				const FDISItemPlacementResult PlacementResult = Inventory->SetSlotItemStack(SlotIdx, InItemStack);
				if (PlacementResult.bSuccess == true)
				{
					return true;
				}
			}
		}
	}

	// 모든 회전과 슬롯에서 배치 실패 시 초기 회전 상태로 복구
	InItemStack->SetRotation(InitialRotation);
	return false;
}
