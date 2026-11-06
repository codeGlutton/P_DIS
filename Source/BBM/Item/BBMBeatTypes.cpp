#include "Item/BBMBeatTypes.h"
#include "Combat/BBMCombatManagerSubsystem.h"
#include "Combat/Pawn/BBMCombatantBase.h"
#include "Combat/GAS/BBMAttributeSet.h"

FBBMBeatAction::FBBMBeatAction() :
	ActionCategory(EBBMActionCategory::Damage)
{
}

FBBMBeatAction::FBBMBeatAction(EBBMActionCategory InCategory) :
	ActionCategory(InCategory)
{
}

EBBMActionCategory FBBMBeatAction::GetActionCategory() const
{
	return ActionCategory;
}

void FBBMBeatAction::ExecuteAction(UBBMCombatManagerSubsystem* CombatMgr, ABBMCombatantBase* Instigator, TMap<TWeakObjectPtr<ABBMCombatantBase>, FBBMCombatantActionLog>& OutCombatLogs) const
{
	// 베이스는 아무 동작도 수행하지 않음
}

FBBMCombatAction::FBBMCombatAction() :
	FBBMBeatAction(EBBMActionCategory::Damage),
	TargetScope(EBBMTargetScope::EnemyFrontline),
	Coefficient(1.0f),
	BaseValue(0.0f)
{
}

FBBMCombatAction::FBBMCombatAction(EBBMActionCategory InCategory, EBBMTargetScope InScope, float InCoefficient, float InBaseValue) :
	FBBMBeatAction(InCategory),
	TargetScope(InScope),
	Coefficient(InCoefficient),
	BaseValue(InBaseValue)
{
}

float FBBMCombatAction::CalculateMagnitude(float InAttributeValue) const
{
	return (InAttributeValue * Coefficient) + BaseValue;
}

void FBBMCombatAction::ExecuteAction(UBBMCombatManagerSubsystem* CombatMgr, ABBMCombatantBase* Instigator, TMap<TWeakObjectPtr<ABBMCombatantBase>, FBBMCombatantActionLog>& OutCombatLogs) const
{
	if (CombatMgr == nullptr || Instigator == nullptr)
	{
		return;
	}

	// 1. 시전자 진영(Lane) 판정
	EBBMCombatLane MyLane = EBBMCombatLane::AllyLane;
	EBBMCombatLane OpposingLane = EBBMCombatLane::EnemyLane;
	if (CombatMgr->GetLivingCombatantsInLane(EBBMCombatLane::AllyLane).Contains(Instigator) == false)
	{
		MyLane = EBBMCombatLane::EnemyLane;
		OpposingLane = EBBMCombatLane::AllyLane;
	}

	// 2. 대상(들) 수집
	TArray<ABBMCombatantBase*> Targets;
	switch (TargetScope)
	{
	case EBBMTargetScope::EnemyFrontline:
		if (ABBMCombatantBase* Front = CombatMgr->GetFrontlineCombatant(OpposingLane))
		{
			Targets.Add(Front);
		}
		break;
	case EBBMTargetScope::EnemyAll:
		Targets = CombatMgr->GetLivingCombatantsInLane(OpposingLane);
		break;
	case EBBMTargetScope::Self:
		Targets.Add(Instigator);
		break;
	case EBBMTargetScope::AllAllies:
		Targets = CombatMgr->GetLivingCombatantsInLane(MyLane);
		break;
	case EBBMTargetScope::AllyFrontline:
		if (ABBMCombatantBase* Front = CombatMgr->GetFrontlineCombatant(MyLane))
		{
			Targets.Add(Front);
		}
		break;
	default:
		break;
	}

	// 3. 수치 계산
	float AttrStat = 10.0f;
	if (const UBBMAttributeSet* AttrSet = Instigator->GetAttributeSet())
	{
		AttrStat = (ActionCategory == EBBMActionCategory::Damage) ? AttrSet->GetAttackPower() : AttrSet->GetDefensePower();
	}
	const float Magnitude = CalculateMagnitude(AttrStat);

	// 4. 액션 범주별 명령 실행 및 로그 기록 위임
	if (ActionCategory == EBBMActionCategory::Damage)
	{
		CombatMgr->ResolveMultiAttack(Instigator, Targets, Magnitude, OutCombatLogs);
	}
	else if (ActionCategory == EBBMActionCategory::Defense)
	{
		for (ABBMCombatantBase* Target : Targets)
		{
			CombatMgr->AddTurnArmor(Target, Magnitude, OutCombatLogs);
		}
	}
}

