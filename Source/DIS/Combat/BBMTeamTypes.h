/*****************************************************************//**
 * @file   BBMTeamTypes.h
 * @brief  BBM(비트 배낭 상인) 전투 팀 및 태도 타입 정의
 * @date   2026-10-31
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "BBMTeamTypes.generated.h"

/**
 * BBM 전투 진영/팀 분류 열거형
 */
UENUM(BlueprintType)
enum class EBBMTeamType : uint8
{
	Neutral	= 0 UMETA(DisplayName = "중립"),
	Player	= 1 UMETA(DisplayName = "플레이어 (아군 상인/소환수)"),
	Monster	= 2 UMETA(DisplayName = "몬스터 (적군)")
};

/**
 * 특정 팀을 기준으로 타 팀들에 대한 태도(Friendly, Hostile, Neutral) 매핑 구조체
 */
USTRUCT(BlueprintType)
struct DIS_API FBBMTeamRelation
{
	GENERATED_BODY()

	/* 생성자 및 메서드 */
public:
	FBBMTeamRelation() = default;

	/**
	 * EBBMTeamType 열거형 항목 중 현재 Attitudes에 없는 누락된 팀 항목을 일괄 추가
	 * @param ContextTeam 현재 관계의 주체 팀 (자신과 같으면 Friendly, 타 팀은 Neutral로 기본 설정)
	 * @return 새로 추가된 팀 항목 개수
	 */
	int32 FillMissingTeams(EBBMTeamType ContextTeam = EBBMTeamType::Neutral);

	/* 프로퍼티 */
public:
	/* 대상 팀별 태도 매핑 */
	UPROPERTY(Category = "BBM|Team", EditAnywhere, BlueprintReadWrite)
	TMap<EBBMTeamType, TEnumAsByte<ETeamAttitude::Type>> Attitudes;
};

