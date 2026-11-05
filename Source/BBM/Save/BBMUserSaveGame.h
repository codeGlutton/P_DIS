/*****************************************************************//**
 * @file   BBMUserSaveGame.h
 * @brief  BBM(비트 배낭 상인) 영구 유저 설정 및 메타 데이터 세이브 게임 클래스 정의
 * @details 게임 수명 전체에 걸쳐 유지되는 유저 오디오/그래픽 세팅 및 메타 해금 데이터를 보관함
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BBMUserSaveGame.generated.h"

/**
 * BBM 영구 유저 설정 및 메타 세이브 게임 클래스
 * 슬롯명 "BBMUserSettings"로 단일 파일 영구 저장됨
 */
UCLASS(BlueprintType, meta = (DisplayName = "BBM User Save Game"))
class BBM_API UBBMUserSaveGame : public USaveGame
{
	GENERATED_BODY()

	/* 정적 상수 */
public:
	/* 유저 세팅 세이브 슬롯 고정 이름 */
	static const FString SlotName;

	/* 유저 세팅 세이브 사용자 인덱스 */
	static constexpr int32 UserIndex = 0;

	/* 생성자 */
public:
	UBBMUserSaveGame();

	/* 프로퍼티 */
public:
	/* 마스터 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "Settings|Audio", EditAnywhere, BlueprintReadWrite)
	float MasterVolume;

	/* 배경음악(BGM) 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "Settings|Audio", EditAnywhere, BlueprintReadWrite)
	float BGMVolume;

	/* 효과음(SFX) 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "Settings|Audio", EditAnywhere, BlueprintReadWrite)
	float SFXVolume;

	/* 해금된 플레이어 캐릭터 PrimaryAssetId 목록 */
	UPROPERTY(Category = "Meta|Unlock", EditAnywhere, BlueprintReadWrite)
	TArray<FPrimaryAssetId> UnlockedCharacterIds;

	/* 누적 런 시도 횟수 */
	UPROPERTY(Category = "Meta|Statistics", EditAnywhere, BlueprintReadWrite)
	int32 TotalRunsAttempted;

	/* 누적 런 최종 클리어 횟수 */
	UPROPERTY(Category = "Meta|Statistics", EditAnywhere, BlueprintReadWrite)
	int32 TotalRunsCleared;
};

