/*****************************************************************//**
 * @file   BBMGameplayEffects.cpp
 * @brief  BBM(비트 배낭 상인) 슬롯 강화 및 버프용 C++ GameplayEffect 구현
 * @date   2026-11-05
 *********************************************************************/

#include "Combat/GAS/BBMGameplayEffects.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "Combat/BBMCombatTypes.h"

UBBMAttackPowerEnhancementEffect::UBBMAttackPowerEnhancementEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UBBMAttributeSet::GetAttackPowerAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = BBMSetByCallerTags::SetByCaller_AttackPower;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModInfo);
}

UBBMDefensePowerEnhancementEffect::UBBMDefensePowerEnhancementEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UBBMAttributeSet::GetDefensePowerAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = BBMSetByCallerTags::SetByCaller_DefensePower;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModInfo);
}

