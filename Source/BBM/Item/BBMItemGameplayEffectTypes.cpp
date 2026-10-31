#include "Item/BBMItemGameplayEffectTypes.h"
#include "Combat/GAS/BBMAttributeSet.h"
#include "GameplayTagsManager.h"

FBBMItemGameplayEffectConfig::FBBMItemGameplayEffectConfig() :
	EffectClass(),
	SetByCallerMagnitudes()
{
}

FBBMItemGameplayEffectConfig::FBBMItemGameplayEffectConfig(TSubclassOf<UGameplayEffect> InEffectClass) :
	EffectClass(InEffectClass),
	SetByCallerMagnitudes()
{
}

bool FBBMItemGameplayEffectConfig::IsValid() const
{
	return EffectClass.IsNull() == false;
}

UBBMItemStatGameplayEffect::UBBMItemStatGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 태그 등록 및 캐싱
	const FGameplayTag AttackTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("BBM.Combat.AttackPower"), TEXT("아이템 공격력 버프 태그"));
	const FGameplayTag DefenseTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("BBM.Combat.DefensePower"), TEXT("아이템 방어력 버프 태그"));
	const FGameplayTag ArmorTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("BBM.Combat.Armor"), TEXT("아이템 방어도 버프 태그"));

	// 1. 공격력 Modifier
	{
		FGameplayModifierInfo ModInfo;
		ModInfo.Attribute = UBBMAttributeSet::GetAttackPowerAttribute();
		ModInfo.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = AttackTag;
		ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(ModInfo);
	}

	// 2. 방어력 Modifier
	{
		FGameplayModifierInfo ModInfo;
		ModInfo.Attribute = UBBMAttributeSet::GetDefensePowerAttribute();
		ModInfo.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = DefenseTag;
		ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(ModInfo);
	}

	// 3. 방어도 Modifier
	{
		FGameplayModifierInfo ModInfo;
		ModInfo.Attribute = UBBMAttributeSet::GetArmorAttribute();
		ModInfo.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = ArmorTag;
		ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(ModInfo);
	}
}

