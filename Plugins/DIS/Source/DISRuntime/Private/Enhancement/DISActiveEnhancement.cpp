#include "Enhancement/DISActiveEnhancement.h"
#include "Enhancement/DISEnhancement.h"

/* FDISActiveEnhancementModEntry */

FDISActiveEnhancementModEntry::FDISActiveEnhancementModEntry() :
	Handle(FDISActiveEnhancementEffectHandle()),
	Magnitude(0.0f)
{
}

FDISActiveEnhancementModEntry::FDISActiveEnhancementModEntry(
	const FDISActiveEnhancementEffectHandle& InHandle,
	float InMagnitude
) :
	Handle(InHandle),
	Magnitude(InMagnitude)
{
}

bool FDISActiveEnhancementModEntry::operator==(const FDISActiveEnhancementModEntry& Other) const
{
	return Handle == Other.Handle;
}

bool FDISActiveEnhancementModEntry::operator==(const FDISActiveEnhancementEffectHandle& InHandle) const
{
	return Handle == InHandle;
}

/* FDISActiveEnhancementModChannel */

FDISActiveEnhancementModChannel::FDISActiveEnhancementModChannel()
{
}

void FDISActiveEnhancementModChannel::AddEntry(const FDISActiveEnhancementEffectHandle& InHandle, float InMagnitude)
{
	for (FDISActiveEnhancementModEntry& Existing : Entries)
	{
		if (Existing.Handle == InHandle)
		{
			Existing.Magnitude = InMagnitude;
			return;
		}
	}

	Entries.Add(FDISActiveEnhancementModEntry(InHandle, InMagnitude));
}

bool FDISActiveEnhancementModChannel::RemoveEntry(const FDISActiveEnhancementEffectHandle& InHandle)
{
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].Handle == InHandle)
		{
			Entries.RemoveAtSwap(Index);
			return true;
		}
	}
	return false;
}

const FDISActiveEnhancementModEntry* FDISActiveEnhancementModChannel::FindEntry(const FDISActiveEnhancementEffectHandle& InHandle) const
{
	for (const FDISActiveEnhancementModEntry& Existing : Entries)
	{
		if (Existing.Handle == InHandle)
		{
			return &Existing;
		}
	}
	return nullptr;
}

void FDISActiveEnhancementModChannel::Empty()
{
	Entries.Empty();
}

int32 FDISActiveEnhancementModChannel::Num() const
{
	return Entries.Num();
}

bool FDISActiveEnhancementModChannel::IsEmpty() const
{
	return Entries.IsEmpty();
}

const TArray<FDISActiveEnhancementModEntry>& FDISActiveEnhancementModChannel::GetEntries() const
{
	return Entries;
}

/* FDISActiveEnhancement */

FDISActiveEnhancement::FDISActiveEnhancement() :
	EnhancementClass(nullptr),
	bIsActive(false),
	bPendingRemove(false),
	CachedCalculatedValue(0.0f)
{
	ResetModifiers();
}

FDISActiveEnhancement::FDISActiveEnhancement(
	TSubclassOf<UDISEnhancement> InEnhancementClass,
	bool bInIsActive
) :
	EnhancementClass(InEnhancementClass),
	bIsActive(bInIsActive),
	bPendingRemove(false),
	CachedCalculatedValue(0.0f)
{
	ResetModifiers();
}

bool FDISActiveEnhancement::IsValid() const
{
	return EnhancementClass != nullptr;
}

const UDISEnhancement* FDISActiveEnhancement::GetEnhancementCDO() const
{
	if (EnhancementClass != nullptr)
	{
		return GetDefault<UDISEnhancement>(EnhancementClass);
	}
	return nullptr;
}

TSubclassOf<UDISEnhancement> FDISActiveEnhancement::GetEnhancementClass() const
{
	return EnhancementClass;
}

void FDISActiveEnhancement::ResetModifiers()
{
	const int32 OpCount = static_cast<int32>(EDISEnhancementLevelOp::Max);
	if (ModifierOpValues.Num() != OpCount)
	{
		ModifierOpValues.SetNum(OpCount);
	}

	for (FDISActiveEnhancementModChannel& Channel : ModifierOpValues)
	{
		Channel.Empty();
	}

	UpdateCalculatedValue();
}

void FDISActiveEnhancement::AddModifier(EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle, float InMagnitude)
{
	const int32 OpIndex = static_cast<int32>(InLevelOp);
	if (ModifierOpValues.IsValidIndex(OpIndex) == false)
	{
		return;
	}

	ModifierOpValues[OpIndex].AddEntry(InHandle, InMagnitude);
	UpdateCalculatedValue();
}

