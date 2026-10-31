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
#include "Combat/BBMAttributeSet.h"
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

	UBBMGlobalDefinition* BBMDef = NewObject<UBBMGlobalDefinition>(GameInstance);
	TestNotNull(TEXT("UBBMGlobalDefinition 인스턴스 생성 검증"), BBMDef);
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
	UBBMGlobalDefinition* BBMDef = NewObject<UBBMGlobalDefinition>(GameInstance);
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
	ClothesDef->Beats.SetNum(4);

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
	PotionDef->Beats.SetNum(1);

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

	// 3. 비트 구성(Beats) 설정 검증: 1비트당 Effect 인스턴스 구조체(TInstancedStruct) 및 Animation 구성
	FBBMBeatDefinition Beat0;
	Beat0.Effects.Add(TInstancedStruct<FBBMEffectBase>::Make<FBBMCombatEffect>(EBBMEffectCategory::Damage, EBBMTargetScope::EnemyFrontline, 1.0f, 0.0f));
	TestTrue(TEXT("Beat0 효과 등록 여부 확인"), Beat0.HasEffects());
	TestEqual(TEXT("Beat0 효과 개수는 1이어야 함"), Beat0.Effects.Num(), 1);

	// TInstancedStruct를 통한 기본 프로퍼티(EffectCategory) 조회 검증
	const FBBMEffectBase* EffectPtr = Beat0.Effects[0].GetPtr<FBBMEffectBase>();
	TestNotNull(TEXT("EffectBase 구조체 조회 성공"), EffectPtr);
	if (EffectPtr != nullptr)
	{
		TestEqual(TEXT("Effect의 EffectCategory는 Damage여야 함"), EffectPtr->GetEffectCategory(), EBBMEffectCategory::Damage);
	}
	const FBBMCombatEffect* CombatEffectPtr = Beat0.Effects[0].GetPtr<FBBMCombatEffect>();
	TestNotNull(TEXT("FBBMCombatEffect 파생 구조체 조회 성공"), CombatEffectPtr);
	if (CombatEffectPtr != nullptr)
	{
		TestEqual(TEXT("CombatEffect의 TargetScope는 EnemyFrontline이어야 함"), CombatEffectPtr->TargetScope, EBBMTargetScope::EnemyFrontline);
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

	ItemDef->Beats.Add(Beat0);
	ItemDef->Beats.Add(Beat1);
	TestEqual(TEXT("Beats TArray 크기는 2여야 함"), ItemDef->Beats.Num(), 2);

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
	ItemDef->Beats.Pop(); // 1개로 줄여서 2칸 점유와 불일치 유발
	FDataValidationContext MismatchContext;
	EDataValidationResult InvalidResult = ItemDef->IsDataValid(MismatchContext);
	TestEqual(TEXT("비트 수 불일치 시 IsDataValid는 Invalid여야 함"), InvalidResult, EDataValidationResult::Invalid);
#endif

	return true;
}

// UBBMItemEntry의 동적 비트(DynamicBeats), GAS 구성(DynamicGrantedEffects) 및 UBBMAttributeSet 검증
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
	ItemDef->Beats.Add(Beat0);
	ItemDef->Beats.Add(Beat1);

	const FGameplayTag AttackStatTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Stat.Combat.AttackPower"));
	FBBMItemGameplayEffectConfig DefaultGE;
	DefaultGE.SetByCallerMagnitudes.Add(AttackStatTag, 15.0f);
	ItemDef->DefaultGrantedEffects.Add(DefaultGE);

	// 2. UBBMItemEntry 인스턴스화 및 복사 검증
	UBBMItemEntry* ItemEntry = NewObject<UBBMItemEntry>();
	TestNotNull(TEXT("ItemEntry 생성 검증"), ItemEntry);
	if (ItemEntry == nullptr)
	{
		return false;
	}

	ItemEntry->InitializeEntry(ItemDef);

	// DynamicBeats 복사 검증
	TestEqual(TEXT("ItemEntry의 DynamicBeats 수는 ItemDef와 동일한 2여야 함"), ItemEntry->GetDynamicBeats().Num(), 2);

	// DynamicGrantedEffects 복사 및 SetByCaller 조회 검증
	TestEqual(TEXT("ItemEntry의 DynamicGrantedEffects 수는 1이어야 함"), ItemEntry->GetDynamicGrantedEffects().Num(), 1);
	TestEqual(TEXT("초기 SetByCaller 공격력 수치는 15.0f여야 함"), ItemEntry->GetSetByCallerMagnitude(AttackStatTag), 15.0f);

	// 3. 런타임 강화 조작 검증: SetByCaller 수치 증가
	const bool bSetSuccess = ItemEntry->SetByCallerMagnitude(AttackStatTag, 25.0f);
	TestTrue(TEXT("SetByCaller 수치 갱신 성공 검증"), bSetSuccess);
	TestEqual(TEXT("강화 후 SetByCaller 공격력 수치는 25.0f여야 함"), ItemEntry->GetSetByCallerMagnitude(AttackStatTag), 25.0f);

	// 런타임 강화 조작 검증: 특정 비트에 Effect 추가 (TInstancedStruct)
	const TInstancedStruct<FBBMEffectBase> NewEffect = TInstancedStruct<FBBMEffectBase>::Make<FBBMCombatEffect>(EBBMEffectCategory::Damage, EBBMTargetScope::EnemyFrontline, 1.2f, 5.0f);
	const bool bAddEffectSuccess = ItemEntry->AddEffectToBeat(0, NewEffect);
	TestTrue(TEXT("비트 0에 이펙트 추가 성공 검증"), bAddEffectSuccess);
	TestTrue(TEXT("비트 0에 이펙트 등록 여부 확인"), ItemEntry->GetDynamicBeats()[0].HasEffects());

	// 런타임 비트 슬롯 확장 검증
	FBBMBeatDefinition ExtraBeat;
	ItemEntry->AddDynamicBeat(ExtraBeat);
	TestEqual(TEXT("슬롯 확장 후 DynamicBeats 수는 3이어야 함"), ItemEntry->GetDynamicBeats().Num(), 3);

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

#endif // WITH_DEV_AUTOMATION_TESTS

