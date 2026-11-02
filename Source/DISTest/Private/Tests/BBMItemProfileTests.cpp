#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "GameplayTagsManager.h"
#include "Global/DISGlobalSettings.h"
#include "Global/BBMGlobalDefinition.h"
#include "Global/BBMGlobalLogic.h"
#include "Inventory/DISInventory.h"
#include "Inventory/DISInventoryDefinition.h"
#include "Item/BBMItemDefinition.h"
#include "Item/BBMItemEntry.h"
#include "Item/BBMItemProfileTypes.h"
#include "Item/BBMItemGameplayEffectTypes.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "Item/BBMEnhancementTypes.h"
#include "Item/BBMBeatEnhancement.h"
#include "Item/BBMBeatEnhancementSubclasses.h"
#include "Curves/CurveFloat.h"
#include "Enhancement/DISEnhancementContainer.h"
#include "Item/DISItemDefinition.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemStack.h"
#include "Animation/AnimInstance.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS

// UDISItemDefinition 및 UBBMItemDefinition의 MaxDurability 기본값(0.0f, 내구도 무제한) 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMItemDefaultDurabilityTest,
	"DIS.BBM.ItemDefaultDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMItemDefaultDurabilityTest::RunTest(const FString& Parameters)
{
	UDISItemDefinition* BaseItemDef = NewObject<UDISItemDefinition>();
	TestNotNull(TEXT("UDISItemDefinition 인스턴스 생성 검증"), BaseItemDef);
	if (BaseItemDef == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("UDISItemDefinition의 MaxDurability 초기값은 0.0f(내구도 무제한)여야 함"), BaseItemDef->MaxDurability, 0.0f);

	UBBMItemDefinition* BBMItemDef = NewObject<UBBMItemDefinition>();
	TestNotNull(TEXT("UBBMItemDefinition 인스턴스 생성 검증"), BBMItemDef);
	if (BBMItemDef == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("UBBMItemDefinition의 MaxDurability 초기값은 0.0f여야 함"), BBMItemDef->MaxDurability, 0.0f);

	UDISItemEntry* ItemEntry = NewObject<UDISItemEntry>();
	TestNotNull(TEXT("UDISItemEntry 인스턴스 생성 검증"), ItemEntry);
	if (ItemEntry == nullptr)
	{
		return false;
	}

	ItemEntry->InitializeEntry(BBMItemDef);
	TestEqual(TEXT("기본 초기화된 아이템 엔트리의 현재 내구도는 0.0f여야 함"), ItemEntry->GetCurrentDurability(), 0.0f);

	return true;
}