bool FDISActiveEnhancement::RemoveModifier(const FDISActiveEnhancementEffectHandle& InHandle)
{
	bool bRemovedAny = false;
	for (FDISActiveEnhancementModChannel& Channel : ModifierOpValues)
	{
		if (Channel.RemoveEntry(InHandle) == true)
		{
			bRemovedAny = true;
		}
	}

	if (bRemovedAny == true)
	{
		UpdateCalculatedValue();
	}

	return bRemovedAny;
}

bool FDISActiveEnhancement::RemoveModifier(EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle)
{
	const int32 OpIndex = static_cast<int32>(InLevelOp);
	if (ModifierOpValues.IsValidIndex(OpIndex) == true)
	{
		const bool bRemoved = ModifierOpValues[OpIndex].RemoveEntry(InHandle);
		if (bRemoved == true)
		{
			UpdateCalculatedValue();
		}
		return bRemoved;
	}
	return false;
}

void FDISActiveEnhancement::UpdateCalculatedValue()
{
	// 1. Override 채널 검사 (등록된 엔트리가 있으면 가장 최근 엔트리의 값 사용)
	const int32 OverrideIndex = static_cast<int32>(EDISEnhancementLevelOp::Override);
	if (ModifierOpValues.IsValidIndex(OverrideIndex) == true && ModifierOpValues[OverrideIndex].IsEmpty() == false)
	{
		const TArray<FDISActiveEnhancementModEntry>& OverrideEntries = ModifierOpValues[OverrideIndex].GetEntries();
		CachedCalculatedValue = OverrideEntries.Last().Magnitude;
		return;
	}

	// 2. Add 채널 수집 (합산)
	float AddSum = 0.0f;
	const int32 AddIndex = static_cast<int32>(EDISEnhancementLevelOp::Add);
	if (ModifierOpValues.IsValidIndex(AddIndex) == true)
	{
		for (const FDISActiveEnhancementModEntry& Entry : ModifierOpValues[AddIndex].GetEntries())
		{
			AddSum += Entry.Magnitude;
		}
	}

	// 3. Multiply 채널 수집 (승산, 기본 1.0f)
	float Multiplier = 1.0f;
	const int32 MultiplyIndex = static_cast<int32>(EDISEnhancementLevelOp::Multiply);
	if (ModifierOpValues.IsValidIndex(MultiplyIndex) == true)
	{
		for (const FDISActiveEnhancementModEntry& Entry : ModifierOpValues[MultiplyIndex].GetEntries())
		{
			Multiplier *= Entry.Magnitude;
		}
	}

	CachedCalculatedValue = AddSum * Multiplier;
}

float FDISActiveEnhancement::GetCalculatedValue() const
{
	return CachedCalculatedValue;
}

int32 FDISActiveEnhancement::GetLevel() const
{
	return FMath::Max(0, FMath::RoundToInt(CachedCalculatedValue));
}

bool FDISActiveEnhancement::HasOverride() const
{
	const int32 OverrideIndex = static_cast<int32>(EDISEnhancementLevelOp::Override);
	return ModifierOpValues.IsValidIndex(OverrideIndex) == true && ModifierOpValues[OverrideIndex].IsEmpty() == false;
}

bool FDISActiveEnhancement::HasModifierOp(EDISEnhancementLevelOp InOp) const
{
	const int32 OpIndex = static_cast<int32>(InOp);
	return ModifierOpValues.IsValidIndex(OpIndex) == true && ModifierOpValues[OpIndex].IsEmpty() == false;
}

bool FDISActiveEnhancement::HasAnyModifiers() const
{
	for (const FDISActiveEnhancementModChannel& Channel : ModifierOpValues)
	{
		if (Channel.IsEmpty() == false)
		{
			return true;
		}
	}
	return false;
}

int32 FDISActiveEnhancement::GetTotalModifierCount() const
{
	int32 Total = 0;
	for (const FDISActiveEnhancementModChannel& Channel : ModifierOpValues)
	{
		Total += Channel.Num();
	}
	return Total;
}

const FDISActiveEnhancementModChannel* FDISActiveEnhancement::GetModChannel(EDISEnhancementLevelOp InOp) const
{
	const int32 OpIndex = static_cast<int32>(InOp);
	if (ModifierOpValues.IsValidIndex(OpIndex) == true)
	{
		return &ModifierOpValues[OpIndex];
	}
	return nullptr;
}

const TArray<FDISActiveEnhancementModChannel>& FDISActiveEnhancement::GetModifierOpValues() const
{
	return ModifierOpValues;
}

bool FDISActiveEnhancement::operator==(const FDISActiveEnhancement& Other) const
{
	return EnhancementClass == Other.EnhancementClass;
}
