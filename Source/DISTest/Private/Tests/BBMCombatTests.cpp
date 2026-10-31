/*****************************************************************//**
 * @file   BBMCombatTests.cpp
 * @brief  BBM 전투 핵심 단위 테스트
 * @details CompositeCurveTable 기반 스탯 초기화, 비트 단위 아이템 GE 부여/원복, GDD 5.3 4단계 피해 해결 파이프라인 검증
 * @date   2026-10-31
 *********************************************************************/

#include "Misc/AutomationTest.h"
#include "AbilitySystemGlobals.h"
#include "Combat/BBMAbilitySystemGlobals.h"
#include "Combat/BBMCombatantBase.h"
#include "Combat/BBMCharacterDefinition.h"
#include "Combat/BBMTeamTypes.h"
#include "Combat/BBMCombatSettings.h"
#include "Combat/BBMCombatSimulator.h"
#include "Combat/BBMAttributeSet.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
