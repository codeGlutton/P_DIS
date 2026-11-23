#include "Components/RichInputKeySelector.h"

#include "Engine/Font.h"
#include "Internationalization/Internationalization.h"
#include "Styling/DefaultStyleCache.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/FrameworkObjectVersion.h"
#include "Widgets/Input/SInputKeySelector.h"

#define LOCTEXT_NAMESPACE "ExtensionWidget"

URichInputKeySelector::URichInputKeySelector(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	WidgetStyle = UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetButtonStyle();
	TextStyle = UE::Slate::Private::FDefaultStyleCache::GetRuntime().GetTextBlockStyle();

#if WITH_EDITOR
	if (IsEditorWidget() == true)
	{
		WidgetStyle = UE::Slate::Private::FDefaultStyleCache::GetEditor().GetButtonStyle();
		TextStyle = UE::Slate::Private::FDefaultStyleCache::GetEditor().GetTextBlockStyle();
	}
#endif // WITH_EDITOR

	KeySelectionText = NSLOCTEXT("InputKeySelector", "DefaultKeySelectionText", "...");
	NoKeySpecifiedText = NSLOCTEXT("InputKeySelector", "DefaultEmptyText", "Empty");
	SelectedKey = FInputChord(EKeys::Invalid);
	bAllowModifierKeys = true;
	bAllowGamepadKeys = false;
	EscapeKeys.AddUnique(EKeys::Gamepad_Special_Right);

	if (IsRunningDedicatedServer() == false)
	{
		static ConstructorHelpers::FObjectFinder<UFont> RobotoFontObj(*UWidget::GetDefaultFontName());
		if (RobotoFontObj.Object != nullptr)
		{
			TextStyle.Font = FSlateFontInfo(RobotoFontObj.Object, 24, FName("Bold"));
		}
	}
}

void URichInputKeySelector::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FFrameworkObjectVersion::GUID);
}

void URichInputKeySelector::SetSelectedKey(const FInputChord& InSelectedKey)
{
	if (SelectedKey == InSelectedKey)
	{
		return;
	}

	if (MyInputKeySelector.IsValid() == true)
	{
		MyInputKeySelector->SetSelectedKey(InSelectedKey);
	}
	else
	{
		SelectedKey = InSelectedKey;
		BroadcastFieldValueChanged(ThisClass::FFieldNotificationClassDescriptor::SelectedKey);
		OnKeySelected.Broadcast(SelectedKey);
	}
}

FInputChord URichInputKeySelector::GetSelectedKey() const
{
	return SelectedKey;
}

void URichInputKeySelector::SetKeySelectionText(FText InKeySelectionText)
{
	if (MyInputKeySelector.IsValid() == true)
	{
		MyInputKeySelector->SetKeySelectionText(InKeySelectionText);
	}
	KeySelectionText = MoveTemp(InKeySelectionText);
}

const FText& URichInputKeySelector::GetKeySelectionText() const
{
	return KeySelectionText;
}

void URichInputKeySelector::SetNoKeySpecifiedText(FText InNoKeySpecifiedText)
{
	if (MyInputKeySelector.IsValid() == true)
	{
		MyInputKeySelector->SetNoKeySpecifiedText(InNoKeySpecifiedText);
	}
	NoKeySpecifiedText = MoveTemp(InNoKeySpecifiedText);
}

const FText& URichInputKeySelector::GetNoKeySpecifiedText() const
{
	return NoKeySpecifiedText;
}

void URichInputKeySelector::SetAllowModifierKeys(bool bInAllowModifierKeys)
{
	if (MyInputKeySelector.IsValid() == true)
	{
		MyInputKeySelector->SetAllowModifierKeys(bInAllowModifierKeys);
	}
	bAllowModifierKeys = bInAllowModifierKeys;
}

bool URichInputKeySelector::AllowModifierKeys() const
{
	return bAllowModifierKeys;
}

void URichInputKeySelector::SetAllowGamepadKeys(bool bInAllowGamepadKeys)
{
	if (MyInputKeySelector.IsValid() == true)
	{
		MyInputKeySelector->SetAllowGamepadKeys(bInAllowGamepadKeys);
	}
	bAllowGamepadKeys = bInAllowGamepadKeys;
}

bool URichInputKeySelector::AllowGamepadKeys() const
{
	return bAllowGamepadKeys;
}

