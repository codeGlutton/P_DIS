/*****************************************************************//**
 * @file   BBMRegionDefinition.h
 * @brief  BBM(비트 배낭 상인) 지역(Region) 정적 정의 데이터 에셋 클래스 정의
 * @details 5회 만남 풀, 로드할 레벨, 다음 연결 지역 분기를 관리하는 정적 정의 에셋
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Stage/BBMStageTypes.h"
#include "BBMRegionDefinition.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UWorld;
class UBBMEncounterDefinition;
class UBBMCombatEncounterDefinition;
class UBBMEventEncounterDefinition;
class UBBMTreasureEncounterDefinition;
class UBBMItemDefinition;
class UBBMGlobalLogic;

/**
 * BBM 지역 에셋 식별자 네임스페이스
 */
namespace BBMPrimaryAssetTypes
{
	/* BBM 지역 정의 에셋 식별자 ("BBMRegion") */
	constexpr const TCHAR* Region = TEXT("BBMRegion");
}

/**
 * BBM 지역(Region) 정적 정의 데이터 에셋 클래스
 * 특정 지역의 레벨 참조, 5회 만남 후보 풀, 5회차 보스 만남 풀, 다음 진입 가능한 연결 지역 후보를 정의함
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Region Definition"))
class BBM_API UBBMRegionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMRegionDefinition();

	/* PrimaryAssetId 오버라이드 */
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* 5회 만남 시퀀스 생성 헬퍼 */
public:
	/**
	 * 해당 지역의 만남 풀(가중치)과 보스 풀을 기반으로 5회 만남 시퀀스를 무작위 생성
	 * @param Stream 난수 결정에 사용할 랜덤 스트림
	 * @return 5개의 만남 정의 포인터 배열 (1~4회: 일반 풀 무작위, 5회: BossEncountersPool 우선 무작위)
	 */
	UFUNCTION(Category = "BBM|Region", BlueprintCallable)
	TArray<UBBMEncounterDefinition*> GenerateFiveEncounters(const FRandomStream& Stream) const;

	/**
	 * 5회 만남 완료 후 유저에게 제시할 다음 지역 분기 선택지 목록 반환
	 * @param MaxOptions 최대 제시할 선택지 개수 (기본 3개)
	 * @param Stream 무작위 셔플용 랜덤 스트림
	 * @return 유저 선택지용 다음 지역 에셋 목록 (동기 로드되어 반환)
	 */
	UFUNCTION(Category = "BBM|Region", BlueprintCallable)
	TArray<UBBMRegionDefinition*> GetNextRegionOptions(int32 MaxOptions, const FRandomStream& Stream) const;

	/* 런타임 추첨 및 희귀도 가중치 헬퍼 */
public:
	/**
	 * 해당 지역의 유효 희귀도 가중치 맵 반환 (미설정 시 기본 4대 희귀도 가중치 사용)
	 * @return 적용할 희귀도별 가중치 맵
	 */
	UFUNCTION(Category = "BBM|Region|Item", BlueprintCallable, BlueprintPure)
	TMap<FGameplayTag, float> GetEffectiveRarityWeights() const;

	/**
	 * 해당 지역의 유효 NPC 요구 아이템 타입 풀 반환 (미설정 시 GlobalLogic 리프 태그 사용)
	 * @param GlobalLogic 전역 로직 서브오브젝트
	 * @return 적용할 NPC 요구 아이템 타입 태그 목록
	 */
	UFUNCTION(Category = "BBM|Region|NPC", BlueprintCallable, BlueprintPure)
	TArray<FGameplayTag> GetEffectiveNpcItemTypePool(const UBBMGlobalLogic* GlobalLogic) const;

	/**
	 * 아이템 풀에서 희귀도 가중치 기반으로 단일 아이템 추첨
	 * @param InPool 추첨 대상 아이템 풀
	 * @param InWeights 적용할 희귀도 가중치 맵 (미등록 희귀도는 가중치 0%로 배제)
	 * @param Stream 난수 스트림
	 * @return 추첨된 아이템 정의 포인터 (추첨 실패 시 nullptr)
	 */
	UFUNCTION(Category = "BBM|Region|Item", BlueprintCallable)
	UBBMItemDefinition* PickRandomItem(const TArray<TSoftObjectPtr<UBBMItemDefinition>>& InPool, const TMap<FGameplayTag, float>& InWeights, const FRandomStream& Stream) const;

	/**
	 * 아이템 풀에서 희귀도 가중치 기반으로 중복 없이 N개 아이템 추첨 (보물 방 선택지용)
	 * @param InPool 추첨 대상 아이템 풀
	 * @param InWeights 적용할 희귀도 가중치 맵
	 * @param Count 추첨할 개수
	 * @param Stream 난수 스트림
	 * @return 추첨된 아이템 정의 포인터 배열 (최대 Count개)
	 */
	UFUNCTION(Category = "BBM|Region|Item", BlueprintCallable)
	TArray<UBBMItemDefinition*> PickRandomItemsWithoutDuplicates(const TArray<TSoftObjectPtr<UBBMItemDefinition>>& InPool, const TMap<FGameplayTag, float>& InWeights, int32 Count, const FRandomStream& Stream) const;

	/**
	 * 특정 희귀도 태그의 가중치 조회 (미등록 시 0.0f 반환)
	 * @param InRarityTag 희귀도 태그
	 * @param InWeights 가중치 맵
	 * @return 적용 가중치
	 */
	UFUNCTION(Category = "BBM|Region|Item", BlueprintCallable, BlueprintPure)
	static float GetRarityWeight(const FGameplayTag& InRarityTag, const TMap<FGameplayTag, float>& InWeights);

	/* Getter 헬퍼 */
