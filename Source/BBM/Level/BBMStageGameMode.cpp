/*****************************************************************//**
 * @file   BBMStageGameMode.cpp
 * @brief  BBM(비트 배낭 상인) 스테이지/인게임 레벨 전용 게임모드 클래스 구현
 * @date   2026-11-06
 *********************************************************************/

#include "Level/BBMStageGameMode.h"
#include "Stage/BBMStageSubsystem.h"
#include "Engine/GameInstance.h"

ABBMStageGameMode::ABBMStageGameMode()
{
}

void ABBMStageGameMode::InitializeLevelSpecificGameMode()
{
	Super::InitializeLevelSpecificGameMode();

	// 활성화된 런 상태가 존재하는 경우 인카운터 및 1D 배틀 진형을 레벨에 적용
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBBMStageSubsystem* StageSS = GI->GetSubsystem<UBBMStageSubsystem>())
		{
			if (StageSS->GetCurrentRegion() != nullptr)
			{
				StageSS->ApplyCurrentEncounterToLevel();
			}
		}
	}
}

