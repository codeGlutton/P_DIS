/*****************************************************************//**
 * @file   BBMRunSaveGame.h
 * @brief  BBM(비트 배낭 상인) 단일 런(Run) 진행 데이터 세이브 게임 클래스 정의
 * @details 진행 중인 한 판의 트라이 상태(플레이어 스탯, 배낭 배치, 5회 만남 및 지역)를 단일 슬롯에 보관함
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Save/BBMSaveTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Stage/BBMStageTypes.h"
#include "BBMRunSaveGame.generated.h"

/**
 * BBM 현재 런 진행 상태 세이브 게임 클래스
 * 슬롯명 "BBMCurrentRun"으로 단일 파일로 저장되며 타이틀 화면에서 이어하기를 지원함
 */
UCLASS(BlueprintType, meta = (DisplayName = "BBM Run Save Game"))
class BBM_API UBBMRunSaveGame : public USaveGame
{
	GENERATED_BODY()

	/* 정적 상수 */
public:
	/* 현재 런 세이브 슬롯 고정 이름 */
	static const FString SlotName;

	/* 현재 런 세이브 사용자 인덱스 */
	static constexpr int32 UserIndex = 0;

	/* 생성자 */
public:
	UBBMRunSaveGame();

	/* 초기화/클리어 */
	void ResetRunData();

	/* 프로퍼티 */
public:
	/* 진행 중인 런이 유효하게 존재하는지 여부 */
	UPROPERTY(Category = "Run|State", EditAnywhere, BlueprintReadWrite)
	bool bHasActiveRun;

	/* 플레이어 캐릭터 PrimaryAssetId */
	UPROPERTY(Category = "Run|Player", EditAnywhere, BlueprintReadWrite)
	FPrimaryAssetId PlayerCharacterId;

	/* 플레이어 현재 체력 */
	UPROPERTY(Category = "Run|Player", EditAnywhere, BlueprintReadWrite)
	float CurrentHealth;

	/* 플레이어 최대 체력 */
	UPROPERTY(Category = "Run|Player", EditAnywhere, BlueprintReadWrite)
	float MaxHealth;

	/* 플레이어 소지 골드 */
	UPROPERTY(Category = "Run|Player", EditAnywhere, BlueprintReadWrite)
	int32 CurrentGold;

	/* 배낭 인벤토리 슬롯 배치 상태 목록 */
	UPROPERTY(Category = "Run|Inventory", EditAnywhere, BlueprintReadWrite)
	TArray<FBBMInventorySlotSaveRecord> SavedInventorySlots;

	/* 전투 중 분리 보관된 장비(UsedEquipmentStorage) 목록 */
	UPROPERTY(Category = "Run|Inventory", EditAnywhere, BlueprintReadWrite)
	TArray<FBBMEquipmentSaveRecord> ArchivedEquipments;

	/* 현재 진입 중인 지역 PrimaryAssetId */
	UPROPERTY(Category = "Run|Progression", EditAnywhere, BlueprintReadWrite)
	FPrimaryAssetId CurrentRegionId;

	/* 현재 만남 번호 (1 ~ 5) */
	UPROPERTY(Category = "Run|Progression", EditAnywhere, BlueprintReadWrite)
	int32 CurrentEncounterIndex;

	/* 이번 지역에서 계획된 5회 만남 시퀀스 PrimaryAssetId 목록 */
	UPROPERTY(Category = "Run|Progression", EditAnywhere, BlueprintReadWrite)
	TArray<FPrimaryAssetId> PlannedEncounterIds;

	/* 이번 지역에서 계획된 5회 만남의 다형성 런타임 데이터 (보물 선택지, 전투 드롭, NPC 소지/요구) */
	UPROPERTY(Category = "Run|Progression", EditAnywhere, BlueprintReadWrite)
	TArray<TInstancedStruct<FBBMRuntimeEncounterData>> SavedPlannedEncounters;

	/* 재현용 난수 시드 */
	UPROPERTY(Category = "Run|Progression", EditAnywhere, BlueprintReadWrite)
	int32 RngSeed;
};

