/*****************************************************************//**
 * @file   BBMGameplayEffects.h
 * @brief  BBM(비트 배낭 상인) 슬롯 강화 및 버프용 C++ GameplayEffect 클래스 모음
 * @details 공격력(AttackPower) 및 방어력(DefensePower)을 SetByCaller 수치 기반으로 가산하는 무한 지속 효과 정의
 * @date   2026-11-05
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBMGameplayEffects.generated.h"

/**
 * 캐릭터의 공격력(AttackPower)을 SetByCaller(BBM.SetByCaller.AttackPower) 수치만큼 가산하는 무한 지속 GameplayEffect
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Attack Power Enhancement Effect"))
class BBM_API UBBMAttackPowerEnhancementEffect : public UGameplayEffect
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMAttackPowerEnhancementEffect();
};

/**
 * 캐릭터의 방어력(DefensePower)을 SetByCaller(BBM.SetByCaller.DefensePower) 수치만큼 가산하는 무한 지속 GameplayEffect
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Defense Power Enhancement Effect"))
class BBM_API UBBMDefensePowerEnhancementEffect : public UGameplayEffect
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMDefensePowerEnhancementEffect();
};

