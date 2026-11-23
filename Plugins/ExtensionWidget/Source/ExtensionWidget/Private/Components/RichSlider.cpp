#include "Components/RichSlider.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SRichSlider.h"

#define LOCTEXT_NAMESPACE "ExtensionWidget"

URichSlider::URichSlider(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	RichStyle(FRichSliderStyle::GetDefault()),
	SliderFillBarColor(FLinearColor::White)
{
}

void URichSlider::SetRichStyle(const FRichSliderStyle& InStyle)
{
	RichStyle = InStyle;

	const TSharedPtr<SRichSlider> SlateRichSlider = GetSlateRichSlider();
	if (SlateRichSlider.IsValid() == true)
	{
		SlateRichSlider->SetRichStyle(&RichStyle);
	}
}

void URichSlider::SetSliderFillBarColor(FLinearColor InValue)
{
	SliderFillBarColor = InValue;

	const TSharedPtr<SRichSlider> SlateRichSlider = GetSlateRichSlider();
	if (SlateRichSlider.IsValid() == true)
	{
		SlateRichSlider->SetSliderFillBarColor(SliderFillBarColor);
	}
}

TSharedPtr<SRichSlider> URichSlider::GetSlateRichSlider() const
{
	return StaticCastSharedPtr<SRichSlider>(MySlider);
}

void URichSlider::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	const TSharedPtr<SRichSlider> SlateRichSlider = GetSlateRichSlider();
	if (SlateRichSlider.IsValid() == false)
	{
		return;
	}

	// USlider::SynchronizeProperties()가 WidgetStyle 재전달을 수행하지 않으므로 디자이너 편집 반영을 위해 함께 동기화함
	SlateRichSlider->SetStyle(&GetWidgetStyle());
	SlateRichSlider->SetRichStyle(&RichStyle);
	SlateRichSlider->SetSliderFillBarColor(SliderFillBarColor);
}

#if WITH_EDITOR
const FText URichSlider::GetPaletteCategory()
{
	return LOCTEXT("ExtensionWidgetPaletteCategory", "Extension Widget");
}
#endif

TSharedRef<SWidget> URichSlider::RebuildWidget()
{
	const TSharedRef<SRichSlider> CreatedSlider = SNew(SRichSlider)
		.Style(&GetWidgetStyle())
		.RichStyle(&RichStyle)
		.SliderFillBarColor(SliderFillBarColor)
		.IsFocusable(IsFocusable)
		.OnMouseCaptureBegin(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnMouseCaptureBegin))
		.OnMouseCaptureEnd(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnMouseCaptureEnd))
		.OnControllerCaptureBegin(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnControllerCaptureBegin))
		.OnControllerCaptureEnd(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnControllerCaptureEnd))
		.OnValueChanged(BIND_UOBJECT_DELEGATE(FOnFloatValueChanged, HandleOnValueChanged))
		.PreventThrottling(bPreventThrottling);

	MySlider = CreatedSlider;
	return CreatedSlider;
}

#undef LOCTEXT_NAMESPACE
