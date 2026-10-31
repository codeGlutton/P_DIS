#include "Combat/Pawn/BBMPlayerMerchantBase.h"

ABBMPlayerMerchantBase::ABBMPlayerMerchantBase()
{
	// 플레이어 상인 기본 팀 설정
	SetGenericTeamId(FGenericTeamId(static_cast<uint8>(EBBMTeamType::Player)));
}

