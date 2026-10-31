/*****************************************************************//**
 * @file   BBMPlayerMerchantBase.h
 * @brief  BBM(비트 배낭 상인) 플레이어 상인 베이스 폰 클래스 정의
 * @details 주인공 상인 캐릭터의 기본 골격을 정의하며 향후 배낭 인벤토리 조작 및 비트 전투 인터랙션을 담당함
 * @date   2026-11-01
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Combat/Pawn/BBMCombatantBase.h"
#include "BBMPlayerMerchantBase.generated.h"

/**
 * BBM 플레이어 상인 베이스 폰 클래스
 * 비트 배낭 상인의 주인공 캐릭터 기본 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Player Merchant Base"))
class BBM_API ABBMPlayerMerchantBase : public ABBMCombatantBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMPlayerMerchantBase();
};

