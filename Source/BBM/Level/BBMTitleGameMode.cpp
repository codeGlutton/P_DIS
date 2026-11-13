/*****************************************************************//**
 * @file   BBMTitleGameMode.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 화면 전용 게임모드 클래스 구현
 * @date   2026-11-06
 *********************************************************************/

#include "Level/BBMTitleGameMode.h"
#include "UI/Widget/BBMTitleWidget.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "UI/BBMWorldUISubsystem.h"
#include "Engine/World.h"

ABBMTitleGameMode::ABBMTitleGameMode()
{
	// 타이틀 레벨 시작 시 타이틀 메뉴 위젯 및 뷰모델 자동 등록
	FBBMWorldUIConfig TitleMenuConfig;
	TitleMenuConfig.WidgetKey = TEXT("TitleMenu");
	TitleMenuConfig.CreationPhase = EBBMWidgetCreationPhase::BeforeEncounter;
	TitleMenuConfig.WidgetConfig.WidgetClass = UBBMTitleWidget::StaticClass();
	TitleMenuConfig.WidgetConfig.ZOrderLayer = EBBMWidgetZOrder::Base;
	TitleMenuConfig.ViewModelConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();

	AutoCreateUIs.Add(TitleMenuConfig);
}

void ABBMTitleGameMode::InitializeLevelSpecificGameMode()
{
	Super::InitializeLevelSpecificGameMode();

	// 타이틀 화면 진입 시 타이틀 ViewModel을 통해 UI 자동 오픈
	if (UWorld* World = GetWorld())
	{
		if (UBBMWorldUISubsystem* UISS = World->GetSubsystem<UBBMWorldUISubsystem>())
		{
			if (UBBMViewModelBase* TitleVM = UISS->GetViewModel(TEXT("TitleMenu")))
			{
				TitleVM->OpenWithAnim();
			}
		}
	}
}

