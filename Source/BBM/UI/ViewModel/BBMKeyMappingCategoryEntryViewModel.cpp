#include "UI/ViewModel/BBMKeyMappingCategoryEntryViewModel.h"
#include "UI/ViewModel/BBMKeyMappingEntryViewModel.h"

void UBBMKeyMappingCategoryEntryViewModel::BeginDestroy()
{
	RemoveAllKeyMappingEntries();

	Super::BeginDestroy();
}

void UBBMKeyMappingCategoryEntryViewModel::SetKeyMappings(const FBBMKeyCategoryMappingData& InCategoryMappingData)
{
	SetCategoryName(InCategoryMappingData.CategoryName);

	const int32 NumKeys = InCategoryMappingData.KeyMappings.Num();
	const int32 NumKeyEntries = KeyMappingEntries.Num();
	if (NumKeys != NumKeyEntries)
	{
		RemoveAllKeyMappingEntries();

		TArray<TObjectPtr<UBBMKeyMappingEntryViewModel>> InitialKeyMappingEntries;
		InitialKeyMappingEntries.Reserve(NumKeys);

		for (int32 EntryIndex = 0; EntryIndex < NumKeys; ++EntryIndex)
		{
			UBBMKeyMappingEntryViewModel* EntryVM = NewObject<UBBMKeyMappingEntryViewModel>(this);
			if (EntryVM != nullptr)
			{
				EntryVM->SetEntryIndex(EntryIndex);
				EntryVM->SetKeyMapping(InCategoryMappingData.KeyMappings[EntryIndex]);
				InitialKeyMappingEntries.Add(EntryVM);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[UBBMKeyMappingCategoryEntryViewModel::SetKeyMappings] UBBMKeyMappingEntryViewModel 생성 실패 (EntryIndex=%d)"), EntryIndex);
			}
		}

		SetKeyMappingEntries(InitialKeyMappingEntries);
	}
	else
	{
		/* 크기 동일 시 VM 갱신 */

		for (int32 EntryIndex = 0; EntryIndex < NumKeys; ++EntryIndex)
		{
			if (KeyMappingEntries.IsValidIndex(EntryIndex) == true && KeyMappingEntries[EntryIndex] != nullptr)
			{
				KeyMappingEntries[EntryIndex]->SetEntryIndex(EntryIndex);
				KeyMappingEntries[EntryIndex]->SetKeyMapping(InCategoryMappingData.KeyMappings[EntryIndex]);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[UBBMKeyMappingCategoryEntryViewModel::SetKeyMappings] UBBMKeyMappingEntryViewModel 접근 실패 (EntryIndex=%d)"), EntryIndex);
			}
		}
	}
}

void UBBMKeyMappingCategoryEntryViewModel::RemapKey(int32 InEntryIndex, const FInputChord& InNewChord)
{
	if (KeyMappingEntries.IsValidIndex(InEntryIndex) == true && KeyMappingEntries[InEntryIndex] != nullptr)
	{
		KeyMappingEntries[InEntryIndex]->SetInputChord(InNewChord);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMKeyMappingCategoryEntryViewModel::RemapKey] UBBMKeyMappingEntryViewModel 접근 실패 (EntryIndex=%d)"), InEntryIndex);
	}
}

void UBBMKeyMappingCategoryEntryViewModel::SetCategoryIndex(int32 InCategoryIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(CategoryIndex, InCategoryIndex);
}

void UBBMKeyMappingCategoryEntryViewModel::SetCategoryName(const FText& InCategoryName)
{
	UE_MVVM_SET_PROPERTY_VALUE(CategoryName, InCategoryName);
}

void UBBMKeyMappingCategoryEntryViewModel::SetKeyMappingEntries(const TArray<TObjectPtr<UBBMKeyMappingEntryViewModel>>& InKeyMappingEntries)
{
	UnbindKeyMappingEntries();

	UE_MVVM_SET_PROPERTY_VALUE(KeyMappingEntries, InKeyMappingEntries);

	RebindKeyMappingEntries();
}

void UBBMKeyMappingCategoryEntryViewModel::RemoveAllKeyMappingEntries()
{
	if (KeyMappingEntries.IsEmpty() == true)
	{
		return;
	}

	UnbindKeyMappingEntries();
	KeyMappingEntries.Reset();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(KeyMappingEntries);
}

void UBBMKeyMappingCategoryEntryViewModel::RebindKeyMappingEntries()
{
	UnbindKeyMappingEntries();

	KeyMappingEntryBindings.Reserve(KeyMappingEntries.Num());
	for (const TObjectPtr<UBBMKeyMappingEntryViewModel>& Entry : KeyMappingEntries)
	{
		if (Entry == nullptr)
		{
			continue;
		}

		FKeyMappingEntryBinding Binding;
		Binding.Entry = Entry;
		Binding.ChordChangedHandle = Entry->OnInputChordChangedNative.AddUObject(
			this,
			&UBBMKeyMappingCategoryEntryViewModel::HandleEntryInputChordChanged
		);
		KeyMappingEntryBindings.Add(MoveTemp(Binding));
	}
}

void UBBMKeyMappingCategoryEntryViewModel::UnbindKeyMappingEntries()
{
	for (const FKeyMappingEntryBinding& Binding : KeyMappingEntryBindings)
	{
		if (Binding.Entry.IsValid() == false)
		{
			continue;
		}

		if (Binding.ChordChangedHandle.IsValid() == true)
		{
			Binding.Entry->OnInputChordChangedNative.Remove(Binding.ChordChangedHandle);
		}
	}

	KeyMappingEntryBindings.Reset();
}

void UBBMKeyMappingCategoryEntryViewModel::HandleEntryInputChordChanged(int32 InEntryIndex, FName InMappingName, const FInputChord& InNewChord)
{
	OnCategoryInputChordChangedNative.Broadcast(CategoryIndex, InEntryIndex, InMappingName, InNewChord);
	OnCategoryInputChordChanged.Broadcast(CategoryIndex, InEntryIndex, InMappingName, InNewChord);
}
