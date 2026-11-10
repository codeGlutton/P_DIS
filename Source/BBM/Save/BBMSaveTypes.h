/*****************************************************************//**
 * @file   BBMSaveTypes.h
 * @brief  BBM(비트 배낭 상인) 세이브 데이터 직렬화용 레코드 구조체 정의
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Item/DISItemTypes.h"
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

