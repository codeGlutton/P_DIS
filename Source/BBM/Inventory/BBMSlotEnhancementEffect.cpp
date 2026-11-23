#include "Inventory/BBMSlotEnhancementEffect.h"
#include "Inventory/BBMSlotEnhancement.h"

UBBMSlotEnhancementEffect::UBBMSlotEnhancementEffect()
{
}

UBBMSlotEffectRangeEnhancementEffect::UBBMSlotEffectRangeEnhancementEffect()
{
	// 슬롯 자체의 UBBMSlotEffectRangeEnhancement 스탯에 Add 1.0f 모디파이어 등록
	// Spec.OverallMagnitude를 곱하여 원하는 슬롯 보너스 레벨만큼 스케일링됨
	Modifiers.Add(UBBMSlotEffectRangeEnhancement::StaticClass(), FDISEnhancementModifier(EDISEnhancementLevelOp::Add, 1.0f));
}

UBBMSlotConditionalActionEnhancementEffect::UBBMSlotConditionalActionEnhancementEffect()
{
	// 슬롯 자체의 UBBMSlotConditionalActionEnhancement 스탯에 Add 1.0f 모디파이어 등록
	// Spec.OverallMagnitude를 곱하여 원하는 슬롯 보너스 레벨만큼 스케일링됨
	Modifiers.Add(UBBMSlotConditionalActionEnhancement::StaticClass(), FDISEnhancementModifier(EDISEnhancementLevelOp::Add, 1.0f));
}

