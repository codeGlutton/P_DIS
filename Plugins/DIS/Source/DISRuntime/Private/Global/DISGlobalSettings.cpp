#include "Global/DISGlobalSettings.h"

UDISGlobalSettings::UDISGlobalSettings() :
	MaxItemTileDimensions(8, 8)
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("DIS Global Settings");
}

FName UDISGlobalSettings::GetCategoryName() const
{
	return TEXT("Game");
}

FName UDISGlobalSettings::GetSectionName() const
{
	return TEXT("DIS Global Settings");
}
