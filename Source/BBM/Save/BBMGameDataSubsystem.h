/*****************************************************************//**
 * @file   BBMGameDataSubsystem.h
 * @brief  BBM(비트 배낭 상인) 영속 게임 데이터(PersistData) 및 비동기 파일 저장 총괄 서브시스템 클래스 정의
 * @details 유저 설정(UBBMUserSaveGame)과 단일 런 세이브(UBBMRunSaveGame)의 수명 관리 및 비동기 I/O 파이프라인을 전담함
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "Stage/BBMStageTypes.h"

/* 델리게이트 선언 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FBBMAsyncSaveCompleteDynamicDelegate, bool, bSuccess);
DECLARE_MULTICAST_DELEGATE_OneParam(FBBMSaveOperationEvent, bool /* bSuccess */);

#include "BBMGameDataSubsystem.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class USaveGame;
class UBBMUserSaveGame;
class UBBMRunSaveGame;
class ABBMPlayerMerchantBase;

/**
 * BBM 영속 게임 데이터(PersistData) 관리 및 비동기 직렬화 전담 서브시스템
 */
UCLASS(meta = (DisplayName = "BBM Game Data Subsystem"))
class BBM_API UBBMGameDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/* 생성자 및 서브시스템 라이프사이클 */
public:
	UBBMGameDataSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* 데이터 접근자 */
public:
	/* 유효하게 진행 중인 런이 존재하는지 여부 반환 (타이틀 이어하기 버튼용) */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable, BlueprintPure)
	bool HasActiveRun() const;

	/* 영구 유저 설정 및 메타 데이터 객체 반환 */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable, BlueprintPure)
	UBBMUserSaveGame* GetUserSaveData() const;

	/* 현재 런 세이브 데이터 객체 반환 */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable, BlueprintPure)
	UBBMRunSaveGame* GetCurrentRunData() const;

	/* 레벨 객체로부터 런 데이터 추출 및 갱신 */
public:
	/**
	 * 현재 레벨의 플레이어 상인 및 인벤토리 객체로부터 런 상태를 추출하여 CurrentRunData를 갱신
	 * @param InMerchant 현재 레벨에 스폰되어 있는 플레이어 상인 폰
	 * @param InCurrentRegionId 현재 머무는 지역 에셋 PrimaryAssetId
	 * @param InNextEncounterIndex 다음 만남 인덱스 번호 (1~5)
	 * @param InPlannedEncounters 계획된 5회 만남 시퀀스 에셋 ID 목록
	 * @param InRngSeed 현재 재현 난수 시드
	 */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable)
	void ExtractLevelData(
		const ABBMPlayerMerchantBase* InMerchant,
		const FPrimaryAssetId& InCurrentRegionId,
		int32 InNextEncounterIndex,
		const TArray<FPrimaryAssetId>& InPlannedEncounters,
		int32 InRngSeed
	);

	/* C++ 전용: 다형성 런타임 인카운터 데이터 포함 레벨 데이터 추출 및 갱신 */
	void ExtractLevelData(
		const ABBMPlayerMerchantBase* InMerchant,
		const FPrimaryAssetId& InCurrentRegionId,
		int32 InNextEncounterIndex,
		const TArray<FPrimaryAssetId>& InPlannedEncounters,
		int32 InRngSeed,
		const TArray<TInstancedStruct<FBBMRuntimeEncounterData>>& InRuntimeEncounters
	);

	/* 비동기 파일 저장 및 로드 파이프라인 */