// BBM 3대 아이템 프로파일(무기 6종, 장비 4종, 소모품 3종) 계층 상속 및 CustomData Override 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMItemTypeProfileHierarchyTest,
	"DIS.BBM.ItemTypeProfileHierarchy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMItemTypeProfileHierarchyTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("테스트용 GameInstance 생성 검증"), GameInstance);
	if (GameInstance == nullptr)
	{
		return false;
	}

	UBBMGlobalLogic* BBMLogic = NewObject<UBBMGlobalLogic>(GameInstance);
	TestNotNull(TEXT("UBBMGlobalLogic 인스턴스 생성 검증"), BBMLogic);
	if (BBMLogic == nullptr)
	{
		return false;
	}
	BBMLogic->Initialize(nullptr);

	UBBMGlobalDefinition* BBMDef = LoadObject<UBBMGlobalDefinition>(nullptr, TEXT("/BBMGameFeatureCore/Globals/DA_GlobalDefinitionCore.DA_GlobalDefinitionCore"));
	if (BBMDef == nullptr)
	{
		BBMDef = NewObject<UBBMGlobalDefinition>(GameInstance);
	}
	TestNotNull(TEXT("UBBMGlobalDefinition 인스턴스(또는 DA 에셋) 준비 검증"), BBMDef);
	if (BBMDef == nullptr)
	{
		return false;
	}

	TestTrue(TEXT("UBBMGlobalDefinition 전역 프로파일 일괄 등록 성공"), BBMLogic->RegisterGlobalDefinition(BBMDef));

	// 1. 방패(Shield)는 Equipment가 아닌 Weapon 프로파일 소속이어야 함
	const FGameplayTag ShieldTag = BBMItemTypeTags::Item_Weapon_Shield.GetTag();
	const FGameplayTag WeaponCategoryTag = BBMItemTypeTags::Item_Weapon.GetTag();
	const FGameplayTag EquipmentCategoryTag = BBMItemTypeTags::Item_Equipment.GetTag();
	const FGameplayTag ConsumableCategoryTag = BBMItemTypeTags::Item_Consumable.GetTag();

	TestTrue(TEXT("방패는 Weapon 카테고리에 속해야 함"), BBMLogic->MatchesItemCategory(ShieldTag, WeaponCategoryTag));
	TestFalse(TEXT("방패는 Equipment 카테고리에 속하지 않아야 함"), BBMLogic->MatchesItemCategory(ShieldTag, EquipmentCategoryTag));

	// 2. 무기 6종 프로파일 검증: 고유 DisplayName + 부모 Weapon CustomData 상속 (스태킹 불가 1, 매 턴 반복 사용 Unlimited)
	struct FExpectedSubTypeInfo
	{
		FGameplayTag Tag;
		const TCHAR* ExpectedDisplayName;
	};

	const TArray<FExpectedSubTypeInfo> WeaponSubTypes = {
		{ BBMItemTypeTags::Item_Weapon_Shield.GetTag(), TEXT("방패") },
		{ BBMItemTypeTags::Item_Weapon_OneHandSword.GetTag(), TEXT("한손검") },
		{ BBMItemTypeTags::Item_Weapon_Spear.GetTag(), TEXT("창") },
		{ BBMItemTypeTags::Item_Weapon_Staff.GetTag(), TEXT("지팡이") },
		{ BBMItemTypeTags::Item_Weapon_Bow.GetTag(), TEXT("활") },
		{ BBMItemTypeTags::Item_Weapon_Dagger.GetTag(), TEXT("단검") }
	};

	for (const FExpectedSubTypeInfo& SubType : WeaponSubTypes)
	{
		const FDISItemTypeProfile& ResolvedProfile = BBMLogic->GetItemTypeProfile(SubType.Tag);
		const FBBMItemTypeCustomData* CustomDataPtr = ResolvedProfile.CustomData.GetPtr<FBBMItemTypeCustomData>();
		TestNotNull(TEXT("무기 하위 타입 CustomData 상속 유효성 검증"), CustomDataPtr);
		if (CustomDataPtr != nullptr)
		{
			TestEqual(TEXT("무기 하위 타입 MaxStackCount는 1이어야 함"), CustomDataPtr->MaxStackCount, BBMItemProfileDefaults::NonStackableCount);
			TestEqual(TEXT("무기 하위 타입 CustomData.UsageRule은 Unlimited여야 함"), CustomDataPtr->UsageRule, EBBMItemUsageRule::Unlimited);
			TestFalse(TEXT("무기 하위 타입은 스태킹 불가여야 함"), CustomDataPtr->CanStack());
		}
		TestEqual(TEXT("무기 하위 타입 고유 DisplayName 검증"), ResolvedProfile.DisplayName.ToString(), FString(SubType.ExpectedDisplayName));
		TestEqual(TEXT("무기 하위 타입 사용 규칙은 Unlimited여야 함"), BBMLogic->GetItemUsageRule(SubType.Tag), EBBMItemUsageRule::Unlimited);
	}

	// 3. 장비 4종 프로파일 검증: 고유 DisplayName + 부모 Equipment CustomData 상속 (스태킹 불가 1, 전투당 1회 사용 OncePerBattle)
	const TArray<FExpectedSubTypeInfo> EquipmentSubTypes = {
		{ BBMItemTypeTags::Item_Equipment_Gloves.GetTag(), TEXT("장갑") },
		{ BBMItemTypeTags::Item_Equipment_Clothes.GetTag(), TEXT("옷") },
		{ BBMItemTypeTags::Item_Equipment_Shoes.GetTag(), TEXT("신발") },
		{ BBMItemTypeTags::Item_Equipment_Pants.GetTag(), TEXT("바지") }
	};

	for (const FExpectedSubTypeInfo& SubType : EquipmentSubTypes)
	{
		const FDISItemTypeProfile& ResolvedProfile = BBMLogic->GetItemTypeProfile(SubType.Tag);
		const FBBMItemTypeCustomData* CustomDataPtr = ResolvedProfile.CustomData.GetPtr<FBBMItemTypeCustomData>();
		TestNotNull(TEXT("장비 하위 타입 CustomData 상속 유효성 검증"), CustomDataPtr);
		if (CustomDataPtr != nullptr)
		{
			TestEqual(TEXT("장비 하위 타입 MaxStackCount는 1이어야 함"), CustomDataPtr->MaxStackCount, BBMItemProfileDefaults::NonStackableCount);
			TestEqual(TEXT("장비 하위 타입 CustomData.UsageRule은 OncePerBattle이어야 함"), CustomDataPtr->UsageRule, EBBMItemUsageRule::OncePerBattle);
			TestFalse(TEXT("장비 하위 타입은 스태킹 불가여야 함"), CustomDataPtr->CanStack());
		}
		TestEqual(TEXT("장비 하위 타입 고유 DisplayName 검증"), ResolvedProfile.DisplayName.ToString(), FString(SubType.ExpectedDisplayName));
		TestTrue(TEXT("장비 하위 타입은 Equipment 카테고리에 속해야 함"), BBMLogic->MatchesItemCategory(SubType.Tag, EquipmentCategoryTag));
		TestEqual(TEXT("장비 하위 타입 사용 규칙은 OncePerBattle이어야 함"), BBMLogic->GetItemUsageRule(SubType.Tag), EBBMItemUsageRule::OncePerBattle);
	}

	// 4. 소모품 3종 프로파일 검증: 고유 DisplayName + 부모 Consumable CustomData 상속 (기본 MaxStackCount = 5, 효과 후 소멸 ConsumeOnUse)
	const TArray<FExpectedSubTypeInfo> ConsumableSubTypes = {
		{ BBMItemTypeTags::Item_Consumable_Potion.GetTag(), TEXT("물약") },
		{ BBMItemTypeTags::Item_Consumable_Throwable.GetTag(), TEXT("투척물") },
		{ BBMItemTypeTags::Item_Consumable_Scroll.GetTag(), TEXT("일회성 주문서") }
	};

	for (const FExpectedSubTypeInfo& SubType : ConsumableSubTypes)
	{
		const FDISItemTypeProfile& ResolvedProfile = BBMLogic->GetItemTypeProfile(SubType.Tag);
		const FBBMItemTypeCustomData* CustomDataPtr = ResolvedProfile.CustomData.GetPtr<FBBMItemTypeCustomData>();
		TestNotNull(TEXT("소모품 하위 타입 CustomData 상속 유효성 검증"), CustomDataPtr);
		if (CustomDataPtr != nullptr)
		{
			TestEqual(TEXT("소모품 하위 타입 기본 MaxStackCount는 5여야 함"), CustomDataPtr->MaxStackCount, BBMItemProfileDefaults::ConsumableMaxStackCount);
			TestEqual(TEXT("소모품 하위 타입 CustomData.UsageRule은 ConsumeOnUse여야 함"), CustomDataPtr->UsageRule, EBBMItemUsageRule::ConsumeOnUse);
			TestTrue(TEXT("소모품 하위 타입은 스태킹 가능해야 함"), CustomDataPtr->CanStack());
		}
		TestEqual(TEXT("소모품 하위 타입 고유 DisplayName 검증"), ResolvedProfile.DisplayName.ToString(), FString(SubType.ExpectedDisplayName));
		TestTrue(TEXT("소모품 하위 타입은 Consumable 카테고리에 속해야 함"), BBMLogic->MatchesItemCategory(SubType.Tag, ConsumableCategoryTag));
		TestEqual(TEXT("소모품 하위 타입 사용 규칙은 ConsumeOnUse여야 함"), BBMLogic->GetItemUsageRule(SubType.Tag), EBBMItemUsageRule::ConsumeOnUse);
	}

	// 5. 자식 프로파일에서 bOverrideCustomData = true 설정 시 부모(MaxStackCount = 5)를 상속받지 않고 고유 CustomData 적용 검증
	const FGameplayTag SpecialScrollTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Item.Consumable.Scroll.Ancient"));
	FDISItemTypeProfile SpecialScrollProfile;
	SpecialScrollProfile.DisplayName = FText::FromString(TEXT("고대 금서 주문서"));
	SpecialScrollProfile.bOverrideCustomData = true;
	SpecialScrollProfile.CustomData = TInstancedStruct<FDISItemTypeCustomData>::Make<FBBMItemTypeCustomData>(2, EBBMItemUsageRule::ConsumeOnUse);

	TestTrue(TEXT("특수 하위 주문서 프로파일 등록 성공"), BBMLogic->RegisterItemTypeProfile(SpecialScrollTag, SpecialScrollProfile));

	const FDISItemTypeProfile& ResolvedSpecialScroll = BBMLogic->GetItemTypeProfile(SpecialScrollTag);
	const FBBMItemTypeCustomData* SpecialScrollDataPtr = ResolvedSpecialScroll.CustomData.GetPtr<FBBMItemTypeCustomData>();
	TestNotNull(TEXT("특수 주문서 자체 CustomData 유효성 검증"), SpecialScrollDataPtr);
	if (SpecialScrollDataPtr != nullptr)
	{
		TestEqual(TEXT("Override 활성화 시 부모(5)를 상속받지 않고 자체 MaxStackCount(2) 적용 검증"), SpecialScrollDataPtr->MaxStackCount, 2);
		TestEqual(TEXT("특수 주문서 자체 CustomData.UsageRule 유지 검증"), SpecialScrollDataPtr->UsageRule, EBBMItemUsageRule::ConsumeOnUse);
	}
	TestEqual(TEXT("특수 주문서 DisplayName 검증"), ResolvedSpecialScroll.DisplayName.ToString(), TEXT("고대 금서 주문서"));
	TestEqual(TEXT("특수 주문서 자체 UsageRule 유지 검증"), BBMLogic->GetItemUsageRule(SpecialScrollTag), EBBMItemUsageRule::ConsumeOnUse);

	TestTrue(TEXT("특수 하위 주문서 프로파일 해제 성공"), BBMLogic->UnregisterItemTypeProfile(SpecialScrollTag));
	TestTrue(TEXT("UBBMGlobalDefinition 일괄 해제 성공"), BBMLogic->UnregisterGlobalDefinition(BBMDef));
	BBMLogic->Deinitialize();

	return true;
}

