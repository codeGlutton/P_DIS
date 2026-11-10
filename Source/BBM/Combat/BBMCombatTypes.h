/*****************************************************************//**
 * @file   BBMCombatTypes.h
 * @brief  BBM(비트 배낭 상인) 1D 타일 라인 전투 및 상태이상 타입 정의
 * @date   2026-11-04
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "BBMCombatTypes.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class ABBMCombatantBase;

/**
 * 1D 라인 전투 진영(레인) 식별 열거형
 */
UENUM(BlueprintType)
enum class EBBMCombatLane : uint8
{
	/* 아군 진영 (플레이어 상인, 소환수, 아군 동료 등) */
	AllyLane = 0 UMETA(DisplayName = "Ally Lane"),

	/* 적군 진영 (몬스터, 보스 등) */
	EnemyLane = 1 UMETA(DisplayName = "Enemy Lane")
};

/**
 * 1D 레인 타일 시프트(이동/넉백) 방향 열거형
 */
UENUM(BlueprintType)
enum class EBBMLaneShiftDirection : uint8
{
	/* 전진 (상대 진영 쪽으로 다가감) */
	Frontward = 0 UMETA(DisplayName = "Frontward (Advance)"),

	/* 후퇴 / 넉백 (상대 진영의 반대, 뒤쪽으로 밀려남) */
	Backward = 1 UMETA(DisplayName = "Backward (Knockback / Retreat)")
};

/**
 * 1D 라인 전투 레인 타일 배치 슬롯 구조체
 * 인덱스 0이 상대 팀과 맞닿아 있는 최전방(Frontline)임
 */
USTRUCT(BlueprintType)
struct BBM_API FBBM1DCombatLane
{
	GENERATED_BODY()

	FBBM1DCombatLane()
	{
	}

	/* 타일 슬롯 배열 (유닛 인스턴스 약참조) */
	UPROPERTY(VisibleAnywhere, Category = "Formation")
	TArray<TWeakObjectPtr<ABBMCombatantBase>> Slots;
};

/**
 * 특정 윈도우 시점의 전투원 어트리뷰트 스탯 스냅샷 구조체
 * 리플렉션 순회로 전체 속성을 캡처하며, 특정 속성 안전 조회 함수를 제공함
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMCombatantSnapshot
{
	GENERATED_BODY()

	/* 리플렉션으로 캡처된 전체 어트리뷰트 수치 맵 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FGameplayAttribute, float> AttributeValues;

	/* 특정 어트리뷰트 수치 안전 조회 헬퍼 */
	float GetAttributeValue(const FGameplayAttribute& Attribute, float DefaultValue = 0.0f) const
	{
		if (const float* ValPtr = AttributeValues.Find(Attribute))
		{
			return *ValPtr;
		}
		return DefaultValue;
	}
};

/**
 * 액션이 실행되는 6대 논리 실행 범주 (6-Phase Logical Action Pipeline)
 */
