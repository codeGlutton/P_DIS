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
#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Inventory/BBMInventoryComponent.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Inventory/BBMInventory.h"
#include "Item/DISItemDefinition.h"
#include "Item/DISItemEntry.h"
#include "Combat/Team/BBMTeamTypes.h"
#include "Combat/Team/BBMCombatSettings.h"
#include "Combat/BBMCombatSimulator.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "GameFeatures/GameFeatureAction_AddBBMGlobalDefinition.h"
#include "Item/DISItemStack.h"
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

	FBBMBeatEffectRange BuffRange(0, 2);
	BuffRange.GrantedEffects.Add(BuffConfig);
	ItemEntry->AddEffectRange(BuffRange);

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

// 9. 플레이어 상인(ABBMPlayerMerchantBase) Definition 주입 및 인벤토리 컴포넌트 자동 초기화 검증
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

// 10. UBBMInventoryComponent 장비 일시 보관소(UsedEquipmentStorage) 아카이브, 유저 지정 슬롯 복원 및 수동 분리 검증
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
	// 유저가 슬롯 7을 선택하여 복원 시도
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
	// 슬롯 1, 2, 3, 5, 6, 7, 8, 9, 10, 11에 더미 아이템 채우기 -> 슬롯 0과 슬롯 4(세로 2칸)만 비어있음
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

// 11. ABBMAICombatantBase 패턴 시퀀스 보관, 랜덤 갱신 및 GetCurrentBeatSequence 검증
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

	// 3. Definition 주입 -> InitializeFromDefinition() 내부에서 자동 1턴 패턴 선정 검증
	AICombatant->SetCharacterDefinition(AIDef);

	TestNotNull(TEXT("정의 주입 후 SelectedPattern이 선정되어 있어야 함"), AICombatant->GetSelectedPattern());
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

