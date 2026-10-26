#include "Misc/AutomationTest.h"
#include "Inventory/DISInventoryDefinition.h"
#include "Inventory/DISInventory.h"
#include "Inventory/DISInventorySlot.h"
#include "Inventory/DISInventoryFilter.h"
#include "Item/DISItemDefinition.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemPlacementRule.h"
#include "Item/DISItemStack.h"
#include "Global/DISGlobalSettings.h"
#include "Enhancement/DISEnhancementContainer.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * DISInventory 및 FDISInventorySlot의 초기화, 슬롯 관리, 확장 용량 검증 (TInstancedStruct 기반)
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventoryCapacityTest,
	"DIS.Inventory.CapacityAndSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventoryCapacityTest::RunTest(const FString& Parameters)
{
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 5;
	Def->MaxExpandableCapacity = 10;
	TestTrue(TEXT("DefaultCapacity < MaxExpandableCapacity이므로 확장 가능해야 함"), Def->CanExpand());

	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	// 1. 초기 슬롯 개수 및 인덱스 정합성 검증
	TestEqual(TEXT("초기 슬롯 개수는 5여야 함"), Inventory->GetSlotCount(), 5);
	for (int32 i = 0; i < 5; ++i)
	{
		UDISInventorySlot* Slot = Inventory->GetSlot(i);
		TestNotNull(TEXT("슬롯 포인터는 유효해야 함"), Slot);
		if (Slot != nullptr)
		{
			TestEqual(TEXT("슬롯 인덱스 정합성 검증"), Slot->GetSlotIndex(), i);
			TestTrue(TEXT("초기 슬롯은 비어있어야 함"), Slot->IsEmpty());
		}
	}

	// 2. 용량 확장 검증 (AddSlots / ExpandCapacity)
	const bool bExpandSuccess = Inventory->AddSlots(3);
	TestTrue(TEXT("용량 3칸 확장은 성공해야 함"), bExpandSuccess);
	TestEqual(TEXT("확장 후 총 슬롯 수는 8이어야 함"), Inventory->GetSlotCount(), 8);

	// 3. 최대 용량 초과 확장 방지 검증 (8 + 3 = 11 > 10)
	const bool bOverExpandSuccess = Inventory->AddSlots(3);
	TestFalse(TEXT("최대 용량 10을 초과하는 확장은 실패해야 함"), bOverExpandSuccess);
	TestEqual(TEXT("슬롯 수는 여전히 8이어야 함"), Inventory->GetSlotCount(), 8);

	// 4. 슬롯 축소 검증 (RemoveSlots 빈 슬롯 축소)
	const bool bRemoveSuccess = Inventory->RemoveSlots(2);
	TestTrue(TEXT("빈 슬롯 2개 축소는 성공해야 함"), bRemoveSuccess);
	TestEqual(TEXT("축소 후 총 슬롯 수는 6이어야 함"), Inventory->GetSlotCount(), 6);

	// 5. 아이템이 있는 슬롯 축소 안전장치 검증 (안전 모드 실패 vs Force 모드 성공 및 퇴출 아이템 수집)
	UDISItemDefinition* ItemDef = NewObject<UDISItemDefinition>();
	UDISItemEntry* Entry = NewObject<UDISItemEntry>();
	Entry->InitializeEntry(ItemDef);
	UDISItemStack* Stack = NewObject<UDISItemStack>();
	Stack->PushEntry(Entry);
	Inventory->SetSlotItemStack(5, Stack); // 마지막 슬롯(인덱스 5)에 아이템 스택 배치

	const bool bSafeRemoveFail = Inventory->RemoveSlots(1);
	TestFalse(TEXT("아이템이 있는 슬롯 축소는 안전 모드에서 실패해야 함"), bSafeRemoveFail);
	TestEqual(TEXT("아이템 보호로 슬롯 수는 여전히 6이어야 함"), Inventory->GetSlotCount(), 6);

	TArray<UDISItemStack*> EvictedStacks;
	const bool bForceRemoveSuccess = Inventory->ForceRemoveSlots(1, EvictedStacks);
	TestTrue(TEXT("ForceRemoveSlots 시에는 강제 축소 성공"), bForceRemoveSuccess);
	TestEqual(TEXT("강제 축소 후 슬롯 수는 5여야 함"), Inventory->GetSlotCount(), 5);
	TestEqual(TEXT("퇴출된 아이템 스택 개수는 1개여야 함"), EvictedStacks.Num(), 1);
	if (EvictedStacks.Num() > 0)
	{
		TestEqual(TEXT("퇴출된 아이템 스택 일치 검증"), EvictedStacks[0], Stack);
	}

	// 6. ResetSlotsToDefault 기본 용량 복귀 검증
	Inventory->AddSlots(3); // 5 -> 8
	TestEqual(TEXT("3칸 확장 후 슬롯 수 8 검증"), Inventory->GetSlotCount(), 8);
	const bool bResetSuccess = Inventory->ResetSlotsToDefault();
	TestTrue(TEXT("ResetSlotsToDefault 성공 검증"), bResetSuccess);
	TestEqual(TEXT("기본 용량(5)으로 복귀 검증"), Inventory->GetSlotCount(), 5);

	// 7. 슬롯 크기 변동에 따른 PlacementRule(모서리 요건) 불일치 아이템 자동 퇴출 검증
	UDISInventoryDefinition* CornerDef = NewObject<UDISInventoryDefinition>();
	CornerDef->DefaultCapacity = 4;
	CornerDef->MaxExpandableCapacity = 10;
	CornerDef->LineBreakTileCount = 2; // 1줄당 2칸 (가로 우선) -> 4칸 시 2행 2열: 0, 1, 2, 3 모두 모서리
	CornerDef->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	UDISInventory* CornerInventory = NewObject<UDISInventory>();
	CornerInventory->InitializeInventory(CornerDef);

	UDISItemDefinition* CornerItemDef = NewObject<UDISItemDefinition>();
	CornerItemDef->DefaultPlacementRules.Add(TInstancedStruct<FDISItemPlacementRule>::Make<FDISCornerPlacementRule>());

	UDISItemEntry* CornerItemEntry = NewObject<UDISItemEntry>();
	CornerItemEntry->InitializeEntry(CornerItemDef);
	UDISItemStack* CornerStack = NewObject<UDISItemStack>();
	CornerStack->PushEntry(CornerItemEntry);

	// 3번 슬롯(1, 1 - 초기 4칸 기준 우측 하단 모서리)에 배치
	TestTrue(TEXT("초기 4칸 인벤토리에서 3번 슬롯은 모서리이므로 배치 성공해야 함"), CornerInventory->SetSlotItemStack(3, CornerStack));

	// 4칸에서 6칸으로 확장 시: 6칸 / 줄바꿈 2 = 3행 그리드가 되어 3번 슬롯(1, 1) 아래에 (1, 2) 슬롯이 생겨 더 이상 모서리가 아님!
	// 안전 모드 ResizeSlots는 규칙 불일치 아이템이 발생하므로 실패해야 함
	const bool bSafeExpandFail = CornerInventory->AddSlots(2);
	TestFalse(TEXT("PlacementRule 불일치 아이템 발생 시 안전 모드 확장은 실패해야 함"), bSafeExpandFail);
	TestEqual(TEXT("실패 후 슬롯 수는 원래대로 4 유지"), CornerInventory->GetSlotCount(), 4);

	// ForceAddSlots 시에는 슬롯이 확장되고, 모서리 요건을 상실한 3번 슬롯의 아이템이 자동으로 퇴출되어야 함!
	TArray<UDISItemStack*> RuleEvictedStacks;
	const bool bForceExpandSuccess = CornerInventory->ForceAddSlots(2, RuleEvictedStacks);
	TestTrue(TEXT("ForceAddSlots는 강제 확장 성공"), bForceExpandSuccess);
	TestEqual(TEXT("확장 후 총 슬롯 수는 6이어야 함"), CornerInventory->GetSlotCount(), 6);
	TestEqual(TEXT("PlacementRule 불일치로 퇴출된 아이템 개수는 1개여야 함"), RuleEvictedStacks.Num(), 1);
	if (RuleEvictedStacks.Num() > 0)
	{
		TestEqual(TEXT("퇴출된 아이템 스택 검증"), RuleEvictedStacks[0], CornerStack);
	}
	TestTrue(TEXT("퇴출된 3번 슬롯은 비어있어야 함"), CornerInventory->IsSlotEmpty(3));

	return true;
}

