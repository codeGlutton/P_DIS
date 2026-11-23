/*****************************************************************//**
 * @file   BBMItemGroupTests.cpp
 * @brief  BBM(비트 배낭 상인) 아이템 그룹 프로파일 및 정의 연동 자동화 테스트
 * @date   2026-10-28
 *********************************************************************/

#include "Misc/AutomationTest.h"
#include "Global/DISGlobalSubsystem.h"
#include "Global/DISGlobalSettings.h"
#include "Global/BBMGlobalLogic.h"
#include "Global/BBMGlobalDefinition.h"
#include "Item/BBMItemDefinition.h"
#include "Item/BBMItemGroupTypes.h"
#include "GameplayTagsManager.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * BBM 아이템 그룹 프로파일 등록, 해제, 조회 및 UBBMItemDefinition 복수 그룹 태그 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMItemGroupProfileAndDefinitionTest,
	"DIS.BBM.ItemGroupProfileAndDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMItemGroupProfileAndDefinitionTest::RunTest(const FString& Parameters)
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

	// 1. 테스트용 그룹 태그 등록
	UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();
	const FGameplayTag TagNoble = TagManager.AddNativeGameplayTag(FName("Test.Group.Noble"));
	const FGameplayTag TagUndead = TagManager.AddNativeGameplayTag(FName("Test.Group.Undead"));
	const FGameplayTag TagMerchant = TagManager.AddNativeGameplayTag(FName("Test.Group.Merchant"));

	TestTrue(TEXT("TagNoble 유효성"), TagNoble.IsValid());
	TestTrue(TEXT("TagUndead 유효성"), TagUndead.IsValid());
	TestTrue(TEXT("TagMerchant 유효성"), TagMerchant.IsValid());

	// 2. 개별 그룹 프로파일 등록 검증
	FBBMItemGroupProfile NobleProfile;
	NobleProfile.DisplayName = FText::FromString(TEXT("귀족"));

	FBBMItemGroupProfile UndeadProfile;
	UndeadProfile.DisplayName = FText::FromString(TEXT("언데드"));

	TestTrue(TEXT("NobleProfile 등록 성공"), BBMLogic->RegisterItemGroupProfile(TagNoble, NobleProfile));
	TestTrue(TEXT("UndeadProfile 등록 성공"), BBMLogic->RegisterItemGroupProfile(TagUndead, UndeadProfile));
	TestFalse(TEXT("중복 등록 방지 검증"), BBMLogic->RegisterItemGroupProfile(TagNoble, NobleProfile));

	// 3. 프로파일 조회 검증
	TestEqual(TEXT("귀족 프로파일 이름 검증"), BBMLogic->GetItemGroupProfile(TagNoble).DisplayName.ToString(), TEXT("귀족"));
	TestEqual(TEXT("언데드 프로파일 이름 검증"), BBMLogic->GetItemGroupProfile(TagUndead).DisplayName.ToString(), TEXT("언데드"));
	TestTrue(TEXT("Noble 등록 여부 확인"), BBMLogic->HasItemGroupProfile(TagNoble));
	TestFalse(TEXT("미등록 Merchant 등록 여부 확인"), BBMLogic->HasItemGroupProfile(TagMerchant));

	// 4. UBBMItemDefinition 복수 그룹 태그 소속 검증
	UBBMItemDefinition* ItemDef = NewObject<UBBMItemDefinition>();
	TestNotNull(TEXT("UBBMItemDefinition 생성 검증"), ItemDef);
	if (ItemDef != nullptr)
	{
		ItemDef->ItemGroupTags.AddTag(TagNoble);
		ItemDef->ItemGroupTags.AddTag(TagUndead);

		TestTrue(TEXT("아이템은 귀족 그룹에 속함"), ItemDef->HasItemGroup(TagNoble));
		TestTrue(TEXT("아이템은 언데드 그룹에 속함"), ItemDef->HasItemGroup(TagUndead));
		TestFalse(TEXT("아이템은 상인 그룹에 속하지 않음"), ItemDef->HasItemGroup(TagMerchant));
		TestEqual(TEXT("소속 그룹 수 2개 검증"), ItemDef->GetItemGroupTags().Num(), 2);
	}

	// 5. UBBMGlobalDefinition 에셋을 통한 일괄 등록 및 해제 검증
	UBBMGlobalDefinition* BBMDef = NewObject<UBBMGlobalDefinition>();
	TestNotNull(TEXT("UBBMGlobalDefinition 생성 검증"), BBMDef);
	if (BBMDef != nullptr)
	{
		FBBMItemGroupProfile MerchantProfile;
		MerchantProfile.DisplayName = FText::FromString(TEXT("상인"));
		BBMDef->ItemGroupProfiles.Add(TagMerchant, MerchantProfile);

		// 일괄 등록
		TestTrue(TEXT("UBBMGlobalDefinition 등록 성공"), BBMLogic->RegisterBBMGlobalDefinition(BBMDef));
		TestTrue(TEXT("등록 후 Merchant 확인"), BBMLogic->HasItemGroupProfile(TagMerchant));
		TestEqual(TEXT("상인 프로파일 이름 검증"), BBMLogic->GetItemGroupProfile(TagMerchant).DisplayName.ToString(), TEXT("상인"));

		// 일괄 해제
		TestTrue(TEXT("UBBMGlobalDefinition 해제 성공"), BBMLogic->UnregisterBBMGlobalDefinition(BBMDef));
		TestFalse(TEXT("해제 후 Merchant 확인"), BBMLogic->HasItemGroupProfile(TagMerchant));
	}

	// 6. 개별 해제 검증
	TestTrue(TEXT("NobleProfile 해제 성공"), BBMLogic->UnregisterItemGroupProfile(TagNoble));
	TestFalse(TEXT("해제 후 Noble 확인"), BBMLogic->HasItemGroupProfile(TagNoble));

	BBMLogic->Deinitialize();
	return true;
}

/**
 * DefaultGame.ini 설정을 통한 UBBMGlobalLogic 클래스 자동 매핑 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMGlobalLogicConfigTest,
	"DIS.BBM.GlobalLogicConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMGlobalLogicConfigTest::RunTest(const FString& Parameters)
{
	const UDISGlobalSettings* Settings = GetDefault<UDISGlobalSettings>();
	TestNotNull(TEXT("UDISGlobalSettings CDO 획득 검증"), Settings);
	if (Settings == nullptr)
	{
		return false;
	}

	UClass* TargetLogicClass = Settings->GetGlobalLogicClass();
	TestNotNull(TEXT("GetGlobalLogicClass 유효성 검증"), TargetLogicClass);
	TestEqual(TEXT("Config에 설정된 로직 클래스는 UBBMGlobalLogic이어야 함"), TargetLogicClass, UBBMGlobalLogic::StaticClass());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
