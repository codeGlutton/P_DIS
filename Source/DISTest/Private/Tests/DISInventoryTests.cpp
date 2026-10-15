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
	Inventory->SetSlotItemEntry(5, Entry); // 마지막 슬롯(인덱스 5)에 아이템 배치

	const bool bSafeRemoveFail = Inventory->RemoveSlots(1);
	TestFalse(TEXT("아이템이 있는 슬롯 축소는 안전 모드에서 실패해야 함"), bSafeRemoveFail);
	TestEqual(TEXT("아이템 보호로 슬롯 수는 여전히 6이어야 함"), Inventory->GetSlotCount(), 6);

	TArray<UDISItemEntry*> EvictedItems;
	const bool bForceRemoveSuccess = Inventory->ForceRemoveSlots(1, EvictedItems);
	TestTrue(TEXT("ForceRemoveSlots 시에는 강제 축소 성공"), bForceRemoveSuccess);
	TestEqual(TEXT("강제 축소 후 슬롯 수는 5여야 함"), Inventory->GetSlotCount(), 5);
	TestEqual(TEXT("퇴출된 아이템 개수는 1개여야 함"), EvictedItems.Num(), 1);
	if (EvictedItems.Num() > 0)
	{
		TestEqual(TEXT("퇴출된 아이템 엔트리 일치 검증"), EvictedItems[0], Entry);
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
	CornerDef->LineBreakTileCount = 4; // 1줄당 4칸 (가로 우선)
	CornerDef->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	UDISInventory* CornerInventory = NewObject<UDISInventory>();
	CornerInventory->InitializeInventory(CornerDef); // 4칸 (0, 1, 2, 3) -> 1행 4열: 0, 3번은 모서리, 1, 2번은 가장자리

	UDISItemDefinition* CornerItemDef = NewObject<UDISItemDefinition>();
	FDISCornerPlacementRule CornerRule;
	CornerItemDef->DefaultPlacementRules.Add(TInstancedStruct<FDISItemPlacementRule>::Make(CornerRule));

	UDISItemEntry* CornerItemEntry = NewObject<UDISItemEntry>();
	CornerItemEntry->InitializeEntry(CornerItemDef);

	// 3번 슬롯(초기 4칸 기준 맨 우측 끝 모서리)에 배치
	TestTrue(TEXT("초기 4칸 인벤토리에서 3번 슬롯은 모서리이므로 배치 성공해야 함"), CornerInventory->SetSlotItemEntry(3, CornerItemEntry));

	// 4칸에서 5칸으로 확장 시: 5칸 / 줄바꿈 4 = 2행 그리드가 되어 3번 슬롯은 (3,0)으로 더 이상 우측 하단 모서리가 아님!
	// 안전 모드 ResizeSlots는 규칙 불일치 아이템이 발생하므로 실패해야 함
	const bool bSafeExpandFail = CornerInventory->AddSlots(1);
	TestFalse(TEXT("PlacementRule 불일치 아이템 발생 시 안전 모드 확장은 실패해야 함"), bSafeExpandFail);
	TestEqual(TEXT("실패 후 슬롯 수는 원래대로 4 유지"), CornerInventory->GetSlotCount(), 4);

	// ForceAddSlots 시에는 슬롯이 확장되고, 모서리 요건을 상실한 3번 슬롯의 아이템이 자동으로 퇴출되어야 함!
	TArray<UDISItemEntry*> RuleEvictedItems;
	const bool bForceExpandSuccess = CornerInventory->ForceAddSlots(1, RuleEvictedItems);
	TestTrue(TEXT("ForceAddSlots는 강제 확장 성공"), bForceExpandSuccess);
	TestEqual(TEXT("확장 후 총 슬롯 수는 5여야 함"), CornerInventory->GetSlotCount(), 5);
	TestEqual(TEXT("PlacementRule 불일치로 퇴출된 아이템 개수는 1개여야 함"), RuleEvictedItems.Num(), 1);
	if (RuleEvictedItems.Num() > 0)
	{
		TestEqual(TEXT("퇴출된 아이템 엔트리 검증"), RuleEvictedItems[0], CornerItemEntry);
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
		Slot0->SetItemEntry(ItemEntryA); // 10.0f
		Slot1->SetItemEntry(ItemEntryB); // 10.0f -> 총 20.0f
	}

	TestEqual(TEXT("현재 총 적재 무게는 20.0f여야 함"), Inventory->GetCurrentWeight(), 20.0f);

	// 3. 무게 초과 아이템 진입 거부 검증 (현재 20.0f, 남은 용량 30.0f -> 무게 35.0f 아이템 거부)
	UDISItemDefinition* HeavyItemDef = NewObject<UDISItemDefinition>();
	HeavyItemDef->Weight = 35.0f;
	UDISItemEntry* HeavyEntry = NewObject<UDISItemEntry>();
	HeavyEntry->InitializeEntry(HeavyItemDef);

	TestFalse(TEXT("최대 적재 무게를 초과하는 아이템은 CanAcceptItem에서 거부되어야 함"), Inventory->CanAcceptItem(HeavyEntry));

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

	if (WeaponTag.IsValid() == true)
	{
		TestFalse(TEXT("Weapon 태그를 가진 아이템은 인벤토리 진입이 거부되어야 함"), Inventory->CanAcceptItem(WeaponEntry));
		TestTrue(TEXT("Potion 태그를 가진 아이템은 인벤토리 진입이 허용되어야 함"), Inventory->CanAcceptItem(PotionEntry));
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
			TestTrue(TEXT("포션 필터가 설정된 슬롯은 포션을 수용해야 함"), Slot0->CanAcceptItem(PotionEntry));
			TestFalse(TEXT("포션 필터가 설정된 슬롯은 무기를 수용하지 않아야 함"), Slot0->CanAcceptItem(WeaponEntry));
		}

		// 슬롯 필터 제거 후 재검증
		Slot0->ClearFilters();
		TestTrue(TEXT("필터가 제거된 슬롯은 포션을 수용해야 함"), Slot0->CanAcceptItem(PotionEntry));
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

	TestFalse(TEXT("블록된 6번 슬롯에는 아이템을 배치할 수 없어야 함"), Inventory->CanAcceptItemAtSlot(ItemEntry, 6));
	TestTrue(TEXT("정상인 0번 슬롯에는 아이템을 배치할 수 있어야 함"), Inventory->CanAcceptItemAtSlot(ItemEntry, 0));

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

	if (WeaponTag.IsValid() == true)
	{
		TestFalse(TEXT("정적 기본 필터에 의해 무기 아이템은 인벤토리 진입이 거부되어야 함"), Inventory->CanAcceptItem(WeaponEntry));
		TestTrue(TEXT("포션 아이템은 인벤토리 진입이 허용되어야 함"), Inventory->CanAcceptItem(PotionEntry));
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

	// 규칙 복사 검증
	TestEqual(TEXT("아이템 엔트리에 기본 배치 규칙 1개가 복사되어야 함"), CornerItemEntry->GetPlacementRules().Num(), 1);

	// 모서리 슬롯(0번)에는 배치 성공, 중앙 슬롯(5번)에는 배치 실패 검증
	TestTrue(TEXT("모서리 슬롯(0번)에는 Corner 규칙 아이템이 배치 가능해야 함"), Inventory->CanAcceptItemAtSlot(CornerItemEntry, 0));
	TestFalse(TEXT("중앙 슬롯(5번)에는 Corner 규칙 아이템이 배치 불가해야 함"), Inventory->CanAcceptItemAtSlot(CornerItemEntry, 5));

	// 2. CenterPlacementRule이 지정된 아이템 정의 생성
	UDISItemDefinition* CenterItemDef = NewObject<UDISItemDefinition>();
	CenterItemDef->DefaultPlacementRules.Add(TInstancedStruct<FDISItemPlacementRule>::Make<FDISCenterPlacementRule>());

	UDISItemEntry* CenterItemEntry = NewObject<UDISItemEntry>();
	CenterItemEntry->InitializeEntry(CenterItemDef);

	// 모서리 슬롯(0번)에는 배치 실패, 중앙 슬롯(5번)에는 배치 성공 검증
	TestFalse(TEXT("모서리 슬롯(0번)에는 Center 규칙 아이템이 배치 불가해야 함"), Inventory->CanAcceptItemAtSlot(CenterItemEntry, 0));
	TestTrue(TEXT("중앙 슬롯(5번)에는 Center 규칙 아이템이 배치 가능해야 함"), Inventory->CanAcceptItemAtSlot(CenterItemEntry, 5));

	// 3. 런타임에 배치 규칙 제거(ClearPlacementRules) 후 재검증
	CornerItemEntry->ClearPlacementRules();
	TestEqual(TEXT("규칙 제거 후 규칙 개수는 0개여야 함"), CornerItemEntry->GetPlacementRules().Num(), 0);
	TestTrue(TEXT("규칙이 제거된 후에는 중앙 슬롯(5번)에도 정상 배치 가능해야 함"), Inventory->CanAcceptItemAtSlot(CornerItemEntry, 5));

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
	TestEqual(TEXT("SlotA와 SlotB가 가리키는 최상단 엔트리 동일 검증"), SlotA->GetItemEntry(), SlotB->GetItemEntry());
	TestEqual(TEXT("SlotA와 SlotB의 스택 수량 동일 검증"), SlotA->GetStackCount(), SlotB->GetStackCount());

	// 3. 한쪽 슬롯에서 스택 엔트리 꺼내기(소비) 수행
	UDISItemEntry* Popped = SlotA->PopItemEntry();
	TestEqual(TEXT("꺼낸 엔트리 일치 검증"), Popped, Entry);

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

	// 1. SetItemEntry로 첫 번째 아이템 장착 -> Container의 EffectTargets에 Entry1 등록 검증
	Slot->SetItemEntry(Entry1);
	TestEqual(TEXT("Entry1 장착 후 EffectTargets 개수 1 검증"), Container->GetEffectTargets().Num(), 1);
	TestTrue(TEXT("Entry1이 EffectTargets에 포함되어 있어야 함"), Container->GetEffectTargets().Contains(Entry1));

	// 2. 임시로 MaxStackCount를 5로 늘리고 PushItemEntry로 두 번째 아이템 스택 추가 -> Entry2도 등록 검증
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile.MaxStackCount = 5;
	Slot->PushItemEntry(Entry2);
	TestEqual(TEXT("Entry2 추가 후 EffectTargets 개수 2 검증"), Container->GetEffectTargets().Num(), 2);
	TestTrue(TEXT("Entry1과 Entry2 모두 EffectTargets에 포함되어 있어야 함"), Container->GetEffectTargets().Contains(Entry1) && Container->GetEffectTargets().Contains(Entry2));

	// 3. PopItemEntry로 1개 꺼냄 -> 꺼내진 엔트리가 EffectTargets에서 제거되었는지 검증
	UDISItemEntry* Popped = Slot->PopItemEntry();
	TestNotNull(TEXT("꺼낸 아이템 유효성 검증"), Popped);
	TestEqual(TEXT("1개 꺼낸 후 EffectTargets 개수 1 검증"), Container->GetEffectTargets().Num(), 1);
	TestFalse(TEXT("꺼내진 엔트리는 EffectTargets에서 제거되어야 함"), Container->GetEffectTargets().Contains(Popped));

	// 4. ClearSlot으로 슬롯 비우기 -> EffectTargets가 완전히 비워지는지 검증
	Slot->ClearSlot();
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

#endif // WITH_DEV_AUTOMATION_TESTS

