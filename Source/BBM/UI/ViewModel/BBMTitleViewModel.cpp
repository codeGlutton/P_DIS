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

UBBMTitleViewModel::UBBMTitleViewModel()
{
}

void UBBMTitleViewModel::NotifyWidgetOpened()
{
	Super::NotifyWidgetOpened();

	// 타이틀 위젯이 열리는 시점에 최신 세이브 런 상태 동기화
	RefreshTitleState();
}

void UBBMTitleViewModel::SetCanContinue(bool bInCanContinue)
{
	UE_MVVM_SET_PROPERTY_VALUE(bCanContinue, bInCanContinue);
}

void UBBMTitleViewModel::SetContinueVisibility(ESlateVisibility InVisibility)
{
	UE_MVVM_SET_PROPERTY_VALUE(ContinueVisibility, InVisibility);
}

void UBBMTitleViewModel::SetActiveRunSummaryText(const FText& InSummaryText)
{
	UE_MVVM_SET_PROPERTY_VALUE(ActiveRunSummaryText, InSummaryText);
}

void UBBMTitleViewModel::RefreshTitleState()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		SetCanContinue(false);
		SetContinueVisibility(ESlateVisibility::Collapsed);
		SetActiveRunSummaryText(FText::GetEmpty());
		return;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (GI == nullptr)
	{
		SetCanContinue(false);
		SetContinueVisibility(ESlateVisibility::Collapsed);
		SetActiveRunSummaryText(FText::GetEmpty());
		return;
	}

	UBBMGameDataSubsystem* GameDataSS = GI->GetSubsystem<UBBMGameDataSubsystem>();
	if (GameDataSS == nullptr)
	{
		SetCanContinue(false);
		SetContinueVisibility(ESlateVisibility::Collapsed);
		SetActiveRunSummaryText(FText::GetEmpty());
		return;
	}

	// 영속 세이브 데이터 로드 보장
	GameDataSS->EnsureSaveDataLoaded();

	const bool bHasRun = GameDataSS->HasActiveRun();
	SetCanContinue(bHasRun);
	SetContinueVisibility(bHasRun ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bHasRun == true)
	{
		if (const UBBMRunSaveGame* RunData = GameDataSS->GetCurrentRunData())
		{
			const FText Summary = FText::Format(
				NSLOCTEXT("BBMTitle", "RunSummaryFmt", "진행도: {0}번째 만남"),
				FText::AsNumber(RunData->CurrentEncounterIndex)
			);
			SetActiveRunSummaryText(Summary);
		}
		else
		{
			SetActiveRunSummaryText(FText::GetEmpty());
		}
	}
	else
	{
		SetActiveRunSummaryText(FText::GetEmpty());
	}
}

void UBBMTitleViewModel::StartNewGame()
{
	OnStartNewGameRequested.Broadcast();
	OnStartNewGameRequestedNative.Broadcast();
}

void UBBMTitleViewModel::ContinueGame()
{
	if (bCanContinue == true)
	{
		OnContinueGameRequested.Broadcast();
		OnContinueGameRequestedNative.Broadcast();
	}
}

void UBBMTitleViewModel::OpenSettings()
{
	OnOpenSettingsRequested.Broadcast();
	OnOpenSettingsRequestedNative.Broadcast();
}

void UBBMTitleViewModel::OpenCredits()
{
	OnOpenCreditsRequested.Broadcast();
	OnOpenCreditsRequestedNative.Broadcast();
}

void UBBMTitleViewModel::QuitGame()
{
	if (UWorld* World = GetWorld())
	{
		UKismetSystemLibrary::QuitGame(World, nullptr, EQuitPreference::Quit, false);
	}
}

