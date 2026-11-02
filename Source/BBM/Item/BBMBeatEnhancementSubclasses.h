/*****************************************************************//**
 * @file   BBMBeatEnhancementSubclasses.h
 * @brief  BBM(비트 배낭 상인) 비트 시퀀스 강화 구체 파생 클래스 정의
 * @details 1번 타입(구간 효과) 및 2, 3번 타입(조건부 액션 주입) 구현
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Item/BBMBeatEnhancement.h"
#include "BBMBeatEnhancementSubclasses.generated.h"

/* 독립 전방 선언 */
class UCurveFloat;

/**
 * 1번 타입: 특정 비트 구간 [Start, End)에 지속되는 GameplayEffect 스케줄을 추가하는 강화
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Effect Range Beat Enhancement"))
class BBM_API UBBMEffectRangeBeatEnhancement : public UBBMBeatEnhancement
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMEffectRangeBeatEnhancement();

	/* UBBMBeatEnhancement 인터페이스 구현 */
public:
	virtual void ApplyBeatModification(int32 InLevel, const FBBMBeatSequence& BaseSequence, FBBMBeatSequence& InOutDynamicSequence) const override;

	/* 프로퍼티 */
public:
	/** 추가할 단일 비트 구간 효과 템플릿 */
	UPROPERTY(Category = "BBM|Enhancement|Range", EditDefaultsOnly, BlueprintReadOnly)
	FBBMBeatEffectRange AddedEffectRange;

	/**
	 * 레벨에 따른 SetByCaller 수치 배율 커브 (선택 사항)
	 * 유효한 커브가 지정되어 있으면 GrantedEffects의 SetByCaller 수치에 Curve(Level)을 곱함
	 */
	UPROPERTY(Category = "BBM|Enhancement|Range", EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> MagnitudeCurve;
};

/**
 * 2, 3번 타입: 공격 비트에 상태이상 추가, 방어 비트에 버프 추가 등 특정 카테고리 비트에 조건부 액션을 주입하는 강화
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Conditional Action Beat Enhancement"))
class BBM_API UBBMConditionalActionBeatEnhancement : public UBBMBeatEnhancement
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMConditionalActionBeatEnhancement();

	/* UBBMBeatEnhancement 인터페이스 구현 */
public:
	virtual void ApplyBeatModification(int32 InLevel, const FBBMBeatSequence& BaseSequence, FBBMBeatSequence& InOutDynamicSequence) const override;

	/* 프로퍼티 */
public:
	/** 단일 조건부 액션 주입 설정 */
	UPROPERTY(Category = "BBM|Enhancement|Action", EditDefaultsOnly, BlueprintReadOnly)
	FBBMConditionalActionConfig ConditionalAction;

	/**
	 * 레벨에 따른 주입 액션 수치 배율 커브 (선택 사항)
	 * 주입할 액션이 FBBMCombatAction인 경우, Coefficient 및 BaseValue에 Curve(Level)을 곱함
	 */
	UPROPERTY(Category = "BBM|Enhancement|Action", EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> MagnitudeCurve;
};

