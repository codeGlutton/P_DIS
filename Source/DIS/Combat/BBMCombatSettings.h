/*****************************************************************//**
 * @file   BBMCombatSettings.h
 * @brief  BBM 전투 및 팀 관계 전역 개발자 환경설정 클래스 정의
 * @details Project Settings를 통해 팀 간 관계(Friendly/Hostile/Neutral)를 관리하며 FGenericTeamId 태도 해석기를 제공함
 * @date   2026-10-31
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Combat/BBMTeamTypes.h"
#include "BBMCombatSettings.generated.h"

/**
 * BBM 전투 및 팀 관계 개발자 설정 클래스
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "BBM Combat Settings"))
class DIS_API UBBMCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMCombatSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/* FGenericTeamId 태도 해결 솔버 (Static Attitude Solver) */
public:
	/* 두 팀 식별자 간의 상호 태도(Friendly, Hostile, Neutral) 판정 */
	static ETeamAttitude::Type GetAttitude(FGenericTeamId OwnId, FGenericTeamId OtherId);

	/* 설정 프로퍼티 */
public:
	/* 팀별 상호 태도 매핑 관계 */
	UPROPERTY(Category = "Team", Config, EditAnywhere, BlueprintReadOnly)
	TMap<EBBMTeamType, FBBMTeamRelation> TeamRelations;
};

