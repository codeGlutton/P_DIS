#include "UI/ViewModel/BBMCreditsViewModel.h"
#include "Global/BBMGlobalSettings.h"
#include "UI/ViewModel/BBMCreditEntryViewModel.h"

void UBBMCreditsViewModel::SetCreditCategories(const TArray<FBBMCreditCategory>& InCreditCategories)
{
	TArray<TObjectPtr<UBBMCreditEntryViewModel>> NewCreditEntries;
	NewCreditEntries.Reserve(InCreditCategories.Num());

	for (const FBBMCreditCategory& Category : InCreditCategories)
	{
		// 패널 ViewModel을 Outer로 삼아 생명 주기를 동일하게 유지함
		UBBMCreditEntryViewModel* EntryVM = NewObject<UBBMCreditEntryViewModel>(this);
		if (EntryVM != nullptr)
		{
			EntryVM->SetCreditCategory(Category);
			NewCreditEntries.Add(EntryVM);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[UBBMCreditsViewModel::SetCreditCategories] UBBMCreditEntryViewModel 생성 실패"));
		}
	}

	UE_MVVM_SET_PROPERTY_VALUE(CreditEntries, NewCreditEntries);
}

void UBBMCreditsViewModel::BeginViewModel()
{
	Super::BeginViewModel();

	const UBBMGlobalSettings* GlobalSettings = GetDefault<UBBMGlobalSettings>();
	if (GlobalSettings == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMCreditsViewModel::BeginViewModel] UBBMGlobalSettings CDO를 조회할 수 없음"));
		return;
	}

	SetCreditCategories(GlobalSettings->CreditCategories);
}

void UBBMCreditsViewModel::EndViewModel()
{
	CreditEntries.Reset();

	Super::EndViewModel();
}
