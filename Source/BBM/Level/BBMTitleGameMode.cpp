/*****************************************************************//**
 * @file   BBMTitleGameMode.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 화면 전용 게임모드 클래스 구현
 * @date   2026-11-06
 *********************************************************************/

#include "Level/BBMTitleGameMode.h"
#include "UI/Widget/BBMTitleWidget.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "UI/ViewModel/BBMSettingsViewModel.h"
#include "UI/ViewModel/BBMCreditsViewModel.h"
#include "UI/ViewModel/BBMCharacterSelectViewModel.h"
#include "UI/BBMWorldUISubsystem.h"
#include "Engine/World.h"

ABBMTitleGameMode::ABBMTitleGameMode()
{
	// 타이틀 레벨 시작 시 타이틀 메뉴 위젯 및 뷰모델 자동 등록

	FBBMWorldUIConfig TitleMenuConfig;
	TitleMenuConfig.WidgetKey = TEXT("TitleMenu");
	TitleMenuConfig.CreationPhase = EBBMWidgetCreationPhase::BeforeEncounter;
	TitleMenuConfig.WidgetConfig.WidgetClass = UBBMTitleWidget::StaticClass();
	TitleMenuConfig.ViewModelConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();

	AutoCreateUIs.Add(TitleMenuConfig);

	// 타이틀 레벨 시작 시 세팅 위젯 및 뷰모델 자동 등록

	FBBMWorldUIConfig SettingsConfig;
	SettingsConfig.WidgetKey = TEXT("Settings");
	SettingsConfig.CreationPhase = EBBMWidgetCreationPhase::BeforeEncounter;
	SettingsConfig.WidgetConfig.WidgetClass = UBBMTitleWidget::StaticClass();
	SettingsConfig.ViewModelConfig.ViewModelClass = UBBMSettingsViewModel::StaticClass();

	AutoCreateUIs.Add(SettingsConfig);

	// 타이틀 레벨 시작 시 캐릭터	선택 위젯 및 뷰모델 자동 등록

	FBBMWorldUIConfig CharacterSelectConfig;
	CharacterSelectConfig.WidgetKey = TEXT("CharacterSelect");
	CharacterSelectConfig.CreationPhase = EBBMWidgetCreationPhase::BeforeEncounter;
	//CharacterSelectConfig.WidgetConfig.WidgetClass = UBBMCharacterSelectWidget::StaticClass();
	CharacterSelectConfig.ViewModelConfig.ViewModelClass = UBBMCharacterSelectViewModel::StaticClass();

	AutoCreateUIs.Add(SettingsConfig);

	// 타이틀 레벨 시작 시 크레딧 위젯 및 뷰모델 자동 등록

	FBBMWorldUIConfig CreditsConfig;
	CreditsConfig.WidgetKey = TEXT("Credits");
	CreditsConfig.CreationPhase = EBBMWidgetCreationPhase::BeforeEncounter;
	CreditsConfig.WidgetConfig.WidgetClass = UBBMTitleWidget::StaticClass();
	CreditsConfig.ViewModelConfig.ViewModelClass = UBBMCreditsViewModel::StaticClass();

	AutoCreateUIs.Add(CreditsConfig);
}

void ABBMTitleGameMode::InitializeLevelSpecificGameMode()
{
	Super::InitializeLevelSpecificGameMode();

	// 타이틀 화면 진입 시 타이틀 ViewModel을 통해 UI 자동 오픈
	if (UWorld* World = GetWorld())
	{
		if (UBBMWorldUISubsystem* UISS = World->GetSubsystem<UBBMWorldUISubsystem>())
		{
			UBBMSettingsViewModel* SettingsVM = UISS->GetViewModelAs<UBBMSettingsViewModel>(TEXT("Settings"));
			if (SettingsVM == nullptr)
			{
				SettingsVM->FinishInitialization();
			}

			UBBMCreditsViewModel* CreditsVM = UISS->GetViewModelAs<UBBMCreditsViewModel>(TEXT("Credits"));
			if (CreditsVM == nullptr)
			{
				CreditsVM->FinishInitialization();
			}

			UBBMCharacterSelectViewModel* CharacterSelectVM = UISS->GetViewModelAs<UBBMCharacterSelectViewModel>(TEXT("CharacterSelect"));
			if (CharacterSelectVM == nullptr)
			{
				CharacterSelectVM->FinishInitialization();
			}

			UBBMTitleViewModel* TitleVM = UISS->GetViewModelAs<UBBMTitleViewModel>(TEXT("TitleMenu"));
			if (TitleVM == nullptr)
			{
				TitleVM->SetSettingsViewModel(SettingsVM);
				TitleVM->SetCreditsViewModel(CreditsVM);
				TitleVM->SetCharacterSelectViewModel(CharacterSelectVM);
				TitleVM->FinishInitialization();

				TitleVM->OpenWithAnim();
			}
		}
	}
}

