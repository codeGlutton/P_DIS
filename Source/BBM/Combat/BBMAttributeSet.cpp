#include "Combat/BBMAttributeSet.h"
#include "GameplayEffectExtension.h"

UBBMAttributeSet::UBBMAttributeSet() :
	Health(TNumericLimits<float>::Max()),
	MaxHealth(TNumericLimits<float>::Max()),
	Armor(0.0f),
	AttackPower(10.0f),
	DefensePower(5.0f)
{
}

void UBBMAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 체력: 0 ~ MaxHealth 범위 클램핑 (생성자 기본값이 실수 최댓값이므로 초기화 순서에 상관없이 보존됨)
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	// 최대 체력: 최소 1.0f 보장
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	// 방어도, 공격력, 방어력: 음수 방지 (0 이상)
	else if (Attribute == GetArmorAttribute() || Attribute == GetAttackPowerAttribute() || Attribute == GetDefensePowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UBBMAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 체력 속성 변경 후 클램핑
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		Health.SetCurrentValue(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	// 최대 체력 속성 변경 후 현재 체력 조정
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		MaxHealth.SetCurrentValue(FMath::Max(GetMaxHealth(), 1.0f));
		if (GetHealth() > GetMaxHealth())
		{
			Health.SetCurrentValue(GetMaxHealth());
		}
	}
	// 방어도 음수 방지
	else if (Data.EvaluatedData.Attribute == GetArmorAttribute())
	{
		Armor.SetCurrentValue(FMath::Max(GetArmor(), 0.0f));
	}
	// 공격력 음수 방지
	else if (Data.EvaluatedData.Attribute == GetAttackPowerAttribute())
	{
		AttackPower.SetCurrentValue(FMath::Max(GetAttackPower(), 0.0f));
	}
	// 방어력 음수 방지
	else if (Data.EvaluatedData.Attribute == GetDefensePowerAttribute())
	{
		DefensePower.SetCurrentValue(FMath::Max(GetDefensePower(), 0.0f));
	}
}
