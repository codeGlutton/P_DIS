#include "Customizations/RichComboBoxKeyCustomizationExtender.h"

#include "Blueprint/UserWidget.h"
#include "Components/RichComboBoxKey.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IPropertyUtilities.h"
#include "MVVMBlueprintView.h"
#include "MVVMBlueprintViewModelContext.h"
#include "MVVMWidgetBlueprintExtension_View.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditor.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "RichComboBoxKeyCustomizationExtender"

TSharedPtr<FRichComboBoxKeyCustomizationExtender> FRichComboBoxKeyCustomizationExtender::MakeInstance()
{
	return MakeShared<FRichComboBoxKeyCustomizationExtender>();
}

void FRichComboBoxKeyCustomizationExtender::CustomizeDetails(
	IDetailLayoutBuilder& InDetailLayout,
	const TArrayView<UWidget*> InWidgets,
	const TSharedRef<FWidgetBlueprintEditor>& InWidgetBlueprintEditor)
{
	if (InWidgets.Num() != 1)
	{
		return;
	}

	URichComboBoxKey* ComboBox = Cast<URichComboBoxKey>(InWidgets[0]);
	if (ComboBox == nullptr)
	{
		return;
	}

	const TSharedRef<IPropertyHandle> UseExtensionHandle = InDetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(URichComboBoxKey, bUseViewModelExtension), URichComboBoxKey::StaticClass());
	const TSharedRef<IPropertyHandle> ContentWidgetClassHandle = InDetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(URichComboBoxKey, ContentWidgetClass), URichComboBoxKey::StaticClass());
	const TSharedRef<IPropertyHandle> ContentVMNameHandle = InDetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(URichComboBoxKey, ContentViewModelName), URichComboBoxKey::StaticClass());
	const TSharedRef<IPropertyHandle> ContentVMClassHandle = InDetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(URichComboBoxKey, ContentViewModelClass), URichComboBoxKey::StaticClass());
	const TSharedRef<IPropertyHandle> ItemWidgetClassHandle = InDetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(URichComboBoxKey, ItemWidgetClass), URichComboBoxKey::StaticClass());
	const TSharedRef<IPropertyHandle> ItemVMNameHandle = InDetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(URichComboBoxKey, ItemViewModelName), URichComboBoxKey::StaticClass());
	const TSharedRef<IPropertyHandle> ItemVMClassHandle = InDetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(URichComboBoxKey, ItemViewModelClass), URichComboBoxKey::StaticClass());

	// 토글 bool, 내부 VM 이름 및 C++ 호환용 VM Class 프로퍼티는 WBP 디테일 패널 혼란 방지를 위해 항상 숨김
	InDetailLayout.HideProperty(UseExtensionHandle);
	InDetailLayout.HideProperty(ContentVMNameHandle);
	InDetailLayout.HideProperty(ContentVMClassHandle);
	InDetailLayout.HideProperty(ItemVMNameHandle);
	InDetailLayout.HideProperty(ItemVMClassHandle);

	IDetailCategoryBuilder& MVVMCategory = InDetailLayout.EditCategory("Viewmodel");

	const TWeakObjectPtr<URichComboBoxKey> WeakComboBox(ComboBox);
	const TWeakPtr<IPropertyUtilities> WeakPropertyUtilities(InDetailLayout.GetPropertyUtilities());

	// PanelWidget과 동일한 스타일의 Viewmodel Extension 활성화/해제 토글 버튼 로우 추가
	MVVMCategory.AddCustomRow(LOCTEXT("ViewmodelExtensionFilter", "Viewmodel Extension"))
		.RowTag(FName("RichComboBoxViewmodelExtension"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("VMExtensionLabel", "Viewmodel Extension"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ToolTipText(LOCTEXT("VMExtensionToolTip", "활성화하면 ViewModelItems에 전달된 키-ViewModel 맵을 사용해 Content 및 Item 위젯을 자동 생성하고 UMVVMView를 대입함."))
		]
		.ValueContent()
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateSP(
					this,
					&FRichComboBoxKeyCustomizationExtender::HandleToggleExtensionClicked,
					WeakComboBox,
					WeakPropertyUtilities,
					TSharedPtr<IPropertyHandle>(UseExtensionHandle),
					TSharedPtr<IPropertyHandle>(ContentWidgetClassHandle),
					TSharedPtr<IPropertyHandle>(ContentVMNameHandle),
					TSharedPtr<IPropertyHandle>(ContentVMClassHandle),
					TSharedPtr<IPropertyHandle>(ItemWidgetClassHandle),
					TSharedPtr<IPropertyHandle>(ItemVMNameHandle),
					TSharedPtr<IPropertyHandle>(ItemVMClassHandle)
				))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SImage)
						.Image(this, &FRichComboBoxKeyCustomizationExtender::GetExtensionButtonIcon, WeakComboBox)
					]
					+ SHorizontalBox::Slot()
					.Padding(FMargin(3.0f, 0.0f, 0.0f, 0.0f))
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "SmallButtonText")
						.Text(this, &FRichComboBoxKeyCustomizationExtender::GetExtensionButtonText, WeakComboBox)
					]
				]
			]
		];

	if (ComboBox->GetUseViewModelExtension() == false)
	{
		InDetailLayout.HideProperty(ContentWidgetClassHandle);
		InDetailLayout.HideProperty(ItemWidgetClassHandle);
		return;
	}

	AddEntrySlotRows(
		MVVMCategory,
		WeakComboBox,
		WeakPropertyUtilities,
		ERichComboBoxExtensionSlot::Content,
		ContentWidgetClassHandle,
		ContentVMNameHandle,
		ContentVMClassHandle
	);

	AddEntrySlotRows(
		MVVMCategory,
		WeakComboBox,
		WeakPropertyUtilities,
		ERichComboBoxExtensionSlot::Item,
		ItemWidgetClassHandle,
		ItemVMNameHandle,
		ItemVMClassHandle
	);
}

