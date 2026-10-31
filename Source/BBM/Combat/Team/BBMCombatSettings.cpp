/*****************************************************************//**
 * @file   BBMCombatSettings.cpp
 * @brief  BBM 전투 및 팀 관계 전역 개발자 환경설정 클래스 구현
 * @date   2026-10-31
 *********************************************************************/

#include "Combat/Team/BBMCombatSettings.h"

UBBMCombatSettings::UBBMCombatSettings(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("BBM Combat Settings");

	// 기본 FGenericTeamId 태도 해결 솔버 바인딩
	FGenericTeamId::SetAttitudeSolver(&UBBMCombatSettings::GetAttitude);
}

ETeamAttitude::Type UBBMCombatSettings::GetAttitude(FGenericTeamId OwnId, FGenericTeamId OtherId)
{
	if (OwnId == OtherId)
	{
		return ETeamAttitude::Friendly;
	}

	const UBBMCombatSettings* Settings = GetDefault<UBBMCombatSettings>();
	if (Settings != nullptr)
	{
		const EBBMTeamType OwnTeam = static_cast<EBBMTeamType>(OwnId.GetId());
		const EBBMTeamType OtherTeam = static_cast<EBBMTeamType>(OtherId.GetId());

		const FBBMTeamRelation* OwnRelation = Settings->TeamRelations.Find(OwnTeam);
		if (OwnRelation != nullptr)
		{
			const TEnumAsByte<ETeamAttitude::Type>* Attitude = OwnRelation->Attitudes.Find(OtherTeam);
			if (Attitude != nullptr)
			{
				return *Attitude;
			}
		}
	}

	return ETeamAttitude::Neutral;
}

