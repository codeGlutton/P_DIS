/*****************************************************************//**
 * @file   BBMCombatTypes.cpp
 * @brief  BBM(비트 배낭 상인) 1D 타일 라인 전투 및 상태이상 네이티브 태그 정의 구현
 * @date   2026-11-04
 *********************************************************************/

#include "Combat/BBMCombatTypes.h"
#include "NativeGameplayTags.h"

namespace BBMStatusTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Poison, "BBM.Status.Poison", "독 상태이상");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Burn, "BBM.Status.Burn", "화상 상태이상");
}

namespace BBMAnimationTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Animation_Action, "BBM.Animation.Action", "전투원 액션 실행 애니메이션 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Animation_Hit, "BBM.Animation.Hit", "전투원 피격 애니메이션 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Animation_Victory, "BBM.Animation.Victory", "전투 승리 세레머니 애니메이션 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Animation_Defeat, "BBM.Animation.Defeat", "전투 패배 애니메이션 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Animation_Death, "BBM.Animation.Death", "전투원 사망 애니메이션 태그");
}

namespace BBMSetByCallerTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_AttackPower, "BBM.SetByCaller.AttackPower", "공격력(AttackPower) 가산 SetByCaller 수치 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_DefensePower, "BBM.SetByCaller.DefensePower", "방어력(DefensePower) 가산 SetByCaller 수치 태그");
}

namespace BBMAnimLayerTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AnimLayer_Weapon_Sword, "BBM.AnimLayer.Weapon.Sword", "검 무기 애님 레이어 식별 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AnimLayer_Weapon_Dagger, "BBM.AnimLayer.Weapon.Dagger", "단검 무기 애님 레이어 식별 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AnimLayer_Weapon_Bow, "BBM.AnimLayer.Weapon.Bow", "활 무기 애님 레이어 식별 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AnimLayer_Armor_Shield, "BBM.AnimLayer.Armor.Shield", "방패 방어구 애님 레이어 식별 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AnimLayer_Armor_Armor, "BBM.AnimLayer.Armor.Armor", "갑옷 방어구 애님 레이어 식별 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AnimLayer_Armor_Boots, "BBM.AnimLayer.Armor.Boots", "신발 방어구 애님 레이어 식별 태그");
}