// 12. ABBMPlayerMerchantBase ExtractionOrder 기반 8비트 시퀀스 합성 검증
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

	// 빈 인벤토리 시 기본 8비트 빈 시퀀스 반환 검증
	const FBBMBeatSequence EmptySeq = Merchant->GetCurrentBeatSequence();
	TestEqual(TEXT("기본 8비트 슬롯 확보 검증"), EmptySeq.Beats.Num(), 8);
	TestEqual(TEXT("효과 범위는 0개여야 함"), EmptySeq.EffectRanges.Num(), 0);

	// GenerateBeatSequenceFromInventory 직접 호출 검증
	const FBBMBeatSequence DirectEmptySeq = Merchant->GenerateBeatSequenceFromInventory();
	TestEqual(TEXT("GenerateBeatSequenceFromInventory 기본 인자 호출 일치 검증"), DirectEmptySeq.Beats.Num(), 8);

	// 2. 가로 2x1 아이템 A 생성 (2비트 차지, EffectRange [0, 2))
	UBBMItemDefinition* ItemDefA = NewObject<UBBMItemDefinition>();
	TArray<FIntPoint> ShapeA;
	ShapeA.Add(FIntPoint(0, 0));
	ShapeA.Add(FIntPoint(1, 0));
	ItemDefA->Shape = FDISTileShape(ShapeA);
	ItemDefA->BeatSequence.Beats.SetNum(2);
	ItemDefA->BeatSequence.EffectRanges.Add(FBBMBeatEffectRange(0, 2));

	UBBMItemEntry* EntryA = NewObject<UBBMItemEntry>();
	EntryA->InitializeEntry(ItemDefA);
	UDISItemStack* StackA = NewObject<UDISItemStack>();
	StackA->PushEntry(EntryA);

	// 3. 가로 2x1 아이템 B 생성 (2비트 차지, EffectRange [0, 2))
	UBBMItemDefinition* ItemDefB = NewObject<UBBMItemDefinition>();
	TArray<FIntPoint> ShapeB;
	ShapeB.Add(FIntPoint(0, 0));
	ShapeB.Add(FIntPoint(1, 0));
	ItemDefB->Shape = FDISTileShape(ShapeB);
	ItemDefB->BeatSequence.Beats.SetNum(2);
	ItemDefB->BeatSequence.EffectRanges.Add(FBBMBeatEffectRange(0, 2));

	UBBMItemEntry* EntryB = NewObject<UBBMItemEntry>();
	EntryB->InitializeEntry(ItemDefB);
	UDISItemStack* StackB = NewObject<UDISItemStack>();
	StackB->PushEntry(EntryB);

	// 슬롯 0에 Item A, 슬롯 2에 Item B 배치 (TopToBottom_LeftToRight 정렬 시 A -> B 순서)
	Inventory->SetSlotItemStack(0, StackA);
	Inventory->SetSlotItemStack(2, StackB);

	// 배치 직후 아직 UpdateBeatSequence() 호출 전에는 이전 캐시(효과 범위 0개) 상태여야 함
	TestEqual(TEXT("Update 전 GetCurrentBeatSequence는 이전 캐시 유지"), Merchant->GetCurrentBeatSequence().EffectRanges.Num(), 0);

	// 4. UpdateBeatSequence() 호출 -> 소유 인벤토리로부터 누적 합성 및 캐시 갱신
	Merchant->UpdateBeatSequence();

	// GetCurrentBeatSequence() 호출 -> 캐시된 8비트 시퀀스 검증
	const FBBMBeatSequence CompiledSeq = Merchant->GetCurrentBeatSequence();
	TestEqual(TEXT("합성된 시퀀스는 8비트여야 함"), CompiledSeq.Beats.Num(), 8);
	TestEqual(TEXT("합성된 시퀀스는 총 2개의 EffectRange를 포함해야 함"), CompiledSeq.EffectRanges.Num(), 2);

	if (CompiledSeq.EffectRanges.Num() == 2)
	{
		// Item A의 범위: [0, 2)
		TestEqual(TEXT("Item A 시작 비트는 0이어야 함"), CompiledSeq.EffectRanges[0].StartBeat, 0);
		TestEqual(TEXT("Item A 종료 비트는 2여야 함"), CompiledSeq.EffectRanges[0].EndBeat, 2);

		// Item B의 범위: 오프셋 2 누적으로 [2, 4)
		TestEqual(TEXT("Item B 시작 비트는 2여야 함"), CompiledSeq.EffectRanges[1].StartBeat, 2);
		TestEqual(TEXT("Item B 종료 비트는 4여야 함"), CompiledSeq.EffectRanges[1].EndBeat, 4);
	}

	// 5. GDD 4.1 8비트 초과 Overflow 규칙 검증:
	// 슬롯 4에 5비트 아이템 C를 배치하면, 누적 2 + 2 + 5 = 9 > 8 이므로 C는 제외되어야 함
	UBBMItemDefinition* ItemDefC = NewObject<UBBMItemDefinition>();
	TArray<FIntPoint> ShapeC;
	for (int32 i = 0; i < 5; ++i) { ShapeC.Add(FIntPoint(i % 4, i / 4)); }
	ItemDefC->Shape = FDISTileShape(ShapeC);
	ItemDefC->BeatSequence.Beats.SetNum(5);
	ItemDefC->BeatSequence.EffectRanges.Add(FBBMBeatEffectRange(0, 5));

	UBBMItemEntry* EntryC = NewObject<UBBMItemEntry>();
	EntryC->InitializeEntry(ItemDefC);
	UDISItemStack* StackC = NewObject<UDISItemStack>();
	StackC->PushEntry(EntryC);

	Inventory->SetSlotItemStack(4, StackC);

	// GenerateBeatSequenceFromInventory() 직접 호출로 갱신 및 캐시 검증
	const FBBMBeatSequence OverflowSeq = Merchant->GenerateBeatSequenceFromInventory();
	TestEqual(TEXT("8비트 초과 시 Overflow 아이템은 제외되어 여전히 2개 범위만 유지되어야 함"), OverflowSeq.EffectRanges.Num(), 2);
	TestEqual(TEXT("GetCurrentBeatSequence도 동일하게 2개 범위 캐시를 반환해야 함"), Merchant->GetCurrentBeatSequence().EffectRanges.Num(), 2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS