#include "UI/ViewModel/BBMTabViewModelBase.h"
#include "UI/ViewModel/BBMTabEntryViewModel.h"

UBBMTabViewModelBase::UBBMTabViewModelBase() :
	SelectedTabIndex(INDEX_NONE),
	PreSelectedTabIndex(INDEX_NONE)
{
}

void UBBMTabViewModelBase::SetTabEntries(const TArray<UBBMTabEntryViewModel*>& InTabEntries)
{
	// 기존 엔트리 대리자 해제 및 새 목록에서 제외되는 선택 엔트리 정리
	UnbindTabEntries();

	TArray<TObjectPtr<UBBMTabEntryViewModel>> NewEntries;
	NewEntries.Reserve(InTabEntries.Num());

	for (UBBMTabEntryViewModel* Entry : InTabEntries)
	{
		if (Entry == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[UBBMTabViewModelBase::SetTabEntries] nullptr 엔트리 항목은 무시됨"));
			continue;
		}

		if (NewEntries.Contains(Entry) == true)
		{
			UE_LOG(LogTemp, Warning, TEXT("[UBBMTabViewModelBase::SetTabEntries] 중복된 엔트리 항목은 무시됨"));
			continue;
		}

		NewEntries.Add(Entry);
	}

	if (TabEntries.IsValidIndex(SelectedTabIndex) == true)
	{
		UBBMTabEntryViewModel* PrevSelectedEntry = TabEntries[SelectedTabIndex];
		if (PrevSelectedEntry != nullptr && NewEntries.Contains(PrevSelectedEntry) == false)
		{
			PrevSelectedEntry->SetIsSelected(false);
		}
	}

	const int32 OldSelectedIndex = SelectedTabIndex;
	TabEntries = MoveTemp(NewEntries);

	// 엔트리가 존재하면 항상 기본 0번 선택, 비어있으면 INDEX_NONE 및 PreSelectedTabIndex 초기화
	const int32 NextSelectedIndex = (TabEntries.IsEmpty() == false) ? 0 : INDEX_NONE;

	UpdateSelectedAndPreSelectedIndices(NextSelectedIndex, INDEX_NONE);
	SyncEntriesIndexAndSelection();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabEntries);
	RebindTabEntries();

	if (OldSelectedIndex != SelectedTabIndex)
	{
		OnSelectedTabChanged(OldSelectedIndex, SelectedTabIndex);
	}
}

void UBBMTabViewModelBase::SetSelectedTabIndex(int32 InTabIndex)
{
	SelectTab(InTabIndex);
}

UBBMTabEntryViewModel* UBBMTabViewModelBase::GetSelectedTabEntry() const
{
	return (TabEntries.IsValidIndex(SelectedTabIndex) == true) ? TabEntries[SelectedTabIndex].Get() : nullptr;
}

void UBBMTabViewModelBase::SelectTab(int32 InTabIndex)
{
	if (TabEntries.IsValidIndex(InTabIndex) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMTabViewModelBase::SelectTab] 유효하지 않은 탭 인덱스(%d / Count=%d)"), InTabIndex, TabEntries.Num());
		return;
	}

	// 현재 선택된 인덱스와 동일하면 무시함
	if (SelectedTabIndex == InTabIndex)
	{
		return;
	}

	const int32 OldSelectedIndex = SelectedTabIndex;
	UpdateSelectedAndPreSelectedIndices(InTabIndex, OldSelectedIndex);

	// 기존 선택 탭 해제 후 신규 선택 탭 활성화
	if (TabEntries.IsValidIndex(OldSelectedIndex) == true && TabEntries[OldSelectedIndex] != nullptr)
	{
		TabEntries[OldSelectedIndex]->SetIsSelected(false);
	}

	if (TabEntries[InTabIndex] != nullptr)
	{
		TabEntries[InTabIndex]->SetIsSelected(true);
	}

	OnSelectedTabChanged(OldSelectedIndex, InTabIndex);
}

int32 UBBMTabViewModelBase::AddTab(UBBMTabEntryViewModel* InEntry)
{
	if (InEntry == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMTabViewModelBase::AddTab] InEntry가 nullptr임"));
		return INDEX_NONE;
	}

	if (TabEntries.Contains(InEntry) == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMTabViewModelBase::AddTab] 이미 등록된 탭 엔트리임"));
		return TabEntries.IndexOfByKey(InEntry);
	}

	const int32 OldSelectedIndex = SelectedTabIndex;
	const int32 NewIndex = TabEntries.Add(InEntry);

	// 추가 전에 선행 인덱스 부여 후 직전 탭을 PreSelected로 보존하고 새 최상위 탭을 자동 선택함
	InEntry->SetTabIndex(NewIndex);
	UpdateSelectedAndPreSelectedIndices(NewIndex, OldSelectedIndex);

	if (TabEntries.IsValidIndex(OldSelectedIndex) == true && TabEntries[OldSelectedIndex] != nullptr)
	{
		TabEntries[OldSelectedIndex]->SetIsSelected(false);
	}

	InEntry->SetIsSelected(true);

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabEntries);
	RebindTabEntries();

	if (OldSelectedIndex != SelectedTabIndex)
	{
		OnSelectedTabChanged(OldSelectedIndex, SelectedTabIndex);
	}

	return NewIndex;
}

UBBMTabEntryViewModel* UBBMTabViewModelBase::AddTabWithText(const FText& InTabText)
{
	UBBMTabEntryViewModel* NewEntry = NewObject<UBBMTabEntryViewModel>(this);
	if (NewEntry == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMTabViewModelBase::AddTabWithText] UBBMTabEntryViewModel 생성 실패"));
		return nullptr;
	}

	NewEntry->SetTabText(InTabText);
	AddTab(NewEntry);
	return NewEntry;
}

bool UBBMTabViewModelBase::RemoveTabAt(int32 InTabIndex)
{
	if (TabEntries.IsValidIndex(InTabIndex) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMTabViewModelBase::RemoveTabAt] 유효하지 않은 제거 인덱스(%d / Count=%d)"), InTabIndex, TabEntries.Num());
		return false;
	}

	const int32 OldSelectedIndex = SelectedTabIndex;
	const int32 OldPreSelectedIndex = PreSelectedTabIndex;

	UBBMTabEntryViewModel* RemovedEntry = TabEntries[InTabIndex];
	UnbindTabEntries();

	if (RemovedEntry != nullptr && RemovedEntry->IsSelected() == true)
	{
		RemovedEntry->SetIsSelected(false);
	}

	TabEntries.RemoveAt(InTabIndex);

	int32 NextSelectedIndex = INDEX_NONE;
	int32 NextPreSelectedIndex = INDEX_NONE;

	if (TabEntries.IsEmpty() == false)
	{
		if (InTabIndex == OldSelectedIndex)
		{
			// 현재 선택된 탭 제거 시 직전 선택 인덱스(제거 보정 포함)로 복귀하되, 유효하지 않으면 인접 탭으로 폴백함
			const int32 ShiftedPreIndex = ShiftIndexAfterRemoval(OldPreSelectedIndex, InTabIndex);
			NextSelectedIndex = (TabEntries.IsValidIndex(ShiftedPreIndex) == true)
				? ShiftedPreIndex
				: FMath::Clamp(InTabIndex, 0, TabEntries.Num() - 1);
			NextPreSelectedIndex = INDEX_NONE;
		}
		else
		{
			NextSelectedIndex = ShiftIndexAfterRemoval(OldSelectedIndex, InTabIndex);
			const int32 ShiftedPreIndex = ShiftIndexAfterRemoval(OldPreSelectedIndex, InTabIndex);
			NextPreSelectedIndex = (ShiftedPreIndex != NextSelectedIndex) ? ShiftedPreIndex : INDEX_NONE;
		}
	}

	UpdateSelectedAndPreSelectedIndices(NextSelectedIndex, NextPreSelectedIndex);
	SyncEntriesIndexAndSelection();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabEntries);
	RebindTabEntries();

	if (OldSelectedIndex != SelectedTabIndex || InTabIndex == OldSelectedIndex)
	{
		OnSelectedTabChanged(OldSelectedIndex, SelectedTabIndex);
	}

	return true;
}

