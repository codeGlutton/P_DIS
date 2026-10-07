#include "Enhancement/DISEnhancementModifier.h"

FDISEnhancementModifier::FDISEnhancementModifier() :
	LevelOp(EDISEnhancementLevelOp::Add),
	LevelMagnitude(1.0f)
{
}

FDISEnhancementModifier::FDISEnhancementModifier(EDISEnhancementLevelOp InLevelOp, float InLevelMagnitude) :
	LevelOp(InLevelOp),
	LevelMagnitude(InLevelMagnitude)
{
}
