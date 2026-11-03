/*****************************************************************//**
 * @file   BBMAICombatantBase.cpp
 * @brief  BBM(비트 배낭 상인) AI 전투원 베이스 폰 클래스 구현
 * @date   2026-11-03
 *********************************************************************/

#include "Combat/Pawn/BBMAICombatantBase.h"
#include "Combat/Pawn/BBMAICombatantDefinition.h"

ABBMAICombatantBase::ABBMAICombatantBase() :
	SelectedPattern(nullptr),
	LastPatternName(NAME_None)
{
	// AI 전투원 기본 팀 설정 (정적 정의 에셋 할당 시 해당 팀으로 자동 동기화됨)
	SetGenericTeamId(FGenericTeamId(static_cast<uint8>(EBBMTeamType::Monster)));
}

void ABBMAICombatantBase::InitializeFromDefinition()
{
	Super::InitializeFromDefinition();
}

void ABBMAICombatantBase::UpdateBeatSequence()
{
	SelectNewPatternRandom();
}

FBBMBeatSequence ABBMAICombatantBase::GetCurrentBeatSequence() const
{
	return CachedBeatSequence;
}

const FBBMAIBeatSequence* ABBMAICombatantBase::SelectNewPattern(FRandomStream& InRandomStream)
{
	const UBBMAICombatantDefinition* AIDef = Cast<UBBMAICombatantDefinition>(GetCharacterDefinition());
	if (AIDef == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ABBMAICombatantBase::SelectNewPattern] CharacterDefinition is not UBBMAICombatantDefinition."));
		SelectedPattern = nullptr;
		CachedBeatSequence = FBBMBeatSequence();
		return nullptr;
	}

	const FBBMAIBeatSequence* NewPattern = AIDef->SelectPattern(InRandomStream, LastPatternName);
	if (NewPattern != nullptr)
	{
		SelectedPattern = NewPattern;
		LastPatternName = NewPattern->PatternName;
		CachedBeatSequence = NewPattern->Sequence;
	}
	else
	{
		SelectedPattern = nullptr;
		CachedBeatSequence = FBBMBeatSequence();
	}

	return SelectedPattern;
}

bool ABBMAICombatantBase::SelectNewPatternRandom()
{
	FRandomStream RandomStream(FMath::Rand());
	return SelectNewPattern(RandomStream) != nullptr;
}

const FBBMAIBeatSequence* ABBMAICombatantBase::GetSelectedPattern() const
{
	return SelectedPattern;
}

FName ABBMAICombatantBase::GetSelectedPatternName() const
{
	if (SelectedPattern != nullptr)
	{
		return SelectedPattern->PatternName;
	}
	return NAME_None;
}
