#include "Combat/Pawn/BBMCharacterDefinition.h"

#define LOCTEXT_NAMESPACE "UBBMCharacterDefinition"

UBBMCharacterDefinition::UBBMCharacterDefinition() :
	DisplayName(LOCTEXT("DefaultDisplayName", "기본 캐릭터")),
	TableKeyName(TEXT("Player")),
	DefaultTeam(EBBMTeamType::Player),
	PawnClass(nullptr)
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

TSoftClassPtr<ABBMCombatantBase> UBBMCharacterDefinition::GetPawnClass() const
{
	return PawnClass;
}

#undef LOCTEXT_NAMESPACE
