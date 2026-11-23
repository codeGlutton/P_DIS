#include "Components/RichComboBoxKey.h"

#include "Blueprint/UserWidget.h"
#include "ExtensionWidgetModule.h"
#include "INotifyFieldValueChanged.h"
#include "MVVMSubsystem.h"
#include "Styling/DefaultStyleCache.h"
#include "View/MVVMView.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ExtensionWidget"

URichComboBoxKey::URichComboBoxKey() :
	Super()
{
	if (IsRunningDedicatedServer() == false)
	{
#if WITH_EDITOR
		WidgetStyle = IsEditorWidget()
			? UE::Slate::Private::FDefaultStyleCache::GetEditor().GetComboBoxStyle()
			: UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetComboBoxStyle();
		ItemStyle = IsEditorWidget()
			? UE::Slate::Private::FDefaultStyleCache::GetEditor().GetTableRowStyle()
			: UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetTableRowStyle();
		ScrollBarStyle = IsEditorWidget()
			? UE::Slate::Private::FDefaultStyleCache::GetEditor().GetScrollBarStyle()
			: UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetScrollBarStyle();
#else
		WidgetStyle = UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetComboBoxStyle();
		ItemStyle = UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetTableRowStyle();
		ScrollBarStyle = UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetScrollBarStyle();
#endif // WITH_EDITOR
	}

	ContentPadding = FMargin(4.0, 2.0);
	ForegroundColor = ItemStyle.TextColor;

	MaxListHeight = 450.0f;
	bHasDownArrow = true;
	bEnableGamepadNavigationMode = true;
	bIsFocusable = true;

	SelectedOption = NAME_None;
	bUseViewModelExtension = false;
	ContentViewModelName = NAME_None;
	ItemViewModelName = NAME_None;
}

void URichComboBoxKey::SetViewModelItems(const TArray<FRichComboBoxKeyViewModelItem>& InItems)
{
	if (ViewModelItems == InItems)
	{
		return;
	}

	// NAME_None 및 중복 키를 정규화해 ViewModelItems, Options, CachedKeyIndices를 1:1로 일치시킴
	TArray<FRichComboBoxKeyViewModelItem> NormalizedItems;
	NormalizedItems.Reserve(InItems.Num());

	TArray<FName> NextOptions;
	NextOptions.Reserve(InItems.Num());

	TMap<FName, int32> NextKeyIndices;
	NextKeyIndices.Reserve(InItems.Num());

	for (const FRichComboBoxKeyViewModelItem& Item : InItems)
	{
		if (Item.Key.IsNone() == true)
		{
			UE_LOG(LogExtensionWidget, Warning, TEXT("URichComboBoxKey(%s)::SetViewModelItems - NAME_None 키 항목이 전달되어 건너뜀"), *GetName());
			continue;
		}

		if (const int32* ExistingIndex = NextKeyIndices.Find(Item.Key))
		{
			UE_LOG(LogExtensionWidget, Warning, TEXT("URichComboBoxKey(%s)::SetViewModelItems - 중복 키(%s)가 감지되어 최신 ViewModel로 덮어씀"), *GetName(), *Item.Key.ToString());
			NormalizedItems[*ExistingIndex].ViewModel = Item.ViewModel;
			continue;
		}

		const int32 NewIndex = NormalizedItems.Add(Item);
		NextOptions.Add(Item.Key);
		NextKeyIndices.Add(Item.Key, NewIndex);
	}

	if (ViewModelItems == NormalizedItems && Options == NextOptions)
	{
		CachedKeyIndices = MoveTemp(NextKeyIndices);
		return;
	}

	ViewModelItems = MoveTemp(NormalizedItems);
	Options = MoveTemp(NextOptions);
	CachedKeyIndices = MoveTemp(NextKeyIndices);

	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->ClearSelection();
		MyComboBox->RefreshOptions();
	}
	else if (SelectedOption != NAME_None)
	{
		SelectedOption = NAME_None;
		BroadcastFieldValueChanged(ThisClass::FFieldNotificationClassDescriptor::SelectedOption);
		OnSelectedOptionChangedNative.Broadcast(NAME_None, INDEX_NONE);
		OnSelectedOptionChanged.Broadcast(NAME_None, INDEX_NONE);
	}

	BroadcastFieldValueChanged(ThisClass::FFieldNotificationClassDescriptor::ViewModelItems);
}

