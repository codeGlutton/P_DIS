#include "UI/ViewModel/BBMComboBoxTextEntryViewModel.h"

void UBBMComboBoxTextEntryViewModel::SetEntryText(const FText& InEntryText)
{
	if (EntryText.EqualTo(InEntryText) == true)
	{
		return;
	}

	EntryText = InEntryText;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(EntryText);
}

void UBBMComboBoxTextEntryViewModel::SetEntryIndex(int32 InEntryIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(EntryIndex, InEntryIndex);
}
