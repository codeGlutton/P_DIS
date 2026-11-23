/*****************************************************************//**
 * @file   BBMEncounterDefinition.h
 * @brief  BBM(비트 배낭 상인) 만남(Encounter) 정적 정의 데이터 에셋 클래스 계층 정의
 * @details 전투, 보물 상자, 모험가 조우 등 개별 만남의 공통 기반 및 도메인별 파생 에셋 정의
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Stage/BBMStageTypes.h"
#include "BBMEncounterDefinition.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMAICombatantDefinition;
class ABBMChestActorBase;
class ABBMEventNpcBase;

/**
 * BBM 만남 에셋 식별자 네임스페이스
 */
namespace BBMPrimaryAssetTypes
{
	/* BBM 만남 정의 에셋 식별자 ("BBMEncounter") */
	constexpr const TCHAR* Encounter = TEXT("BBMEncounter");
}

/**
 * BBM 만남(Encounter) 정적 정의 베이스 데이터 에셋 클래스
 * 모든 만남(전투, 보물, 모험가 조우)의 공통 속성(표시 이름, 스폰 키, 만남 유형)을 정의함
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Encounter Definition"))
class BBM_API UBBMEncounterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMEncounterDefinition();

	/* PrimaryAssetId 오버라이드 */
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* Getter 헬퍼 */
public:
	/* 만남 유형 반환 */
	EBBMEncounterType GetEncounterType() const;

	/* 월드 세팅에서 참조할 스폰/카메라 Key 반환 */
	const FName& GetSpawnSettingKey() const;

	/* 만남 표시 이름 반환 */
	const FText& GetDisplayName() const;

	/* 프로퍼티 */
public:
	/* 만남 유형 (전투, 보스, 이벤트, 보물) */
	UPROPERTY(Category = "Encounter", EditDefaultsOnly, BlueprintReadOnly)
	EBBMEncounterType EncounterType;

	/* WorldSettings의 SpawnSettings에서 조회할 스폰 및 카메라 Key 이름 */
	UPROPERTY(Category = "Encounter", EditDefaultsOnly, BlueprintReadOnly)
	FName SpawnSettingKey;

	/* UI에 표시될 만남 명칭 */
	UPROPERTY(Category = "Encounter", EditDefaultsOnly, BlueprintReadOnly)
	FText DisplayName;
};

/**
 * BBM 전투 만남(Combat Encounter) 정적 정의 데이터 에셋 클래스
 * 일반 몬스터 전투 및 지역 보스 전투에서 스폰할 AI 전투원 목록을 소프트 레퍼런스로 관리함
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Combat Encounter Definition"))
class BBM_API UBBMCombatEncounterDefinition : public UBBMEncounterDefinition
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMCombatEncounterDefinition();

	/* Getter */
public:
	/* 스폰할 AI 전투원 정적 정의 소프트 레퍼런스 목록 반환 */
	const TArray<TSoftObjectPtr<UBBMAICombatantDefinition>>& GetMonsterList() const;

	/* 프로퍼티 */
public:
	/* 전투 만남 시 룸에 스폰할 AI 전투원 정적 정의 소프트 레퍼런스 목록 */
	UPROPERTY(Category = "Combat", EditDefaultsOnly, BlueprintReadOnly, meta = (AssetBundles = "Encounter"))
	TArray<TSoftObjectPtr<UBBMAICombatantDefinition>> MonsterList;
};

/**
 * BBM 보물 만남(Treasure Encounter) 정적 정의 데이터 에셋 클래스
 * 룸 스폰 포인트에 생성할 보물 상자 액터 클래스 및 보상 테이블/아이템 풀 Key를 관리함
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Treasure Encounter Definition"))
class BBM_API UBBMTreasureEncounterDefinition : public UBBMEncounterDefinition
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMTreasureEncounterDefinition();

	/* Getter */
public:
	/* 스폰할 보물 상자 액터 클래스 소프트 레퍼런스 반환 */
	const TSoftClassPtr<ABBMChestActorBase>& GetChestClass() const;

	/* 프로퍼티 */
public:
	/* 룸 스폰 포인트에 생성할 보물 상자 액터 클래스 소프트 레퍼런스 */
	UPROPERTY(Category = "Treasure", EditDefaultsOnly, BlueprintReadOnly, meta = (AssetBundles = "Encounter"))
	TSoftClassPtr<ABBMChestActorBase> ChestClass;
};

/**
 * BBM 모험가/사람 조우 만남(Event Encounter) 정적 정의 데이터 에셋 클래스
 * 만남 방에 등장할 NPC 액터 클래스 목록을 관리함
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Event Encounter Definition"))
class BBM_API UBBMEventEncounterDefinition : public UBBMEncounterDefinition
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMEventEncounterDefinition();

	/* Getter */
public:
	/* 방에 등장할 NPC 액터 클래스 목록 반환 */
	const TArray<TSoftClassPtr<ABBMEventNpcBase>>& GetNpcClasses() const;

	/* 프로퍼티 */
public:
	/* 만남 방에 등장할 NPC 액터 클래스 목록 (기본 3종 설정 권장) */
	UPROPERTY(Category = "Event", EditDefaultsOnly, BlueprintReadOnly, meta = (AssetBundles = "Encounter"))
	TArray<TSoftClassPtr<ABBMEventNpcBase>> NpcClasses;
};
