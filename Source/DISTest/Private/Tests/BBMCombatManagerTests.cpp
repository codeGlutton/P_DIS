/*****************************************************************//**
 * @file   BBMCombatManagerTests.cpp
 * @brief  BBM(비트 배낭 상인) 전투 핵심 및 1D 타일 라인 전투 매니저 종합 단위 테스트
 * @details 스탯 커브테이블, 아이템 비트 GE 수명주기, 팀 태도, AI/플레이어 비트 시퀀스,
 *          1D 타일 라인 진형/시프트, 3단계 방어 및 상태이상 틱 총괄 검증
 * @date   2026-11-04
 *********************************************************************/

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Combat/BBMCombatManagerSubsystem.h"
#include "Combat/BBM1DTileMapActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Combat/BBMCombatTypes.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "Combat/GAS/BBMAbilitySystemGlobals.h"
#include "Combat/GAS/BBMGameplayEffects.h"
#include "Combat/Pawn/BBMCombatantBase.h"
#include "Combat/Pawn/BBMPlayerMerchantBase.h"
#include "Combat/Pawn/BBMAICombatantBase.h"
#include "Combat/Pawn/BBMCharacterDefinition.h"
#include "Combat/Pawn/BBMAICombatantDefinition.h"
#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Combat/Team/BBMTeamTypes.h"
#include "Combat/Team/BBMCombatSettings.h"
#include "Inventory/BBMInventoryComponent.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Inventory/BBMInventory.h"
#include "Item/DISItemDefinition.h"
#include "Item/DISItemEntry.h"
#include "Item/DISItemStack.h"
#include "Item/BBMItemDefinition.h"
#include "Item/BBMItemEntry.h"
#include "Item/BBMItemGameplayEffectTypes.h"
#include "Item/BBMBeatTypes.h"
#include "Item/BBMItemProfileTypes.h"
#include "Stage/BBMEncounterDefinition.h"
#include "Stage/BBMStageTypes.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/Animation/BBMCombatantAnimInstance.h"
#include "GameFeatures/GameFeatureAction_AddBBMGlobalDefinition.h"
#include "Engine/CompositeCurveTable.h"
#include "Engine/CurveTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayTagsManager.h"
#include "Curves/RichCurve.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * GAS 액터 스폰 및 안전한 테스트 월드 생명주기 관리 스코프 구조체
 */
struct FBBMScopedTestWorld
{
	UWorld* World = nullptr;

	FBBMScopedTestWorld()
	{
		UAbilitySystemGlobals::Get().InitGlobalData();
		World = UWorld::CreateWorld(EWorldType::Game, false);
		if (World != nullptr)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			FURL URL;
			World->InitializeActorsForPlay(URL);
		}
	}

	~FBBMScopedTestWorld()
	{
		if (World != nullptr)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

	ABBMCombatantBase* SpawnCombatant()
	{
		if (World == nullptr)
		{
			return nullptr;
		}
		return World->SpawnActor<ABBMCombatantBase>();
	}
};