UObject* URichComboBoxKey::FindViewModelByKey(FName InKey) const
{
	const int32 FoundIndex = FindItemIndexByKey(InKey);
	if (ViewModelItems.IsValidIndex(FoundIndex) == true)
	{
		return ViewModelItems[FoundIndex].ViewModel.Get();
	}

	return nullptr;
}

int32 URichComboBoxKey::FindItemIndexByKey(FName InKey) const
{
	if (const int32* FoundIndex = CachedKeyIndices.Find(InKey))
	{
		return *FoundIndex;
	}

	return INDEX_NONE;
}

void URichComboBoxKey::AddOption(FName Option)
{
	if (Options.AddUnique(Option) >= 0)
	{
		CachedKeyIndices.Add(Option, Options.Num() - 1);

		if (MyComboBox.IsValid() == true)
		{
			MyComboBox->RefreshOptions();
		}
	}
}

bool URichComboBoxKey::RemoveOption(FName Option)
{
	const bool bResult = (Options.RemoveSingle(Option) > 0);
	if (bResult == true)
	{
		CachedKeyIndices.Remove(Option);
		for (int32 OptionIndex = 0; OptionIndex < Options.Num(); ++OptionIndex)
		{
			CachedKeyIndices[Options[OptionIndex]] = OptionIndex;
		}

		if (Option == SelectedOption)
		{
			ClearSelection();
		}

		if (MyComboBox.IsValid() == true)
		{
			MyComboBox->RefreshOptions();
		}
	}

	return bResult;
}

void URichComboBoxKey::ClearOptions()
{
	Options.Reset();
	CachedKeyIndices.Reset();

	ClearSelection();
	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->RefreshOptions();
	}
}

void URichComboBoxKey::ClearSelection()
{
	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->ClearSelection();
	}
	else if (SelectedOption != NAME_None)
	{
		SelectedOption = NAME_None;
		BroadcastFieldValueChanged(ThisClass::FFieldNotificationClassDescriptor::SelectedOption);
		OnSelectedOptionChangedNative.Broadcast(NAME_None, INDEX_NONE);
		OnSelectedOptionChanged.Broadcast(NAME_None, INDEX_NONE);
	}
}

void URichComboBoxKey::SetSelectedOption(FName Option)
{
	if (SelectedOption == Option)
	{
		return;
	}

	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->SetSelectedItem(Option);
	}
	else
	{
		SelectedOption = Option;
		BroadcastFieldValueChanged(ThisClass::FFieldNotificationClassDescriptor::SelectedOption);
		const int32 ItemIndex = FindItemIndexByKey(SelectedOption);
		OnSelectedOptionChangedNative.Broadcast(SelectedOption, ItemIndex);
		OnSelectedOptionChanged.Broadcast(SelectedOption, ItemIndex);
	}
}

FName URichComboBoxKey::GetSelectedOption() const
{
	return SelectedOption;
}

bool URichComboBoxKey::IsOpen() const
{
	return (MyComboBox.IsValid() == true && MyComboBox->IsOpen() == true);
}

void URichComboBoxKey::SetContentPadding(FMargin InPadding)
{
	ContentPadding = InPadding;
	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->SetButtonContentPadding(InPadding);
	}
}

FMargin URichComboBoxKey::GetContentPadding() const
{
	return ContentPadding;
}

bool URichComboBoxKey::IsEnableGamepadNavigationMode() const
{
	return bEnableGamepadNavigationMode;
}

void URichComboBoxKey::SetEnableGamepadNavigationMode(bool InEnableGamepadNavigationMode)
{
	bEnableGamepadNavigationMode = InEnableGamepadNavigationMode;
	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->SetEnableGamepadNavigationMode(bEnableGamepadNavigationMode);
	}
}

bool URichComboBoxKey::IsHasDownArrow() const
{
	return bHasDownArrow;
}

