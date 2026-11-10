/*****************************************************************//**
 * @file   BBMUserSaveGame.cpp
 * @brief  BBM(비트 배낭 상인) 영구 유저 설정 및 메타 데이터 세이브 게임 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Save/BBMUserSaveGame.h"

const FString UBBMUserSaveGame::SlotName = TEXT("BBMUserSettings");

UBBMUserSaveGame::UBBMUserSaveGame() :
	MasterVolume(1.0f),
	BGMVolume(0.8f),
	SFXVolume(1.0f),
	TotalRunsAttempted(0),
	TotalRunsCleared(0)
{
}

