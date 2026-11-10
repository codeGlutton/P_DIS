/*****************************************************************//**
 * @file   BBMRunSaveGame.cpp
 * @brief  BBM(비트 배낭 상인) 단일 런(Run) 진행 데이터 세이브 게임 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Save/BBMRunSaveGame.h"

const FString UBBMRunSaveGame::SlotName = TEXT("BBMCurrentRun");

UBBMRunSaveGame::UBBMRunSaveGame()
{
	ResetRunData();
}

void UBBMRunSaveGame::ResetRunData()
{
	bHasActiveRun = false;
	PlayerCharacterId = FPrimaryAssetId();
	CurrentHealth = 100.0f;
	MaxHealth = 100.0f;
	CurrentGold = 0;
	SavedInventorySlots.Empty();
	ArchivedEquipments.Empty();
	CurrentRegionId = FPrimaryAssetId();
	CurrentEncounterIndex = 0;
	PlannedEncounterIds.Empty();
	SavedPlannedEncounters.Empty();
	RngSeed = 0;
}

