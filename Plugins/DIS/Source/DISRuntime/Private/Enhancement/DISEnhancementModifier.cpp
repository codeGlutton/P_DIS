#include "Enhancement/DISEnhancementModifier.h"

FDISEnhancementModifier::FDISEnhancementModifier() :
	Target(EDISEnhancementModifierTarget::CurrentLevel),
	LevelOp(EDISEnhancementLevelOp::Add),
	LevelMagnitude(1.0f)
{
}

FDISEnhancementModifier::FDISEnhancementModifier(EDISEnhancementLevelOp InLevelOp, float InLevelMagnitude) :
	Target(EDISEnhancementModifierTarget::CurrentLevel),
	LevelOp(InLevelOp),
	LevelMagnitude(InLevelMagnitude)
{
}

FDISEnhancementModifier::FDISEnhancementModifier(EDISEnhancementModifierTarget InTarget, EDISEnhancementLevelOp InLevelOp, float InLevelMagnitude) :
	Target(InTarget),
	LevelOp(InLevelOp),
	LevelMagnitude(InLevelMagnitude)
{
}
