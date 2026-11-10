/*****************************************************************//**
 * @file   BBMPlayerCharacterDefinition.h
 * @brief  BBM(비트 배낭 상인) 플레이어 상인 캐릭터 정적 정의 데이터 에셋 클래스 정의
 * @details 플레이어 상인의 기본 스탯 설정 및 소유할 배낭 인벤토리 정적 정의(UBBMInventoryDefinition)를 관리함
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Combat/Pawn/BBMCharacterDefinition.h"
#include "BBMPlayerCharacterDefinition.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMInventoryDefinition;
class UBBMRegionDefinition;

/**
 * BBM 플레이어 캐릭터 에셋 식별자 네임스페이스
 */
namespace BBMPrimaryAssetTypes
{
	/* BBM 플레이어 캐릭터 정의 에셋 식별자 ("BBMPlayerCharacter") */
	constexpr const TCHAR* PlayerCharacter = TEXT("BBMPlayerCharacter");
}

/**
 * BBM 플레이어 캐릭터 정적 정의 에셋 클래스
 * 플레이어 상인의 스탯, 기본 팀, 소유 배낭 인벤토리 정의(UBBMInventoryDefinition) 및 시작 지역(UBBMRegionDefinition)을 보관함
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Player Character Definition"))
class BBM_API UBBMPlayerCharacterDefinition : public UBBMCharacterDefinition
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMPlayerCharacterDefinition();

	/* PrimaryAssetId 오버라이드 */
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* Getter 헬퍼 */
public:
	/* 배낭 인벤토리 정적 정의 에셋 반환 */
	const UBBMInventoryDefinition* GetInventoryDefinition() const;

	/* 플레이어 캐릭터의 런 시작 지역 정적 정의 에셋 반환 */
	const UBBMRegionDefinition* GetStartRegionDefinition() const;

	/* 플레이어 고유 정적 프로퍼티 */
public:
	/* 플레이어 캐릭터가 생성 및 초기화 시 소유할 배낭 인벤토리 정적 정의 에셋 */
	UPROPERTY(Category = "Player|Inventory", EditDefaultsOnly, BlueprintReadOnly, meta = (AssetBundles = "Data"))
	TSoftObjectPtr<UBBMInventoryDefinition> InventoryDefinition;

	/* 플레이어 캐릭터가 런 시작 시 처음 진입할 시작 지역 정적 정의 에셋 */
	UPROPERTY(Category = "Player|Progression", EditDefaultsOnly, BlueprintReadOnly, meta = (AssetBundles = "Data"))
	TSoftObjectPtr<UBBMRegionDefinition> StartRegionDefinition;
};
