/*****************************************************************//**
 * @file   BBMStageGameMode.h
 * @brief  BBM(비트 배낭 상인) 스테이지/인게임 레벨 전용 게임모드 클래스 정의
 * @details 인게임 레벨 진입 시 활성 런을 감지하여 초기 1번 만남 스폰 및 전투 환경을 자동 트리거함
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Level/BBMGameModeBase.h"
#include "BBMStageGameMode.generated.h"

/**
 * BBM 스테이지(인게임) 전용 게임모드 클래스
 * BeforeEncounter 위젯 생성 후 인카운터(전투/이벤트/보물)를 레벨에 적용하고 AfterEncounter 위젯을 생성함
 */
UCLASS(Blueprintable, meta = (DisplayName = "BBM Stage Game Mode"))
class BBM_API ABBMStageGameMode : public ABBMGameModeBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMStageGameMode();

	/* 게임모드 라이프사이클 오버라이드 */
protected:
	virtual void InitializeLevelSpecificGameMode() override;
};

