#include "Enhancement/DISEnhancementEffectSpec.h"
#include "Enhancement/DISEnhancementEffect.h"

FDISEnhancementEffectSpec::FDISEnhancementEffectSpec() :
	EffectClass(nullptr),
	OverallMagnitude(1.0f),
	Instigator(nullptr)
{
}

FDISEnhancementEffectSpec::FDISEnhancementEffectSpec(
	TSubclassOf<UDISEnhancementEffect> InEffectClass,
	float InOverallMagnitude,
	UObject* InInstigator
) :
	EffectClass(InEffectClass),
	OverallMagnitude(InOverallMagnitude),
	Instigator(InInstigator)
{
}

bool FDISEnhancementEffectSpec::IsValid() const
{
	return EffectClass != nullptr;
}
