/*****************************************************************//**
 * @file   DISEnhancementEffectSpec.h
 * @brief  DIS 강화 효과(Effect) 적용 요청 명세(Spec) 데이터 정의
 * @date   2026-09-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "DISEnhancementEffectSpec.generated.h"

class UDISEnhancementEffect;

/**
 * 장착/해제 가능한 강화 효과(Effect) 부여 시 요청되는 명세 구조체
 * (개별 스탯 대상 및 연산자는 EffectClass CDO를 따르며, 전체 배율 OverallMagnitude를 보유함)
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISEnhancementEffectSpec
{
	GENERATED_BODY()

public:
	FDISEnhancementEffectSpec();
	explicit FDISEnhancementEffectSpec(
		TSubclassOf<UDISEnhancementEffect> InEffectClass,
		float InOverallMagnitude = 1.0f
	);

	/* 명세 유효성 검사 */
public:
	bool IsValid() const;

	/* 런타임 요청 프로퍼티 */
public:
	/* 적용할 강화 효과 정의 클래스 */
	UPROPERTY(Category = "Enhancement Effect Spec", EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDISEnhancementEffect> EffectClass;

	/* 전체적인 크기/배율 계수 (기본값 1.0, EffectClass CDO 내부의 각 Modifier Magnitude에 곱해짐) */
	UPROPERTY(Category = "Enhancement Effect Spec", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OverallMagnitude;
};
