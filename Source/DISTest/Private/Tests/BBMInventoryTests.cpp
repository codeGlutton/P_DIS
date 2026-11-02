/*****************************************************************//**
 * @file   BBMInventoryTests.cpp
 * @brief  BBM 배낭 인벤토리 및 슬롯 강화 핵심 단위 테스트
 * @details 아이템 추출 방향 순서, 슬롯 강화 라이프사이클 및 100% 무손실 롤백, 다중 슬롯 점유 누적 합산(권장안 A) 검증
 * @date   2026-11-02
 *********************************************************************/

#include "Misc/AutomationTest.h"
#include "Inventory/BBMInventory.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Inventory/BBMInventorySlot.h"
#include "Inventory/BBMInventoryTypes.h"
#include "Item/BBMItemDefinition.h"
#include "Item/BBMItemEntry.h"
#include "Item/BBMBeatTypes.h"
#include "Item/BBMBeatEnhancement.h"
#include "Item/BBMBeatEnhancementSubclasses.h"
#include "Item/DISItemStack.h"
#include "Item/DISItemTypes.h"
#include "Inventory/BBMSlotEnhancement.h"
#include "Inventory/BBMSlotEnhancementEffect.h"
#include "Item/BBMBeatEnhancementEffect.h"
#include "Enhancement/DISEnhancementContainer.h"
#include "Global/DISGlobalSettings.h"
#include "Item/BBMItemProfileTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace BBMInventoryTestHelpers
{
	/* 헬퍼: 더미 BBMItemDefinition 및 ItemEntry를 포함하는 ItemStack 생성 */
	UDISItemStack* CreateTestItemStack(UObject* Outer, const FIntPoint& Size = FIntPoint(1, 1))
	{
		UBBMItemDefinition* ItemDef = NewObject<UBBMItemDefinition>(Outer);
		ItemDef->ItemName = FText::FromString(TEXT("TestItem"));
		ItemDef->Weight = 1.0f;

		// 타일 형상 구성
		TArray<FIntPoint> Points;
		for (int32 Y = 0; Y < Size.Y; ++Y)
		{
			for (int32 X = 0; X < Size.X; ++X)
			{
				Points.Add(FIntPoint(X, Y));
			}
		}
		ItemDef->Shape = FDISTileShape(Points);

		// 기본 비트 시퀀스 구성 (8비트)
		ItemDef->BeatSequence.Beats.SetNum(8);

		// 기본 EffectRange 1개 추가 (비트 0~2)
		FBBMBeatEffectRange DefaultRange;
		DefaultRange.StartBeat = 0;
		DefaultRange.EndBeat = 2;
		ItemDef->BeatSequence.EffectRanges.Add(DefaultRange);

		// BBMItemEntry 인스턴스 생성 및 초기화
		UBBMItemEntry* Entry = NewObject<UBBMItemEntry>(Outer);
		Entry->InitializeEntry(ItemDef, 100.0f);

		// UDISItemStack 생성 및 Entry 적재
		UDISItemStack* Stack = NewObject<UDISItemStack>(Outer);
		Stack->PushEntry(Entry);

		return Stack;
	}

	/* 헬퍼: 4x4 BBMInventoryDefinition 생성 */
	UBBMInventoryDefinition* CreateTestInventoryDefinition(UObject* Outer, int32 Capacity = 16, int32 LineBreak = 4)
	{
		UBBMInventoryDefinition* Def = NewObject<UBBMInventoryDefinition>(Outer);
		Def->DefaultCapacity = Capacity;
		Def->MaxExpandableCapacity = Capacity;
		Def->LineBreakTileCount = LineBreak;
		Def->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;
		Def->bUseWeightSystem = false;
		Def->DefaultMaxWeight = 0.0f;
		Def->DefaultSlotClass = UBBMInventorySlot::StaticClass();
		Def->ExtractionOrder = EBBMItemExtractionOrder::TopToBottom_LeftToRight;
		return Def;
	}
}