// 1. CompositeCurveTable을 통한 레벨별 어트리뷰트 스탯 초기화 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatantCompositeCurveTableTest,
	"DIS.BBM.Combat.CompositeCurveTableStats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMCombatantCompositeCurveTableTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestNotNull(TEXT("테스트 월드 생성 검증"), TestWorld.World);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. 기본 스탯 CurveTable 생성 및 RichCurve 행 추가
	UCurveTable* BaseStatsTable = NewObject<UCurveTable>();
	TestNotNull(TEXT("BaseStatsTable 생성 검증"), BaseStatsTable);
	if (BaseStatsTable == nullptr)
	{
		return false;
	}

	FRichCurve& HealthCurve = BaseStatsTable->AddRichCurve(FName(TEXT("Player.BBMAttributeSet.Health")));
	HealthCurve.AddKey(1.0f, 150.0f);
	HealthCurve.AddKey(2.0f, 250.0f);

	FRichCurve& MaxHealthCurve = BaseStatsTable->AddRichCurve(FName(TEXT("Player.BBMAttributeSet.MaxHealth")));
	MaxHealthCurve.AddKey(1.0f, 150.0f);
	MaxHealthCurve.AddKey(2.0f, 250.0f);

	FRichCurve& AttackCurve = BaseStatsTable->AddRichCurve(FName(TEXT("Player.BBMAttributeSet.AttackPower")));
	AttackCurve.AddKey(1.0f, 30.0f);
	AttackCurve.AddKey(2.0f, 50.0f);

	FRichCurve& DefenseCurve = BaseStatsTable->AddRichCurve(FName(TEXT("Player.BBMAttributeSet.DefensePower")));
	DefenseCurve.AddKey(1.0f, 15.0f);
	DefenseCurve.AddKey(2.0f, 25.0f);

	FRichCurve& ArmorCurve = BaseStatsTable->AddRichCurve(FName(TEXT("Player.BBMAttributeSet.Armor")));
	ArmorCurve.AddKey(1.0f, 20.0f);
	ArmorCurve.AddKey(2.0f, 40.0f);

	// 2. CompositeCurveTable 생성 및 BaseStatsTable을 부모로 등록
	UCompositeCurveTable* CompTable = NewObject<UCompositeCurveTable>();
	TestNotNull(TEXT("CompositeCurveTable 생성 검증"), CompTable);
	if (CompTable == nullptr)
	{
		return false;
	}
	CompTable->AppendParentTables({ BaseStatsTable });

	// 3. 전역 BBMAbilitySystemGlobals의 Initter에 CompositeCurveTable 데이터 주입
	UBBMAbilitySystemGlobals* BBMGlobals = Cast<UBBMAbilitySystemGlobals>(&UAbilitySystemGlobals::Get());
	if (BBMGlobals != nullptr)
	{
		BBMGlobals->AddAttributeDefaultTableInstance(CompTable);
	}
	else
	{
		FAttributeSetInitter* GlobalInitter = UAbilitySystemGlobals::Get().GetAttributeSetInitter();
		TestNotNull(TEXT("전역 Initter 유효성 검증"), GlobalInitter);
		if (GlobalInitter == nullptr)
		{
			return false;
		}
		GlobalInitter->PreloadAttributeSetData({ CompTable });
	}

	// 4. 전투원 폰 스폰 및 캐릭터 정의(CharacterDefinition) 설정
	ABBMCombatantBase* Combatant = TestWorld.SpawnCombatant();
	TestNotNull(TEXT("ABBMCombatantBase 스폰 검증"), Combatant);
	if (Combatant == nullptr)
	{
		return false;
	}

	UBBMCharacterDefinition* CharDef = NewObject<UBBMCharacterDefinition>();
	TestNotNull(TEXT("UBBMCharacterDefinition 생성 검증"), CharDef);
	if (CharDef == nullptr)
	{
		return false;
	}
	CharDef->TableKeyName = FName(TEXT("Player"));
	CharDef->DisplayName = FText::FromString(TEXT("테스트 주인공"));
	CharDef->DefaultTeam = EBBMTeamType::Player;

	// PrimaryAssetId 검증
	const FPrimaryAssetId AssetId = CharDef->GetPrimaryAssetId();
	TestEqual(TEXT("PrimaryAssetType은 BBMCharacter여야 함"), AssetId.PrimaryAssetType, FPrimaryAssetType(BBMPrimaryAssetTypes::Character));
	TestEqual(TEXT("PrimaryAssetName은 CharDef 이름이어야 함"), AssetId.PrimaryAssetName, CharDef->GetFName());

	Combatant->SetCharacterDefinition(CharDef);

	TestEqual(TEXT("CharacterDefinition 설정 후 기본 팀 동기화 검증"), Combatant->GetTeamType(), EBBMTeamType::Player);

	UAbilitySystemComponent* ASC = Combatant->GetAbilitySystemComponent();
	TestNotNull(TEXT("전투원 ASC 검증"), ASC);
	if (ASC == nullptr)
	{
		return false;
	}

	// 5. 레벨 1 스탯 주입 검증 (CharacterDefinition의 TableKeyName 활용)
	Combatant->InitializeStatsForLevel(1);
	TestEqual(TEXT("레벨 1 Health 초기화 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetHealthAttribute()), 150.0f);
	TestEqual(TEXT("레벨 1 MaxHealth 초기화 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetMaxHealthAttribute()), 150.0f);
	TestEqual(TEXT("레벨 1 AttackPower 초기화 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetAttackPowerAttribute()), 30.0f);
	TestEqual(TEXT("레벨 1 DefensePower 초기화 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetDefensePowerAttribute()), 15.0f);
	TestEqual(TEXT("레벨 1 Armor 초기화 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetArmorAttribute()), 20.0f);

	// 6. 레벨 2 스탯 갱신 주입 검증 (레벨 변동 시 스탯 정상 갱신)
	Combatant->InitializeStatsForLevel(2);
	TestEqual(TEXT("레벨 2 Health 갱신 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetHealthAttribute()), 250.0f);
	TestEqual(TEXT("레벨 2 MaxHealth 갱신 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetMaxHealthAttribute()), 250.0f);
	TestEqual(TEXT("레벨 2 AttackPower 갱신 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetAttackPowerAttribute()), 50.0f);
	TestEqual(TEXT("레벨 2 DefensePower 갱신 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetDefensePowerAttribute()), 25.0f);
	TestEqual(TEXT("레벨 2 Armor 갱신 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetArmorAttribute()), 40.0f);

	return true;
}

// 2. 비트 단위 아이템 GAS GameplayEffect 부여 및 종료 시 100% 원복 검증 (UBBMCombatManagerSubsystem 기반)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatItemEffectLifecycleTest,
	"DIS.BBM.Combat.ItemEffectLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMCombatItemEffectLifecycleTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestNotNull(TEXT("테스트 월드 생성 검증"), TestWorld.World);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. 전투원 스폰 및 기본 공격력(10.0f) 확인
	ABBMCombatantBase* Combatant = TestWorld.SpawnCombatant();
	TestNotNull(TEXT("전투원 스폰 검증"), Combatant);
	if (Combatant == nullptr)
	{
		return false;
	}

	const float InitialAttack = Combatant->GetAttributeValue(UBBMAttributeSet::GetAttackPowerAttribute());
	TestEqual(TEXT("전투원 초기 기본 공격력은 10.0f여야 함"), InitialAttack, 10.0f);

	// 2. 런타임 아이템 엔트리 생성 및 공격력 버프(+25.0f) 설정
	UBBMItemEntry* ItemEntry = NewObject<UBBMItemEntry>();
	TestNotNull(TEXT("UBBMItemEntry 생성 검증"), ItemEntry);
	if (ItemEntry == nullptr)
	{
		return false;
	}

	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("BBM.Combat.AttackPower"));
	FBBMItemGameplayEffectConfig BuffConfig(UBBMItemStatGameplayEffect::StaticClass());
	BuffConfig.SetByCallerMagnitudes.Add(AttackTag, 25.0f);

	FBBMBeatEffectRange BuffRange(0, 2);
	BuffRange.GrantedEffects.Add(BuffConfig);
	ItemEntry->AddEffectRange(BuffRange);

	// 3. 비트 시작: 아이템 효과 캐릭터 ASC에 적용 (UBBMCombatManagerSubsystem 헬퍼 활용)
	TArray<FActiveGameplayEffectHandle> ActiveHandles;
	const bool bApplyResult = UBBMCombatManagerSubsystem::ApplyItemGrantedEffects(Combatant, ItemEntry, ActiveHandles);
	TestTrue(TEXT("아이템 Granted Effects 적용 성공"), bApplyResult);
	TestEqual(TEXT("적용된 Active Handle 개수는 1개여야 함"), ActiveHandles.Num(), 1);

	// 버프 적용 후 공격력 검증 (10.0f + 25.0f = 35.0f)
	TestEqual(TEXT("아이템 효과 부여 후 공격력 증폭 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetAttackPowerAttribute()), 35.0f);

	// 4. 비트 종료: 핸들을 통해 활성 효과 일괄 해제
	UBBMCombatManagerSubsystem::RemoveItemGrantedEffects(Combatant, ActiveHandles);

	// 해제 후 원래 공격력(10.0f)으로 100% 원복 검증
	TestEqual(TEXT("비트 종료 후 공격력 완전 원복 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetAttackPowerAttribute()), InitialAttack);

	return true;
}

// 3. FGenericTeamId 및 IGenericTeamAgentInterface 팀 관계 태도 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatantTeamAttitudeTest,
	"DIS.BBM.Combat.TeamAttitude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMCombatantTeamAttitudeTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestNotNull(TEXT("테스트 월드 생성 검증"), TestWorld.World);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. 플레이어 1 및 플레이어 2 스폰
	ABBMCombatantBase* Player1 = TestWorld.SpawnCombatant();
	ABBMCombatantBase* Player2 = TestWorld.SpawnCombatant();
	ABBMCombatantBase* Monster = TestWorld.SpawnCombatant();
	ABBMCombatantBase* Neutral = TestWorld.SpawnCombatant();

	TestNotNull(TEXT("Player1 스폰 검증"), Player1);
	TestNotNull(TEXT("Player2 스폰 검증"), Player2);
	TestNotNull(TEXT("Monster 스폰 검증"), Monster);
	TestNotNull(TEXT("Neutral 스폰 검증"), Neutral);
	if (Player1 == nullptr || Player2 == nullptr || Monster == nullptr || Neutral == nullptr)
	{
		return false;
	}

	// 2. 팀 ID 할당
	Player1->SetTeamType(EBBMTeamType::Player);
	Player2->SetTeamType(EBBMTeamType::Player);
	Monster->SetTeamType(EBBMTeamType::Monster);
	Neutral->SetTeamType(EBBMTeamType::Neutral);

	// 3. 플레이어 vs 플레이어: 우호(Friendly) 검증
	TestEqual(TEXT("Player1 -> Player2 태도는 Friendly여야 함"), Player1->GetTeamAttitudeTowards(*Player2), ETeamAttitude::Friendly);
	TestTrue(TEXT("Player1은 Player2에게 우호적이어야 함"), Player1->IsFriendlyTowards(Player2));
	TestFalse(TEXT("Player1은 Player2에게 적대적이지 않아야 함"), Player1->IsHostileTowards(Player2));

	// 4. 플레이어 vs 몬스터: 적대(Hostile) 검증
	TestEqual(TEXT("Player1 -> Monster 태도는 Hostile이어야 함"), Player1->GetTeamAttitudeTowards(*Monster), ETeamAttitude::Hostile);
	TestTrue(TEXT("Player1은 Monster에게 적대적이어야 함"), Player1->IsHostileTowards(Monster));
	TestFalse(TEXT("Player1은 Monster에게 우호적이지 않아야 함"), Player1->IsFriendlyTowards(Monster));

	// 5. 몬스터 vs 플레이어: 적대(Hostile) 상호 검증
	TestEqual(TEXT("Monster -> Player1 태도는 Hostile이어야 함"), Monster->GetTeamAttitudeTowards(*Player1), ETeamAttitude::Hostile);
	TestTrue(TEXT("Monster는 Player1에게 적대적이어야 함"), Monster->IsHostileTowards(Player1));

	// 6. 플레이어 vs 중립: 중립(Neutral) 검증
	TestEqual(TEXT("Player1 -> Neutral 태도는 Neutral이어야 함"), Player1->GetTeamAttitudeTowards(*Neutral), ETeamAttitude::Neutral);
	TestFalse(TEXT("중립은 우호적이지 않음"), Player1->IsFriendlyTowards(Neutral));
	TestFalse(TEXT("중립은 적대적이지 않음"), Player1->IsHostileTowards(Neutral));

	return true;
}

// 4. GameFeatureAction_AddBBMGlobalDefinition을 통한 AbilityDefaultTableNames 등록 및 해제 프로퍼티 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMGameFeatureActionAttributeDefaultTablesTest,
	"DIS.BBM.Combat.GameFeatureActionAttributeDefaultTables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMGameFeatureActionAttributeDefaultTablesTest::RunTest(const FString& Parameters)
{
	UGameFeatureAction_AddBBMGlobalDefinition* Action = NewObject<UGameFeatureAction_AddBBMGlobalDefinition>();
	TestNotNull(TEXT("UGameFeatureAction_AddBBMGlobalDefinition 생성 검증"), Action);
	if (Action == nullptr)
	{
		return false;
	}

	// AttributeDefaultTableNames 프로퍼티 등록 검증
	Action->AttributeDefaultTableNames.Add(FSoftObjectPath(TEXT("/Game/Test/TestCurveTable.TestCurveTable")));
	TestEqual(TEXT("AttributeDefaultTableNames 원소 추가 검증"), Action->AttributeDefaultTableNames.Num(), 1);

	return true;
}

// 5. FBBMTeamRelation 누락된 팀 일괄 자동 생성 및 멱등성 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMTeamRelationFillMissingTeamsTest,
	"DIS.BBM.Combat.TeamRelationFillMissingTeams",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMTeamRelationFillMissingTeamsTest::RunTest(const FString& Parameters)
{
	FBBMTeamRelation Relation;
	TestEqual(TEXT("초기 태도 목록은 비어있어야 함"), Relation.Attitudes.Num(), 0);

	// Player 팀 기준 누락된 팀 일괄 생성 호출
	const int32 AddedCount = Relation.FillMissingTeams(EBBMTeamType::Player);

	// 1. 추가된 개수 및 총 목록 수 검증 (Player, Monster, Neutral 총 3개)
	TestEqual(TEXT("3개의 팀(Neutral, Player, Monster)이 추가되어야 함"), AddedCount, 3);
	TestEqual(TEXT("Attitudes 원소 수는 3이어야 함"), Relation.Attitudes.Num(), 3);

	// 2. 주체 팀(Player)과 일치하면 Friendly, 타 팀은 Neutral 기본값 할당 검증
	TestEqual(TEXT("Player는 ContextTeam이므로 Friendly여야 함"), Relation.Attitudes.FindRef(EBBMTeamType::Player), TEnumAsByte<ETeamAttitude::Type>(ETeamAttitude::Friendly));
	TestEqual(TEXT("Monster는 Neutral이어야 함"), Relation.Attitudes.FindRef(EBBMTeamType::Monster), TEnumAsByte<ETeamAttitude::Type>(ETeamAttitude::Neutral));
	TestEqual(TEXT("Neutral은 Neutral이어야 함"), Relation.Attitudes.FindRef(EBBMTeamType::Neutral), TEnumAsByte<ETeamAttitude::Type>(ETeamAttitude::Neutral));

	// 3. 특정 팀 태도 수동 변경 후 다시 호출했을 때 기존 값이 보존되는지(Idempotent) 검증
	Relation.Attitudes[EBBMTeamType::Monster] = ETeamAttitude::Hostile;
	const int32 SecondAddedCount = Relation.FillMissingTeams(EBBMTeamType::Player);

	TestEqual(TEXT("이미 모든 팀이 등록되어 있으므로 추가된 개수는 0이어야 함"), SecondAddedCount, 0);
	TestEqual(TEXT("기존에 설정한 Hostile 태도가 보존되어야 함"), Relation.Attitudes.FindRef(EBBMTeamType::Monster), TEnumAsByte<ETeamAttitude::Type>(ETeamAttitude::Hostile));

	return true;
}

// 6. BBMCombatantBase 파생 객체(PlayerMerchant, AICombatant) 및 소환수/몬스터 팀 동기화 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatantSubclassesTest,
	"DIS.BBM.Combat.CombatantSubclasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMCombatantSubclassesTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestNotNull(TEXT("테스트 월드 생성 검증"), TestWorld.World);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. 플레이어 상인 스폰 및 기본 팀(Player) 검증
	ABBMPlayerMerchantBase* Merchant = TestWorld.World->SpawnActor<ABBMPlayerMerchantBase>();
	TestNotNull(TEXT("ABBMPlayerMerchantBase 스폰 검증"), Merchant);
	if (Merchant != nullptr)
	{
		TestEqual(TEXT("플레이어 상인의 기본 팀은 Player여야 함"), Merchant->GetTeamType(), EBBMTeamType::Player);
		TestNotNull(TEXT("플레이어 상인의 ASC 검증"), Merchant->GetAbilitySystemComponent());
		TestNotNull(TEXT("플레이어 상인의 AttributeSet 검증"), Merchant->GetAttributeSet());
	}

	// 2. 적군 AI 몬스터 스폰 및 팀(Monster) 검증
	ABBMAICombatantBase* Monster = TestWorld.World->SpawnActor<ABBMAICombatantBase>();
	TestNotNull(TEXT("ABBMAICombatantBase(적군 몬스터) 스폰 검증"), Monster);
	if (Monster != nullptr)
	{
		TestEqual(TEXT("몬스터의 기본 팀은 Monster여야 함"), Monster->GetTeamType(), EBBMTeamType::Monster);
		TestNotNull(TEXT("몬스터의 ASC 검증"), Monster->GetAbilitySystemComponent());
		TestNotNull(TEXT("몬스터의 AttributeSet 검증"), Monster->GetAttributeSet());
	}

	// 3. 아군 AI 동료 소환수 스폰 및 Definition 통한 Player 팀 동기화 검증
	ABBMAICombatantBase* Companion = TestWorld.World->SpawnActor<ABBMAICombatantBase>();
	TestNotNull(TEXT("ABBMAICombatantBase(아군 동료) 스폰 검증"), Companion);
	if (Companion != nullptr)
	{
		UBBMCharacterDefinition* CompanionDef = NewObject<UBBMCharacterDefinition>();
		CompanionDef->DisplayName = FText::FromString(TEXT("동료 슬라임"));
		CompanionDef->DefaultTeam = EBBMTeamType::Player;
		Companion->SetCharacterDefinition(CompanionDef);

		TestEqual(TEXT("소환수 Definition 주입 후 팀이 Player로 자동 동기화되어야 함"), Companion->GetTeamType(), EBBMTeamType::Player);
	}

	// 4. 상호 태도 검증: 플레이어 vs 몬스터 -> Hostile / 플레이어 vs 동료 소환수 -> Friendly
	if (Merchant != nullptr && Monster != nullptr)
	{
		TestEqual(TEXT("Merchant -> Monster 태도는 Hostile이어야 함"), Merchant->GetTeamAttitudeTowards(*Monster), ETeamAttitude::Hostile);
		TestEqual(TEXT("Monster -> Merchant 태도는 Hostile이어야 함"), Monster->GetTeamAttitudeTowards(*Merchant), ETeamAttitude::Hostile);
	}

	if (Merchant != nullptr && Companion != nullptr)
	{
		TestEqual(TEXT("Merchant -> Companion 태도는 Friendly여야 함"), Merchant->GetTeamAttitudeTowards(*Companion), ETeamAttitude::Friendly);
		TestEqual(TEXT("Companion -> Merchant 태도는 Friendly여야 함"), Companion->GetTeamAttitudeTowards(*Merchant), ETeamAttitude::Friendly);
	}

	return true;
}

// 7. UBBMAICombatantDefinition 8비트 패턴 풀 및 FRandomStream 기반 가중치/연속방지 추첨 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMAICombatantDefinitionPatternTest,
	"DIS.BBM.Combat.AICombatantDefinitionPattern",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMAICombatantDefinitionPatternTest::RunTest(const FString& Parameters)
{
	UBBMAICombatantDefinition* AIDef = NewObject<UBBMAICombatantDefinition>();
	TestNotNull(TEXT("UBBMAICombatantDefinition 생성 검증"), AIDef);
	if (AIDef == nullptr)
	{
		return false;
	}

	// 1. 기본 식별자 및 팀 검증
	const FPrimaryAssetId AssetId = AIDef->GetPrimaryAssetId();
	TestEqual(TEXT("PrimaryAssetType은 BBMAICombatant여야 함"), AssetId.PrimaryAssetType, FPrimaryAssetType(BBMPrimaryAssetTypes::AICombatant));
	TestEqual(TEXT("기본 팀은 Monster여야 함"), AIDef->GetDefaultTeam(), EBBMTeamType::Monster);

	// 2. 패턴 풀 구성: PatternA (가중치 0, 선택 불가), PatternB (가중치 10, 연속불가), PatternC (가중치 90, 연속가능)
	FBBMAIBeatSequence PatternA;
	PatternA.PatternName = FName(TEXT("PatternA_Zero"));
	PatternA.SelectionWeight = 0.0f;
	PatternA.Sequence.Beats.SetNum(8);

	FBBMAIBeatSequence PatternB;
	PatternB.PatternName = FName(TEXT("PatternB_Rare"));
	PatternB.SelectionWeight = 10.0f;
	PatternB.bCanRepeatConsecutively = false;
	PatternB.Sequence.Beats.SetNum(8);

	// PatternC는 비트 2~5 구간에 활성화되는 EffectRange를 포함하도록 설정
	FBBMAIBeatSequence PatternC;
	PatternC.PatternName = FName(TEXT("PatternC_Common"));
	PatternC.SelectionWeight = 90.0f;
	PatternC.bCanRepeatConsecutively = true;
	PatternC.Sequence.Beats.SetNum(8);
	PatternC.Sequence.EffectRanges.Add(FBBMBeatEffectRange(2, 5));

	AIDef->PatternPool.Add(PatternA);
	AIDef->PatternPool.Add(PatternB);
	AIDef->PatternPool.Add(PatternC);

	// 3. FRandomStream 기반 추첨 검증
	FRandomStream Stream(42);
	const FBBMAIBeatSequence* Selected = AIDef->SelectPattern(Stream, NAME_None);
	TestNotNull(TEXT("패턴 선택 결과 검증"), Selected);
	if (Selected != nullptr)
	{
		TestNotEqual(TEXT("가중치 0인 PatternA는 절대 선택되지 않아야 함"), Selected->PatternName, PatternA.PatternName);
	}

	// 4. 연속 방지 검증: LastPatternName을 PatternB로 전달 시, bCanRepeatConsecutively = false이므로 PatternC만 당첨되어야 함
	for (int32 i = 0; i < 10; ++i)
	{
		const FBBMAIBeatSequence* NoRepeatSelected = AIDef->SelectPattern(Stream, FName(TEXT("PatternB_Rare")));
		TestNotNull(TEXT("연속 방지 추첨 결과 검증"), NoRepeatSelected);
		if (NoRepeatSelected != nullptr)
		{
			TestEqual(TEXT("PatternB는 연속 사용이 금지되어 있으므로 반드시 PatternC가 나와야 함"), NoRepeatSelected->PatternName, PatternC.PatternName);
		}
	}

	// 5. FBBMBeatEffectRange 및 FBBMBeatSequence 컴포지션 단위 검증
	{
		const FBBMBeatEffectRange Range(2, 5);
		TestTrue(TEXT("0 <= 2 < 5 <= 8 범위는 IsValid() == true 여야 함"), Range.IsValid());

		// IsActiveAtBeat [2, 5)
		TestFalse(TEXT("Beat 1은 범위 밖"), Range.IsActiveAtBeat(1));
		TestTrue(TEXT("Beat 2는 활성"), Range.IsActiveAtBeat(2));
		TestTrue(TEXT("Beat 4는 활성"), Range.IsActiveAtBeat(4));
		TestFalse(TEXT("Beat 5는 범위 밖 (Exclusive)"), Range.IsActiveAtBeat(5));

		// IsStartingAtBeat (Beat 2에서 부여 시작)
		TestFalse(TEXT("Beat 1은 시작 비트 아님"), Range.IsStartingAtBeat(1));
		TestTrue(TEXT("Beat 2는 시작 비트"), Range.IsStartingAtBeat(2));
		TestFalse(TEXT("Beat 3은 시작 비트 아님"), Range.IsStartingAtBeat(3));

		// IsEndingAtBeat (Beat 4 종료 시 회수 타이밍: 4 + 1 == 5)
		TestFalse(TEXT("Beat 3 종료 시는 회수 타이밍 아님"), Range.IsEndingAtBeat(3));
		TestTrue(TEXT("Beat 4 종료 시는 회수 타이밍"), Range.IsEndingAtBeat(4));
		TestFalse(TEXT("Beat 5는 회수 타이밍 아님"), Range.IsEndingAtBeat(5));

		// 비정상 범위 검증
		const FBBMBeatEffectRange InvertedRange(5, 2);
		TestFalse(TEXT("Start > End 범위는 IsValid() == false 여야 함"), InvertedRange.IsValid());

		const FBBMBeatEffectRange OutOfBoundsRange(0, 9);
		TestFalse(TEXT("End > 8 범위는 IsValid() == false 여야 함"), OutOfBoundsRange.IsValid());
	}

#if WITH_EDITOR
	// 6. IsDataValid 8비트 규격 및 EffectRange 검증: 7비트 패턴 포함 시 Invalid 검증
	FDataValidationContext Context;
	const EDataValidationResult ValidResult = AIDef->IsDataValid(Context);
	TestEqual(TEXT("정상 8비트 패턴들은 Valid 판정이어야 함"), ValidResult, EDataValidationResult::Valid);

	// 8비트가 아닌 비정상 패턴 추가
	FBBMAIBeatSequence InvalidPattern;
	InvalidPattern.PatternName = FName(TEXT("InvalidPattern_7Beats"));
	InvalidPattern.SelectionWeight = 1.0f;
	InvalidPattern.Sequence.Beats.SetNum(7); // 7비트 (규격 위반)
	AIDef->PatternPool.Add(InvalidPattern);

	FDataValidationContext InvalidContext;
	const EDataValidationResult InvalidResult = AIDef->IsDataValid(InvalidContext);
	TestEqual(TEXT("7비트 패턴 포함 시 Invalid 판정이어야 함"), InvalidResult, EDataValidationResult::Invalid);

	// 비정상 EffectRange가 포함된 패턴 검증
	AIDef->PatternPool.Pop(); // InvalidPattern 제거
	FBBMAIBeatSequence BadRangePattern;
	BadRangePattern.PatternName = FName(TEXT("BadRangePattern"));
	BadRangePattern.SelectionWeight = 1.0f;
	BadRangePattern.Sequence.Beats.SetNum(8);
	BadRangePattern.Sequence.EffectRanges.Add(FBBMBeatEffectRange(4, 2)); // Start > End
	AIDef->PatternPool.Add(BadRangePattern);

	FDataValidationContext BadRangeContext;
	const EDataValidationResult BadRangeResult = AIDef->IsDataValid(BadRangeContext);
	TestEqual(TEXT("비정상 EffectRange 포함 시 Invalid 판정이어야 함"), BadRangeResult, EDataValidationResult::Invalid);
#endif

	return true;
}

// 8. 플레이어 상인(ABBMPlayerMerchantBase) Definition 주입 및 인벤토리 컴포넌트 자동 초기화 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMPlayerMerchantInventoryTest,
	"DIS.BBM.Combat.PlayerMerchantInventoryInitialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMPlayerMerchantInventoryTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. 플레이어 상인 폰 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABBMPlayerMerchantBase* Merchant = TestWorld.World->SpawnActor<ABBMPlayerMerchantBase>(
		ABBMPlayerMerchantBase::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	TestTrue(TEXT("플레이어 상인이 정상 스폰되어야 함"), Merchant != nullptr);
	if (Merchant == nullptr)
	{
		return false;
	}

	// 2. 인벤토리 래퍼 컴포넌트 존재 검증
	UBBMInventoryComponent* InvenComp = Merchant->GetInventoryComponent();
	TestTrue(TEXT("플레이어 상인은 UBBMInventoryComponent를 소유해야 함"), InvenComp != nullptr);
	TestEqual(TEXT("기본 팀은 Player여야 함"), Merchant->GetTeamType(), EBBMTeamType::Player);

	// 3. 2D 배낭 인벤토리 정의 생성 (4열 x 3행 = 12칸)
	UBBMInventoryDefinition* InvenDef = NewObject<UBBMInventoryDefinition>();
	InvenDef->DefaultCapacity = 12;
	InvenDef->MaxExpandableCapacity = 12;
	InvenDef->LineBreakTileCount = 4;
	InvenDef->ExtractionOrder = EBBMItemExtractionOrder::TopToBottom_LeftToRight;

	// 4. 플레이어 캐릭터 전용 정의(PDA) 생성 및 인벤토리 정의 바인딩
	UBBMPlayerCharacterDefinition* PlayerDef = NewObject<UBBMPlayerCharacterDefinition>();
	PlayerDef->DisplayName = FText::FromString(TEXT("BBM Hero Merchant"));
	PlayerDef->TableKeyName = FName(TEXT("Player"));
	PlayerDef->DefaultTeam = EBBMTeamType::Player;
	PlayerDef->InventoryDefinition = InvenDef;
	PlayerDef->PawnClass = ABBMPlayerMerchantBase::StaticClass();

	TestEqual(TEXT("PlayerCharacterDefinition PrimaryAssetId 검증"), PlayerDef->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(BBMPrimaryAssetTypes::PlayerCharacter));
	TestTrue(TEXT("PlayerCharacterDefinition에서 인벤토리 정의 반환 검증"), PlayerDef->GetInventoryDefinition() == InvenDef);
	TestTrue(TEXT("PlayerCharacterDefinition PawnClass 검증"), PlayerDef->GetPawnClass() == ABBMPlayerMerchantBase::StaticClass());

	// 5. 플레이어 상인에 캐릭터 정의 설정 -> 인벤토리 컴포넌트 자동 초기화
	Merchant->SetCharacterDefinition(PlayerDef);

	TestTrue(TEXT("GetPlayerCharacterDefinition 일치 검증"), Merchant->GetPlayerCharacterDefinition() == PlayerDef);
	TestTrue(TEXT("인벤토리 컴포넌트 내 인벤토리가 유효해야 함"), InvenComp->HasValidInventory() == true);
	TestTrue(TEXT("인벤토리 컴포넌트의 인벤토리 정의 일치 검증"), InvenComp->GetInventoryDefinition() == InvenDef);

	// 6. 실제 배낭 인벤토리 런타임 인스턴스 검증
	UBBMInventory* Inventory = Merchant->GetInventory();
	TestTrue(TEXT("Merchant->GetInventory()가 유효해야 함"), Inventory != nullptr);
	if (Inventory != nullptr)
	{
		TestEqual(TEXT("인벤토리 슬롯 수량은 12개여야 함"), Inventory->GetSlotCount(), 12);
	}

	// 7. DeferredSpawn 방식 사전 주입 시나리오 검증
	ABBMPlayerMerchantBase* DeferredMerchant = TestWorld.World->SpawnActorDeferred<ABBMPlayerMerchantBase>(
		ABBMPlayerMerchantBase::StaticClass(),
		FTransform::Identity
	);

	TestTrue(TEXT("DeferredSpawn 상인 생성 성공 검증"), DeferredMerchant != nullptr);
	if (DeferredMerchant != nullptr)
	{
		// 스폰 완료(FinishSpawning) 전에 Definition 주입
		DeferredMerchant->SetCharacterDefinition(PlayerDef);
		DeferredMerchant->FinishSpawning(FTransform::Identity);

		TestTrue(TEXT("Deferred 주입 상인의 인벤토리 컴포넌트 유효 검증"), DeferredMerchant->GetInventoryComponent() != nullptr);
		TestTrue(TEXT("Deferred 주입 상인의 인벤토리 인스턴스 자동 초기화 검증"), DeferredMerchant->GetInventory() != nullptr);
		if (DeferredMerchant->GetInventory() != nullptr)
		{
			TestEqual(TEXT("Deferred 주입 상인 인벤토리 슬롯 수량 검증"), DeferredMerchant->GetInventory()->GetSlotCount(), 12);
		}
	}

	return true;
}

// 9. UBBMInventoryComponent 장비 일시 보관소(UsedEquipmentStorage) 아카이브, 유저 지정 슬롯 복원 및 수동 분리 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMInventoryComponentEquipmentStorageTest,
	"DIS.BBM.Combat.InventoryComponentEquipmentStorage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMInventoryComponentEquipmentStorageTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. 플레이어 상인 스폰 및 인벤토리 초기화 (4열 x 3행 = 12슬롯)
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABBMPlayerMerchantBase* Merchant = TestWorld.World->SpawnActor<ABBMPlayerMerchantBase>(
		ABBMPlayerMerchantBase::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	TestNotNull(TEXT("Merchant 스폰 검증"), Merchant);
	if (Merchant == nullptr)
	{
		return false;
	}

	UBBMInventoryComponent* InvenComp = Merchant->GetInventoryComponent();
	TestNotNull(TEXT("InvenComp 유효 검증"), InvenComp);
	if (InvenComp == nullptr)
	{
		return false;
	}

	UBBMInventoryDefinition* InvenDef = NewObject<UBBMInventoryDefinition>();
	InvenDef->DefaultCapacity = 12;
	InvenDef->MaxExpandableCapacity = 12;
	InvenDef->LineBreakTileCount = 4;
	InvenComp->InitializeInventory(InvenDef);

	UBBMInventory* Inventory = InvenComp->GetInventory();
	TestNotNull(TEXT("Inventory 유효 검증"), Inventory);
	if (Inventory == nullptr)
	{
		return false;
	}

	// 2. 테스트용 1x1 장비 아이템 스택 2개 생성
	UDISItemDefinition* EquipDefA = NewObject<UDISItemDefinition>();
	UDISItemEntry* EntryA = NewObject<UDISItemEntry>();
	EntryA->InitializeEntry(EquipDefA);
	UDISItemStack* EquipStackA = NewObject<UDISItemStack>();
	EquipStackA->PushEntry(EntryA);

	UDISItemDefinition* EquipDefB = NewObject<UDISItemDefinition>();
	UDISItemEntry* EntryB = NewObject<UDISItemEntry>();
	EntryB->InitializeEntry(EquipDefB);
	UDISItemStack* EquipStackB = NewObject<UDISItemStack>();
	EquipStackB->PushEntry(EntryB);

	// 슬롯 2와 5에 임의 배치
	Inventory->SetSlotItemStack(2, EquipStackA);
	Inventory->SetSlotItemStack(5, EquipStackB);

	TestEqual(TEXT("슬롯 2 아이템 배치 검증"), Inventory->GetSlotItemStack(2), EquipStackA);
	TestEqual(TEXT("슬롯 5 아이템 배치 검증"), Inventory->GetSlotItemStack(5), EquipStackB);

	// 3. EquipStackA 아카이브 (UsedEquipmentStorage 이관)
	const bool bArchiveASuccess = InvenComp->ArchiveEquipment(EquipStackA);
	TestTrue(TEXT("EquipStackA 아카이브 성공 검증"), bArchiveASuccess);
	TestTrue(TEXT("EquipStackA가 아카이브 목록에 존재해야 함"), InvenComp->IsEquipmentArchived(EquipStackA));
	TestEqual(TEXT("아카이브된 장비 수량은 1이어야 함"), InvenComp->GetArchivedEquipmentCount(), 1);
	TestTrue(TEXT("아카이브 후 배낭의 슬롯 2는 비어있어야 함"), Inventory->IsSlotEmpty(2));

	// 중복 아카이브 방지 검증
	const bool bDuplicateArchive = InvenComp->ArchiveEquipment(EquipStackA);
	TestFalse(TEXT("이미 보관된 장비의 중복 아카이브는 실패해야 함"), bDuplicateArchive);

	// 4. 슬롯 인덱스 기준 EquipStackB 아카이브 (ArchiveEquipmentAtSlot)
	const bool bArchiveBSuccess = InvenComp->ArchiveEquipmentAtSlot(5);
	TestTrue(TEXT("슬롯 5 장비 아카이브 성공 검증"), bArchiveBSuccess);
	TestTrue(TEXT("EquipStackB가 아카이브 목록에 존재해야 함"), InvenComp->IsEquipmentArchived(EquipStackB));
	TestEqual(TEXT("아카이브된 장비 수량은 2여야 함"), InvenComp->GetArchivedEquipmentCount(), 2);
	TestTrue(TEXT("아카이브 후 배낭의 슬롯 5는 비어있어야 함"), Inventory->IsSlotEmpty(5));

	// 5. 유저가 선택한 슬롯으로 복원 (RestoreArchivedEquipmentToSlot)
	const bool bRestoreToSlotSuccess = InvenComp->RestoreArchivedEquipmentToSlot(EquipStackA, 7);
	TestTrue(TEXT("유저 지정 슬롯 7 복원 성공 검증"), bRestoreToSlotSuccess);
	TestEqual(TEXT("슬롯 7에 EquipStackA가 정확히 배치되어야 함"), Inventory->GetSlotItemStack(7), EquipStackA);
	TestFalse(TEXT("복원 후 EquipStackA는 아카이브 목록에서 제거되어야 함"), InvenComp->IsEquipmentArchived(EquipStackA));
	TestEqual(TEXT("잔여 아카이브 장비 수량은 1이어야 함"), InvenComp->GetArchivedEquipmentCount(), 1);

	// 이미 다른 아이템이 있는 슬롯(7)에 중복 복원 시도 시 실패 검증
	const bool bConflictRestore = InvenComp->RestoreArchivedEquipmentToSlot(EquipStackB, 7);
	TestFalse(TEXT("점유 중인 슬롯으로의 복원은 실패해야 함"), bConflictRestore);

	// 6. 보관소에서 수동 꺼내기 검증 (TakeArchivedEquipment - UI 드래그 상호작용 지원)
	UDISItemStack* TakenStack = InvenComp->TakeArchivedEquipment(EquipStackB);
	TestEqual(TEXT("꺼낸 장비 스택 일치 검증"), TakenStack, EquipStackB);
	TestFalse(TEXT("꺼낸 후 보관소에서 제거되어야 함"), InvenComp->IsEquipmentArchived(EquipStackB));
	TestFalse(TEXT("보관소는 비어있어야 함"), InvenComp->HasArchivedEquipment());

	// 7. 가로 2x1 장비가 회전을 통해 세로 빈 공간에 자동으로 맞춰 복원되는지 검증
	for (int32 SlotIdx = 0; SlotIdx < 12; ++SlotIdx)
	{
		Inventory->ClearSlot(SlotIdx);
	}

	UDISItemDefinition* WideDef = NewObject<UDISItemDefinition>();
	WideDef->bCanRotate = true;
	TArray<FIntPoint> WidePoints;
	WidePoints.Add(FIntPoint(0, 0));
	WidePoints.Add(FIntPoint(1, 0)); // 가로 2x1
	WideDef->Shape = FDISTileShape(WidePoints);

	UDISItemEntry* WideEntry = NewObject<UDISItemEntry>();
	WideEntry->InitializeEntry(WideDef);
	UDISItemStack* WideStack = NewObject<UDISItemStack>();
	WideStack->PushEntry(WideEntry);

	// 슬롯 0에 임시 배치 후 아카이브
	Inventory->SetSlotItemStack(0, WideStack);
	InvenComp->ArchiveEquipment(WideStack);
	TestTrue(TEXT("WideStack 아카이브 성공"), InvenComp->IsEquipmentArchived(WideStack));

	// 인벤토리 상태를 세로 2칸만 남도록 설정:
	for (int32 SlotIdx = 0; SlotIdx < 12; ++SlotIdx)
	{
		if (SlotIdx != 0 && SlotIdx != 4)
		{
			UDISItemDefinition* DummyDef = NewObject<UDISItemDefinition>();
			UDISItemEntry* DummyEntry = NewObject<UDISItemEntry>();
			DummyEntry->InitializeEntry(DummyDef);
			UDISItemStack* DummyStack = NewObject<UDISItemStack>();
			DummyStack->PushEntry(DummyEntry);
			Inventory->SetSlotItemStack(SlotIdx, DummyStack);
		}
	}

	// 가로 2x1로는 슬롯 0에 배치 불가 (슬롯 1이 차있음), 하지만 90도 회전(세로 1x2) 시 슬롯 0, 4에 들어갈 수 있음
	const bool bRotateFitSuccess = InvenComp->RestoreArchivedEquipment(WideStack, true);
	TestTrue(TEXT("회전을 통한 빈 공간 맞춤 복원 성공 검증"), bRotateFitSuccess);
	TestEqual(TEXT("WideStack은 슬롯 0에 세로로 회전되어 배치되어야 함"), Inventory->GetItemStackRootSlot(WideStack), 0);
	TestFalse(TEXT("복원 후 WideStack은 아카이브에서 제거되어야 함"), InvenComp->IsEquipmentArchived(WideStack));

	// 8. ClearArchivedEquipment 강제 정리 검증
	UDISItemDefinition* ClearTestDef = NewObject<UDISItemDefinition>();
	UDISItemEntry* ClearTestEntry = NewObject<UDISItemEntry>();
	ClearTestEntry->InitializeEntry(ClearTestDef);
	UDISItemStack* ClearTestStack = NewObject<UDISItemStack>();
	ClearTestStack->PushEntry(ClearTestEntry);

	Inventory->ClearSlot(0); // 슬롯 0 비우기
	Inventory->SetSlotItemStack(0, ClearTestStack);
	InvenComp->ArchiveEquipment(ClearTestStack);
	TestTrue(TEXT("보관 확인"), InvenComp->HasArchivedEquipment());

	InvenComp->ClearArchivedEquipment();
	TestFalse(TEXT("Clear 후 보관소가 비어있어야 함"), InvenComp->HasArchivedEquipment());

	return true;
}

// 10. ABBMAICombatantBase 패턴 시퀀스 보관, 랜덤 갱신 및 GetCurrentBeatSequence 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatantAIBeatSequenceTest,
	"DIS.BBM.Combat.AIBeatSequenceSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMCombatantAIBeatSequenceTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. AI 전투원 폰 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABBMAICombatantBase* AICombatant = TestWorld.World->SpawnActor<ABBMAICombatantBase>(
		ABBMAICombatantBase::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	TestNotNull(TEXT("AICombatant 스폰 검증"), AICombatant);
	if (AICombatant == nullptr)
	{
		return false;
	}

	// Definition 주입 전 기본 비트 시퀀스는 빈 시퀀스여야 함
	TestEqual(TEXT("초기 시퀀스 비트 수는 0이어야 함"), AICombatant->GetCurrentBeatSequence().Beats.Num(), 0);

	// 2. 8비트 패턴 풀을 갖춘 AI 정적 정의 에셋 구성
	UBBMAICombatantDefinition* AIDef = NewObject<UBBMAICombatantDefinition>();
	AIDef->DisplayName = FText::FromString(TEXT("TestMonster"));
	AIDef->TableKeyName = FName(TEXT("Monster"));
	AIDef->DefaultTeam = EBBMTeamType::Monster;

	// 패턴 A: 8비트
	FBBMAIBeatSequence PatternA;
	PatternA.PatternName = FName(TEXT("PatternA"));
	PatternA.SelectionWeight = 1.0f;
	PatternA.bCanRepeatConsecutively = true;
	PatternA.Sequence.Beats.SetNum(8);
	PatternA.Sequence.EffectRanges.Add(FBBMBeatEffectRange(0, 4));
	AIDef->PatternPool.Add(PatternA);

	// 패턴 B: 8비트
	FBBMAIBeatSequence PatternB;
	PatternB.PatternName = FName(TEXT("PatternB"));
	PatternB.SelectionWeight = 2.0f;
	PatternB.bCanRepeatConsecutively = true;
	PatternB.Sequence.Beats.SetNum(8);
	PatternB.Sequence.EffectRanges.Add(FBBMBeatEffectRange(4, 8));
	AIDef->PatternPool.Add(PatternB);

	// 3. Definition 주입 검증 (초기 캐시는 비어있으며 전투 단계에서 직접 UpdateBeatSequence로 갱신함)
	AICombatant->SetCharacterDefinition(AIDef);

	TestNull(TEXT("초기에는 SelectedPattern이 비어있어야 함"), AICombatant->GetSelectedPattern());
	TestEqual(TEXT("초기 비트 시퀀스는 비어있어야 함"), AICombatant->GetCurrentBeatSequence().Beats.Num(), 0);

	// 전투 단계: UpdateBeatSequence를 통해 1턴 AI 패턴 무작위 선정 및 캐시 갱신
	AICombatant->UpdateBeatSequence();

	TestNotNull(TEXT("UpdateBeatSequence 호출 후 SelectedPattern이 선정되어 있어야 함"), AICombatant->GetSelectedPattern());
	TestTrue(TEXT("선정된 패턴 이름이 유효해야 함"), AICombatant->GetSelectedPatternName() == FName(TEXT("PatternA")) || AICombatant->GetSelectedPatternName() == FName(TEXT("PatternB")));

	// GetCurrentBeatSequence()가 선정된 패턴 시퀀스와 일치하는지 검증
	const FBBMBeatSequence CurSeq = AICombatant->GetCurrentBeatSequence();
	TestTrue(TEXT("8비트 완성형 시퀀스여야 함"), CurSeq.IsFullEightBeats());
	TestEqual(TEXT("시퀀스 EffectRange 수량 검증"), CurSeq.EffectRanges.Num(), 1);

	// 4. 결정론적 RandomStream을 사용한 새 패턴 갱신 헬퍼 검증
	FRandomStream FixedStream(42);
	const FBBMAIBeatSequence* SelectedNew = AICombatant->SelectNewPattern(FixedStream);
	TestNotNull(TEXT("SelectNewPattern 성공 검증"), SelectedNew);
	TestEqual(TEXT("GetCurrentBeatSequence가 갱신된 패턴과 일치해야 함"), AICombatant->GetCurrentBeatSequence().EffectRanges[0].StartBeat, SelectedNew->Sequence.EffectRanges[0].StartBeat);

	// 5. SelectNewPatternRandom 헬퍼 검증
	const bool bRandomSuccess = AICombatant->SelectNewPatternRandom();
	TestTrue(TEXT("SelectNewPatternRandom 성공 검증"), bRandomSuccess);
	TestNotNull(TEXT("SelectedPattern 유효 검증"), AICombatant->GetSelectedPattern());

	// 6. UpdateBeatSequence 가상 함수 갱신 검증
	AICombatant->UpdateBeatSequence();
	TestNotNull(TEXT("UpdateBeatSequence 호출 후에도 SelectedPattern 유효 검증"), AICombatant->GetSelectedPattern());
	TestEqual(TEXT("UpdateBeatSequence 후 GetCurrentBeatSequence 캐시 일치 검증"), AICombatant->GetCurrentBeatSequence().EffectRanges[0].StartBeat, AICombatant->GetSelectedPattern()->Sequence.EffectRanges[0].StartBeat);

	return true;
}

// 11. ABBMPlayerMerchantBase ExtractionOrder 기반 8비트 시퀀스 합성 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatantPlayerBeatSequenceTest,
	"DIS.BBM.Combat.PlayerBeatSequenceGeneration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMCombatantPlayerBeatSequenceTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. 플레이어 상인 스폰 및 인벤토리 초기화
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABBMPlayerMerchantBase* Merchant = TestWorld.World->SpawnActor<ABBMPlayerMerchantBase>(
		ABBMPlayerMerchantBase::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	TestNotNull(TEXT("Merchant 스폰 검증"), Merchant);
	if (Merchant == nullptr)
	{
		return false;
	}

	// 4열 x 3행 인벤토리 정의
	UBBMInventoryDefinition* InvenDef = NewObject<UBBMInventoryDefinition>();
	InvenDef->DefaultCapacity = 12;
	InvenDef->MaxExpandableCapacity = 12;
	InvenDef->LineBreakTileCount = 4;
	InvenDef->ExtractionOrder = EBBMItemExtractionOrder::TopToBottom_LeftToRight;

	UBBMPlayerCharacterDefinition* PlayerDef = NewObject<UBBMPlayerCharacterDefinition>();
	PlayerDef->InventoryDefinition = InvenDef;
	PlayerDef->PawnClass = ABBMPlayerMerchantBase::StaticClass();

	Merchant->SetCharacterDefinition(PlayerDef);
	UBBMInventory* Inventory = Merchant->GetInventory();
	TestNotNull(TEXT("Inventory 유효 검증"), Inventory);
	if (Inventory == nullptr)
	{
		return false;
	}

	// 초기 캐시는 비어있어야 함 (전투 단계에서 UpdateBeatSequence를 통해 갱신)
	const FBBMBeatSequence InitialSeq = Merchant->GetCurrentBeatSequence();
	TestEqual(TEXT("초기 캐시 비트 수는 0이어야 함"), InitialSeq.Beats.Num(), 0);

	// 전투 단계: UpdateBeatSequence를 통해 배낭 아이템 기반 비트 시퀀스 갱신
	Merchant->UpdateBeatSequence();

	// 빈 인벤토리 시 기본 8비트 빈 시퀀스 반환 검증
	const FBBMBeatSequence EmptySeq = Merchant->GetCurrentBeatSequence();
	TestEqual(TEXT("기본 8비트 슬롯 확보 검증"), EmptySeq.Beats.Num(), 8);
	TestEqual(TEXT("효과 범위는 0개여야 함"), EmptySeq.EffectRanges.Num(), 0);

	// GenerateBeatSequenceFromInventory 직접 호출 검증
	const FBBMBeatSequence DirectEmptySeq = Merchant->GenerateBeatSequenceFromInventory();
	TestEqual(TEXT("GenerateBeatSequenceFromInventory 기본 인자 호출 일치 검증"), DirectEmptySeq.Beats.Num(), 8);

	// 2. 가로 2x1 아이템 A 생성 (2비트 차지, EffectRange [0, 2), AnimLayerRange [0, 2)) - 장비(Item_Equipment)
	UBBMItemDefinition* ItemDefA = NewObject<UBBMItemDefinition>();
	ItemDefA->ItemTypeTag = BBMItemTypeTags::Item_Equipment;
	TArray<FIntPoint> ShapeA;
	ShapeA.Add(FIntPoint(0, 0));
	ShapeA.Add(FIntPoint(1, 0));
	ItemDefA->Shape = FDISTileShape(ShapeA);
	ItemDefA->BeatSequence.Beats.SetNum(2);
	ItemDefA->BeatSequence.EffectRanges.Add(FBBMBeatEffectRange(0, 2));
	ItemDefA->BeatSequence.AnimLayerRanges.Add(FBBMBeatAnimLayerRange(0, 2, BBMAnimLayerTags::AnimLayer_Weapon_Sword.GetTag()));

	UBBMItemEntry* EntryA = NewObject<UBBMItemEntry>();
	EntryA->InitializeEntry(ItemDefA);
	UDISItemStack* StackA = NewObject<UDISItemStack>();
	StackA->PushEntry(EntryA);

	// 3. 가로 2x1 아이템 B 생성 (2비트 차지, EffectRange [0, 2), AnimLayerRange [0, 2)) - 소모품(Item_Consumable)
	UBBMItemDefinition* ItemDefB = NewObject<UBBMItemDefinition>();
	ItemDefB->ItemTypeTag = BBMItemTypeTags::Item_Consumable;
	TArray<FIntPoint> ShapeB;
	ShapeB.Add(FIntPoint(0, 0));
	ShapeB.Add(FIntPoint(1, 0));
	ItemDefB->Shape = FDISTileShape(ShapeB);
	ItemDefB->BeatSequence.Beats.SetNum(2);
	ItemDefB->BeatSequence.EffectRanges.Add(FBBMBeatEffectRange(0, 2));
	ItemDefB->BeatSequence.AnimLayerRanges.Add(FBBMBeatAnimLayerRange(0, 2, BBMAnimLayerTags::AnimLayer_Armor_Boots.GetTag()));

	UBBMItemEntry* EntryB = NewObject<UBBMItemEntry>();
	EntryB->InitializeEntry(ItemDefB);
	UDISItemStack* StackB = NewObject<UDISItemStack>();
	StackB->PushEntry(EntryB);

	// 슬롯 0에 Item A, 슬롯 2에 Item B 배치 (TopToBottom_LeftToRight 정렬 시 A -> B 순서)
	Inventory->SetSlotItemStack(0, StackA);
	Inventory->SetSlotItemStack(2, StackB);

	// 배치 직후 아직 UpdateBeatSequence() 호출 전에는 이전 캐시(효과 범위 0개) 상태여야 함
	TestEqual(TEXT("Update 전 GetCurrentBeatSequence는 이전 캐시 유지"), Merchant->GetCurrentBeatSequence().EffectRanges.Num(), 0);

	// 4. UpdateBeatSequence() 호출 -> 소유 인벤토리로부터 누적 합성 및 캐시 갱신 (장비는 UsedEquipmentStorage 이관, 소모품은 소멸)
	Merchant->UpdateBeatSequence();

	// GetCurrentBeatSequence() 호출 -> 캐시된 8비트 시퀀스 검증
	const FBBMBeatSequence CompiledSeq = Merchant->GetCurrentBeatSequence();
	TestEqual(TEXT("합성된 시퀀스는 8비트여야 함"), CompiledSeq.Beats.Num(), 8);
	TestEqual(TEXT("합성된 시퀀스는 총 2개의 EffectRange를 포함해야 함"), CompiledSeq.EffectRanges.Num(), 2);
	TestEqual(TEXT("합성된 시퀀스는 총 2개의 AnimLayerRange를 포함해야 함"), CompiledSeq.AnimLayerRanges.Num(), 2);

	if (CompiledSeq.EffectRanges.Num() == 2)
	{
		// Item A의 범위: [0, 2)
		TestEqual(TEXT("Item A 시작 비트는 0이어야 함"), CompiledSeq.EffectRanges[0].StartBeat, 0);
		TestEqual(TEXT("Item A 종료 비트는 2여야 함"), CompiledSeq.EffectRanges[0].EndBeat, 2);

		// Item B의 범위: 오프셋 2 누적으로 [2, 4)
		TestEqual(TEXT("Item B 시작 비트는 2여야 함"), CompiledSeq.EffectRanges[1].StartBeat, 2);
		TestEqual(TEXT("Item B 종료 비트는 4여야 함"), CompiledSeq.EffectRanges[1].EndBeat, 4);
	}

	if (CompiledSeq.AnimLayerRanges.Num() == 2)
	{
		// Item A의 AnimLayer 범위: [0, 2)
		TestEqual(TEXT("Item A AnimLayer 시작 비트는 0이어야 함"), CompiledSeq.AnimLayerRanges[0].StartBeat, 0);
		TestEqual(TEXT("Item A AnimLayer 종료 비트는 2여야 함"), CompiledSeq.AnimLayerRanges[0].EndBeat, 2);

		// Item B의 AnimLayer 범위: 오프셋 2 누적으로 [2, 4)
		TestEqual(TEXT("Item B AnimLayer 시작 비트는 2여야 함"), CompiledSeq.AnimLayerRanges[1].StartBeat, 2);
		TestEqual(TEXT("Item B AnimLayer 종료 비트는 4여야 함"), CompiledSeq.AnimLayerRanges[1].EndBeat, 4);
	}

	// 장비 아이템 보관소 이관 및 인벤토리 슬롯 제거 검증
	TestTrue(TEXT("장비 아이템 StackA는 UsedEquipmentStorage로 이관되어야 함"), Merchant->GetInventoryComponent()->IsEquipmentArchived(StackA));
	TestEqual(TEXT("장비 아이템 StackA는 인벤토리에서 제거되어 슬롯 검색 불가여야 함"), Inventory->GetItemStackRootSlot(StackA), static_cast<int32>(INDEX_NONE));

	// 소모품 아이템 인벤토리 슬롯 완전 소멸 검증
	TestEqual(TEXT("소모품 아이템 StackB는 인벤토리에서 완전 제거되어 슬롯 검색 불가여야 함"), Inventory->GetItemStackRootSlot(StackB), static_cast<int32>(INDEX_NONE));

	// 5. GDD 4.1 8비트 초과 Overflow 규칙 검증:
	// 앞선 단계에서 StackA(장비)는 UsedEquipmentStorage로 이관되고, StackB(소모품)는 소멸되었음.
	// 이제 슬롯 0에 4비트 무기 W1, 슬롯 4에 4비트 무기 W2를 배치하여 8비트를 꽉 채움.
	UBBMItemDefinition* WeaponDef1 = NewObject<UBBMItemDefinition>();
	WeaponDef1->ItemTypeTag = BBMItemTypeTags::Item_Weapon;
	TArray<FIntPoint> ShapeW1;
	for (int32 i = 0; i < 4; ++i) { ShapeW1.Add(FIntPoint(i, 0)); }
	WeaponDef1->Shape = FDISTileShape(ShapeW1);
	WeaponDef1->BeatSequence.Beats.SetNum(4);
	WeaponDef1->BeatSequence.EffectRanges.Add(FBBMBeatEffectRange(0, 4));

	UBBMItemEntry* EntryW1 = NewObject<UBBMItemEntry>();
	EntryW1->InitializeEntry(WeaponDef1);
	UDISItemStack* StackW1 = NewObject<UDISItemStack>();
	StackW1->PushEntry(EntryW1);

	UBBMItemDefinition* WeaponDef2 = NewObject<UBBMItemDefinition>();
	WeaponDef2->ItemTypeTag = BBMItemTypeTags::Item_Weapon;
	TArray<FIntPoint> ShapeW2;
	for (int32 i = 0; i < 4; ++i) { ShapeW2.Add(FIntPoint(i, 1)); }
	WeaponDef2->Shape = FDISTileShape(ShapeW2);
	WeaponDef2->BeatSequence.Beats.SetNum(4);
	WeaponDef2->BeatSequence.EffectRanges.Add(FBBMBeatEffectRange(0, 4));

	UBBMItemEntry* EntryW2 = NewObject<UBBMItemEntry>();
	EntryW2->InitializeEntry(WeaponDef2);
	UDISItemStack* StackW2 = NewObject<UDISItemStack>();
	StackW2->PushEntry(EntryW2);

	Inventory->SetSlotItemStack(0, StackW1);
	Inventory->SetSlotItemStack(4, StackW2);

	// 8비트를 채운 상태에서 5비트 아이템 C를 슬롯 8에 추가 배치
	UBBMItemDefinition* ItemDefC = NewObject<UBBMItemDefinition>();
	TArray<FIntPoint> ShapeC;
	for (int32 i = 0; i < 5; ++i) { ShapeC.Add(FIntPoint(i % 4, 2 + (i / 4))); }
	ItemDefC->Shape = FDISTileShape(ShapeC);
	ItemDefC->BeatSequence.Beats.SetNum(5);
	ItemDefC->BeatSequence.EffectRanges.Add(FBBMBeatEffectRange(0, 5));

	UBBMItemEntry* EntryC = NewObject<UBBMItemEntry>();
	EntryC->InitializeEntry(ItemDefC);
	UDISItemStack* StackC = NewObject<UDISItemStack>();
	StackC->PushEntry(EntryC);

	Inventory->SetSlotItemStack(8, StackC);

	// GenerateBeatSequenceFromInventory() 직접 호출로 갱신 및 캐시 검증
	const FBBMBeatSequence OverflowSeq = Merchant->GenerateBeatSequenceFromInventory();
	TestEqual(TEXT("8비트 초과 시 Overflow 아이템은 제외되어 여전히 2개 범위만 유지되어야 함"), OverflowSeq.EffectRanges.Num(), 2);
	TestEqual(TEXT("GetCurrentBeatSequence도 동일하게 2개 범위 캐시를 반환해야 함"), Merchant->GetCurrentBeatSequence().EffectRanges.Num(), 2);

	return true;
}

// 12. BBM 1D 타일 라인 전투 매니저 종합 단위 테스트 (진형, 시프트, 3단계 방어 및 ResolveAttack, 턴 방어도 초기화, 상태이상 독/화상 틱, 비트 커맨드 실행)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatManagerTest,
	"DIS.BBM.Combat.CombatManager",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FBBMCombatManagerTest::RunTest(const FString& Parameters)
{
	// 1. GAS 전역 초기화 및 테스트 월드/WorldContext 준비
	UAbilitySystemGlobals::Get().InitGlobalData();

	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("테스트 월드 생성 성공"), TestWorld);
	if (TestWorld == nullptr)
	{
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);
	FURL URL;
	TestWorld->InitializeActorsForPlay(URL);

	UBBMCombatManagerSubsystem* CombatMgr = NewObject<UBBMCombatManagerSubsystem>(TestWorld);
	FSubsystemCollection<UWorldSubsystem> SubsystemCollection;
	CombatMgr->Initialize(SubsystemCollection);

	// 2. 테스트용 플레이어 및 몬스터 전투원 생성 및 GAS ActorInfo 초기화
	ABBMPlayerMerchantBase* PlayerMerchant = TestWorld->SpawnActor<ABBMPlayerMerchantBase>();
	TestNotNull(TEXT("플레이어 상인 액터 스폰 성공"), PlayerMerchant);
	PlayerMerchant->GetAbilitySystemComponent()->InitAbilityActorInfo(PlayerMerchant, PlayerMerchant);

	ABBMAICombatantBase* MonsterA = TestWorld->SpawnActor<ABBMAICombatantBase>();
	TestNotNull(TEXT("몬스터 A 스폰 성공"), MonsterA);
	MonsterA->GetAbilitySystemComponent()->InitAbilityActorInfo(MonsterA, MonsterA);

	ABBMAICombatantBase* MonsterB = TestWorld->SpawnActor<ABBMAICombatantBase>();
	TestNotNull(TEXT("몬스터 B 스폰 성공"), MonsterB);
	MonsterB->GetAbilitySystemComponent()->InitAbilityActorInfo(MonsterB, MonsterB);

	// 기본 체력 설정 (100 HP)
	PlayerMerchant->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetMaxHealthAttribute(), 100.0f);
	PlayerMerchant->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), 100.0f);

	MonsterA->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetMaxHealthAttribute(), 100.0f);
	MonsterA->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), 100.0f);

	MonsterB->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetMaxHealthAttribute(), 100.0f);
	MonsterB->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), 100.0f);

	// 3. 1D 타일 라인 진형 초기화 및 최전방 조회 검증
	TArray<ABBMAICombatantBase*> MonsterList = { MonsterA, MonsterB };
	CombatMgr->InitializeBattleFormation(PlayerMerchant, MonsterList);

	ABBMCombatantBase* AllyFront = CombatMgr->GetFrontlineCombatant(EBBMCombatLane::AllyLane);
	TestEqual(TEXT("아군 최전방은 플레이어 상인이어야 함"), AllyFront, Cast<ABBMCombatantBase>(PlayerMerchant));

	ABBMCombatantBase* EnemyFront = CombatMgr->GetFrontlineCombatant(EBBMCombatLane::EnemyLane);
	TestEqual(TEXT("적군 최전방은 몬스터 A여야 함"), EnemyFront, Cast<ABBMCombatantBase>(MonsterA));

	TArray<ABBMCombatantBase*> LivingEnemies = CombatMgr->GetLivingCombatantsInLane(EBBMCombatLane::EnemyLane);
	TestEqual(TEXT("적군 생존 유닛은 2마리여야 함"), LivingEnemies.Num(), 2);

	// 전체 전투원 헬퍼(GetAllLivingCombatants, GetAllCombatants) 검증
	TArray<ABBMCombatantBase*> AllLiving = CombatMgr->GetAllLivingCombatants();
	TestEqual(TEXT("전체 생존 유닛은 3명(상인 1 + 몬스터 2)이어야 함"), AllLiving.Num(), 3);
	TestTrue(TEXT("전체 생존 목록에 플레이어 상인 포함 검증"), AllLiving.Contains(Cast<ABBMCombatantBase>(PlayerMerchant)));
	TestTrue(TEXT("전체 생존 목록에 몬스터 A 포함 검증"), AllLiving.Contains(Cast<ABBMCombatantBase>(MonsterA)));
	TestTrue(TEXT("전체 생존 목록에 몬스터 B 포함 검증"), AllLiving.Contains(Cast<ABBMCombatantBase>(MonsterB)));
	TestEqual(TEXT("GetAllCombatants(true)와 GetAllLivingCombatants() 일치 검증"), CombatMgr->GetAllCombatants(true).Num(), 3);

	// 4. 1D 타일 라인 시프트(ShiftLane) 검증
	CombatMgr->ShiftLane(EBBMCombatLane::EnemyLane, EBBMLaneShiftDirection::Backward, 1);
	TestEqual(TEXT("넉백 후에도 생존 최전방 탐색은 MonsterA를 반환해야 함"), CombatMgr->GetFrontlineCombatant(EBBMCombatLane::EnemyLane), Cast<ABBMCombatantBase>(MonsterA));

	CombatMgr->ShiftLane(EBBMCombatLane::EnemyLane, EBBMLaneShiftDirection::Frontward, 1);

	// 5. 엄격한 3단계 방어 및 공격 해결(ResolveAttack) 검증
	// 5-1. 패링 상태: 완전 무효화 (0 피해)
	TMap<TWeakObjectPtr<ABBMCombatantBase>, FBBMCombatantActionLog> CombatLogs;
	const float ParryDamage = CombatMgr->ResolveAttack(PlayerMerchant, MonsterA, 50.0f, true, CombatLogs);
	TestEqual(TEXT("패링 상태 시 피해는 0이어야 함"), ParryDamage, 0.0f);
	TestEqual(TEXT("패링 시 체력 보존 (100 HP)"), MonsterA->GetAttributeSet()->GetHealth(), 100.0f);

	// 5-2. 턴 방어도(10) 및 지속 방어도(20) 부여
	CombatMgr->AddTurnArmor(MonsterA, 10.0f, CombatLogs);
	CombatMgr->AddPersistentArmor(MonsterA, 20.0f, CombatLogs);
	TestEqual(TEXT("턴 방어도 10 검증"), MonsterA->GetAttributeSet()->GetTurnArmor(), 10.0f);
	TestEqual(TEXT("지속 방어도 20 검증"), MonsterA->GetAttributeSet()->GetArmor(), 20.0f);

	// 15 피해 공격: 턴 방어도(10) 우선 흡수 소진 -> 지속 방어도에서 5 흡수(남은 지속 방어도 15) -> 체력 피해 0
	CombatLogs.Empty();
	float DealtDamage1 = CombatMgr->ResolveAttack(PlayerMerchant, MonsterA, 15.0f, false, CombatLogs);
	TestEqual(TEXT("방어도로 완전 흡수되어 실제 체력 피해는 0이어야 함"), DealtDamage1, 0.0f);
	TestEqual(TEXT("턴 방어도는 완전 소진되어 0이어야 함"), MonsterA->GetAttributeSet()->GetTurnArmor(), 0.0f);
	TestEqual(TEXT("지속 방어도는 15가 남아있어야 함"), MonsterA->GetAttributeSet()->GetArmor(), 15.0f);
	TestEqual(TEXT("체력은 여전히 100이어야 함"), MonsterA->GetAttributeSet()->GetHealth(), 100.0f);

	// 30 피해 공격: 남은 지속 방어도 15 흡수 소진 -> 잔여 15 체력 피해 결착
	CombatLogs.Empty();
	float DealtDamage2 = CombatMgr->ResolveAttack(PlayerMerchant, MonsterA, 30.0f, false, CombatLogs);
	TestEqual(TEXT("지속 방어도를 뚫고 들어간 체력 피해는 15여야 함"), DealtDamage2, 15.0f);
	TestEqual(TEXT("지속 방어도는 0이어야 함"), MonsterA->GetAttributeSet()->GetArmor(), 0.0f);
	TestEqual(TEXT("체력은 85가 되어야 함"), MonsterA->GetAttributeSet()->GetHealth(), 85.0f);

	// 6. 턴 방어도 일괄 초기화(ClearAllTurnArmors) 검증
	CombatLogs.Empty();
	CombatMgr->AddTurnArmor(PlayerMerchant, 25.0f, CombatLogs);
	CombatMgr->AddPersistentArmor(PlayerMerchant, 30.0f, CombatLogs);
	CombatMgr->ClearAllTurnArmors(CombatLogs);
	TestEqual(TEXT("턴 방어도는 0으로 초기화되어야 함"), PlayerMerchant->GetAttributeSet()->GetTurnArmor(), 0.0f);
	TestEqual(TEXT("지속 방어도는 보존되어야 함 (30)"), PlayerMerchant->GetAttributeSet()->GetArmor(), 30.0f);

	// 7. GameplayTag 기반 상태이상 및 턴 종료 틱(ProcessTurnEndStatusTicks) 검증
	CombatLogs.Empty();
	CombatMgr->ApplyStatusEffect(MonsterB, BBMStatusTags::Status_Poison, 3, CombatLogs);
	CombatMgr->ApplyStatusEffect(MonsterB, BBMStatusTags::Status_Burn, 2, CombatLogs);
	TestEqual(TEXT("독 3스택 검증"), CombatMgr->GetStatusStacks(MonsterB, BBMStatusTags::Status_Poison), 3);
	TestEqual(TEXT("화상 2스택 검증"), CombatMgr->GetStatusStacks(MonsterB, BBMStatusTags::Status_Burn), 2);

	// 턴 틱 실행: 독(3피해) + 화상(5피해) = 8 피해 결착, 독 스택 2로 감소, 화상 스택 1로 감소
	CombatLogs.Empty();
	CombatMgr->ProcessTurnEndStatusTicks(CombatLogs);
	TestEqual(TEXT("턴 틱 후 체력은 92 HP여야 함"), MonsterB->GetAttributeSet()->GetHealth(), 92.0f);
	TestEqual(TEXT("독 스택은 1 감소하여 2여야 함"), CombatMgr->GetStatusStacks(MonsterB, BBMStatusTags::Status_Poison), 2);
	TestEqual(TEXT("화상 스택은 1 감소하여 1이어야 함"), CombatMgr->GetStatusStacks(MonsterB, BBMStatusTags::Status_Burn), 1);

	// 8. FBBMBeatAction 커맨드 패턴(ExecuteAction) 다형성 실행 검증
	CombatLogs.Empty();
	FBBMCombatAction AttackCommand(EBBMActionCategory::Damage, EBBMTargetScope::EnemyFrontline, 1.0f, 5.0f);
	const float PrevMonsterAHealth = MonsterA->GetAttributeSet()->GetHealth();
	AttackCommand.ExecuteAction(CombatMgr, PlayerMerchant, CombatLogs);
	TestEqual(TEXT("커맨드 실행으로 15 데미지가 최전방 몬스터 A에 적용되어야 함"), MonsterA->GetAttributeSet()->GetHealth(), PrevMonsterAHealth - 15.0f);

	// 이동 커맨드 실행
	FBBMMovementAction MoveCommand(EBBMActionCategory::PostMovement, EBBMLaneShiftDirection::Backward, 1);
	MoveCommand.ExecuteAction(CombatMgr, PlayerMerchant, CombatLogs);

	// 9. 기획 표준 턴(8비트) 시뮬레이션(SimulateTurn), 사망 처리, 승리 델리게이트 종합 검증
	bool bVictoryBroadcasted = false;
	TArray<UDISItemStack*> ReceivedUsedEquipment;
	TArray<FPrimaryAssetId> ReceivedRewardItems;
	CombatMgr->OnCombatVictoryNative.AddLambda([&](const TArray<UDISItemStack*>& InUsedEquipment, const TArray<FPrimaryAssetId>& InRewardItems)
	{
		bVictoryBroadcasted = true;
		ReceivedUsedEquipment = InUsedEquipment;
		ReceivedRewardItems = InRewardItems;
	});

	// 보상 아이템 등록
	const FPrimaryAssetId TestRewardId(TEXT("BBMItem:TestSword"));
	CombatMgr->AddPendingRewardItem(TestRewardId);

	// 플레이어 배낭 인벤토리 초기화
	UBBMInventoryDefinition* InvenDef = NewObject<UBBMInventoryDefinition>();
	InvenDef->DefaultCapacity = 16;
	InvenDef->MaxExpandableCapacity = 16;
	InvenDef->LineBreakTileCount = 4;
	InvenDef->ExtractionOrder = EBBMItemExtractionOrder::TopToBottom_LeftToRight;
	PlayerMerchant->GetInventoryComponent()->InitializeInventory(InvenDef);

	// 몬스터 A, B 체력을 10으로 조정 (치명타 1방에 즉시 사망하도록 설정)
	MonsterA->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), 10.0f);
	MonsterB->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), 10.0f);

	// 플레이어에게 2칸 장비 아이템 배치 (비트 0: 몬스터 A 처치, 비트 1: 몬스터 B 처치)
	UBBMItemDefinition* SlashItemDef = NewObject<UBBMItemDefinition>();
	SlashItemDef->ItemTypeTag = BBMItemTypeTags::Item_Equipment;
	TArray<FIntPoint> SlashShape;
	SlashShape.Add(FIntPoint(0, 0));
	SlashShape.Add(FIntPoint(1, 0));
	SlashItemDef->Shape = FDISTileShape(SlashShape);
	SlashItemDef->BeatSequence.Beats.SetNum(2);

	// 비트 0 치명적 일격 (Monster A 체력 초과 피해)
	SlashItemDef->BeatSequence.Beats[0].Actions.Add(TInstancedStruct<FBBMBeatAction>::Make<FBBMCombatAction>(EBBMActionCategory::Damage, EBBMTargetScope::EnemyFrontline, 1.0f, 100.0f));

	// 비트 1 치명적 일격 (Monster B 체력 초과 피해)
	SlashItemDef->BeatSequence.Beats[1].Actions.Add(TInstancedStruct<FBBMBeatAction>::Make<FBBMCombatAction>(EBBMActionCategory::Damage, EBBMTargetScope::EnemyFrontline, 1.0f, 100.0f));

	UBBMItemEntry* SlashEntry = NewObject<UBBMItemEntry>();
	SlashEntry->InitializeEntry(SlashItemDef);
	UDISItemStack* SlashStack = NewObject<UDISItemStack>();
	SlashStack->PushEntry(SlashEntry);

	UBBMInventory* Inven = PlayerMerchant->GetInventory();
	TestNotNull(TEXT("플레이어 인벤토리 유효 검증"), Inven);
	if (Inven != nullptr)
	{
		const FDISItemPlacementResult PlaceResult = Inven->SetSlotItemStack(0, SlashStack);
		TestTrue(TEXT("아이템 배치 성공 검증"), PlaceResult.bSuccess);
	}

	// 턴 시뮬레이션 실행 (8비트 파이프라인)
	FBBMTurnVisualResult OutTurnResult;
	const bool bSimResult = CombatMgr->SimulateTurn(OutTurnResult);
	TestTrue(TEXT("SimulateTurn 턴 시뮬레이션 정상 완료 검증"), bSimResult);
	TestTrue(TEXT("비트 결과가 수집되어야 함"), OutTurnResult.BeatResults.Num() > 0);

	// 몬스터 A 사망 및 사망 애니메이션 태그 부착 검증
	TestTrue(TEXT("몬스터 A 사망 검증"), MonsterA->IsDead());
	TestTrue(TEXT("몬스터 A에게 BBM.Animation.Death 태그가 부여되어야 함"), MonsterA->GetAbilitySystemComponent()->HasMatchingGameplayTag(BBMAnimationTags::Animation_Death));

	// 몬스터 B 사망 검증
	TestTrue(TEXT("몬스터 B 사망 검증"), MonsterB->IsDead());

	// 적군 전멸로 인한 승리 델리게이트 브로드캐스트 검증
	TestTrue(TEXT("전투 승리 델리게이트(OnCombatVictory)가 호출되어야 함"), bVictoryBroadcasted);
	TestTrue(TEXT("승리 델리게이트로 전달된 UsedEquipmentStorage에 SlashStack이 포함되어야 함"), ReceivedUsedEquipment.Contains(SlashStack));
	TestTrue(TEXT("승리 델리게이트로 전달된 RewardItems에 TestRewardId가 포함되어야 함"), ReceivedRewardItems.Contains(TestRewardId));

	// 10. 정리 및 파괴
	CombatMgr->ClearCurrentEncounter();
	CombatMgr->Deinitialize();

	PlayerMerchant->Destroy();
	MonsterA->Destroy();
	MonsterB->Destroy();
	GEngine->DestroyWorldContext(TestWorld);
	TestWorld->DestroyWorld(false);

	return true;
}

// 7. FBBMArmorAction 및 SetByCaller 강화 GameplayEffect 단위 테스트
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMArmorActionAndEnhancementEffectTest,
	"DIS.BBM.Combat.ArmorActionAndEnhancementEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMArmorActionAndEnhancementEffectTest::RunTest(const FString& Parameters)
{
	// 1. 범용 애니메이션 및 SetByCaller 네이티브 태그 유효성 검증
	TestTrue(TEXT("Animation_Action 태그 유효성 검증"), BBMAnimationTags::Animation_Action.GetTag().IsValid());
	TestTrue(TEXT("Animation_Hit 태그 유효성 검증"), BBMAnimationTags::Animation_Hit.GetTag().IsValid());
	TestTrue(TEXT("Animation_Victory 태그 유효성 검증"), BBMAnimationTags::Animation_Victory.GetTag().IsValid());
	TestTrue(TEXT("Animation_Defeat 태그 유효성 검증"), BBMAnimationTags::Animation_Defeat.GetTag().IsValid());
	TestTrue(TEXT("Animation_Death 태그 유효성 검증"), BBMAnimationTags::Animation_Death.GetTag().IsValid());

	TestTrue(TEXT("SetByCaller_AttackPower 태그 유효성 검증"), BBMSetByCallerTags::SetByCaller_AttackPower.GetTag().IsValid());
	TestTrue(TEXT("SetByCaller_DefensePower 태그 유효성 검증"), BBMSetByCallerTags::SetByCaller_DefensePower.GetTag().IsValid());

	// 2. 테스트 월드 및 전투원 스폰
	FBBMScopedTestWorld Scope;
	UWorld* TestWorld = Scope.World;
	TestNotNull(TEXT("테스트 월드 유효 검증"), TestWorld);

	UBBMCombatManagerSubsystem* CombatMgr = TestWorld->GetSubsystem<UBBMCombatManagerSubsystem>();
	TestNotNull(TEXT("CombatManagerSubsystem 유효 검증"), CombatMgr);

	ABBMCombatantBase* Hero = Scope.SpawnCombatant();
	TestNotNull(TEXT("영웅 전투원 스폰 유효 검증"), Hero);

	// 초기 스탯 주입 (DefensePower: 20, AttackPower: 15)
	UAbilitySystemComponent* HeroASC = Hero->GetAbilitySystemComponent();
	UBBMAttributeSet* HeroAttr = Hero->GetAttributeSet();
	TestNotNull(TEXT("영웅 ASC 유효 검증"), HeroASC);
	TestNotNull(TEXT("영웅 AttrSet 유효 검증"), HeroAttr);

	HeroAttr->SetDefensePower(20.0f);
	HeroAttr->SetAttackPower(15.0f);
	HeroAttr->SetHealth(100.0f);
	HeroAttr->SetMaxHealth(100.0f);
	HeroAttr->SetArmor(0.0f);
	HeroAttr->SetTurnArmor(0.0f);

	// 레인 배치
	CombatMgr->AddCombatantToLane(EBBMCombatLane::AllyLane, Hero, 0);

	// 3. FBBMArmorAction (턴 방어도 TurnArmor 검증)
	// 계수 1.5, 기본값 10.0 -> 20.0 * 1.5 + 10.0 = 40.0
	TMap<TWeakObjectPtr<ABBMCombatantBase>, FBBMCombatantActionLog> DummyLogs;
	FBBMArmorAction TurnArmorAction(true, EBBMTargetScope::Self, 1.5f, 10.0f);
	TurnArmorAction.ExecuteAction(CombatMgr, Hero, DummyLogs);

	TestEqual(TEXT("턴 방어도 40.0 부여 검증"), HeroAttr->GetTurnArmor(), 40.0f);
	TestEqual(TEXT("지속 방어도는 0 유지 검증"), HeroAttr->GetArmor(), 0.0f);

	// 4. FBBMArmorAction (지속 방어도 Persistent Armor 검증)
	// 계수 0.5, 기본값 5.0 -> 20.0 * 0.5 + 5.0 = 15.0
	DummyLogs.Empty();
	FBBMArmorAction PersistentArmorAction(false, EBBMTargetScope::Self, 0.5f, 5.0f);
	PersistentArmorAction.ExecuteAction(CombatMgr, Hero, DummyLogs);

	TestEqual(TEXT("지속 방어도 15.0 부여 검증"), HeroAttr->GetArmor(), 15.0f);
	TestEqual(TEXT("턴 방어도는 40.0 유지 검증"), HeroAttr->GetTurnArmor(), 40.0f);

	// 5. UBBMAttackPowerEnhancementEffect SetByCaller 적용 검증 (+12.5)
	FGameplayEffectContextHandle ContextHandle = HeroASC->MakeEffectContext();
	FGameplayEffectSpecHandle AttackSpecHandle = HeroASC->MakeOutgoingSpec(UBBMAttackPowerEnhancementEffect::StaticClass(), 1.0f, ContextHandle);
	TestTrue(TEXT("AttackPower GE Spec 생성 검증"), AttackSpecHandle.IsValid());

	AttackSpecHandle.Data->SetSetByCallerMagnitude(BBMSetByCallerTags::SetByCaller_AttackPower.GetTag(), 12.5f);
	HeroASC->ApplyGameplayEffectSpecToSelf(*AttackSpecHandle.Data.Get());

	TestEqual(TEXT("공격력 15.0 + 12.5 = 27.5 증가 검증"), HeroAttr->GetAttackPower(), 27.5f);

	// 6. UBBMDefensePowerEnhancementEffect SetByCaller 적용 검증 (+8.0)
	FGameplayEffectSpecHandle DefenseSpecHandle = HeroASC->MakeOutgoingSpec(UBBMDefensePowerEnhancementEffect::StaticClass(), 1.0f, ContextHandle);
	TestTrue(TEXT("DefensePower GE Spec 생성 검증"), DefenseSpecHandle.IsValid());

	DefenseSpecHandle.Data->SetSetByCallerMagnitude(BBMSetByCallerTags::SetByCaller_DefensePower.GetTag(), 8.0f);
	HeroASC->ApplyGameplayEffectSpecToSelf(*DefenseSpecHandle.Data.Get());

	TestEqual(TEXT("방어력 20.0 + 8.0 = 28.0 증가 검증"), HeroAttr->GetDefensePower(), 28.0f);

	// 정리
	CombatMgr->ClearCurrentEncounter();
	Hero->Destroy();

	return true;
}

// 8. 비트당 FBBMBeatAnimationConfig의 AnimationStateTag 수집 및 순수 애니메이션 이벤트 발행 단위 테스트
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMAnimationStateTagCollectionTest,
	"DIS.BBM.Combat.AnimationStateTagCollection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMAnimationStateTagCollectionTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld Scope;
	UWorld* TestWorld = Scope.World;
	TestNotNull(TEXT("테스트 월드가 생성되어야 함"), TestWorld);

	UBBMCombatManagerSubsystem* CombatMgr = TestWorld->GetSubsystem<UBBMCombatManagerSubsystem>();
	TestNotNull(TEXT("BBMCombatManagerSubsystem 획득 검증"), CombatMgr);

	// 테스트용 아군 전투원 스폰
	ABBMPlayerMerchantBase* Hero = TestWorld->SpawnActor<ABBMPlayerMerchantBase>();
	TestNotNull(TEXT("Hero 스폰 검증"), Hero);
	Hero->InitializeStats();

	// 플레이어 배낭 인벤토리 초기화
	UBBMInventoryDefinition* InvenDef = NewObject<UBBMInventoryDefinition>();
	InvenDef->DefaultCapacity = 4;
	InvenDef->MaxExpandableCapacity = 4;
	InvenDef->LineBreakTileCount = 2;
	InvenDef->ExtractionOrder = EBBMItemExtractionOrder::TopToBottom_LeftToRight;
	Hero->GetInventoryComponent()->InitializeInventory(InvenDef);

	// 테스트용 적 몬스터 스폰
	ABBMAICombatantBase* Monster = TestWorld->SpawnActor<ABBMAICombatantBase>();
	TestNotNull(TEXT("Monster 스폰 검증"), Monster);
	Monster->InitializeStats();
	// 몬스터 체력을 높게 설정하여 즉시 사망 방지
	Monster->GetAbilitySystemComponent()->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), 100.0f);

	// 전투 진형 배치
	TArray<ABBMAICombatantBase*> MonsterList;
	MonsterList.Add(Monster);
	CombatMgr->InitializeBattleFormation(Hero, MonsterList);

	// 2칸 아이템 생성
	// 비트 0: 액션(Damage) + 애니메이션(ImpactWindow, BBM.Animation.Action)
	// 비트 1: 액션 없음(휴식) + 순수 애니메이션(PrepDefenseWindow, BBM.Animation.Action)
	UBBMItemDefinition* TestItemDef = NewObject<UBBMItemDefinition>();
	TestItemDef->ItemTypeTag = BBMItemTypeTags::Item_Equipment;
	TArray<FIntPoint> ItemShape;
	ItemShape.Add(FIntPoint(0, 0));
	ItemShape.Add(FIntPoint(1, 0));
	TestItemDef->Shape = FDISTileShape(ItemShape);
	TestItemDef->BeatSequence.Beats.SetNum(2);

	// 비트 0: 공격 액션 + 타격 애니메이션 태그
	TestItemDef->BeatSequence.Beats[0].Actions.Add(TInstancedStruct<FBBMBeatAction>::Make<FBBMCombatAction>(
		EBBMActionCategory::Damage,
		EBBMTargetScope::EnemyFrontline,
		1.0f,
		5.0f
	));
	TestItemDef->BeatSequence.Beats[0].Animations.Add(FBBMBeatAnimationConfig(
		EBBMVisualWindow::ImpactWindow,
		BBMAnimationTags::Animation_Action.GetTag()
	));

	// 비트 1: 액션 없음 + 준비 방어 순수 애니메이션 태그
	TestItemDef->BeatSequence.Beats[1].Animations.Add(FBBMBeatAnimationConfig(
		EBBMVisualWindow::PrepDefenseWindow,
		BBMAnimationTags::Animation_Action.GetTag()
	));

	UBBMItemEntry* ItemEntry = NewObject<UBBMItemEntry>();
	ItemEntry->InitializeEntry(TestItemDef);
	UDISItemStack* ItemStack = NewObject<UDISItemStack>();
	ItemStack->PushEntry(ItemEntry);

	UBBMInventory* Inven = Hero->GetInventory();
	TestNotNull(TEXT("플레이어 인벤토리 유효 검증"), Inven);
	if (Inven != nullptr)
	{
		const FDISItemPlacementResult PlaceResult = Inven->SetSlotItemStack(0, ItemStack);
		TestTrue(TEXT("아이템 배치 성공 검증"), PlaceResult.bSuccess);
	}

	// 턴 시뮬레이션 실행 (플레이어 비트 시퀀스가 인벤토리로부터 자동 합성됨)
	FBBMTurnVisualResult OutTurnResult;
	const bool bSimResult = CombatMgr->SimulateTurn(OutTurnResult);
	TestTrue(TEXT("SimulateTurn 정상 완료 검증"), bSimResult);
	TestTrue(TEXT("비트 결과 수집 검증"), OutTurnResult.BeatResults.Num() >= 2);

	// 비트 0 및 비트 1 애니메이션 태그 검증
	bool bFoundBeat0Anim = false;
	bool bFoundBeat1PureAnim = false;

	for (const FBBMBeatResult& BeatRes : OutTurnResult.BeatResults)
	{
		for (const FBBMVisualWindowResult& WinRes : BeatRes.WindowResults)
		{
			if (BeatRes.BeatIndex == 0 && WinRes.VisualWindow == EBBMVisualWindow::ImpactWindow)
			{
				if (const FBBMCombatantActionLog* LogPtr = WinRes.CombatLogs.Find(Hero))
				{
					if (LogPtr->AnimationTags.HasTag(BBMAnimationTags::Animation_Action))
					{
						bFoundBeat0Anim = true;
					}
				}
			}
			if (BeatRes.BeatIndex == 1 && WinRes.VisualWindow == EBBMVisualWindow::PrepDefenseWindow)
			{
				if (const FBBMCombatantActionLog* LogPtr = WinRes.CombatLogs.Find(Hero))
				{
					if (LogPtr->AnimationTags.HasTag(BBMAnimationTags::Animation_Action))
					{
						bFoundBeat1PureAnim = true;
					}
				}
			}
		}
	}

	TestTrue(TEXT("비트 0 액션 실행 시 AnimationStateTag 정상 수집 검증"), bFoundBeat0Anim);
	TestTrue(TEXT("비트 1 액션 부재 시 순수 애니메이션 이벤트 및 AnimationStateTag 정상 발행 검증"), bFoundBeat1PureAnim);

	// 전투원 및 AttributeSet 스냅샷 리플렉션 순회 헬퍼 함수 검증
	const FBBMCombatantSnapshot HeroSnapshot = Hero->CreateAttributeSnapshot();
	TestTrue(TEXT("Hero 어트리뷰트 스냅샷에 속성이 캡처되어야 함"), HeroSnapshot.AttributeValues.Num() > 0);
	TestEqual(TEXT("Hero 스냅샷 Health 값 일치 검증"), HeroSnapshot.GetAttributeValue(UBBMAttributeSet::GetHealthAttribute()), Hero->GetAttributeValue(UBBMAttributeSet::GetHealthAttribute()));
	TestTrue(TEXT("Hero 스냅샷 MaxHealth 유효 검증"), HeroSnapshot.GetAttributeValue(UBBMAttributeSet::GetMaxHealthAttribute()) > 0.0f);

	// 정리
	CombatMgr->ClearCurrentEncounter();
	Hero->Destroy();
	Monster->Destroy();

	return true;
}

