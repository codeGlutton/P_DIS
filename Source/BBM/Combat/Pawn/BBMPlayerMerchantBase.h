/*****************************************************************//**
 * @file   BBMPlayerMerchantBase.h
 * @brief  BBM(비트 배낭 상인) 플레이어 상인 베이스 폰 클래스 정의
 * @details 주인공 상인 캐릭터의 기본 골격을 정의하며 배낭 인벤토리 컴포넌트(UBBMInventoryComponent) 및 플레이어 정적 정의를 관리함
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Combat/Pawn/BBMCombatantBase.h"
#include "BBMPlayerMerchantBase.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMInventoryComponent;
class UBBMInventory;
class UBBMPlayerCharacterDefinition;

/**
 * BBM 플레이어 상인 베이스 폰 클래스
 * 비트 배낭 상인의 주인공 캐릭터 클래스로, 배낭 인벤토리 래퍼 컴포넌트를 소유하고 플레이어 전용 정의(UBBMPlayerCharacterDefinition)를 기반으로 초기화됨
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Player Merchant Base"))
class BBM_API ABBMPlayerMerchantBase : public ABBMCombatantBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMPlayerMerchantBase();

	/* ABBMCombatantBase 가상 함수 오버라이드 */
public:
	virtual void InitializeFromDefinition() override;

	/* 이번 턴에 실행할 8비트 행동 시퀀스 갱신 (인벤토리 기반 합성 후 캐시 저장) */
	virtual void UpdateBeatSequence() override;

	/* 이번 턴에 실행할 8비트 행동 시퀀스 반환 (캐시된 시퀀스 반환) */
	virtual FBBMBeatSequence GetCurrentBeatSequence() const override;

	/* 8비트 행동 시퀀스 생성 및 캐시 갱신 */
public:
	/**
	 * 소유한 배낭 인벤토리의 ExtractionOrder에 따라 정렬된 아이템들을 순차 누적하여 이번 턴 8비트 행동 시퀀스를 합성하고 캐시를 갱신
	 * @return 누적 합성되어 캐시된 8비트 행동 시퀀스 (8비트 초과 분은 자동 오버플로우 제외)
	 */
	UFUNCTION(Category = "BBM|Beat", BlueprintCallable)
	FBBMBeatSequence GenerateBeatSequenceFromInventory();

	/* 인벤토리 및 플레이어 정의 접근자 */
public:
	/* 배낭 인벤토리 래퍼 컴포넌트 반환 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable, BlueprintPure)
	UBBMInventoryComponent* GetInventoryComponent() const;

	/* 래퍼 컴포넌트 내 실제 배낭 인벤토리 런타임 인스턴스 반환 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable, BlueprintPure)
	UBBMInventory* GetInventory() const;

	/* 플레이어 전용 정적 정의 에셋(UBBMPlayerCharacterDefinition) 반환 */
	UFUNCTION(Category = "BBM|Character", BlueprintCallable, BlueprintPure)
	const UBBMPlayerCharacterDefinition* GetPlayerCharacterDefinition() const;

	/* 인벤토리 초기화 실행 */
public:
	/* 소유한 CharacterDefinition으로부터 인벤토리 정의를 추출하여 인벤토리 컴포넌트 초기화 */
	UFUNCTION(Category = "BBM|Inventory", BlueprintCallable)
	void InitializeInventoryFromDefinition();

	/* 컴포넌트 프로퍼티 */
protected:
	/* 플레이어 전용 배낭 인벤토리 래퍼 컴포넌트 */
	UPROPERTY(Category = "BBM|Inventory", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBBMInventoryComponent> InventoryComponent;
};
