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
	ShiningTransition = CreateDefaultSubobject<UBBMSingleAnimTransition>(TEXT("ShiningTransition"));
	BlockingTransition = CreateDefaultSubobject<UBBMDualAnimTransition>(TEXT("BlockingTransition"));
}

void UBBMTitleViewModel::OnOpen()
{
	Super::OnOpen();

	// 타이틀 뷰모델이 열리는 시점에 최신 세이브 런 상태 동기화 및 반짝임 루프 애니메이션 시작
	RefreshTitleState();
}

void UBBMTitleViewModel::OnFinishOpenAnim()
{
	Super::OnFinishOpenAnim();

	if (ShiningTransition != nullptr)
	{
		ShiningTransition->Execute(true);
	}
}

void UBBMTitleViewModel::OnClosed()
{
	Super::OnClosed();

	// 타이틀이 완전히 닫히면 무한 반복 중인 반짝임 애니메이션 및 블로킹 상태를 즉시 정리
	if (ShiningTransition != nullptr)
	{
		ShiningTransition->Execute(false);
	}
}

void UBBMTitleViewModel::SetCanContinue(bool bInCanContinue)
{
	UE_MVVM_SET_PROPERTY_VALUE(bCanContinue, bInCanContinue);
}

void UBBMTitleViewModel::SetActiveRunEncounterIndex(int32 InEncounterIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(ActiveRunEncounterIndex, InEncounterIndex);
}

void UBBMTitleViewModel::SetSettingsViewModel(UBBMSettingsViewModel* InSettingsViewModel)
{
	UE_MVVM_SET_PROPERTY_VALUE(SettingsViewModel, InSettingsViewModel);
}

void UBBMTitleViewModel::SetCharacterSelectViewModel(UBBMCharacterSelectViewModel* InCharacterSelectViewModel)
{
	UE_MVVM_SET_PROPERTY_VALUE(CharacterSelectViewModel, InCharacterSelectViewModel);
}

void UBBMTitleViewModel::SetCreditsViewModel(UBBMCreditsViewModel* InCreditsViewModel)
{
	UE_MVVM_SET_PROPERTY_VALUE(CreditsViewModel, InCreditsViewModel);
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

