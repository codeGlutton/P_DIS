/*****************************************************************//**
 * @file   BBMCombatSimulator.h
 * @brief  BBM(비트 배낭 상인) 전투 시뮬레이터 및 교전 해결기 클래스 정의
 * @details 비트 단위 아이템 GAS GameplayEffect 적용/해제 및 GDD 5.3 4단계 피해 처리를 전담함
 * @date   2026-10-31
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ActiveGameplayEffectHandle.h"
#include "BBMCombatSimulator.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class ABBMCombatantBase;
class UBBMItemEntry;
class UBBMEffectBase;

/**
 * BBM 전투 시뮬레이션 및 교전 해결 클래스
 * 1. 비트 시작 시 아이템의 런타임 GAS Effect(SetByCaller 포함)를 캐릭터에 적용하고 핸들 관리
 * 2. 비트 종료 시 Active Effect를 일괄 해제하여 스탯 원복
 * 3. GDD 5.3 규격에 따른 엄격한 4단계 피해 처리(패링 -> 액티브 가드 -> 쌓아둔 방어도 -> 체력)
 */
UCLASS(BlueprintType, meta = (DisplayName = "BBM Combat Simulator"))
class BBM_API UBBMCombatSimulator : public UObject
{
	GENERATED_BODY()

	/* 비트 단위 아이템 GAS GameplayEffect 라이프사이클 관리 */
public:
	/* 아이템 사용 확정(비트 시작) 시 ItemEntry의 동적 Granted GameplayEffect들을 전투원 ASC에 적용 */
	UFUNCTION(Category = "BBM|Combat Simulator", BlueprintCallable)
	static bool ApplyItemGrantedEffects(ABBMCombatantBase* TargetCombatant, UBBMItemEntry* ItemEntry, TArray<FActiveGameplayEffectHandle>& OutHandles);

	/* 비트 종료 시 부여되었던 아이템의 Active GameplayEffect들을 전투원 ASC에서 일괄 제거하여 스탯 원복 */
	UFUNCTION(Category = "BBM|Combat Simulator", BlueprintCallable)
	static void RemoveItemGrantedEffects(ABBMCombatantBase* TargetCombatant, const TArray<FActiveGameplayEffectHandle>& InHandles);

	/* GDD 5.3 엄격한 4단계 피해 처리 및 교전 해결기 */
public:
	/* 4단계 피해 처리 (1.패링 체크 -> 2.액티브 가드 방어력 까기 -> 3.쌓아둔 방어력 까기 -> 4.체력 피해) */
	UFUNCTION(Category = "BBM|Combat Simulator", BlueprintCallable)
	static float ResolveDamage(ABBMCombatantBase* TargetCombatant, float IncomingDamage, bool bTargetIsParrying, UPARAM(ref) float& InOutActiveGuard);

	/* 지속 방어도 획득 처리 (GainArmor) */
	UFUNCTION(Category = "BBM|Combat Simulator", BlueprintCallable)
	static void ResolveGainArmor(ABBMCombatantBase* TargetCombatant, float InArmorAmount);
};

