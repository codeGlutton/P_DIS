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
	BaseMaxLevel(0),
	CachedCalculatedRawLevel(0.0f),
	CachedCalculatedMaxLevel(0.0f),
	CachedCalculatedValue(0.0f)
{
	ResetModifiers();
}

FDISActiveEnhancement::FDISActiveEnhancement(
	TSubclassOf<UDISEnhancement> InEnhancementClass,
	bool bInIsActive,
	int32 InBaseMaxLevel
) :
	EnhancementClass(InEnhancementClass),
	bIsActive(bInIsActive),
	bPendingRemove(false),
	BaseMaxLevel(InBaseMaxLevel),
	CachedCalculatedRawLevel(0.0f),
	CachedCalculatedMaxLevel(static_cast<float>(InBaseMaxLevel)),
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

void FDISActiveEnhancement::SetBaseMaxLevel(int32 InBaseMaxLevel)
{
	BaseMaxLevel = InBaseMaxLevel;
	UpdateCalculatedValue();
}

int32 FDISActiveEnhancement::GetBaseMaxLevel() const
{
	return BaseMaxLevel;
}

void FDISActiveEnhancement::ResetModifiers()
{
	const int32 OpCount = static_cast<int32>(EDISEnhancementLevelOp::Max);
	if (LevelModifierOpValues.Num() != OpCount)
	{
		LevelModifierOpValues.SetNum(OpCount);
	}
	if (MaxLevelModifierOpValues.Num() != OpCount)
	{
		MaxLevelModifierOpValues.SetNum(OpCount);
	}

	for (FDISActiveEnhancementModChannel& Channel : LevelModifierOpValues)
	{
		Channel.Empty();
	}
	for (FDISActiveEnhancementModChannel& Channel : MaxLevelModifierOpValues)
	{
		Channel.Empty();
	}

	UpdateCalculatedValue();
}

void FDISActiveEnhancement::AddModifier(EDISEnhancementModifierTarget InTarget, EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle, float InMagnitude)
{
	const int32 OpIndex = static_cast<int32>(InLevelOp);
	TArray<FDISActiveEnhancementModChannel>& TargetChannels = (InTarget == EDISEnhancementModifierTarget::CurrentLevel) ? LevelModifierOpValues : MaxLevelModifierOpValues;

	if (TargetChannels.IsValidIndex(OpIndex) == false)
	{
		return;
	}

	TargetChannels[OpIndex].AddEntry(InHandle, InMagnitude);
	UpdateCalculatedValue();
}