FBBMMovementAction::FBBMMovementAction() :
	FBBMBeatAction(EBBMActionCategory::PreMovement),
	Direction(EBBMLaneShiftDirection::Frontward),
	StepCount(1)
{
}

FBBMMovementAction::FBBMMovementAction(EBBMActionCategory InCategory, EBBMLaneShiftDirection InDirection, int32 InStepCount) :
	FBBMBeatAction(InCategory),
	Direction(InDirection),
	StepCount(InStepCount)
{
}

void FBBMMovementAction::ExecuteAction(UBBMCombatManagerSubsystem* CombatMgr, ABBMCombatantBase* Instigator, TMap<TWeakObjectPtr<ABBMCombatantBase>, FBBMCombatantActionLog>& OutCombatLogs) const
{
	if (CombatMgr == nullptr || Instigator == nullptr)
	{
		return;
	}

	EBBMCombatLane MyLane = EBBMCombatLane::AllyLane;
	if (CombatMgr->GetLivingCombatantsInLane(EBBMCombatLane::AllyLane).Contains(Instigator) == false)
	{
		MyLane = EBBMCombatLane::EnemyLane;
	}

	CombatMgr->ShiftLane(MyLane, Direction, StepCount);
}

FBBMArmorAction::FBBMArmorAction() :
	FBBMBeatAction(EBBMActionCategory::Defense),
	bIsTurnArmor(true),
	TargetScope(EBBMTargetScope::Self),
	Coefficient(1.0f),
	BaseValue(0.0f)
{
}

FBBMArmorAction::FBBMArmorAction(bool bInIsTurnArmor, EBBMTargetScope InScope, float InCoefficient, float InBaseValue) :
	FBBMBeatAction(EBBMActionCategory::Defense),
	bIsTurnArmor(bInIsTurnArmor),
	TargetScope(InScope),
	Coefficient(InCoefficient),
	BaseValue(InBaseValue)
{
}

float FBBMArmorAction::CalculateMagnitude(float InAttributeValue) const
{
	return (InAttributeValue * Coefficient) + BaseValue;
}

void FBBMArmorAction::ExecuteAction(UBBMCombatManagerSubsystem* CombatMgr, ABBMCombatantBase* Instigator, TMap<TWeakObjectPtr<ABBMCombatantBase>, FBBMCombatantActionLog>& OutCombatLogs) const
{
	if (CombatMgr == nullptr || Instigator == nullptr)
	{
		return;
	}

	// 1. 시전자 진영(Lane) 판정
	EBBMCombatLane MyLane = EBBMCombatLane::AllyLane;
	if (CombatMgr->GetLivingCombatantsInLane(EBBMCombatLane::AllyLane).Contains(Instigator) == false)
	{
		MyLane = EBBMCombatLane::EnemyLane;
	}

	// 2. 대상(들) 수집
	TArray<ABBMCombatantBase*> Targets;
	switch (TargetScope)
	{
	case EBBMTargetScope::Self:
		Targets.Add(Instigator);
		break;
	case EBBMTargetScope::AllAllies:
		Targets = CombatMgr->GetLivingCombatantsInLane(MyLane);
		break;
	case EBBMTargetScope::AllyFrontline:
		if (ABBMCombatantBase* Front = CombatMgr->GetFrontlineCombatant(MyLane))
		{
			Targets.Add(Front);
		}
		break;
	default:
		Targets.Add(Instigator);
		break;
	}

	// 3. DefensePower 기반 방어도 계산
	float DefPower = 10.0f;
	if (const UBBMAttributeSet* AttrSet = Instigator->GetAttributeSet())
	{
		DefPower = AttrSet->GetDefensePower();
	}
	const float Magnitude = CalculateMagnitude(DefPower);

	// 4. 지속 방어도 또는 턴 방어도 부여 및 로그 기록 위임
	for (ABBMCombatantBase* Target : Targets)
	{
		if (bIsTurnArmor == true)
		{
			CombatMgr->AddTurnArmor(Target, Magnitude, OutCombatLogs);
		}
		else
		{
			CombatMgr->AddPersistentArmor(Target, Magnitude, OutCombatLogs);
		}
	}
}

