/*****************************************************************//**
 * @file   BBMInventoryComponent.h
 * @brief  BBM(비트 배낭 상인) 배낭 인벤토리 래퍼 액터 컴포넌트 클래스 정의
 * @details UBBMInventory를 소유 및 관리하고, 전투 중 사용된 장비의 임시 보관(UsedEquipmentStorage) 및 유저 선택 슬롯 복원/빈 공간 자동 복원을 지원하는 컴포넌트
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Save/BBMSaveTypes.h"
#include "BBMInventoryComponent.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMInventory;
class UBBMInventoryDefinition;
class UDISItemStack;

/**
 * BBM 전용 배낭 인벤토리 래퍼 컴포넌트
 * 플레이어 캐릭터 등 액터에 부착되어 UBBMInventory 런타임 인스턴스를 소유하고 라이프사이클을 관리하며,
 * 전투 중 1회 사용된 장비(OncePerBattle)를 별도 보관소(UsedEquipmentStorage)로 이관 및 전투 종료 시 유저 선택 슬롯 또는 빈 공간으로 복원하는 기능을 제공함
 */
UCLASS(ClassGroup = (BBM), meta = (BlueprintSpawnableComponent, DisplayName = "BBM Inventory Component"))
class BBM_API UBBMInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

	/* 생성자 및 라이프사이클 */
public:
	UBBMInventoryComponent();

	/* 인벤토리 초기화 및 관리 */
public:
	/**
	 * 정적 인벤토리 정의(UBBMInventoryDefinition)를 바탕으로 내부 UBBMInventory 인스턴스 생성 및 초기화
	 * @param InDefinition 초기화에 사용할 BBM 인벤토리 정적 정의 에셋
	 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable)
	void InitializeInventory(const UBBMInventoryDefinition* InDefinition);

	/* 세이브 데이터 복원 */
public:
	/**
	 * 세이브 레코드 목록(FBBMInventorySlotSaveRecord)을 바탕으로 배낭 인벤토리 슬롯 배치 및 분리 보관소 장비를 복원
	 * @param SlotRecords 슬롯별 아이템 배치 레코드 배열
	 * @param ArchivedRecords 일시 분리 보관된 장비 레코드 배열
	 * @return 복원 성공 여부
	 */
	UFUNCTION(Category = "BBM|Inventory|Save", BlueprintCallable)
	bool RestoreInventoryFromSaveRecords(
		const TArray<FBBMInventorySlotSaveRecord>& SlotRecords,
		const TArray<FBBMEquipmentSaveRecord>& ArchivedRecords
	);

	/* 배낭 인벤토리의 모든 슬롯 아이템 및 분리 보관소 장비 완전 초기화 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable)
	void ClearAllInventory();

	/* 인벤토리 접근자 */
public:
	/* 소유 중인 BBM 배낭 인벤토리 인스턴스 반환 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable, BlueprintPure)
	UBBMInventory* GetInventory() const;

	/* 현재 적용된 정적 인벤토리 정의 에셋 반환 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable, BlueprintPure)
	const UBBMInventoryDefinition* GetInventoryDefinition() const;

	/* 아이템 추출 순서 편의 헬퍼 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable, BlueprintPure)
	TArray<UDISItemStack*> GetItemStacksInExtractionOrder() const;

	/* 인벤토리 인스턴스 유효성 검사 헬퍼 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable, BlueprintPure)
	bool HasValidInventory() const;

	/* 장비 일시 보관소 (UsedEquipmentStorage) 관리 */
