/*****************************************************************//**
 * @file   BBMTitleGameMode.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 화면 전용 게임모드 클래스 구현
 * @date   2026-11-06
 *********************************************************************/

#include "Level/BBMTitleGameMode.h"
#include "UI/BBMTitleWidget.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "UI/BBMWorldWidgetSubsystem.h"
#include "Engine/World.h"

ABBMTitleGameMode::ABBMTitleGameMode()
{
	// 타이틀 레벨 시작 시 타이틀 메뉴 위젯 및 뷰모델 자동 등록
	FBBMWorldWidgetConfig TitleMenuConfig;
	TitleMenuConfig.WidgetKey = TEXT("TitleMenu");
	TitleMenuConfig.WidgetClass = UBBMTitleWidget::StaticClass();
	TitleMenuConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();
	TitleMenuConfig.CreationPhase = EBBMWidgetCreationPhase::BeforeEncounter;
	TitleMenuConfig.ZOrderLayer = EBBMWidgetZOrder::Base;

	AutoCreateWidgets.Add(TitleMenuConfig);
}

void ABBMTitleGameMode::InitializeLevelSpecificGameMode()
{
	Super::InitializeLevelSpecificGameMode();

	// 타이틀 화면 진입 시 타이틀 위젯 자동 오픈
	if (UWorld* World = GetWorld())
	{
		if (UBBMWorldWidgetSubsystem* WidgetSS = World->GetSubsystem<UBBMWorldWidgetSubsystem>())
		{
			if (UBBMUserWidgetBase* TitleWidget = WidgetSS->GetWidget(TEXT("TitleMenu")))
			{
				TitleWidget->Open();
			}
		}
	}
}

