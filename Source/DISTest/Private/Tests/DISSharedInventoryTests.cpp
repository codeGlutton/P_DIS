#include "Misc/AutomationTest.h"
#include "Inventory/DISSharedInventoryDefinition.h"
#include "Inventory/DISSharedInventory.h"
#include "Inventory/DISSharedInventorySlot.h"
#include "Inventory/DISInventoryDefinition.h"
#include "Inventory/DISInventory.h"
#include "Inventory/DISInventoryFilter.h"
#include "Item/DISItemDefinition.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemStack.h"
#include "Global/DISGlobalSettings.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * UDISSharedInventory 및 UDISSharedInventorySlot의 1D 선형 고정 슬롯, 무충돌 등록, 중복 방지, 스왑 및 필터 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISSharedInventoryCoreTest,
	"DIS.SharedInventory.Core",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISSharedInventoryCoreTest::RunTest(const FString& Parameters)
{
	// 전역 MaxStackCount 임시 확장 (스태킹 테스트용)
	const FDISItemTypeProfile OriginalDefaultProfile = GetDefault<UDISGlobalSettings>()->DefaultItemProfile;
	FDISItemTypeProfile& DefaultProfile = GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile;
	DefaultProfile.bOverrideCustomData = true;
	DefaultProfile.CustomData.InitializeAs<FDISItemTypeCustomData>(5);

	// 1. 초기화 및 고정 4칸 슬롯 생성 검증
	UDISSharedInventoryDefinition* SharedDef = NewObject<UDISSharedInventoryDefinition>();
	SharedDef->DefaultCapacity = 4;

	UDISSharedInventory* SharedInventory = NewObject<UDISSharedInventory>();
	SharedInventory->InitializeInventory(SharedDef);

	TestEqual(TEXT("초기 슬롯 개수는 고정 4개여야 함"), SharedInventory->GetSlotCount(), 4);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UDISSharedInventorySlot* Slot = SharedInventory->GetSlot(Index);
		TestNotNull(TEXT("슬롯 포인터는 유효해야 함"), Slot);
		if (Slot != nullptr)
		{
			TestEqual(TEXT("슬롯 인덱스 정합성 검증"), Slot->GetSlotIndex(), Index);
			TestTrue(TEXT("초기 슬롯은 비어있어야 함"), Slot->IsEmpty());
		}
	}

	// 2. 다중 타일(3x3 대형 무기) 아이템 단일 칸 등록 검증 (물리 충돌 없음)
	UDISItemDefinition* WeaponDef = NewObject<UDISItemDefinition>();
	WeaponDef->Shape.Points.Empty();
	for (int32 X = 0; X < 3; ++X)
	{
		for (int32 Y = 0; Y < 3; ++Y)
		{
			WeaponDef->Shape.Points.Add(FIntPoint(X, Y));
		}
	}

	UDISItemStack* WeaponStack = NewObject<UDISItemStack>();
	UDISItemEntry* WeaponEntry = NewObject<UDISItemEntry>();
	WeaponEntry->InitializeEntry(WeaponDef);
	WeaponStack->PushEntry(WeaponEntry);

	const bool bPlaceWeaponSuccess = SharedInventory->SetSlotItemStack(0, WeaponStack);
	TestTrue(TEXT("3x3 대형 무기도 단일 슬롯 0에 등록 성공해야 함"), bPlaceWeaponSuccess);
	TestEqual(TEXT("슬롯 0은 WeaponStack 참조"), SharedInventory->GetSlotItemStack(0), WeaponStack);
	TestFalse(TEXT("슬롯 0은 비어있지 않음"), SharedInventory->IsSlotEmpty(0));
	TestTrue(TEXT("슬롯 1은 비어있어야 함 (공간 점유 없음)"), SharedInventory->IsSlotEmpty(1));
	TestTrue(TEXT("슬롯 2는 비어있어야 함 (공간 점유 없음)"), SharedInventory->IsSlotEmpty(2));
	TestTrue(TEXT("슬롯 3은 비어있어야 함 (공간 점유 없음)"), SharedInventory->IsSlotEmpty(3));

	// 3. 동일 아이템 중복 등록 방지 (No Duplicates) 검증
	// 슬롯 0에 등록되어 있던 WeaponStack을 슬롯 2에 등록 시도
	const bool bMoveWeaponSuccess = SharedInventory->SetSlotItemStack(2, WeaponStack);
	TestTrue(TEXT("슬롯 2에 WeaponStack 이동 등록 성공"), bMoveWeaponSuccess);
	TestTrue(TEXT("기존 슬롯 0은 자동으로 비워져야 함"), SharedInventory->IsSlotEmpty(0));
	TestNull(TEXT("슬롯 0의 스택 포인터는 nullptr"), SharedInventory->GetSlotItemStack(0));
	TestEqual(TEXT("슬롯 2에만 WeaponStack 존재"), SharedInventory->GetSlotItemStack(2), WeaponStack);
	TestEqual(TEXT("FindSlotWithItemStack 결과는 2"), SharedInventory->FindSlotWithItemStack(WeaponStack), 2);

	// 4. 슬롯 위치 교환 (SwapSlots) 검증
	UDISItemDefinition* PotionDef = NewObject<UDISItemDefinition>();
	PotionDef->Shape.Points = { FIntPoint(0, 0) };

	UDISItemStack* PotionStack = NewObject<UDISItemStack>();
	UDISItemEntry* PotionEntry = NewObject<UDISItemEntry>();
	PotionEntry->InitializeEntry(PotionDef);
	PotionStack->PushEntry(PotionEntry);

	SharedInventory->SetSlotItemStack(0, PotionStack);
	TestEqual(TEXT("슬롯 0에 포션 등록 확인"), SharedInventory->GetSlotItemStack(0), PotionStack);

	// 슬롯 0(Potion) <-> 슬롯 2(Weapon) 스왑 실행
	const bool bSwapSuccess = SharedInventory->SwapSlots(0, 2);
	TestTrue(TEXT("SwapSlots(0, 2) 성공해야 함"), bSwapSuccess);
	TestEqual(TEXT("스왑 후 슬롯 0은 WeaponStack이어야 함"), SharedInventory->GetSlotItemStack(0), WeaponStack);
	TestEqual(TEXT("스왑 후 슬롯 2는 PotionStack이어야 함"), SharedInventory->GetSlotItemStack(2), PotionStack);

	// 동일 슬롯 스왑 시 true 반환
	TestTrue(TEXT("동일 슬롯 SwapSlots(0, 0)은 true"), SharedInventory->SwapSlots(0, 0));

	// 5. 개별 슬롯 필터 검증
	// 슬롯 1에 태그 필터 (Item.Consumable 태그 필수) 추가
	const FGameplayTag ConsumableTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Test.Item.Type.Consumable"));
	PotionDef->ItemTypeTag = ConsumableTag;

	UDISSharedInventorySlot* Slot1 = SharedInventory->GetSlot(1);
	TestNotNull(TEXT("슬롯 1 포인터 유효"), Slot1);
	if (Slot1 != nullptr)
	{
		FDISTagInventoryFilter TagFilter;
		TagFilter.RequiredTags.AddTag(ConsumableTag);
		Slot1->EmplaceFilter<FDISTagInventoryFilter>(TagFilter);

		// 슬롯 1에 무기(태그 없음) 등록 시도 -> 거부되어야 함
		const bool bRejectWeapon = SharedInventory->SetSlotItemStack(1, WeaponStack);
		TestFalse(TEXT("슬롯 1 필터에 맞지 않는 무기는 거부되어야 함"), bRejectWeapon);
		TestTrue(TEXT("슬롯 1은 여전히 비어있어야 함"), SharedInventory->IsSlotEmpty(1));

		// 슬롯 1과 슬롯 0(Weapon) 스왑 시도 -> 거부되어야 함
		const bool bRejectSwap = SharedInventory->SwapSlots(0, 1);
		TestFalse(TEXT("필터 규칙을 위반하는 SwapSlots는 거부되어야 함"), bRejectSwap);
		TestEqual(TEXT("슬롯 0은 여전히 Weapon 유지"), SharedInventory->GetSlotItemStack(0), WeaponStack);
		TestTrue(TEXT("슬롯 1은 여전히 비어있음"), SharedInventory->IsSlotEmpty(1));

		// 슬롯 2의 포션을 슬롯 1로 등록 시도 -> 성공해야 함
		const bool bAcceptPotion = SharedInventory->SetSlotItemStack(1, PotionStack);
		TestTrue(TEXT("슬롯 1 필터를 만족하는 포션은 등록 성공해야 함"), bAcceptPotion);
		TestEqual(TEXT("슬롯 1에 포션 등록 확인"), SharedInventory->GetSlotItemStack(1), PotionStack);
		TestTrue(TEXT("이전 슬롯 2는 자동 해제됨"), SharedInventory->IsSlotEmpty(2));
	}

	// 6. 메인 인벤토리와 스택 포인터 공유 및 동기화 검증
	UDISInventoryDefinition* MainInvDef = NewObject<UDISInventoryDefinition>();
	MainInvDef->DefaultCapacity = 10;
	MainInvDef->LineBreakTileCount = 5;

	UDISInventory* MainInventory = NewObject<UDISInventory>();
	MainInventory->InitializeInventory(MainInvDef);

	// 포션 스택을 메인 인벤토리 슬롯 0에 배치
	MainInventory->SetSlotItemStack(0, PotionStack);
	TestEqual(TEXT("메인 인벤토리 슬롯 0에 포션 스택 배치"), MainInventory->GetSlotItemStack(0), PotionStack);
	TestEqual(TEXT("공유 인벤토리 슬롯 1에도 동일 포션 스택 참조 중"), SharedInventory->GetSlotItemStack(1), PotionStack);
	TestEqual(TEXT("초기 포션 수량은 1개"), SharedInventory->GetSlotStackCount(1), 1);

	// 메인 인벤토리 또는 스택 자체에 포션 엔트리 추가
	UDISItemEntry* ExtraPotion = NewObject<UDISItemEntry>();
	ExtraPotion->InitializeEntry(PotionDef);
	const bool bPushSuccess = PotionStack->PushEntry(ExtraPotion);
	TestTrue(TEXT("추가 포션 엔트리 Push 성공"), bPushSuccess);

	// 양쪽 인벤토리에서 실시간으로 동일하게 2개로 조회되어야 함!
	TestEqual(TEXT("메인 인벤토리 슬롯 0의 수량은 2개"), MainInventory->GetSlotStackCount(0), 2);
	TestEqual(TEXT("공유 인벤토리 슬롯 1의 수량도 2개로 실시간 동기화"), SharedInventory->GetSlotStackCount(1), 2);

	// 7. 인덱스 경계 방어 검증
	TestFalse(TEXT("음수 슬롯 인덱스 등록 실패"), SharedInventory->SetSlotItemStack(-1, WeaponStack));
	TestFalse(TEXT("범위 초과 슬롯 인덱스 등록 실패"), SharedInventory->SetSlotItemStack(4, WeaponStack));
	TestNull(TEXT("범위 초과 슬롯 조회는 nullptr"), SharedInventory->GetSlot(4));
	TestFalse(TEXT("범위 초과 SwapSlots 실패"), SharedInventory->SwapSlots(0, 5));

	// 전역 MaxStackCount 복원
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile = OriginalDefaultProfile;

	return true;
}

