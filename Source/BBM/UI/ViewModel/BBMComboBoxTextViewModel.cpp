#include "UI/ViewModel/BBMComboBoxTextViewModel.h"

#include "UI/ViewModel/BBMComboBoxTextEntryViewModel.h"

void UBBMComboBoxTextViewModel::SetSelectedKey(FName InSelectedKey)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(SelectedKey, InSelectedKey) == false)
	{
		return;
	}

	OnSelectedKeyChangedNative.Broadcast(SelectedKey);
	OnSelectedKeyChanged.Broadcast(SelectedKey);
}

UBBMComboBoxTextEntryViewModel* UBBMComboBoxTextViewModel::AddComboBoxTextEntry(const FName& InKey, const FText& InDisplayText)
{
	if (InKey.IsNone() == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMComboBoxTextViewModel::AddComboBoxTextEntry] NAME_None 키는 추가할 수 없음"));
		return nullptr;
	}

	for (const FRichComboBoxKeyViewModelItem& ExistingItem : ComboBoxItems)
	{
		if (ExistingItem.Key == InKey)
		{
			UE_LOG(LogTemp, Warning, TEXT("[UBBMComboBoxTextViewModel::AddComboBoxTextEntry] 중복 키(%s) 감지 - 기존 엔트리 텍스트만 갱신함"), *InKey.ToString());
			UBBMComboBoxTextEntryViewModel* ExistingEntryVM = Cast<UBBMComboBoxTextEntryViewModel>(ExistingItem.ViewModel.Get());
			if (ExistingEntryVM != nullptr)
			{
				ExistingEntryVM->SetEntryText(InDisplayText);
			}
			return ExistingEntryVM;
		}
	}

	UBBMComboBoxTextEntryViewModel* EntryVM = NewObject<UBBMComboBoxTextEntryViewModel>(this);
	if (EntryVM == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMComboBoxTextViewModel::AddComboBoxTextEntry] UBBMComboBoxTextEntryViewModel 생성 실패 (Key=%s)"), *InKey.ToString());
		return nullptr;
	}

	const int32 NewIndex = ComboBoxItems.Num();
	EntryVM->SetEntryText(InDisplayText);
	EntryVM->SetEntryIndex(NewIndex);

	FRichComboBoxKeyViewModelItem NewItem;
	NewItem.Key = InKey;
	NewItem.ViewModel = EntryVM;
	ComboBoxItems.Add(MoveTemp(NewItem));

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ComboBoxItems);
	return EntryVM;
}

void UBBMComboBoxTextViewModel::RemoveAllComboBoxTextEntries()
{
	if (ComboBoxItems.IsEmpty() == false)
	{
		ComboBoxItems.Reset();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ComboBoxItems);
	}

	SetSelectedKey(NAME_None);
}
