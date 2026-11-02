/*****************************************************************//**
 * @file   BBMBeatEnhancementEffect.h
 * @brief  BBM 비트 강화 스탯을 수정/부여하는 UDISEnhancementEffect 구체 파생 클래스
 * @details 슬롯 및 아이템의 UDISEnhancementContainer에 적용되어 FDISActiveEnhancement 레벨을 가산함
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Enhancement/DISEnhancementEffect.h"
#include "BBMBeatEnhancementEffect.generated.h"

class UDISEnhancement;

/**
 * BBM 전용 강화 효과 구체 기본 베이스 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Beat Enhancement Effect"))
class BBM_API UBBMBeatEnhancementEffect : public UDISEnhancementEffect
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMBeatEnhancementEffect();

	/* 모디파이어 설정 헬퍼 */
public:
	/* 런타임/C++ 전용 모디파이어 추가 헬퍼 */
	void AddModifier(TSubclassOf<UDISEnhancement> InEnhancementClass, EDISEnhancementLevelOp InLevelOp, float InMagnitude);

	/* 특정 강화 클래스와 레벨 가산 모디파이어를 가진 동적 효과 인스턴스 생성 팩토리 */
	static UBBMBeatEnhancementEffect* CreateBeatEnhancementEffect(
		UObject* Outer,
		TSubclassOf<UDISEnhancement> InEnhancementClass,
		float InBonusLevel
	);

	/* UDISEnhancement 스탯 클래스에 대응하는 표준 UDISEnhancementEffect 클래스 조회 */
	static TSubclassOf<UDISEnhancementEffect> GetEffectClassForEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass);
};

/**
 * 구간 효과 강화(UBBMEffectRangeBeatEnhancement)를 1.0f 단위로 부여하는 표준 강화 효과 클래스
 * Spec.OverallMagnitude를 통해 원하는 가산 레벨(BonusLevel)만큼 스케일링됨
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Effect Range Beat Enhancement Effect"))
class BBM_API UBBMEffectRangeBeatEnhancementEffect : public UDISEnhancementEffect
{
	GENERATED_BODY()

public:
	UBBMEffectRangeBeatEnhancementEffect();
};

/**
 * 조건부 액션 주입 강화(UBBMConditionalActionBeatEnhancement)를 1.0f 단위로 부여하는 표준 강화 효과 클래스
 * Spec.OverallMagnitude를 통해 원하는 가산 레벨(BonusLevel)만큼 스케일링됨
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Conditional Action Beat Enhancement Effect"))
class BBM_API UBBMConditionalActionBeatEnhancementEffect : public UDISEnhancementEffect
{
	GENERATED_BODY()

public:
	UBBMConditionalActionBeatEnhancementEffect();
};