bool FDISActiveEnhancement::RemoveModifier(const FDISActiveEnhancementEffectHandle& InHandle)
{
	bool bRemovedAny = false;
	for (FDISActiveEnhancementModChannel& Channel : LevelModifierOpValues)
	{
		if (Channel.RemoveEntry(InHandle) == true)
		{
			bRemovedAny = true;
		}
	}
	for (FDISActiveEnhancementModChannel& Channel : MaxLevelModifierOpValues)
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

bool FDISActiveEnhancement::RemoveModifier(EDISEnhancementModifierTarget InTarget, EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle)
{
	const int32 OpIndex = static_cast<int32>(InLevelOp);
	TArray<FDISActiveEnhancementModChannel>& TargetChannels = (InTarget == EDISEnhancementModifierTarget::CurrentLevel) ? LevelModifierOpValues : MaxLevelModifierOpValues;

	if (TargetChannels.IsValidIndex(OpIndex) == true)
	{
		const bool bRemoved = TargetChannels[OpIndex].RemoveEntry(InHandle);
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
	const int32 OverrideIndex = static_cast<int32>(EDISEnhancementLevelOp::Override);
	const int32 AddIndex = static_cast<int32>(EDISEnhancementLevelOp::Add);
	const int32 MultiplyIndex = static_cast<int32>(EDISEnhancementLevelOp::Multiply);

	// 1. MaxLevel 계산
	if (MaxLevelModifierOpValues.IsValidIndex(OverrideIndex) == true && MaxLevelModifierOpValues[OverrideIndex].IsEmpty() == false)
	{
		const TArray<FDISActiveEnhancementModEntry>& OverrideEntries = MaxLevelModifierOpValues[OverrideIndex].GetEntries();
		CachedCalculatedMaxLevel = FMath::Max(0.0f, OverrideEntries.Last().Magnitude);
	}
	else
	{
		float MaxAddSum = static_cast<float>(BaseMaxLevel);
		if (MaxLevelModifierOpValues.IsValidIndex(AddIndex) == true)
		{
			for (const FDISActiveEnhancementModEntry& Entry : MaxLevelModifierOpValues[AddIndex].GetEntries())
			{
				MaxAddSum += Entry.Magnitude;
			}
		}

		float MaxMultiplier = 1.0f;
		if (MaxLevelModifierOpValues.IsValidIndex(MultiplyIndex) == true)
		{
			for (const FDISActiveEnhancementModEntry& Entry : MaxLevelModifierOpValues[MultiplyIndex].GetEntries())
			{
				MaxMultiplier *= Entry.Magnitude;
			}
		}

		CachedCalculatedMaxLevel = FMath::Max(0.0f, MaxAddSum * MaxMultiplier);
	}

	// 2. CurrentLevel 계산 (기본 0.0f)
	if (LevelModifierOpValues.IsValidIndex(OverrideIndex) == true && LevelModifierOpValues[OverrideIndex].IsEmpty() == false)
	{
		const TArray<FDISActiveEnhancementModEntry>& OverrideEntries = LevelModifierOpValues[OverrideIndex].GetEntries();
		CachedCalculatedRawLevel = OverrideEntries.Last().Magnitude;
	}
	else
	{
		float AddSum = 0.0f;
		if (LevelModifierOpValues.IsValidIndex(AddIndex) == true)
		{
			for (const FDISActiveEnhancementModEntry& Entry : LevelModifierOpValues[AddIndex].GetEntries())
			{
				AddSum += Entry.Magnitude;
			}
		}

		float Multiplier = 1.0f;
		if (LevelModifierOpValues.IsValidIndex(MultiplyIndex) == true)
		{
			for (const FDISActiveEnhancementModEntry& Entry : LevelModifierOpValues[MultiplyIndex].GetEntries())
			{
				Multiplier *= Entry.Magnitude;
			}
		}

		CachedCalculatedRawLevel = AddSum * Multiplier;
	}

	// 3. MaxLevel 상한 클램핑
	CachedCalculatedValue = FMath::Clamp(CachedCalculatedRawLevel, 0.0f, CachedCalculatedMaxLevel);
}

float FDISActiveEnhancement::GetCalculatedValue() const
{
	return CachedCalculatedValue;
}

float FDISActiveEnhancement::GetRawCalculatedValue() const
{
	return CachedCalculatedRawLevel;
}

float FDISActiveEnhancement::GetCalculatedMaxValue() const
{
	return CachedCalculatedMaxLevel;
}

int32 FDISActiveEnhancement::GetLevel() const
{
	return FMath::RoundToInt(CachedCalculatedValue);
}

int32 FDISActiveEnhancement::GetMaxLevel() const
{
	return FMath::RoundToInt(CachedCalculatedMaxLevel);
}

bool FDISActiveEnhancement::HasOverride(EDISEnhancementModifierTarget InTarget) const
{
	const int32 OverrideIndex = static_cast<int32>(EDISEnhancementLevelOp::Override);
	const TArray<FDISActiveEnhancementModChannel>& TargetChannels = (InTarget == EDISEnhancementModifierTarget::CurrentLevel) ? LevelModifierOpValues : MaxLevelModifierOpValues;

	return TargetChannels.IsValidIndex(OverrideIndex) == true && TargetChannels[OverrideIndex].IsEmpty() == false;
}

bool FDISActiveEnhancement::HasModifierOp(EDISEnhancementModifierTarget InTarget, EDISEnhancementLevelOp InOp) const
{
	const int32 OpIndex = static_cast<int32>(InOp);
	const TArray<FDISActiveEnhancementModChannel>& TargetChannels = (InTarget == EDISEnhancementModifierTarget::CurrentLevel) ? LevelModifierOpValues : MaxLevelModifierOpValues;

	return TargetChannels.IsValidIndex(OpIndex) == true && TargetChannels[OpIndex].IsEmpty() == false;
}

bool FDISActiveEnhancement::HasAnyModifiers() const
{
	for (const FDISActiveEnhancementModChannel& Channel : LevelModifierOpValues)
	{
		if (Channel.IsEmpty() == false)
		{
			return true;
		}
	}
	for (const FDISActiveEnhancementModChannel& Channel : MaxLevelModifierOpValues)
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
	for (const FDISActiveEnhancementModChannel& Channel : LevelModifierOpValues)
	{
		Total += Channel.Num();
	}
	for (const FDISActiveEnhancementModChannel& Channel : MaxLevelModifierOpValues)
	{
		Total += Channel.Num();
	}
	return Total;
}

const FDISActiveEnhancementModChannel* FDISActiveEnhancement::GetLevelModChannel(EDISEnhancementLevelOp InOp) const
{
	const int32 OpIndex = static_cast<int32>(InOp);
	if (LevelModifierOpValues.IsValidIndex(OpIndex) == true)
	{
		return &LevelModifierOpValues[OpIndex];
	}
	return nullptr;
}

const FDISActiveEnhancementModChannel* FDISActiveEnhancement::GetMaxLevelModChannel(EDISEnhancementLevelOp InOp) const
{
	const int32 OpIndex = static_cast<int32>(InOp);
	if (MaxLevelModifierOpValues.IsValidIndex(OpIndex) == true)
	{
		return &MaxLevelModifierOpValues[OpIndex];
	}
	return nullptr;
}

const TArray<FDISActiveEnhancementModChannel>& FDISActiveEnhancement::GetLevelModifierOpValues() const
{
	return LevelModifierOpValues;
}

const TArray<FDISActiveEnhancementModChannel>& FDISActiveEnhancement::GetMaxLevelModifierOpValues() const
{
	return MaxLevelModifierOpValues;
}

bool FDISActiveEnhancement::operator==(const FDISActiveEnhancement& Other) const
{
	return EnhancementClass == Other.EnhancementClass;
}
