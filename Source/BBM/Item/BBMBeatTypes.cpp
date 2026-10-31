#include "Item/BBMBeatTypes.h"

FBBMEffectBase::FBBMEffectBase() :
	EffectCategory(EBBMEffectCategory::Damage)
{
}

FBBMEffectBase::FBBMEffectBase(EBBMEffectCategory InCategory) :
	EffectCategory(InCategory)
{
}

EBBMEffectCategory FBBMEffectBase::GetEffectCategory() const
{
	return EffectCategory;
}

FBBMCombatEffect::FBBMCombatEffect() :
	FBBMEffectBase(EBBMEffectCategory::Damage),
	TargetScope(EBBMTargetScope::EnemyFrontline),
	Coefficient(1.0f),
	BaseValue(0.0f)
{
}

FBBMCombatEffect::FBBMCombatEffect(EBBMEffectCategory InCategory, EBBMTargetScope InScope, float InCoefficient, float InBaseValue) :
	FBBMEffectBase(InCategory),
	TargetScope(InScope),
	Coefficient(InCoefficient),
	BaseValue(InBaseValue)
{
}

float FBBMCombatEffect::CalculateMagnitude(float InAttributeValue) const
{
	return (InAttributeValue * Coefficient) + BaseValue;
}

FBBMMovementEffect::FBBMMovementEffect() :
	FBBMEffectBase(EBBMEffectCategory::PreMovement),
	StepCount(1)
{
}

FBBMMovementEffect::FBBMMovementEffect(EBBMEffectCategory InCategory, int32 InStepCount) :
	FBBMEffectBase(InCategory),
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
	Effects(),
	Animations()
{
}

bool FBBMBeatDefinition::HasEffects() const
{
	return Effects.IsEmpty() == false;
}

bool FBBMBeatDefinition::HasAnimations() const
{
	return Animations.IsEmpty() == false;
}

FBBMBeatEffectRange::FBBMBeatEffectRange() :
	RangeName(NAME_None),
	StartBeat(0),
	EndBeat(1),
	GrantedEffects()
{
}

FBBMBeatEffectRange::FBBMBeatEffectRange(int32 InStartBeat, int32 InEndBeat, FName InRangeName) :
	RangeName(InRangeName),
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
	// GDD 3.3 규격: 정확히 8개의 비트로 구성되어야 함
	if (Beats.Num() != 8)
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


