#include "Item/BBMItemEntry.h"
#include "Item/BBMItemDefinition.h"

UBBMItemEntry::UBBMItemEntry() :
	DynamicBeats(),
	DynamicGrantedEffects()
{
}

void UBBMItemEntry::InitializeEntry_Implementation(const UDISItemDefinition* InItemDefinition, float InDurability)
{
	Super::InitializeEntry_Implementation(InItemDefinition, InDurability);

	const UBBMItemDefinition* BBMDef = Cast<UBBMItemDefinition>(InItemDefinition);
	if (BBMDef != nullptr)
	{
		DynamicBeats = BBMDef->Beats;
		DynamicGrantedEffects = BBMDef->DefaultGrantedEffects;
	}
}

const TArray<FBBMBeatDefinition>& UBBMItemEntry::GetDynamicBeats() const
{
	return DynamicBeats;
}

TArray<FBBMBeatDefinition>& UBBMItemEntry::GetMutableDynamicBeats()
{
	return DynamicBeats;
}

bool UBBMItemEntry::AddEffectToBeat(int32 BeatIndex, const TInstancedStruct<FBBMEffectBase>& InEffect)
{
	if (DynamicBeats.IsValidIndex(BeatIndex) == false || InEffect.IsValid() == false)
	{
		return false;
	}

	DynamicBeats[BeatIndex].Effects.Add(InEffect);
	return true;
}

bool UBBMItemEntry::K2_AddEffectToBeat(int32 BeatIndex, const FInstancedStruct& InEffect)
{
	if (DynamicBeats.IsValidIndex(BeatIndex) == false || InEffect.IsValid() == false)
	{
		return false;
	}

	const UScriptStruct* ScriptStruct = InEffect.GetScriptStruct();
	if (ScriptStruct == nullptr || ScriptStruct->IsChildOf(FBBMEffectBase::StaticStruct()) == false)
	{
		return false;
	}

	TInstancedStruct<FBBMEffectBase> TypedEffect;
	TypedEffect.InitializeAsScriptStruct(ScriptStruct, InEffect.GetMemory());
	DynamicBeats[BeatIndex].Effects.Add(TypedEffect);
	return true;
}

void UBBMItemEntry::AddDynamicBeat(const FBBMBeatDefinition& InBeat)
{
	DynamicBeats.Add(InBeat);
}

const TArray<FBBMItemGameplayEffectConfig>& UBBMItemEntry::GetDynamicGrantedEffects() const
{
	return DynamicGrantedEffects;
}

TArray<FBBMItemGameplayEffectConfig>& UBBMItemEntry::GetMutableDynamicGrantedEffects()
{
	return DynamicGrantedEffects;
}

void UBBMItemEntry::AddDynamicGrantedEffect(const FBBMItemGameplayEffectConfig& InConfig)
{
	DynamicGrantedEffects.Add(InConfig);
}

bool UBBMItemEntry::SetByCallerMagnitude(const FGameplayTag& InTag, float InValue)
{
	if (InTag.IsValid() == false)
	{
		return false;
	}

	bool bFound = false;
	for (FBBMItemGameplayEffectConfig& Config : DynamicGrantedEffects)
	{
		if (Config.SetByCallerMagnitudes.Contains(InTag) == true)
		{
			Config.SetByCallerMagnitudes[InTag] = InValue;
			bFound = true;
		}
	}

	// 기존 효과들에 태그가 없고 효과 목록이 유효한 경우 첫 번째 효과에 등록
	if (bFound == false && DynamicGrantedEffects.IsEmpty() == false)
	{
		DynamicGrantedEffects[0].SetByCallerMagnitudes.Add(InTag, InValue);
		bFound = true;
	}

	return bFound;
}

float UBBMItemEntry::GetSetByCallerMagnitude(const FGameplayTag& InTag, float DefaultValue) const
{
	if (InTag.IsValid() == false)
	{
		return DefaultValue;
	}

	for (const FBBMItemGameplayEffectConfig& Config : DynamicGrantedEffects)
	{
		if (const float* FoundValue = Config.SetByCallerMagnitudes.Find(InTag))
		{
			return *FoundValue;
		}
	}

	return DefaultValue;
}
