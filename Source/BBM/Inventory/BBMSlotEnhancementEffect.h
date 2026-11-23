/*****************************************************************//**
 * @file   BBMSlotEnhancementEffect.h
 * @brief  BBM(비트 배낭 상인) 슬롯에 적용되어 슬롯 강화 스탯을 활성화하는 UDISEnhancementEffect 클래스 정의
 * @details 슬롯 자체 EnhancementContainer에 장착되어 UBBMSlotEnhancement 스탯 레벨을 가산함
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Enhancement/DISEnhancementEffect.h"
#include "BBMSlotEnhancementEffect.generated.h"

/**
 * 슬롯에 적용되는 강화 효과 기본 베이스 클래스 (CDO 기반)
 */
UCLASS(BlueprintType, Blueprintable, Abstract, meta = (DisplayName = "BBM Slot Enhancement Effect"))
class BBM_API UBBMSlotEnhancementEffect : public UDISEnhancementEffect
{
	GENERATED_BODY()

public:
	UBBMSlotEnhancementEffect();
};

/**
 * 슬롯에 UBBMSlotEffectRangeEnhancement(구간 효과 슬롯 강화) 스탯을 1.0f 단위로 부여하는 강화 효과 클래스
 * Spec.OverallMagnitude를 통해 슬롯 강화 보너스 레벨이 스케일링됨
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Slot Effect Range Enhancement Effect"))
class BBM_API UBBMSlotEffectRangeEnhancementEffect : public UBBMSlotEnhancementEffect
{
	GENERATED_BODY()

public:
	UBBMSlotEffectRangeEnhancementEffect();
};

/**
 * 슬롯에 UBBMSlotConditionalActionEnhancement(조건부 액션 주입 슬롯 강화) 스탯을 1.0f 단위로 부여하는 강화 효과 클래스
 * Spec.OverallMagnitude를 통해 슬롯 강화 보너스 레벨이 스케일링됨
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Slot Conditional Action Enhancement Effect"))
class BBM_API UBBMSlotConditionalActionEnhancementEffect : public UBBMSlotEnhancementEffect
{
	GENERATED_BODY()

public:
	UBBMSlotConditionalActionEnhancementEffect();
};

