#include "Enhancement/DISEnhanceableInterface.h"
#include "Enhancement/DISEnhancement.h"

int32 IDISEnhanceableInterface::GetBaseMaxEnhancementLevel(TSubclassOf<UDISEnhancement> InEnhancementClass) const
{
	if (InEnhancementClass != nullptr)
	{
		if (const UDISEnhancement* CDO = InEnhancementClass->GetDefaultObject<UDISEnhancement>())
		{
			return CDO->GetDefaultMaxLevel();
		}
	}

	return 0;
}

