#include "Misc/AutomationTest.h"
#include "Global/DISGlobalSubsystem.h"
#include "Global/DISGlobalDefinition.h"
#include "Item/DISItemTypes.h"
#include "GameplayTagsManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * UDISGlobalSubsystem의 희귀도 프로파일(FDISRarityProfile) 등록, 해제, 서열(Rank) 비교 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDISRarityProfileAndComparisonTest,
	"DIS.Global.RarityProfileAndComparison",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDISRarityProfileAndComparisonTest::RunTest(const FString& Parameters)
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

	// 1. 테스트용 GameplayTag 등록
	UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();
	const FGameplayTag TagCommon = TagManager.AddNativeGameplayTag(FName("Test.Rarity.Common"));
	const FGameplayTag TagRare = TagManager.AddNativeGameplayTag(FName("Test.Rarity.Rare"));
	const FGameplayTag TagEpic = TagManager.AddNativeGameplayTag(FName("Test.Rarity.Epic"));
	const FGameplayTag TagUnregistered = TagManager.AddNativeGameplayTag(FName("Test.Rarity.Unregistered"));

	TestTrue(TEXT("TagCommon 유효성 검증"), TagCommon.IsValid());
	TestTrue(TEXT("TagRare 유효성 검증"), TagRare.IsValid());
	TestTrue(TEXT("TagEpic 유효성 검증"), TagEpic.IsValid());

	// 2. 프로파일 데이터 정의
	FDISRarityProfile ProfileCommon;
	ProfileCommon.DisplayName = FText::FromString(TEXT("일반"));
	ProfileCommon.Color = FLinearColor::White;
	ProfileCommon.Rank = 10;

	FDISRarityProfile ProfileRare;
	ProfileRare.DisplayName = FText::FromString(TEXT("희귀"));
	ProfileRare.Color = FLinearColor::Blue;
	ProfileRare.Rank = 30;

	FDISRarityProfile ProfileEpic;
	ProfileEpic.DisplayName = FText::FromString(TEXT("영웅"));
	ProfileEpic.Color = FLinearColor(0.5f, 0.0f, 0.5f, 1.0f);
	ProfileEpic.Rank = 50;

	// 3. 서브시스템에 동적 등록
	TestTrue(TEXT("ProfileCommon 등록 성공"), GlobalSubsystem->RegisterRarityProfile(TagCommon, ProfileCommon));
	TestTrue(TEXT("ProfileRare 등록 성공"), GlobalSubsystem->RegisterRarityProfile(TagRare, ProfileRare));
	TestTrue(TEXT("ProfileEpic 등록 성공"), GlobalSubsystem->RegisterRarityProfile(TagEpic, ProfileEpic));
	TestFalse(TEXT("중복 등록 방지 검증"), GlobalSubsystem->RegisterRarityProfile(TagCommon, ProfileCommon));

	// 4. 서열(Rank) 조회 검증
	TestEqual(TEXT("TagCommon 서열은 10이어야 함"), GlobalSubsystem->GetRarityRank(TagCommon), 10);
	TestEqual(TEXT("TagRare 서열은 30이어야 함"), GlobalSubsystem->GetRarityRank(TagRare), 30);
	TestEqual(TEXT("TagEpic 서열은 50이어야 함"), GlobalSubsystem->GetRarityRank(TagEpic), 50);
	TestEqual(TEXT("미등록 태그 서열은 기본값 0이어야 함"), GlobalSubsystem->GetRarityRank(TagUnregistered), 0);

	// 5. CompareRarity 검증 (TagA > TagB: 1, TagA == TagB: 0, TagA < TagB: -1)
	TestEqual(TEXT("Rare vs Common 비교는 1 (Rare가 상위)"), GlobalSubsystem->CompareRarity(TagRare, TagCommon), 1);
	TestEqual(TEXT("Common vs Rare 비교는 -1 (Common이 하위)"), GlobalSubsystem->CompareRarity(TagCommon, TagRare), -1);
	TestEqual(TEXT("Rare vs Rare 비교는 0 (동일 등급)"), GlobalSubsystem->CompareRarity(TagRare, TagRare), 0);
	TestEqual(TEXT("Epic vs Unregistered 비교는 1 (Epic이 미등록 0보다 상위)"), GlobalSubsystem->CompareRarity(TagEpic, TagUnregistered), 1);

	// 6. UDISGlobalDefinition 에셋을 통한 일괄 등록 및 해제 검증
	UDISGlobalDefinition* GlobalDef = NewObject<UDISGlobalDefinition>();
	TestNotNull(TEXT("UDISGlobalDefinition 생성 검증"), GlobalDef);
	if (GlobalDef != nullptr)
	{
		const FGameplayTag TagLegendary = TagManager.AddNativeGameplayTag(FName("Test.Rarity.Legendary"));
		FDISRarityProfile ProfileLegendary;
		ProfileLegendary.DisplayName = FText::FromString(TEXT("전설"));
		ProfileLegendary.Color = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);
		ProfileLegendary.Rank = 100;

		GlobalDef->RarityProfiles.Add(TagLegendary, ProfileLegendary);

		// 일괄 등록
		TestTrue(TEXT("GlobalDefinition 등록 성공"), GlobalSubsystem->RegisterGlobalDefinition(GlobalDef));
		TestEqual(TEXT("전설 등급 서열 100 검증"), GlobalSubsystem->GetRarityRank(TagLegendary), 100);
		TestEqual(TEXT("전설 vs 영웅 비교는 1 (전설이 상위)"), GlobalSubsystem->CompareRarity(TagLegendary, TagEpic), 1);

		// 일괄 해제
		TestTrue(TEXT("GlobalDefinition 해제 성공"), GlobalSubsystem->UnregisterGlobalDefinition(GlobalDef));
		TestEqual(TEXT("해제 후 전설 등급 서열은 기본값 0이어야 함"), GlobalSubsystem->GetRarityRank(TagLegendary), 0);
	}

	// 8. 개별 해제 검증
	TestTrue(TEXT("ProfileCommon 해제 성공"), GlobalSubsystem->UnregisterRarityProfile(TagCommon));
	TestEqual(TEXT("해제 후 TagCommon 서열은 기본값 0이어야 함"), GlobalSubsystem->GetRarityRank(TagCommon), 0);
	TestFalse(TEXT("이미 해제된 태그 재해제 시 false 반환"), GlobalSubsystem->UnregisterRarityProfile(TagCommon));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