void FRichComboBoxKeyCustomizationExtender::AddEntrySlotRows(
	IDetailCategoryBuilder& InMVVMCategory,
	TWeakObjectPtr<URichComboBoxKey> InComboBox,
	TWeakPtr<IPropertyUtilities> InPropertyUtilities,
	ERichComboBoxExtensionSlot InSlot,
	const TSharedPtr<IPropertyHandle>& InWidgetClassHandle,
	const TSharedPtr<IPropertyHandle>& InVMNameHandle,
	const TSharedPtr<IPropertyHandle>& InVMClassHandle)
{
	const bool bIsContent = (InSlot == ERichComboBoxExtensionSlot::Content);
	const FText SlotDisplayPrefix = bIsContent ? LOCTEXT("ContentPrefix", "Content") : LOCTEXT("ItemPrefix", "Item");

	if (InWidgetClassHandle.IsValid() == true)
	{
		InWidgetClassHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(
			this,
			&FRichComboBoxKeyCustomizationExtender::HandleWidgetClassChanged,
			InComboBox,
			InPropertyUtilities,
			InWidgetClassHandle,
			InVMNameHandle,
			InVMClassHandle,
			InSlot
		));
		InMVVMCategory.AddProperty(InWidgetClassHandle);
	}

	InMVVMCategory.AddCustomRow(FText::Format(LOCTEXT("EntryVMSearchFormat", "{0} Entry Viewmodel"), SlotDisplayPrefix))
		.RowTag(bIsContent ? FName("RichComboBoxContentEntryVM") : FName("RichComboBoxItemEntryVM"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("EntryVMLabelFormat", "{0} Entry Viewmodel"), SlotDisplayPrefix))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ToolTipText(LOCTEXT("EntryVMToolTip", "지정한 엔트리 위젯 블루프린트에 등록된 ViewModel 중 자동 주입할 대상을 선택함."))
		]
		.ValueContent()
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SComboButton)
				.OnGetMenuContent(FOnGetContent::CreateSP(this, &FRichComboBoxKeyCustomizationExtender::BuildEntryViewModelsMenu, InComboBox, InPropertyUtilities, InVMNameHandle, InVMClassHandle, InSlot))
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &FRichComboBoxKeyCustomizationExtender::GetSelectedEntryViewModelText, InComboBox, InSlot)
					.ToolTipText(this, &FRichComboBoxKeyCustomizationExtender::GetSelectedEntryViewModelText, InComboBox, InSlot)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				PropertyCustomizationHelpers::MakeClearButton(
					FSimpleDelegate::CreateSP(
						this,
						&FRichComboBoxKeyCustomizationExtender::ApplyEntryViewModelSelection,
						InComboBox,
						InPropertyUtilities,
						InVMNameHandle,
						InVMClassHandle,
						InSlot,
						FName(NAME_None),
						static_cast<UClass*>(nullptr)
					)
				)
			]
		];
}

