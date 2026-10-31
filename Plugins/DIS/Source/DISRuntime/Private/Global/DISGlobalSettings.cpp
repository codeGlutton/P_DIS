#include "Global/DISGlobalSettings.h"
#include "Global/DISGlobalLogic.h"

UDISGlobalSettings::UDISGlobalSettings() :
	GlobalLogicClass(UDISGlobalLogic::StaticClass()),
	MaxItemTileDimensions(8, 8)
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("DIS Global Settings");
}

UClass* UDISGlobalSettings::GetGlobalLogicClass() const
{
	return (GlobalLogicClass != nullptr) ? GlobalLogicClass.Get() : UDISGlobalLogic::StaticClass();
}

FName UDISGlobalSettings::GetCategoryName() const
{
	return TEXT("Game");
}

FName UDISGlobalSettings::GetSectionName() const
{
	return TEXT("DIS Global Settings");
}
