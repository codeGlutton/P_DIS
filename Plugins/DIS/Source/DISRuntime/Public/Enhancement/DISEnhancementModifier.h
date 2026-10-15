/*****************************************************************//**
 * @file   DISEnhancementModifier.h
 * @brief  DIS 강화 레벨 연산 방식(LevelOp) 및 모디파이어(Modifier) 데이터 정의
 * @date   2026-09-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "DISEnhancementModifier.generated.h"

/**
 * 강화 모디파이어가 변동을 주는 대상 속성 (현재 레벨 vs 최대 레벨 상한)
 */
UENUM(BlueprintType)
enum class EDISEnhancementModifierTarget : uint8
{
	/* 현재 강화 레벨 수치에 모디파이어 적용 */
	CurrentLevel UMETA(DisplayName = "Current Level"),

	/* 해당 강화 스탯의 최대 레벨 상한치에 모디파이어 적용 */
	MaxLevel     UMETA(DisplayName = "Max Level"),
};

/**
 * 강화 레벨 적용 연산 방식 (가산, 승산, 덮어쓰기)
 */
UENUM(BlueprintType)
enum class EDISEnhancementLevelOp : uint8
{
	/* 기본 강화 레벨에 Magnitude를 가산 */
	Add       UMETA(DisplayName = "Add"),

	/* 기본 강화 레벨에 Magnitude를 승산 */
	Multiply  UMETA(DisplayName = "Multiply"),

	/* 기존 강화 레벨을 Magnitude로 덮어쓰기 */
	Override  UMETA(DisplayName = "Override"),

	/* 연산자 총 개수 카운터 */
	Max       UMETA(Hidden)
};

/**
 * 특정 강화 스탯에 적용되는 개별 모디파이어(연산자 + 크기) 구조체
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISEnhancementModifier
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FDISEnhancementModifier();
	FDISEnhancementModifier(EDISEnhancementLevelOp InLevelOp, float InLevelMagnitude);
	FDISEnhancementModifier(EDISEnhancementModifierTarget InTarget, EDISEnhancementLevelOp InLevelOp, float InLevelMagnitude);

	/* 프로퍼티 */
public:
	/* 모디파이어 연산 대상 (현재 레벨 vs 최대 레벨) */
	UPROPERTY(Category = "Enhancement|Modifier", EditAnywhere, BlueprintReadWrite)
	EDISEnhancementModifierTarget Target;

	/* 레벨 연산자 (Add, Multiply, Override) */
	UPROPERTY(Category = "Enhancement|Modifier", EditAnywhere, BlueprintReadWrite)
	EDISEnhancementLevelOp LevelOp;

	/* 레벨 연산 크기 */
	UPROPERTY(Category = "Enhancement|Modifier", EditAnywhere, BlueprintReadWrite)
	float LevelMagnitude;
};
