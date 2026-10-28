#include "Misc/AutomationTest.h"
#include "Global/DISGlobalSubsystem.h"
#include "Item/DISItemTypes.h"
#include "GameplayTagsManager.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * UDISGlobalSubsystem의 아이템 타입 계층 상속, 캐싱, 오버라이드 및 계층 유틸리티 자동화 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISItemTypeHierarchyTest,
	"DIS.Global.ItemTypeHierarchyAndInheritance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISItemTypeHierarchyTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("테스트용 GameInstance 생성 성공 검증"), GameInstance);
	if (GameInstance == nullptr)
	{
		return false;
	}

	UDISGlobalSubsystem* GlobalSubsystem = NewObject<UDISGlobalSubsystem>(GameInstance);
	TestNotNull(TEXT("UDISGlobalSubsystem 인스턴스 생성 성공 검증"), GlobalSubsystem);
	if (GlobalSubsystem == nullptr)
	{
		return false;
	}

	// 1. 테스트용 계층형 GameplayTag 등록
	UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();
	const FGameplayTag TagEquipment = TagManager.AddNativeGameplayTag(FName("Test.Item.Equipment"));
	const FGameplayTag TagWeapon = TagManager.AddNativeGameplayTag(FName("Test.Item.Equipment.Weapon"));
	const FGameplayTag TagSword = TagManager.AddNativeGameplayTag(FName("Test.Item.Equipment.Weapon.Sword"));
	const FGameplayTag TagTwoHanded = TagManager.AddNativeGameplayTag(FName("Test.Item.Equipment.Weapon.Sword.TwoHanded"));
	const FGameplayTag TagDagger = TagManager.AddNativeGameplayTag(FName("Test.Item.Equipment.Weapon.Dagger"));
	const FGameplayTag TagConsumable = TagManager.AddNativeGameplayTag(FName("Test.Item.Consumable"));

	TestTrue(TEXT("TagEquipment 유효성 검증"), TagEquipment.IsValid());
	TestTrue(TEXT("TagTwoHanded 유효성 검증"), TagTwoHanded.IsValid());
	TestTrue(TEXT("TagDagger 유효성 검증"), TagDagger.IsValid());

	// 2. 대분류(Equipment) 프로파일만 서브시스템에 등록
	FDISItemTypeProfile EquipmentProfile;
	EquipmentProfile.DisplayName = FText::FromString(TEXT("장비 대분류"));
	EquipmentProfile.MaxStackCount = 1;

	TestTrue(TEXT("대분류 Equipment 등록 성공"), GlobalSubsystem->RegisterItemTypeProfile(TagEquipment, EquipmentProfile));

	// 3. 미등록 최하위 소분류(TwoHanded) 조회 시 대분류로부터 상속 검증
	const FDISItemTypeProfile& ResolvedTwoHanded = GlobalSubsystem->GetItemTypeProfile(TagTwoHanded);
	TestEqual(TEXT("TwoHanded는 부모 Equipment의 MaxStackCount(1)을 상속받아야 함"), ResolvedTwoHanded.MaxStackCount, 1);
	TestTrue(TEXT("TwoHanded는 부모 Equipment의 DisplayName을 상속받아야 함"), ResolvedTwoHanded.DisplayName.EqualTo(EquipmentProfile.DisplayName));

	// 4. 소분류(Dagger)에 오버라이드 프로파일 등록
	FDISItemTypeProfile DaggerProfile;
	DaggerProfile.DisplayName = FText::FromString(TEXT("단검 소분류"));
	DaggerProfile.MaxStackCount = 5;

	TestTrue(TEXT("소분류 Dagger 오버라이드 등록 성공"), GlobalSubsystem->RegisterItemTypeProfile(TagDagger, DaggerProfile));

	// 5. 오버라이드된 소분류 조회 검증 (대분류 1이 아닌 소분류 5 적용)
	const FDISItemTypeProfile& ResolvedDagger = GlobalSubsystem->GetItemTypeProfile(TagDagger);
	TestEqual(TEXT("Dagger는 오버라이드된 MaxStackCount(5)를 반환해야 함"), ResolvedDagger.MaxStackCount, 5);
	TestTrue(TEXT("Dagger는 오버라이드된 DisplayName을 반환해야 함"), ResolvedDagger.DisplayName.EqualTo(DaggerProfile.DisplayName));

	// 6. GetItemTypeTagHierarchy 오름차순(Root -> Leaf) 정렬 검증
	TArray<FGameplayTag> HierarchyTags = GlobalSubsystem->GetItemTypeTagHierarchy(TagSword);
	TestTrue(TEXT("계층 태그 배열은 0개 초과여야 함"), HierarchyTags.Num() > 0);

	// Root 태그의 문자열 길이는 마지막 Leaf 태그보다 짧아야 함
	if (HierarchyTags.Num() >= 2)
	{
		const int32 FirstLen = HierarchyTags[0].GetTagName().GetStringLength();
		const int32 LastLen = HierarchyTags.Last().GetTagName().GetStringLength();
		TestTrue(TEXT("Hierarchy 배열의 첫 번째 태그가 마지막 태그보다 상위(길이가 짧음)여야 함"), FirstLen < LastLen);
		TestEqual(TEXT("Hierarchy 배열의 마지막 요소는 요청한 태그여야 함"), HierarchyTags.Last(), TagSword);
	}

	// 7. MatchesItemCategory 및 MatchesAnyItemCategory 정합성 검증
	TestTrue(TEXT("Sword는 상위 Equipment 카테고리에 매칭되어야 함"), UDISGlobalSubsystem::MatchesItemCategory(TagSword, TagEquipment));
	TestTrue(TEXT("Sword는 상위 Weapon 카테고리에 매칭되어야 함"), UDISGlobalSubsystem::MatchesItemCategory(TagSword, TagWeapon));
	TestFalse(TEXT("Sword는 무관한 Consumable 카테고리에 매칭되지 않아야 함"), UDISGlobalSubsystem::MatchesItemCategory(TagSword, TagConsumable));

	FGameplayTagContainer CategoryContainer;
	CategoryContainer.AddTag(TagConsumable);
	CategoryContainer.AddTag(TagEquipment);
	TestTrue(TEXT("Sword는 [Consumable, Equipment] 컨테이너에 매칭되어야 함"), UDISGlobalSubsystem::MatchesAnyItemCategory(TagSword, CategoryContainer));

	return true;
}

#endif
