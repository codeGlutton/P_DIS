#include "Enhancement/DISEnhancementEffectSpec.h"
#include "Enhancement/DISEnhancementEffect.h"

FDISEnhancementEffectSpec::FDISEnhancementEffectSpec() :
	EffectClass(nullptr),
	OverallMagnitude(1.0f)
{
}

FDISEnhancementEffectSpec::FDISEnhancementEffectSpec(
	TSubclassOf<UDISEnhancementEffect> InEffectClass,
	float InOverallMagnitude
) :
	EffectClass(InEffectClass),
	OverallMagnitude(InOverallMagnitude)
{
}

bool FDISEnhancementEffectSpec::IsValid() const
{
	return EffectClass != nullptr;
}
