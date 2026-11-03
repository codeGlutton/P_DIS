/*****************************************************************//**
 * @file   BBMPlayerCharacterDefinition.cpp
 * @brief  BBM(비트 배낭 상인) 플레이어 상인 캐릭터 정적 정의 데이터 에셋 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Inventory/BBMInventoryDefinition.h"

UBBMPlayerCharacterDefinition::UBBMPlayerCharacterDefinition() :
	InventoryDefinition(nullptr)
{
	DisplayName = FText::FromString(TEXT("Player Merchant"));
	TableKeyName = FName(TEXT("Player"));
	DefaultTeam = EBBMTeamType::Player;
}

FPrimaryAssetId UBBMPlayerCharacterDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(BBMPrimaryAssetTypes::PlayerCharacter, GetFName());
}

const UBBMInventoryDefinition* UBBMPlayerCharacterDefinition::GetInventoryDefinition() const
{
	return InventoryDefinition.Get();
}
