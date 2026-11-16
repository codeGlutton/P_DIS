#include "UI/ViewModel/BBMTabEntryViewModel.h"

void UBBMTabEntryViewModel::SetTabText(const FText& InTabText)
{
	UE_MVVM_SET_PROPERTY_VALUE(TabText, InTabText);
}

void UBBMTabEntryViewModel::SetIsSelected(bool bInIsSelected)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bIsSelected, bInIsSelected) == false)
	{
		return;
	}

	// 선택 여부가 실제로 변경된 경우에만 현재 TabIndex와 함께 선택/해제 대리자를 호출함
	if (bIsSelected == true)
	{
		OnSelectedNative.Broadcast(TabIndex);
		OnSelected.Broadcast(TabIndex);
	}
	else
	{
		OnDeselectedNative.Broadcast(TabIndex);
		OnDeselected.Broadcast(TabIndex);
	}
}

void UBBMTabEntryViewModel::SetTabIndex(int32 InTabIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(TabIndex, InTabIndex);
}

void UBBMTabEntryViewModel::RequestSelect()
{
	OnSelectRequestedNative.Broadcast(TabIndex);
	OnSelectRequested.Broadcast(TabIndex);
}
