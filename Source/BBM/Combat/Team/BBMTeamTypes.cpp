#include "Combat/Team/BBMTeamTypes.h"
#include "UObject/EnumProperty.h"

int32 FBBMTeamRelation::FillMissingTeams(EBBMTeamType ContextTeam)
{
	int32 AddedCount = 0;
	const UEnum* TeamEnum = StaticEnum<EBBMTeamType>();
	if (TeamEnum == nullptr)
	{
		return AddedCount;
	}

	const int32 NumEnums = TeamEnum->NumEnums();
	for (int32 Index = 0; Index < NumEnums; ++Index)
	{
		// _MAX 및 Hidden 항목 제외
		if (TeamEnum->ContainsExistingMax() == true && Index == NumEnums - 1)
		{
			continue;
		}

		if (TeamEnum->HasMetaData(TEXT("Hidden"), Index) == true)
		{
			continue;
		}

		const int64 EnumValue = TeamEnum->GetValueByIndex(Index);
		if (EnumValue < 0)
		{
			continue;
		}

		const EBBMTeamType Team = static_cast<EBBMTeamType>(EnumValue);
		if (Attitudes.Contains(Team) == false)
		{
			// 현재 주체 팀과 동일한 팀이면 Friendly, 그 외는 Neutral로 기본 설정
			const ETeamAttitude::Type DefaultAttitude = (Team == ContextTeam) ? ETeamAttitude::Friendly : ETeamAttitude::Neutral;
			Attitudes.Add(Team, DefaultAttitude);
			++AddedCount;
		}
	}

	return AddedCount;
}

