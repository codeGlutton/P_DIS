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

/* 독립 전방 선언 (상단에 단독 배치) */
struct FBBMAIBeatSequence;
struct FRandomStream;

/**
 * BBM AI 조작 전투원 베이스 폰 클래스
 * 플레이어가 아닌 시스템 AI가 조작하는 모든 유닛(몬스터, 보스, 아군 소환수, 동료 용병)의 공통 기반으로,
 * 턴마다 AI 패턴 풀(PatternPool)에서 8비트 행동 시퀀스를 선정하여 보관 및 실행함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM AI Combatant Base"))
class BBM_API ABBMAICombatantBase : public ABBMCombatantBase
{
	GENERATED_BODY()

	/* 생성자 및 라이프사이클 */
public:
	ABBMAICombatantBase();

	/* ABBMCombatantBase 가상 함수 오버라이드 */
public:
	/* 캐릭터 정적 정의 주입 시 스탯 초기화 및 초기 1턴 AI 패턴 무작위 선정 */
	virtual void InitializeFromDefinition() override;

	/* 이번 턴에 실행할 8비트 행동 시퀀스 갱신 (새 패턴 선정 및 캐시 업데이트) */
	virtual void UpdateBeatSequence() override;

	/* 이번 턴에 실행할 8비트 행동 시퀀스 반환 (캐시된 시퀀스 반환) */
	virtual const FBBMBeatSequence& GetCurrentBeatSequence() const override;

	/* AI 패턴 선정 및 관리 */
public:
	/**
	 * 이번 턴에 실행할 행동 패턴을 랜덤 스트림 기반으로 새로 선정 및 갱신
	 * @param InRandomStream 난수 결정에 사용할 랜덤 스트림 (결정론적 재현 보장)
	 * @return 선정된 패턴 포인터 (실패 시 nullptr)
	 */
	const FBBMAIBeatSequence* SelectNewPattern(FRandomStream& InRandomStream);

	/**
	 * 이번 턴에 실행할 행동 패턴을 기본 무작위 난수로 새로 선정 및 갱신
	 * @return 패턴 선정 성공 여부
	 */
	UFUNCTION(Category = "BBM|AI", BlueprintCallable)
	bool SelectNewPatternRandom();

	/* 현재 선정된 AI 행동 패턴 포인터 반환 */
	const FBBMAIBeatSequence* GetSelectedPattern() const;

	/* 현재 선정된 AI 행동 패턴 이름 반환 */
	UFUNCTION(Category = "BBM|AI", BlueprintCallable, BlueprintPure)
	FName GetSelectedPatternName() const;

	/* 런타임 상태 프로퍼티 */
protected:
	/* 이번 턴에 실행하기로 선정된 AI 8비트 행동 패턴 레코드 포인터 */
	const FBBMAIBeatSequence* SelectedPattern;

	/* 직전에 사용했던 패턴 이름 (연속 사용 방지용) */
	UPROPERTY(Category = "BBM|AI", VisibleInstanceOnly, BlueprintReadOnly)
	FName LastPatternName;
};