FReply FRichComboBoxKeyCustomizationExtender::HandleToggleExtensionClicked(
	TWeakObjectPtr<URichComboBoxKey> InComboBox,
	TWeakPtr<IPropertyUtilities> InPropertyUtilities,
	TSharedPtr<IPropertyHandle> InUseExtensionHandle,
	TSharedPtr<IPropertyHandle> InContentClassHandle,
	TSharedPtr<IPropertyHandle> InContentVMNameHandle,
	TSharedPtr<IPropertyHandle> InContentVMClassHandle,
	TSharedPtr<IPropertyHandle> InItemClassHandle,
	TSharedPtr<IPropertyHandle> InItemVMNameHandle,
	TSharedPtr<IPropertyHandle> InItemVMClassHandle)
{
	URichComboBoxKey* ComboBox = InComboBox.Get();
	if (ComboBox == nullptr || InUseExtensionHandle.IsValid() == false)
	{
		return FReply::Handled();
	}

	const bool bNewEnabled = (ComboBox->GetUseViewModelExtension() == false);
	const FScopedTransaction Transaction(bNewEnabled
		? LOCTEXT("EnableRichComboBoxVMExtension", "Add Viewmodel Extension")
		: LOCTEXT("DisableRichComboBoxVMExtension", "Remove Viewmodel Extension"));

	InUseExtensionHandle->SetValue(bNewEnabled);

	if (bNewEnabled == false)
	{
		const UObject* NullObject = nullptr;
		if (InContentClassHandle.IsValid() == true)
		{
			InContentClassHandle->SetValue(NullObject);
		}
		if (InItemClassHandle.IsValid() == true)
		{
			InItemClassHandle->SetValue(NullObject);
		}

		WriteSlotViewModelProperties(ComboBox, InContentVMNameHandle, InContentVMClassHandle, ERichComboBoxExtensionSlot::Content, NAME_None, nullptr);
		WriteSlotViewModelProperties(ComboBox, InItemVMNameHandle, InItemVMClassHandle, ERichComboBoxExtensionSlot::Item, NAME_None, nullptr);
	}

	if (const TSharedPtr<IPropertyUtilities> PropertyUtilities = InPropertyUtilities.Pin())
	{
		PropertyUtilities->ForceRefresh();
	}

	return FReply::Handled();
}

void FRichComboBoxKeyCustomizationExtender::HandleWidgetClassChanged(
	TWeakObjectPtr<URichComboBoxKey> InComboBox,
	TWeakPtr<IPropertyUtilities> InPropertyUtilities,
	TSharedPtr<IPropertyHandle> InWidgetClassHandle,
	TSharedPtr<IPropertyHandle> InVMNameHandle,
	TSharedPtr<IPropertyHandle> InVMClassHandle,
	ERichComboBoxExtensionSlot InSlot)
{
	URichComboBoxKey* ComboBox = InComboBox.Get();
	if (ComboBox == nullptr || InVMNameHandle.IsValid() == false || InVMClassHandle.IsValid() == false)
	{
		return;
	}

	UObject* HandleClassObject = nullptr;
	UClass* WidgetClass = nullptr;
	if (InWidgetClassHandle.IsValid() == true && InWidgetClassHandle->GetValue(HandleClassObject) == FPropertyAccess::Success)
	{
		WidgetClass = Cast<UClass>(HandleClassObject);
	}
	if (WidgetClass == nullptr)
	{
		WidgetClass = GetSlotWidgetClass(ComboBox, InSlot).Get();
	}

	const UMVVMBlueprintView* BlueprintView = FindBlueprintViewForWidgetClass(WidgetClass);
	const FName CurrentVMName = GetSlotViewModelName(ComboBox, InSlot);

	// 새 위젯 블루프린트에 등록된 ViewModel이 정확히 1개라면 즉시 자동 매핑해 작업 효율을 높임
	if (BlueprintView != nullptr && BlueprintView->GetViewModels().Num() == 1)
	{
		const FMVVMBlueprintViewModelContext& OnlyContext = BlueprintView->GetViewModels()[0];
		WriteSlotViewModelProperties(
			ComboBox,
			InVMNameHandle,
			InVMClassHandle,
			InSlot,
			OnlyContext.GetViewModelName(),
			OnlyContext.GetViewModelClass()
		);
	}
	else if (CurrentVMName.IsNone() == false)
	{
		const bool bValidInNewView = (BlueprintView != nullptr && BlueprintView->FindViewModel(CurrentVMName) != nullptr);
		if (bValidInNewView == false)
		{
			WriteSlotViewModelProperties(ComboBox, InVMNameHandle, InVMClassHandle, InSlot, NAME_None, nullptr);
		}
	}

	if (const TSharedPtr<IPropertyUtilities> PropertyUtilities = InPropertyUtilities.Pin())
	{
		PropertyUtilities->ForceRefresh();
	}
}

