#include "Enhancement/DISActiveEnhancementEffect.h"
#include "Enhancement/DISEnhancementEffect.h"
#include "Enhancement/DISEnhancement.h"

FDISActiveEnhancementEffectHandle::FDISActiveEnhancementEffectHandle() :
	HandleId(INDEX_NONE)
{
}

FDISActiveEnhancementEffectHandle::FDISActiveEnhancementEffectHandle(int32 InHandleId) :
	HandleId(InHandleId)
{
}

bool FDISActiveEnhancementEffectHandle::IsValid() const
{
	return HandleId != INDEX_NONE;
}

int32 FDISActiveEnhancementEffectHandle::GetHandleId() const
{
	return HandleId;
}

bool FDISActiveEnhancementEffectHandle::operator==(const FDISActiveEnhancementEffectHandle& Other) const
{
	return HandleId == Other.HandleId;
}

bool FDISActiveEnhancementEffectHandle::operator!=(const FDISActiveEnhancementEffectHandle& Other) const
{
	return HandleId != Other.HandleId;
}

FString FDISActiveEnhancementEffectHandle::ToString() const
{
	return FString::Printf(TEXT("DISActiveEffectHandle_%d"), HandleId);
}

FDISActiveEnhancementEffect::FDISActiveEnhancementEffect() :
	Spec(FDISEnhancementEffectSpec()),
	Handle(FDISActiveEnhancementEffectHandle()),
	bIsActive(false),
	bPendingRemove(false)
{
}

FDISActiveEnhancementEffect::FDISActiveEnhancementEffect(
	const FDISEnhancementEffectSpec& InSpec,
	const FDISActiveEnhancementEffectHandle& InHandle
) :
	Spec(InSpec),
	Handle(InHandle),
	bIsActive(true),
	bPendingRemove(false)
{
	RecalculateModifiers();
}

bool FDISActiveEnhancementEffect::IsValid() const
{
	return Spec.IsValid() && Handle.IsValid();
}

const UDISEnhancementEffect* FDISActiveEnhancementEffect::GetEffectCDO() const
{
	if (Spec.EffectClass != nullptr)
	{
		return GetDefault<UDISEnhancementEffect>(Spec.EffectClass);
	}
	return nullptr;
}

TSubclassOf<UDISEnhancementEffect> FDISActiveEnhancementEffect::GetEffectClass() const
{
	return Spec.EffectClass;
}

TArray<TSubclassOf<UDISEnhancement>> FDISActiveEnhancementEffect::GetTargetEnhancementClasses() const
{
	TArray<TSubclassOf<UDISEnhancement>> TargetClasses;
	CalculatedModifiers.GetKeys(TargetClasses);
	return TargetClasses;
}

const FDISEnhancementModifier* FDISActiveEnhancementEffect::FindCalculatedModifier(TSubclassOf<UDISEnhancement> InEnhancementClass) const
{
	if (InEnhancementClass == nullptr)
	{
		return nullptr;
	}

	return CalculatedModifiers.Find(InEnhancementClass);
}

const TMap<TSubclassOf<UDISEnhancement>, FDISEnhancementModifier>& FDISActiveEnhancementEffect::GetCalculatedModifiers() const
{
	return CalculatedModifiers;
}

void FDISActiveEnhancementEffect::RecalculateModifiers()
{
	CalculatedModifiers.Empty();

	const UDISEnhancementEffect* CDO = GetEffectCDO();
	if (CDO == nullptr)
	{
		return;
	}

	for (const TPair<TSubclassOf<UDISEnhancement>, FDISEnhancementModifier>& Pair : CDO->GetModifiers())
	{
		if (Pair.Key != nullptr)
		{
			FDISEnhancementModifier CalculatedMod = Pair.Value;
			// EffectClass CDO의 LevelMagnitude에 Spec.OverallMagnitude를 곱하여 산출
			CalculatedMod.LevelMagnitude = Pair.Value.LevelMagnitude * Spec.OverallMagnitude;
			CalculatedModifiers.Add(Pair.Key, CalculatedMod);
		}
	}
}

const FDISActiveEnhancementEffectHandle& FDISActiveEnhancementEffect::GetHandle() const
{
	return Handle;
}

const FDISEnhancementEffectSpec& FDISActiveEnhancementEffect::GetSpec() const
{
	return Spec;
}

bool FDISActiveEnhancementEffect::operator==(const FDISActiveEnhancementEffect& Other) const
{
	return Handle == Other.Handle;
}
