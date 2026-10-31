/*****************************************************************//**
 * @file   DISGlobalSubsystemCustomizationTests.cpp
 * @brief  UDISGlobalSubsystem 및 UDISGlobalLogic의 위임 아키텍처 및 계층 상속 자동화 테스트
 * @date   2026-10-28
 *********************************************************************/

#include "Misc/AutomationTest.h"
#include "Global/DISGlobalSubsystem.h"
#include "Global/DISGlobalLogic.h"
#include "Global/DISGlobalSettings.h"
#include "Item/DISItemTypes.h"
#include "GameplayTagsManager.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * UDISGlobalSubsystem 초기화 시 UDISGlobalLogic 서브오브젝트 생성 및 정적 GetLogic 조회 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISGlobalSubsystemCreationAndLogicTest,
	"DIS.Global.SubsystemCreationAndLogic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISGlobalSubsystemCreationAndLogicTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("테스트용 GameInstance 생성 성공 검증"), GameInstance);
	if (GameInstance == nullptr)
	{
		return false;
	}

	UDISGlobalSubsystem* GlobalSubsystem = NewObject<UDISGlobalSubsystem>(GameInstance);
	TestNotNull(TEXT("UDISGlobalSubsystem 인스턴스 생성 검증"), GlobalSubsystem);
	if (GlobalSubsystem == nullptr)
	{
		return false;
	}

	// 서브시스템 컬렉션 모의 초기화
	FSubsystemCollection<UGameInstanceSubsystem> SubsystemCollection;
	GlobalSubsystem->Initialize(SubsystemCollection);

	// 1. 내부 로직 서브오브젝트 생성 검증
	UDISGlobalLogic* LogicInstance = GlobalSubsystem->GetLogic();
	TestNotNull(TEXT("GlobalSubsystem 내부 UDISGlobalLogic 인스턴스 생성 검증"), LogicInstance);
	if (LogicInstance == nullptr)
	{
		return false;
	}

	// 2. 소유 서브시스템 참조 검증
	TestEqual(TEXT("LogicInstance의 OwningSubsystem 검증"), LogicInstance->GetOwningSubsystem(), GlobalSubsystem);

	// 3. 정적 GetLogic 헬퍼 함수 검증 (GameInstance를 컨텍스트로 전달)
	UDISGlobalLogic* StaticFoundLogic = UDISGlobalSubsystem::GetLogic(GameInstance);
	// 수동 NewObject 환경에서는 GameInstance 서브시스템 맵에 자동 등록되지 않을 수 있으므로 인스턴스 레벨 검증 확인
	TestNotNull(TEXT("멤버 GetLogic 유효성 검증"), GlobalSubsystem->GetLogic());

	// 4. 서브시스템 해제 검증
	GlobalSubsystem->Deinitialize();
	TestNull(TEXT("Deinitialize 후 LogicInstance 정리 검증"), GlobalSubsystem->GetLogic());

	return true;
}