public:
	/**
	 * 특정 장비 아이템 스택을 배낭 인벤토리에서 일시 보관소(UsedEquipmentStorage)로 이관(아카이브)함
	 * 배낭 인벤토리에서 슬롯 점유를 해제한 후 보관소에 등록함
	 * @param InItemStack 아카이브할 장비 아이템 스택
	 * @return 보관 성공 여부 (인벤토리에 없거나 이미 보관 중이면 false)
	 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable)
	bool ArchiveEquipment(UDISItemStack* InItemStack);

	/**
	 * 특정 슬롯에 위치한 장비 아이템 스택을 일시 보관소(UsedEquipmentStorage)로 이관(아카이브)함
	 * @param SlotIndex 아카이브할 장비가 위치한 배낭 슬롯 인덱스
	 * @return 보관 성공 여부
	 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable)
	bool ArchiveEquipmentAtSlot(int32 SlotIndex);

	/**
	 * 일시 보관 중인 특정 장비를 유저가 선택한 특정 배낭 슬롯 위치에 복원 배치함
	 * @param InItemStack 복원할 장비 아이템 스택
	 * @param TargetSlotIndex 유저가 선택한 대상 슬롯 인덱스
	 * @return 복원 성공 여부 (슬롯 수용 불가이거나 보관소에 없으면 false)
	 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable)
	bool RestoreArchivedEquipmentToSlot(UDISItemStack* InItemStack, int32 TargetSlotIndex);

	/**
	 * 일시 보관소에서 특정 장비를 꺼내어 보관소에서 분리 반환함 (UI 드래그앤드롭 등 유저 수동 조작 지원용)
	 * @param InItemStack 꺼낼 장비 아이템 스택
	 * @return 보관소에서 꺼낸 장비 아이템 스택 (존재하지 않으면 nullptr)
	 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable)
	UDISItemStack* TakeArchivedEquipment(UDISItemStack* InItemStack);

	/**
	 * 일시 보관 중인 특정 장비 1개를 배낭의 비어있는 공간에 자동으로 채워 복원함
	 * @param InItemStack 복원할 장비 아이템 스택
	 * @param bTryRotateToFit 비어있는 공간에 맞추기 위해 회전을 시도할지 여부
	 * @return 복원 성공 여부
	 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable)
	bool RestoreArchivedEquipment(UDISItemStack* InItemStack, bool bTryRotateToFit = true);

	/**
	 * 일시 보관 중인 모든 장비들을 배낭 인벤토리의 비어있는 공간에 최대한 채워 일괄 복원함
	 * @param OutFailedToRestoreStacks 공간 부족 등으로 복원에 실패한 아이템 스택 목록
	 * @param bTryRotateToFit 비어있는 공간에 맞추기 위해 회전을 시도할지 여부
	 * @return 성공적으로 배낭에 복원된 장비 개수
	 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable)
	int32 RestoreAllArchivedEquipment(TArray<UDISItemStack*>& OutFailedToRestoreStacks, bool bTryRotateToFit = true);

	/* 특정 장비 아이템 스택이 현재 일시 보관소에 보관 중인지 여부 반환 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable, BlueprintPure)
	bool IsEquipmentArchived(const UDISItemStack* InItemStack) const;

	/* 현재 일시 보관 중인 모든 장비 아이템 스택 목록 반환 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable, BlueprintPure)
	TArray<UDISItemStack*> GetArchivedEquipmentStacks() const;

	/* UsedEquipmentStorage 내부 배열 직접 참조 반환 (C++ 편의 접근자) */
	const TArray<TObjectPtr<UDISItemStack>>& GetUsedEquipmentStorage() const { return UsedEquipmentStorage; }
	TArray<TObjectPtr<UDISItemStack>>& GetMutableUsedEquipmentStorage() { return UsedEquipmentStorage; }

	/* 현재 일시 보관 중인 장비 총 개수 반환 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable, BlueprintPure)
	int32 GetArchivedEquipmentCount() const;

	/* 일시 보관 중인 장비가 1개 이상 존재하는지 여부 반환 */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable, BlueprintPure)
	bool HasArchivedEquipment() const;

	/* 일시 보관 중인 모든 장비 목록 초기화 (강제 비우기) */
	UFUNCTION(Category = "BBM|Inventory|Equipment", BlueprintCallable)
	void ClearArchivedEquipment();

	/* 내부 유틸리티 */
protected:
	/* 인벤토리의 비어있는 슬롯 공간에 아이템 스택을 맞추어 배치 시도 (성공 시 true) */
	bool TryFitItemStackIntoEmptySpace(UDISItemStack* InItemStack, bool bTryRotateToFit);

	/* 프로퍼티 */
protected:
	/* 런타임 배낭 인벤토리 인스턴스 */
	UPROPERTY(Category = "BBM|Inventory", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBBMInventory> Inventory;

	/* 전투 중 사용되어 일시 분리 보관된 장비 아이템 스택 목록 (GDD 4.2 UsedEquipmentStorage) */
	UPROPERTY(Category = "BBM|Inventory|Equipment", VisibleInstanceOnly, BlueprintReadOnly)
	TArray<TObjectPtr<UDISItemStack>> UsedEquipmentStorage;
};
