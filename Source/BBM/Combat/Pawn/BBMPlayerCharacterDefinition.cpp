/*****************************************************************//**
 * @file   BBMPlayerCharacterDefinition.cpp
 * @brief  BBM(비트 배낭 상인) 플레이어 상인 캐릭터 정적 정의 데이터 에셋 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Combat/Pawn/BBMPlayerCharacterDefinition.h"
#include "Inventory/BBMInventoryDefinition.h"
#include "Stage/BBMRegionDefinition.h"

#define LOCTEXT_NAMESPACE "UBBMPlayerCharacterDefinition"

UBBMPlayerCharacterDefinition::UBBMPlayerCharacterDefinition() :
	InventoryDefinition(nullptr),
	StartRegionDefinition(nullptr)
{
	DisplayName = LOCTEXT("DefaultDisplayName", "플레이어 상인");
	TableKeyName = FName(TEXT("Player"));
	DefaultTeam = EBBMTeamType::Player;
}

FPrimaryAssetId UBBMPlayerCharacterDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(BBMPrimaryAssetTypes::PlayerCharacter, GetFName());
}

const UBBMInventoryDefinition* UBBMPlayerCharacterDefinition::GetInventoryDefinition() const
{
	if (InventoryDefinition.IsValid() == true)
	{
		return InventoryDefinition.Get();
	}
	return InventoryDefinition.LoadSynchronous();
}

const UBBMRegionDefinition* UBBMPlayerCharacterDefinition::GetStartRegionDefinition() const
{
	if (StartRegionDefinition.IsValid() == true)
	{
		return StartRegionDefinition.Get();
	}
	return StartRegionDefinition.LoadSynchronous();
}

#undef LOCTEXT_NAMESPACE