// BBM 장비(1회 사용 후 배낭에서 숨김 및 별도 보관 칸 이관) 및 소모품(최대 5스택 및 사용 후 소멸) 인벤토리 동작 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMItemUsageLifecycleTest,
	"DIS.BBM.ItemUsageLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMItemUsageLifecycleTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("테스트용 GameInstance 생성 검증"), GameInstance);
	if (GameInstance == nullptr)
	{
		return false;
	}

	UBBMGlobalLogic* BBMLogic = NewObject<UBBMGlobalLogic>(GameInstance);
	UBBMGlobalDefinition* BBMDef = LoadObject<UBBMGlobalDefinition>(nullptr, TEXT("/BBMGameFeatureCore/Globals/DA_GlobalDefinitionCore.DA_GlobalDefinitionCore"));
	if (BBMDef == nullptr)
	{
		BBMDef = NewObject<UBBMGlobalDefinition>(GameInstance);
	}
	if (BBMLogic == nullptr || BBMDef == nullptr)
	{
		return false;
	}
	BBMLogic->Initialize(nullptr);
	BBMLogic->RegisterGlobalDefinition(BBMDef);

	// 1. 중앙 그리드 배낭 인벤토리(4x4)와 사용 완료 장비 보관용 비가시 별도 인벤토리(4x4) 생성
	UDISInventoryDefinition* BackpackDef = NewObject<UDISInventoryDefinition>(GameInstance);
	BackpackDef->DefaultCapacity = 16;
	BackpackDef->MaxExpandableCapacity = 32;
	BackpackDef->LineBreakTileCount = 4;
	BackpackDef->SlotIndexingDirection = EDISSlotIndexingDirection::Horizontal;

	UDISInventory* BackpackInventory = NewObject<UDISInventory>(GameInstance);
	BackpackInventory->InitializeInventory(BackpackDef);

	UDISInventory* UsedEquipmentStorage = NewObject<UDISInventory>(GameInstance);
	UsedEquipmentStorage->InitializeInventory(BackpackDef);

	// 2. 장비(2x2 옷) 배치 후 전투 1회 사용 -> 다음 턴 진입 시 별도 보관 인벤토리 이관 검증
	UBBMItemDefinition* ClothesDef = NewObject<UBBMItemDefinition>(GameInstance);
	ClothesDef->ItemTypeTag = BBMItemTypeTags::Item_Equipment_Clothes.GetTag();
	ClothesDef->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(1, 1) };
	ClothesDef->BeatSequence.Beats.SetNum(4);

	TestEqual(TEXT("옷 아이템의 사용 규칙은 OncePerBattle이어야 함"), BBMLogic->GetItemUsageRule(ClothesDef->ItemTypeTag), EBBMItemUsageRule::OncePerBattle);

	UDISItemEntry* ClothesEntry = NewObject<UDISItemEntry>(GameInstance);
	ClothesEntry->InitializeEntry(ClothesDef);

	UDISItemStack* ClothesStack = NewObject<UDISItemStack>(GameInstance);
	TestTrue(TEXT("옷 엔트리 스택 삽입 성공"), ClothesStack->PushEntry(ClothesEntry));

	const FDISItemPlacementResult EquipPlaceResult = BackpackInventory->SetSlotItemStack(0, ClothesStack);
	TestTrue(TEXT("배낭 0번 슬롯에 2x2 옷 배치 성공"), EquipPlaceResult.bSuccess);
	TestFalse(TEXT("사용 전 배낭 0번 슬롯 점유 중"), BackpackInventory->IsSlotEmpty(0));
	TestFalse(TEXT("사용 전 배낭 5번 슬롯 점유 중"), BackpackInventory->IsSlotEmpty(5));

	// 턴 종료 처리 시뮬레이션: OncePerBattle 규칙에 따라 사용된 장비를 별도 보관 칸으로 이관
	if (BBMLogic->GetItemUsageRule(ClothesDef->ItemTypeTag) == EBBMItemUsageRule::OncePerBattle)
	{
		const FDISItemPlacementResult ArchiveResult = UsedEquipmentStorage->SetSlotItemStack(0, ClothesStack);
		TestTrue(TEXT("사용 완료된 장비를 별도 보관 칸으로 이관 성공"), ArchiveResult.bSuccess);
	}

	// 다음 턴부터 배낭 인벤토리 칸을 전혀 차지하지 않아 다른 아이템 배치가 가능하며 데이터는 별도 칸에 안전히 보관됨을 검증
	TestTrue(TEXT("이관 후 배낭 0번 슬롯은 비워져야 함"), BackpackInventory->IsSlotEmpty(0));
	TestTrue(TEXT("이관 후 배낭 1번 슬롯은 비워져야 함"), BackpackInventory->IsSlotEmpty(1));
	TestTrue(TEXT("이관 후 배낭 4번 슬롯은 비워져야 함"), BackpackInventory->IsSlotEmpty(4));
	TestTrue(TEXT("이관 후 배낭 5번 슬롯은 비워져야 함"), BackpackInventory->IsSlotEmpty(5));
	TestEqual(TEXT("장비 데이터의 소유 인벤토리는 별도 보관 인벤토리로 변경됨"), ClothesStack->GetOwningInventory(), UsedEquipmentStorage);
	TestEqual(TEXT("별도 보관 칸에 원본 장비 엔트리 데이터가 온전히 유지됨"), UsedEquipmentStorage->GetSlotItemStack(0)->GetTopEntry(), ClothesEntry);

	// 3. 소모품(1x1 물약) 기본 5스택 적재 및 효과 발동 후 소멸(ConsumeOnUse) 검증
	const FGameplayTag PotionTag = BBMItemTypeTags::Item_Consumable_Potion.GetTag();
	const FDISItemTypeCustomData* PotionCustomData = BBMLogic->GetItemTypeProfile(PotionTag).CustomData.GetPtr();
	TestNotNull(TEXT("소모품 물약 CustomData 유효성 검증"), PotionCustomData);
	if (PotionCustomData != nullptr)
	{
		TestEqual(TEXT("소모품 물약의 기본 최대 스택은 5여야 함"), PotionCustomData->MaxStackCount, BBMItemProfileDefaults::ConsumableMaxStackCount);
	}

	// 헤드리스 테스트 환경에서 UDISItemStack 폴백 프로파일에 소모품 프로파일을 임시 동기화하여 스택 동작 검증
	const FDISItemTypeProfile OriginalDefaultProfile = GetDefault<UDISGlobalSettings>()->DefaultItemProfile;
	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile = BBMLogic->GetItemTypeProfile(PotionTag);

	UBBMItemDefinition* PotionDef = NewObject<UBBMItemDefinition>(GameInstance);
	PotionDef->ItemTypeTag = PotionTag;
	PotionDef->Shape.Points = { FIntPoint(0, 0) };
	PotionDef->BeatSequence.Beats.SetNum(1);

	UDISItemStack* PotionStack = NewObject<UDISItemStack>(GameInstance);
	for (int32 Index = 0; Index < BBMItemProfileDefaults::ConsumableMaxStackCount; ++Index)
	{
		UDISItemEntry* PotionEntry = NewObject<UDISItemEntry>(GameInstance);
		PotionEntry->InitializeEntry(PotionDef);
		TestTrue(TEXT("물약 1~5번째 스택 적재 성공"), PotionStack->PushEntry(PotionEntry));
	}

	UDISItemEntry* OverflowPotionEntry = NewObject<UDISItemEntry>(GameInstance);
	OverflowPotionEntry->InitializeEntry(PotionDef);
	TestFalse(TEXT("기본 최대 스택(5) 초과 6번째 물약 적재는 거부되어야 함"), PotionStack->PushEntry(OverflowPotionEntry));

	// 이전에 옷이 차지하고 있던 배낭 0번 슬롯이 비워졌으므로 물약 스택을 즉시 배치 가능해야 함
	const FDISItemPlacementResult PotionPlaceResult = BackpackInventory->SetSlotItemStack(0, PotionStack);
	TestTrue(TEXT("장비가 빠진 배낭 0번 슬롯에 물약 5스택 배치 성공"), PotionPlaceResult.bSuccess);
	TestEqual(TEXT("배낭 0번 슬롯 초기 물약 수량은 5개"), BackpackInventory->GetSlotStackCount(0), 5);

	// 물약 5회 순차 소모 시뮬레이션: 마지막 1개 소모 시 배낭에서 완전히 사라지는지 검증
	for (int32 UseIndex = 0; UseIndex < BBMItemProfileDefaults::ConsumableMaxStackCount; ++UseIndex)
	{
		UDISItemEntry* ConsumedEntry = PotionStack->PopEntry();
		TestNotNull(TEXT("소모된 물약 엔트리 유효성 검증"), ConsumedEntry);

		if (PotionStack->IsEmpty() == true)
		{
			BackpackInventory->ClearItemStack(PotionStack);
		}
	}

	TestTrue(TEXT("모든 스택을 소진한 소모품 슬롯은 배낭에서 완전히 사라져 비어있어야 함"), BackpackInventory->IsSlotEmpty(0));
	TestNull(TEXT("소멸 처리된 소모품 슬롯의 스택 포인터는 nullptr이어야 함"), BackpackInventory->GetSlotItemStack(0));

	GetMutableDefault<UDISGlobalSettings>()->DefaultItemProfile = OriginalDefaultProfile;
	BBMLogic->UnregisterGlobalDefinition(BBMDef);
	BBMLogic->Deinitialize();

	return true;
}