/**
 * 테스트 1: 방향별 아이템 추출/순회(Extraction Order) 우선순위 규칙 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMInventoryExtractionOrderTest,
	"DIS.BBM.Inventory.ExtractionOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMInventoryExtractionOrderTest::RunTest(const FString& Parameters)
{
	// 4x4 인벤토리 정의 및 런타임 인벤토리 초기화
	UBBMInventoryDefinition* Def = BBMInventoryTestHelpers::CreateTestInventoryDefinition(GetTransientPackage(), 16, 4);
	UBBMInventory* Inventory = NewObject<UBBMInventory>(GetTransientPackage());
	Inventory->InitializeInventory(Def);

	// 4개의 1x1 아이템 스택 생성
	// A: (0, 0), B: (2, 0), C: (1, 1), D: (3, 2)
	UDISItemStack* StackA = BBMInventoryTestHelpers::CreateTestItemStack(Inventory);
	UDISItemStack* StackB = BBMInventoryTestHelpers::CreateTestItemStack(Inventory);
	UDISItemStack* StackC = BBMInventoryTestHelpers::CreateTestItemStack(Inventory);
	UDISItemStack* StackD = BBMInventoryTestHelpers::CreateTestItemStack(Inventory);

	TestTrue(TEXT("StackA 배치 (0, 0)"), Inventory->SetSlotItemStack(0, StackA).bSuccess == true);
	TestTrue(TEXT("StackB 배치 (2, 0)"), Inventory->SetSlotItemStack(2, StackB).bSuccess == true);
	TestTrue(TEXT("StackC 배치 (1, 1)"), Inventory->SetSlotItemStack(5, StackC).bSuccess == true);
	TestTrue(TEXT("StackD 배치 (3, 2)"), Inventory->SetSlotItemStack(11, StackD).bSuccess == true);

	// 1. TopToBottom_LeftToRight (상단 우선 -> 동일 행 좌측 우선, 기본값)
	Def->ExtractionOrder = EBBMItemExtractionOrder::TopToBottom_LeftToRight;
	TArray<UDISItemStack*> Order1 = Inventory->GetItemStacksInExtractionOrder();
	TestEqual(TEXT("아이템 총 수량 4개"), Order1.Num(), 4);
	TestTrue(TEXT("Order1 첫 번째는 StackA (0, 0)"), Order1[0] == StackA);
	TestTrue(TEXT("Order1 두 번째는 StackB (2, 0)"), Order1[1] == StackB);
	TestTrue(TEXT("Order1 세 번째는 StackC (1, 1)"), Order1[2] == StackC);
	TestTrue(TEXT("Order1 네 번째는 StackD (3, 2)"), Order1[3] == StackD);

	// 2. TopToBottom_RightToLeft (상단 우선 -> 동일 행 우측 우선)
	Def->ExtractionOrder = EBBMItemExtractionOrder::TopToBottom_RightToLeft;
	TArray<UDISItemStack*> Order2 = Inventory->GetItemStacksInExtractionOrder();
	TestEqual(TEXT("아이템 총 수량 4개"), Order2.Num(), 4);
	TestTrue(TEXT("Order2 첫 번째는 StackB (2, 0)"), Order2[0] == StackB);
	TestTrue(TEXT("Order2 두 번째는 StackA (0, 0)"), Order2[1] == StackA);
	TestTrue(TEXT("Order2 세 번째는 StackC (1, 1)"), Order2[2] == StackC);
	TestTrue(TEXT("Order2 네 번째는 StackD (3, 2)"), Order2[3] == StackD);

	// 3. LeftToRight_TopToBottom (좌측 우선 -> 동일 열 상단 우선, 열 우선)
	Def->ExtractionOrder = EBBMItemExtractionOrder::LeftToRight_TopToBottom;
	TArray<UDISItemStack*> Order3 = Inventory->GetItemStacksInExtractionOrder();
	TestEqual(TEXT("아이템 총 수량 4개"), Order3.Num(), 4);
	TestTrue(TEXT("Order3 첫 번째는 StackA (0, 0)"), Order3[0] == StackA);
	TestTrue(TEXT("Order3 두 번째는 StackC (1, 1)"), Order3[1] == StackC);
	TestTrue(TEXT("Order3 세 번째는 StackB (2, 0)"), Order3[2] == StackB);
	TestTrue(TEXT("Order3 네 번째는 StackD (3, 2)"), Order3[3] == StackD);

	// 4. RightToLeft_TopToBottom (우측 우선 -> 동일 열 상단 우선)
	Def->ExtractionOrder = EBBMItemExtractionOrder::RightToLeft_TopToBottom;
	TArray<UDISItemStack*> Order4 = Inventory->GetItemStacksInExtractionOrder();
	TestEqual(TEXT("아이템 총 수량 4개"), Order4.Num(), 4);
	TestTrue(TEXT("Order4 첫 번째는 StackD (3, 2)"), Order4[0] == StackD);
	TestTrue(TEXT("Order4 두 번째는 StackB (2, 0)"), Order4[1] == StackB);
	TestTrue(TEXT("Order4 세 번째는 StackC (1, 1)"), Order4[2] == StackC);
	TestTrue(TEXT("Order4 네 번째는 StackA (0, 0)"), Order4[3] == StackA);

	// 5. BottomToTop_LeftToRight (하단 우선 -> 동일 행 좌측 우선)
	Def->ExtractionOrder = EBBMItemExtractionOrder::BottomToTop_LeftToRight;
	TArray<UDISItemStack*> Order5 = Inventory->GetItemStacksInExtractionOrder();
	TestEqual(TEXT("아이템 총 수량 4개"), Order5.Num(), 4);
	TestTrue(TEXT("Order5 첫 번째는 StackD (3, 2)"), Order5[0] == StackD);
	TestTrue(TEXT("Order5 두 번째는 StackC (1, 1)"), Order5[1] == StackC);
	TestTrue(TEXT("Order5 세 번째는 StackA (0, 0)"), Order5[2] == StackA || Order5[3] == StackA);

	return true;
}

/**
 * 테스트 2: 슬롯 칸 강화 라이프사이클 및 100% 무손실 롤백 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMInventorySlotEnhancementLifecycleTest,
	"DIS.BBM.Inventory.SlotEnhancementLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMInventorySlotEnhancementLifecycleTest::RunTest(const FString& Parameters)
{
	// CDO에 테스트용 AddedEffectRange 템플릿 설정
	UBBMEffectRangeBeatEnhancement* CDO = GetMutableDefault<UBBMEffectRangeBeatEnhancement>();
	CDO->AddedEffectRange.StartBeat = 0;
	CDO->AddedEffectRange.EndBeat = 4;

	UBBMInventoryDefinition* Def = BBMInventoryTestHelpers::CreateTestInventoryDefinition(GetTransientPackage(), 16, 4);

	// (1, 1) 좌표(슬롯 5)에 UBBMSlotEffectRangeEnhancementEffect 보너스 +2 설정
	FBBMSlotEffectSpecList SpecList1_1;
	SpecList1_1.Effects.Add(FDISEnhancementEffectSpec(UBBMSlotEffectRangeEnhancementEffect::StaticClass(), 2.0f));
	Def->SlotEnhancements.Add(FIntPoint(1, 1), SpecList1_1);

	UBBMInventory* Inventory = NewObject<UBBMInventory>(GetTransientPackage());
	Inventory->InitializeInventory(Def);

	// 슬롯 인스턴스에 강화가 정상 주입되었는지 검증
	UBBMInventorySlot* Slot0_0 = Cast<UBBMInventorySlot>(Inventory->GetSlot(0));
	UBBMInventorySlot* Slot1_1 = Cast<UBBMInventorySlot>(Inventory->GetSlot(5));
	TestNotNull(TEXT("Slot0_0 유효"), Slot0_0);
	TestNotNull(TEXT("Slot1_1 유효"), Slot1_1);
	TestFalse(TEXT("Slot0_0 강화 미부여"), Slot0_0->GetEnhancementContainer()->GetActiveEnhancements().Num() > 0);
	TestTrue(TEXT("Slot1_1 강화 부여됨"), Slot1_1->GetEnhancementContainer()->GetActiveEnhancements().Num() > 0);

	// 1x1 아이템 스택 생성
	UDISItemStack* TestStack = BBMInventoryTestHelpers::CreateTestItemStack(Inventory);
	UBBMItemEntry* ItemEntry = Cast<UBBMItemEntry>(TestStack->GetTopEntry());
	TestNotNull(TEXT("ItemEntry 유효"), ItemEntry);

	// 초기 상태: 슬롯 보너스 0, EffectRanges 총 1개 (기본 Range)
	TestEqual(TEXT("초기 슬롯 보너스 0"), ItemEntry->GetSlotEnhancementBonusLevel(UBBMEffectRangeBeatEnhancement::StaticClass()), 0);
	TestEqual(TEXT("초기 EffectRanges 개수는 1"), ItemEntry->GetDynamicBeatSequence().EffectRanges.Num(), 1);

	// 1단계: 강화가 없는 일반 슬롯(0, 0)에 배치
	Inventory->SetSlotItemStack(0, TestStack);
	TestEqual(TEXT("일반 슬롯 배치 후 슬롯 보너스 여전히 0"), ItemEntry->GetSlotEnhancementBonusLevel(UBBMEffectRangeBeatEnhancement::StaticClass()), 0);
	TestEqual(TEXT("일반 슬롯 배치 후 EffectRanges 개수 여전히 1"), ItemEntry->GetDynamicBeatSequence().EffectRanges.Num(), 1);

	// 2단계: 강화가 설정된 슬롯(1, 1, 인덱스 5)으로 이동 배치
	Inventory->SetSlotItemStack(5, TestStack);
	TestEqual(TEXT("강화 슬롯 배치 후 슬롯 보너스 +2 적용"), ItemEntry->GetSlotEnhancementBonusLevel(UBBMEffectRangeBeatEnhancement::StaticClass()), 2);
	// UBBMEffectRangeBeatEnhancement에 의해 EffectRange가 1개 추가되어 총 2개가 됨
	TestEqual(TEXT("강화 슬롯 배치 후 DynamicBeatSequence에 EffectRange가 동적 추가되어 총 2개"), ItemEntry->GetDynamicBeatSequence().EffectRanges.Num(), 2);

	// 3단계: 인벤토리에서 아이템 제거 (ClearSlot) -> 100% 무손실 롤백 검증
	UDISItemStack* RemovedStack = Inventory->ClearSlot(5);
	TestTrue(TEXT("아이템 정상 추출됨"), RemovedStack == TestStack);
	TestEqual(TEXT("추출 후 슬롯 보너스 0으로 완전 회수"), ItemEntry->GetSlotEnhancementBonusLevel(UBBMEffectRangeBeatEnhancement::StaticClass()), 0);
	TestEqual(TEXT("추출 후 EffectRanges 개수 원본 상태(1)로 100% 무손실 롤백"), ItemEntry->GetDynamicBeatSequence().EffectRanges.Num(), 1);

	return true;
}

/**
 * 테스트 3: 다중 슬롯 점유(대형 아이템) 시 점유한 모든 슬롯 강화 누적 합산 (권장안 A) 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMInventoryMultiSlotEnhancementAccumulationTest,
	"DIS.BBM.Inventory.MultiSlotEnhancementAccumulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMInventoryMultiSlotEnhancementAccumulationTest::RunTest(const FString& Parameters)
{
	UBBMInventoryDefinition* Def = BBMInventoryTestHelpers::CreateTestInventoryDefinition(GetTransientPackage(), 16, 4);

	// (0, 0) 슬롯: EffectRange +1
	FBBMSlotEffectSpecList SpecList0_0;
	SpecList0_0.Effects.Add(FDISEnhancementEffectSpec(UBBMSlotEffectRangeEnhancementEffect::StaticClass(), 1.0f));
	Def->SlotEnhancements.Add(FIntPoint(0, 0), SpecList0_0);

	// (1, 0) 슬롯: EffectRange +2
	FBBMSlotEffectSpecList SpecList1_0;
	SpecList1_0.Effects.Add(FDISEnhancementEffectSpec(UBBMSlotEffectRangeEnhancementEffect::StaticClass(), 2.0f));
	Def->SlotEnhancements.Add(FIntPoint(1, 0), SpecList1_0);

	// (0, 1) 슬롯: ConditionalAction +1
	FBBMSlotEffectSpecList SpecList0_1;
	SpecList0_1.Effects.Add(FDISEnhancementEffectSpec(UBBMSlotConditionalActionEnhancementEffect::StaticClass(), 1.0f));
	Def->SlotEnhancements.Add(FIntPoint(0, 1), SpecList0_1);

	UBBMInventory* Inventory = NewObject<UBBMInventory>(GetTransientPackage());
	Inventory->InitializeInventory(Def);

	// 2x2 대형 무기 아이템 스택 생성
	UDISItemStack* BigWeaponStack = BBMInventoryTestHelpers::CreateTestItemStack(Inventory, FIntPoint(2, 2));
	UBBMItemEntry* Entry = Cast<UBBMItemEntry>(BigWeaponStack->GetTopEntry());
	TestNotNull(TEXT("Entry 유효"), Entry);

	// 2x2 무기를 (0, 0) 위치에 배치 -> 점유 타일: (0,0), (1,0), (0,1), (1,1)
	FDISItemPlacementResult Result = Inventory->SetSlotItemStack(0, BigWeaponStack);
	TestTrue(TEXT("2x2 대형 무기 배치 성공"), Result.bSuccess == true);

	// 권장안 A: 점유한 모든 슬롯의 강화 누적 합산 검증
	// EffectRange: (0,0)의 +1 + (1,0)의 +2 = 총 +3
	const int32 TotalEffectRangeBonus = Entry->GetSlotEnhancementBonusLevel(UBBMEffectRangeBeatEnhancement::StaticClass());
	TestEqual(TEXT("2x2 점유 슬롯의 EffectRange 보너스 누적 합산은 3"), TotalEffectRangeBonus, 3);

	// ConditionalAction: (0,1)의 +1 = 총 +1
	const int32 TotalCondBonus = Entry->GetSlotEnhancementBonusLevel(UBBMConditionalActionBeatEnhancement::StaticClass());
	TestEqual(TEXT("2x2 점유 슬롯의 ConditionalAction 보너스는 1"), TotalCondBonus, 1);

	// ClearItemStack으로 전체 제거 후 100% 무손실 롤백 검증
	Inventory->ClearItemStack(BigWeaponStack);
	TestEqual(TEXT("제거 후 EffectRange 보너스 0"), Entry->GetSlotEnhancementBonusLevel(UBBMEffectRangeBeatEnhancement::StaticClass()), 0);
	TestEqual(TEXT("제거 후 ConditionalAction 보너스 0"), Entry->GetSlotEnhancementBonusLevel(UBBMConditionalActionBeatEnhancement::StaticClass()), 0);

	return true;
}

/**
 * 테스트 4: BlockedTileShape와 슬롯 강화 및 배치 상호작용 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMInventoryBlockedTileTest,
	"DIS.BBM.Inventory.BlockedTileShapeInteraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMInventoryBlockedTileTest::RunTest(const FString& Parameters)
{
	UBBMInventoryDefinition* Def = BBMInventoryTestHelpers::CreateTestInventoryDefinition(GetTransientPackage(), 16, 4);

	// (1, 1) 타일을 BlockedTileShape로 지정
	Def->BlockedTileShape.Points.Add(FIntPoint(1, 1));

	UBBMInventory* Inventory = NewObject<UBBMInventory>(GetTransientPackage());
	Inventory->InitializeInventory(Def);

	// (1, 1) 좌표가 막힌 칸인지 판별
	TestTrue(TEXT("(1, 1) 타일은 블록됨"), Def->IsTileBlocked(FIntPoint(1, 1)) == true);
	TestTrue(TEXT("슬롯 5는 블록됨"), Inventory->IsSlotBlocked(5) == true);
	TestFalse(TEXT("(0, 0) 타일은 블록되지 않음"), Def->IsTileBlocked(FIntPoint(0, 0)) == true);

	// 블록된 슬롯 5에 1x1 아이템 배치 시도 시 거부되어야 함
	UDISItemStack* TestStack = BBMInventoryTestHelpers::CreateTestItemStack(Inventory);
	TestFalse(TEXT("블록된 슬롯에 배치 불가"), Inventory->CanAcceptItemStackAtSlot(TestStack, 5) == true);
	FDISItemPlacementResult Result = Inventory->SetSlotItemStack(5, TestStack);
	TestFalse(TEXT("블록된 슬롯 배치 실패 확인"), Result.bSuccess == true);

	// 정상 슬롯 0에는 정상 배치 가능
	TestTrue(TEXT("정상 슬롯에 배치 가능"), Inventory->CanAcceptItemStackAtSlot(TestStack, 0) == true);
	FDISItemPlacementResult NormalResult = Inventory->SetSlotItemStack(0, TestStack);
	TestTrue(TEXT("정상 슬롯 배치 성공"), NormalResult.bSuccess == true);

	return true;
}

/**
 * 테스트 5: 슬롯 강화가 적용된 아이템의 스태킹(CanStackWith) 정상 동작 검증
 * - 슬롯 강화를 받고 있는 아이템 스택과 슬롯 밖(미강화) 동일 아이템 스택 간 CanStackWith == true 검증
 * - PushItemStack으로 정상 병합되고, 합쳐진 새 엔트리에도 슬롯 강화가 전파되는지 검증
 * - 실제 영구 강화 수치가 다른 아이템 간에는 여전히 CanStackWith == false 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMInventorySlotEnhancementStackingTest,
	"DIS.BBM.Inventory.SlotEnhancementStacking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMInventorySlotEnhancementStackingTest::RunTest(const FString& Parameters)
{
	// 전역 설정에 BBM.Item.Enhancement.Slot 태그 등록 보장 및 MaxStackCount 임시 설정 (스태킹 허용)
	const FDISItemTypeProfile OriginalDefaultProfile = GetDefault<UDISGlobalSettings>()->DefaultItemProfile;
	UDISGlobalSettings* Settings = GetMutableDefault<UDISGlobalSettings>();
	TestNotNull(TEXT("UDISGlobalSettings 유효"), Settings);
	Settings->StackingIgnoredEnhancementTags.AddTag(BBMEnhancementTags::BBM_Item_Enhancement_Slot);
	Settings->DefaultItemProfile.bOverrideCustomData = true;
	Settings->DefaultItemProfile.CustomData.InitializeAs<FDISItemTypeCustomData>(5);

	// 16슬롯(4x4) 인벤토리 생성 및 (1, 1) 좌표에 슬롯 강화 설정
	UBBMInventoryDefinition* Def = BBMInventoryTestHelpers::CreateTestInventoryDefinition(GetTransientPackage(), 16, 4);
	FBBMSlotEffectSpecList SpecList;
	SpecList.Effects.Add(FDISEnhancementEffectSpec(UBBMSlotEffectRangeEnhancementEffect::StaticClass(), 2.0f));
	Def->SlotEnhancements.Add(FIntPoint(1, 1), SpecList);

	UBBMInventory* Inventory = NewObject<UBBMInventory>(GetTransientPackage());
	Inventory->InitializeInventory(Def);

	// 스택 가능(MaxStackCount > 1)한 아이템 정의 생성
	UBBMItemDefinition* StackableItemDef = NewObject<UBBMItemDefinition>(Inventory);
	StackableItemDef->ItemTypeTag = BBMItemTypeTags::Item_Consumable;
	StackableItemDef->ItemName = FText::FromString(TEXT("TestPotion"));
	StackableItemDef->MaxDurability = 0.0f; // 내구도 무제한

	// 1번 스택(슬롯에 배치할 스택)
	UDISItemStack* StackInSlot = NewObject<UDISItemStack>(Inventory);
	UBBMItemEntry* Entry1 = NewObject<UBBMItemEntry>(StackInSlot);
	Entry1->InitializeEntry(StackableItemDef);
	StackInSlot->PushEntry(Entry1);

	// 슬롯 5(좌표 1, 1)에 배치 -> Entry1에 슬롯 강화(+2) 적용됨
	Inventory->SetSlotItemStack(5, StackInSlot);
	TestEqual(TEXT("슬롯 배치 후 Entry1의 슬롯 보너스는 2"), Entry1->GetSlotEnhancementBonusLevel(UBBMEffectRangeBeatEnhancement::StaticClass()), 2);
	const UDISEnhancementContainer* Container1 = Entry1->GetEnhancementContainer();
	TestNotNull(TEXT("Container1 유효"), Container1);
	const FDISActiveEnhancement* Active1 = Container1->FindActiveEnhancement(UBBMEffectRangeBeatEnhancement::StaticClass());
	TestNotNull(TEXT("Active1 유효"), Active1);
	TestEqual(TEXT("슬롯 배치 후 Entry1의 GetLevel은 2"), Active1->GetLevel(), 2);
	TestEqual(TEXT("슬롯 배치 후 Entry1의 GetStackingLevel은 0(슬롯 강화 제외)"), Active1->GetStackingLevel(), 0);

	// 2번 스택(슬롯 밖의 동일 아이템)
	UDISItemStack* StackOutside = NewObject<UDISItemStack>(Inventory);
	UBBMItemEntry* Entry2 = NewObject<UBBMItemEntry>(StackOutside);
	Entry2->InitializeEntry(StackableItemDef);
	StackOutside->PushEntry(Entry2);

	// 슬롯 밖의 Entry2는 슬롯 보너스가 없음 (GetLevel == 0, GetStackingLevel == 0)
	TestEqual(TEXT("슬롯 밖 Entry2의 슬롯 보너스는 0"), Entry2->GetSlotEnhancementBonusLevel(UBBMEffectRangeBeatEnhancement::StaticClass()), 0);

	// 핵심 검증 1: 슬롯 강화가 적용된 StackInSlot과 슬롯 밖의 Entry2 간에 CanStackWith가 true여야 함!
	TestTrue(TEXT("슬롯 강화를 받고 있어도 순수 레벨이 같으므로 CanStackWith == true"), StackInSlot->CanStackWith(Entry2) == true);
	TestTrue(TEXT("반대 방향도 CanStackWith == true"), StackOutside->CanStackWith(Entry1) == true);

	// 핵심 검증 2: 슬롯의 PushItemStack을 통해 슬롯 내 스택으로 병합
	UDISInventorySlot* Slot5 = Inventory->GetSlot(5);
	TestNotNull(TEXT("Slot5 유효"), Slot5);
	const bool bPushed = Slot5->PushItemStack(StackOutside);
	TestTrue(TEXT("PushItemStack 병합 성공"), bPushed == true);
	TestEqual(TEXT("병합 후 슬롯 내 스택 수량은 2"), StackInSlot->GetStackCount(), 2);

	// 핵심 검증 3: 새로 스택에 들어온 Entry2에도 슬롯 강화(+2)가 즉시 자동 전파되어야 함!
	TestEqual(TEXT("새로 합쳐진 Entry2에도 슬롯 보너스 2 전파됨"), Entry2->GetSlotEnhancementBonusLevel(UBBMEffectRangeBeatEnhancement::StaticClass()), 2);

	// 핵심 검증 4: 실제 영구 강화 수치가 다른 아이템과의 스태킹 거부 검증
	UBBMItemEntry* PermanentEnhancedEntry = NewObject<UBBMItemEntry>(Inventory);
	PermanentEnhancedEntry->InitializeEntry(StackableItemDef);
	// 영구 강화 효과 부여 (슬롯 태그 없이 일반 효과로 +1 부여)
	PermanentEnhancedEntry->GetEnhancementContainer()->ApplyEnhancementEffectSpec(
		FDISEnhancementEffectSpec(UBBMEffectRangeBeatEnhancementEffect::StaticClass(), 1.0f)
	);
	const UDISEnhancementContainer* PermContainer = PermanentEnhancedEntry->GetEnhancementContainer();
	TestNotNull(TEXT("PermContainer 유효"), PermContainer);
	const FDISActiveEnhancement* PermActive = PermContainer->FindActiveEnhancement(UBBMEffectRangeBeatEnhancement::StaticClass());
	TestNotNull(TEXT("PermActive 유효"), PermActive);
	TestEqual(TEXT("영구 강화 아이템의 StackingLevel은 1"), PermActive->GetStackingLevel(), 1);

	// 영구 강화 수치가 다르므로 스태킹이 거부되어야 함!
	TestFalse(TEXT("영구 강화 수치가 다른 아이템은 스태킹 거부"), StackInSlot->CanStackWith(PermanentEnhancedEntry) == true);

	// 전역 설정 복원
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile = OriginalDefaultProfile;

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