UENUM(BlueprintType)
enum class EBBMActionCategory : uint8
{
	PreMovement		= 0 UMETA(DisplayName = "1. 선 Movement"),
	PreBuffDebuff	= 1 UMETA(DisplayName = "2. 선 버프/디버프"),
	Defense			= 2 UMETA(DisplayName = "3. 방어"),
	Damage			= 3 UMETA(DisplayName = "4. 데미지"),
	PostBuffDebuff	= 4 UMETA(DisplayName = "5. 후 버프/디버프"),
	PostMovement	= 5 UMETA(DisplayName = "6. 후 Movement")
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
 * 특정 윈도우 내에서 단일 전투원에게 발생한 액션 로그 및 애니메이션 태그
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMCombatantActionLog
{
	GENERATED_BODY()

	/* 속성별 수치 변화량 (Health 피해량, TurnArmor 흡수량, PersistentArmor 흡수량 등) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FGameplayAttribute, float> AttributeDeltas;

	/* 상태이상 태그별 스택 변화량 (독 +2스택, 화상 -1스택 등) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, int32> StatusStackDeltas;

	/* 해당 윈도우 시점에 사망 판정 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsDead = false;

	/* 해당 윈도우에서 실행할 애니메이션 태그 모음 (액션, 피격, 사망 등) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayTagContainer AnimationTags;
};

/**
 * 1개 비주얼 윈도우(PreMove / PrepDefense / Impact / PostMove) 연출 단위 결과 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMVisualWindowResult
{
	GENERATED_BODY()

	/* 현재 윈도우 구간 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EBBMVisualWindow VisualWindow = EBBMVisualWindow::PreMoveWindow;

	/* 전투원별 윈도우 종료 시점 스탯 스냅샷 (C++ 전용) */
	TMap<TWeakObjectPtr<ABBMCombatantBase>, FBBMCombatantSnapshot> Snapshots;

	/* 전투원별 윈도우 내 발생 액션 로그 (C++ 전용) */
	TMap<TWeakObjectPtr<ABBMCombatantBase>, FBBMCombatantActionLog> CombatLogs;
};

/**
 * 1비트 동안 발생한 연출 결과 (무기/장비 AnimLayer 전환 및 4개 윈도우 순차 결과)
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMBeatResult
{
	GENERATED_BODY()

	/* 비트 인덱스 (0~7) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 BeatIndex = 0;

	/* 비트 시작 시 전투원별 전환할 AnimLayer 태그 (C++ 전용) */
	TMap<TWeakObjectPtr<ABBMCombatantBase>, FGameplayTag> StartAnimLayerTags;

	/* 비트 종료 시 전투원별 해제할 AnimLayer 태그 (C++ 전용) */
	TMap<TWeakObjectPtr<ABBMCombatantBase>, FGameplayTag> EndAnimLayerTags;

	/* 비트 내 윈도우 순차 연출 목록 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FBBMVisualWindowResult> WindowResults;
};

/**
 * 1턴(8비트 + 턴 종료) 시뮬레이션의 최종 결과물 패킷
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMTurnVisualResult
{
	GENERATED_BODY()

	/* 비트 0~7 순차 연출 결과 배열 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FBBMBeatResult> BeatResults;

	/* 상태이상 연출 결과 (상태이상 틱 피해) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FBBMVisualWindowResult StatusTickResult;

	/* 턴 종료 연출 결과 (턴 방어도 초기화) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FBBMVisualWindowResult TurnEndResult;
};

/**
 * BBM 전투 상태이상 식별 네이티브 태그 네임스페이스
 */
namespace BBMStatusTags
{
	/* 독 상태이상 태그 (턴 종료 시 스택 수만큼 피해 적용 후 1스택 감소) */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Poison);

	/* 화상 상태이상 태그 (턴 종료 시 고정 피해 적용 후 1스택 감소) */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Burn);
}

/**
 * BBM 전투 애니메이션 상태 식별 네이티브 태그 네임스페이스
 */
namespace BBMAnimationTags
{
	/* 기본 액션 실행 애니메이션 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Action);

	/* 피격(히트) 애니메이션 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Hit);

	/* 전투 승리 세레머니 애니메이션 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Victory);

	/* 전투 패배 애니메이션 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Defeat);

	/* 사망 애니메이션 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Death);
}

/**
 * BBM 강화 및 버프 GameplayEffect SetByCaller 수치 식별 네이티브 태그 네임스페이스
 */
namespace BBMSetByCallerTags
{
	/* 공격력(AttackPower) 가산 SetByCaller 수치 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AttackPower);

	/* 방어력(DefensePower) 가산 SetByCaller 수치 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_DefensePower);
}

/**
 * BBM 무기 및 장비 Linked Anim Layer 식별 네이티브 태그 네임스페이스
 */
namespace BBMAnimLayerTags
{
	/* 검(Sword) Anim Layer 식별 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AnimLayer_Weapon_Sword);

	/* 단검(Dagger) Anim Layer 식별 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AnimLayer_Weapon_Dagger);

	/* 활(Bow) Anim Layer 식별 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AnimLayer_Weapon_Bow);

	/* 방패(Shield) Anim Layer 식별 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AnimLayer_Armor_Shield);

	/* 갑옷(Armor) Anim Layer 식별 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AnimLayer_Armor_Armor);

	/* 신발(Boots) Anim Layer 식별 태그 */
	BBM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AnimLayer_Armor_Boots);
}