public:
	/* 지역 표시명 반환 */
	const FText& GetRegionName() const;

	/* 지역 레벨 소프트 레퍼런스 반환 */
	const TSoftObjectPtr<UWorld>& GetLevelReference() const;

	/* 등록된 일반 전투 만남 후보 풀 반환 */
	const TArray<TSoftObjectPtr<UBBMCombatEncounterDefinition>>& GetCombatEncountersPool() const;

	/* 등록된 모험가 조우/이벤트 만남 후보 풀 반환 */
	const TArray<TSoftObjectPtr<UBBMEventEncounterDefinition>>& GetEventEncountersPool() const;

	/* 등록된 보물 상자 만남 후보 풀 반환 */
	const TArray<TSoftObjectPtr<UBBMTreasureEncounterDefinition>>& GetTreasureEncountersPool() const;

	/* 등록된 5회차 보스 만남 후보 풀 반환 */
	const TArray<TSoftObjectPtr<UBBMCombatEncounterDefinition>>& GetBossEncountersPool() const;

	/* 유효 인카운터 타입 가중치 맵 반환 (미설정 시 기본 Combat 60%, Event 20%, Treasure 20%) */
	UFUNCTION(Category = "BBM|Region|Encounters", BlueprintCallable, BlueprintPure)
	TMap<EBBMEncounterType, float> GetEffectiveEncounterTypeWeights() const;

	/* 다음 이동 가능한 지역 후보 소프트 레퍼런스 목록 반환 */
	const TArray<TSoftObjectPtr<UBBMRegionDefinition>>& GetNextPossibleRegions() const;

	/* 프로퍼티 */
public:
	/* 지역 명칭 */
	UPROPERTY(Category = "Region", EditDefaultsOnly, BlueprintReadOnly)
	FText RegionName;

	/* 해당 지역에서 사용할 레벨(맵) 소프트 오브젝트 레퍼런스 */
	UPROPERTY(Category = "Region", EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> LevelReference;

	/* 일반 만남(1~4회차) 생성 시 인카운터 타입별 출현 가중치 (미설정 시 기본 Combat 60%, Event 20%, Treasure 20%) */
	UPROPERTY(Category = "Region|Encounters", EditDefaultsOnly, BlueprintReadOnly)
	TMap<EBBMEncounterType, float> EncounterTypeWeights;

	/* 일반 전투 만남 후보 풀 (1~4회차용, 풀 후보이므로 자동 로드 방지를 위해 번들 미포함) */
	UPROPERTY(Category = "Region|Encounters", EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UBBMCombatEncounterDefinition>> CombatEncountersPool;

	/* 모험가 조우/이벤트 만남 후보 풀 (1~4회차용, 풀 후보이므로 자동 로드 방지를 위해 번들 미포함) */
	UPROPERTY(Category = "Region|Encounters", EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UBBMEventEncounterDefinition>> EventEncountersPool;

	/* 보물 상자 만남 후보 풀 (1~4회차용, 풀 후보이므로 자동 로드 방지를 위해 번들 미포함) */
	UPROPERTY(Category = "Region|Encounters", EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UBBMTreasureEncounterDefinition>> TreasureEncountersPool;

	/* 5회차에 등장할 보스 만남 후보 풀 (풀 후보이므로 번들 미포함, 비어있을 경우 CombatEncountersPool에서 대체 추출) */
	UPROPERTY(Category = "Region|Encounters", EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UBBMCombatEncounterDefinition>> BossEncountersPool;

	/* 5회 만남 클리어 시 유저 분기 선택지로 제시 가능한 다음 지역 후보 목록 (번들 미포함, 필요 시 동기/비동기 로드) */
	UPROPERTY(Category = "Region|Progression", EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UBBMRegionDefinition>> NextPossibleRegions;

	/* 해당 지역에서 등장 가능한 기본 아이템 풀 (풀 후보이므로 자동 로드 방지를 위해 번들 미포함) */
	UPROPERTY(Category = "Region|Items", EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UBBMItemDefinition>> AvailableItems;

	/* 용사 만남 방(NPC 조우)에서 등장 가능한 외부 유입 아이템 풀 (풀 후보이므로 자동 로드 방지를 위해 번들 미포함) */
	UPROPERTY(Category = "Region|Items", EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UBBMItemDefinition>> InflowItems;

	/* 희귀도별 등장 확률 가중치 맵 (비어있으면 GlobalLogic 기본값 사용, 미등록 등급은 0% 취급) */
	UPROPERTY(Category = "Region|Items", EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, float> RarityWeights;

	/* NPC들이 요구/선호하는 아이템 타입 태그 풀 (비어있으면 GlobalLogic의 리프 태그 목록 사용) */
	UPROPERTY(Category = "Region|NPC", EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGameplayTag> NpcDesiredItemTypePool;
};
