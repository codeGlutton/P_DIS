/*****************************************************************//**
 * @file   BBMCombatTypes.h
 * @brief  BBM(비트 배낭 상인) 1D 타일 라인 전투 및 상태이상 타입 정의
 * @date   2026-11-04
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
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
