#include "UI/ViewModel/BBMCreditEntryViewModel.h"

FText UBBMCreditEntryViewModel::GetCategoryName() const
{
	return CreditCategory.CategoryName;
}

FText UBBMCreditEntryViewModel::GetEntriesText() const
{
	if (CreditCategory.Entries.IsEmpty() == true)
	{
		return FText::GetEmpty();
	}

	// UMG TextBlock 다중 줄 바인딩을 위해 항목 배열을 개행 문자로 결합함
	return FText::Join(FText::FromString(TEXT("\n")), CreditCategory.Entries);
}

void UBBMCreditEntryViewModel::SetCreditCategory(const FBBMCreditCategory& InCreditCategory)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(CreditCategory, InCreditCategory) == false)
	{
		return;
	}

	// 구조체 내부 파생 텍스트 getter들도 함께 갱신 통지함
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCategoryName);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEntriesText);
}
