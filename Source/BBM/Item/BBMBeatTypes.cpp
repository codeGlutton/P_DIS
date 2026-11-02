#include "Item/BBMBeatTypes.h"

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

FBBMMovementAction::FBBMMovementAction() :
	FBBMBeatAction(EBBMActionCategory::PreMovement),
	StepCount(1)
{
}

FBBMMovementAction::FBBMMovementAction(EBBMActionCategory InCategory, int32 InStepCount) :
	FBBMBeatAction(InCategory),
	StepCount(InStepCount)
{
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

FBBMBeatSequence::FBBMBeatSequence() :
	Beats(),
	EffectRanges()
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

	return true;
}

bool FBBMBeatSequence::IsFullEightBeats() const
{
	// GDD 3.3 규격: 완전한 1턴 실행을 위해서는 정확히 8개의 비트로 구성되어야 함
	return (Beats.Num() == 8) && (IsValid() == true);
}


