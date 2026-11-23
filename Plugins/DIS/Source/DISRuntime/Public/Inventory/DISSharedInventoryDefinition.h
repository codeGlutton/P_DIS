/*****************************************************************//**
 * @file   DISSharedInventoryDefinition.h
 * @brief  DIS 공유/단축(Shared/QuickSlot) 인벤토리 정적 정의(PrimaryDataAsset) 클래스 정의
 * @details 1차원 선형 고정 슬롯 용량, 슬롯 클래스 및 전역 기본 필터를 정의하는 불변 에셋
 * @date   2026-10-26
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "Inventory/DISInventoryFilter.h"
#include "DISSharedInventoryDefinition.generated.h"

class UDISSharedInventorySlot;

/**
 * 공유 인벤토리 정의 PrimaryAssetType 식별자 모음
 */
namespace DISPrimaryAssetTypes
{
	/* 공유 인벤토리 정의 에셋 식별자 ("DISSharedInventory") */
	constexpr const TCHAR* SharedInventory = TEXT("DISSharedInventory");
}

/**
 * 퀵슬롯 및 단축창 등 1차원 선형 공유 인벤토리의 불변(Immutable) 정적 스펙 에셋
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "DIS Shared Inventory Definition"))
class DISRUNTIME_API UDISSharedInventoryDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

	/* 생성자 및 PrimaryAssetId */
public:
	UDISSharedInventoryDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* 공유 인벤토리 정적 프로퍼티 */
public:
	/* 항상 고정되는 슬롯 용량 (기본값 = 4) */
	UPROPERTY(Category = "Shared Inventory|Capacity", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", UIMin = "1"))
	int32 DefaultCapacity;

	/* 인벤토리가 기본 생성할 슬롯 UObject 클래스 타입 (UDISSharedInventorySlot 파생 클래스 선택, 미지정 시 UDISSharedInventorySlot 사용) */
	UPROPERTY(Category = "Shared Inventory|Slot", EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UDISSharedInventorySlot> DefaultSlotClass;

	/* 인벤토리 생성 시 기본 적용할 전역 필터 목록 (TInstancedStruct 다형성 지원) */
	UPROPERTY(Category = "Shared Inventory|Filter", EditDefaultsOnly, BlueprintReadOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FDISInventoryFilter>> DefaultFilters;
};