FBBMBeatAnimationConfig::FBBMBeatAnimationConfig() :
	VisualWindow(EBBMVisualWindow::PreMoveWindow),
	StateTag()
{
}

FBBMBeatAnimationConfig::FBBMBeatAnimationConfig(EBBMVisualWindow InVisualWindow, const FGameplayTag& InStateTag) :
	VisualWindow(InVisualWindow),
	StateTag(InStateTag)
{
}

bool FBBMBeatAnimationConfig::IsValid() const
{
	return StateTag.IsValid();
}

FBBMBeatDefinition::FBBMBeatDefinition() :
	Actions(),
	Animations()
{
}

bool FBBMBeatDefinition::HasActions() const
{
	return Actions.IsEmpty() == false;
}

bool FBBMBeatDefinition::HasAnimations() const
{
	return Animations.IsEmpty() == false;
}

FBBMBeatEffectRange::FBBMBeatEffectRange() :
	StartBeat(0),
	EndBeat(1),
	GrantedEffects()
{
}

FBBMBeatEffectRange::FBBMBeatEffectRange(int32 InStartBeat, int32 InEndBeat) :
	StartBeat(InStartBeat),
	EndBeat(InEndBeat),
	GrantedEffects()
{
}

bool FBBMBeatEffectRange::IsValid() const
{
	// 0 <= StartBeat < EndBeat <= 8 범위 검증
	return (StartBeat >= 0) && (EndBeat <= 8) && (StartBeat < EndBeat);
}

FBBMBeatAnimLayerRange::FBBMBeatAnimLayerRange() :
	StartBeat(0),
	EndBeat(1),
	AnimLayerTag()
{
}

FBBMBeatAnimLayerRange::FBBMBeatAnimLayerRange(int32 InStartBeat, int32 InEndBeat) :
	StartBeat(InStartBeat),
	EndBeat(InEndBeat),
	AnimLayerTag()
{
}

FBBMBeatAnimLayerRange::FBBMBeatAnimLayerRange(int32 InStartBeat, int32 InEndBeat, const FGameplayTag& InAnimLayerTag) :
	StartBeat(InStartBeat),
	EndBeat(InEndBeat),
	AnimLayerTag(InAnimLayerTag)
{
}

bool FBBMBeatAnimLayerRange::IsValid() const
{
	// 0 <= StartBeat < EndBeat <= 8 범위 검증 및 유효 태그 검증
	return (StartBeat >= 0) && (EndBeat <= 8) && (StartBeat < EndBeat) && AnimLayerTag.IsValid();
}

FBBMBeatSequence::FBBMBeatSequence() :
	Beats(),
	EffectRanges(),
	AnimLayerRanges()
{
}

bool FBBMBeatSequence::IsValid() const
{
	// 최대 8비트 이하의 비트 시퀀스여야 함 (아이템은 1~8비트 가변, 턴/AI는 8비트)
	if (Beats.Num() > 8)
	{
		return false;
	}

	// 모든 EffectRange가 유효한 비트 범위인지 검증
	for (const FBBMBeatEffectRange& Range : EffectRanges)
	{
		if (Range.IsValid() == false)
		{
			return false;
		}
	}

	// 모든 AnimLayerRange가 유효한 비트 범위인지 검증
	for (const FBBMBeatAnimLayerRange& LayerRange : AnimLayerRanges)
	{
		if (LayerRange.IsValid() == false)
		{
			return false;
		}
	}

	return true;
}

bool FBBMBeatSequence::IsFullEightBeats() const
{
	// GDD 3.3 규격: 완전한 1턴 실행을 위해서는 정확히 8개의 비트로 구성되어야 함
	return (Beats.Num() == 8) && (IsValid() == true);
}


