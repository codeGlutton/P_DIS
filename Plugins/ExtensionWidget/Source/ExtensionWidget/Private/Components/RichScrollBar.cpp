#include "Components/RichScrollBar.h"

#include "Styling/CoreStyle.h"
#include "Widgets/SRichScrollBar.h"

#define LOCTEXT_NAMESPACE "ExtensionWidget"

URichScrollBar::URichScrollBar(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	WidgetStyle(FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("Scrollbar")),
	bAlwaysShowScrollbar(false),
	Orientation(Orient_Vertical),
	Thickness(16.0f, 16.0f),
	Padding(2.0f),
	RichConfig()
{
}

void URichScrollBar::SetState(float InOffsetFraction, float InThumbSizeFraction)
{
	if (MyRichScrollBar.IsValid() == true)
	{
		MyRichScrollBar->SetState(InOffsetFraction, InThumbSizeFraction, false);
	}
}

void URichScrollBar::SetRichConfig(const FRichScrollBarConfig& InConfig)
{
	RichConfig = InConfig;
	if (MyRichScrollBar.IsValid() == true)
	{
		MyRichScrollBar->SetConfig(RichConfig);
	}
}

void URichScrollBar::SetAlwaysShowScrollbar(bool bInAlwaysShow)
{
	bAlwaysShowScrollbar = bInAlwaysShow;
	if (MyRichScrollBar.IsValid() == true)
	{
		MyRichScrollBar->SetAlwaysShowScrollbar(bAlwaysShowScrollbar);
	}
}

void URichScrollBar::SetOrientation(EOrientation InOrientation)
{
	Orientation = InOrientation;
	if (MyRichScrollBar.IsValid() == true)
	{
		MyRichScrollBar->SetOrientation(Orientation.GetValue());
	}
}

void URichScrollBar::SetThickness(const FVector2D& InThickness)
{
	Thickness = InThickness;
	if (MyRichScrollBar.IsValid() == true)
	{
		MyRichScrollBar->SetThickness(Thickness);
	}
}

void URichScrollBar::SetPadding(const FMargin& InPadding)
{
	Padding = InPadding;
	if (MyRichScrollBar.IsValid() == true)
	{
		MyRichScrollBar->SetTrackPadding(Padding);
	}
}

float URichScrollBar::GetScrollOffsetFraction() const
{
	if (MyRichScrollBar.IsValid() == true)
	{
		return MyRichScrollBar->GetOffsetFraction();
	}

	return 0.0f;
}

float URichScrollBar::GetThumbSizeFraction() const
{
	if (MyRichScrollBar.IsValid() == true)
	{
		return MyRichScrollBar->GetThumbSizeFraction();
	}

	return 1.0f;
}

bool URichScrollBar::IsScrollNeeded() const
{
	if (MyRichScrollBar.IsValid() == true)
	{
		return MyRichScrollBar->IsNeeded();
	}

	return false;
}

TSharedPtr<SRichScrollBar> URichScrollBar::GetSlateRichScrollBar() const
{
	return MyRichScrollBar;
}

namespace
{
	EVisibility ConvertSlateVisibilityToRaw(ESlateVisibility InVisibility)
	{
		switch (InVisibility)
		{
		case ESlateVisibility::Visible:
			return EVisibility::Visible;
		case ESlateVisibility::Collapsed:
			return EVisibility::Collapsed;
		case ESlateVisibility::Hidden:
			return EVisibility::Hidden;
		case ESlateVisibility::HitTestInvisible:
			return EVisibility::HitTestInvisible;
		case ESlateVisibility::SelfHitTestInvisible:
		default:
			return EVisibility::SelfHitTestInvisible;
		}
	}
}

void URichScrollBar::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);

	if (MyRichScrollBar.IsValid() == true)
	{
		MyRichScrollBar->SetUserVisibility(ConvertSlateVisibilityToRaw(InVisibility));
	}
}

void URichScrollBar::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyRichScrollBar.IsValid() == false)
	{
		return;
	}

	MyRichScrollBar->SetUserVisibility(ConvertSlateVisibilityToRaw(GetVisibility()));
	MyRichScrollBar->SetStyle(&WidgetStyle);
	MyRichScrollBar->SetOrientation(Orientation.GetValue());
	MyRichScrollBar->SetAlwaysShowScrollbar(bAlwaysShowScrollbar);
	MyRichScrollBar->SetThickness(Thickness);
	MyRichScrollBar->SetTrackPadding(Padding);
	MyRichScrollBar->SetConfig(RichConfig);
}

void URichScrollBar::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyRichScrollBar.Reset();
}

#if WITH_EDITOR
const FText URichScrollBar::GetPaletteCategory()
{
	return LOCTEXT("ExtensionWidgetPaletteCategory", "Extension Widget");
}
#endif

TSharedRef<SWidget> URichScrollBar::RebuildWidget()
{
	MyRichScrollBar = SNew(SRichScrollBar)
		.Style(&WidgetStyle)
		.Orientation(Orientation.GetValue())
		.AlwaysShowScrollbar(bAlwaysShowScrollbar)
		.Thickness(Thickness)
		.Padding(Padding)
		.Config(RichConfig)
		.OnUserScrolled(FOnUserScrolled::CreateUObject(this, &URichScrollBar::HandleSlateUserScrolled));

	return MyRichScrollBar.ToSharedRef();
}

void URichScrollBar::HandleSlateUserScrolled(float InNewOffsetFraction)
{
	OnUserScrolled.Broadcast(InNewOffsetFraction);
}

#undef LOCTEXT_NAMESPACE