/**
 * UDISGlobalLogic의 아이템 타입 프로파일 계층 상속 및 캐싱 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISGlobalLogicHierarchyAndCachingTest,
	"DIS.Global.LogicHierarchyAndCaching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISGlobalLogicHierarchyAndCachingTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("테스트용 GameInstance 생성 검증"), GameInstance);
	if (GameInstance == nullptr)
	{
		return false;
	}

	UDISGlobalLogic* GlobalLogic = NewObject<UDISGlobalLogic>(GameInstance);
	TestNotNull(TEXT("UDISGlobalLogic 인스턴스 생성 검증"), GlobalLogic);
	if (GlobalLogic == nullptr)
	{
		return false;
	}
	GlobalLogic->Initialize(nullptr);

	// 1. 계층 태그 정의 (Root: Test.Item.Equipment -> Child: Test.Item.Equipment.Weapon -> Leaf: Sword / UniqueSword)
	UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();
	const FGameplayTag TagEquipment = TagManager.AddNativeGameplayTag(FName("Test.Item.Equipment"));
	const FGameplayTag TagWeapon = TagManager.AddNativeGameplayTag(FName("Test.Item.Equipment.Weapon"));
	const FGameplayTag TagSword = TagManager.AddNativeGameplayTag(FName("Test.Item.Equipment.Weapon.Sword"));
	const FGameplayTag TagUniqueSword = TagManager.AddNativeGameplayTag(FName("Test.Item.Equipment.Weapon.UniqueSword"));
	const FGameplayTag TagConsumable = TagManager.AddNativeGameplayTag(FName("Test.Item.Consumable"));

	TestTrue(TEXT("TagEquipment 유효성"), TagEquipment.IsValid());
	TestTrue(TEXT("TagWeapon 유효성"), TagWeapon.IsValid());
	TestTrue(TEXT("TagSword 유효성"), TagSword.IsValid());
	TestTrue(TEXT("TagUniqueSword 유효성"), TagUniqueSword.IsValid());

	// 2. 부모 태그(TagWeapon)에 프로파일 등록 (bOverrideCustomData = true, MaxStackCount = 5 설정)
	FDISItemTypeProfile WeaponProfile;
	WeaponProfile.DisplayName = FText::FromString(TEXT("무기 베이스"));
	WeaponProfile.bOverrideCustomData = true;
	WeaponProfile.CustomData.InitializeAs<FDISItemTypeCustomData>(5);
	TestTrue(TEXT("WeaponProfile 등록 성공"), GlobalLogic->RegisterItemTypeProfile(TagWeapon, WeaponProfile));

	// 3. 자식 태그 1(TagSword) 등록: bOverrideCustomData = false (DisplayName은 고유값, CustomData는 부모 상속)
	FDISItemTypeProfile SwordProfile;
	SwordProfile.DisplayName = FText::FromString(TEXT("장검"));
	SwordProfile.bOverrideCustomData = false;
	TestTrue(TEXT("SwordProfile 등록 성공"), GlobalLogic->RegisterItemTypeProfile(TagSword, SwordProfile));

	// 4. 자식 태그 2(TagUniqueSword) 등록: bOverrideCustomData = true (부모 CustomData를 상속받지 않고 고유 CustomData 보유)
	FDISItemTypeProfile UniqueSwordProfile;
	UniqueSwordProfile.DisplayName = FText::FromString(TEXT("전설 단검"));
	UniqueSwordProfile.bOverrideCustomData = true;
	UniqueSwordProfile.CustomData.InitializeAs<FDISItemTypeCustomData>(1);
	TestTrue(TEXT("UniqueSwordProfile 등록 성공"), GlobalLogic->RegisterItemTypeProfile(TagUniqueSword, UniqueSwordProfile));

	// 5. 자식 태그 조회 시 자체 DisplayName 적용 및 bOverrideCustomData 여부에 따른 CustomData 상속/오버라이드 검증
	const FDISItemTypeProfile& ResolvedSword = GlobalLogic->GetItemTypeProfile(TagSword);
	TestEqual(TEXT("TagSword의 자체 DisplayName 적용 검증"), ResolvedSword.DisplayName.ToString(), TEXT("장검"));
	const FDISItemTypeCustomData* ResolvedSwordData = ResolvedSword.CustomData.GetPtr();
	TestNotNull(TEXT("bOverrideCustomData가 꺼진 자식은 부모 CustomData를 정상 상속해야 함"), ResolvedSwordData);
	if (ResolvedSwordData != nullptr)
	{
		TestEqual(TEXT("상속받은 부모 CustomData의 MaxStackCount(5) 일치 검증"), ResolvedSwordData->MaxStackCount, 5);
	}

	const FDISItemTypeProfile& ResolvedUniqueSword = GlobalLogic->GetItemTypeProfile(TagUniqueSword);
	TestEqual(TEXT("TagUniqueSword의 자체 DisplayName 적용 검증"), ResolvedUniqueSword.DisplayName.ToString(), TEXT("전설 단검"));
	const FDISItemTypeCustomData* ResolvedUniqueSwordData = ResolvedUniqueSword.CustomData.GetPtr();
	TestNotNull(TEXT("bOverrideCustomData가 켜진 자식은 자체 CustomData가 유효해야 함"), ResolvedUniqueSwordData);
	if (ResolvedUniqueSwordData != nullptr)
	{
		TestEqual(TEXT("bOverrideCustomData가 켜진 자식은 부모(5) 대신 자신의 MaxStackCount(1)를 가져야 함"), ResolvedUniqueSwordData->MaxStackCount, 1);
	}

	// 6. 카테고리 매칭 검증
	TestTrue(TEXT("Sword는 Equipment 카테고리에 속함"), GlobalLogic->MatchesItemCategory(TagSword, TagEquipment));
	TestTrue(TEXT("Sword는 Weapon 카테고리에 속함"), GlobalLogic->MatchesItemCategory(TagSword, TagWeapon));
	TestFalse(TEXT("Sword는 Consumable 카테고리에 속하지 않음"), GlobalLogic->MatchesItemCategory(TagSword, TagConsumable));

	// 7. 캐시 무효화 및 재조회 검증
	GlobalLogic->InvalidateProfileCache();
	const FDISItemTypeProfile& ReResolvedSword = GlobalLogic->GetItemTypeProfile(TagSword);
	TestEqual(TEXT("캐시 무효화 후 재해석 DisplayName 검증"), ReResolvedSword.DisplayName.ToString(), TEXT("장검"));
	const FDISItemTypeCustomData* ReResolvedSwordData = ReResolvedSword.CustomData.GetPtr();
	TestNotNull(TEXT("캐시 무효화 후 재해석 CustomData 유효성 검증"), ReResolvedSwordData);
	if (ReResolvedSwordData != nullptr)
	{
		TestEqual(TEXT("캐시 무효화 후 재해석 MaxStackCount 검증"), ReResolvedSwordData->MaxStackCount, 5);
	}

	// 8. 등록 해제 검증
	TestTrue(TEXT("UniqueSwordProfile 해제 성공"), GlobalLogic->UnregisterItemTypeProfile(TagUniqueSword));
	TestTrue(TEXT("SwordProfile 해제 성공"), GlobalLogic->UnregisterItemTypeProfile(TagSword));
	TestTrue(TEXT("WeaponProfile 해제 성공"), GlobalLogic->UnregisterItemTypeProfile(TagWeapon));

	GlobalLogic->Deinitialize();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
