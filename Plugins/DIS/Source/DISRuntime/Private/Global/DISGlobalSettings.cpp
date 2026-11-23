#include "Global/DISGlobalSettings.h"
#include "Global/DISGlobalLogic.h"

#define LOCTEXT_NAMESPACE "UDISGlobalSettings"

UDISGlobalSettings::UDISGlobalSettings() :
	GlobalLogicClass(UDISGlobalLogic::StaticClass()),
	MaxItemTileDimensions(8, 8),
	AffixDelimiter(LOCTEXT("AffixDelimiter", ", ")),
	EnhancedDisplayNamePrefixAndSuffixFormat(LOCTEXT("EnhancedDisplayNamePrefixAndSuffixFormat", "{Prefix} {BaseName} {Suffix}")),
	EnhancedDisplayNamePrefixOnlyFormat(LOCTEXT("EnhancedDisplayNamePrefixOnlyFormat", "{Prefix} {BaseName}")),
	EnhancedDisplayNameSuffixOnlyFormat(LOCTEXT("EnhancedDisplayNameSuffixOnlyFormat", "{BaseName} {Suffix}"))
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("DIS Global Settings");
}

#undef LOCTEXT_NAMESPACE

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
