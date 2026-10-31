#include "Combat/BBMCombatSimulator.h"
#include "Combat/BBMCombatantBase.h"
#include "Combat/BBMAttributeSet.h"
#include "Item/BBMItemEntry.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

bool UBBMCombatSimulator::ApplyItemGrantedEffects(ABBMCombatantBase* TargetCombatant, UBBMItemEntry* ItemEntry, TArray<FActiveGameplayEffectHandle>& OutHandles)
{
	if (TargetCombatant == nullptr || ItemEntry == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = TargetCombatant->GetAbilitySystemComponent();
	if (ASC == nullptr)
	{
		return false;
	}

	const TArray<FBBMItemGameplayEffectConfig>& GrantedEffects = ItemEntry->GetDynamicGrantedEffects();
	for (const FBBMItemGameplayEffectConfig& Config : GrantedEffects)
	{
		TSubclassOf<UGameplayEffect> EffectClass = Config.EffectClass.Get();
		if (EffectClass == nullptr)
		{
			EffectClass = Config.EffectClass.LoadSynchronous();
		}

		if (EffectClass == nullptr)
		{
			continue;
		}

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddInstigator(TargetCombatant, TargetCombatant);

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid() == true && SpecHandle.Data.IsValid() == true)
		{
			// 필수 기본 스탯 태그가 설정되어 있다면 0.0f로 초기화하여 SetByCaller 미지정 에러 방지
			static const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("BBM.Combat.AttackPower"), false);
			static const FGameplayTag DefenseTag = FGameplayTag::RequestGameplayTag(FName("BBM.Combat.DefensePower"), false);
			static const FGameplayTag ArmorTag = FGameplayTag::RequestGameplayTag(FName("BBM.Combat.Armor"), false);

			if (AttackTag.IsValid() == true)
			{
				SpecHandle.Data->SetSetByCallerMagnitude(AttackTag, 0.0f);
			}
			if (DefenseTag.IsValid() == true)
			{
				SpecHandle.Data->SetSetByCallerMagnitude(DefenseTag, 0.0f);
			}
			if (ArmorTag.IsValid() == true)
			{
				SpecHandle.Data->SetSetByCallerMagnitude(ArmorTag, 0.0f);
			}

			// SetByCallerMagnitude 동적 주입
			for (const TPair<FGameplayTag, float>& Pair : Config.SetByCallerMagnitudes)
			{
				SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
			}

			FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			if (ActiveHandle.IsValid() == true)
			{
				OutHandles.Add(ActiveHandle);
			}
		}
	}

	return true;
}

void UBBMCombatSimulator::RemoveItemGrantedEffects(ABBMCombatantBase* TargetCombatant, const TArray<FActiveGameplayEffectHandle>& InHandles)
{
	if (TargetCombatant == nullptr)
	{
		return;
	}

	UAbilitySystemComponent* ASC = TargetCombatant->GetAbilitySystemComponent();
	if (ASC == nullptr)
	{
		return;
	}

	for (const FActiveGameplayEffectHandle& Handle : InHandles)
	{
		if (Handle.IsValid() == true)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}
}

float UBBMCombatSimulator::ResolveDamage(ABBMCombatantBase* TargetCombatant, float IncomingDamage, bool bTargetIsParrying, float& InOutActiveGuard)
{
	if (TargetCombatant == nullptr || IncomingDamage <= 0.0f)
	{
		return 0.0f;
	}

	// 1단계: 패링 체크 (피격자가 패링 상태면 공격 완전 무효화, 방어력/체력 100% 보존)
	if (bTargetIsParrying == true)
	{
		return 0.0f;
	}

	UAbilitySystemComponent* ASC = TargetCombatant->GetAbilitySystemComponent();
	UBBMAttributeSet* AttributeSet = TargetCombatant->GetAttributeSet();
	if (ASC == nullptr || AttributeSet == nullptr)
	{
		return 0.0f;
	}

	// 2단계: 액티브 가드 방어력 까기 (D' = FMath::Max(0, D - ActiveGuard))
	const float GuardAbsorb = FMath::Min(FMath::Max(InOutActiveGuard, 0.0f), IncomingDamage);
	InOutActiveGuard = FMath::Max(0.0f, InOutActiveGuard - GuardAbsorb);
	float RemainingDamage = IncomingDamage - GuardAbsorb;

	if (RemainingDamage <= 0.0f)
	{
		return 0.0f;
	}

	// 3단계: 쌓아둔 방어력 까기 (D'' = FMath::Max(0, D' - PersistentArmor))
	const float CurrentArmor = AttributeSet->GetArmor();
	const float ArmorAbsorb = FMath::Min(CurrentArmor, RemainingDamage);
	const float NewArmor = FMath::Max(0.0f, CurrentArmor - ArmorAbsorb);
	ASC->SetNumericAttributeBase(UBBMAttributeSet::GetArmorAttribute(), NewArmor);
	RemainingDamage -= ArmorAbsorb;

	if (RemainingDamage <= 0.0f)
	{
		return 0.0f;
	}

	// 4단계: 남은 데미지 처리 (CurrentHP = FMath::Max(0, CurrentHP - D''))
	const float CurrentHealth = AttributeSet->GetHealth();
	const float ActualHealthDamage = FMath::Min(CurrentHealth, RemainingDamage);
	const float NewHealth = FMath::Max(0.0f, CurrentHealth - RemainingDamage);
	ASC->SetNumericAttributeBase(UBBMAttributeSet::GetHealthAttribute(), NewHealth);

	return ActualHealthDamage;
}

void UBBMCombatSimulator::ResolveGainArmor(ABBMCombatantBase* TargetCombatant, float InArmorAmount)
{
	if (TargetCombatant == nullptr || InArmorAmount <= 0.0f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = TargetCombatant->GetAbilitySystemComponent();
	UBBMAttributeSet* AttributeSet = TargetCombatant->GetAttributeSet();
	if (ASC != nullptr && AttributeSet != nullptr)
	{
		const float NewArmor = AttributeSet->GetArmor() + InArmorAmount;
		ASC->SetNumericAttributeBase(UBBMAttributeSet::GetArmorAttribute(), NewArmor);
	}
}
