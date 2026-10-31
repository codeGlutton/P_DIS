/*****************************************************************//**
 * @file   BBMAICombatantBase.h
 * @brief  BBM(비트 배낭 상인) AI 조작 전투원 베이스 폰 클래스 정의
 * @details 적군 몬스터/보스뿐만 아니라 아군 동료 소환수/모험가 등 시스템 AI가 자율 조작하는 전투원의 공통 기반 클래스
 * @date   2026-11-01
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Combat/Pawn/BBMCombatantBase.h"
#include "BBMAICombatantBase.generated.h"

/**
 * BBM AI 조작 전투원 베이스 폰 클래스
 * 플레이어가 아닌 시스템 AI가 조작하는 모든 유닛(몬스터, 보스, 아군 소환수, 동료 용병)의 공통 기반
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM AI Combatant Base"))
class BBM_API ABBMAICombatantBase : public ABBMCombatantBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMAICombatantBase();
};

