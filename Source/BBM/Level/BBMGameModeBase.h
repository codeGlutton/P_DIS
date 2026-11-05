/*****************************************************************//**
 * @file   BBMGameModeBase.h
 * @brief  BBM(비트 배낭 상인) 기본 게임모드 베이스 클래스 정의
 * @details 인게임 레벨 진입 시 활성 런을 감지하여 초기 1번 만남 스폰을 트리거하고 타이틀 레벨과의 동작을 격리함
 * @date   2026-11-04
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BBMGameModeBase.generated.h"

/**
 * BBM 기본 게임모드 베이스 클래스
 * 인게임 레벨에서는 bEnableCombatEncounter = true로 두어 초기 만남 스폰을 자동 트리거하며,
 * 타이틀 전용 게임모드에서는 false로 지정하여 인카운터/전투 매니저가 0% 유휴 상태를 유지하도록 격리함
 */
UCLASS(Blueprintable, meta = (DisplayName = "BBM Game Mode Base"))
class BBM_API ABBMGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	ABBMGameModeBase();

	/* AActor 라이프사이클 오버라이드 */
public:
	virtual void BeginPlay() override;

	/* 설정 프로퍼티 */
protected:
	/* 해당 레벨에서 인카운터 및 전투 매니저 초기화 자동 연동 활성화 여부 (인게임: true, 타이틀: false) */
	UPROPERTY(Category = "BBM|Encounter", EditDefaultsOnly, BlueprintReadOnly)
	bool bEnableCombatEncounter;
};

