/*****************************************************************//**
 * @file   BBMAbilitySystemGlobals.h
 * @brief  BBM(비트 배낭 상인) 전역 GAS 싱글톤 클래스 정의
 * @details DLC 및 테스트 환경에서의 동적 CurveTable 주입을 지원하며 Initter 안전성을 보장함
 * @date   2026-10-31
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "BBMAbilitySystemGlobals.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UCurveTable;

/**
 * BBM 전역 AbilitySystemGlobals 싱글톤 클래스
 * DLC 및 테스트 환경에서의 동적 CurveTable 주입을 지원하며 Initter 안전성을 보장함
 */
UCLASS(Config = Game)
class BBM_API UBBMAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMAbilitySystemGlobals();

	/* UAbilitySystemGlobals 가상 함수 오버라이드 */
public:
	/* 전역 데이터 초기화 (Initter 사전 할당 보장) */
	virtual void InitGlobalData() override;

	/* 동적 CurveTable 주입 및 Initter 편의 API */
public:
	/* 런타임/테스트용 CurveTable 인스턴스 직접 동적 등록 */
	void AddAttributeDefaultTableInstance(UCurveTable* InCurveTable);

	/* Initter 할당 여부 확인 */
	bool HasAttributeSetInitter() const;
};

