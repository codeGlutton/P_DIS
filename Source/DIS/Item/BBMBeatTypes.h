/*****************************************************************//**
 * @file   BBMBeatTypes.h
 * @brief  BBM(비트 배낭 상인) 비트 시퀀스, 연출 타임라인 및 효과 타입 정의
 * @date   2026-10-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
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
struct DIS_API FBBMEffectBase
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
struct DIS_API FBBMCombatEffect : public FBBMEffectBase
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
struct DIS_API FBBMMovementEffect : public FBBMEffectBase
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
struct DIS_API FBBMBeatAnimationConfig
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
struct DIS_API FBBMBeatDefinition
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

