/*****************************************************************//**
 * @file   BBMCharacterDefinition.h
 * @brief  BBM(비트 배낭 상인) 캐릭터 기본 정적 정의 데이터 에셋 클래스 정의
 * @details 캐릭터 표시명, GAS CurveTable 행 파싱 키 이름(TableKeyName), 기본 소속 팀 등을 정의함
 * @date   2026-10-31
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Team/BBMTeamTypes.h"
#include "BBMCharacterDefinition.generated.h"

/**
 * BBM 호스트 게임 전역에서 사용하는 PrimaryAssetType 식별자 모음
 */
namespace BBMPrimaryAssetTypes
{
	/* BBM 캐릭터 정의 에셋 식별자 ("BBMCharacter") */
	constexpr const TCHAR* Character = TEXT("BBMCharacter");
}

/**
 * BBM 캐릭터 기본 정적 정의 데이터 에셋 클래스
 * 주인공 상인, 소환수 동료, 적 몬스터의 정적 정보를 관리함
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Character Definition"))
class BBM_API UBBMCharacterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMCharacterDefinition();

	/* PrimaryAssetId 오버라이드 */
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* Getter 헬퍼 */
public:
	/* 캐릭터 표시 이름 반환 */
	const FText& GetDisplayName() const;

	/* GAS 스탯 CurveTable 행 파싱 키 이름 반환 */
	FName GetTableKeyName() const;

	/* 기본 소속 팀 반환 */
	EBBMTeamType GetDefaultTeam() const;

	/* 정적 프로퍼티 */
public:
	/* UI 및 인게임 표기용 캐릭터 이름 */
	UPROPERTY(Category = "Character", EditDefaultsOnly, BlueprintReadOnly)
	FText DisplayName;

	/* GAS CompositeCurveTable 내 스탯 행 파싱 키 이름 (예: Player, Goblin, Boss 등) */
	UPROPERTY(Category = "Character|Stats", EditDefaultsOnly, BlueprintReadOnly)
	FName TableKeyName;

	/* 캐릭터의 기본 소속 팀 */
	UPROPERTY(Category = "Character|Team", EditDefaultsOnly, BlueprintReadOnly)
	EBBMTeamType DefaultTeam;
};