// UBBMItemDefinition의 가격(Price), 비트 구성(Beats), 비트 수 조회(GetBeatCount), 링크드 애님 레이어 및 DataValidation 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMItemBeatAndAnimationTest,
	"DIS.BBM.ItemBeatAndAnimation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMItemBeatAndAnimationTest::RunTest(const FString& Parameters)
{
	UBBMItemDefinition* ItemDef = NewObject<UBBMItemDefinition>();
	TestNotNull(TEXT("ItemDef 인스턴스 생성 검증"), ItemDef);
	if (ItemDef == nullptr)
	{
		return false;
	}

	// 1. 가격(Price) 기본값 및 설정 검증
	TestEqual(TEXT("초기 가격은 0이어야 함"), ItemDef->Price, 0);
	ItemDef->Price = 150;
	TestEqual(TEXT("설정된 가격(150) 일치 검증"), ItemDef->Price, 150);

	// 2. 비트 수 조회 헬퍼(GetBeatCount) 검증: 점유 타일 수와 일치해야 함
	ItemDef->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0) }; // 2칸 점유 아이템
	TestEqual(TEXT("2칸 점유 아이템의 GetBeatCount()는 2여야 함"), ItemDef->GetBeatCount(), 2);

	// 3. 비트 구성(Beats) 설정 검증: 1비트당 Action 인스턴스 구조체(TInstancedStruct) 및 Animation 구성
	FBBMBeatDefinition Beat0;
	Beat0.Actions.Add(TInstancedStruct<FBBMBeatAction>::Make<FBBMCombatAction>(EBBMActionCategory::Damage, EBBMTargetScope::EnemyFrontline, 1.0f, 0.0f));
	TestTrue(TEXT("Beat0 액션 등록 여부 확인"), Beat0.HasActions());
	TestEqual(TEXT("Beat0 액션 개수는 1이어야 함"), Beat0.Actions.Num(), 1);

	// TInstancedStruct를 통한 기본 프로퍼티(ActionCategory) 조회 검증
	const FBBMBeatAction* ActionPtr = Beat0.Actions[0].GetPtr<FBBMBeatAction>();
	TestNotNull(TEXT("BeatAction 구조체 조회 성공"), ActionPtr);
	if (ActionPtr != nullptr)
	{
		TestEqual(TEXT("Action의 ActionCategory는 Damage여야 함"), ActionPtr->GetActionCategory(), EBBMActionCategory::Damage);
	}
	const FBBMCombatAction* CombatActionPtr = Beat0.Actions[0].GetPtr<FBBMCombatAction>();
	TestNotNull(TEXT("FBBMCombatAction 파생 구조체 조회 성공"), CombatActionPtr);
	if (CombatActionPtr != nullptr)
	{
		TestEqual(TEXT("CombatAction의 TargetScope는 EnemyFrontline이어야 함"), CombatActionPtr->TargetScope, EBBMTargetScope::EnemyFrontline);
	}

	// Beat0 애니메이션 구성: 빈 상태 검증 (비어있을 수 있음)
	TestFalse(TEXT("Beat0 애니메이션 구성은 비어있어야 함"), Beat0.HasAnimations());

	// Beat1 구성: Animation 설정 등록 (ImpactWindow에서 상태 태그 적용)
	FBBMBeatDefinition Beat1;
	const FGameplayTag AttackAnimTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Animation.State.Attack"));
	Beat1.Animations.Emplace(EBBMVisualWindow::ImpactWindow, AttackAnimTag);
	TestTrue(TEXT("Beat1 애니메이션 등록 여부 확인"), Beat1.HasAnimations());
	TestEqual(TEXT("Beat1 애니메이션 개수 1 검증"), Beat1.Animations.Num(), 1);
	TestTrue(TEXT("Beat1 애니메이션 유효성 검증"), Beat1.Animations[0].IsValid());
	TestEqual(TEXT("Beat1 애니메이션 윈도우 검증"), Beat1.Animations[0].VisualWindow, EBBMVisualWindow::ImpactWindow);
	TestEqual(TEXT("Beat1 애니메이션 상태 태그 검증"), Beat1.Animations[0].StateTag, AttackAnimTag);

	ItemDef->BeatSequence.Beats.Add(Beat0);
	ItemDef->BeatSequence.Beats.Add(Beat1);
	TestEqual(TEXT("Beats TArray 크기는 2여야 함"), ItemDef->BeatSequence.Beats.Num(), 2);
	TestEqual(TEXT("GetBeats() 헬퍼 크기도 2여야 함"), ItemDef->GetBeats().Num(), 2);

	// 4. 링크드 애님 레이어(LinkedAnimLayers) 등록 검증
	ItemDef->LinkedAnimLayers.Add(UAnimInstance::StaticClass());
	TestEqual(TEXT("링크드 애님 레이어 개수는 1이어야 함"), ItemDef->LinkedAnimLayers.Num(), 1);
	TestEqual(TEXT("등록된 레이어 클래스 일치 검증"), ItemDef->LinkedAnimLayers[0].Get(), UAnimInstance::StaticClass());

