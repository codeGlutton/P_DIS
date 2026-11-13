/*****************************************************************//**
 * @file   BBMTitleViewModel.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 화면 전용 MVVM 뷰모델 클래스 구현
 * @date   2026-11-07
 *********************************************************************/

#include "UI/ViewModel/BBMTitleViewModel.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Save/BBMRunSaveGame.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"

#include "UI/ViewModel/BBMSettingsViewModel.h"
#include "UI/ViewModel/BBMCharacterSelectViewModel.h"
#include "UI/ViewModel/BBMCreditsViewModel.h"

UBBMTitleViewModel::UBBMTitleViewModel()
{
	SettingsViewModel = CreateDefaultSubobject<UBBMSettingsViewModel>(TEXT("SettingsViewModel"));
	CharacterSelectViewModel = CreateDefaultSubobject<UBBMCharacterSelectViewModel>(TEXT("CharacterSelectViewModel"));
	CreditsViewModel = CreateDefaultSubobject<UBBMCreditsViewModel>(TEXT("CreditsViewModel"));
}

void UBBMTitleViewModel::OnOpen()
{
	Super::OnOpen();

	// 타이틀 뷰모델이 열리는 시점에 최신 세이브 런 상태 동기화
	RefreshTitleState();
}

ESlateVisibility UBBMTitleViewModel::GetContinueVisibility() const
{
	return bCanContinue == true ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
}

void UBBMTitleViewModel::SetCanContinue(bool bInCanContinue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bCanContinue, bInCanContinue) == false)
	{
		return;
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetContinueVisibility);
}

void UBBMTitleViewModel::SetActiveRunEncounterIndex(int32 InEncounterIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(ActiveRunEncounterIndex, InEncounterIndex);
}

void UBBMTitleViewModel::RefreshTitleState()
{
	UWorld* World = GetWorld();
	UGameInstance* GI = World != nullptr ? World->GetGameInstance() : nullptr;
	UBBMGameDataSubsystem* GameDataSS = GI != nullptr ? GI->GetSubsystem<UBBMGameDataSubsystem>() : nullptr;
	if (GameDataSS == nullptr)
	{
		SetCanContinue(false);
		SetActiveRunEncounterIndex(0);
		return;
	}

	// 영속 세이브 데이터 로드 보장
	GameDataSS->EnsureSaveDataLoaded();

	const bool bHasRun = GameDataSS->HasActiveRun();
	SetCanContinue(bHasRun);

	const UBBMRunSaveGame* RunData = (bHasRun == true) ? GameDataSS->GetCurrentRunData() : nullptr;
	SetActiveRunEncounterIndex(RunData != nullptr ? RunData->CurrentEncounterIndex : 0);
}

void UBBMTitleViewModel::ContinueGame()
{
	if (bCanContinue == true)
	{
	}
}

void UBBMTitleViewModel::OpenSettings()
{
	if (SettingsViewModel != nullptr)
	{
		SettingsViewModel->OpenWithAnim();
	}
}

void UBBMTitleViewModel::OpenCredits()
{
	if (CreditsViewModel != nullptr)
	{
		CreditsViewModel->OpenWithAnim();
	}
}

void UBBMTitleViewModel::OpenCharacterSelections()
{
	if (CharacterSelectViewModel != nullptr)
	{
		CharacterSelectViewModel->OpenWithAnim();
	}
}

void UBBMTitleViewModel::QuitGame()
{
	if (UWorld* World = GetWorld())
	{
		UKismetSystemLibrary::QuitGame(World, nullptr, EQuitPreference::Quit, false);
	}
}

