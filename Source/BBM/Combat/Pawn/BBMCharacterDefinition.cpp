#include "Combat/Pawn/BBMCharacterDefinition.h"

UBBMCharacterDefinition::UBBMCharacterDefinition() :
	DisplayName(FText::FromString(TEXT("기본 캐릭터"))),
	TableKeyName(TEXT("Player")),
	DefaultTeam(EBBMTeamType::Player)
{
}

FPrimaryAssetId UBBMCharacterDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(BBMPrimaryAssetTypes::Character, GetFName());
}

const FText& UBBMCharacterDefinition::GetDisplayName() const
{
	return DisplayName;
}

FName UBBMCharacterDefinition::GetTableKeyName() const
{
	return TableKeyName;
}

EBBMTeamType UBBMCharacterDefinition::GetDefaultTeam() const
{
	return DefaultTeam;
}
