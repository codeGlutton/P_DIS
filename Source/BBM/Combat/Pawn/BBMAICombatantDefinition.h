/*****************************************************************//**
 * @file   BBMAICombatantDefinition.h
 * @brief  BBM(비트 배낭 상인) AI 조작 전투원 정적 정의 데이터 에셋 클래스 정의
 * @details 8비트 완성형 행동 패턴 풀(PatternPool) 및 FRandomStream 기반 가중치 추첨을 지원함 (몬스터, 보스, 소환수, 용병 공통)
 * @date   2026-11-01
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Combat/Pawn/BBMCharacterDefinition.h"
#include "Item/BBMBeatTypes.h"
#include "BBMAICombatantDefinition.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class FDataValidationContext;
struct FRandomStream;

/**
 * BBM AI 전투원 에셋 식별자 네임스페이스
 */
namespace BBMPrimaryAssetTypes
{
	/* BBM AI 전투원 정의 에셋 식별자 ("BBMAICombatant") */
	constexpr const TCHAR* AICombatant = TEXT("BBMAICombatant");
}

/**
 * AI 전투원의 1턴(8비트) 완성형 행동 패턴 구조체
 * AI 가중치/연속방지 메타데이터와 실제 8비트 실행 본체 시퀀스(FBBMBeatSequence)를 컴포지션으로 소유함
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMAIBeatSequence
{
	GENERATED_BODY()

	/* 생성자 및 헬퍼 */
public:
	FBBMAIBeatSequence();

	/* 패턴 및 내부 시퀀스 유효성 검사 */
	bool IsValid() const;

	/* 프로퍼티 */
public:
	/* 패턴 식별명 (예: "SlashAndShield", "HealingChant", "HeavySmash") */
	UPROPERTY(Category = "Pattern", EditAnywhere, BlueprintReadWrite)
	FName PatternName;

	/* 가중치 기반 무작위 추첨 시 사용되는 상대 가중치 (기본값: 1.0) */
	UPROPERTY(Category = "Pattern", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float SelectionWeight;

	/* 이전 턴과 동일한 패턴의 연속 사용 허용 여부 */
	UPROPERTY(Category = "Pattern", EditAnywhere, BlueprintReadWrite)
	bool bCanRepeatConsecutively;

	/* 8비트 실행 본체 시퀀스 (에디터 3단 타임라인 커스텀 UI 적용) */
	UPROPERTY(Category = "Pattern", EditAnywhere, BlueprintReadWrite)
	FBBMBeatSequence Sequence;
};

/**
 * BBM AI 전투원 정적 정의 에셋 클래스
 * 8비트 완성형 패턴 풀을 보유하고 FRandomStream 기반 가중치/연속방지 추첨을 제공함 (몬스터/소환수 공통)
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM AI Combatant Definition"))
class BBM_API UBBMAICombatantDefinition : public UBBMCharacterDefinition
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMAICombatantDefinition();

	/* PrimaryAssetId 오버라이드 */
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	/* 데이터 유효성 검사 (8비트 규격 및 가중치 검증) */
public:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/* 패턴 추첨 API */
public:
	/**
	 * 랜덤 스트림과 이전 패턴 이름을 고려하여 패턴 풀에서 1개의 8비트 시퀀스를 가중치 추첨
	 * @param InRandomStream 난수 결정에 사용할 랜덤 스트림 (결정론적 재현 보장)
	 * @param LastPatternName 직전 턴에 사용했던 패턴 이름 (연속 사용 방지용)
	 * @return 추첨된 패턴 포인터 (풀이 비어있으면 nullptr)
	 */
	const FBBMAIBeatSequence* SelectPattern(FRandomStream& InRandomStream, FName LastPatternName = NAME_None) const;

	/* AI 전투원 고유 프로퍼티 */
public:
	/* AI 전투원이 사용할 수 있는 8비트 행동 패턴 풀 */
	UPROPERTY(Category = "AI|Patterns", EditDefaultsOnly, BlueprintReadOnly)
	TArray<FBBMAIBeatSequence> PatternPool;
};

