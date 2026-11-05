/*****************************************************************//**
 * @file   BBMStageSubsystem.h
 * @brief  BBM(비트 배낭 상인) 5회 만남 및 지역(Region) 진행 총괄 서브시스템 클래스 정의
 * @details 런 진행 라이프사이클, 5회 만남 큐, WorldSettings 스폰 포인트 연동 및 Loading UI/비동기 저장 트랜지션을 총괄함
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "Stage/BBMStageTypes.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMRegionDefinition;
class UBBMEncounterDefinition;
class UBBMPlayerCharacterDefinition;
class ABBMPlayerMerchantBase;
struct FStreamableHandle;

/* 델리게이트 선언 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBBMOnEncounterChangedSignature, const UBBMEncounterDefinition*, NewEncounter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBBMOnRegionCompletedSignature, const UBBMRegionDefinition*, ClearedRegion);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBBMOnStageTransitionEventSignature);

#include "BBMStageSubsystem.generated.h"

/**
 * BBM 5회 만남 및 지역 진행 총괄 서브시스템
 * 포켓로그 스타일의 5회 만남 순회, 방 전환 및 레벨 스폰 세팅 적용, 비동기 세이브 연동을 관리함
 */
UCLASS(meta = (DisplayName = "BBM Stage Subsystem"))
class BBM_API UBBMStageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/* 생성자 및 라이프사이클 */
public:
	UBBMStageSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* 런 시작 및 복원 */
public:
	/**
	 * 플레이어 캐릭터 정의를 바탕으로 신규 런을 시작하고 초기 5회 만남 시퀀스를 생성
	 * @param PlayerDef 플레이어 상인 캐릭터 정적 정의 에셋 (StartRegionDefinition 포함)
	 * @return 런 시작 성공 여부
	 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable)
	bool StartNewRun(const UBBMPlayerCharacterDefinition* PlayerDef);

	/* 현재 진행 상태 조회 */
public:
	/* 현재 진입 중인 지역 정적 정의 반환 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable, BlueprintPure)
	const UBBMRegionDefinition* GetCurrentRegion() const;

	/* 현재 만남 번호 반환 (1 ~ 5) */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable, BlueprintPure)
	int32 GetCurrentEncounterIndex() const;

	/* 현재 활성화된 만남 정적 정의 에셋 반환 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable, BlueprintPure)
	const UBBMEncounterDefinition* GetCurrentEncounter() const;

	/* 현재 활성화된 만남의 기본 런타임 데이터 포인터 반환 */
	const FBBMRuntimeEncounterData* GetCurrentRuntimeEncounterData() const;

	/* 현재 활성화된 만남의 파생 런타임 데이터 포인터 반환 템플릿 */
	template<typename T>
	const T* GetCurrentRuntimeEncounterDataAs() const
	{
		const int32 Index = CurrentEncounterIndex - 1;
		if (PlannedEncounters.IsValidIndex(Index) && PlannedEncounters[Index].IsValid())
		{
			return PlannedEncounters[Index].GetPtr<T>();
		}
		return nullptr;
	}

	/* 이번 지역에서 계획된 전체 5회 만남 다형성 런타임 데이터 목록 반환 */
	const TArray<TInstancedStruct<FBBMRuntimeEncounterData>>& GetPlannedEncounters() const;

	/* 이번 지역에서 계획된 전체 만남 수량 반환 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable, BlueprintPure)
	int32 GetPlannedEncountersCount() const;

	/* 5회 만남 완료 여부 확인 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable, BlueprintPure)
	bool IsCurrentRegionCompleted() const;

	/* 만남 에셋 비동기 사전 로드(Preload) */
public:
	/**
	 * 특정 만남 인덱스(1~5)의 필요한 에셋들을 UAssetManager를 통해 비동기 사전 로드(Preload)
	 * @param EncounterIndex 프리로드할 만남 인덱스 (1~5)
	 * @return 프리로드 시작 성공 여부
	 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable)
	bool PreloadEncounterAssets(int32 EncounterIndex);

	/**
	 * C++ 전용 완료 콜백 포함 오버로딩
	 * @param EncounterIndex 프리로드할 만남 인덱스 (1~5)
	 * @param OnComplete 비동기 로드 완료 시 호출될 델리게이트
	 * @return 프리로드 시작 성공 여부
	 */
	bool PreloadEncounterAssets(int32 EncounterIndex, FSimpleDelegate OnComplete);

	/* 현재 진행 중인 프리로드 스트리밍 핸들 반환 */
	TSharedPtr<FStreamableHandle> GetCurrentEncounterStreamableHandle() const;

	/* 만남 전환 및 비동기 저장 트랜지션 파이프라인 */
