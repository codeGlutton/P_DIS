#include "Global/BBMGlobalSettings.h"

UBBMGlobalSettings::UBBMGlobalSettings(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("BBM Global Settings");

	// 기본 FGenericTeamId 태도 해결 솔버 바인딩
	FGenericTeamId::SetAttitudeSolver(&UBBMGlobalSettings::GetAttitude);
}

FName UBBMGlobalSettings::GetCategoryName() const
{
	return TEXT("Game");
}

FName UBBMGlobalSettings::GetSectionName() const
{
	return TEXT("BBM Global Settings");
}

ETeamAttitude::Type UBBMGlobalSettings::GetAttitude(FGenericTeamId OwnId, FGenericTeamId OtherId)
{
	if (OwnId == OtherId)
	{
		return ETeamAttitude::Friendly;
	}

	const UBBMGlobalSettings* Settings = GetDefault<UBBMGlobalSettings>();
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
