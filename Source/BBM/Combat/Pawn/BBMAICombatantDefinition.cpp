#include "Combat/Pawn/BBMAICombatantDefinition.h"
#include "Math/RandomStream.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#define LOCTEXT_NAMESPACE "UBBMAICombatantDefinition"
#endif

FBBMAIBeatSequence::FBBMAIBeatSequence() :
	PatternName(NAME_None),
	SelectionWeight(1.0f),
	bCanRepeatConsecutively(true),
	Sequence()
{
}

bool FBBMAIBeatSequence::IsValid() const
{
	return (SelectionWeight >= 0.0f) && Sequence.IsValid();
}

UBBMAICombatantDefinition::UBBMAICombatantDefinition()
{
	DisplayName = FText::FromString(TEXT("기본 AI 전투원"));
	TableKeyName = FName(TEXT("Monster"));
	DefaultTeam = EBBMTeamType::Monster;
}

FPrimaryAssetId UBBMAICombatantDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(BBMPrimaryAssetTypes::AICombatant, GetFName());
}

const FBBMAIBeatSequence* UBBMAICombatantDefinition::SelectPattern(FRandomStream& InRandomStream, FName LastPatternName) const
{
	if (PatternPool.IsEmpty() == true)
	{
		return nullptr;
	}

	// 1. 후보군 필터링 (가중치 0 이하 제외, 연속 사용 불가 패턴 제외)
	TArray<const FBBMAIBeatSequence*> ValidCandidates;
	ValidCandidates.Reserve(PatternPool.Num());
	float TotalWeight = 0.0f;

	for (const FBBMAIBeatSequence& Pattern : PatternPool)
	{
		if (Pattern.SelectionWeight <= 0.0f)
		{
			continue;
		}

		// 직전 패턴과 같고 연속 사용이 금지된 경우 제외
		if (LastPatternName != NAME_None && Pattern.PatternName == LastPatternName && Pattern.bCanRepeatConsecutively == false)
		{
			continue;
		}

		ValidCandidates.Add(&Pattern);
		TotalWeight += Pattern.SelectionWeight;
	}

	// 2. 만약 연속 사용 방지로 인해 후보가 전멸한 경우, 가중치가 유효한 모든 패턴으로 대체 허용
	if (ValidCandidates.IsEmpty() == true)
	{
		for (const FBBMAIBeatSequence& Pattern : PatternPool)
		{
			if (Pattern.SelectionWeight > 0.0f)
			{
				ValidCandidates.Add(&Pattern);
				TotalWeight += Pattern.SelectionWeight;
			}
		}
	}

	// 3. 유효 가중치 패턴이 전혀 없으면 첫 번째 패턴을 안전 fallback으로 반환
	if (ValidCandidates.IsEmpty() == true || TotalWeight <= 0.0f)
	{
		return &PatternPool[0];
	}

	// 4. FRandomStream 기반 가중치 룰렛 휠 추첨
	const float RandomRoll = InRandomStream.FRandRange(0.0f, TotalWeight);
	float AccumulatedWeight = 0.0f;

	for (const FBBMAIBeatSequence* Candidate : ValidCandidates)
	{
		if (Candidate != nullptr)
		{
			AccumulatedWeight += Candidate->SelectionWeight;
			if (RandomRoll <= AccumulatedWeight)
			{
				return Candidate;
			}
		}
	}

	return ValidCandidates.Last();
}

#if WITH_EDITOR
EDataValidationResult UBBMAICombatantDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (PatternPool.IsEmpty() == true)
	{
		Context.AddWarning(LOCTEXT("NoPatternsWarning", "AI 전투원 패턴 풀이 비어있습니다. 최소 1개 이상의 8비트 패턴을 등록해야 합니다."));
		Result = EDataValidationResult::Invalid;
	}

	for (int32 Index = 0; Index < PatternPool.Num(); ++Index)
	{
		const FBBMAIBeatSequence& Pattern = PatternPool[Index];

		// 8비트 규격 검증 (GDD 3.3 규격)
		if (Pattern.Sequence.Beats.Num() != 8)
		{
			Context.AddError(FText::Format(
				LOCTEXT("InvalidBeatsNumError", "패턴 [{0}] (인덱스 {1})의 비트 수가 8개가 아닙니다 (현재: {2}개)."),
				FText::FromName(Pattern.PatternName),
				FText::AsNumber(Index),
				FText::AsNumber(Pattern.Sequence.Beats.Num())
			));
			Result = EDataValidationResult::Invalid;
		}

		// EffectRanges 유효 범위 검증 (0 <= Start < End <= 8)
		for (int32 RangeIndex = 0; RangeIndex < Pattern.Sequence.EffectRanges.Num(); ++RangeIndex)
		{
			const FBBMBeatEffectRange& Range = Pattern.Sequence.EffectRanges[RangeIndex];
			if (Range.IsValid() == false)
			{
				Context.AddError(FText::Format(
					LOCTEXT("InvalidEffectRangeError", "패턴 [{0}]의 EffectRange [{1}] (이름: {2}) 범위가 유효하지 않습니다 (Start: {3}, End: {4}). 0 <= Start < End <= 8 이어야 합니다."),
					FText::FromName(Pattern.PatternName),
					FText::AsNumber(RangeIndex),
					FText::FromName(Range.RangeName),
					FText::AsNumber(Range.StartBeat),
					FText::AsNumber(Range.EndBeat)
				));
				Result = EDataValidationResult::Invalid;
			}
		}

		// 가중치 음수 방어
		if (Pattern.SelectionWeight < 0.0f)
		{
			Context.AddError(FText::Format(
				LOCTEXT("NegativeWeightError", "패턴 [{0}]의 선택 가중치가 음수입니다 ({1}). 0 이상이어야 합니다."),
				FText::FromName(Pattern.PatternName),
				FText::AsNumber(Pattern.SelectionWeight)
			));
			Result = EDataValidationResult::Invalid;
		}
	}

	// 검증 에러가 없고 기존 검증 상태가 NotValidated인 경우 Valid로 명시 승격
	if (Context.GetNumErrors() == 0 && Result == EDataValidationResult::NotValidated)
	{
		Result = EDataValidationResult::Valid;
	}

	return Result;
}
#undef LOCTEXT_NAMESPACE
#endif

