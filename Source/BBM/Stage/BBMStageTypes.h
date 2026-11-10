/*****************************************************************//**
 * @file   BBMStageTypes.h
 * @brief  BBM(비트 배낭 상인) 지역 및 만남 관련 타입 정의
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "BBMStageTypes.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMEncounterDefinition;
class UBBMItemDefinition;
class ABBMEventNpcBase;

/**
 * 만남(Encounter) 유형
 */
UENUM(BlueprintType)
enum class EBBMEncounterType : uint8
{
	/* 일반 몬스터 전투 */
	Combat UMETA(DisplayName = "Combat"),

	/* 지역 최종 보스 전투 */
	BossCombat UMETA(DisplayName = "Boss Combat"),

	/* 모험가와의 조우 (상점, 대화, 퀘스트 등) */
	Event UMETA(DisplayName = "Event / Adventure"),

	/* 보물 상자 발견 */
	Treasure UMETA(DisplayName = "Treasure")
};

/**
 * 지역 만남 풀에서 특정 만남의 스폰 가중치를 설정하는 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMEncounterSpawnWeight
{
	GENERATED_BODY()

	/* 함수 먼저 선언 */
public:
	FBBMEncounterSpawnWeight();
	FBBMEncounterSpawnWeight(TSoftObjectPtr<UBBMEncounterDefinition> InDef, float InWeight = 1.0f);
	FBBMEncounterSpawnWeight(const UBBMEncounterDefinition* InDef, float InWeight = 1.0f);

	/* 멤버 변수 */
public:
	/* 스폰 후보 만남 정적 정의 에셋 (풀 후보이므로 번들 미지정) */
	UPROPERTY(Category = "Encounter", EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UBBMEncounterDefinition> EncounterDefinition;

	/* 만남 선택 가중치 (기본값 1.0, 높을수록 선정 확률 증가) */
	UPROPERTY(Category = "Encounter", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float SelectionWeight;
};

/**
 * 용사 만남 방(Event Encounter) 런타임 NPC 정보 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMRuntimeNpcData
{
	GENERATED_BODY()

	FBBMRuntimeNpcData();

	/* 스폰할 NPC 액터 클래스 */
	UPROPERTY(Category = "NPC", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "World"))
	TSoftClassPtr<ABBMEventNpcBase> NpcClass;

	/* NPC가 소유하고 플레이어 상인에게 제공할 수 있는 아이템 */
	UPROPERTY(Category = "NPC", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "Data"))
	TSoftObjectPtr<UBBMItemDefinition> OfferedItem;

	/* NPC가 플레이어에게 요구하는 아이템 세부 타입 태그 (NPC 3인 간 중복 없음) */
	UPROPERTY(Category = "NPC", EditAnywhere, BlueprintReadWrite)
	FGameplayTag DesiredItemTypeTag;
};

/**
 * 만남 런타임 기본 데이터 구조체 (베이스)
 * EncounterType은 중복으로 두지 않고 EncounterDefinition을 통해 편의 조회함
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMRuntimeEncounterData
{
	GENERATED_BODY()

	FBBMRuntimeEncounterData();
	virtual ~FBBMRuntimeEncounterData() = default;

	/* 만남 정적 정의 에셋 참조 (런타임 선정 데이터이므로 번들 등록) */
	UPROPERTY(Category = "Encounter", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "Data"))
	TSoftObjectPtr<UBBMEncounterDefinition> EncounterDefinition;

	/* 만남 유형 편의 조회 헬퍼 (EncounterDefinition으로부터 조회) */
	EBBMEncounterType GetEncounterType() const;

	/* 해당 만남에서 필요한 PrimaryAssetId(PAD) 목록을 수집하여 반환 (비동기 프리로드 및 스트리밍용) */
	virtual void CollectPrimaryAssetIds(TArray<FPrimaryAssetId>& OutPrimaryAssetIds) const;
};

/**
 * [보물 방] 런타임 데이터 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMRuntimeTreasureEncounterData : public FBBMRuntimeEncounterData
{
	GENERATED_BODY()

	FBBMRuntimeTreasureEncounterData();

	/* 3개의 보물 아이템 선택지 목록 */
	UPROPERTY(Category = "Treasure", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "Data"))
	TArray<TSoftObjectPtr<UBBMItemDefinition>> TreasureOptions;

	/* 유저가 최종 선택한 보물 아이템 */
	UPROPERTY(Category = "Treasure", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "Data"))
	TSoftObjectPtr<UBBMItemDefinition> SelectedRewardItem;

	virtual void CollectPrimaryAssetIds(TArray<FPrimaryAssetId>& OutPrimaryAssetIds) const override;
};

/**
 * [전투 방] 런타임 데이터 구조체 (일반 및 보스 전투 공통)
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMRuntimeCombatEncounterData : public FBBMRuntimeEncounterData
{
	GENERATED_BODY()

	FBBMRuntimeCombatEncounterData();

	/* 처치 시 지급될 사전 추첨 드롭 보상 아이템 목록 */
	UPROPERTY(Category = "Combat", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "Data"))
	TArray<TSoftObjectPtr<UBBMItemDefinition>> CombatRewardItems;

	virtual void CollectPrimaryAssetIds(TArray<FPrimaryAssetId>& OutPrimaryAssetIds) const override;
};

/**
 * [용사 만남 방] 런타임 데이터 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMRuntimeEventEncounterData : public FBBMRuntimeEncounterData
{
	GENERATED_BODY()

	FBBMRuntimeEventEncounterData();

	/* 방에 스폰될 3명의 NPC 런타임 정보 목록 */
	UPROPERTY(Category = "Event", EditAnywhere, BlueprintReadWrite)
	TArray<FBBMRuntimeNpcData> EventNpcs;

	virtual void CollectPrimaryAssetIds(TArray<FPrimaryAssetId>& OutPrimaryAssetIds) const override;
};