#if WITH_EDITOR
	// 5. DataValidation 검증: 비트 수와 점유 타일 수가 일치할 때 Valid
	FDataValidationContext Context;
	EDataValidationResult ValidResult = ItemDef->IsDataValid(Context);
	TestEqual(TEXT("비트 수와 점유 타일 수가 일치할 때 IsDataValid는 Valid여야 함"), ValidResult, EDataValidationResult::Valid);

	// 비트 수와 점유 타일 수 불일치 시 Invalid 및 Error 발생 검증
	ItemDef->BeatSequence.Beats.Pop(); // 1개로 줄여서 2칸 점유와 불일치 유발
	FDataValidationContext MismatchContext;
	EDataValidationResult InvalidResult = ItemDef->IsDataValid(MismatchContext);
	TestEqual(TEXT("비트 수 불일치 시 IsDataValid는 Invalid여야 함"), InvalidResult, EDataValidationResult::Invalid);
#endif

	return true;
}

// UBBMItemEntry의 동적 비트(DynamicBeats), 구간 효과(EffectRanges) 및 UBBMAttributeSet 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMItemGASAndDynamicBeatsTest,
	"DIS.BBM.ItemGASAndDynamicBeats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMItemGASAndDynamicBeatsTest::RunTest(const FString& Parameters)
{
	// 1. UBBMItemDefinition 생성 및 기본 GAS Effect/비트 설정
	UBBMItemDefinition* ItemDef = NewObject<UBBMItemDefinition>();
	TestNotNull(TEXT("ItemDef 생성 검증"), ItemDef);
	if (ItemDef == nullptr)
	{
		return false;
	}

	ItemDef->Shape.Points = { FIntPoint(0, 0), FIntPoint(1, 0) }; // 2칸 점유
	FBBMBeatDefinition Beat0;
	FBBMBeatDefinition Beat1;
	ItemDef->BeatSequence.Beats.Add(Beat0);
	ItemDef->BeatSequence.Beats.Add(Beat1);

	const FGameplayTag AttackStatTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Stat.Combat.AttackPower"));
	FBBMItemGameplayEffectConfig DefaultGE;
	DefaultGE.SetByCallerMagnitudes.Add(AttackStatTag, 15.0f);
	FBBMBeatEffectRange DefaultRange(0, 2);
	DefaultRange.GrantedEffects.Add(DefaultGE);
	ItemDef->BeatSequence.EffectRanges.Add(DefaultRange);

	// 2. UBBMItemEntry 인스턴스화 및 복사 검증
	UBBMItemEntry* ItemEntry = NewObject<UBBMItemEntry>();
	TestNotNull(TEXT("ItemEntry 생성 검증"), ItemEntry);
	if (ItemEntry == nullptr)
	{
		return false;
	}

	ItemEntry->InitializeEntry(ItemDef);

	// DynamicBeatSequence 및 DynamicBeats 복사 검증
	TestEqual(TEXT("ItemEntry의 DynamicBeatSequence 비트 수는 ItemDef와 동일한 2여야 함"), ItemEntry->GetDynamicBeatSequence().Beats.Num(), 2);
	TestEqual(TEXT("ItemEntry의 DynamicBeats 수는 ItemDef와 동일한 2여야 함"), ItemEntry->GetDynamicBeats().Num(), 2);

	// EffectRanges 복사 및 SetByCaller 조회 검증
	TestEqual(TEXT("ItemEntry의 EffectRanges 수는 1이어야 함"), ItemEntry->GetEffectRanges().Num(), 1);
	TestTrue(TEXT("0번 EffectRange 유효성 검증"), ItemEntry->GetEffectRanges().IsValidIndex(0));
	if (ItemEntry->GetEffectRanges().IsValidIndex(0) == true && ItemEntry->GetEffectRanges()[0].GrantedEffects.IsEmpty() == false)
	{
		TestEqual(TEXT("초기 SetByCaller 공격력 수치는 15.0f여야 함"), ItemEntry->GetEffectRanges()[0].GrantedEffects[0].SetByCallerMagnitudes[AttackStatTag], 15.0f);

		// 3. 런타임 강화 조작 검증: SetByCaller 수치 증가
		ItemEntry->GetMutableEffectRanges()[0].GrantedEffects[0].SetByCallerMagnitudes[AttackStatTag] = 25.0f;
		TestEqual(TEXT("강화 후 SetByCaller 공격력 수치는 25.0f여야 함"), ItemEntry->GetEffectRanges()[0].GrantedEffects[0].SetByCallerMagnitudes[AttackStatTag], 25.0f);
	}

	// 런타임 강화 조작 검증: 특정 비트에 Action 추가 (TInstancedStruct)
	const TInstancedStruct<FBBMBeatAction> NewAction = TInstancedStruct<FBBMBeatAction>::Make<FBBMCombatAction>(EBBMActionCategory::Damage, EBBMTargetScope::EnemyFrontline, 1.2f, 5.0f);
	const bool bAddActionSuccess = ItemEntry->AddActionToBeat(0, NewAction);
	TestTrue(TEXT("비트 0에 액션 추가 성공 검증"), bAddActionSuccess);
	TestTrue(TEXT("비트 0에 액션 등록 여부 확인"), ItemEntry->GetDynamicBeats()[0].HasActions());

	// 런타임 EffectRange 추가 및 제거 검증
	FBBMBeatEffectRange ExtraRange(1, 2);
	ItemEntry->AddEffectRange(ExtraRange);
	TestEqual(TEXT("신규 EffectRange 추가 후 개수는 2여야 함"), ItemEntry->GetEffectRanges().Num(), 2);
	TestTrue(TEXT("신규 EffectRange 정상 제거 성공"), ItemEntry->RemoveEffectRange(1));
	TestEqual(TEXT("제거 후 EffectRanges 개수는 1이어야 함"), ItemEntry->GetEffectRanges().Num(), 1);

	// 4. UBBMAttributeSet 스탯 및 클램핑 검증
	UBBMAttributeSet* AttributeSet = NewObject<UBBMAttributeSet>();
	TestNotNull(TEXT("AttributeSet 인스턴스 생성 검증"), AttributeSet);
	if (AttributeSet != nullptr)
	{
		TestEqual(TEXT("기본 체력은 FLT_MAX여야 함"), AttributeSet->GetHealth(), TNumericLimits<float>::Max());
		TestEqual(TEXT("기본 최대 체력은 FLT_MAX여야 함"), AttributeSet->GetMaxHealth(), TNumericLimits<float>::Max());
		TestEqual(TEXT("기본 방어도는 0.0f여야 함"), AttributeSet->GetArmor(), 0.0f);
		TestEqual(TEXT("기본 공격력은 10.0f여야 함"), AttributeSet->GetAttackPower(), 10.0f);
		TestEqual(TEXT("기본 방어력은 5.0f여야 함"), AttributeSet->GetDefensePower(), 5.0f);

		// 클램핑 검증: MaxHealth를 100.0f로 세팅한 후 PreAttributeChange에 의한 MaxHealth 초과 방지 및 음수 방지 검증
		AttributeSet->InitMaxHealth(100.0f);
		float ClampedHealth = 150.0f;
		AttributeSet->PreAttributeChange(UBBMAttributeSet::GetHealthAttribute(), ClampedHealth);
		TestEqual(TEXT("PreAttributeChange 호출 시 MaxHealth(100.0f)로 클램핑되어야 함"), ClampedHealth, 100.0f);

		float ClampedNegativeArmor = -20.0f;
		AttributeSet->PreAttributeChange(UBBMAttributeSet::GetArmorAttribute(), ClampedNegativeArmor);
		TestEqual(TEXT("PreAttributeChange 호출 시 음수 방어도는 0.0f로 클램핑되어야 함"), ClampedNegativeArmor, 0.0f);
	}

	return true;
}