TSharedRef<SWidget> FRichComboBoxKeyCustomizationExtender::BuildEntryViewModelsMenu(
	TWeakObjectPtr<URichComboBoxKey> InComboBox,
	TWeakPtr<IPropertyUtilities> InPropertyUtilities,
	TSharedPtr<IPropertyHandle> InVMNameHandle,
	TSharedPtr<IPropertyHandle> InVMClassHandle,
	ERichComboBoxExtensionSlot InSlot)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	const URichComboBoxKey* ComboBox = InComboBox.Get();
	if (ComboBox != nullptr)
	{
		const TSubclassOf<UUserWidget> WidgetClass = GetSlotWidgetClass(ComboBox, InSlot);
		if (const UMVVMBlueprintView* BlueprintView = FindBlueprintViewForWidgetClass(WidgetClass.Get()))
		{
			for (const FMVVMBlueprintViewModelContext& EntryVM : BlueprintView->GetViewModels())
			{
				FUIAction ItemAction(FExecuteAction::CreateSP(
					this,
					&FRichComboBoxKeyCustomizationExtender::ApplyEntryViewModelSelection,
					InComboBox,
					InPropertyUtilities,
					InVMNameHandle,
					InVMClassHandle,
					InSlot,
					EntryVM.GetViewModelName(),
					EntryVM.GetViewModelClass()
				));

				const FText SubText = (EntryVM.GetViewModelClass() != nullptr)
					? EntryVM.GetViewModelClass()->GetDisplayNameText()
					: FText::GetEmpty();

				MenuBuilder.AddMenuEntry(
					FText::FromName(EntryVM.GetViewModelName()),
					SubText,
					FSlateIcon(),
					ItemAction
				);
			}
		}
	}

	return MenuBuilder.MakeWidget();
}

void FRichComboBoxKeyCustomizationExtender::ApplyEntryViewModelSelection(
	TWeakObjectPtr<URichComboBoxKey> InComboBox,
	TWeakPtr<IPropertyUtilities> InPropertyUtilities,
	TSharedPtr<IPropertyHandle> InVMNameHandle,
	TSharedPtr<IPropertyHandle> InVMClassHandle,
	ERichComboBoxExtensionSlot InSlot,
	FName InViewModelName,
	UClass* InViewModelClass)
{
	URichComboBoxKey* ComboBox = InComboBox.Get();
	if (ComboBox == nullptr)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetComboBoxEntryViewModel", "Set ComboBox Entry ViewModel"));
	WriteSlotViewModelProperties(ComboBox, InVMNameHandle, InVMClassHandle, InSlot, InViewModelName, InViewModelClass);

	if (const TSharedPtr<IPropertyUtilities> PropertyUtilities = InPropertyUtilities.Pin())
	{
		PropertyUtilities->ForceRefresh();
	}
}

