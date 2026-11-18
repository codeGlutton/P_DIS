#include "UI/ViewModel/BBMKeyMappingEntryViewModel.h"

void UBBMKeyMappingEntryViewModel::SetKeyMapping(const FBBMKeyMappingData& InKeyMappingData)
{
	SetMappingName(InKeyMappingData.MappingName);
	SetDisplayName(InKeyMappingData.DisplayName);
	SetInputChord(InKeyMappingData.InputChord);
}

void UBBMKeyMappingEntryViewModel::SetEntryIndex(int32 InEntryIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(EntryIndex, InEntryIndex);
}

void UBBMKeyMappingEntryViewModel::SetMappingName(FName InMappingName)
{
	UE_MVVM_SET_PROPERTY_VALUE(MappingName, InMappingName);
}

void UBBMKeyMappingEntryViewModel::SetDisplayName(const FText& InDisplayName)
{
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InDisplayName);
}

void UBBMKeyMappingEntryViewModel::SetInputChord(const FInputChord& InInputChord)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(InputChord, InInputChord) == false)
	{
		return;
	}

	const FInputChord TempInputChord = InInputChord;
	RequestToChangeInputChord(TempInputChord);
}

void UBBMKeyMappingEntryViewModel::RequestToChangeInputChord(const FInputChord& InInputChord)
{
	OnInputChordChangedNative.Broadcast(EntryIndex, MappingName, InInputChord);
	OnInputChordChanged.Broadcast(EntryIndex, MappingName, InInputChord);
}