public:
	/* 현재 런 세이브 데이터를 파일에 비동기로 저장 (C++ 델리게이트 지원) */
	void SaveCurrentRunAsync(FSimpleDelegate OnComplete = FSimpleDelegate());

	/* 현재 런 세이브 데이터를 파일에 비동기로 저장 (블루프린트 지원) */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable, meta = (DisplayName = "SaveCurrentRunAsync"))
	void K2_SaveCurrentRunAsync(FBBMAsyncSaveCompleteDynamicDelegate OnComplete);

	/* 현재 런 세이브 데이터를 파일에서 비동기로 로드 (C++ 지원) */
	void LoadCurrentRunAsync(FSimpleDelegate OnComplete = FSimpleDelegate());

	/* 현재 런 세이브 데이터를 파일에서 비동기로 로드 (블루프린트 지원) */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable, meta = (DisplayName = "LoadCurrentRunAsync"))
	void K2_LoadCurrentRunAsync(FBBMAsyncSaveCompleteDynamicDelegate OnComplete);

	/* 영구 유저 설정 데이터를 파일에 비동기로 저장 */
	void SaveUserSettingAsync(FSimpleDelegate OnComplete = FSimpleDelegate());

	/* 영구 유저 설정 데이터를 파일에서 비동기로 로드 */
	void LoadUserSettingAsync(FSimpleDelegate OnComplete = FSimpleDelegate());

	/* 게임 오버 또는 최종 클리어 시 런 데이터 삭제 */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable)
	void ClearCurrentRunData();

	/* 세이브 파일 안전 백업 및 복구 헬퍼 */
public:
	/* 슬롯 이름에 해당하는 실제 .sav 파일 절대 경로 반환 */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable, BlueprintPure)
	static FString GetSaveFilePath(const FString& InSlotName);

	/* 슬롯 이름에 해당하는 백업 .bak 파일 절대 경로 반환 */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable, BlueprintPure)
	static FString GetBackupFilePath(const FString& InSlotName);

	/**
	 * 저장 전 안전 백업 처리: 기존 .sav 파일이 디스크에 존재하면 .bak 파일로 변환
	 * @param InSlotName 슬롯 이름
	 * @return 백업 변환 수행 여부 (기존 파일이 없으면 false)
	 */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable)
	static bool PrepareBackupBeforeSave(const FString& InSlotName);

	/**
	 * 로드 전 백업 복구 처리: 기존 .sav 파일이 없고 .bak 파일이 디스크에 존재하면 .bak을 .sav로 복사 복구
	 * @param InSlotName 슬롯 이름
	 * @return 복구 성공 여부
	 */
	UFUNCTION(Category = "BBM|Save", BlueprintCallable)
	static bool RestoreFromBackupIfNeeded(const FString& InSlotName);

	/* 내부 비동기 저장 및 로드 파이프라인 */
protected:
	/**
	 * USaveGame 객체를 메모리 직렬화 후 백그라운드 스레드에서 백업 복제 및 비동기 파일 저장
	 * @param InSaveGame 직렬화할 세이브 게임 객체
	 * @param InSlotName 슬롯 이름
	 * @param InUserIndex 사용자 인덱스
	 * @param OnComplete 비동기 저장 완료 시 메인 스레드에서 호출될 콜백
	 */
	void SaveGameAsyncInternal(USaveGame* InSaveGame, const FString& InSlotName, int32 InUserIndex, TFunction<void(bool)> OnComplete);

	/**
	 * 백그라운드 스레드에서 파일 확인 및 백업 복구 후 바이트를 읽어 메인 스레드에서 역직렬화
	 * @param InSlotName 슬롯 이름
	 * @param InUserIndex 사용자 인덱스
	 * @param OnComplete 비동기 로드 완료 시 메인 스레드에서 호출될 콜백
	 */
	void LoadGameAsyncInternal(const FString& InSlotName, int32 InUserIndex, TFunction<void(USaveGame*, bool)> OnComplete);

	/* 멤버 프로퍼티 */
protected:
	/* 게임 전체에 걸쳐 유지되는 영구 유저 설정 및 메타 데이터 */
	UPROPERTY(Category = "BBM|Save", Transient, BlueprintReadOnly)
	TObjectPtr<UBBMUserSaveGame> UserSaveData;

	/* 현재 게임 트라이(Run) 동안 유지되는 단일 런 세이브 데이터 */
	UPROPERTY(Category = "BBM|Save", Transient, BlueprintReadOnly)
	TObjectPtr<UBBMRunSaveGame> CurrentRunData;

	/* 현재 비동기 세이브/로드가 진행 중인지 여부 */
	UPROPERTY(Category = "BBM|Save", Transient, BlueprintReadOnly)
	bool bIsAsyncOperationInProgress;
};
