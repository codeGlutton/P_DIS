/*****************************************************************//**
 * @file   BBMSpawnTypes.h
 * @brief  BBM(비트 배낭 상인) 레벨 스폰 포인트 및 카메라 설정 구조체 정의
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "BBMSpawnTypes.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class AActor;

/**
 * 특정 룸/만남에서 사용할 스폰 포인트 및 메인 카메라 액터 묶음 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMRoomSpawnSetting
{
	GENERATED_BODY()

	/* 함수 먼저 선언 */
public:
	FBBMRoomSpawnSetting() :
		MainCameraPoint(nullptr),
		RoomStartPoint(nullptr)
	{
	}

	FBBMRoomSpawnSetting(AActor* InCameraPoint, AActor* InStartPoint) :
		MainCameraPoint(InCameraPoint),
		RoomStartPoint(InStartPoint)
	{
	}

	/* 유효성 검사 헬퍼 */
	bool IsValid() const
	{
		return MainCameraPoint != nullptr && RoomStartPoint != nullptr;
	}

	/* 멤버 변수 (UPROPERTY 순서: Category -> Visible/Edit -> Blueprint -> Meta) */
public:
	/* 해당 만남 룸의 뷰를 비출 메인 카메라 액터 */
	UPROPERTY(Category = "Spawn|Camera", EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> MainCameraPoint;

	/* 플레이어 캐릭터가 스폰되거나 배치될 시작 기준 포인트 액터 */
	UPROPERTY(Category = "Spawn|Point", EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> RoomStartPoint;
};

