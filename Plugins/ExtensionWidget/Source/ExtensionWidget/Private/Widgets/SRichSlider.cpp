#include "Widgets/SRichSlider.h"

#include "Rendering/DrawElements.h"

SLATE_IMPLEMENT_WIDGET(SRichSlider)
void SRichSlider::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(
		AttributeInitializer,
		"SliderFillBarColor",
		SliderFillBarColorSlateAttribute,
		EInvalidateWidgetReason::Paint
	);
}

SRichSlider::SRichSlider() :
	RichStyle(&FRichSliderStyle::GetDefault()),
	SliderFillBarColorSlateAttribute(*this, FLinearColor::White)
{
}

void SRichSlider::Construct(const FArguments& InArgs)
{
	SSlider::FArguments BaseArgs;
	BaseArgs.IndentHandle(InArgs._IndentHandle);
	BaseArgs.MouseUsesStep(InArgs._MouseUsesStep);
	BaseArgs.RequiresControllerLock(InArgs._RequiresControllerLock);
	BaseArgs.Locked(InArgs._Locked);
	BaseArgs.Orientation(InArgs._Orientation);
	BaseArgs.SliderBarColor(InArgs._SliderBarColor);
	BaseArgs.SliderHandleColor(InArgs._SliderHandleColor);
	BaseArgs.Style(InArgs._Style);
	BaseArgs.StepSize(InArgs._StepSize);
	BaseArgs.Value(InArgs._Value);
	BaseArgs.MinValue(InArgs._MinValue);
	BaseArgs.MaxValue(InArgs._MaxValue);
	BaseArgs.IsFocusable(InArgs._IsFocusable);
	BaseArgs.OnMouseCaptureBegin(InArgs._OnMouseCaptureBegin);
	BaseArgs.OnMouseCaptureEnd(InArgs._OnMouseCaptureEnd);
	BaseArgs.OnControllerCaptureBegin(InArgs._OnControllerCaptureBegin);
	BaseArgs.OnControllerCaptureEnd(InArgs._OnControllerCaptureEnd);
	BaseArgs.OnValueChanged(InArgs._OnValueChanged);
	BaseArgs.PreventThrottling(InArgs._PreventThrottling);

	SSlider::Construct(BaseArgs);

	RichStyle = (InArgs._RichStyle != nullptr) ? InArgs._RichStyle : &FRichSliderStyle::GetDefault();
	SliderFillBarColorSlateAttribute.Assign(*this, InArgs._SliderFillBarColor);
}