void FRichComboBoxKeyCustomizationExtender::WriteSlotViewModelProperties(
	URichComboBoxKey* InComboBox,
	const TSharedPtr<IPropertyHandle>& InVMNameHandle,
	const TSharedPtr<IPropertyHandle>& InVMClassHandle,
	ERichComboBoxExtensionSlot InSlot,
	FName InViewModelName,
	UClass* InViewModelClass)
{
	// 클래스 핸들을 먼저 갱신하고 이름 핸들을 갱신하여 핸들 무효화를 방지하며, 실패 시 대상 객체에 직접 반영함
	bool bClassWritten = false;
	if (InVMClassHandle.IsValid() == true && InVMClassHandle->IsValidHandle() == true)
	{
		const FString ClassPathString = (InViewModelClass != nullptr) ? InViewModelClass->GetPathName() : TEXT("None");
		bClassWritten = (InVMClassHandle->SetValueFromFormattedString(ClassPathString) == FPropertyAccess::Success);
	}

	bool bNameWritten = false;
	if (InVMNameHandle.IsValid() == true && InVMNameHandle->IsValidHandle() == true)
	{
		bNameWritten = (InVMNameHandle->SetValue(InViewModelName) == FPropertyAccess::Success);
	}

	if (InComboBox != nullptr && (bClassWritten == false || bNameWritten == false))
	{
		InComboBox->Modify();
		if (InSlot == ERichComboBoxExtensionSlot::Content)
		{
			InComboBox->SetContentViewModelName(InViewModelName);
			InComboBox->SetContentViewModelClass(InViewModelClass);
		}
		else
		{
			InComboBox->SetItemViewModelName(InViewModelName);
			InComboBox->SetItemViewModelClass(InViewModelClass);
		}
	}
}

FText FRichComboBoxKeyCustomizationExtender::GetExtensionButtonText(TWeakObjectPtr<URichComboBoxKey> InComboBox) const
{
	const URichComboBoxKey* ComboBox = InComboBox.Get();
	const bool bEnabled = (ComboBox != nullptr && ComboBox->GetUseViewModelExtension() == true);
	return bEnabled
		? LOCTEXT("RemoveVMExtText", "Remove Viewmodel Extension")
		: LOCTEXT("AddVMExtText", "Add Viewmodel Extension");
}

const FSlateBrush* FRichComboBoxKeyCustomizationExtender::GetExtensionButtonIcon(TWeakObjectPtr<URichComboBoxKey> InComboBox) const
{
	const URichComboBoxKey* ComboBox = InComboBox.Get();
	const bool bEnabled = (ComboBox != nullptr && ComboBox->GetUseViewModelExtension() == true);
	return bEnabled ? FAppStyle::Get().GetBrush("Icons.X") : FAppStyle::Get().GetBrush("Icons.Plus");
}

FText FRichComboBoxKeyCustomizationExtender::GetSelectedEntryViewModelText(
	TWeakObjectPtr<URichComboBoxKey> InComboBox,
	ERichComboBoxExtensionSlot InSlot) const
{
	const URichComboBoxKey* ComboBox = InComboBox.Get();
	if (ComboBox != nullptr)
	{
		const FName SelectedVMName = GetSlotViewModelName(ComboBox, InSlot);
		if (SelectedVMName.IsNone() == false)
		{
			return FText::FromName(SelectedVMName);
		}
	}

	return LOCTEXT("NoEntryViewmodel", "No Viewmodel");
}

TSubclassOf<UUserWidget> FRichComboBoxKeyCustomizationExtender::GetSlotWidgetClass(
	const URichComboBoxKey* InComboBox,
	ERichComboBoxExtensionSlot InSlot)
{
	if (InComboBox == nullptr)
	{
		return nullptr;
	}

	return (InSlot == ERichComboBoxExtensionSlot::Content)
		? InComboBox->GetContentWidgetClass()
		: InComboBox->GetItemWidgetClass();
}

FName FRichComboBoxKeyCustomizationExtender::GetSlotViewModelName(
	const URichComboBoxKey* InComboBox,
	ERichComboBoxExtensionSlot InSlot)
{
	if (InComboBox == nullptr)
	{
		return NAME_None;
	}

	return (InSlot == ERichComboBoxExtensionSlot::Content)
		? InComboBox->GetContentViewModelName()
		: InComboBox->GetItemViewModelName();
}

const UMVVMBlueprintView* FRichComboBoxKeyCustomizationExtender::FindBlueprintViewForWidgetClass(const UClass* InWidgetClass)
{
	if (InWidgetClass == nullptr)
	{
		return nullptr;
	}

	const UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(InWidgetClass->ClassGeneratedBy);
	if (WidgetBlueprint == nullptr)
	{
		return nullptr;
	}

	const UMVVMWidgetBlueprintExtension_View* ViewExtension = UMVVMWidgetBlueprintExtension_View::GetExtension<UMVVMWidgetBlueprintExtension_View>(WidgetBlueprint);
	if (ViewExtension == nullptr)
	{
		return nullptr;
	}

	return ViewExtension->GetBlueprintView();
}

#undef LOCTEXT_NAMESPACE
