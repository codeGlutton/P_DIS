#include "Enhancement/DISEnhancementEffectSpec.h"
#include "Enhancement/DISEnhancementEffect.h"

FDISEnhancementEffectSpec::FDISEnhancementEffectSpec() :
	EffectClass(nullptr),
	OverallMagnitude(1.0f),
	Instigator(nullptr),
	DynamicGrantedTags()
{
}

FDISEnhancementEffectSpec::FDISEnhancementEffectSpec(
	TSubclassOf<UDISEnhancementEffect> InEffectClass,
	float InOverallMagnitude,
	UObject* InInstigator,
	const FGameplayTagContainer& InDynamicTags
) :
	EffectClass(InEffectClass),
	OverallMagnitude(InOverallMagnitude),
	Instigator(InInstigator),
	DynamicGrantedTags(InDynamicTags)
{
}

bool FDISEnhancementEffectSpec::IsValid() const
{
	return EffectClass != nullptr;
}
