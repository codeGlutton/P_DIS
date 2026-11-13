#include "Audio/BBMAudioSettings.h"

UBBMAudioSettings::UBBMAudioSettings(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("BBM Audio Settings");
}

