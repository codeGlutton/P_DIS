/*****************************************************************//**
 * @file   BBMCombatTests.cpp
 * @brief  BBM 전투 핵심 단위 테스트
 * @details CompositeCurveTable 기반 스탯 초기화, 비트 단위 아이템 GE 부여/원복, GDD 5.3 4단계 피해 해결 파이프라인 검증
 * @date   2026-10-31
 *********************************************************************/

#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "AbilitySystemGlobals.h"
#include "Combat/GAS/BBMAbilitySystemGlobals.h"
#include "Combat/Pawn/BBMCombatantBase.h"
#include "Combat/Pawn/BBMPlayerMerchantBase.h"
#include "Combat/Pawn/BBMAICombatantBase.h"
#include "Combat/Pawn/BBMCharacterDefinition.h"
#include "Combat/Pawn/BBMAICombatantDefinition.h"
#include "Combat/Team/BBMTeamTypes.h"
#include "Combat/Team/BBMCombatSettings.h"
#include "Combat/BBMCombatSimulator.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "GameFeatures/GameFeatureAction_AddBBMGlobalDefinition.h"
#include "Item/BBMItemDefinition.h"
#include "Item/BBMItemEntry.h"
#include "Item/BBMItemGameplayEffectTypes.h"
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

// 2. 비트 단위 아이템 GAS GameplayEffect 부여 및 종료 시 100% 원복 검증
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
	ItemEntry->AddDynamicGrantedEffect(BuffConfig);

	// 3. 비트 시작: 아이템 효과 캐릭터 ASC에 적용
	TArray<FActiveGameplayEffectHandle> ActiveHandles;
	const bool bApplyResult = UBBMCombatSimulator::ApplyItemGrantedEffects(Combatant, ItemEntry, ActiveHandles);
	TestTrue(TEXT("아이템 Granted Effects 적용 성공"), bApplyResult);
	TestEqual(TEXT("적용된 Active Handle 개수는 1개여야 함"), ActiveHandles.Num(), 1);

	// 버프 적용 후 공격력 검증 (10.0f + 25.0f = 35.0f)
	TestEqual(TEXT("아이템 효과 부여 후 공격력 증폭 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetAttackPowerAttribute()), 35.0f);

	// 4. 비트 종료: 핸들을 통해 활성 효과 일괄 해제
	UBBMCombatSimulator::RemoveItemGrantedEffects(Combatant, ActiveHandles);

	// 해제 후 원래 공격력(10.0f)으로 100% 원복 검증
	TestEqual(TEXT("비트 종료 후 공격력 완전 원복 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetAttackPowerAttribute()), InitialAttack);

	return true;
}

// 3. GDD 5.3 규격에 따른 엄격한 4단계 피해 처리 파이프라인 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCombatResolveDamagePipelineTest,
	"DIS.BBM.Combat.ResolveDamagePipeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMCombatResolveDamagePipelineTest::RunTest(const FString& Parameters)
{
	FBBMScopedTestWorld TestWorld;
	TestNotNull(TEXT("테스트 월드 생성 검증"), TestWorld.World);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 1. 전투원 스폰 및 스탯 설정 (체력 100, 방어도 20)
	ABBMCombatantBase* Combatant = TestWorld.SpawnCombatant();
	TestNotNull(TEXT("전투원 스폰 검증"), Combatant);
	if (Combatant == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = Combatant->GetAbilitySystemComponent();
	TestNotNull(TEXT("전투원 ASC 검증"), ASC);
	if (ASC == nullptr)
	{
		return false;
	}

	ASC->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), 100.0f);
	ASC->SetNumericAttributeBase(UBBMAttributeSet::GetMaxHealthAttribute(), 100.0f);
	ASC->SetNumericAttributeBase(UBBMAttributeSet::GetArmorAttribute(), 20.0f);

	// 2. 1단계: 패링 체크 (패링 중이면 100 피해도 완전 무효화, 방어력/체력 100% 보존)
	float ActiveGuard = 0.0f;
	float DealtDamage = UBBMCombatSimulator::ResolveDamage(Combatant, 100.0f, true, ActiveGuard);
	TestEqual(TEXT("패링 상태 시 입힌 피해량은 0이어야 함"), DealtDamage, 0.0f);
	TestEqual(TEXT("패링 성공 시 체력 보존 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetHealthAttribute()), 100.0f);
	TestEqual(TEXT("패링 성공 시 방어도 보존 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetArmorAttribute()), 20.0f);

	// 3. 2~4단계: 액티브 가드(30) + 쌓아둔 방어도(20)로 60 피해 흡수
	// - 가드 30 흡수 -> 잔여 가드 0, 잔여 피해 30
	// - 방어도 20 흡수 -> 잔여 방어도 0, 잔여 피해 10
	// - 체력 10 차감 -> 잔여 체력 90
	ActiveGuard = 30.0f;
	DealtDamage = UBBMCombatSimulator::ResolveDamage(Combatant, 60.0f, false, ActiveGuard);
	TestEqual(TEXT("가드 및 방어도 차감 후 체력에 실제 입힌 피해량은 10이어야 함"), DealtDamage, 10.0f);
	TestEqual(TEXT("액티브 가드 전량 소진 검증"), ActiveGuard, 0.0f);
	TestEqual(TEXT("쌓아둔 방어도 전량 소진 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetArmorAttribute()), 0.0f);
	TestEqual(TEXT("남은 최종 체력은 90이어야 함"), Combatant->GetAttributeValue(UBBMAttributeSet::GetHealthAttribute()), 90.0f);
	TestFalse(TEXT("전투원은 아직 생존 상태여야 함"), Combatant->IsDead());

	// 4. 치명적 피해 적용 및 사망(IsDead) 검증
	// 잔여 체력 90인 상태에서 피해 150 직격
	ActiveGuard = 0.0f;
	DealtDamage = UBBMCombatSimulator::ResolveDamage(Combatant, 150.0f, false, ActiveGuard);
	TestEqual(TEXT("체력 90 전량 소진 검증"), DealtDamage, 90.0f);
	TestEqual(TEXT("체력은 0.0f이어야 함"), Combatant->GetAttributeValue(UBBMAttributeSet::GetHealthAttribute()), 0.0f);
	TestTrue(TEXT("체력 0 소진 시 IsDead는 true여야 함"), Combatant->IsDead());

	// 5. 지속 방어도 획득(ResolveGainArmor) 검증
	UBBMCombatSimulator::ResolveGainArmor(Combatant, 50.0f);
	TestEqual(TEXT("방어도 50 획득 후 수치 검증"), Combatant->GetAttributeValue(UBBMAttributeSet::GetArmorAttribute()), 50.0f);

	return true;
}

// 4. FGenericTeamId 및 IGenericTeamAgentInterface 팀 관계 태도 검증
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
	PatternC.Sequence.EffectRanges.Add(FBBMBeatEffectRange(2, 5, FName(TEXT("RageBuffRange"))));

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
		const FBBMBeatEffectRange Range(2, 5, FName(TEXT("TestRange")));
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
		const FBBMBeatEffectRange InvertedRange(5, 2, FName(TEXT("InvertedRange")));
		TestFalse(TEXT("Start > End 범위는 IsValid() == false 여야 함"), InvertedRange.IsValid());

		const FBBMBeatEffectRange OutOfBoundsRange(0, 9, FName(TEXT("OutOfBoundsRange")));
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
	BadRangePattern.Sequence.EffectRanges.Add(FBBMBeatEffectRange(4, 2, FName(TEXT("BadRange")))); // Start > End
	AIDef->PatternPool.Add(BadRangePattern);

	FDataValidationContext BadRangeContext;
	const EDataValidationResult BadRangeResult = AIDef->IsDataValid(BadRangeContext);
	TestEqual(TEXT("비정상 EffectRange 포함 시 Invalid 판정이어야 함"), BadRangeResult, EDataValidationResult::Invalid);
#endif

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
