#include "Item/BBMBeatEnhancementSubclasses.h"
#include "Curves/CurveFloat.h"

/* ====================================================================
 * UBBMEffectRangeBeatEnhancement 구현 (1번 타입)
 * ==================================================================== */

UBBMEffectRangeBeatEnhancement::UBBMEffectRangeBeatEnhancement() :
	AddedEffectRange(),
	MagnitudeCurve(nullptr)
{
	// EffectRange는 액션들이 정렬/주입된 후 마지막 단계에서 추가되도록 기본 우선순위 100 부여
	ExecutionPriority = 100;
}

void UBBMEffectRangeBeatEnhancement::ApplyBeatModification(int32 InLevel, const FBBMBeatSequence& BaseSequence, FBBMBeatSequence& InOutDynamicSequence) const
{
	if (AddedEffectRange.IsValid() == false)
	{
		return;
	}

	FBBMBeatEffectRange RangeToAdd = AddedEffectRange;

	// Curve 기반 수치 스케일링 (지정되지 않은 경우 원본 1.0f 배율 유지)
	if (MagnitudeCurve != nullptr)
	{
		const float Scale = MagnitudeCurve->GetFloatValue(static_cast<float>(InLevel));
		for (FBBMItemGameplayEffectConfig& EffectConfig : RangeToAdd.GrantedEffects)
		{
			for (auto& Pair : EffectConfig.SetByCallerMagnitudes)
			{
				Pair.Value *= Scale;
			}
		}
	}

	InOutDynamicSequence.EffectRanges.Add(RangeToAdd);
}

/* ====================================================================
 * UBBMConditionalActionBeatEnhancement 구현 (2, 3번 타입)
 * ==================================================================== */

UBBMConditionalActionBeatEnhancement::UBBMConditionalActionBeatEnhancement() :
	ConditionalAction(),
	MagnitudeCurve(nullptr)
{
	// 조건부 액션은 기본 액션이 배치된 후 주입되도록 기본 우선순위 50 부여
	ExecutionPriority = 50;
}

void UBBMConditionalActionBeatEnhancement::ApplyBeatModification(int32 InLevel, const FBBMBeatSequence& BaseSequence, FBBMBeatSequence& InOutDynamicSequence) const
{
	if (ConditionalAction.IsValid() == false)
	{
		return;
	}

	// 주입할 액션 인스턴스 복사 및 Curve 기반 레벨 스케일링
	TInstancedStruct<FBBMBeatAction> ScaledAction = ConditionalAction.ActionToInject;
	if (MagnitudeCurve != nullptr)
	{
		const float Scale = MagnitudeCurve->GetFloatValue(static_cast<float>(InLevel));
		if (FBBMCombatAction* CombatAction = ScaledAction.GetMutablePtr<FBBMCombatAction>())
		{
			CombatAction->Coefficient *= Scale;
			CombatAction->BaseValue *= Scale;
		}
	}

	// bTargetBaseBeatsOnly에 따라 탐색 대상 시퀀스 결정
	const FBBMBeatSequence& ScanTarget = ConditionalAction.bTargetBaseBeatsOnly ? BaseSequence : InOutDynamicSequence;
	const int32 NumBeats = ScanTarget.Beats.Num();

	for (int32 BeatIndex = 0; BeatIndex < NumBeats; ++BeatIndex)
	{
		// 해당 비트에 트리거 액션(Damage, Defense 등)이 존재하는지 확인
		const bool bHasTrigger = ScanTarget.Beats[BeatIndex].Actions.ContainsByPredicate(
			[this](const TInstancedStruct<FBBMBeatAction>& Action)
			{
				const FBBMBeatAction* ActionPtr = Action.GetPtr<FBBMBeatAction>();
				return (ActionPtr != nullptr) && (ActionPtr->ActionCategory == ConditionalAction.TriggerCategory);
			});

		if (bHasTrigger == true && InOutDynamicSequence.Beats.IsValidIndex(BeatIndex) == true)
		{
			InOutDynamicSequence.Beats[BeatIndex].Actions.Add(ScaledAction);
		}
	}
}

