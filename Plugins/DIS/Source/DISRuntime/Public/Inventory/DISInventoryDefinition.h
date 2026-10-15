/*****************************************************************//**
 * @file   DISInventoryDefinition.h
 * @brief  DIS 정적 인벤토리 정의(PrimaryDataAsset) 클래스 정의
 * @details 인벤토리 규격(용량, 인덱싱 방향, 타일 형상 및 블록 타일) 기본 규칙을 설정하는 불변 에셋
 * @date   2026-10-14
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "Inventory/DISInventoryFilter.h"
#include "Item/DISItemTypes.h"
#include "DISInventoryDefinition.generated.h"

/**
 * 인벤토리 정의 PrimaryAssetType 식별자 모음
 */
namespace DISPrimaryAssetTypes
{
	/* 인벤토리 정의 에셋 식별자 ("DISInventory") */
	constexpr const TCHAR* Inventory = TEXT("DISInventory");
}

/**
 * 2D 인벤토리 슬롯 인덱스 증가 방향
 */
UENUM(BlueprintType)
enum class EDISSlotIndexingDirection : uint8
{
	/* 가로 우선: 좌측에서 우측으로 인덱스가 증가하고 한 줄이 차면 다음 줄로 이동 (Row-Major) */
	Horizontal = 0 UMETA(DisplayName = "Horizontal (Row-Major)"),

	/* 세로 우선: 상단에서 하단으로 인덱스가 증가하고 한 열이 차면 다음 열로 이동 (Column-Major) */
	Vertical   = 1 UMETA(DisplayName = "Vertical (Column-Major)")
};

class UDISInventorySlot;

/**
 * 인벤토리의 불변(Immutable) 정적 스펙 및 초기화 규칙을 정의하는 PrimaryDataAsset
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "DIS Inventory Definition"))
class DISRUNTIME_API UDISInventoryDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDISInventoryDefinition();

	/* PrimaryAssetId 오버라이드 */
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* UObject 인터페이스 */
public:
	virtual void PostLoad() override;

#if WITH_EDITOR
	/* 데이터 유효성 검사 및 프로퍼티 변경 이벤트 */
public:
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;

private:
	/* 에디터 환경에서 BlockedTileShape의 EditorFixedGridDimensions 규격을 최신화 */
	void UpdateBlockedTileShapeGridDimensions();
#endif

	/* 용량 정보 조회 */
public:
	/* 슬롯 용량 확장 가능 여부 (최대 확장 가능 용량이 기본 용량보다 큰 경우 true) */
	UFUNCTION(Category = "DIS|Inventory Definition", BlueprintCallable, BlueprintPure)
	bool CanExpand() const;

	/* 2D 공간 좌표 및 타일 블록 검증 헬퍼 */
public:
	/* 슬롯 1차원 인덱스를 2D 그리드 레이아웃 좌표(X, Y)로 변환 (줄 바꿈 미설정 또는 범위 초과 시 FIntPoint::NoneValue 반환) */
	UFUNCTION(Category = "DIS|Inventory Definition", BlueprintCallable, BlueprintPure)
	FIntPoint SlotIndexToCoord(int32 SlotIndex) const;

	/* 2D 그리드 레이아웃 좌표(X, Y)를 슬롯 1차원 인덱스로 변환 (줄 바꿈 미설정 또는 범위 초과 시 INDEX_NONE 반환) */
	UFUNCTION(Category = "DIS|Inventory Definition", BlueprintCallable, BlueprintPure)
	int32 CoordToSlotIndex(const FIntPoint& Coord) const;

	/* 특정 2D 그리드 좌표가 사용 불가하게 막힌(블록된) 타일인지 여부 검증 (음수 좌표 시 false 반환) */
	UFUNCTION(Category = "DIS|Inventory Definition", BlueprintCallable, BlueprintPure)
	bool IsTileBlocked(const FIntPoint& Coord) const;

	/* 특정 슬롯 인덱스 위치가 사용 불가하게 막힌(블록된) 칸인지 여부 검증 (범위 초과 시 false 반환) */
	UFUNCTION(Category = "DIS|Inventory Definition", BlueprintCallable, BlueprintPure)
	bool IsSlotBlocked(int32 SlotIndex) const;

	/* 인벤토리 용량 및 확장 속성 */
public:
	/* 인벤토리 초기 슬롯 용량 (기본 시작 크기) */
	UPROPERTY(Category = "Inventory Definition|Capacity", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", UIMin = "1"))
	int32 DefaultCapacity;

	/* 확장을 통해 도달할 수 있는 최대 슬롯 한도 (DefaultCapacity 이상이어야 함) */
	UPROPERTY(Category = "Inventory Definition|Capacity", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxExpandableCapacity;

	/* 슬롯 레이아웃 인덱싱 속성 */
public:
	/* 슬롯 인덱스 번호 증가 방향 (가로 우선 / 세로 우선) */
	UPROPERTY(Category = "Inventory Definition|Layout", EditDefaultsOnly, BlueprintReadOnly)
	EDISSlotIndexingDirection SlotIndexingDirection;

	/* 인벤토리 슬롯 줄 바꿈 기준 칸 수 (가로 기준 시 행당 열 수, 세로 기준 시 열당 행 수) */
	UPROPERTY(Category = "Inventory Definition|Layout", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", UIMin = "1"))
	int32 LineBreakTileCount;

	/* 무게 시스템 속성 */
public:
	/* 인벤토리 무게 시스템 적용 여부 */
	UPROPERTY(Category = "Inventory Definition|Weight", EditDefaultsOnly, BlueprintReadOnly)
	bool bUseWeightSystem;

	/* 인벤토리의 기본 최대 적재 허용 무게 */
	UPROPERTY(Category = "Inventory Definition|Weight", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DefaultMaxWeight;

	/* 공간 타일 점유 시스템 속성 */
public:
	/* 인벤토리 내에서 비활성화/막힌 판정으로 처리할 타일 형상 (MaxExpandableCapacity 영역 중 막힌 칸) */
	UPROPERTY(Category = "Inventory Definition|Spatial", EditDefaultsOnly, BlueprintReadOnly)
	FDISTileShape BlockedTileShape;

	/* 슬롯 클래스 타입 속성 */
public:
	/* 인벤토리가 기본 생성할 슬롯 UObject 클래스 타입 (UDISInventorySlot 파생 클래스 선택, 미지정 시 UDISInventorySlot 사용) */
	UPROPERTY(Category = "Inventory Definition|Slot", EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UDISInventorySlot> DefaultSlotClass;

	/* 인벤토리 기본 필터 속성 */
public:
	/* 인벤토리 생성 시 기본 적용할 전역 필터 목록 (TInstancedStruct 다형성 지원) */
	UPROPERTY(Category = "Inventory Definition|Filter", EditDefaultsOnly, BlueprintReadOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FDISInventoryFilter>> DefaultFilters;
};
