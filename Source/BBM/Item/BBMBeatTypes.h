/*****************************************************************//**
 * @file   BBMBeatTypes.h
 * @brief  BBM(비트 배낭 상인) 비트 시퀀스, 연출 타임라인 및 효과 타입 정의
 * @date   2026-10-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Item/BBMItemGameplayEffectTypes.h"
#include "BBMBeatTypes.generated.h"

/**
 * 효과가 실행되는 6대 논리 실행 범주 (6-Phase Logical Pipeline)
 */
UENUM(BlueprintType)
enum class EBBMEffectCategory : uint8
{
	PreMovement		= 0 UMETA(DisplayName = "1. 선 Movement"),
	PreBuffDebuff	= 1 UMETA(DisplayName = "2. 선 버프/디버프"),
	Defense			= 2 UMETA(DisplayName = "3. 방어"),
	Damage			= 3 UMETA(DisplayName = "4. 데미지"),
	PostBuffDebuff	= 4 UMETA(DisplayName = "5. 후 버프/디버프"),
	PostMovement	= 5 UMETA(DisplayName = "6. 후 Movement")
};

/**
 * 비트 효과의 대상 적용 범위 (1D 라인 전투 타깃 규격)
 */
UENUM(BlueprintType)
enum class EBBMTargetScope : uint8
{
	EnemyFrontline	= 0 UMETA(DisplayName = "적군 최전방 (단일)"),
	EnemyAll		= 1 UMETA(DisplayName = "적군 전체"),
	Self			= 2 UMETA(DisplayName = "시전자 본인"),
	AllAllies		= 3 UMETA(DisplayName = "아군 전체"),
	AllyFrontline	= 4 UMETA(DisplayName = "아군 최전방 (단일)")
};

/**
 * 1비트 내 4대 비주얼 연출 윈도우 (4-Slot Visual Timeline)
 */
UENUM(BlueprintType)
enum class EBBMVisualWindow : uint8
{
	PreMoveWindow		= 0 UMETA(DisplayName = "1구간 (0~25%): 선 이동 모션"),
	PrepDefenseWindow	= 1 UMETA(DisplayName = "2구간 (25~50%): 선 버프 및 방어 모션"),
	ImpactWindow		= 2 UMETA(DisplayName = "3구간 (50~75%): 타격 및 후속 디버프"),
	PostMoveWindow		= 3 UMETA(DisplayName = "4구간 (75~100%): 넉백 및 후퇴 모션")
};

/**
 * BBM 비트 효과 베이스 구조체
 * 다형성 인스턴스 구조체(TInstancedStruct)의 기본 타입으로, 6대 논리 실행 범주(EffectCategory)만 소유함
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMEffectBase
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FBBMEffectBase();
	explicit FBBMEffectBase(EBBMEffectCategory InCategory);
	virtual ~FBBMEffectBase() = default;

	/* 효과 범주 조회 */
public:
	EBBMEffectCategory GetEffectCategory() const;

	/* 프로퍼티 */
public:
	/* 효과가 속한 6대 논리 실행 범주 */
	UPROPERTY(Category = "BBM|Effect", EditAnywhere, BlueprintReadWrite)
	EBBMEffectCategory EffectCategory;
};

/**
 * 전투 교전 효과 구조체 (공격/방어/가드 등)
 * 대상 범위(TargetScope), 캐릭터 스탯 계수(Coefficient), 기본 수치(BaseValue)를 소유함
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMCombatEffect : public FBBMEffectBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FBBMCombatEffect();
	FBBMCombatEffect(EBBMEffectCategory InCategory, EBBMTargetScope InScope, float InCoefficient = 1.0f, float InBaseValue = 0.0f);

	/* 수치 계산 헬퍼 (InAttributeValue * Coefficient + BaseValue) */
public:
	float CalculateMagnitude(float InAttributeValue) const;

	/* 프로퍼티 */
public:
	/* 효과의 대상 적용 범위 */
	UPROPERTY(Category = "BBM|Effect", EditAnywhere, BlueprintReadWrite)
	EBBMTargetScope TargetScope;

	/* 캐릭터 어트리뷰트 스탯에 곱해질 계수 */
	UPROPERTY(Category = "BBM|Effect", EditAnywhere, BlueprintReadWrite)
	float Coefficient;

	/* 기본 고정 가산 수치 */
	UPROPERTY(Category = "BBM|Effect", EditAnywhere, BlueprintReadWrite)
	float BaseValue;
};

/**
 * 이동 및 진형 변경 효과 구조체 (선 이동, 후 넉백/후퇴 등)
 * 이동할 칸 수(StepCount)만 소유함
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMMovementEffect : public FBBMEffectBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FBBMMovementEffect();
	explicit FBBMMovementEffect(EBBMEffectCategory InCategory, int32 InStepCount = 1);

	/* 프로퍼티 */
public:
	/* 이동할 레인 칸 수 */
	UPROPERTY(Category = "BBM|Effect", EditAnywhere, BlueprintReadWrite)
	int32 StepCount;
};

/**
 * 한 비트 내에서 실행될 애니메이션 연출 구성
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMBeatAnimationConfig
{
	GENERATED_BODY()

	/* 생성자 및 헬퍼 */
