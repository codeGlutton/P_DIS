#include "Components/RichListView.h"

#include "Widgets/SRichListView.h"
#include "Widgets/SRichScrollBar.h"

#define LOCTEXT_NAMESPACE "ExtensionWidget"

URichListView::URichListView(const FObjectInitializer& Initializer) :
	Super(Initializer),
	RichScrollBarConfig(),
	bAlwaysShowRichScrollbar(false),
	RichScrollbarPlacement(ERichScrollBarPlacement::End)
{
}

void URichListView::SetRichScrollBarConfig(const FRichScrollBarConfig& InConfig)
{
	RichScrollBarConfig = InConfig;
	ApplyRichScrollBarProperties();
}

void URichListView::SetAlwaysShowRichScrollbar(bool bInAlwaysShow)
{
	bAlwaysShowRichScrollbar = bInAlwaysShow;
	ApplyRichScrollBarProperties();
}

void URichListView::SetRichScrollBarStyle(const FScrollBarStyle& InStyle)
{
	ScrollBarStyle = InStyle;
	ApplyRichScrollBarProperties();
}

void URichListView::SetEntryWidgetClass(TSubclassOf<UUserWidget> InEntryWidgetClass)
{
	EntryWidgetClass = InEntryWidgetClass;
}

TSharedPtr<SRichScrollBar> URichListView::GetRichScrollBar() const
{
	const TSharedPtr<SRichListView<UObject*>> SlateRichListView = GetSlateRichListView();
	if (SlateRichListView.IsValid() == true)
	{
		return SlateRichListView->GetRichScrollBar();
	}

	return nullptr;
}

void URichListView::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyRichScrollBarProperties();
}

#if WITH_EDITOR
const FText URichListView::GetPaletteCategory()
{
	return LOCTEXT("ExtensionWidgetPaletteCategory", "Extension Widget");
}
#endif

TSharedRef<STableViewBase> URichListView::RebuildListWidget()
{
	const TSharedRef<SRichListView<UObject*>> CreatedListView = ConstructListView<SRichListView>();
	MyListView = CreatedListView;
	ApplyRichScrollBarProperties();
	return CreatedListView;
}

TSharedPtr<SRichListView<UObject*>> URichListView::GetSlateRichListView() const
{
	return StaticCastSharedPtr<SRichListView<UObject*>>(MyListView);
}

void URichListView::ApplyRichScrollBarProperties()
{
	const TSharedPtr<SRichListView<UObject*>> SlateRichListView = GetSlateRichListView();
	if (SlateRichListView.IsValid() == false)
	{
		return;
	}

	SlateRichListView->ConfigureRichScrollBar(
		&ScrollBarStyle,
		GetOrientation(),
		bAlwaysShowRichScrollbar,
		GetScrollBarPadding(),
		RichScrollBarConfig,
		RichScrollbarPlacement
	);
}

#undef LOCTEXT_NAMESPACE
