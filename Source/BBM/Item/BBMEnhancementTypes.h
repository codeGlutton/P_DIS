/*****************************************************************//**
 * @file   BBMEnhancementTypes.h
 * @brief  BBM(비트 배낭 상인) 비트 기반 강화 시스템 설정 구조체 정의
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Item/BBMBeatTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "BBMEnhancementTypes.generated.h"

/**
 * 특정 비트 액션 카테고리를 탐색하여 부가 액션을 주입하는 조건부 설정 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMConditionalActionConfig
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FBBMConditionalActionConfig() :
		TriggerCategory(EBBMActionCategory::Damage),
		ActionToInject(),
		bTargetBaseBeatsOnly(false)
	{
	}

	FBBMConditionalActionConfig(
		EBBMActionCategory InTriggerCategory,
		const TInstancedStruct<FBBMBeatAction>& InActionToInject,
		bool bInTargetBaseBeatsOnly = false) :
		TriggerCategory(InTriggerCategory),
		ActionToInject(InActionToInject),
		bTargetBaseBeatsOnly(bInTargetBaseBeatsOnly)
	{
	}

	/* 유효성 검사 */
	FORCEINLINE bool IsValid() const
	{
		return ActionToInject.IsValid();
	}

	/* 프로퍼티 */
public:
	/** 탐색할 대상 트리거 액션 카테고리 (Damage: 공격 비트, Defense: 방어 비트 등) */
	UPROPERTY(Category = "BBM|Enhancement", EditAnywhere, BlueprintReadWrite)
	EBBMActionCategory TriggerCategory;

	/** 조건을 만족하는 비트에 주입할 신규 액션 (출혈/화상 CombatAction, 방어 버프 등) */
	UPROPERTY(Category = "BBM|Enhancement", EditAnywhere, BlueprintReadWrite)
	TInstancedStruct<FBBMBeatAction> ActionToInject;

	/**
	 * 원본 아이템 기본 비트에만 한정할 것인가?
	 * - false (기본값): 앞선 강화로 추가된 공격 비트를 포함하여 '모든' 공격 비트에 트리거
	 * - true: 원본 아이템의 기본 공격 비트에만 한정 트리거
	 */
	UPROPERTY(Category = "BBM|Enhancement", EditAnywhere, BlueprintReadWrite)
	bool bTargetBaseBeatsOnly;
};