/**
 * 22. 전투원 애님 인스턴스(UBBMCombatantAnimInstance) 태그 매핑 및 FBBMBeatAnimLayerRange 검증 테스트
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatantAnimInstanceMappingTest,
	"DIS.BBM.Combat.AnimInstanceTagMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMCombatantAnimInstanceMappingTest::RunTest(const FString& Parameters)
{
	// 1. FBBMBeatAnimLayerRange 유효성 검증
	const FGameplayTag SwordLayerTag = BBMAnimLayerTags::AnimLayer_Weapon_Sword.GetTag();
	FBBMBeatAnimLayerRange InvalidRangeNoTag(0, 2);
	TestFalse(TEXT("태그가 비어있는 AnimLayerRange는 IsValid() false여야 함"), InvalidRangeNoTag.IsValid());

	FBBMBeatAnimLayerRange ValidRangeWithTag(0, 2, SwordLayerTag);
	TestTrue(TEXT("0~2 비트 및 유효 태그 소유 시 AnimLayerRange는 IsValid() true여야 함"), ValidRangeWithTag.IsValid());

	FBBMBeatAnimLayerRange InvalidRangeOutOfBounds(0, 9, SwordLayerTag);
	TestFalse(TEXT("EndBeat가 8을 초과하면 IsValid() false여야 함"), InvalidRangeOutOfBounds.IsValid());

	// 2. UBBMCombatantAnimInstance 태그 조회 검증 (UAnimInstance의 Within = SkeletalMeshComponent 규격 준수)
	USkeletalMeshComponent* DummyMesh = NewObject<USkeletalMeshComponent>();
	UBBMCombatantAnimInstance* AnimInst = NewObject<UBBMCombatantAnimInstance>(DummyMesh);
	TestNotNull(TEXT("애님 인스턴스 생성 유효 검증"), AnimInst);

	TestFalse(TEXT("초기 상태에서 미등록 태그 조회는 false여야 함"), AnimInst->HasAnimLayer(SwordLayerTag));
	TestNull(TEXT("초기 상태에서 미등록 태그 클래스 조회는 nullptr이어야 함"), AnimInst->FindAnimLayerClass(SwordLayerTag));

	// 파생 테스트 클래스 또는 리플렉션/친구 클래스 대신 Protected 멤버 설정 테스트용 구조 확인
	// AnimInst의 GetAnimLayerMap은 const 참조를 반환하므로, UPROPERTY 필드를 리플렉션 또는 파생 클래스를 통해 설정 가능
	// 단, 테스트에서 리플렉션을 통해 AnimLayerMap에 등록하여 매핑 정상 동작 확인
	if (FMapProperty* MapProp = CastField<FMapProperty>(UBBMCombatantAnimInstance::StaticClass()->FindPropertyByName(FName("AnimLayerMap"))))
	{
		FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(AnimInst));
		const int32 NewIdx = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
		uint8* PairPtr = MapHelper.GetPairPtr(NewIdx);

		FGameplayTag* KeyPtr = reinterpret_cast<FGameplayTag*>(PairPtr);
		*KeyPtr = SwordLayerTag;

		TSubclassOf<UAnimInstance>* ValPtr = reinterpret_cast<TSubclassOf<UAnimInstance>*>(PairPtr + MapProp->MapLayout.ValueOffset);
		*ValPtr = UAnimInstance::StaticClass();

		MapHelper.Rehash();
	}

	TestTrue(TEXT("매핑 등록 후 HasAnimLayer는 true여야 함"), AnimInst->HasAnimLayer(SwordLayerTag));
	TestEqual(TEXT("매핑 등록 후 FindAnimLayerClass는 UAnimInstance::StaticClass()여야 함"), AnimInst->FindAnimLayerClass(SwordLayerTag).Get(), UAnimInstance::StaticClass());

	return true;
}

// 17. 1D 배틀 타일 라인 중앙점(Center Point) 기준 좌/우 대치 배치 및 상인 스폰 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatManagerCenterPointLayoutTest,
	"DIS.BBM.Combat.CenterPointFormationLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMCombatManagerCenterPointLayoutTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestNotNull(TEXT("테스트 월드 유효 검증"), TestWorld.World);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMCombatManagerSubsystem* CombatMgr = TestWorld.World->GetSubsystem<UBBMCombatManagerSubsystem>();
	TestNotNull(TEXT("CombatManagerSubsystem 유효 검증"), CombatMgr);
	if (CombatMgr == nullptr)
	{
		return false;
	}

	// 1. 기준 중앙점 더미 액터 스폰 (위치: (1000, 2000, 100), 회전: Yaw = 0)
	const FVector CenterLoc(1000.0f, 2000.0f, 100.0f);
	const FRotator CenterRot(0.0f, 0.0f, 0.0f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* CenterActor = TestWorld.World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(CenterRot, CenterLoc), SpawnParams);
	TestNotNull(TEXT("중앙점 기준 액터 스폰 유효 검증"), CenterActor);
	if (CenterActor != nullptr)
	{
		USceneComponent* SceneComp = NewObject<USceneComponent>(CenterActor);
		CenterActor->SetRootComponent(SceneComp);
		CenterActor->SetActorLocationAndRotation(CenterLoc, CenterRot);
	}

	// 2. 몬스터 1마리를 포함한 전투 인카운터 정의 생성
	UBBMAICombatantDefinition* MonsterDef = NewObject<UBBMAICombatantDefinition>();
	MonsterDef->TableKeyName = FName(TEXT("TestSlime"));

	UBBMCombatEncounterDefinition* CombatDef = NewObject<UBBMCombatEncounterDefinition>();
	CombatDef->EncounterType = EBBMEncounterType::Combat;
	CombatDef->MonsterList.Add(MonsterDef);

	FBBMRuntimeCombatEncounterData RuntimeData;
	RuntimeData.EncounterDefinition = CombatDef;

	// 3. SetupEncounter 호출 (PlayerDef는 nullptr 전달하여 기본 ABBMPlayerMerchantBase 스폰 유도)
	const bool bSetupSuccess = CombatMgr->SetupEncounter(&RuntimeData, CenterActor, nullptr);
	TestTrue(TEXT("중앙점 기준 SetupEncounter 성공 검증"), bSetupSuccess);

	// 3-1. 1D TileMap Actor 스폰 및 인스턴스 검증
	ABBM1DTileMapActor* TileMap = CombatMgr->GetActiveTileMapActor();
	TestNotNull(TEXT("SetupEncounter 시 1D TileMap Actor가 스폰되어야 함"), TileMap);
	if (TileMap != nullptr)
	{
		TestEqual(TEXT("타일 맵 액터의 위치는 중앙점 위치와 일치해야 함"), TileMap->GetActorLocation(), CenterLoc);
		TestEqual(TEXT("타일 ISM 컴포넌트의 인스턴스 수는 총 6개(아군 3 + 적군 3)여야 함"), TileMap->GetTileISMComponent()->GetInstanceCount(), 6);
	}

	// 4. 플레이어 상인(아군) 위치 및 회전 검증 (중앙점에서 -LineForward 방향으로 180cm, 적을 바라봄)
	ABBMPlayerMerchantBase* Merchant = CombatMgr->GetActivePlayerMerchant();
	TestNotNull(TEXT("스폰된 ActivePlayerMerchant 유효 검증"), Merchant);
	if (Merchant != nullptr)
	{
		const FVector PlayerLoc = Merchant->GetActorLocation();
		TestEqual(TEXT("상인 X 좌표는 CenterX - 180 이어야 함"), PlayerLoc.X, 1000.0 - 180.0);
		TestEqual(TEXT("상인 Y 좌표는 CenterY 여야 함"), PlayerLoc.Y, 2000.0);
		TestEqual(TEXT("상인 Yaw는 CenterRot(적 방향)과 일치해야 함"), Merchant->GetActorRotation().Yaw, 0.0);
		TestEqual(TEXT("아군 레인 슬롯 0에 상인이 배치되어야 함"), CombatMgr->GetFrontlineCombatant(EBBMCombatLane::AllyLane), Cast<ABBMCombatantBase>(Merchant));
	}

	// 5. 몬스터(적군) 위치 및 회전 검증 (중앙점에서 +LineForward 방향으로 180cm, 플레이어를 마주봄)
	ABBMCombatantBase* EnemyFront = CombatMgr->GetFrontlineCombatant(EBBMCombatLane::EnemyLane);
	TestNotNull(TEXT("스폰된 적 최전방 몬스터 유효 검증"), EnemyFront);
	if (EnemyFront != nullptr)
	{
		const FVector MonsterLoc = EnemyFront->GetActorLocation();
		TestEqual(TEXT("몬스터 X 좌표는 CenterX + 180 이어야 함"), MonsterLoc.X, 1000.0 + 180.0);
		TestEqual(TEXT("몬스터 Y 좌표는 CenterY 여야 함"), MonsterLoc.Y, 2000.0);
		TestEqual(TEXT("몬스터 Yaw는 플레이어를 마주보는 180도여야 함"), FMath::Abs(EnemyFront->GetActorRotation().Yaw), 180.0);
	}

	// 6. 다음 인카운터 진입 시 상인 재활용(텔레포트) 검증
	const FVector NewCenterLoc(3000.0f, 4000.0f, 100.0f);
	AActor* NewCenterActor = TestWorld.World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(CenterRot, NewCenterLoc), SpawnParams);
	if (NewCenterActor != nullptr)
	{
		USceneComponent* SceneComp2 = NewObject<USceneComponent>(NewCenterActor);
		NewCenterActor->SetRootComponent(SceneComp2);
		NewCenterActor->SetActorLocationAndRotation(NewCenterLoc, CenterRot);
	}

	const bool bSecondSetup = CombatMgr->SetupEncounter(&RuntimeData, NewCenterActor, nullptr);
	TestTrue(TEXT("두 번째 인카운터 SetupEncounter 성공 검증"), bSecondSetup);
	TestEqual(TEXT("상인 객체는 새로 생성되지 않고 기존 객체가 재활용되어야 함"), CombatMgr->GetActivePlayerMerchant(), Merchant);
	if (Merchant != nullptr)
	{
		TestEqual(TEXT("재활용된 상인은 새 중앙점 왼쪽으로 텔레포트되어야 함"), Merchant->GetActorLocation().X, 3000.0 - 180.0);
		TestEqual(TEXT("재활용된 상인 Y 좌표 일치"), Merchant->GetActorLocation().Y, 4000.0);
	}

	// 정리
	CombatMgr->ClearCurrentEncounter();
	TestNull(TEXT("ClearCurrentEncounter 호출 후 ActiveTileMapActor는 nullptr이어야 함"), CombatMgr->GetActiveTileMapActor());
	if (Merchant != nullptr)
	{
		Merchant->Destroy();
	}
	if (CenterActor != nullptr)
	{
		CenterActor->Destroy();
	}
	if (NewCenterActor != nullptr)
	{
		NewCenterActor->Destroy();
	}

	return true;
}

// 27. ABBM1DTileMapActor 타일 및 슬롯 트랜스폼 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBM1DTileMapActorTest,
	"DIS.BBM.Combat.1DTileMapActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBM1DTileMapActorTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	const FVector Origin(500.0f, 500.0f, 0.0f);
	const FRotator Rotation(0.0f, 0.0f, 0.0f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABBM1DTileMapActor* TileMap = TestWorld.World->SpawnActor<ABBM1DTileMapActor>(
		ABBM1DTileMapActor::StaticClass(),
		FTransform(Rotation, Origin),
		SpawnParams
	);

	TestNotNull(TEXT("1D TileMap Actor 스폰 성공 검증"), TileMap);
	if (TileMap == nullptr)
	{
		return false;
	}

	// 1. 기본 설정 및 슬롯 수 검증
	TestEqual(TEXT("아군 슬롯 수는 3이어야 함"), TileMap->GetSlotCount(EBBMCombatLane::AllyLane), 3);
	TestEqual(TEXT("적군 슬롯 수는 3이어야 함"), TileMap->GetSlotCount(EBBMCombatLane::EnemyLane), 3);
	TestNotNull(TEXT("TileISMComponent 유효 검증"), TileMap->GetTileISMComponent());
	TestEqual(TEXT("기본 생성 시 총 6개의 타일 인스턴스가 생성되어야 함"), TileMap->GetTileISMComponent()->GetInstanceCount(), 6);

	// 2. 슬롯 트랜스폼 대칭성 검증 (CenterOffset = 180, SlotSpacing = 120, TileSize.Z = 10 -> Z오프셋 = 5)
	const FTransform AllySlot0 = TileMap->GetSlotTransform(EBBMCombatLane::AllyLane, 0);
	const FTransform EnemySlot0 = TileMap->GetSlotTransform(EBBMCombatLane::EnemyLane, 0);

	TestEqual(TEXT("아군 0번 슬롯 X는 500 - 180 이어야 함"), AllySlot0.GetLocation().X, 500.0 - 180.0);
	TestEqual(TEXT("적군 0번 슬롯 X는 500 + 180 이어야 함"), EnemySlot0.GetLocation().X, 500.0 + 180.0);
	TestEqual(TEXT("아군 0번 슬롯 Z는 0 + 5 이어야 함"), AllySlot0.GetLocation().Z, 5.0);
	TestEqual(TEXT("적군 0번 슬롯 Z는 0 + 5 이어야 함"), EnemySlot0.GetLocation().Z, 5.0);
	TestEqual(TEXT("아군 회전은 0도(적 방향)여야 함"), AllySlot0.GetRotation().Rotator().Yaw, 0.0);
	TestEqual(TEXT("적군 회전은 180도(아군 방향)여야 함"), FMath::Abs(EnemySlot0.GetRotation().Rotator().Yaw), 180.0);

	// 3. 후열 슬롯(Index 2) 위치 검증 (Offset 180 + 2 * 120 = 420)
	const FTransform AllySlot2 = TileMap->GetSlotTransform(EBBMCombatLane::AllyLane, 2);
	const FTransform EnemySlot2 = TileMap->GetSlotTransform(EBBMCombatLane::EnemyLane, 2);
	TestEqual(TEXT("아군 2번 슬롯 X는 500 - 420 이어야 함"), AllySlot2.GetLocation().X, 500.0 - 420.0);
	TestEqual(TEXT("적군 2번 슬롯 X는 500 + 420 이어야 함"), EnemySlot2.GetLocation().X, 500.0 + 420.0);

	// 4. 타일 크기 변경 후 RebuildTiles 검증
	TileMap->TileSize = FVector(200.0f, 200.0f, 20.0f);
	TileMap->CenterOffset = 200.0f;
	TileMap->RebuildTiles();

	const FTransform NewAllySlot0 = TileMap->GetSlotTransform(EBBMCombatLane::AllyLane, 0);
	TestEqual(TEXT("새 CenterOffset 반영 확인"), NewAllySlot0.GetLocation().X, 500.0 - 200.0);
	TestEqual(TEXT("새 타일 높이(20)에 따른 Z오프셋(10) 반영 확인"), NewAllySlot0.GetLocation().Z, 10.0);

	TileMap->Destroy();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