bool UBBMTabViewModelBase::RemoveTab(UBBMTabEntryViewModel* InEntry)
{
	if (InEntry == nullptr)
	{
		return false;
	}

	const int32 FoundIndex = TabEntries.IndexOfByKey(InEntry);
	if (FoundIndex == INDEX_NONE)
	{
		return false;
	}

	return RemoveTabAt(FoundIndex);
}

void UBBMTabViewModelBase::RemoveAllTabs()
{
	if (TabEntries.IsEmpty() == true && SelectedTabIndex == INDEX_NONE && PreSelectedTabIndex == INDEX_NONE)
	{
		return;
	}

	const int32 OldSelectedIndex = SelectedTabIndex;
	UnbindTabEntries();

	for (UBBMTabEntryViewModel* Entry : TabEntries)
	{
		if (Entry != nullptr && Entry->IsSelected() == true)
		{
			Entry->SetIsSelected(false);
		}
	}

	TabEntries.Reset();
	UpdateSelectedAndPreSelectedIndices(INDEX_NONE, INDEX_NONE);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabEntries);

	if (OldSelectedIndex != INDEX_NONE)
	{
		OnSelectedTabChanged(OldSelectedIndex, INDEX_NONE);
	}
}

bool UBBMTabViewModelBase::SwapTabs(int32 InIndexA, int32 InIndexB)
{
	if (TabEntries.IsValidIndex(InIndexA) == false || TabEntries.IsValidIndex(InIndexB) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMTabViewModelBase::SwapTabs] 유효하지 않은 스왑 인덱스(%d, %d / Count=%d)"), InIndexA, InIndexB, TabEntries.Num());
		return false;
	}

	if (InIndexA == InIndexB)
	{
		return true;
	}

	const int32 OldSelectedIndex = SelectedTabIndex;
	TabEntries.Swap(InIndexA, InIndexB);

	// 두 탭 위치 교환에 맞춰 SelectedTabIndex와 PreSelectedTabIndex도 해당 엔트리를 따라 동반 이동시킴
	const int32 NextSelectedIndex = RemapIndexAfterSwap(SelectedTabIndex, InIndexA, InIndexB);
	const int32 NextPreSelectedIndex = RemapIndexAfterSwap(PreSelectedTabIndex, InIndexA, InIndexB);
	UpdateSelectedAndPreSelectedIndices(NextSelectedIndex, NextPreSelectedIndex);

	SyncEntriesIndexAndSelection();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabEntries);
	RebindTabEntries();

	if (OldSelectedIndex != SelectedTabIndex)
	{
		OnSelectedTabChanged(OldSelectedIndex, SelectedTabIndex);
	}

	return true;
}

void UBBMTabViewModelBase::RefreshTabEntries()
{
	const int32 OldSelectedIndex = SelectedTabIndex;

	int32 NextSelectedIndex = SelectedTabIndex;
	int32 NextPreSelectedIndex = PreSelectedTabIndex;

	if (TabEntries.IsEmpty() == true)
	{
		NextSelectedIndex = INDEX_NONE;
		NextPreSelectedIndex = INDEX_NONE;
	}
	else
	{
		if (TabEntries.IsValidIndex(NextSelectedIndex) == false)
		{
			NextSelectedIndex = 0;
		}

		if (TabEntries.IsValidIndex(NextPreSelectedIndex) == false || NextPreSelectedIndex == NextSelectedIndex)
		{
			NextPreSelectedIndex = INDEX_NONE;
		}
	}

	UpdateSelectedAndPreSelectedIndices(NextSelectedIndex, NextPreSelectedIndex);
	SyncEntriesIndexAndSelection();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabEntries);
	RebindTabEntries();

	if (OldSelectedIndex != SelectedTabIndex)
	{
		OnSelectedTabChanged(OldSelectedIndex, SelectedTabIndex);
	}
}