public:
	/**
	 * 현재 만남을 완료하고 다음 만남 또는 다음 지역 분기로 전진
	 * 만남 직전 레벨 데이터 추출 -> 비동기 파일 저장 -> 스폰/카메라 전환 -> Loading UI 해제 순차 실행
	 * @param CurrentMerchant 현재 레벨에 위치한 플레이어 상인 폰
	 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable)
	void AdvanceToNextEncounter(ABBMPlayerMerchantBase* CurrentMerchant);

	/* 만남을 현재 레벨의 WorldSettings 스폰/카메라 포인트에 적용 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable)
	bool ApplyCurrentEncounterToLevel(ABBMPlayerMerchantBase* CurrentMerchant);

	/* 지역 분기 선택 및 다음 지역 진입 */
public:
	/* 5회 만남 완료 후 유저에게 제시할 다음 지역 분기 선택지 목록 반환 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable, BlueprintPure)
	TArray<UBBMRegionDefinition*> GetNextRegionOptions(int32 MaxOptions = 3) const;

	/**
	 * 유저가 선택한 다음 지역으로 이동하여 새로운 5회 만남 큐를 생성
	 * @param InChosenRegion 유저가 선택한 다음 지역 정의 에셋
	 * @param CurrentMerchant 현재 레벨의 플레이어 상인 폰
	 * @return 지역 전환 성공 여부
	 */
	UFUNCTION(Category = "BBM|Stage", BlueprintCallable)
	bool SelectNextRegion(const UBBMRegionDefinition* InChosenRegion, ABBMPlayerMerchantBase* CurrentMerchant);

	/* 델리게이트 이벤트 */
public:
	/* 방 전환 애니메이션 및 Loading UI 표시 시작 이벤트 */
	UPROPERTY(Category = "BBM|Stage", BlueprintAssignable)
	FBBMOnStageTransitionEventSignature OnTransitionStarted;

	/* 방 전환 완료 및 Loading UI 해제 이벤트 */
	UPROPERTY(Category = "BBM|Stage", BlueprintAssignable)
	FBBMOnStageTransitionEventSignature OnTransitionFinished;

	/* 만남 변경 이벤트 */
	UPROPERTY(Category = "BBM|Stage", BlueprintAssignable)
	FBBMOnEncounterChangedSignature OnEncounterChanged;

	/* 5회 만남 완료 및 지역 클리어 이벤트 (다음 지역 분기 선택 UI 오픈용) */
	UPROPERTY(Category = "BBM|Stage", BlueprintAssignable)
	FBBMOnRegionCompletedSignature OnRegionCompleted;

	/* 내부 헬퍼 */
protected:
	/* 지역 정적 정의로부터 5회 만남 시퀀스 및 다형성 런타임 데이터 생성 */
	void GenerateRuntimeEncounters(const UBBMRegionDefinition* InRegion);

	/* 내부 프로퍼티 */
protected:
	/* 현재 진입 중인 지역 정적 정의 에셋 */
	UPROPERTY(Category = "BBM|Stage", Transient, BlueprintReadOnly)
	TObjectPtr<const UBBMRegionDefinition> CurrentRegion;

	/* 현재 만남 번호 (1 ~ 5) */
	UPROPERTY(Category = "BBM|Stage", Transient, BlueprintReadOnly)
	int32 CurrentEncounterIndex;

	/* 이번 지역에서 계획된 5회 만남 시퀀스 다형성 런타임 데이터 목록 (보물 선택지, 전투 드롭, NPC 데이터 내장) */
	UPROPERTY(Category = "BBM|Stage", Transient, BlueprintReadOnly)
	TArray<TInstancedStruct<FBBMRuntimeEncounterData>> PlannedEncounters;

	/* 런 진행용 난수 스트림 */
	FRandomStream StageRngStream;

	/* 현재 활성화 또는 사전 로드된 만남의 에셋 스트리밍 핸들 */
	TSharedPtr<FStreamableHandle> CurrentEncounterStreamableHandle;
};
