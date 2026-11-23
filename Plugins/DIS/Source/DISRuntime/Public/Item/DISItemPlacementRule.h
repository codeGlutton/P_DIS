/*****************************************************************//**
 * @file   DISItemPlacementRule.h
 * @brief  DIS 아이템의 슬롯 배치 제한 규칙(Placement Rule) 및 컨텍스트 정의
 * @details TInstancedStruct 기반으로 아이템이 인벤토리/슬롯에 배치될 수 있는지 평가하는 다형성 규칙
 * @date   2026-10-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DISItemPlacementRule.generated.h"

class UDISInventory;
class UDISItemEntry;

/**
 * 아이템 배치 제한 규칙 평가 시 전달되는 인벤토리 및 슬롯 환경 컨텍스트 구조체
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISItemPlacementContext
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FDISItemPlacementContext();

	/* 컨텍스트 프로퍼티 */
public:
	/* 대상 인벤토리 객체 */
	UPROPERTY(Category = "Item Placement Context", BlueprintReadOnly)
	TWeakObjectPtr<const UDISInventory> Inventory;

	/* 배치 대상 슬롯 인덱스 */
	UPROPERTY(Category = "Item Placement Context", BlueprintReadOnly)
	int32 TargetSlotIndex;

	/* 배치 대상 슬롯의 2D 타일 좌표 */
	UPROPERTY(Category = "Item Placement Context", BlueprintReadOnly)
	FIntPoint TargetCoord;

	/* 인벤토리 소유자 객체 (선택 사항) */
	UPROPERTY(Category = "Item Placement Context", BlueprintReadOnly)
	TWeakObjectPtr<const UObject> InventoryOwner;
};

/**
 * 아이템이 특정 슬롯/인벤토리 위치에 배치될 수 있는지 검증하기 위한 베이스 구조체 (TInstancedStruct 다형성 지원)
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISItemPlacementRule
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FDISItemPlacementRule() = default;
	virtual ~FDISItemPlacementRule() = default;

	/* 규칙 평가 메서드 */
public:
	/* 아이템이 지정된 슬롯 컨텍스트에 배치될 수 있는지 여부 검증 (기본값: true, 가상 함수 오버라이드 지원) */
	virtual bool CanPlaceItem(const UDISItemEntry* ItemEntry, const FDISItemPlacementContext& Context) const;
};

/**
 * 인벤토리의 모서리(Corner) 또는 BlockTile과 인접하여 구석을 형성한 슬롯에만 배치를 허용하는 제한 규칙
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISCornerPlacementRule : public FDISItemPlacementRule
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FDISCornerPlacementRule() = default;
	virtual ~FDISCornerPlacementRule() override = default;

	/* FDISItemPlacementRule 가상 함수 오버라이드 */
public:
	virtual bool CanPlaceItem(const UDISItemEntry* ItemEntry, const FDISItemPlacementContext& Context) const override;
};

/**
 * 인벤토리의 외곽 및 BlockTile과 인접하지 않은 내부 중앙(Center) 영역 슬롯에만 배치를 허용하는 제한 규칙
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISCenterPlacementRule : public FDISItemPlacementRule
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FDISCenterPlacementRule() = default;
	virtual ~FDISCenterPlacementRule() override = default;

	/* FDISItemPlacementRule 가상 함수 오버라이드 */
public:
	virtual bool CanPlaceItem(const UDISItemEntry* ItemEntry, const FDISItemPlacementContext& Context) const override;
};