/**
 * DISInventory의 무게 시스템(활성화/비활성화, Override, 적재 무게 합산) 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventoryWeightTest,
	"DIS.Inventory.WeightSystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventoryWeightTest::RunTest(const FString& Parameters)
{
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 4;
	Def->bUseWeightSystem = true;
	Def->DefaultMaxWeight = 50.0f;

	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	// 1. 기본 무게 및 런타임 변경/초기화 검증
	TestEqual(TEXT("초기 최대 무게는 정의의 50.0f여야 함"), Inventory->GetMaxWeight(), 50.0f);

	Inventory->SetMaxWeight(75.0f);
	TestEqual(TEXT("SetMaxWeight 후 최대 무게는 75.0f여야 함"), Inventory->GetMaxWeight(), 75.0f);

	Inventory->ResetMaxWeight();
	TestEqual(TEXT("ResetMaxWeight 후 최대 무게는 정의의 50.0f로 복구되어야 함"), Inventory->GetMaxWeight(), 50.0f);

	// 2. 아이템 슬롯 배치 및 총 무게 계산 검증
	UDISItemDefinition* ItemDefA = NewObject<UDISItemDefinition>();
	ItemDefA->Weight = 10.0f;
	UDISItemEntry* ItemEntryA = NewObject<UDISItemEntry>();
	ItemEntryA->InitializeEntry(ItemDefA);

	UDISItemEntry* ItemEntryB = NewObject<UDISItemEntry>();
	ItemEntryB->InitializeEntry(ItemDefA);

	UDISInventorySlot* Slot0 = Inventory->GetSlot(0);
	UDISInventorySlot* Slot1 = Inventory->GetSlot(1);
	TestNotNull(TEXT("0번 슬롯이 유효해야 함"), Slot0);
	TestNotNull(TEXT("1번 슬롯이 유효해야 함"), Slot1);
	if (Slot0 != nullptr && Slot1 != nullptr)
	{
		UDISItemStack* Stack0 = NewObject<UDISItemStack>();
		Stack0->PushEntry(ItemEntryA);
		Slot0->SetItemStack(Stack0); // 10.0f

		UDISItemStack* Stack1 = NewObject<UDISItemStack>();
		Stack1->PushEntry(ItemEntryB);
		Slot1->SetItemStack(Stack1); // 10.0f -> 총 20.0f
	}

	TestEqual(TEXT("현재 총 적재 무게는 20.0f여야 함"), Inventory->GetCurrentWeight(), 20.0f);

	// 3. 무게 초과 아이템 진입 거부 검증 (현재 20.0f, 남은 용량 30.0f -> 무게 35.0f 아이템 거부)
	UDISItemDefinition* HeavyItemDef = NewObject<UDISItemDefinition>();
	HeavyItemDef->Weight = 35.0f;
	UDISItemEntry* HeavyEntry = NewObject<UDISItemEntry>();
	HeavyEntry->InitializeEntry(HeavyItemDef);
	UDISItemStack* HeavyStack = NewObject<UDISItemStack>();
	HeavyStack->PushEntry(HeavyEntry);

	TestFalse(TEXT("최대 적재 무게를 초과하는 아이템은 CanAcceptItemStack에서 거부되어야 함"), Inventory->CanAcceptItemStack(HeavyStack));

	return true;
}

/**
 * DISInventory 및 FDISInventorySlot의 TInstancedStruct 기반 동적 필터 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventoryFilterTest,
	"DIS.Inventory.FilterSystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventoryFilterTest::RunTest(const FString& Parameters)
{
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 2;

	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	// 아이템 정의 생성
	const FGameplayTag PotionTag = FGameplayTag::RequestGameplayTag(FName("Item.Consumable.Potion"), false);
	const FGameplayTag WeaponTag = FGameplayTag::RequestGameplayTag(FName("Item.Equipment.Weapon"), false);

	UDISItemDefinition* PotionDef = NewObject<UDISItemDefinition>();
	PotionDef->ItemTypeTag = PotionTag;
	UDISItemEntry* PotionEntry = NewObject<UDISItemEntry>();
	PotionEntry->InitializeEntry(PotionDef);

	UDISItemDefinition* WeaponDef = NewObject<UDISItemDefinition>();
	WeaponDef->ItemTypeTag = WeaponTag;
	UDISItemEntry* WeaponEntry = NewObject<UDISItemEntry>();
	WeaponEntry->InitializeEntry(WeaponDef);

	// 1. 태그 필터 생성: WeaponTag 거부
	FDISTagInventoryFilter& DenyWeaponFilter = Inventory->EmplaceFilter<FDISTagInventoryFilter>();
	if (WeaponTag.IsValid() == true)
	{
		DenyWeaponFilter.DeniedTags.AddTag(WeaponTag);
	}

	UDISItemStack* PotionStack = NewObject<UDISItemStack>();
	PotionStack->PushEntry(PotionEntry);
	UDISItemStack* WeaponStack = NewObject<UDISItemStack>();
	WeaponStack->PushEntry(WeaponEntry);

	if (WeaponTag.IsValid() == true)
	{
		TestFalse(TEXT("Weapon 태그를 가진 아이템은 인벤토리 진입이 거부되어야 함"), Inventory->CanAcceptItemStack(WeaponStack));
		TestTrue(TEXT("Potion 태그를 가진 아이템은 인벤토리 진입이 허용되어야 함"), Inventory->CanAcceptItemStack(PotionStack));
	}

	// 2. 슬롯 전용 필터 검증
	UDISInventorySlot* Slot0 = Inventory->GetSlot(0);
	TestNotNull(TEXT("0번 슬롯이 유효해야 함"), Slot0);

	if (Slot0 != nullptr)
	{
		FDISTagInventoryFilter& RequirePotionFilter = Slot0->EmplaceFilter<FDISTagInventoryFilter>();
		if (PotionTag.IsValid() == true)
		{
			RequirePotionFilter.RequiredTags.AddTag(PotionTag);
		}

		if (PotionTag.IsValid() == true)
		{
			TestTrue(TEXT("포션 필터가 설정된 슬롯은 포션을 수용해야 함"), Slot0->CanAcceptItemStack(PotionStack));
			TestFalse(TEXT("포션 필터가 설정된 슬롯은 무기를 수용하지 않아야 함"), Slot0->CanAcceptItemStack(WeaponStack));
		}

		// 슬롯 필터 제거 후 재검증
		Slot0->ClearFilters();
		TestTrue(TEXT("필터가 제거된 슬롯은 포션을 수용해야 함"), Slot0->CanAcceptItemStack(PotionStack));
	}

	return true;
}

/**
 * 인벤토리의 슬롯 구조체 타입 결정 및 팩토리 메서드 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventorySlotFactoryTest,
	"DIS.Inventory.SlotFactory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventorySlotFactoryTest::RunTest(const FString& Parameters)
{
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 2;
	Def->DefaultSlotClass = UDISInventorySlot::StaticClass();

	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	const TArray<TObjectPtr<UDISInventorySlot>>& Slots = Inventory->GetSlots();
	TestEqual(TEXT("슬롯 개수는 2여야 함"), Slots.Num(), 2);
	for (const TObjectPtr<UDISInventorySlot>& Slot : Slots)
	{
		TestNotNull(TEXT("슬롯 객체는 유효해야 함"), Slot.Get());
		TestTrue(TEXT("슬롯의 클래스는 UDISInventorySlot이어야 함"), Slot->IsA<UDISInventorySlot>());
	}

	return true;
}

/**
 * 인벤토리의 공간 타일 레이아웃, 인덱싱 방향 및 블록 타일 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventorySpatialLayoutTest,
	"DIS.Inventory.SpatialLayoutAndBlockedTiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventorySpatialLayoutTest::RunTest(const FString& Parameters)
{
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 16;
	Def->MaxExpandableCapacity = 16;
	Def->LineBreakTileCount = 4;
	Def->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	// 1. 가로 우선(Horizontal) 좌표 변환 검증
	// 슬롯 6 -> 6 % 4 = 2 (X), 6 / 4 = 1 (Y)
	const FIntPoint Coord6 = Def->SlotIndexToCoord(6);
	TestEqual(TEXT("가로 우선 슬롯 6의 X좌표는 2여야 함"), Coord6.X, 2);
	TestEqual(TEXT("가로 우선 슬롯 6의 Y좌표는 1이어야 함"), Coord6.Y, 1);
	TestEqual(TEXT("좌표 (2, 1)을 슬롯 인덱스로 역변환 시 6이어야 함"), Def->CoordToSlotIndex(Coord6), 6);

	// 2. 세로 우선(Vertical) 좌표 변환 검증
	Def->SlotIndexingDirection = EDISSlotIndexingDirection::Vertical;
	// 슬롯 6 -> 6 / 4 = 1 (X), 6 % 4 = 2 (Y)
	const FIntPoint Coord6V = Def->SlotIndexToCoord(6);
	TestEqual(TEXT("세로 우선 슬롯 6의 X좌표는 1이어야 함"), Coord6V.X, 1);
	TestEqual(TEXT("세로 우선 슬롯 6의 Y좌표는 2여야 함"), Coord6V.Y, 2);
	TestEqual(TEXT("좌표 (1, 2)를 세로 우선 역변환 시 6이어야 함"), Def->CoordToSlotIndex(Coord6V), 6);

	// 3. 블록 타일 형상 검증
	Def->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;
	// (2, 1) 타일 (인덱스 6)을 블록 설정
	Def->BlockedTileShape.Points.Add(FIntPoint(2, 1));

	TestTrue(TEXT("좌표 (2, 1)은 블록된 타일이어야 함"), Def->IsTileBlocked(FIntPoint(2, 1)));
	TestFalse(TEXT("좌표 (0, 0)은 블록되지 않아야 함"), Def->IsTileBlocked(FIntPoint(0, 0)));
	TestTrue(TEXT("슬롯 6번은 블록된 칸이어야 함"), Def->IsSlotBlocked(6));
	TestFalse(TEXT("슬롯 0번은 블록되지 않아야 함"), Def->IsSlotBlocked(0));

	// 4. 인벤토리 인스턴스 연동 및 CanAcceptItemAtSlot 검증
	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	TestTrue(TEXT("인벤토리 6번 슬롯 블록 여부 검증"), Inventory->IsSlotBlocked(6));
	TestFalse(TEXT("인벤토리 0번 슬롯 블록 여부 검증"), Inventory->IsSlotBlocked(0));

	UDISItemDefinition* ItemDef = NewObject<UDISItemDefinition>();
	UDISItemEntry* ItemEntry = NewObject<UDISItemEntry>();
	ItemEntry->InitializeEntry(ItemDef);
	UDISItemStack* ItemStack = NewObject<UDISItemStack>();
	ItemStack->PushEntry(ItemEntry);

	TestFalse(TEXT("블록된 6번 슬롯에는 아이템을 배치할 수 없어야 함"), Inventory->CanAcceptItemStackAtSlot(ItemStack, 6));
	TestTrue(TEXT("정상인 0번 슬롯에는 아이템을 배치할 수 있어야 함"), Inventory->CanAcceptItemStackAtSlot(ItemStack, 0));

	// 5. 블록 타일이 지정되지 않은 일반 슬롯의 블록 판정 및 LineBreakTileCount 유효성 검증
	UDISInventoryDefinition* PlainDef = NewObject<UDISInventoryDefinition>();
	PlainDef->DefaultCapacity = 10;
	PlainDef->MaxExpandableCapacity = 10;
	PlainDef->LineBreakTileCount = 5;
	PlainDef->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	// LineBreakTileCount 기반 2D 레이아웃 좌표 변환 검증
	const FIntPoint Coord3 = PlainDef->SlotIndexToCoord(3);
	TestEqual(TEXT("레이아웃 X좌표는 3이어야 함"), Coord3.X, 3);
	TestEqual(TEXT("레이아웃 Y좌표는 0이어야 함"), Coord3.Y, 0);
	TestEqual(TEXT("좌표 (3, 0)의 슬롯 인덱스는 3이어야 함"), PlainDef->CoordToSlotIndex(Coord3), 3);

	// 블록 설정이 없으므로 기본적으로 모든 슬롯/타일은 블록되지 않아야 함
	TestFalse(TEXT("블록되지 않은 타일 좌표는 IsTileBlocked가 false여야 함"), PlainDef->IsTileBlocked(FIntPoint(1, 1)));
	TestFalse(TEXT("블록되지 않은 슬롯은 IsSlotBlocked가 false여야 함"), PlainDef->IsSlotBlocked(3));

	// LineBreakTileCount가 유효하지 않은 경우 NoneValue / INDEX_NONE 반환 검증
	PlainDef->LineBreakTileCount = 0;
	TestTrue(TEXT("LineBreakTileCount가 0 이하일 때 SlotIndexToCoord는 NoneValue여야 함"), PlainDef->SlotIndexToCoord(3) == FIntPoint::NoneValue);
	TestEqual(TEXT("LineBreakTileCount가 0 이하일 때 CoordToSlotIndex는 INDEX_NONE이어야 함"), PlainDef->CoordToSlotIndex(FIntPoint(1, 1)), INDEX_NONE);

	return true;
}

/**
 * 인벤토리 정적 정의 에셋의 기본 필터(DefaultFilters) 복사 및 동작 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventoryDefaultFilterTest,
	"DIS.Inventory.DefaultFilters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventoryDefaultFilterTest::RunTest(const FString& Parameters)
{
	const FGameplayTag PotionTag = FGameplayTag::RequestGameplayTag(FName("Item.Consumable.Potion"), false);
	const FGameplayTag WeaponTag = FGameplayTag::RequestGameplayTag(FName("Item.Equipment.Weapon"), false);

	// 1. Definition에 기본 필터(WeaponTag 거부) 설정
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 4;
	Def->MaxExpandableCapacity = 4;

	FDISTagInventoryFilter DenyWeaponFilter;
	if (WeaponTag.IsValid() == true)
	{
		DenyWeaponFilter.DeniedTags.AddTag(WeaponTag);
	}
	Def->DefaultFilters.Add(TInstancedStruct<FDISInventoryFilter>::Make<FDISTagInventoryFilter>(DenyWeaponFilter));

	// 2. 인벤토리 인스턴스 생성 및 초기화
	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	// 인벤토리에 기본 필터가 복사되었는지 검증
	TestEqual(TEXT("인벤토리 필터 개수는 1개여야 함"), Inventory->GetFilters().Num(), 1);

	// 3. 아이템 생성
	UDISItemDefinition* PotionDef = NewObject<UDISItemDefinition>();
	PotionDef->ItemTypeTag = PotionTag;
	UDISItemEntry* PotionEntry = NewObject<UDISItemEntry>();
	PotionEntry->InitializeEntry(PotionDef);

	UDISItemDefinition* WeaponDef = NewObject<UDISItemDefinition>();
	WeaponDef->ItemTypeTag = WeaponTag;
	UDISItemEntry* WeaponEntry = NewObject<UDISItemEntry>();
	WeaponEntry->InitializeEntry(WeaponDef);

	UDISItemStack* PotionStack = NewObject<UDISItemStack>();
	PotionStack->PushEntry(PotionEntry);
	UDISItemStack* WeaponStack = NewObject<UDISItemStack>();
	WeaponStack->PushEntry(WeaponEntry);

	if (WeaponTag.IsValid() == true)
	{
		TestFalse(TEXT("정적 기본 필터에 의해 무기 아이템은 인벤토리 진입이 거부되어야 함"), Inventory->CanAcceptItemStack(WeaponStack));
		TestTrue(TEXT("포션 아이템은 인벤토리 진입이 허용되어야 함"), Inventory->CanAcceptItemStack(PotionStack));
	}

	return true;
}

/**
 * 인벤토리 슬롯의 모서리(Corner), 가장자리(Edge), 중앙(Center) 및 BlockTile 인접 구석 판별 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventorySpatialClassificationTest,
	"DIS.Inventory.SpatialClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventorySpatialClassificationTest::RunTest(const FString& Parameters)
{
	// 4x4 (16슬롯, 가로 4칸) 인벤토리 구성
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 16;
	Def->MaxExpandableCapacity = 16;
	Def->LineBreakTileCount = 4;
	Def->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	// 1. 외곽 4개 꼭짓점 모서리 판정 검증
	// (0,0)=0, (3,0)=3, (0,3)=12, (3,3)=15
	TestTrue(TEXT("0번 슬롯(0,0)은 모서리여야 함"), Inventory->IsSlotCorner(0));
	TestTrue(TEXT("3번 슬롯(3,0)은 모서리여야 함"), Inventory->IsSlotCorner(3));
	TestTrue(TEXT("12번 슬롯(0,3)은 모서리여야 함"), Inventory->IsSlotCorner(12));
	TestTrue(TEXT("15번 슬롯(3,3)은 모서리여야 함"), Inventory->IsSlotCorner(15));

	// 2. 외곽이지만 꼭짓점이 아닌 슬롯(가장자리) 판정 검증
	// (1,0)=1, (2,0)=2, (0,1)=4 등
	TestFalse(TEXT("1번 슬롯(1,0)은 모서리가 아니어야 함"), Inventory->IsSlotCorner(1));
	TestTrue(TEXT("1번 슬롯(1,0)은 가장자리여야 함"), Inventory->IsSlotEdge(1));
	TestFalse(TEXT("1번 슬롯(1,0)은 중앙이 아니어야 함"), Inventory->IsSlotCenter(1));

	// 3. 내부 슬롯(중앙) 판정 검증
	// (1,1)=5, (2,1)=6, (1,2)=9, (2,2)=10
	TestFalse(TEXT("5번 슬롯(1,1)은 모서리가 아니어야 함"), Inventory->IsSlotCorner(5));
	TestFalse(TEXT("5번 슬롯(1,1)은 가장자리가 아니어야 함"), Inventory->IsSlotEdge(5));
	TestTrue(TEXT("5번 슬롯(1,1)은 중앙이어야 함"), Inventory->IsSlotCenter(5));

	// 4. BlockTile과 인접하여 형성된 내부 구석(Corner) 판정 검증
	// (1,0)=1번 슬롯과 (0,1)=4번 슬롯을 BlockTile로 설정하면,
	// (1,1)=5번 슬롯은 위(1,0)와 왼쪽(0,1)이 모두 막혀 직각 구석(Corner)이 됨!
	UDISInventoryDefinition* BlockedDef = NewObject<UDISInventoryDefinition>();
	BlockedDef->DefaultCapacity = 16;
	BlockedDef->MaxExpandableCapacity = 16;
	BlockedDef->LineBreakTileCount = 4;
	BlockedDef->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;
	BlockedDef->BlockedTileShape.Points.Add(FIntPoint(1, 0)); // 1번 슬롯
	BlockedDef->BlockedTileShape.Points.Add(FIntPoint(0, 1)); // 4번 슬롯

	UDISInventory* BlockedInv = NewObject<UDISInventory>();
	BlockedInv->InitializeInventory(BlockedDef);

	TestTrue(TEXT("블록 타일과 2면(상+좌) 인접한 5번 슬롯은 모서리로 판정되어야 함"), BlockedInv->IsSlotCorner(5));
	TestFalse(TEXT("구석이 된 5번 슬롯은 더 이상 중앙이 아니어야 함"), BlockedInv->IsSlotCenter(5));

	return true;
}

/**
 * UDISItemEntry의 배치 제한 규칙(Placement Rules) 및 CanAcceptItemAtSlot 연동 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISItemPlacementRuleTest,
	"DIS.Item.PlacementRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISItemPlacementRuleTest::RunTest(const FString& Parameters)
{
	// 4x4 (16슬롯) 인벤토리 구성
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 16;
	Def->MaxExpandableCapacity = 16;
	Def->LineBreakTileCount = 4;
	Def->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	// 1. CornerPlacementRule이 지정된 아이템 정의 생성
	UDISItemDefinition* CornerItemDef = NewObject<UDISItemDefinition>();
	CornerItemDef->DefaultPlacementRules.Add(TInstancedStruct<FDISItemPlacementRule>::Make<FDISCornerPlacementRule>());

	UDISItemEntry* CornerItemEntry = NewObject<UDISItemEntry>();
	CornerItemEntry->InitializeEntry(CornerItemDef);
	UDISItemStack* CornerStack = NewObject<UDISItemStack>();
	CornerStack->PushEntry(CornerItemEntry);

	// 규칙 복사 검증
	TestEqual(TEXT("아이템 엔트리에 기본 배치 규칙 1개가 복사되어야 함"), CornerItemEntry->GetPlacementRules().Num(), 1);

	// 모서리 슬롯(0번)에는 배치 성공, 중앙 슬롯(5번)에는 배치 실패 검증
	TestTrue(TEXT("모서리 슬롯(0번)에는 Corner 규칙 아이템이 배치 가능해야 함"), Inventory->CanAcceptItemStackAtSlot(CornerStack, 0));
	TestFalse(TEXT("중앙 슬롯(5번)에는 Corner 규칙 아이템이 배치 불가해야 함"), Inventory->CanAcceptItemStackAtSlot(CornerStack, 5));

	// 2. CenterPlacementRule이 지정된 아이템 정의 생성
	UDISItemDefinition* CenterItemDef = NewObject<UDISItemDefinition>();
	CenterItemDef->DefaultPlacementRules.Add(TInstancedStruct<FDISItemPlacementRule>::Make<FDISCenterPlacementRule>());

	UDISItemEntry* CenterItemEntry = NewObject<UDISItemEntry>();
	CenterItemEntry->InitializeEntry(CenterItemDef);
	UDISItemStack* CenterStack = NewObject<UDISItemStack>();
	CenterStack->PushEntry(CenterItemEntry);

	// 모서리 슬롯(0번)에는 배치 실패, 중앙 슬롯(5번)에는 배치 성공 검증
	TestFalse(TEXT("모서리 슬롯(0번)에는 Center 규칙 아이템이 배치 불가해야 함"), Inventory->CanAcceptItemStackAtSlot(CenterStack, 0));
	TestTrue(TEXT("중앙 슬롯(5번)에는 Center 규칙 아이템이 배치 가능해야 함"), Inventory->CanAcceptItemStackAtSlot(CenterStack, 5));

	// 3. 런타임에 배치 규칙 제거(ClearPlacementRules) 후 재검증
	CornerItemEntry->ClearPlacementRules();
	TestEqual(TEXT("규칙 제거 후 규칙 개수는 0개여야 함"), CornerItemEntry->GetPlacementRules().Num(), 0);
	TestTrue(TEXT("규칙이 제거된 후에는 중앙 슬롯(5번)에도 정상 배치 가능해야 함"), Inventory->CanAcceptItemStackAtSlot(CornerStack, 5));

	return true;
}

/**
 * UDISItemStack의 내구도 오름차순 정렬 및 최저 내구도 우선 소모(Top) 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISItemStackDurabilityOrderTest,
	"DIS.Item.StackDurabilityOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISItemStackDurabilityOrderTest::RunTest(const FString& Parameters)
{
	UDISItemDefinition* ItemDef = NewObject<UDISItemDefinition>();
	ItemDef->MaxDurability = 100.0f;

	// 내구도가 서로 다른 3개의 동일 아이템 엔트리 생성 (80.0, 30.0, 60.0)
	UDISItemEntry* Entry80 = NewObject<UDISItemEntry>();
	Entry80->InitializeEntry(ItemDef, 80.0f);

	UDISItemEntry* Entry30 = NewObject<UDISItemEntry>();
	Entry30->InitializeEntry(ItemDef, 30.0f);

	UDISItemEntry* Entry60 = NewObject<UDISItemEntry>();
	Entry60->InitializeEntry(ItemDef, 60.0f);

	// 스택 UObject 생성
	UDISItemStack* Stack = NewObject<UDISItemStack>();
	TestNotNull(TEXT("스택 UObject 생성 검증"), Stack);
	if (Stack == nullptr)
	{
		return false;
	}
	TestTrue(TEXT("초기 스택은 비어있어야 함"), Stack->IsEmpty());

	// 임시로 MaxStackCount를 5로 지정하여 삽입 가능하도록 설정
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = 5;

	// 80 -> 30 -> 60 순서로 삽입
	TestTrue(TEXT("80 내구도 아이템 삽입 성공"), Stack->PushEntry(Entry80));
	TestTrue(TEXT("30 내구도 아이템 삽입 성공"), Stack->PushEntry(Entry30));
	TestTrue(TEXT("60 내구도 아이템 삽입 성공"), Stack->PushEntry(Entry60));

	TestEqual(TEXT("총 보관 수량은 3이어야 함"), Stack->GetStackCount(), 3);

	// 최상단 아이템은 내구도가 가장 낮은 30.0f 아이템이어야 함
	UDISItemEntry* Top1 = Stack->GetTopEntry();
	TestNotNull(TEXT("Top 아이템은 유효해야 함"), Top1);
	if (Top1 != nullptr)
	{
		TestEqual(TEXT("Top 아이템의 내구도는 최저치인 30.0f여야 함"), Top1->GetCurrentDurability(), 30.0f);
	}

	// 1차 꺼내기 (Pop) -> 30.0f 엔트리가 꺼내져야 함
	UDISItemEntry* Popped1 = Stack->PopEntry();
	TestEqual(TEXT("1차 꺼낸 엔트리는 Entry30이어야 함"), Popped1, Entry30);
	TestEqual(TEXT("남은 수량은 2여야 함"), Stack->GetStackCount(), 2);

	// 그 다음 최상단은 60.0f 엔트리여야 함
	UDISItemEntry* Top2 = Stack->GetTopEntry();
	TestNotNull(TEXT("Top 아이템은 유효해야 함"), Top2);
	if (Top2 != nullptr)
	{
		TestEqual(TEXT("그 다음 Top 아이템의 내구도는 60.0f여야 함"), Top2->GetCurrentDurability(), 60.0f);
	}

	// 2차 꺼내기 (Pop) -> 60.0f 엔트리
	UDISItemEntry* Popped2 = Stack->PopEntry();
	TestEqual(TEXT("2차 꺼낸 엔트리는 Entry60이어야 함"), Popped2, Entry60);

	// 3차 꺼내기 (Pop) -> 80.0f 엔트리
	UDISItemEntry* Popped3 = Stack->PopEntry();
	TestEqual(TEXT("3차 꺼낸 엔트리는 Entry80이어야 함"), Popped3, Entry80);
	TestTrue(TEXT("모두 꺼낸 후 스택은 비어있어야 함"), Stack->IsEmpty());

	// 설정 복원
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = 1;

	return true;
}

/**
 * FDISItemTypeProfile의 MaxStackCount 한도 및 스택 불가(MaxStackCount <= 1) 판정 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISItemProfileMaxStackTest,
	"DIS.Item.ProfileMaxStack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISItemProfileMaxStackTest::RunTest(const FString& Parameters)
{
	UDISItemDefinition* ItemDef = NewObject<UDISItemDefinition>();
	ItemDef->MaxDurability = 100.0f;

	UDISItemEntry* Entry1 = NewObject<UDISItemEntry>();
	Entry1->InitializeEntry(ItemDef);

	UDISItemEntry* Entry2 = NewObject<UDISItemEntry>();
	Entry2->InitializeEntry(ItemDef);

	UDISItemStack* Stack = NewObject<UDISItemStack>();
	TestNotNull(TEXT("스택 UObject 생성 검증"), Stack);
	if (Stack == nullptr)
	{
		return false;
	}

	// 1. 기본 설정 (MaxStackCount = 1): 스택 불가 검증
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = 1;
	TestFalse(TEXT("MaxStackCount가 1일 때 CanStack()은 false여야 함"), GetDefault<UDISGlobalSettings>()->DefaultItemProfile.CanStack());

	TestTrue(TEXT("비어있는 스택에는 첫 번째 아이템 배치 가능"), Stack->PushEntry(Entry1));
	TestFalse(TEXT("MaxStackCount가 1일 때는 동일 아이템이라도 추가 스택 불가능해야 함"), Stack->PushEntry(Entry2));
	TestEqual(TEXT("스택 수량은 1이어야 함"), Stack->GetStackCount(), 1);

	// 2. MaxStackCount = 2로 변경 시 추가 스택 허용 검증
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = 2;
	TestTrue(TEXT("MaxStackCount가 2일 때 CanStack()은 true여야 함"), GetDefault<UDISGlobalSettings>()->DefaultItemProfile.CanStack());

	TestTrue(TEXT("한도가 2로 늘어났으므로 두 번째 아이템 스택 추가 가능해야 함"), Stack->PushEntry(Entry2));
	TestEqual(TEXT("스택 수량은 2여야 함"), Stack->GetStackCount(), 2);

	// 3. 한도 초과(3번째 아이템) 거부 검증
	UDISItemEntry* Entry3 = NewObject<UDISItemEntry>();
	Entry3->InitializeEntry(ItemDef);
	TestFalse(TEXT("최대 한도인 2에 도달했으므로 3번째 아이템 스택은 거부되어야 함"), Stack->PushEntry(Entry3));

	// 설정 복원
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = 1;

	return true;
}

/**
 * UDISInventorySlot의 UObject 기능 및 IDISEnhanceableInterface 슬롯 강화 컨테이너 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventorySlotEnhancementTest,
	"DIS.Inventory.SlotEnhancement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventorySlotEnhancementTest::RunTest(const FString& Parameters)
{
	UDISInventorySlot* Slot = NewObject<UDISInventorySlot>();
	Slot->InitializeSlot(0);

	// 1. IDISEnhanceableInterface 구현 및 컨테이너 유효성 검증
	IDISEnhanceableInterface* EnhanceableSlot = Cast<IDISEnhanceableInterface>(Slot);
	TestNotNull(TEXT("슬롯은 IDISEnhanceableInterface로 캐스팅 가능해야 함"), EnhanceableSlot);

	UDISEnhancementContainer* Container = Slot->GetEnhancementContainer();
	TestNotNull(TEXT("슬롯의 강화 전담 컨테이너는 유효해야 함"), Container);

	if (Container != nullptr)
	{
		UObject* OwnerObj = Container->GetOwnerInterface().GetObject();
		UObject* ExpectedSlotObj = static_cast<UObject*>(Slot);
		TestEqual(TEXT("컨테이너의 소유자는 슬롯이어야 함"), OwnerObj, ExpectedSlotObj);
	}

	return true;
}

/**
 * 복수 슬롯(SlotA, SlotB)이 동일한 UDISItemStack 인스턴스를 공유하여 공간 점유를 지원하는지 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventorySlotMultiSlotSharingTest,
	"DIS.Inventory.SlotMultiSlotSharing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventorySlotMultiSlotSharingTest::RunTest(const FString& Parameters)
{
	UDISInventorySlot* SlotA = NewObject<UDISInventorySlot>();
	UDISInventorySlot* SlotB = NewObject<UDISInventorySlot>();
	SlotA->InitializeSlot(0);
	SlotB->InitializeSlot(1);

	UDISItemDefinition* ItemDef = NewObject<UDISItemDefinition>();
	ItemDef->MaxDurability = 100.0f;

	UDISItemEntry* Entry = NewObject<UDISItemEntry>();
	Entry->InitializeEntry(ItemDef, 100.0f);

	// 1. 스택 생성 및 아이템 추가
	UDISItemStack* SharedStack = NewObject<UDISItemStack>();
	SharedStack->PushEntry(Entry);

	// 2. SlotA와 SlotB에 동일한 스택 인스턴스 공유 설정 (2칸 차지 아이템 가정)
	SlotA->SetItemStack(SharedStack);
	SlotB->SetItemStack(SharedStack);

	TestEqual(TEXT("SlotA의 스택 객체 일치 검증"), SlotA->GetItemStack(), SharedStack);
	TestEqual(TEXT("SlotB의 스택 객체 일치 검증"), SlotB->GetItemStack(), SharedStack);
	TestEqual(TEXT("SlotA와 SlotB가 가리키는 최상단 엔트리 동일 검증"), SlotA->GetItemStack()->GetTopEntry(), SlotB->GetItemStack()->GetTopEntry());
	TestEqual(TEXT("SlotA와 SlotB의 스택 수량 동일 검증"), SlotA->GetStackCount(), SlotB->GetStackCount());

	// 3. 한쪽 슬롯에서 스택 엔트리 꺼내기(소비) 수행
	UDISItemStack* PoppedStack = SlotA->PopItemStack(1);
	TestNotNull(TEXT("꺼낸 스택 유효성 검증"), PoppedStack);
	if (PoppedStack != nullptr)
	{
		TestEqual(TEXT("꺼낸 엔트리 일치 검증"), PoppedStack->GetTopEntry(), Entry);
	}

	// 4. 스택이 비워졌으므로 SlotB에서도 즉시 비어있는 상태로 관측되어야 함
	TestTrue(TEXT("SlotA 비어있음 검증"), SlotA->IsEmpty());
	TestTrue(TEXT("SlotB 비어있음 동기화 검증"), SlotB->IsEmpty());

	return true;
}

/**
 * UDISInventorySlot에 아이템(또는 ItemStack) 장착 시 내부 엔트리들이 슬롯의 UDISEnhancementContainer의 EffectTargets로 등록되고,
 * 해제 시 EffectTargets에서 제거되는지 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventorySlotItemEnhancementTargetBindingTest,
	"DIS.Inventory.SlotItemEnhancementTargetBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventorySlotItemEnhancementTargetBindingTest::RunTest(const FString& Parameters)
{
	UDISInventorySlot* Slot = NewObject<UDISInventorySlot>();
	Slot->InitializeSlot(0);

	UDISEnhancementContainer* Container = Slot->GetEnhancementContainer();
	TestNotNull(TEXT("슬롯 강화 컨테이너 유효성 검증"), Container);
	if (Container == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("초기 컨테이너의 EffectTargets는 비어있어야 함"), Container->GetEffectTargets().Num(), 0);

	UDISItemDefinition* ItemDef = NewObject<UDISItemDefinition>();
	ItemDef->MaxDurability = 100.0f;

	UDISItemEntry* Entry1 = NewObject<UDISItemEntry>();
	Entry1->InitializeEntry(ItemDef, 100.0f);

	UDISItemEntry* Entry2 = NewObject<UDISItemEntry>();
	Entry2->InitializeEntry(ItemDef, 80.0f);

	// 1. SetItemStack으로 첫 번째 아이템 장착 -> Container의 EffectTargets에 Entry1 등록 검증
	UDISItemStack* Stack1 = NewObject<UDISItemStack>();
	Stack1->PushEntry(Entry1);
	Slot->SetItemStack(Stack1);
	TestEqual(TEXT("Entry1 장착 후 EffectTargets 개수 1 검증"), Container->GetEffectTargets().Num(), 1);
	TestTrue(TEXT("Entry1이 EffectTargets에 포함되어 있어야 함"), Container->GetEffectTargets().Contains(Entry1));

	// 2. 임시로 MaxStackCount를 5로 늘리고 PushItemStack으로 두 번째 아이템 스택 추가 -> Entry2도 등록 검증
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = 5;
	UDISItemStack* Stack2 = NewObject<UDISItemStack>();
	Stack2->PushEntry(Entry2);
	Slot->PushItemStack(Stack2);
	TestEqual(TEXT("Entry2 추가 후 EffectTargets 개수 2 검증"), Container->GetEffectTargets().Num(), 2);
	TestTrue(TEXT("Entry1과 Entry2 모두 EffectTargets에 포함되어 있어야 함"), Container->GetEffectTargets().Contains(Entry1) && Container->GetEffectTargets().Contains(Entry2));

	// 3. PopItemStack으로 1개 꺼냄 -> 꺼내진 엔트리가 EffectTargets에서 제거되었는지 검증
	UDISItemStack* PoppedStack = Slot->PopItemStack(1);
	TestNotNull(TEXT("꺼낸 스택 유효성 검증"), PoppedStack);
	if (PoppedStack != nullptr)
	{
		TestEqual(TEXT("꺼낸 엔트리 일치 검증"), PoppedStack->GetTopEntry(), Entry2);
		TestFalse(TEXT("꺼내진 엔트리는 EffectTargets에서 제거되어야 함"), Container->GetEffectTargets().Contains(PoppedStack->GetTopEntry()));
	}
	TestEqual(TEXT("1개 꺼낸 후 EffectTargets 개수 1 검증"), Container->GetEffectTargets().Num(), 1);

	// 4. ClearSlot으로 슬롯 비우기 -> EffectTargets가 완전히 비워지는지 검증
	UDISItemStack* Cleared = Slot->ClearSlot();
	TestNotNull(TEXT("ClearSlot으로 반환된 스택 유효성 검증"), Cleared);
	TestEqual(TEXT("ClearSlot 후 EffectTargets 개수 0 검증"), Container->GetEffectTargets().Num(), 0);

	// 5. SetItemStack으로 새로운 스택 일괄 장착 및 해제 검증
	UDISItemStack* NewStack = NewObject<UDISItemStack>();
	UDISItemEntry* Entry3 = NewObject<UDISItemEntry>();
	Entry3->InitializeEntry(ItemDef, 50.0f);
	NewStack->PushEntry(Entry3);

	Slot->SetItemStack(NewStack);
	TestEqual(TEXT("SetItemStack 후 EffectTargets 개수 1 검증"), Container->GetEffectTargets().Num(), 1);
	TestTrue(TEXT("Entry3이 EffectTargets에 포함되어 있어야 함"), Container->GetEffectTargets().Contains(Entry3));

	// SetItemStack(nullptr)로 해제 시 EffectTargets 제거 검증
	Slot->SetItemStack(nullptr);
	TestEqual(TEXT("SetItemStack(nullptr) 후 EffectTargets 개수 0 검증"), Container->GetEffectTargets().Num(), 0);

	// 설정 복원
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = 1;

	return true;
}

/**
 * UDISItemStack의 회전(Rotation) 상태, 시계/반시계 조작, bCanRotate 제약 및 상대 점유 타일 좌표(GetOccupiedPoints) 회전 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISItemStackRotationTest,
	"DIS.Item.StackRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISItemStackRotationTest::RunTest(const FString& Parameters)
{
	// 1. 회전 가능한 2x1 가로 직사각형 아이템 정의 생성
	UDISItemDefinition* RotatableDef = NewObject<UDISItemDefinition>();
	RotatableDef->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0) };
	RotatableDef->InitialRotation = EDISItemRotation::Rotate0;
	RotatableDef->bCanRotate = true;

	UDISItemEntry* RotatableEntry = NewObject<UDISItemEntry>();
	RotatableEntry->InitializeEntry(RotatableDef);

	// 스택 생성 및 엔트리 삽입
	UDISItemStack* Stack = NewObject<UDISItemStack>();
	TestNotNull(TEXT("Stack 생성 검증"), Stack);
	if (Stack == nullptr)
	{
		return false;
	}

	TestTrue(TEXT("아이템 삽입 성공"), Stack->PushEntry(RotatableEntry));
	TestTrue(TEXT("CanRotate()가 true여야 함"), Stack->CanRotate());
	TestEqual(TEXT("초기 회전 각도는 Rotate0이어야 함"), Stack->GetRotation(), EDISItemRotation::Rotate0);

	// 2. 0도 상태의 점유 타일 좌표 검증: (0, 0), (1, 0)
	TArray<FIntPoint> Points0 = Stack->GetOccupiedPoints();
	TestEqual(TEXT("점유 타일 수는 2여야 함"), Points0.Num(), 2);
	TestTrue(TEXT("0도: (0,0) 포함 검증"), Points0.Contains(FIntPoint(0, 0)));
	TestTrue(TEXT("0도: (1,0) 포함 검증"), Points0.Contains(FIntPoint(1, 0)));

	// 3. 시계 방향 90도 회전 검증 (0 -> 90)
	TestTrue(TEXT("시계 방향 회전 성공 여부"), Stack->RotateClockwise());
	TestEqual(TEXT("회전 후 각도는 Rotate90이어야 함"), Stack->GetRotation(), EDISItemRotation::Rotate90);

	// 90도 상태의 점유 타일 좌표 검증: (0, 0), (0, 1)
	TArray<FIntPoint> Points90 = Stack->GetOccupiedPoints();
	TestEqual(TEXT("점유 타일 수는 2여야 함"), Points90.Num(), 2);
	TestTrue(TEXT("90도: (0,0) 포함 검증"), Points90.Contains(FIntPoint(0, 0)));
	TestTrue(TEXT("90도: (0,1) 포함 검증"), Points90.Contains(FIntPoint(0, 1)));

	// 4. 시계 방향 순환 회전 검증 (90 -> 180 -> 270 -> 0)
	TestTrue(TEXT("180도 회전 성공"), Stack->RotateClockwise());
	TestEqual(TEXT("현재 각도 Rotate180 검증"), Stack->GetRotation(), EDISItemRotation::Rotate180);

	TestTrue(TEXT("270도 회전 성공"), Stack->RotateClockwise());
	TestEqual(TEXT("현재 각도 Rotate270 검증"), Stack->GetRotation(), EDISItemRotation::Rotate270);

	TestTrue(TEXT("0도 복귀 회전 성공"), Stack->RotateClockwise());
	TestEqual(TEXT("현재 각도 Rotate0 복귀 검증"), Stack->GetRotation(), EDISItemRotation::Rotate0);

	// 5. 반시계 방향 회전 검증 (0 -> 270 -> 180)
	TestTrue(TEXT("반시계 방향 회전 성공"), Stack->RotateCounterClockwise());
	TestEqual(TEXT("현재 각도 Rotate270 검증"), Stack->GetRotation(), EDISItemRotation::Rotate270);

	TestTrue(TEXT("반시계 방향 회전 성공"), Stack->RotateCounterClockwise());
	TestEqual(TEXT("현재 각도 Rotate180 검증"), Stack->GetRotation(), EDISItemRotation::Rotate180);

	// 6. 회전 불가(bCanRotate == false) 아이템에 대한 회전 방어 검증
	UDISItemDefinition* NonRotatableDef = NewObject<UDISItemDefinition>();
	NonRotatableDef->bCanRotate = false;
	NonRotatableDef->InitialRotation = EDISItemRotation::Rotate0;

	UDISItemEntry* NonRotatableEntry = NewObject<UDISItemEntry>();
	NonRotatableEntry->InitializeEntry(NonRotatableDef);

	UDISItemStack* NonRotatableStack = NewObject<UDISItemStack>();
	NonRotatableStack->PushEntry(NonRotatableEntry);

	TestFalse(TEXT("회전 불가 아이템의 CanRotate()는 false여야 함"), NonRotatableStack->CanRotate());
	TestFalse(TEXT("회전 불가 아이템의 RotateClockwise()는 false를 반환해야 함"), NonRotatableStack->RotateClockwise());
	TestEqual(TEXT("회전 불가 아이템의 각도는 Rotate0으로 유지되어야 함"), NonRotatableStack->GetRotation(), EDISItemRotation::Rotate0);
	TestFalse(TEXT("회전 불가 아이템의 RotateCounterClockwise()는 false를 반환해야 함"), NonRotatableStack->RotateCounterClockwise());
	TestEqual(TEXT("회전 불가 아이템의 각도는 Rotate0으로 유지되어야 함"), NonRotatableStack->GetRotation(), EDISItemRotation::Rotate0);

	return true;
}

/**
 * 다중 타일 점유 형상(Footprint) 아이템의 블록 타일 침범, 인벤토리 경계 초과, 기존 아이템과의 점유 충돌 및 일괄 바인딩/해제 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventoryMultiTileSpatialTest,
	"DIS.Inventory.MultiTileSpatial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventoryMultiTileSpatialTest::RunTest(const FString& Parameters)
{
	// 4x4 (16칸, 1줄당 4칸 가로 우선) 그리드 인벤토리 정의 생성
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 16;
	Def->MaxExpandableCapacity = 16;
	Def->LineBreakTileCount = 4;
	Def->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	// (2, 1) 타일 (인덱스 6)을 블록 타일로 설정
	Def->BlockedTileShape.Points.Add(FIntPoint(2, 1));

	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	// 1. 2x2 타일 형상 아이템 정의 생성: (0,0), (1,0), (0,1), (1,1)
	UDISItemDefinition* Item2x2Def = NewObject<UDISItemDefinition>();
	Item2x2Def->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(1, 1) };
	Item2x2Def->bCanRotate = true;

	UDISItemEntry* Item2x2Entry = NewObject<UDISItemEntry>();
	Item2x2Entry->InitializeEntry(Item2x2Def);

	UDISItemStack* Stack2x2 = NewObject<UDISItemStack>();
	Stack2x2->PushEntry(Item2x2Entry);

	// 2. 정상 위치 (0, 0) [인덱스 0]에 2x2 아이템 배치 검증
	// 점유 타일: (0,0)->0, (1,0)->1, (0,1)->4, (1,1)->5 (모두 정상 타일)
	TestTrue(TEXT("0번 슬롯에 2x2 아이템 배치 가능해야 함"), Inventory->CanAcceptItemStackAtSlot(Stack2x2, 0));
	TestTrue(TEXT("0번 슬롯에 2x2 아이템 배치 성공"), Inventory->SetSlotItemStack(0, Stack2x2));

	// 4개 점유 슬롯 전체에 동일 스택이 공유 바인딩되었는지 검증
	TestEqual(TEXT("0번 슬롯 스택 일치"), Inventory->GetSlotItemStack(0), Stack2x2);
	TestEqual(TEXT("1번 슬롯 스택 일치"), Inventory->GetSlotItemStack(1), Stack2x2);
	TestEqual(TEXT("4번 슬롯 스택 일치"), Inventory->GetSlotItemStack(4), Stack2x2);
	TestEqual(TEXT("5번 슬롯 스택 일치"), Inventory->GetSlotItemStack(5), Stack2x2);
	TestFalse(TEXT("0번 슬롯은 비어있지 않아야 함"), Inventory->IsSlotEmpty(0));
	TestFalse(TEXT("5번 슬롯은 비어있지 않아야 함"), Inventory->IsSlotEmpty(5));

	// 3. 기존 점유 아이템과의 충돌(Overlap) 검증
	UDISItemDefinition* SingleDef = NewObject<UDISItemDefinition>();
	UDISItemEntry* SingleEntry = NewObject<UDISItemEntry>();
	SingleEntry->InitializeEntry(SingleDef);
	UDISItemStack* SingleStack = NewObject<UDISItemStack>();
	SingleStack->PushEntry(SingleEntry);

	// 2x2 아이템이 점유 중인 1번, 4번, 5번 슬롯에 다른 아이템 배치 시도 시 거부되어야 함
	TestFalse(TEXT("2x2 아이템이 이미 점유 중인 1번 슬롯에는 다른 아이템 배치 불가"), Inventory->CanAcceptItemStackAtSlot(SingleStack, 1));
	TestFalse(TEXT("2x2 아이템이 이미 점유 중인 5번 슬롯에는 다른 아이템 배치 불가"), Inventory->CanAcceptItemStackAtSlot(SingleStack, 5));

	// 4. 새 아이템이 블록 타일을 침범하는 경우 검증
	// 1번 슬롯(1, 0)에 2x2 배치 시도 -> 점유 타일: (1,0)->1, (2,0)->2, (1,1)->5, (2,1)->6 (6번은 블록 타일!)
	UDISItemEntry* Another2x2Entry = NewObject<UDISItemEntry>();
	Another2x2Entry->InitializeEntry(Item2x2Def);
	UDISItemStack* Another2x2Stack = NewObject<UDISItemStack>();
	Another2x2Stack->PushEntry(Another2x2Entry);

	TestFalse(TEXT("블록 타일(6번)을 포함하게 되는 1번 슬롯에는 2x2 아이템 배치 불가"), Inventory->CanAcceptItemStackAtSlot(Another2x2Stack, 1));

	// 5. 새 아이템이 인벤토리 경계 밖으로 삐져나가는 경우 검증
	// 3번 슬롯(3, 0)은 우측 끝이므로 2x2 배치 시 (4, 0), (4, 1)이 되어 경계 초과!
	TestFalse(TEXT("인벤토리 우측 경계를 벗어나는 3번 슬롯에는 2x2 아이템 배치 불가"), Inventory->CanAcceptItemStackAtSlot(Another2x2Stack, 3));
	// 12번 슬롯(0, 3)은 하단 끝이므로 2x2 배치 시 (0, 4), (1, 4)가 되어 하단 경계 초과!
	TestFalse(TEXT("인벤토리 하단 경계를 벗어나는 12번 슬롯에는 2x2 아이템 배치 불가"), Inventory->CanAcceptItemStackAtSlot(Another2x2Stack, 12));

	// 6. 다중 슬롯 아이템의 ClearSlot 일괄 분리 검증
	// 4번 슬롯(2x2 아이템이 점유 중인 타일 중 하나)을 비울 때, 0, 1, 4, 5번 슬롯이 모두 비워져야 함
	UDISItemStack* ClearedStack = Inventory->ClearSlot(4);
	TestEqual(TEXT("ClearSlot으로 반환된 스택은 Stack2x2와 일치해야 함"), ClearedStack, Stack2x2);
	TestTrue(TEXT("0번 슬롯이 비워져야 함"), Inventory->IsSlotEmpty(0));
	TestTrue(TEXT("1번 슬롯이 비워져야 함"), Inventory->IsSlotEmpty(1));
	TestTrue(TEXT("4번 슬롯이 비워져야 함"), Inventory->IsSlotEmpty(4));
	TestTrue(TEXT("5번 슬롯이 비워져야 함"), Inventory->IsSlotEmpty(5));

	return true;
}

/**
 * 아이템 추가 시 스태킹 가능한 기존 스택과 일부 또는 전체 타일이 겹치는 경우의 스태킹 병합 검증 테스트
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventoryPartialOverlapStackingTest,
	"DIS.Inventory.PartialOverlapStacking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventoryPartialOverlapStackingTest::RunTest(const FString& Parameters)
{
	// 전역 MaxStackCount를 5로 임시 조정
	const int32 OriginalMaxStack = GetDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount;
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = 5;

	UDISInventory* Inventory = NewObject<UDISInventory>();
	UDISInventoryDefinition* InvenDef = NewObject<UDISInventoryDefinition>();
	InvenDef->DefaultCapacity = 16;
	InvenDef->LineBreakTileCount = 4;
	InvenDef->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;
	InvenDef->MaxExpandableCapacity = 32;
	Inventory->InitializeInventory(InvenDef);

	// 1. 1x1 스택 가능 아이템 정의 생성
	UDISItemDefinition* PotionDef = NewObject<UDISItemDefinition>();

	UDISItemEntry* PotionEntry1 = NewObject<UDISItemEntry>();
	PotionEntry1->InitializeEntry(PotionDef);
	UDISItemEntry* PotionEntry2 = NewObject<UDISItemEntry>();
	PotionEntry2->InitializeEntry(PotionDef);

	UDISItemStack* ExistingPotionStack = NewObject<UDISItemStack>();
	ExistingPotionStack->PushEntry(PotionEntry1);
	ExistingPotionStack->PushEntry(PotionEntry2);

	// 슬롯 0에 포션 2개 배치
	TestTrue(TEXT("0번 슬롯에 기존 포션 스택 배치 성공"), Inventory->SetSlotItemStack(0, ExistingPotionStack));
	TestEqual(TEXT("0번 슬롯 포션 수량은 2개"), Inventory->GetSlotStackCount(0), 2);

	// 새 포션 스택(3개) 생성
	UDISItemStack* NewPotionStack = NewObject<UDISItemStack>();
	for (int32 i = 0; i < 3; ++i)
	{
		UDISItemEntry* Entry = NewObject<UDISItemEntry>();
		Entry->InitializeEntry(PotionDef);
		NewPotionStack->PushEntry(Entry);
	}

	// 2. 1x1 완전 일치 겹침 시 스태킹 검증
	TestTrue(TEXT("스태킹 가능한 0번 슬롯에 새 포션 스택 수용 가능해야 함"), Inventory->CanAcceptItemStackAtSlot(NewPotionStack, 0));
	TestTrue(TEXT("0번 슬롯에 새 포션 스택 병합 배치 성공"), Inventory->SetSlotItemStack(0, NewPotionStack));
	TestEqual(TEXT("0번 슬롯 포션 수량은 2+3=5개로 증가해야 함"), Inventory->GetSlotStackCount(0), 5);
	TestTrue(TEXT("새 포션 스택은 모두 흡수되어 비워져야 함"), NewPotionStack->IsEmpty());

	// 3. 꽉 찬 스택(5개)에 추가 시도 시 거부 검증
	UDISItemStack* ExtraPotionStack = NewObject<UDISItemStack>();
	UDISItemEntry* ExtraEntry = NewObject<UDISItemEntry>();
	ExtraEntry->InitializeEntry(PotionDef);
	ExtraPotionStack->PushEntry(ExtraEntry);

	TestFalse(TEXT("이미 최대 스택에 도달한 0번 슬롯에는 추가 수용 불가"), Inventory->CanAcceptItemStackAtSlot(ExtraPotionStack, 0));
	TestFalse(TEXT("최대 스택 슬롯에 SetSlotItemStack 실패해야 함"), Inventory->SetSlotItemStack(0, ExtraPotionStack));

	// 4. 2x2 다중 슬롯 아이템의 일부 타일 겹침 스태킹 검증
	UDISItemDefinition* Scroll2x2Def = NewObject<UDISItemDefinition>();
	Scroll2x2Def->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(1, 1) };

	UDISItemStack* BaseScrollStack = NewObject<UDISItemStack>();
	UDISItemEntry* ScrollEntry1 = NewObject<UDISItemEntry>();
	ScrollEntry1->InitializeEntry(Scroll2x2Def);
	BaseScrollStack->PushEntry(ScrollEntry1);

	// 슬롯 0에 2x2 스크롤 1개 배치 -> 점유 타일: 0(0,0), 1(1,0), 4(0,1), 5(1,1)
	// (기존 포션 슬롯 0을 비우고 배치)
	Inventory->ClearSlot(0);
	TestTrue(TEXT("0번 슬롯에 2x2 스크롤 배치 성공"), Inventory->SetSlotItemStack(0, BaseScrollStack));
	TestEqual(TEXT("0번 슬롯 스크롤 수량 1개"), Inventory->GetSlotStackCount(0), 1);
	TestEqual(TEXT("5번 슬롯 스크롤 수량 1개 (공유 바인딩)"), Inventory->GetSlotStackCount(5), 1);

	// 새 2x2 스크롤 스택(2개) 생성
	UDISItemStack* IncomingScrollStack = NewObject<UDISItemStack>();
	UDISItemEntry* ScrollEntry2 = NewObject<UDISItemEntry>();
	ScrollEntry2->InitializeEntry(Scroll2x2Def);
	UDISItemEntry* ScrollEntry3 = NewObject<UDISItemEntry>();
	ScrollEntry3->InitializeEntry(Scroll2x2Def);
	IncomingScrollStack->PushEntry(ScrollEntry2);
	IncomingScrollStack->PushEntry(ScrollEntry3);

	// 슬롯 5(1, 1)에 새 2x2 스크롤 배치 시도:
	// IncomingScroll의 점유 타일: (1,1)->5, (2,1)->6, (1,2)->9, (2,2)->10
	// 슬롯 5만 기존 2x2 스택과 겹치고, 6, 9, 10번은 비어있음 -> 단 하나의 스태킹 가능한 기존 스택과 일부 타일 겹침!
	TestTrue(TEXT("기존 2x2 스택과 일부 타일(슬롯 5)이 겹치는 위치에 스태킹 가능해야 함"), Inventory->CanAcceptItemStackAtSlot(IncomingScrollStack, 5));
	TestTrue(TEXT("일부 타일 겹침 위치에서 SetSlotItemStack 스태킹 성공해야 함"), Inventory->SetSlotItemStack(5, IncomingScrollStack));

	// 기존 슬롯 0, 1, 4, 5번의 스크롤 스택 수량이 1+2=3개로 증가했는지 검증
	TestEqual(TEXT("0번 슬롯 스택 수량 3개로 증가"), Inventory->GetSlotStackCount(0), 3);
	TestEqual(TEXT("5번 슬롯 스택 수량 3개로 동기화"), Inventory->GetSlotStackCount(5), 3);
	TestTrue(TEXT("새 2x2 스택은 비워져야 함"), IncomingScrollStack->IsEmpty());

	// 겹치지 않았던 슬롯 6, 9, 10번은 여전히 비어있어야 함
	TestTrue(TEXT("6번 슬롯은 비어있어야 함"), Inventory->IsSlotEmpty(6));
	TestTrue(TEXT("9번 슬롯은 비어있어야 함"), Inventory->IsSlotEmpty(9));
	TestTrue(TEXT("10번 슬롯은 비어있어야 함"), Inventory->IsSlotEmpty(10));

	// 5. 다중 스택 충돌 및 유저 선택(팝업) API 검증
	// 가로 2칸 (0,0), (1,0) 형상의 동일한 두루마리 정의 생성
	Inventory->ClearSlot(0);
	UDISItemDefinition* Scroll2x1Def = NewObject<UDISItemDefinition>();
	Scroll2x1Def->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0) };

	// 슬롯 0에 두루마리 스택 1 (수량 1) 배치 -> 점유 타일: 슬롯 0, 슬롯 1
	UDISItemStack* ScrollStack1 = NewObject<UDISItemStack>();
	UDISItemEntry* S1Entry = NewObject<UDISItemEntry>();
	S1Entry->InitializeEntry(Scroll2x1Def);
	ScrollStack1->PushEntry(S1Entry);
	Inventory->SetSlotItemStack(0, ScrollStack1);

	// 슬롯 2에 두루마리 스택 2 (수량 2) 배치 -> 점유 타일: 슬롯 2, 슬롯 3
	UDISItemStack* ScrollStack2 = NewObject<UDISItemStack>();
	for (int32 i = 0; i < 2; ++i)
	{
		UDISItemEntry* S2Entry = NewObject<UDISItemEntry>();
		S2Entry->InitializeEntry(Scroll2x1Def);
		ScrollStack2->PushEntry(S2Entry);
	}
	Inventory->SetSlotItemStack(2, ScrollStack2);

	TestEqual(TEXT("0번 슬롯 두루마리 스택1 수량 1"), Inventory->GetSlotStackCount(0), 1);
	TestEqual(TEXT("1번 슬롯 두루마리 스택1 수량 1 (공유)"), Inventory->GetSlotStackCount(1), 1);
	TestEqual(TEXT("2번 슬롯 두루마리 스택2 수량 2"), Inventory->GetSlotStackCount(2), 2);
	TestEqual(TEXT("3번 슬롯 두루마리 스택2 수량 2 (공유)"), Inventory->GetSlotStackCount(3), 2);

	// 동일한 정의의 새 두루마리 스택 (수량 1) 생성
	UDISItemStack* IncomingScroll = NewObject<UDISItemStack>();
	UDISItemEntry* S3Entry = NewObject<UDISItemEntry>();
	S3Entry->InitializeEntry(Scroll2x1Def);
	IncomingScroll->PushEntry(S3Entry);

	// 슬롯 1(1, 0)에 2x1 두루마리 배치 시도 -> 점유 타일: 슬롯 1(스택1과 겹침), 슬롯 2(스택2와 겹침)!
	// 5-A. 두 겹침 대상 모두 동일한 두루마리이고 스태킹 가능하므로 CanAcceptItemStackAtSlot은 true
	TestTrue(TEXT("다중 충돌이라도 모든 겹침 대상이 스태킹 가능하면 수용 가능해야 함"), Inventory->CanAcceptItemStackAtSlot(IncomingScroll, 1));

	// 5-B. GetStackableCandidatesAtSlot 쿼리 결과에 스택1과 스택2가 모두 후보로 반환되어야 함
	TArray<UDISItemStack*> Candidates = Inventory->GetStackableCandidatesAtSlot(IncomingScroll, 1);
	TestEqual(TEXT("스태킹 가능한 다중 후보 개수는 2개여야 함"), Candidates.Num(), 2);
	TestTrue(TEXT("후보 목록에 ScrollStack1 포함"), Candidates.Contains(ScrollStack1));
	TestTrue(TEXT("후보 목록에 ScrollStack2 포함"), Candidates.Contains(ScrollStack2));

	// 5-C. 선호 타깃 스택 미지정 시: 유저 선택이 필요하므로 SetSlotItemStack은 RequiresSelection 상태 반환 및 실패
	FDISItemPlacementResult SelectionResult = Inventory->SetSlotItemStack(1, IncomingScroll);
	TestFalse(TEXT("다중 후보 상황에서 선호 스택 미지정 시 SetSlotItemStack은 실패(유저 선택 대기)해야 함"), SelectionResult.bSuccess);
	TestEqual(TEXT("선택 대기 상태 ResultType은 RequiresSelection이어야 함"), SelectionResult.ResultType, EDISItemPlacementResult::RequiresSelection);
	TestEqual(TEXT("SelectionResult의 CandidateStacks 개수는 2개여야 함"), SelectionResult.CandidateStacks.Num(), 2);
	TestEqual(TEXT("임의 병합되지 않고 0번 슬롯 수량 여전히 1 유지"), Inventory->GetSlotStackCount(0), 1);
	TestEqual(TEXT("임의 병합되지 않고 2번 슬롯 수량 여전히 2 유지"), Inventory->GetSlotStackCount(2), 2);

	// 5-D. 유저가 팝업 창에서 ScrollStack2를 선택했다고 가정 -> SetSlotItemStack에 PreferredTargetStack 전달하여 병합 성공
	FDISItemPlacementResult MergeResult = Inventory->SetSlotItemStack(1, IncomingScroll, ScrollStack2);
	TestTrue(TEXT("선택된 ScrollStack2를 지정하여 SetSlotItemStack 스태킹 성공"), MergeResult.bSuccess);
	TestEqual(TEXT("완전 병합 ResultType은 FullyMerged이어야 함"), MergeResult.ResultType, EDISItemPlacementResult::FullyMerged);
	TestEqual(TEXT("병합된 수량은 1이어야 함"), MergeResult.MergedCount, 1);
	TestEqual(TEXT("남은 수량은 0이어야 함"), MergeResult.RemainingCount, 0);
	TestEqual(TEXT("0번 슬롯 ScrollStack1은 그대로 1개 유지"), Inventory->GetSlotStackCount(0), 1);
	TestEqual(TEXT("선택된 2번 슬롯 ScrollStack2는 2+1=3개로 증가"), Inventory->GetSlotStackCount(2), 3);
	TestTrue(TEXT("진입했던 두루마리 스택은 완전히 흡수되어 비워져야 함"), IncomingScroll->IsEmpty());

	// 6. 충돌 대상 중 하나라도 스태킹 가능하고 여유 공간이 있으면 수용 및 스태킹 성공 검증
	// 슬롯 8(두루마리4, 2x1 점유: 슬롯 8, 9), 슬롯 10에 스택 불가 무기 아이템 배치
	UDISItemStack* ScrollStack4 = NewObject<UDISItemStack>();
	UDISItemEntry* S4Entry = NewObject<UDISItemEntry>();
	S4Entry->InitializeEntry(Scroll2x1Def);
	ScrollStack4->PushEntry(S4Entry);
	Inventory->SetSlotItemStack(8, ScrollStack4);

	UDISItemDefinition* WeaponDef = NewObject<UDISItemDefinition>();
	UDISItemStack* WeaponStack = NewObject<UDISItemStack>();
	UDISItemEntry* WeaponEntry = NewObject<UDISItemEntry>();
	WeaponEntry->InitializeEntry(WeaponDef);
	WeaponStack->PushEntry(WeaponEntry);
	Inventory->SetSlotItemStack(10, WeaponStack);

	// 슬롯 9에 2x1 두루마리 배치 시도 -> 슬롯 9(두루마리4, 여유 4개) + 슬롯 10(무기, 스택불가)
	// 무기와 겹치더라도 슬롯 9의 두루마리4로 스태킹 흡수 가능하므로 통과!
	UDISItemStack* OverlappingScroll = NewObject<UDISItemStack>();
	UDISItemEntry* S5Entry = NewObject<UDISItemEntry>();
	S5Entry->InitializeEntry(Scroll2x1Def);
	OverlappingScroll->PushEntry(S5Entry);

	TestTrue(TEXT("무기와 겹치더라도 여유 있는 스태킹 대상(두루마리4)이 존재하면 수용 가능해야 함"), Inventory->CanAcceptItemStackAtSlot(OverlappingScroll, 9));
	FDISItemPlacementResult OverlapMergeResult = Inventory->SetSlotItemStack(9, OverlappingScroll);
	TestTrue(TEXT("두루마리4로 정상 흡수되어 SetSlotItemStack 성공해야 함"), OverlapMergeResult.bSuccess);
	TestEqual(TEXT("완전 병합 ResultType 확인"), OverlapMergeResult.ResultType, EDISItemPlacementResult::FullyMerged);
	TestEqual(TEXT("슬롯 8의 두루마리4 수량은 1+1=2개로 증가"), Inventory->GetSlotStackCount(8), 2);
	TestEqual(TEXT("슬롯 10의 무기는 온전히 1개 유지"), Inventory->GetSlotStackCount(10), 1);

	// 7. 부분 스태킹 및 잔여물(찌꺼기) 반환 검증 (PartiallyMerged)
	// 슬롯 8의 ScrollStack4는 현재 2개 (최대 5개이므로 여유는 3개)
	// 여기에 4개짜리 진입 스택(PartialScrollStack)을 드롭 시도
	UDISItemStack* PartialScrollStack = NewObject<UDISItemStack>();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UDISItemEntry* Entry = NewObject<UDISItemEntry>();
		Entry->InitializeEntry(Scroll2x1Def);
		PartialScrollStack->PushEntry(Entry);
	}
	TestEqual(TEXT("진입 스택 초기 수량은 4개"), PartialScrollStack->GetStackCount(), 4);

	// 슬롯 8에 드롭 실행 -> 3개만 흡수되고 1개는 잔여물로 남아야 함
	FDISItemPlacementResult PartialResult = Inventory->SetSlotItemStack(8, PartialScrollStack);
	TestTrue(TEXT("부분 스태킹 성공해야 함"), PartialResult.bSuccess);
	TestEqual(TEXT("결과 상태는 PartiallyMerged여야 함"), PartialResult.ResultType, EDISItemPlacementResult::PartiallyMerged);
	TestEqual(TEXT("병합된 수량은 여유 한도인 3개여야 함"), PartialResult.MergedCount, 3);
	TestEqual(TEXT("남은 잔여 수량은 1개여야 함"), PartialResult.RemainingCount, 1);
	TestEqual(TEXT("슬롯 8의 ScrollStack4는 2+3=5개(최대치)로 채워짐"), Inventory->GetSlotStackCount(8), 5);
	TestEqual(TEXT("PartialScrollStack 객체 자체의 수량도 1개로 감소"), PartialScrollStack->GetStackCount(), 1);
	TestFalse(TEXT("진입 스택은 완전히 비워지지 않고 1개 보존"), PartialScrollStack->IsEmpty());

	// 8. 꽉 찬 스택(5/5)에 추가 드롭 시도 시 거부 검증
	UDISItemStack* RejectScroll = NewObject<UDISItemStack>();
	UDISItemEntry* RejectEntry = NewObject<UDISItemEntry>();
	RejectEntry->InitializeEntry(Scroll2x1Def);
	RejectScroll->PushEntry(RejectEntry);

	TestFalse(TEXT("모든 후보가 꽉 찬 경우 CanAcceptItemStackAtSlot 거부되어야 함"), Inventory->CanAcceptItemStackAtSlot(RejectScroll, 8));
	FDISItemPlacementResult RejectResult = Inventory->SetSlotItemStack(8, RejectScroll);
	TestFalse(TEXT("꽉 찬 스택에 SetSlotItemStack 실패해야 함"), RejectResult.bSuccess);
	TestEqual(TEXT("실패 상태는 Failed여야 함"), RejectResult.ResultType, EDISItemPlacementResult::Failed);

	// 전역 MaxStackCount 복원
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = OriginalMaxStack;

	return true;
}

/**
 * UDISInventory 및 UDISItemStack의 인벤토리 내 좌상단 기준 회전 및 충돌/경계 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISInventoryItemStackRotationTest,
	"DIS.Inventory.ItemStackRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISInventoryItemStackRotationTest::RunTest(const FString& Parameters)
{
	// 4x4 그리드 인벤토리 정의 (총 16슬롯)
	UDISInventoryDefinition* Def = NewObject<UDISInventoryDefinition>();
	Def->DefaultCapacity = 16;
	Def->LineBreakTileCount = 4;
	Def->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	UDISInventory* Inventory = NewObject<UDISInventory>();
	Inventory->InitializeInventory(Def);

	// 1. 1x3 가로 아이템 정의 (회전 허용)
	UDISItemDefinition* Def1x3 = NewObject<UDISItemDefinition>();
	Def1x3->bCanRotate = true;
	Def1x3->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0) };

	UDISItemStack* Stack1x3 = NewObject<UDISItemStack>();
	UDISItemEntry* Entry1x3 = NewObject<UDISItemEntry>();
	Entry1x3->InitializeEntry(Def1x3);
	Stack1x3->PushEntry(Entry1x3);

	// 슬롯 0에 배치: 점유 슬롯은 0, 1, 2
	const FDISItemPlacementResult PlaceResult = Inventory->SetSlotItemStack(0, Stack1x3);
	TestTrue(TEXT("1x3 아이템 초기 배치 성공"), PlaceResult.bSuccess);
	TestEqual(TEXT("초기 회전값은 Rotate0"), Stack1x3->GetRotation(), EDISItemRotation::Rotate0);
	TestEqual(TEXT("BaseCoord는 (0,0)"), Inventory->GetItemStackBaseCoord(Stack1x3), FIntPoint(0, 0));
	TestEqual(TEXT("RootSlot은 0"), Inventory->GetItemStackRootSlot(Stack1x3), 0);

	TArray<int32> Occupied = Inventory->GetItemStackOccupiedSlots(Stack1x3);
	TestEqual(TEXT("점유 슬롯 개수는 3개"), Occupied.Num(), 3);
	TestTrue(TEXT("점유 슬롯 0 포함"), Occupied.Contains(0));
	TestTrue(TEXT("점유 슬롯 1 포함"), Occupied.Contains(1));
	TestTrue(TEXT("점유 슬롯 2 포함"), Occupied.Contains(2));

	// 2. 시계방향 회전 실행: 1x3(가로) -> 3x1(세로)
	// 좌상단 (0,0) 고정 -> 점유 슬롯: 0 (0,0), 4 (0,1), 8 (0,2)
	TestTrue(TEXT("시계방향 회전 가능 판별"), Inventory->CanRotateItemStack(Stack1x3, true));
	const bool bRotatedCW = Inventory->RotateItemStack(Stack1x3, true);
	TestTrue(TEXT("시계방향 회전 성공"), bRotatedCW);
	TestEqual(TEXT("회전 각도는 Rotate90"), Stack1x3->GetRotation(), EDISItemRotation::Rotate90);
	TestEqual(TEXT("회전 후 BaseCoord는 여전히 (0,0) 유지"), Inventory->GetItemStackBaseCoord(Stack1x3), FIntPoint(0, 0));
	TestEqual(TEXT("RootSlot은 여전히 0 유지"), Inventory->GetItemStackRootSlot(Stack1x3), 0);

	// 이전 슬롯 1, 2는 비워져야 하고, 신규 슬롯 4, 8이 점유되어야 함
	TestTrue(TEXT("슬롯 1은 비워져야 함"), Inventory->IsSlotEmpty(1));
	TestTrue(TEXT("슬롯 2는 비워져야 함"), Inventory->IsSlotEmpty(2));
	TestEqual(TEXT("슬롯 0은 Stack1x3 참조"), Inventory->GetSlotItemStack(0), Stack1x3);
	TestEqual(TEXT("슬롯 4는 Stack1x3 참조"), Inventory->GetSlotItemStack(4), Stack1x3);
	TestEqual(TEXT("슬롯 8은 Stack1x3 참조"), Inventory->GetSlotItemStack(8), Stack1x3);

	// 3. 반시계방향 회전 실행: 3x1(세로) -> 다시 1x3(가로)
	TestTrue(TEXT("반시계방향 회전 가능 판별"), Inventory->CanRotateItemStack(Stack1x3, false));
	const bool bRotatedCCW = Inventory->RotateItemStack(Stack1x3, false);
	TestTrue(TEXT("반시계방향 회전 성공"), bRotatedCCW);
	TestEqual(TEXT("회전 각도는 Rotate0으로 복귀"), Stack1x3->GetRotation(), EDISItemRotation::Rotate0);
	TestTrue(TEXT("슬롯 4는 비워져야 함"), Inventory->IsSlotEmpty(4));
	TestTrue(TEXT("슬롯 8은 비워져야 함"), Inventory->IsSlotEmpty(8));
	TestEqual(TEXT("슬롯 1은 다시 점유됨"), Inventory->GetSlotItemStack(1), Stack1x3);
	TestEqual(TEXT("슬롯 2는 다시 점유됨"), Inventory->GetSlotItemStack(2), Stack1x3);

	// 4. 충돌로 인한 회전 실패 검증
	// 슬롯 4(좌표 0,1)에 1x1 방해 아이템 배치
	UDISItemDefinition* ObstacleDef = NewObject<UDISItemDefinition>();
	UDISItemStack* ObstacleStack = NewObject<UDISItemStack>();
	UDISItemEntry* ObstacleEntry = NewObject<UDISItemEntry>();
	ObstacleEntry->InitializeEntry(ObstacleDef);
	ObstacleStack->PushEntry(ObstacleEntry);
	Inventory->SetSlotItemStack(4, ObstacleStack);

	// 현재 Stack1x3은 (0,0)-(2,0)을 차지하고 있으며, 시계방향 회전 시 슬롯 4와 충돌해야 함
	TestFalse(TEXT("슬롯 4에 타 아이템이 있으므로 CanRotateItemStack 거부되어야 함"), Inventory->CanRotateItemStack(Stack1x3, true));
	const bool bConflictRotateFail = Inventory->RotateItemStack(Stack1x3, true);
	TestFalse(TEXT("충돌 시 RotateItemStack 실패해야 함"), bConflictRotateFail);
	TestEqual(TEXT("회전 실패 시 각도는 기존 Rotate0 유지"), Stack1x3->GetRotation(), EDISItemRotation::Rotate0);
	TestEqual(TEXT("슬롯 1은 여전히 Stack1x3 유지"), Inventory->GetSlotItemStack(1), Stack1x3);
	TestEqual(TEXT("슬롯 4는 장애물 유지"), Inventory->GetSlotItemStack(4), ObstacleStack);

	// 5. 경계 초과로 인한 회전 실패 검증
	// 장애물 제거 후 Stack1x3을 하단 슬롯 8(좌표 0,2)로 이동 (점유: 8, 9, 10)
	Inventory->ClearSlot(4);
	Inventory->ClearSlot(0); // 기존 1x3 제거
	Inventory->SetSlotItemStack(8, Stack1x3);
	TestEqual(TEXT("슬롯 8 배치 후 BaseCoord (0,2) 확인"), Inventory->GetItemStackBaseCoord(Stack1x3), FIntPoint(0, 2));

	// 4x4 그리드에서 (0,2) 위치에서 3칸 세로로 회전하면 (0,2), (0,3), (0,4)가 되어 Y=4가 경계 밖!
	TestFalse(TEXT("하단 경계 초과로 CanRotateItemStack 거부되어야 함"), Inventory->CanRotateItemStack(Stack1x3, true));
	const bool bOutOfBoundsRotateFail = Inventory->RotateItemStack(Stack1x3, true);
	TestFalse(TEXT("경계 초과 시 RotateItemStack 실패해야 함"), bOutOfBoundsRotateFail);
	TestEqual(TEXT("회전 실패 후 각도는 Rotate0 유지"), Stack1x3->GetRotation(), EDISItemRotation::Rotate0);

	// 6. bCanRotate == false 회전 금지 아이템 검증
	UDISItemDefinition* FixedDef = NewObject<UDISItemDefinition>();
	FixedDef->bCanRotate = false;
	FixedDef->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0) };

	UDISItemStack* FixedStack = NewObject<UDISItemStack>();
	UDISItemEntry* FixedEntry = NewObject<UDISItemEntry>();
	FixedEntry->InitializeEntry(FixedDef);
	FixedStack->PushEntry(FixedEntry);
	Inventory->SetSlotItemStack(0, FixedStack);

	TestFalse(TEXT("bCanRotate가 false이면 CanRotateItemStack 거부"), Inventory->CanRotateItemStack(FixedStack, true));
	TestFalse(TEXT("bCanRotate가 false이면 RotateItemStack 실패"), Inventory->RotateItemStack(FixedStack, true));

	// 7. RotateItemStackAtSlot 편의 래퍼 검증
	// 슬롯 0에 있던 FixedStack 및 슬롯 8에 있던 Stack1x3 정리
	Inventory->ClearSlot(0);
	Inventory->ClearSlot(8);
	const FDISItemPlacementResult MoveResult = Inventory->SetSlotItemStack(1, Stack1x3);
	TestTrue(TEXT("슬롯 1에 Stack1x3 배치 성공"), MoveResult.bSuccess);
	// (1,0)에서 시계방향 회전 시 점유: (1,0) -> 1, (1,1) -> 5, (1,2) -> 9. 경계 내이므로 성공해야 함!
	const bool bSlotWrapperSuccess = Inventory->RotateItemStackAtSlot(1, true);
	TestTrue(TEXT("RotateItemStackAtSlot(1) 회전 성공"), bSlotWrapperSuccess);
	TestEqual(TEXT("회전 후 각도 90도"), Stack1x3->GetRotation(), EDISItemRotation::Rotate90);
	TestEqual(TEXT("슬롯 5는 Stack1x3 참조"), Inventory->GetSlotItemStack(5), Stack1x3);

	// 빈 슬롯에 대한 RotateItemStackAtSlot 호출 시 실패
	TestFalse(TEXT("빈 슬롯(15) 회전 호출은 실패해야 함"), Inventory->RotateItemStackAtSlot(15, true));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