// 6. 다형성 비트 강화 시스템(UBBMBeatEnhancement)의 3대 타입, Curve 레벨 스케일링, 무손실 롤백 및 순서 독립성 검증
class FBBMBeatEnhancementContainerTestAccessor : public UDISEnhancementContainer
{
public:
	void SetEnhancementLevel(TSubclassOf<UDISEnhancement> InClass, int32 InLevel)
	{
		for (FDISActiveEnhancement& Existing : ActiveEnhancements)
		{
			if (Existing.EnhancementClass == InClass)
			{
				Existing.ResetModifiers();
				if (InLevel > 0)
				{
					Existing.AddModifier(EDISEnhancementModifierTarget::CurrentLevel, EDISEnhancementLevelOp::Add, FDISActiveEnhancementEffectHandle(1), static_cast<float>(InLevel));
					Existing.bIsActive = true;
				}
				else
				{
					Existing.bIsActive = false;
				}
				return;
			}
		}

		FDISActiveEnhancement NewEnh(InClass, InLevel > 0, 10);
		if (InLevel > 0)
		{
			NewEnh.AddModifier(EDISEnhancementModifierTarget::CurrentLevel, EDISEnhancementLevelOp::Add, FDISActiveEnhancementEffectHandle(1), static_cast<float>(InLevel));
		}
		ActiveEnhancements.Add(NewEnh);
	}