void SRichSlider::SetRichStyle(const FRichSliderStyle* InRichStyle)
{
	const FRichSliderStyle* ResolvedStyle = (InRichStyle != nullptr) ? InRichStyle : &FRichSliderStyle::GetDefault();
	if (RichStyle != ResolvedStyle)
	{
		RichStyle = ResolvedStyle;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
	else
	{
		// 동일 구조체 포인터(URichSlider::RichStyle 등) 내부 브러시가 편집된 경우에도 갱신을 보장함
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SRichSlider::SetSliderFillBarColor(TAttribute<FSlateColor> InSliderFillBarColor)
{
	SliderFillBarColorSlateAttribute.Assign(*this, MoveTemp(InSliderFillBarColor));
}

const FSlateBrush* SRichSlider::GetFillBarImage() const
{
	if (RichStyle == nullptr)
	{
		return nullptr;
	}

	if (IsEnabled() == false || IsLocked() == true)
	{
		return &RichStyle->DisabledFillBarImage;
	}

	if (IsHovered() == true)
	{
		return &RichStyle->HoveredFillBarImage;
	}

	return &RichStyle->NormalFillBarImage;
}

int32 SRichSlider::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const
{
	if (Style == nullptr)
	{
		return LayerId;
	}

	const float AllottedWidth = (Orientation == Orient_Horizontal) ? AllottedGeometry.GetLocalSize().X : AllottedGeometry.GetLocalSize().Y;
	const float AllottedHeight = (Orientation == Orient_Horizontal) ? AllottedGeometry.GetLocalSize().Y : AllottedGeometry.GetLocalSize().X;

	const FSlateBrush* BarImage = SSlider::GetBarImage();
	const FSlateBrush* ThumbImage = SSlider::GetThumbImage();
	if (BarImage == nullptr || ThumbImage == nullptr)
	{
		return LayerId;
	}

	const FVector2f HandleSize = ThumbImage->ImageSize;
	const FVector2f HalfHandleSize = 0.5f * HandleSize;
	const float Indentation = (GetIndentHandleAttribute().Get() == true) ? HandleSize.X : 0.0f;

	const float SliderPercent = FMath::Clamp(GetNormalizedValue(), 0.0f, 1.0f);
	const float SliderLength = AllottedWidth - (Indentation + HandleSize.X);
	const float SliderHandleOffset = SliderPercent * SliderLength;
	const float SliderY = 0.5f * AllottedHeight;

	const FVector2f HandleTopLeftPoint(SliderHandleOffset + (0.5f * Indentation), SliderY - HalfHandleSize.Y);
	const float HandleCenterX = HandleTopLeftPoint.X + HalfHandleSize.X;

	const FVector2f SliderStartPoint(HalfHandleSize.X, SliderY);
	const FVector2f SliderEndPoint(AllottedWidth - HalfHandleSize.X, SliderY);

	FGeometry SliderGeometry = AllottedGeometry;
	if (Orientation == Orient_Vertical)
	{
		// 수직 슬라이더는 하단(0)~상단(1) 배치를 위해 SSlider와 동일하게 90도 반시계 회전 Child Geometry 생성
		const FSlateRenderTransform SlateRenderTransform = TransformCast<FSlateRenderTransform>(
			Concatenate(Inverse(FVector2f(AllottedWidth, 0.0f)), FQuat2D(FMath::DegreesToRadians(-90.0f)))
		);
		SliderGeometry = AllottedGeometry.MakeChild(
			FVector2f(AllottedWidth, AllottedHeight),
			FSlateLayoutTransform(),
			SlateRenderTransform,
			FVector2f::ZeroVector
		);
	}

	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect DrawEffects = (bEnabled == true) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	// 1. 배경 트랙(Empty Bar) 렌더링
	const FVector2f BarTopLeft(SliderStartPoint.X, SliderStartPoint.Y - Style->BarThickness * 0.5f);
	const FVector2f BarSize(FMath::Max(0.0f, SliderEndPoint.X - SliderStartPoint.X), FMath::Max(0.0f, Style->BarThickness));
	if (BarSize.X > 0.0f && BarSize.Y > 0.0f)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			SliderGeometry.ToPaintGeometry(BarSize, FSlateLayoutTransform(BarTopLeft)),
			BarImage,
			DrawEffects,
			BarImage->GetTint(InWidgetStyle) * GetSliderBarColorAttribute().Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint()
		);
	}

	++LayerId;

	// 2. 채움 트랙(Fill Bar) 렌더링 — SProgressBar의 PushTransformedClip 기법으로 항상 핸들 정중앙 뒤까지 클리핑
	const FSlateBrush* FillBarImage = GetFillBarImage();
	if (FillBarImage != nullptr && FillBarImage->DrawAs != ESlateBrushDrawType::NoDrawType && SliderPercent > 0.0f && BarSize.X > 0.0f && BarSize.Y > 0.0f)
	{
		const FMargin& FillPadding = (RichStyle != nullptr) ? RichStyle->FillBarBorderPadding : FMargin(0.0f);
		const float FillLeft = BarTopLeft.X + FillPadding.Left;
		const float FillTop = BarTopLeft.Y + FillPadding.Top;
		const float FillRight = (BarTopLeft.X + BarSize.X) - FillPadding.Right;
		const float FillBottom = (BarTopLeft.Y + BarSize.Y) - FillPadding.Bottom;

		const FVector2f FillBoxSize(FillRight - FillLeft, FillBottom - FillTop);
		if (FillBoxSize.X > 0.0f && FillBoxSize.Y > 0.0f)
		{
			const float ClipRight = FMath::Clamp(HandleCenterX, FillLeft, FillRight);
			if (ClipRight > FillLeft)
			{
				const FSlateRenderTransform& Transform = SliderGeometry.GetAccumulatedRenderTransform();
				const FSlateClippingZone ClippingZone(
					Transform.TransformPoint(FVector2f(FillLeft, FillTop)),
					Transform.TransformPoint(FVector2f(ClipRight, FillTop)),
					Transform.TransformPoint(FVector2f(FillLeft, FillBottom)),
					Transform.TransformPoint(FVector2f(ClipRight, FillBottom))
				);

				if (ClippingZone.HasZeroArea() == false)
				{
					OutDrawElements.PushClip(ClippingZone);

					FSlateDrawElement::MakeBox(
						OutDrawElements,
						LayerId,
						SliderGeometry.ToPaintGeometry(FillBoxSize, FSlateLayoutTransform(FVector2f(FillLeft, FillTop))),
						FillBarImage,
						DrawEffects,
						FillBarImage->GetTint(InWidgetStyle) * SliderFillBarColorSlateAttribute.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint()
					);

					OutDrawElements.PopClip();
				}
			}
		}
	}

	++LayerId;

	// 3. 핸들(Thumb) 렌더링 — 채움 끝점 이음새를 자연스럽게 덮도록 최상위 레이어에 배치
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		SliderGeometry.ToPaintGeometry(HandleSize, FSlateLayoutTransform(HandleTopLeftPoint)),
		ThumbImage,
		DrawEffects,
		ThumbImage->GetTint(InWidgetStyle) * GetSliderHandleColorAttribute().Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint()
	);

	return LayerId;
}