void URichComboBoxKey::SetHasDownArrow(bool InHasDownArrow)
{
	bHasDownArrow = InHasDownArrow;
	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->SetHasDownArrow(InHasDownArrow);
	}
}

float URichComboBoxKey::GetMaxListHeight() const
{
	return MaxListHeight;
}

void URichComboBoxKey::SetMaxListHeight(float InMaxHeight)
{
	MaxListHeight = InMaxHeight;
	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->SetMaxHeight(MaxListHeight);
	}
}

const FComboBoxStyle& URichComboBoxKey::GetWidgetStyle() const
{
	return WidgetStyle;
}

void URichComboBoxKey::SetWidgetStyle(const FComboBoxStyle& InWidgetStyle)
{
	WidgetStyle = InWidgetStyle;
	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->InvalidateStyle();
	}
}

const FTableRowStyle& URichComboBoxKey::GetItemStyle() const
{
	return ItemStyle;
}

void URichComboBoxKey::SetItemStyle(const FTableRowStyle& InItemStyle)
{
	ItemStyle = InItemStyle;
	if (MyComboBox.IsValid() == true)
	{
		MyComboBox->InvalidateItemStyle();
	}
}

const FScrollBarStyle& URichComboBoxKey::GetScrollBarStyle() const
{
	return ScrollBarStyle;
}

bool URichComboBoxKey::IsFocusable() const
{
	return bIsFocusable;
}

FSlateColor URichComboBoxKey::GetForegroundColor() const
{
	return ForegroundColor;
}

void URichComboBoxKey::SetUseViewModelExtension(bool bInUseViewModelExtension)
{
	if (bUseViewModelExtension == bInUseViewModelExtension)
	{
		return;
	}

	bUseViewModelExtension = bInUseViewModelExtension;

	if (GetCachedWidget().IsValid() == true)
	{
		GenerateContent();
		if (MyComboBox.IsValid() == true)
		{
			MyComboBox->RefreshOptions();
		}
	}
}

void URichComboBoxKey::SetContentWidgetClass(TSubclassOf<UUserWidget> InContentWidgetClass)
{
	ContentWidgetClass = InContentWidgetClass;
}

void URichComboBoxKey::SetContentViewModelName(FName InContentViewModelName)
{
	ContentViewModelName = InContentViewModelName;
}

void URichComboBoxKey::SetContentViewModelClass(TSubclassOf<UObject> InContentViewModelClass)
{
	ContentViewModelClass = InContentViewModelClass;
}

void URichComboBoxKey::SetItemWidgetClass(TSubclassOf<UUserWidget> InItemWidgetClass)
{
	ItemWidgetClass = InItemWidgetClass;
}

void URichComboBoxKey::SetItemViewModelName(FName InItemViewModelName)
{
	ItemViewModelName = InItemViewModelName;
}

void URichComboBoxKey::SetItemViewModelClass(TSubclassOf<UObject> InItemViewModelClass)
{
	ItemViewModelClass = InItemViewModelClass;
}

void URichComboBoxKey::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyComboBox.Reset();
	ComboBoxContent.Reset();
}

#if WITH_EDITOR
const FText URichComboBoxKey::GetPaletteCategory()
{
	return LOCTEXT("ExtensionWidgetPaletteCategory", "Extension Widget");
}
#endif

TSharedRef<SWidget> URichComboBoxKey::RebuildWidget()
{
	MyComboBox =
		SNew(SComboBox<FName>)
		.ComboBoxStyle(&WidgetStyle)
		.ItemStyle(&ItemStyle)
		.ForegroundColor(ForegroundColor)
		.OptionsSource(&Options)
		.InitiallySelectedItem(SelectedOption)
		.ContentPadding(ContentPadding)
		.MaxListHeight(MaxListHeight)
		.HasDownArrow(bHasDownArrow)
		.EnableGamepadNavigationMode(bEnableGamepadNavigationMode)
		.OnGenerateWidget_UObject(this, &URichComboBoxKey::HandleGenerateItemWidget)
		.OnSelectionChanged_UObject(this, &URichComboBoxKey::HandleSelectionChanged)
		.OnComboBoxOpening_UObject(this, &URichComboBoxKey::HandleOpening)
		.IsFocusable(bIsFocusable)
		.ScrollBarStyle(&ScrollBarStyle)
		[
			SAssignNew(ComboBoxContent, SBox)
		];

	GenerateContent();

	return MyComboBox.ToSharedRef();
}