	void ClearAllEnhancements()
	{
		ActiveEnhancements.Empty();
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMBeatEnhancementPipelineTest,
	"DIS.BBM.ItemBeatEnhancementPipeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMBeatEnhancementPipelineTest::RunTest(const FString& Parameters)
{
	// 0. 베이스 아이템 정의 생성: 2비트 (비트 0: 공격 액션, 비트 1: 방어 액션)
	UBBMItemDefinition* ItemDef = NewObject<UBBMItemDefinition>();
	ItemDef->BeatSequence.Beats.SetNum(2);

	// 비트 0: 기본 공격 액션 (Damage, 1.0f, 10.0f)
	ItemDef->BeatSequence.Beats[0].Actions.Add(
		TInstancedStruct<FBBMBeatAction>::Make<FBBMCombatAction>(
			EBBMActionCategory::Damage, EBBMTargetScope::EnemyFrontline, 1.0f, 10.0f));

	// 비트 1: 기본 방어 액션 (Defense, 1.0f, 5.0f)
	ItemDef->BeatSequence.Beats[1].Actions.Add(
		TInstancedStruct<FBBMBeatAction>::Make<FBBMCombatAction>(
			EBBMActionCategory::Defense, EBBMTargetScope::Self, 1.0f, 5.0f));

	// 기본 EffectRange (0~2 비트)
	ItemDef->BeatSequence.EffectRanges.Add(FBBMBeatEffectRange(0, 2));

	// 아이템 엔트리 생성 및 초기화
	UBBMItemEntry* ItemEntry = NewObject<UBBMItemEntry>();
	ItemEntry->InitializeEntry(ItemDef);

	TestEqual(TEXT("초기 비트 수는 2여야 함"), ItemEntry->GetDynamicBeats().Num(), 2);
	TestEqual(TEXT("초기 EffectRanges 수는 1이어야 함"), ItemEntry->GetEffectRanges().Num(), 1);
	TestEqual(TEXT("비트 0의 액션 수는 1이어야 함"), ItemEntry->GetDynamicBeats()[0].Actions.Num(), 1);
	TestEqual(TEXT("비트 1의 액션 수는 1이어야 함"), ItemEntry->GetDynamicBeats()[1].Actions.Num(), 1);

	UDISEnhancementContainer* RawContainer = ItemEntry->GetEnhancementContainer();
	TestNotNull(TEXT("EnhancementContainer 유효성 검증"), RawContainer);
	if (RawContainer == nullptr)
	{
		return false;
	}

	FBBMBeatEnhancementContainerTestAccessor* Accessor = static_cast<FBBMBeatEnhancementContainerTestAccessor*>(RawContainer);

	// 1. 타입 1: UBBMEffectRangeBeatEnhancement CDO 설정 (비트 구간 효과 추가 및 Curve 레벨 스케일링 검증)
	UBBMEffectRangeBeatEnhancement* RangeCDO = GetMutableDefault<UBBMEffectRangeBeatEnhancement>();
	RangeCDO->AddedEffectRange = FBBMBeatEffectRange(1, 2);
	FBBMItemGameplayEffectConfig BuffConfig;
	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("BBM.Combat.AttackPower"));
	BuffConfig.SetByCallerMagnitudes.Add(AttackTag, 10.0f);
	RangeCDO->AddedEffectRange.GrantedEffects.Add(BuffConfig);

	// Curve 생성: 레벨 1 -> 1.0배, 레벨 2 -> 2.5배
	UCurveFloat* RangeCurve = NewObject<UCurveFloat>();
	RangeCurve->FloatCurve.AddKey(1.0f, 1.0f);
	RangeCurve->FloatCurve.AddKey(2.0f, 2.5f);
	RangeCDO->MagnitudeCurve = RangeCurve;

	// 2. 타입 2: UBBMConditionalActionBeatEnhancement CDO 설정 (공격 비트에 출혈 상태이상 조건부 주입)
	UBBMConditionalActionBeatEnhancement* ActionCDO = GetMutableDefault<UBBMConditionalActionBeatEnhancement>();
	ActionCDO->ConditionalAction.TriggerCategory = EBBMActionCategory::Damage;
	ActionCDO->ConditionalAction.ActionToInject = TInstancedStruct<FBBMBeatAction>::Make<FBBMCombatAction>(
		EBBMActionCategory::Damage, EBBMTargetScope::EnemyFrontline, 0.5f, 2.0f);
	ActionCDO->ConditionalAction.bTargetBaseBeatsOnly = false;

	UCurveFloat* BleedCurve = NewObject<UCurveFloat>();
	BleedCurve->FloatCurve.AddKey(1.0f, 1.0f);
	BleedCurve->FloatCurve.AddKey(2.0f, 2.0f);
	ActionCDO->MagnitudeCurve = BleedCurve;

	// 레벨 1 Range 강화 적용 검증
	Accessor->SetEnhancementLevel(UBBMEffectRangeBeatEnhancement::StaticClass(), 1);
	ItemEntry->RebuildDynamicBeatSequence();

	TestEqual(TEXT("레벨 1 Range 강화 적용 후 EffectRanges 수는 2여야 함"), ItemEntry->GetEffectRanges().Num(), 2);
	if (ItemEntry->GetEffectRanges().Num() >= 2 && ItemEntry->GetEffectRanges()[1].GrantedEffects.Num() > 0)
	{
		TestEqual(TEXT("추가된 Range의 SetByCaller 수치는 10.0f(1.0배)여야 함"),
			ItemEntry->GetEffectRanges()[1].GrantedEffects[0].SetByCallerMagnitudes[AttackTag], 10.0f);
	}

	// Range 레벨 2로 증가 (Curve 2.5배율 검증)
	Accessor->SetEnhancementLevel(UBBMEffectRangeBeatEnhancement::StaticClass(), 2);
	ItemEntry->RebuildDynamicBeatSequence();

	TestEqual(TEXT("레벨 2 Range 강화 적용 후 SetByCaller 수치는 25.0f(2.5배)여야 함"),
		ItemEntry->GetEffectRanges()[1].GrantedEffects[0].SetByCallerMagnitudes[AttackTag], 25.0f);

	// 출혈(Bleed) 강화 레벨 1 적용 -> 비트 0(공격 비트)에 액션 주입 확인
	Accessor->SetEnhancementLevel(UBBMConditionalActionBeatEnhancement::StaticClass(), 1);
	ItemEntry->RebuildDynamicBeatSequence();

	TestEqual(TEXT("비트 0(공격 비트)의 액션 수는 2개여야 함"), ItemEntry->GetDynamicBeats()[0].Actions.Num(), 2);
	TestEqual(TEXT("비트 1(방어 비트)의 액션 수는 영향 없이 1개여야 함"), ItemEntry->GetDynamicBeats()[1].Actions.Num(), 1);

	// 출혈 레벨 2로 증가 (Curve 2.0배율 검증: 계수 0.5f * 2 = 1.0f, 기본값 2.0f * 2 = 4.0f)
	Accessor->SetEnhancementLevel(UBBMConditionalActionBeatEnhancement::StaticClass(), 2);
	ItemEntry->RebuildDynamicBeatSequence();

	if (ItemEntry->GetDynamicBeats()[0].Actions.Num() >= 2)
	{
		const FBBMCombatAction* ScaledCombat = ItemEntry->GetDynamicBeats()[0].Actions[1].GetPtr<FBBMCombatAction>();
		TestNotNull(TEXT("주입된 액션은 FBBMCombatAction이어야 함"), ScaledCombat);
		if (ScaledCombat != nullptr)
		{
			TestEqual(TEXT("레벨 2 출혈 계수는 1.0f(0.5f*2)여야 함"), ScaledCombat->Coefficient, 1.0f);
			TestEqual(TEXT("레벨 2 출혈 기본값은 4.0f(2.0f*2)여야 함"), ScaledCombat->BaseValue, 4.0f);
		}
	}

	// 무손실 롤백(Rollback) 검증: 출혈 강화 해제 (레벨 0)
	Accessor->SetEnhancementLevel(UBBMConditionalActionBeatEnhancement::StaticClass(), 0);
	ItemEntry->RebuildDynamicBeatSequence();

	TestEqual(TEXT("출혈 강화 해제 후 비트 0의 액션 수는 다시 1개로 원복되어야 함"), ItemEntry->GetDynamicBeats()[0].Actions.Num(), 1);
	TestEqual(TEXT("EffectRanges 수는 여전히 2개 유지되어야 함"), ItemEntry->GetEffectRanges().Num(), 2);

	// 3. 타입 3 검증: 방어 비트에 버프 조건부 주입
	ActionCDO->ConditionalAction.TriggerCategory = EBBMActionCategory::Defense;
	ActionCDO->ConditionalAction.ActionToInject = TInstancedStruct<FBBMBeatAction>::Make<FBBMCombatAction>(
		EBBMActionCategory::Defense, EBBMTargetScope::Self, 1.0f, 5.0f);
	ActionCDO->MagnitudeCurve = nullptr;

	Accessor->SetEnhancementLevel(UBBMConditionalActionBeatEnhancement::StaticClass(), 1);
	ItemEntry->RebuildDynamicBeatSequence();

	TestEqual(TEXT("비트 0(공격 비트) 액션 수는 1개여야 함"), ItemEntry->GetDynamicBeats()[0].Actions.Num(), 1);
	TestEqual(TEXT("비트 1(방어 비트) 액션 수는 2개여야 함"), ItemEntry->GetDynamicBeats()[1].Actions.Num(), 2);

	// 모든 강화 완전 해제 -> 원본 상태로 100% 원복 검증
	Accessor->ClearAllEnhancements();
	ItemEntry->RebuildDynamicBeatSequence();

	TestEqual(TEXT("모든 강화 해제 후 EffectRanges 수는 원본과 동일한 1개여야 함"), ItemEntry->GetEffectRanges().Num(), 1);
	TestEqual(TEXT("모든 강화 해제 후 비트 0 액션 수는 1개여야 함"), ItemEntry->GetDynamicBeats()[0].Actions.Num(), 1);
	TestEqual(TEXT("모든 강화 해제 후 비트 1 액션 수는 1개여야 함"), ItemEntry->GetDynamicBeats()[1].Actions.Num(), 1);

	// 테스트 종료 후 CDO 상태 기본값으로 정리
	RangeCDO->AddedEffectRange = FBBMBeatEffectRange();
	RangeCDO->MagnitudeCurve = nullptr;
	ActionCDO->ConditionalAction = FBBMConditionalActionConfig();
	ActionCDO->MagnitudeCurve = nullptr;

	return true;
}

// BBM 4대 표준 희귀도 네이티브 태그 및 Rank 서열 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMItemRarityTagsTest,
	"DIS.BBM.ItemRarityTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMItemRarityTagsTest::RunTest(const FString& Parameters)
{
	// 1. 4대 표준 희귀도 태그 유효성 검증
	TestTrue(TEXT("Item_Rarity_Common 태그 유효성"), BBMItemRarityTags::Item_Rarity_Common.GetTag().IsValid());
	TestTrue(TEXT("Item_Rarity_Rare 태그 유효성"), BBMItemRarityTags::Item_Rarity_Rare.GetTag().IsValid());
	TestTrue(TEXT("Item_Rarity_Epic 태그 유효성"), BBMItemRarityTags::Item_Rarity_Epic.GetTag().IsValid());
	TestTrue(TEXT("Item_Rarity_Legendary 태그 유효성"), BBMItemRarityTags::Item_Rarity_Legendary.GetTag().IsValid());

	// 2. 태그 이름 확인
	TestEqual(TEXT("Item_Rarity_Common 태그명 확인"), BBMItemRarityTags::Item_Rarity_Common.GetTag().GetTagName(), FName(TEXT("Item.Rarity.Common")));
	TestEqual(TEXT("Item_Rarity_Rare 태그명 확인"), BBMItemRarityTags::Item_Rarity_Rare.GetTag().GetTagName(), FName(TEXT("Item.Rarity.Rare")));
	TestEqual(TEXT("Item_Rarity_Epic 태그명 확인"), BBMItemRarityTags::Item_Rarity_Epic.GetTag().GetTagName(), FName(TEXT("Item.Rarity.Epic")));
	TestEqual(TEXT("Item_Rarity_Legendary 태그명 확인"), BBMItemRarityTags::Item_Rarity_Legendary.GetTag().GetTagName(), FName(TEXT("Item.Rarity.Legendary")));

	// 3. 서열(Rank) 상대 관계 검증 (Common < Rare < Epic < Legendary)
	TestTrue(TEXT("CommonRank < RareRank"), BBMItemRarityDefaults::CommonRank < BBMItemRarityDefaults::RareRank);
	TestTrue(TEXT("RareRank < EpicRank"), BBMItemRarityDefaults::RareRank < BBMItemRarityDefaults::EpicRank);
	TestTrue(TEXT("EpicRank < LegendaryRank"), BBMItemRarityDefaults::EpicRank < BBMItemRarityDefaults::LegendaryRank);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

