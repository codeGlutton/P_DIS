#include "Combat/Animation/BBMCombatantAnimInstance.h"

UBBMCombatantAnimInstance::UBBMCombatantAnimInstance()
{
}

TSubclassOf<UAnimInstance> UBBMCombatantAnimInstance::FindAnimLayerClass(const FGameplayTag& LayerTag) const
{
	if (LayerTag.IsValid() == false)
	{
		return nullptr;
	}

	if (const TSubclassOf<UAnimInstance>* FoundClass = AnimLayerMap.Find(LayerTag))
	{
		return *FoundClass;
	}

	return nullptr;
}

bool UBBMCombatantAnimInstance::HasAnimLayer(const FGameplayTag& LayerTag) const
{
	if (LayerTag.IsValid() == false)
	{
		return false;
	}

	return AnimLayerMap.Contains(LayerTag);
}

const TMap<FGameplayTag, TSubclassOf<UAnimInstance>>& UBBMCombatantAnimInstance::GetAnimLayerMap() const
{
	return AnimLayerMap;
}

