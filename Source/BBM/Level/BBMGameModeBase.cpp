/*****************************************************************//**
 * @file   BBMGameModeBase.cpp
 * @brief  BBM(비트 배낭 상인) 기본 게임모드 베이스 클래스 구현
 * @date   2026-11-04
 *********************************************************************/

#include "Level/BBMGameModeBase.h"
#include "UI/BBMWorldWidgetSubsystem.h"
#include "UI/BBMUserWidgetBase.h"
#include "Engine/World.h"

ABBMGameModeBase::ABBMGameModeBase()
{
	// 베이스 게임모드는 폰 및 플레이어 컨트롤러를 기본적으로 비워둠 (파생 클래스에서 필요 시 지정)
	PlayerControllerClass = nullptr;
	DefaultPawnClass = nullptr;
}

void ABBMGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// 1단계: 인카운터 초기화 전 위젯 생성 (예: FadeLoading 위젯 등)
	CreateWorldWidgetsByPhase(EBBMWidgetCreationPhase::BeforeEncounter);

	// 2단계: 레벨 특화 게임모드 초기화 실행 (스테이지 게임모드에서 인카운터 배치 실행)
	InitializeLevelSpecificGameMode();

	// 3단계: 인카운터 초기화 후 위젯 생성 (예: 인게임 전투 HUD 등)
	CreateWorldWidgetsByPhase(EBBMWidgetCreationPhase::AfterEncounter);
}

void ABBMGameModeBase::CreateWorldWidgetsByPhase(EBBMWidgetCreationPhase Phase)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	UBBMWorldWidgetSubsystem* WidgetSS = World->GetSubsystem<UBBMWorldWidgetSubsystem>();
	if (WidgetSS == nullptr)
	{
		return;
	}

	for (const FBBMWorldWidgetConfig& Config : AutoCreateWidgets)
	{
		if (Config.CreationPhase == Phase && Config.WidgetClass != nullptr && Config.WidgetKey != NAME_None)
		{
			WidgetSS->CreateAndRegisterWidget(
				Config.WidgetClass,
				Config.WidgetKey,
				Config.ZOrderLayer
			);
		}
	}
}

void ABBMGameModeBase::InitializeLevelSpecificGameMode()
{
	// 베이스 클래스는 기본적으로 빈 구현 (파생 클래스에서 오버라이드)
}
