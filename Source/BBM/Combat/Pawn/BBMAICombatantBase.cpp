#include "Combat/Pawn/BBMAICombatantBase.h"

ABBMAICombatantBase::ABBMAICombatantBase()
{
	// AI 전투원 기본 팀 설정 (정적 정의 에셋 할당 시 해당 팀으로 자동 동기화됨)
	SetGenericTeamId(FGenericTeamId(static_cast<uint8>(EBBMTeamType::Monster)));
}

