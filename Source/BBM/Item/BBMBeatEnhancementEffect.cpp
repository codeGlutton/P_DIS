#include "Item/BBMBeatEnhancementEffect.h"
#include "Item/BBMBeatEnhancementSubclasses.h"
#include "Enhancement/DISEnhancement.h"

UBBMBeatEnhancementEffect::UBBMBeatEnhancementEffect()
{
}

void UBBMBeatEnhancementEffect::AddModifier(TSubclassOf<UDISEnhancement> InEnhancementClass, EDISEnhancementLevelOp InLevelOp, float InMagnitude)
{
	if (InEnhancementClass == nullptr)
	{
		return;
	}

	Modifiers.Add(InEnhancementClass, FDISEnhancementModifier(InLevelOp, InMagnitude));
}

UBBMBeatEnhancementEffect* UBBMBeatEnhancementEffect::CreateBeatEnhancementEffect(
	UObject* Outer,
	TSubclassOf<UDISEnhancement> InEnhancementClass,
	float InBonusLevel)
{
	if (Outer == nullptr || InEnhancementClass == nullptr || InBonusLevel <= 0.0f)
	{
		return nullptr;
	}

	UBBMBeatEnhancementEffect* NewEffect = NewObject<UBBMBeatEnhancementEffect>(Outer);
	NewEffect->AddModifier(InEnhancementClass, EDISEnhancementLevelOp::Add, InBonusLevel);
	return NewEffect;
}

TSubclassOf<UDISEnhancementEffect> UBBMBeatEnhancementEffect::GetEffectClassForEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass)
{
	if (InEnhancementClass == nullptr)
	{
		return nullptr;
	}

	if (InEnhancementClass->IsChildOf(UBBMEffectRangeBeatEnhancement::StaticClass()))
	{
		return UBBMEffectRangeBeatEnhancementEffect::StaticClass();
	}

	if (InEnhancementClass->IsChildOf(UBBMConditionalActionBeatEnhancement::StaticClass()))
	{
		return UBBMConditionalActionBeatEnhancementEffect::StaticClass();
	}

	return nullptr;
}

UBBMEffectRangeBeatEnhancementEffect::UBBMEffectRangeBeatEnhancementEffect()
{
	// CDO 기본 모디파이어: UBBMEffectRangeBeatEnhancement 스탯에 Add 1.0f 모디파이어 등록
	// Spec.OverallMagnitude를 곱하여 원하는 레벨만큼 가산됨
	Modifiers.Add(UBBMEffectRangeBeatEnhancement::StaticClass(), FDISEnhancementModifier(EDISEnhancementLevelOp::Add, 1.0f));
}

UBBMConditionalActionBeatEnhancementEffect::UBBMConditionalActionBeatEnhancementEffect()
{
	// CDO 기본 모디파이어: UBBMConditionalActionBeatEnhancement 스탯에 Add 1.0f 모디파이어 등록
	// Spec.OverallMagnitude를 곱하여 원하는 레벨만큼 가산됨
	Modifiers.Add(UBBMConditionalActionBeatEnhancement::StaticClass(), FDISEnhancementModifier(EDISEnhancementLevelOp::Add, 1.0f));
}

