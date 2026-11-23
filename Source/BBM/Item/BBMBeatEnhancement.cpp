#include "Item/BBMBeatEnhancement.h"
#include "Item/BBMItemEntry.h"

UBBMBeatEnhancement::UBBMBeatEnhancement() :
	ExecutionPriority(0)
{
}

void UBBMBeatEnhancement::OnEnhancementActivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const
{
	Super::OnEnhancementActivated_Implementation(Target, ActiveEnhancement);

	if (UBBMItemEntry* ItemEntry = Cast<UBBMItemEntry>(Target))
	{
		ItemEntry->RebuildDynamicBeatSequence();
	}
}

void UBBMBeatEnhancement::OnEnhancementDeactivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const
{
	Super::OnEnhancementDeactivated_Implementation(Target, ActiveEnhancement);

	if (UBBMItemEntry* ItemEntry = Cast<UBBMItemEntry>(Target))
	{
		ItemEntry->RebuildDynamicBeatSequence();
	}
}

void UBBMBeatEnhancement::OnEnhancementLevelChanged_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement, int32 OldLevel, int32 NewLevel) const
{
	Super::OnEnhancementLevelChanged_Implementation(Target, ActiveEnhancement, OldLevel, NewLevel);

	if (OldLevel == NewLevel)
	{
		return;
	}

	if (UBBMItemEntry* ItemEntry = Cast<UBBMItemEntry>(Target))
	{
		ItemEntry->RebuildDynamicBeatSequence();
	}
}

void UBBMBeatEnhancement::ApplyBeatModification(int32 InLevel, const FBBMBeatSequence& BaseSequence, FBBMBeatSequence& InOutDynamicSequence) const
{
	// 기본 구현은 no-op (파생 클래스에서 오버라이드)
}

