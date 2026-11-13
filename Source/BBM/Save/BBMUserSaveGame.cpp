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
	DialogueVolume(1.0f),
	bEnableBeatVisualAssist(true),
	WindowMode(EWindowMode::WindowedFullscreen),
	ScreenResolution(1920, 1080),
	bVSyncEnabled(false),
	TotalRunsAttempted(0),
	TotalRunsCleared(0)
{
}

FBBMUserSettingsData UBBMUserSaveGame::ToSettingsData() const
{
	FBBMUserSettingsData OutData;
	OutData.MasterVolume = MasterVolume;
	OutData.BGMVolume = BGMVolume;
	OutData.SFXVolume = SFXVolume;
	OutData.DialogueVolume = DialogueVolume;
	OutData.bEnableBeatVisualAssist = bEnableBeatVisualAssist;
	OutData.WindowMode = WindowMode;
	OutData.ScreenResolution = ScreenResolution;
	OutData.bVSyncEnabled = bVSyncEnabled;
	OutData.ClampValues();
	return OutData;
}

void UBBMUserSaveGame::ApplySettingsData(const FBBMUserSettingsData& InSettingsData)
{
	FBBMUserSettingsData ClampedData = InSettingsData;
	ClampedData.ClampValues();

	MasterVolume = ClampedData.MasterVolume;
	BGMVolume = ClampedData.BGMVolume;
	SFXVolume = ClampedData.SFXVolume;
	DialogueVolume = ClampedData.DialogueVolume;
	bEnableBeatVisualAssist = ClampedData.bEnableBeatVisualAssist;
	WindowMode = ClampedData.WindowMode;
	ScreenResolution = ClampedData.ScreenResolution;
	bVSyncEnabled = ClampedData.bVSyncEnabled;
}