void URichComboBoxKey::InitScrollBarStyle(const FScrollBarStyle& InScrollBarStyle)
{
	ensureMsgf(MyComboBox.IsValid() == false, TEXT("The widget is already created."));
	ScrollBarStyle = InScrollBarStyle;
}

void URichComboBoxKey::InitIsFocusable(bool InIsFocusable)
{
	ensureMsgf(MyComboBox.IsValid() == false, TEXT("The widget is already created."));
	bIsFocusable = InIsFocusable;
}

void URichComboBoxKey::InitForegroundColor(FSlateColor InForegroundColor)
{
	ensureMsgf(MyComboBox.IsValid() == false, TEXT("The widget is already created."));
	ForegroundColor = InForegroundColor;
}

void URichComboBoxKey::GenerateContent()
{
	if (ComboBoxContent.IsValid() == false)
	{
		return;
	}

	bool bSetToNull = true;
	if (IsDesignTime() == true)
	{
		if (SelectedOption.IsNone() == false)
		{
			ComboBoxContent->SetContent(
				SNew(STextBlock)
				.Text(FText::FromName(SelectedOption))
			);
			bSetToNull = false;
		}
	}
	else if (bUseViewModelExtension == true)
	{
		// bUseViewModelExtension 활성화 시 OnGenerateContentWidget 대리자 대신 ViewModel 자동 주입 경로만 사용함
		if (UUserWidget* EntryWidget = GenerateEntryWidgetWithViewModel(SelectedOption, ContentWidgetClass, ContentViewModelName, ContentViewModelClass))
		{
			ComboBoxContent->SetContent(EntryWidget->TakeWidget());
			bSetToNull = false;
		}
	}
	else
	{
		if (OnGenerateContentWidget.IsBound() == true)
		{
			if (UWidget* Widget = OnGenerateContentWidget.Execute(SelectedOption))
			{
				ComboBoxContent->SetContent(Widget->TakeWidget());
				bSetToNull = false;
			}
		}
		else
		{
			ComboBoxContent->SetContent(
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("WarningMissingOnGenerateContentWidget", "{0} Missing OnGenerateContentWidget"), FText::FromName(SelectedOption)))
			);
			bSetToNull = false;
		}
	}

	if (bSetToNull == true)
	{
		ComboBoxContent->SetContent(SNullWidget::NullWidget);
	}
}

TSharedRef<SWidget> URichComboBoxKey::HandleGenerateItemWidget(FName Item)
{
	if (IsDesignTime() == true)
	{
		return SNew(STextBlock)
			.Text(FText::FromName(Item));
	}

	if (bUseViewModelExtension == true)
	{
		// bUseViewModelExtension 활성화 시 OnGenerateItemWidget 대리자 대신 ViewModel 자동 주입 경로만 사용함
		if (UUserWidget* EntryWidget = GenerateEntryWidgetWithViewModel(Item, ItemWidgetClass, ItemViewModelName, ItemViewModelClass))
		{
			return EntryWidget->TakeWidget();
		}

		return SNullWidget::NullWidget;
	}

	if (OnGenerateItemWidget.IsBound() == true)
	{
		if (UWidget* Widget = OnGenerateItemWidget.Execute(Item))
		{
			return Widget->TakeWidget();
		}

		return SNullWidget::NullWidget;
	}

	return SNew(STextBlock)
		.Text(FText::Format(LOCTEXT("WarningMissingOnGenerateItemWidget", "{0} Missing OnGenerateItemWidget"), FText::FromName(Item)));
}

