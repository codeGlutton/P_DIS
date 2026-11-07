/*****************************************************************//**
 * @file   BBMTitleGameMode.h
 * @brief  BBM(비트 배낭 상인) 타이틀 화면 전용 게임모드 클래스 정의
 * @details 인카운터 및 전투 연동을 배제하고 타이틀 메뉴 및 초기 진입 전용 규칙을 관리함
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Level/BBMGameModeBase.h"
#include "BBMTitleGameMode.generated.h"

/**
 * BBM 타이틀 화면 전용 게임모드 클래스
 * 인카운터 배치 및 전투 매니저가 0% 유휴 상태를 유지하도록 인게임 로직과 격리됨
 */
UCLASS(Blueprintable, meta = (DisplayName = "BBM Title Game Mode"))
class BBM_API ABBMTitleGameMode : public ABBMGameModeBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMTitleGameMode();

	/* ABBMGameModeBase 가상 함수 오버라이드 */
protected:
	virtual void InitializeLevelSpecificGameMode() override;
};

