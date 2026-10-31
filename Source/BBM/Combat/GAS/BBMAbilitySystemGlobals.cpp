#include "Combat/GAS/BBMAbilitySystemGlobals.h"
#include "Engine/CurveTable.h"

UBBMAbilitySystemGlobals::UBBMAbilitySystemGlobals()
{
}

void UBBMAbilitySystemGlobals::InitGlobalData()
{
	Super::InitGlobalData();

	// GlobalAttributeDefaultsTables가 비어있어도 Initter가 상시 할당되어 있도록 안전 보장
	if (GlobalAttributeSetInitter.IsValid() == false)
	{
		AllocAttributeSetInitter();
	}
}

void UBBMAbilitySystemGlobals::AddAttributeDefaultTableInstance(UCurveTable* InCurveTable)
{
	if (InCurveTable == nullptr)
	{
		return;
	}

	GlobalAttributeDefaultsTables.AddUnique(InCurveTable);
	ReloadAttributeDefaults();
}

bool UBBMAbilitySystemGlobals::HasAttributeSetInitter() const
{
	return GlobalAttributeSetInitter.IsValid();
}

