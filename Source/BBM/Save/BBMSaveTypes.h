/*****************************************************************//**
 * @file   BBMSaveTypes.h
 * @brief  BBM(비트 배낭 상인) 세이브 데이터 직렬화용 레코드 구조체 정의
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Item/DISItemTypes.h"
#include "GameFramework/GameUserSettings.h"
#include "BBMSaveTypes.generated.h"

/**
 * 인벤토리 특정 슬롯의 아이템 배치 상태 직렬화 레코드
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMInventorySlotSaveRecord
{
	GENERATED_BODY()

	FBBMInventorySlotSaveRecord() :
		SlotIndex(INDEX_NONE),
		StackCount(0),
		Rotation(EDISItemRotation::Rotate0)
	{
	}

public:
	/* 슬롯 1D 인덱스 */
	UPROPERTY(Category = "Save", EditAnywhere, BlueprintReadWrite)
	int32 SlotIndex;

	/* 아이템 정적 정의 에셋 소프트 오브젝트 경로 */
	UPROPERTY(Category = "Save", EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath ItemDefinitionPath;

	/* 스택 내 아이템 개수 */
	UPROPERTY(Category = "Save", EditAnywhere, BlueprintReadWrite)
	int32 StackCount;

	/* 아이템 회전각 */
	UPROPERTY(Category = "Save", EditAnywhere, BlueprintReadWrite)
	EDISItemRotation Rotation;
};

/**
 * 전투 중 분리 보관소(UsedEquipmentStorage)에 저장된 장비 직렬화 레코드
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMEquipmentSaveRecord
{
	GENERATED_BODY()

	FBBMEquipmentSaveRecord() :
		StackCount(1),
		Rotation(EDISItemRotation::Rotate0)
	{
	}

public:
	/* 장비 정적 정의 에셋 소프트 오브젝트 경로 */
	UPROPERTY(Category = "Save", EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath ItemDefinitionPath;

	/* 스택 수량 */
	UPROPERTY(Category = "Save", EditAnywhere, BlueprintReadWrite)
	int32 StackCount;

	/* 원래 회전 상태 */
	UPROPERTY(Category = "Save", EditAnywhere, BlueprintReadWrite)
	EDISItemRotation Rotation;
};

/**
 * 유저 환경설정 전달·캐싱·대리자 통지용 스냅샷 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMUserSettingsData
{
	GENERATED_BODY()

	FBBMUserSettingsData() :
		MasterVolume(1.0f),
		BGMVolume(0.8f),
		SFXVolume(1.0f),
		DialogueVolume(1.0f),
		bEnableBeatVisualAssist(true),
		WindowMode(EWindowMode::WindowedFullscreen),
		ScreenResolution(1920, 1080),
		bVSyncEnabled(false)
	{
	}

public:
	/* 범위 및 유효성 클램핑 */
	void ClampValues()
	{
		MasterVolume = FMath::Clamp(MasterVolume, 0.0f, 1.0f);
		BGMVolume = FMath::Clamp(BGMVolume, 0.0f, 1.0f);
		SFXVolume = FMath::Clamp(SFXVolume, 0.0f, 1.0f);
		DialogueVolume = FMath::Clamp(DialogueVolume, 0.0f, 1.0f);
		ScreenResolution.X = FMath::Max(320, ScreenResolution.X);
		ScreenResolution.Y = FMath::Max(240, ScreenResolution.Y);
	}

	bool operator==(const FBBMUserSettingsData& Other) const
	{
		return FMath::IsNearlyEqual(MasterVolume, Other.MasterVolume)
			&& FMath::IsNearlyEqual(BGMVolume, Other.BGMVolume)
			&& FMath::IsNearlyEqual(SFXVolume, Other.SFXVolume)
			&& FMath::IsNearlyEqual(DialogueVolume, Other.DialogueVolume)
			&& bEnableBeatVisualAssist == Other.bEnableBeatVisualAssist
			&& WindowMode == Other.WindowMode
			&& ScreenResolution == Other.ScreenResolution
			&& bVSyncEnabled == Other.bVSyncEnabled;
	}

	bool operator!=(const FBBMUserSettingsData& Other) const
	{
		return !(*this == Other);
	}

public:
	/* 마스터 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "Settings|Audio", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MasterVolume;

	/* 배경음악(BGM) 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "Settings|Audio", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BGMVolume;

	/* 효과음(SFX) 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "Settings|Audio", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SFXVolume;

	/* 다이얼로그 볼륨 (0.0 ~ 1.0) */
	UPROPERTY(Category = "Settings|Audio", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DialogueVolume;

	/* 전투 리듬 펄스 및 박자 가이드 링 시각 보조 효과 활성화 여부 */
	UPROPERTY(Category = "Settings|Gameplay", EditAnywhere, BlueprintReadWrite)
	bool bEnableBeatVisualAssist;

	/* 창 모드 설정 (전체화면 / 테두리 없는 창모드 / 창모드) */
	UPROPERTY(Category = "Settings|Display", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EWindowMode::Type> WindowMode;

	/* 화면 해상도 (Width x Height) */
	UPROPERTY(Category = "Settings|Display", EditAnywhere, BlueprintReadWrite)
	FIntPoint ScreenResolution;

	/* 수직 동기화(VSync) 활성화 여부 */
	UPROPERTY(Category = "Settings|Display", EditAnywhere, BlueprintReadWrite)
	bool bVSyncEnabled;
};