void URichComboBoxKey::HandleSelectionChanged(FName Item, ESelectInfo::Type SelectionType)
{
	if (SelectedOption != Item)
	{
		SelectedOption = Item;
		BroadcastFieldValueChanged(ThisClass::FFieldNotificationClassDescriptor::SelectedOption);
		const int32 ItemIndex = FindItemIndexByKey(SelectedOption);
		OnSelectedOptionChangedNative.Broadcast(SelectedOption, ItemIndex);
		OnSelectedOptionChanged.Broadcast(SelectedOption, ItemIndex);

		if (IsDesignTime() == false)
		{
			OnSelectionChanged.Broadcast(Item, SelectionType);
		}
		GenerateContent();
	}
}

void URichComboBoxKey::HandleOpening()
{
	if (IsDesignTime() == false)
	{
		OnOpening.Broadcast();
	}
}

UUserWidget* URichComboBoxKey::GenerateEntryWidgetWithViewModel(
	FName InKey,
	TSubclassOf<UUserWidget> InWidgetClass,
	FName InViewModelName,
	TSubclassOf<UObject> InExpectedViewModelClass)
{
	if (InWidgetClass == nullptr)
	{
		return nullptr;
	}

	UUserWidget* EntryWidget = UUserWidget::CreateWidgetInstance(*this, InWidgetClass, NAME_None);
	if (EntryWidget == nullptr)
	{
		UE_LOG(LogExtensionWidget, Warning, TEXT("URichComboBoxKey(%s)::GenerateEntryWidgetWithViewModel - 위젯(%s) 생성에 실패함"), *GetName(), *InWidgetClass->GetName());
		return nullptr;
	}

	UObject* ViewModelObject = FindViewModelByKey(InKey);
	if (ViewModelObject == nullptr)
	{
		return EntryWidget;
	}

	if (InExpectedViewModelClass != nullptr && ViewModelObject->GetClass()->IsChildOf(InExpectedViewModelClass) == false)
	{
		UE_LOG(
			LogExtensionWidget,
			Warning,
			TEXT("URichComboBoxKey(%s) - 키(%s)의 ViewModel(%s)이 지정된 클래스(%s)와 일치하지 않음"),
			*GetName(),
			*InKey.ToString(),
			*ViewModelObject->GetClass()->GetName(),
			*InExpectedViewModelClass->GetName()
		);
	}

	if (ViewModelObject->Implements<UNotifyFieldValueChanged>() == false)
	{
		UE_LOG(
			LogExtensionWidget,
			Warning,
			TEXT("URichComboBoxKey(%s) - 키(%s)의 객체(%s)가 INotifyFieldValueChanged를 구현하지 않아 MVVMView에 대입할 수 없음"),
			*GetName(),
			*InKey.ToString(),
			*ViewModelObject->GetName()
		);
		return EntryWidget;
	}

	UMVVMView* MVVMView = UMVVMSubsystem::GetViewFromUserWidget(EntryWidget);
	if (MVVMView == nullptr)
	{
		UE_LOG(
			LogExtensionWidget,
			Warning,
			TEXT("URichComboBoxKey(%s) - 생성된 위젯(%s)에 UMVVMView 확장이 존재하지 않아 ViewModel을 대입할 수 없음"),
			*GetName(),
			*EntryWidget->GetName()
		);
		return EntryWidget;
	}

	bool bAssigned = false;
	if (InViewModelName.IsNone() == false)
	{
		bAssigned = MVVMView->SetViewModel(InViewModelName, TScriptInterface<INotifyFieldValueChanged>(ViewModelObject));
	}

	if (bAssigned == false)
	{
		bAssigned = MVVMView->SetViewModelByClass(TScriptInterface<INotifyFieldValueChanged>(ViewModelObject));
	}

	if (bAssigned == false)
	{
		UE_LOG(
			LogExtensionWidget,
			Warning,
			TEXT("URichComboBoxKey(%s) - 위젯(%s)에 키(%s)의 ViewModel(%s, 이름: %s) 대입에 실패함"),
			*GetName(),
			*EntryWidget->GetName(),
			*InKey.ToString(),
			*ViewModelObject->GetClass()->GetName(),
			*InViewModelName.ToString()
		);
	}

	return EntryWidget;
}

#undef LOCTEXT_NAMESPACE
