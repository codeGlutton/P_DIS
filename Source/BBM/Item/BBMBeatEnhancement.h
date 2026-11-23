/*****************************************************************//**
 * @file   BBMBeatEnhancement.h
 * @brief  BBM(비트 배낭 상인) 비트 시퀀스 강화 추상 베이스 클래스 정의
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Enhancement/DISEnhancement.h"
#include "Item/BBMBeatTypes.h"
#include "Item/BBMEnhancementTypes.h"
#include "BBMBeatEnhancement.generated.h"

/**
 * BBM 전용 비트 시퀀스 수정 기능을 제공하는 강화 속성 추상 베이스 클래스 (CDO 기반)
 * ExecutionPriority와 다형성 적용 가상함수(ApplyBeatModification)를 제공함
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisplayName = "BBM Beat Enhancement Base"))
class BBM_API UBBMBeatEnhancement : public UDISEnhancement
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMBeatEnhancement();

	/* UDISEnhancement 라이프사이클 가상함수 오버라이드 */
public:
	virtual void OnEnhancementActivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const override;
	virtual void OnEnhancementDeactivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const override;
	virtual void OnEnhancementLevelChanged_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement, int32 OldLevel, int32 NewLevel) const override;

	/* 다형성 비트 시퀀스 수정 가상함수 */
public:
	/**
	 * 현재 강화 레벨 및 원본 시퀀스를 참조하여 동적 비트 시퀀스(InOutDynamicSequence)를 수정함
	 * @param InLevel 현재 활성화된 강화 레벨 (1 이상)
	 * @param BaseSequence 아이템 정의(UBBMItemDefinition)의 불변 원본 시퀀스
	 * @param InOutDynamicSequence 현재 재계산 중인 런타임 가변 비트 시퀀스
	 */
	virtual void ApplyBeatModification(int32 InLevel, const FBBMBeatSequence& BaseSequence, FBBMBeatSequence& InOutDynamicSequence) const;

	/* 공통 프로퍼티 */
public:
	/** 파이프라인 내에서의 평가 우선순위 (낮을수록 먼저 적용됨. 기본값 0) */
	UPROPERTY(Category = "BBM|Enhancement", EditDefaultsOnly, BlueprintReadOnly)
	int32 ExecutionPriority;
};

