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

