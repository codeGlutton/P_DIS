#include "Enhancement/DISEnhancementEffect.h"
#include "Enhancement/DISEnhancement.h"

UDISEnhancementEffect::UDISEnhancementEffect()
{
}

const TMap<TSubclassOf<UDISEnhancement>, FDISEnhancementModifier>& UDISEnhancementEffect::GetModifiers() const
{
	return Modifiers;
}

const FDISEnhancementModifier* UDISEnhancementEffect::FindModifier(TSubclassOf<UDISEnhancement> InEnhancementClass) const
{
	if (InEnhancementClass == nullptr)
	{
		return nullptr;
	}

	return Modifiers.Find(InEnhancementClass);
}