/**
 * 인벤토리 간 ItemStack 이동 시 이전 인벤토리 자동 해제, 퀵슬롯 연동 자동 무효화, 부분/완전 스태킹 병합 시 소유권 정합성 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISCrossInventoryOwnershipTest,
	"DIS.Ownership.CrossInventoryTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISCrossInventoryOwnershipTest::RunTest(const FString& Parameters)
{
	const FDISItemTypeProfile OriginalDefaultProfile = GetDefault<UDISGlobalSettings>()->DefaultItemProfile;
	FDISItemTypeProfile& DefaultProfile = GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile;
	DefaultProfile.bOverrideCustomData = true;
	DefaultProfile.CustomData.InitializeAs<FDISItemTypeCustomData>(5);

	// 인벤토리 정의 생성 (4x4, 총 16칸)
	UDISInventoryDefinition* InvDef = NewObject<UDISInventoryDefinition>();
	InvDef->DefaultCapacity = 16;
	InvDef->MaxExpandableCapacity = 16;
	InvDef->LineBreakTileCount = 4;

	UDISInventory* InvA = NewObject<UDISInventory>();
	InvA->InitializeInventory(InvDef);

	UDISInventory* InvB = NewObject<UDISInventory>();
	InvB->InitializeInventory(InvDef);

	// 퀵슬롯 공유 인벤토리 생성 (4칸)
	UDISSharedInventoryDefinition* SharedDef = NewObject<UDISSharedInventoryDefinition>();
	SharedDef->DefaultCapacity = 4;

	UDISSharedInventory* QuickSlotInv = NewObject<UDISSharedInventory>();
	QuickSlotInv->InitializeInventory(SharedDef);

	// 2x2 대형 아이템 정의 및 스택 생성
	UDISItemDefinition* BigItemDef = NewObject<UDISItemDefinition>();
	BigItemDef->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(1, 1) };
	BigItemDef->bCanRotate = true;

	UDISItemEntry* BigEntry = NewObject<UDISItemEntry>();
	BigEntry->InitializeEntry(BigItemDef);

	UDISItemStack* BigStack = NewObject<UDISItemStack>();
	BigStack->PushEntry(BigEntry);

	// 1. InvA 슬롯 0에 2x2 아이템 배치
	FDISItemPlacementResult PlaceResultA = InvA->SetSlotItemStack(0, BigStack);
	TestTrue(TEXT("InvA에 BigStack 배치 성공"), PlaceResultA.bSuccess);
	TestEqual(TEXT("BigStack의 OwningInventory는 InvA"), BigStack->GetOwningInventory(), InvA);
	TestEqual(TEXT("InvA 슬롯 0 점유 확인"), InvA->GetSlotItemStack(0), BigStack);
	TestEqual(TEXT("InvA 슬롯 1 점유 확인"), InvA->GetSlotItemStack(1), BigStack);
	TestEqual(TEXT("InvA 슬롯 4 점유 확인"), InvA->GetSlotItemStack(4), BigStack);
	TestEqual(TEXT("InvA 슬롯 5 점유 확인"), InvA->GetSlotItemStack(5), BigStack);

	// 2. 퀵슬롯 0번에 BigStack 등록
	const bool bQuickSlotSet = QuickSlotInv->SetSlotItemStack(0, BigStack);
	TestTrue(TEXT("퀵슬롯 0에 BigStack 등록 성공"), bQuickSlotSet);
	TestEqual(TEXT("퀵슬롯 0에서 BigStack 참조"), QuickSlotInv->GetSlotItemStack(0), BigStack);

	// 3. InvA 내부에서 다른 슬롯(슬롯 10)으로 위치 이동 시 퀵슬롯 유지 검증
	// (슬롯 10은 (2, 2) 위치이므로 2x2 아이템 배치 가능)
	FDISItemPlacementResult MoveInternalResult = InvA->SetSlotItemStack(10, BigStack);
	TestTrue(TEXT("InvA 내부 재배치 성공"), MoveInternalResult.bSuccess);
	TestEqual(TEXT("내부 이동 후에도 OwningInventory는 여전히 InvA"), BigStack->GetOwningInventory(), InvA);
	TestEqual(TEXT("내부 이동 시 퀵슬롯 0의 등록 상태 유지"), QuickSlotInv->GetSlotItemStack(0), BigStack);

	// 4. InvA의 BigStack을 InvB 슬롯 0으로 이동 배치
	FDISItemPlacementResult PlaceResultB = InvB->SetSlotItemStack(0, BigStack);
	TestTrue(TEXT("InvB에 BigStack 배치 성공"), PlaceResultB.bSuccess);
	TestEqual(TEXT("BigStack의 OwningInventory는 InvB로 갱신"), BigStack->GetOwningInventory(), InvB);

	// 원본 InvA의 모든 점유 슬롯에서 자동 해제되었는지 검증
	TestNull(TEXT("이전 인벤토리 InvA 슬롯 10 자동 해제"), InvA->GetSlotItemStack(10));
	TestNull(TEXT("이전 인벤토리 InvA 슬롯 11 자동 해제"), InvA->GetSlotItemStack(11));
	TestNull(TEXT("이전 인벤토리 InvA 슬롯 14 자동 해제"), InvA->GetSlotItemStack(14));
	TestNull(TEXT("이전 인벤토리 InvA 슬롯 15 자동 해제"), InvA->GetSlotItemStack(15));

	// 신규 인벤토리 InvB 슬롯 점유 검증
	TestEqual(TEXT("InvB 슬롯 0 점유 확인"), InvB->GetSlotItemStack(0), BigStack);
	TestEqual(TEXT("InvB 슬롯 1 점유 확인"), InvB->GetSlotItemStack(1), BigStack);
	TestEqual(TEXT("InvB 슬롯 4 점유 확인"), InvB->GetSlotItemStack(4), BigStack);
	TestEqual(TEXT("InvB 슬롯 5 점유 확인"), InvB->GetSlotItemStack(5), BigStack);

	// 타 인벤토리로 이전되었으므로 퀵슬롯이 옵저버에 의해 자동으로 Clear되었는지 검증
	TestTrue(TEXT("타 인벤토리 이전 시 퀵슬롯 0 자동 ClearSlot 검증"), QuickSlotInv->IsSlotEmpty(0));
	TestNull(TEXT("퀵슬롯 0의 ItemStack은 nullptr"), QuickSlotInv->GetSlotItemStack(0));

	// 5. ClearItemStack 시 퀵슬롯 자동 무효화 검증
	UDISItemDefinition* PotionDef = NewObject<UDISItemDefinition>();
	PotionDef->Shape.Points = { FIntPoint(0, 0) };

	UDISItemEntry* PotionEntry = NewObject<UDISItemEntry>();
	PotionEntry->InitializeEntry(PotionDef);

	UDISItemStack* PotionStack = NewObject<UDISItemStack>();
	PotionStack->PushEntry(PotionEntry);

	InvA->SetSlotItemStack(0, PotionStack);
	TestEqual(TEXT("PotionStack 소유 인벤토리는 InvA"), PotionStack->GetOwningInventory(), InvA);

	QuickSlotInv->SetSlotItemStack(1, PotionStack);
	TestEqual(TEXT("퀵슬롯 1에 PotionStack 등록"), QuickSlotInv->GetSlotItemStack(1), PotionStack);

	const bool bCleared = InvA->ClearItemStack(PotionStack);
	TestTrue(TEXT("InvA->ClearItemStack 성공"), bCleared);
	TestNull(TEXT("Clear 후 PotionStack의 소유 인벤토리는 nullptr"), PotionStack->GetOwningInventory());
	TestNull(TEXT("InvA 슬롯 0은 비워짐"), InvA->GetSlotItemStack(0));
	TestTrue(TEXT("ClearItemStack 시 퀵슬롯 1 자동 해제"), QuickSlotInv->IsSlotEmpty(1));

	// 6. 스태킹 부분 병합(PartiallyMerged) 시 이전 인벤토리 스택 보존 검증
	// SourceStack: 수량 3개 (InvA 슬롯 2)
	UDISItemStack* SourceStack = NewObject<UDISItemStack>();
	for (int32 i = 0; i < 3; ++i)
	{
		UDISItemEntry* Entry = NewObject<UDISItemEntry>();
		Entry->InitializeEntry(PotionDef);
		SourceStack->PushEntry(Entry);
	}
	InvA->SetSlotItemStack(2, SourceStack);

	// TargetStack: 수량 4개 (InvB 슬롯 2, 최대 수량 5개이므로 1개만 수용 가능)
	UDISItemStack* TargetStack = NewObject<UDISItemStack>();
	for (int32 i = 0; i < 4; ++i)
	{
		UDISItemEntry* Entry = NewObject<UDISItemEntry>();
		Entry->InitializeEntry(PotionDef);
		TargetStack->PushEntry(Entry);
	}
	InvB->SetSlotItemStack(2, TargetStack);

	FDISItemPlacementResult PartialResult = InvB->SetSlotItemStack(2, SourceStack);
	TestTrue(TEXT("부분 병합 성공"), PartialResult.bSuccess);
	TestEqual(TEXT("결과 타입은 PartiallyMerged"), PartialResult.ResultType, EDISItemPlacementResult::PartiallyMerged);
	TestEqual(TEXT("병합된 수량은 1개"), PartialResult.MergedCount, 1);
	TestEqual(TEXT("TargetStack 수량은 최대 5개"), TargetStack->GetStackCount(), 5);
	TestEqual(TEXT("SourceStack 잔여 수량은 2개"), SourceStack->GetStackCount(), 2);
	TestEqual(TEXT("부분 병합 후 SourceStack 소유 인벤토리는 여전히 InvA"), SourceStack->GetOwningInventory(), InvA);
	TestEqual(TEXT("InvA 슬롯 2에 여전히 SourceStack 잔존"), InvA->GetSlotItemStack(2), SourceStack);

	// 7. 스태킹 완전 병합(FullyMerged) 시 이전 인벤토리 스택 자동 제거 검증
	// TargetStack2: 수량 2개 (InvB 슬롯 3, 여유 3개)
	UDISItemStack* TargetStack2 = NewObject<UDISItemStack>();
	for (int32 i = 0; i < 2; ++i)
	{
		UDISItemEntry* Entry = NewObject<UDISItemEntry>();
		Entry->InitializeEntry(PotionDef);
		TargetStack2->PushEntry(Entry);
	}
	InvB->SetSlotItemStack(3, TargetStack2);

	// InvA 슬롯 2의 SourceStack(잔여 2개)을 InvB 슬롯 3에 병합
	FDISItemPlacementResult FullResult = InvB->SetSlotItemStack(3, SourceStack);
	TestTrue(TEXT("완전 병합 성공"), FullResult.bSuccess);
	TestEqual(TEXT("결과 타입은 FullyMerged"), FullResult.ResultType, EDISItemPlacementResult::FullyMerged);
	TestEqual(TEXT("병합된 수량은 2개"), FullResult.MergedCount, 2);
	TestEqual(TEXT("TargetStack2 수량은 4개"), TargetStack2->GetStackCount(), 4);
	TestTrue(TEXT("SourceStack은 완전히 비워짐"), SourceStack->IsEmpty());
	TestNull(TEXT("완전 병합 시 InvA 슬롯 2에서 완전히 제거됨"), InvA->GetSlotItemStack(2));

	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile = OriginalDefaultProfile;
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