bool URichInputKeySelector::GetIsSelectingKey() const
{
	return (MyInputKeySelector.IsValid() == true) ? MyInputKeySelector->GetIsSelectingKey() : false;
}

void URichInputKeySelector::SetButtonStyle(const FButtonStyle& InButtonStyle)
{
	if (MyInputKeySelector.IsValid() == true)
	{
		MyInputKeySelector->SetButtonStyle(&InButtonStyle);
	}
	WidgetStyle = InButtonStyle;
}

const FButtonStyle& URichInputKeySelector::GetButtonStyle() const
{
	return WidgetStyle;
}

void URichInputKeySelector::SetTextStyle(const FTextBlockStyle& InTextStyle)
{
	if (MyInputKeySelector.IsValid() == true)
	{
		MyInputKeySelector->SetTextStyle(&InTextStyle);
	}
	TextStyle = InTextStyle;
}

const FTextBlockStyle& URichInputKeySelector::GetTextStyle() const
{
	return TextStyle;
}

void URichInputKeySelector::SetEscapeKeys(const TArray<FKey>& InKeys)
{
	if (MyInputKeySelector.IsValid() == true)
	{
		MyInputKeySelector->SetEscapeKeys(InKeys);
	}
	EscapeKeys = InKeys;
}

void URichInputKeySelector::SetMargin(const FMargin& InMargin)
{
	if (MyInputKeySelector.IsValid() == true)
	{
		MyInputKeySelector->SetMargin(InMargin);
	}
	Margin = InMargin;
}

const FMargin& URichInputKeySelector::GetMargin() const
{
	return Margin;
}

#if WITH_EDITOR
const FText URichInputKeySelector::GetPaletteCategory()
{
	return LOCTEXT("ExtensionWidgetPaletteCategory", "Extension Widget");
}
#endif

void URichInputKeySelector::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyInputKeySelector.IsValid() == false)
	{
		return;
	}

	MyInputKeySelector->SetSelectedKey(SelectedKey);
	MyInputKeySelector->SetMargin(Margin);
	MyInputKeySelector->SetButtonStyle(&WidgetStyle);
	MyInputKeySelector->SetTextStyle(&TextStyle);
	MyInputKeySelector->SetKeySelectionText(KeySelectionText);
	MyInputKeySelector->SetNoKeySpecifiedText(NoKeySpecifiedText);
	MyInputKeySelector->SetAllowModifierKeys(bAllowModifierKeys);
	MyInputKeySelector->SetAllowGamepadKeys(bAllowGamepadKeys);
	MyInputKeySelector->SetEscapeKeys(EscapeKeys);
}

TSharedRef<SWidget> URichInputKeySelector::RebuildWidget()
{
	MyInputKeySelector = SNew(SInputKeySelector)
		.SelectedKey(SelectedKey)
		.Margin(Margin)
		.ButtonStyle(&WidgetStyle)
		.TextStyle(&TextStyle)
		.KeySelectionText(KeySelectionText)
		.NoKeySpecifiedText(NoKeySpecifiedText)
		.AllowModifierKeys(bAllowModifierKeys)
		.AllowGamepadKeys(bAllowGamepadKeys)
		.EscapeKeys(EscapeKeys)
		.OnKeySelected(BIND_UOBJECT_DELEGATE(SInputKeySelector::FOnKeySelected, HandleKeySelected))
		.OnIsSelectingKeyChanged(BIND_UOBJECT_DELEGATE(SInputKeySelector::FOnIsSelectingKeyChanged, HandleIsSelectingKeyChanged));

	return MyInputKeySelector.ToSharedRef();
}

void URichInputKeySelector::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyInputKeySelector.Reset();
}

void URichInputKeySelector::HandleKeySelected(const FInputChord& InSelectedKey)
{
	if (SelectedKey != InSelectedKey)
	{
		SelectedKey = InSelectedKey;
		BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::SelectedKey);
		OnKeySelected.Broadcast(SelectedKey);
	}
}

void URichInputKeySelector::HandleIsSelectingKeyChanged()
{
	OnIsSelectingKeyChanged.Broadcast();
}

void URichInputKeySelector::SetTextBlockVisibility(const ESlateVisibility InVisibility)
{
	if (MyInputKeySelector.IsValid() == true)
	{
		const EVisibility SlateVisibility = UWidget::ConvertSerializedVisibilityToRuntime(InVisibility);
		MyInputKeySelector->SetTextBlockVisibility(SlateVisibility);
	}
}

#undef LOCTEXT_NAMESPACE