void UBBMTabViewModelBase::EndViewModel()
{
	UnbindTabEntries();

	Super::EndViewModel();
}

void UBBMTabViewModelBase::OnSelectedTabChanged(int32 InOldTabIndex, int32 InNewTabIndex)
{
	OnTabSelectionChangedNative.Broadcast(InOldTabIndex, InNewTabIndex);
	OnTabSelectionChanged.Broadcast(InOldTabIndex, InNewTabIndex);
}

void UBBMTabViewModelBase::HandleEntrySelectRequested(int32 InTabIndex)
{
	SelectTab(InTabIndex);
}

void UBBMTabViewModelBase::UpdateSelectedAndPreSelectedIndices(int32 InNewSelectedIndex, int32 InNewPreSelectedIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(PreSelectedTabIndex, InNewPreSelectedIndex);
	UE_MVVM_SET_PROPERTY_VALUE(SelectedTabIndex, InNewSelectedIndex);
}

void UBBMTabViewModelBase::SyncEntriesIndexAndSelection()
{
	// OnSelected/OnDeselected 발송 시 최신 TabIndex를 전달할 수 있도록 인덱스를 먼저 전부 대입함
	for (int32 Index = 0; Index < TabEntries.Num(); ++Index)
	{
		UBBMTabEntryViewModel* Entry = TabEntries[Index];
		if (Entry != nullptr)
		{
			Entry->SetTabIndex(Index);
		}
	}

	// 선택 변경 시 기존 선택 해제 -> 신규 선택 순서가 보장되도록 비선택 항목을 먼저 해제하고 선택 항목을 활성화함
	for (int32 Index = 0; Index < TabEntries.Num(); ++Index)
	{
		UBBMTabEntryViewModel* Entry = TabEntries[Index];
		if (Entry != nullptr && Index != SelectedTabIndex)
		{
			Entry->SetIsSelected(false);
		}
	}

	if (TabEntries.IsValidIndex(SelectedTabIndex) == true && TabEntries[SelectedTabIndex] != nullptr)
	{
		TabEntries[SelectedTabIndex]->SetIsSelected(true);
	}
}

void UBBMTabViewModelBase::RebindTabEntries()
{
	UnbindTabEntries();

	EntryBindings.Reserve(TabEntries.Num());
	for (UBBMTabEntryViewModel* Entry : TabEntries)
	{
		if (Entry == nullptr)
		{
			continue;
		}

		FTabEntryBinding NewBinding;
		NewBinding.Entry = Entry;
		NewBinding.SelectRequestedHandle = Entry->OnSelectRequestedNative.AddUObject(
			this,
			&UBBMTabViewModelBase::HandleEntrySelectRequested
		);
		EntryBindings.Add(MoveTemp(NewBinding));
	}
}

void UBBMTabViewModelBase::UnbindTabEntries()
{
	for (FTabEntryBinding& Binding : EntryBindings)
	{
		if (Binding.Entry.IsValid() == false)
		{
			continue;
		}

		if (Binding.SelectRequestedHandle.IsValid() == true)
		{
			Binding.Entry->OnSelectRequestedNative.Remove(Binding.SelectRequestedHandle);
		}
	}

	EntryBindings.Reset();
}

int32 UBBMTabViewModelBase::ShiftIndexAfterRemoval(int32 InTargetIndex, int32 InRemovedIndex)
{
	if (InTargetIndex == INDEX_NONE || InTargetIndex == InRemovedIndex)
	{
		return INDEX_NONE;
	}

	return (InTargetIndex > InRemovedIndex) ? (InTargetIndex - 1) : InTargetIndex;
}

int32 UBBMTabViewModelBase::RemapIndexAfterSwap(int32 InTargetIndex, int32 InIndexA, int32 InIndexB)
{
	if (InTargetIndex == InIndexA)
	{
		return InIndexB;
	}

	if (InTargetIndex == InIndexB)
	{
		return InIndexA;
	}

	return InTargetIndex;
}