public:
	FBBMBeatAnimationConfig();
	FBBMBeatAnimationConfig(EBBMVisualWindow InVisualWindow, const FGameplayTag& InStateTag);

	/* 유효한 애니메이션 상태 설정인지 확인 */
	bool IsValid() const;

	/* 프로퍼티 */
public:
	/* 비트 내 애니메이션 상태 변경 타이밍 (4대 연출 윈도우) */
	UPROPERTY(Category = "BBM|Animation", EditAnywhere, BlueprintReadWrite)
	EBBMVisualWindow VisualWindow;

	/* 변경할 대상 상태 태그 */
	UPROPERTY(Category = "BBM|Animation", EditAnywhere, BlueprintReadWrite)
	FGameplayTag StateTag;
};

/**
 * 단일 비트의 효과(TInstancedStruct 목록) 및 애니메이션 연출 구성
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMBeatDefinition
{
	GENERATED_BODY()

	/* 생성자 및 헬퍼 */
public:
	FBBMBeatDefinition();

	/* 효과 등록 여부 확인 */
	bool HasEffects() const;

	/* 애니메이션 설정 등록 여부 확인 */
	bool HasAnimations() const;

	/* 프로퍼티 */
public:
	/* 한 비트에 영향을 주는 Effect 인스턴스 구조체 목록 */
	UPROPERTY(Category = "BBM|Beat", EditAnywhere, BlueprintReadWrite)
	TArray<TInstancedStruct<FBBMEffectBase>> Effects;

	/* 한 비트에 실행될 애니메이션 구성 목록 */
	UPROPERTY(Category = "BBM|Beat", EditAnywhere, BlueprintReadWrite)
	TArray<FBBMBeatAnimationConfig> Animations;
};

/**
 * 특정 비트 구간 [StartBeat, EndBeat) 동안 캐릭터(ASC)에 부여 및 유지될 GameplayEffect 범위 스케줄 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMBeatEffectRange
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FBBMBeatEffectRange();
	FBBMBeatEffectRange(int32 InStartBeat, int32 InEndBeat, FName InRangeName = NAME_None);

	/* 비트 구간 판별 헬퍼 */
public:
	/* 특정 비트 인덱스에서 효과가 활성화 상태인지 확인 (StartBeat <= BeatIndex < EndBeat) */
	FORCEINLINE bool IsActiveAtBeat(int32 BeatIndex) const
	{
		return BeatIndex >= StartBeat && BeatIndex < EndBeat;
	}

	/* 특정 비트 인덱스가 효과의 시작 비트인지 확인 (부여 타이밍) */
	FORCEINLINE bool IsStartingAtBeat(int32 BeatIndex) const
	{
		return BeatIndex == StartBeat;
	}

	/* 특정 비트 인덱스 종료 시 효과가 회수되어야 하는지 확인 (해제 타이밍, BeatIndex + 1 == EndBeat) */
	FORCEINLINE bool IsEndingAtBeat(int32 BeatIndex) const
	{
		return (BeatIndex + 1) == EndBeat;
	}

	/* 유효한 비트 범위인지 검사 (0 <= StartBeat < EndBeat <= 8) */
	bool IsValid() const;

	/* 프로퍼티 */
public:
	/* 디버깅 및 에디터 식별용 구간 명칭 (예: "WeaponGrantedBuff", "RagePhase") */
	UPROPERTY(Category = "BBM|Beat", EditAnywhere, BlueprintReadWrite)
	FName RangeName;

	/* 효과 부여 시작 비트 인덱스 (0~7, 0은 1번째 비트 시작 시점) */
	UPROPERTY(Category = "BBM|Beat", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "7"))
	int32 StartBeat;

	/* 효과 회수 종료 비트 인덱스 (1~8, 1-based Exclusive, 8은 8번째 비트 종료 시점) */
	UPROPERTY(Category = "BBM|Beat", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "8"))
	int32 EndBeat;

	/* 해당 비트 구간 동안 캐릭터에 부여될 GameplayEffect 및 SetByCaller 수치 설정 목록 */
	UPROPERTY(Category = "BBM|Beat", EditAnywhere, BlueprintReadWrite)
	TArray<FBBMItemGameplayEffectConfig> GrantedEffects;
};

/**
 * 1턴(8비트) 동안 실행될 비트 시퀀스 및 비트 구간별 활성 효과 스케줄을 담는 공통 기반 구조체
 * 플레이어(배낭 인벤토리 배치 결과)와 AI(몬스터 행동 패턴의 실행 본체) 모두 동일한 데이터 규격으로 사용함
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMBeatSequence
{
	GENERATED_BODY()

	/* 생성자 및 헬퍼 */
public:
	FBBMBeatSequence();

	/* 8비트 규격 및 유효성 검사 (Beats.Num() == 8 && 모든 EffectRanges의 IsValid() == true) */
	bool IsValid() const;

	/* 프로퍼티 */
public:
	/* 8비트 시퀀스 정의 목록 (GDD 3.3 규격에 따라 8개 비트) */
	UPROPERTY(Category = "BBM|Beat", EditAnywhere, BlueprintReadWrite)
	TArray<FBBMBeatDefinition> Beats;

	/* 비트 구간별 활성화되는 GameplayEffect 스케줄 목록 */
	UPROPERTY(Category = "BBM|Beat", EditAnywhere, BlueprintReadWrite)
	TArray<FBBMBeatEffectRange> EffectRanges;
};


