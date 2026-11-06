/*****************************************************************//**
 * @file   BBMGameModeBase.cpp
 * @brief  BBM(비트 배낭 상인) 기본 게임모드 베이스 클래스 구현
 * @date   2026-11-04
 *********************************************************************/

#include "Level/BBMGameModeBase.h"
#include "Level/BBMPlayerController.h"
#include "Level/BBMCameraActorBase.h"
#include "Stage/BBMStageSubsystem.h"
#include "Combat/Pawn/BBMPlayerMerchantBase.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

ABBMGameModeBase::ABBMGameModeBase() :
	bEnableCombatEncounter(true)
{
	PlayerControllerClass = ABBMPlayerController::StaticClass();
	DefaultPawnClass = ABBMCameraActorBase::StaticClass();
}

bool ABBMGameModeBase::IsCombatEncounterEnabled() const
{
	return bEnableCombatEncounter;
}

void ABBMGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// 인게임 레벨에서 인카운터 연동이 활성화된 경우에만 실행 (타이틀 레벨 격리)
	if (bEnableCombatEncounter == true)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UBBMStageSubsystem* StageSS = GI->GetSubsystem<UBBMStageSubsystem>())
			{
				// 활성화된 런 상태가 존재하는지 검증
				if (StageSS->GetCurrentRegion() != nullptr)
				{
					// 인카운터 및 1D 배틀 진형을 레벨에 적용 (카메라 이동, 상인/적 스폰, OnEncounterChanged 및 OnTransitionFinished 완결)
					StageSS->ApplyCurrentEncounterToLevel();
				}
			}
		}
	}
}

