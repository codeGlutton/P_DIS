/*****************************************************************//**
 * @file   BBMCharacterDefinition.cpp
 * @brief  BBM(비트 배낭 상인) 캐릭터 기본 정적 정의 데이터 에셋 클래스 구현
 * @date   2026-10-31
 *********************************************************************/

#include "Combat/BBMCharacterDefinition.h"

UBBMCharacterDefinition::UBBMCharacterDefinition() :
	DisplayName(FText::FromString(TEXT("기본 캐릭터"))),
	TableKeyName(TEXT("Player")),
	DefaultTeam(EBBMTeamType::Player)
{
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

