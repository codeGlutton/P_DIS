#include "Widgets/SRichScrollBar.h"

#include "Rendering/DrawElements.h"
#include "Types/SlateStructs.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SRichScrollTrack.h"

/**
 * 스크롤바 양단 전용 화살표 버튼 위젯 (마우스 캡처 및 상태 전환을 결정론적으로 제어함)
 */
class SRichArrowButton : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SRichArrowButton)
		: _Orientation(Orient_Vertical)
		, _bIsStartArrow(true)
		, _Config(FRichScrollBarConfig())
	{
	}
		SLATE_ARGUMENT(EOrientation, Orientation)
		SLATE_ARGUMENT(bool, bIsStartArrow)
		SLATE_ARGUMENT(FRichScrollBarConfig, Config)
		SLATE_EVENT(FSimpleDelegate, OnPressed)
		SLATE_EVENT(FSimpleDelegate, OnReleasedOrLost)
	SLATE_END_ARGS()

public:
	SRichArrowButton() :
		Orientation(Orient_Vertical),
		bIsStartArrow(true),
		Config(),
		bIsPressed(false),
		bIsHovered(false)
	{
	}

	void Construct(const FArguments& InArgs)
	{
		Orientation = InArgs._Orientation;
		bIsStartArrow = InArgs._bIsStartArrow;
		Config = InArgs._Config;
		OnPressedDelegate = InArgs._OnPressed;
		OnReleasedOrLostDelegate = InArgs._OnReleasedOrLost;

		SBorder::Construct(
			SBorder::FArguments()
			.BorderImage(this, &SRichArrowButton::GetCurrentBrush)
			.Padding(0.0f)
		);
	}

	void SetOrientation(EOrientation InOrientation)
	{
		Orientation = InOrientation;
	}

	void SetConfig(const FRichScrollBarConfig& InConfig)
	{
		Config = InConfig;
	}

	bool IsPressed() const
	{
		return bIsPressed;
	}

public:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			bIsPressed = true;
			OnPressedDelegate.ExecuteIfBound();
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed == true)
		{
			ReleaseHoldState();
			return FReply::Handled().ReleaseMouseCapture();
		}

		return FReply::Unhandled();
	}

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		bIsHovered = true;
		SBorder::OnMouseEnter(MyGeometry, MouseEvent);
	}

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		bIsHovered = false;
		SBorder::OnMouseLeave(MouseEvent);
	}

	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override
	{
		ReleaseHoldState();
		SBorder::OnMouseCaptureLost(CaptureLostEvent);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		int32 OutLayerId = SBorder::OnPaint(
			Args,
			AllottedGeometry,
			MyCullingRect,
			OutDrawElements,
			LayerId,
			InWidgetStyle,
			bParentEnabled
		);

		// 커스텀 버튼 브러시가 없는 기본 상태에서는 방향 벡터 화살표 심볼을 그려 즉시 식별 가능하게 함
		if (Config.ArrowConfig.bUseCustomArrowStyle == false)
		{
			DrawDefaultArrowGlyph(AllottedGeometry, OutDrawElements, OutLayerId + 1, InWidgetStyle);
			OutLayerId += 1;
		}

		return OutLayerId;
	}

private:
	void ReleaseHoldState()
	{
		if (bIsPressed == true)
		{
			bIsPressed = false;
			OnReleasedOrLostDelegate.ExecuteIfBound();
		}
	}

	const FSlateBrush* GetCurrentBrush() const
	{
		if (Config.ArrowConfig.bUseCustomArrowStyle == true)
		{
			const FButtonStyle& TargetStyle = (bIsStartArrow == true)
				? Config.ArrowConfig.StartArrowStyle
				: Config.ArrowConfig.EndArrowStyle;

			if (bIsPressed == true)
			{
				return &TargetStyle.Pressed;
			}
			if (bIsHovered == true)
			{
				return &TargetStyle.Hovered;
			}
			return &TargetStyle.Normal;
		}

		// 기본 폴백 브러시: 눌림·호버 상태를 배경 명도로 구분
		if (bIsPressed == true)
		{
			return FCoreStyle::Get().GetBrush("GenericWhiteBox");
		}
		if (bIsHovered == true)
		{
			return FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder");
		}
		return FCoreStyle::Get().GetBrush("ToolPanel.DarkGroupBorder");
	}

	void DrawDefaultArrowGlyph(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 InLayerId,
		const FWidgetStyle& InWidgetStyle) const
	{
		const FVector2f Size(
			static_cast<float>(AllottedGeometry.GetLocalSize().X),
			static_cast<float>(AllottedGeometry.GetLocalSize().Y)
		);
		const float MinExtent = FMath::Min(Size.X, Size.Y);
		if (MinExtent <= 4.0f)
		{
			return;
		}

		const FVector2f Center = Size * 0.5f;
		const float HalfSpan = FMath::Clamp(MinExtent * 0.25f, 2.0f, 7.0f);

		TArray<FVector2f> Points;
		Points.Reserve(3);

		if (Orientation == Orient_Vertical)
		{
			const float TipSign = (bIsStartArrow == true) ? -1.0f : 1.0f;
			Points.Add(FVector2f(Center.X - HalfSpan, Center.Y - TipSign * (HalfSpan * 0.55f)));
			Points.Add(FVector2f(Center.X, Center.Y + TipSign * (HalfSpan * 0.65f)));
			Points.Add(FVector2f(Center.X + HalfSpan, Center.Y - TipSign * (HalfSpan * 0.55f)));
		}
		else
		{
			const float TipSign = (bIsStartArrow == true) ? -1.0f : 1.0f;
			Points.Add(FVector2f(Center.X - TipSign * (HalfSpan * 0.55f), Center.Y - HalfSpan));
			Points.Add(FVector2f(Center.X + TipSign * (HalfSpan * 0.65f), Center.Y));
			Points.Add(FVector2f(Center.X - TipSign * (HalfSpan * 0.55f), Center.Y + HalfSpan));
		}

		const FLinearColor Tint = Config.ArrowConfig.DefaultArrowGlyphColor * InWidgetStyle.GetColorAndOpacityTint();
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			InLayerId,
			AllottedGeometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			Tint,
			true,
			1.5f
		);
	}

private:
	EOrientation Orientation;
	bool bIsStartArrow;
	FRichScrollBarConfig Config;
	bool bIsPressed;
	bool bIsHovered;
	FSimpleDelegate OnPressedDelegate;
	FSimpleDelegate OnReleasedOrLostDelegate;
};

SRichScrollBar::SRichScrollBar() :
	ScrollBarStyle(&FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("Scrollbar")),
	Orientation(Orient_Vertical),
	bAlwaysShowScrollbar(false),
	UserVisibility(EVisibility::Visible),
	Config(),
	bDraggingThumb(false),
	bIsThumbHovered(false),
	DragGrabThumbFraction(0.5f),
	bIsHoldingArrow(false),
	ActiveHoldDirection(1),
	HoldElapsedSincePress(0.0f),
	HoldAccumulatorSinceLastRepeat(0.0f)
{
}

SRichScrollBar::~SRichScrollBar()
{
	StopArrowHoldTimer();
}

void SRichScrollBar::Construct(const FArguments& InArgs)
{
	ScrollBarStyle = (InArgs._Style != nullptr)
		? InArgs._Style
		: &FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("Scrollbar");
	Orientation = InArgs._Orientation;
	bAlwaysShowScrollbar = InArgs._AlwaysShowScrollbar;
	ThicknessAttribute = InArgs._Thickness;
	Config = InArgs._Config;
	OnUserScrolled = InArgs._OnUserScrolled;
	OnArrowScrolled = InArgs._OnArrowScrolled;

	SBorder::Construct(
		SBorder::FArguments()
		.BorderImage(FCoreStyle::Get().GetBrush("NoBrush"))
		.Padding(0.0f)
	);

	// 트랙(스크롤바 본체)과 양단 화살표 버튼을 생성한 뒤 [화살표 / 스크롤바 / 화살표] 박스로 조립
	Track = SNew(SRichScrollTrack)
		.Orientation(Orientation)
		.Config(Config)
		.TopSlotImage(this, &SRichScrollBar::GetTopSlotBrush)
		.ThumbImage(this, &SRichScrollBar::GetThumbBrush)
		.BottomSlotImage(this, &SRichScrollBar::GetBottomSlotBrush);

	TrackBorder = SNew(SBorder)
		.BorderImage(this, &SRichScrollBar::GetTrackBackgroundBrush)
		.Padding(InArgs._Padding)
		[
			Track.ToSharedRef()
		];

	StartArrowButton = SNew(SRichArrowButton)
		.Orientation(Orientation)
		.bIsStartArrow(true)
		.Config(Config)
		.OnPressed(FSimpleDelegate::CreateSP(this, &SRichScrollBar::HandleArrowButtonPressed, -1))
		.OnReleasedOrLost(FSimpleDelegate::CreateSP(this, &SRichScrollBar::HandleArrowButtonReleasedOrLost));

	EndArrowButton = SNew(SRichArrowButton)
		.Orientation(Orientation)
		.bIsStartArrow(false)
		.Config(Config)
		.OnPressed(FSimpleDelegate::CreateSP(this, &SRichScrollBar::HandleArrowButtonPressed, 1))
		.OnReleasedOrLost(FSimpleDelegate::CreateSP(this, &SRichScrollBar::HandleArrowButtonReleasedOrLost));

	RebuildOrientationLayout();
	RefreshVisibility();
}

void SRichScrollBar::SetState(float InOffsetFraction, float InThumbSizeFraction, bool bCallOnUserScrolled)
{
	if (Track.IsValid() == false)
	{
		return;
	}

	Track->SetSizes(InOffsetFraction, InThumbSizeFraction);
	RefreshVisibility();

	if (bCallOnUserScrolled == true)
	{
		OnUserScrolled.ExecuteIfBound(Track->GetOffsetFraction());
	}
}

void SRichScrollBar::SetOnUserScrolled(const FOnUserScrolled& InHandler)
{
	OnUserScrolled = InHandler;
}

void SRichScrollBar::SetOnArrowScrolled(const FOnRichArrowScrolled& InHandler)
{
	OnArrowScrolled = InHandler;
}

void SRichScrollBar::SetUserVisibility(TAttribute<EVisibility> InUserVisibility)
{
	UserVisibility = MoveTemp(InUserVisibility);
	RefreshVisibility();
}

void SRichScrollBar::SetThickness(TAttribute<FVector2D> InThickness)
{
	ThicknessAttribute = MoveTemp(InThickness);
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SRichScrollBar::ClearThicknessOverride()
{
	ThicknessAttribute = TAttribute<FVector2D>();
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SRichScrollBar::SetTrackPadding(TAttribute<FMargin> InPadding)
{
	if (TrackBorder.IsValid() == true)
	{
		TrackBorder->SetPadding(MoveTemp(InPadding));
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SRichScrollBar::SetStyle(const FScrollBarStyle* InStyle)
{
	ScrollBarStyle = (InStyle != nullptr)
		? InStyle
		: &FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("Scrollbar");
	Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
}

void SRichScrollBar::SetOrientation(EOrientation InOrientation)
{
	if (Orientation == InOrientation)
	{
		return;
	}

	Orientation = InOrientation;
	if (Track.IsValid() == true)
	{
		Track->SetOrientation(Orientation);
	}
	if (StartArrowButton.IsValid() == true)
	{
		StartArrowButton->SetOrientation(Orientation);
	}
	if (EndArrowButton.IsValid() == true)
	{
		EndArrowButton->SetOrientation(Orientation);
	}

	RebuildOrientationLayout();
}

void SRichScrollBar::SetAlwaysShowScrollbar(bool bInAlwaysShow)
{
	bAlwaysShowScrollbar = bInAlwaysShow;
	RefreshVisibility();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SRichScrollBar::SetConfig(const FRichScrollBarConfig& InConfig)
{
	Config = InConfig;

	if (Track.IsValid() == true)
	{
		Track->SetConfig(Config);
	}
	if (StartArrowButton.IsValid() == true)
	{
		StartArrowButton->SetConfig(Config);
	}
	if (EndArrowButton.IsValid() == true)
	{
		EndArrowButton->SetConfig(Config);
	}

	// 화살표 크기 박스의 축 길이를 최신 설정에 맞게 갱신
	const float ArrowLength = FMath::Max(4.0f, Config.ArrowConfig.ArrowButtonLength);
	const auto ApplyArrowSize = [&](const TSharedPtr<SBox>& SizeBox)
	{
		if (SizeBox.IsValid() == false)
		{
			return;
		}

		if (Orientation == Orient_Vertical)
		{
			SizeBox->SetWidthOverride(FOptionalSize());
			SizeBox->SetHeightOverride(ArrowLength);
		}
		else
		{
			SizeBox->SetWidthOverride(ArrowLength);
			SizeBox->SetHeightOverride(FOptionalSize());
		}
	};

	ApplyArrowSize(StartArrowSizeBox);
	ApplyArrowSize(EndArrowSizeBox);

	if (Config.ArrowConfig.bShowArrowButtons == false || Config.ArrowConfig.bEnableHoldScroll == false)
	{
		EndArrowHold();
	}

	RefreshVisibility();
	Invalidate(EInvalidateWidgetReason::Layout);
}

bool SRichScrollBar::IsNeeded() const
{
	if (Track.IsValid() == false)
	{
		return false;
	}

	return Track->IsNeeded();
}

float SRichScrollBar::DistanceFromTop() const
{
	if (Track.IsValid() == false)
	{
		return 0.0f;
	}

	return Track->DistanceFromTop();
}

float SRichScrollBar::DistanceFromBottom() const
{
	if (Track.IsValid() == false)
	{
		return 0.0f;
	}

	return Track->DistanceFromBottom();
}

bool SRichScrollBar::AlwaysShowScrollbar() const
{
	return bAlwaysShowScrollbar;
}

float SRichScrollBar::GetOffsetFraction() const
{
	if (Track.IsValid() == false)
	{
		return 0.0f;
	}

	return Track->GetOffsetFraction();
}

float SRichScrollBar::GetThumbSizeFraction() const
{
	if (Track.IsValid() == false)
	{
		return 1.0f;
	}

	return Track->GetThumbSizeFraction();
}

bool SRichScrollBar::IsDraggingThumb() const
{
	return bDraggingThumb;
}

bool SRichScrollBar::IsHoldingArrow() const
{
	return bIsHoldingArrow;
}

const FRichScrollBarConfig& SRichScrollBar::GetConfig() const
{
	return Config;
}

void SRichScrollBar::BeginArrowHold(int32 InDirection)
{
	HandleArrowButtonPressed(InDirection);
}

void SRichScrollBar::EndArrowHold()
{
	HandleArrowButtonReleasedOrLost();
}

void SRichScrollBar::StepArrowScroll(int32 InDirection, float InStepUnits)
{
	if (Track.IsValid() == false || InDirection == 0 || InStepUnits <= 0.0f)
	{
		return;
	}

	const float SignedDirection = (InDirection < 0) ? -1.0f : 1.0f;
	const float SignedUnits = SignedDirection * InStepUnits;

	// 외부 스크롤 컨테이너(ListView 등)가 픽셀 델타 핸들러를 연결했으면 직접 픽셀 스크롤을 위임
	if (OnArrowScrolled.IsBound() == true)
	{
		OnArrowScrolled.Execute(SignedUnits);
		return;
	}

	// 단독 사용 시에는 기준 길이를 이용해 정규화 오프셋을 증감하고 SetState + OnUserScrolled 발행
	const float NormalizedDelta = ConvertUnitsToNormalizedDelta(SignedUnits);
	ExecuteScrollFractionAndNotify(Track->GetOffsetFraction() + NormalizedDelta);
}

FReply SRichScrollBar::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || Track.IsValid() == false || IsNeeded() == false)
	{
		return FReply::Unhandled();
	}

	const FGeometry& TrackGeo = Track->GetCachedGeometry();
	const float TrackLength = Track->GetTrackLength(TrackGeo);
	if (TrackLength <= UE_KINDA_SMALL_NUMBER)
	{
		return FReply::Unhandled();
	}

	const FVector2D TrackLocalPos = TrackGeo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const float CursorAxisPx = (Orientation == Orient_Vertical)
		? static_cast<float>(TrackLocalPos.Y)
		: static_cast<float>(TrackLocalPos.X);

	float ThumbStartPx = 0.0f;
	float ThumbLengthPx = 0.0f;
	Track->GetThumbBounds(TrackGeo, ThumbStartPx, ThumbLengthPx);

	bDraggingThumb = true;

	// 썸 내부를 클릭했으면 잡은 지점 비율을 유지하고, 트랙 빈 영역 클릭이면 썸 중앙을 클릭 지점으로 이동
	if (CursorAxisPx >= ThumbStartPx && CursorAxisPx <= (ThumbStartPx + ThumbLengthPx) && ThumbLengthPx > UE_KINDA_SMALL_NUMBER)
	{
		DragGrabThumbFraction = FMath::Clamp((CursorAxisPx - ThumbStartPx) / ThumbLengthPx, 0.0f, 1.0f);
	}
	else
	{
		DragGrabThumbFraction = 0.5f;
		ExecuteDragFromPointer(MouseEvent);
	}

	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SRichScrollBar::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDraggingThumb == true)
	{
		bDraggingThumb = false;
		RefreshVisibility();
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply SRichScrollBar::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UpdateHoverState(MouseEvent);

	if (bDraggingThumb == true && HasMouseCapture() == true)
	{
		ExecuteDragFromPointer(MouseEvent);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SRichScrollBar::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SBorder::OnMouseEnter(MyGeometry, MouseEvent);
	UpdateHoverState(MouseEvent);
}

void SRichScrollBar::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SBorder::OnMouseLeave(MouseEvent);
	bIsThumbHovered = false;
}

void SRichScrollBar::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bDraggingThumb = false;
	RefreshVisibility();
	SBorder::OnMouseCaptureLost(CaptureLostEvent);
}

FVector2D SRichScrollBar::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	const FVector2D BaseSize = SBorder::ComputeDesiredSize(LayoutScaleMultiplier);
	const float StyleThickness = (ScrollBarStyle != nullptr) ? ScrollBarStyle->Thickness : 16.0f;
	const FVector2D Thickness = (ThicknessAttribute.IsSet() == true)
		? ThicknessAttribute.Get(FVector2D(StyleThickness, StyleThickness))
		: FVector2D(StyleThickness, StyleThickness);

	if (Orientation == Orient_Vertical)
	{
		return FVector2D(FMath::Max(Thickness.X, BaseSize.X), BaseSize.Y);
	}

	return FVector2D(BaseSize.X, FMath::Max(Thickness.Y, BaseSize.Y));
}

void SRichScrollBar::RebuildOrientationLayout()
{
	if (StartArrowButton.IsValid() == false || TrackBorder.IsValid() == false || EndArrowButton.IsValid() == false)
	{
		return;
	}

	const float ArrowLength = FMath::Max(4.0f, Config.ArrowConfig.ArrowButtonLength);
	const TAttribute<FMargin> StartArrowPaddingAttr = TAttribute<FMargin>::CreateSP(this, &SRichScrollBar::GetStartArrowSlotPadding);
	const TAttribute<FMargin> EndArrowPaddingAttr = TAttribute<FMargin>::CreateSP(this, &SRichScrollBar::GetEndArrowSlotPadding);

	SAssignNew(StartArrowSizeBox, SBox)
	.Visibility(this, &SRichScrollBar::ComputeArrowButtonVisibility)
	[
		StartArrowButton.ToSharedRef()
	];

	SAssignNew(EndArrowSizeBox, SBox)
	.Visibility(this, &SRichScrollBar::ComputeArrowButtonVisibility)
	[
		EndArrowButton.ToSharedRef()
	];

	if (Orientation == Orient_Vertical)
	{
		StartArrowSizeBox->SetHeightOverride(ArrowLength);
		EndArrowSizeBox->SetHeightOverride(ArrowLength);

		SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StartArrowPaddingAttr)
			[
				StartArrowSizeBox.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				TrackBorder.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(EndArrowPaddingAttr)
			[
				EndArrowSizeBox.ToSharedRef()
			]
		);
	}
	else
	{
		StartArrowSizeBox->SetWidthOverride(ArrowLength);
		EndArrowSizeBox->SetWidthOverride(ArrowLength);

		SetContent(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(StartArrowPaddingAttr)
			[
				StartArrowSizeBox.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				TrackBorder.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(EndArrowPaddingAttr)
			[
				EndArrowSizeBox.ToSharedRef()
			]
		);
	}
}

FMargin SRichScrollBar::GetStartArrowSlotPadding() const
{
	return Config.ArrowConfig.ArrowPadding;
}

FMargin SRichScrollBar::GetEndArrowSlotPadding() const
{
	const FMargin& PaddingValue = Config.ArrowConfig.ArrowPadding;
	if (Orientation == Orient_Vertical)
	{
		return FMargin(PaddingValue.Left, PaddingValue.Bottom, PaddingValue.Right, PaddingValue.Top);
	}

	return FMargin(PaddingValue.Right, PaddingValue.Top, PaddingValue.Left, PaddingValue.Bottom);
}

const FSlateBrush* SRichScrollBar::GetTrackBackgroundBrush() const
{
	if (ScrollBarStyle == nullptr)
	{
		return FCoreStyle::Get().GetBrush("NoBrush");
	}

	return (Orientation == Orient_Vertical)
		? &ScrollBarStyle->VerticalBackgroundImage
		: &ScrollBarStyle->HorizontalBackgroundImage;
}

const FSlateBrush* SRichScrollBar::GetTopSlotBrush() const
{
	if (ScrollBarStyle == nullptr)
	{
		return FCoreStyle::Get().GetBrush("NoBrush");
	}

	if (IsNeeded() == false && bAlwaysShowScrollbar == false && ShouldPreventAutoCollapse() == false)
	{
		return FCoreStyle::Get().GetBrush("NoBrush");
	}

	return (Orientation == Orient_Vertical)
		? &ScrollBarStyle->VerticalTopSlotImage
		: &ScrollBarStyle->HorizontalTopSlotImage;
}

const FSlateBrush* SRichScrollBar::GetThumbBrush() const
{
	if (ScrollBarStyle == nullptr)
	{
		return FCoreStyle::Get().GetBrush("NoBrush");
	}

	// 스크롤이 불필요해도 상시 표기 또는 홀드 보호 중이면 일반 썸 브러시를 유지해 시각적 인지성을 보장함
	if (IsNeeded() == false)
	{
		if (bAlwaysShowScrollbar == true || ShouldPreventAutoCollapse() == true)
		{
			return &ScrollBarStyle->NormalThumbImage;
		}

		return FCoreStyle::Get().GetBrush("NoBrush");
	}

	if (bDraggingThumb == true)
	{
		return &ScrollBarStyle->DraggedThumbImage;
	}

	if (bIsThumbHovered == true)
	{
		return &ScrollBarStyle->HoveredThumbImage;
	}

	return &ScrollBarStyle->NormalThumbImage;
}

const FSlateBrush* SRichScrollBar::GetBottomSlotBrush() const
{
	if (ScrollBarStyle == nullptr)
	{
		return FCoreStyle::Get().GetBrush("NoBrush");
	}

	if (IsNeeded() == false && bAlwaysShowScrollbar == false && ShouldPreventAutoCollapse() == false)
	{
		return FCoreStyle::Get().GetBrush("NoBrush");
	}

	return (Orientation == Orient_Vertical)
		? &ScrollBarStyle->VerticalBottomSlotImage
		: &ScrollBarStyle->HorizontalBottomSlotImage;
}

EVisibility SRichScrollBar::ComputeScrollbarVisibility() const
{
	const EVisibility DesiredUserVisibility = UserVisibility.Get(EVisibility::Visible);
	if (DesiredUserVisibility == EVisibility::Collapsed || DesiredUserVisibility == EVisibility::Hidden)
	{
		return DesiredUserVisibility;
	}

	if (bAlwaysShowScrollbar == true || IsNeeded() == true)
	{
		return DesiredUserVisibility;
	}

	// 썸 드래그 또는 화살표 버튼 홀드 중에는 마우스를 떼기 전까지 Collapsed로 전환하지 않음
	if (ShouldPreventAutoCollapse() == true)
	{
		return DesiredUserVisibility;
	}

	return EVisibility::Collapsed;
}

EVisibility SRichScrollBar::ComputeArrowButtonVisibility() const
{
	if (Config.ArrowConfig.bShowArrowButtons == false)
	{
		return EVisibility::Collapsed;
	}

	// 트랙과 달리 스크롤이 불필요할 때 상시 트랙만 켜진 상태에서도 화살표는 함께 감추는 정책을 우선 적용 가능
	if (bAlwaysShowScrollbar == false && IsNeeded() == false && ShouldPreventAutoCollapse() == false)
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

void SRichScrollBar::RefreshVisibility()
{
	const EVisibility TargetVisibility = ComputeScrollbarVisibility();
	if (GetVisibility() != TargetVisibility)
	{
		SBorder::SetVisibility(TargetVisibility);
		Invalidate(EInvalidateWidgetReason::Visibility | EInvalidateWidgetReason::Layout);
	}
}

void SRichScrollBar::UpdateHoverState(const FPointerEvent& MouseEvent)
{
	if (Track.IsValid() == false || IsNeeded() == false)
	{
		bIsThumbHovered = false;
		return;
	}

	const FGeometry& TrackGeo = Track->GetCachedGeometry();
	const FVector2D TrackLocalPos = TrackGeo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D TrackSize = TrackGeo.GetLocalSize();

	if (TrackLocalPos.X < 0.0 || TrackLocalPos.Y < 0.0 || TrackLocalPos.X > TrackSize.X || TrackLocalPos.Y > TrackSize.Y)
	{
		bIsThumbHovered = false;
		return;
	}

	const float CursorAxisPx = (Orientation == Orient_Vertical)
		? static_cast<float>(TrackLocalPos.Y)
		: static_cast<float>(TrackLocalPos.X);

	float ThumbStartPx = 0.0f;
	float ThumbLengthPx = 0.0f;
	Track->GetThumbBounds(TrackGeo, ThumbStartPx, ThumbLengthPx);

	bIsThumbHovered = (CursorAxisPx >= ThumbStartPx) && (CursorAxisPx <= (ThumbStartPx + ThumbLengthPx));
}

void SRichScrollBar::ExecuteDragFromPointer(const FPointerEvent& MouseEvent)
{
	if (Track.IsValid() == false)
	{
		return;
	}

	const FGeometry& TrackGeo = Track->GetCachedGeometry();
	const float TrackLength = Track->GetTrackLength(TrackGeo);
	const FVector2D TrackLocalPos = TrackGeo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const float CursorAxisPx = (Orientation == Orient_Vertical)
		? static_cast<float>(TrackLocalPos.Y)
		: static_cast<float>(TrackLocalPos.X);

	const float NewOffsetFraction = Track->ConvertTrackDragToOffsetFraction(
		CursorAxisPx,
		DragGrabThumbFraction,
		TrackLength
	);

	ExecuteScrollFractionAndNotify(NewOffsetFraction);
}

void SRichScrollBar::ExecuteScrollFractionAndNotify(float InOffsetFraction)
{
	if (Track.IsValid() == false)
	{
		return;
	}

	Track->SetSizes(InOffsetFraction, Track->GetThumbSizeFraction());
	OnUserScrolled.ExecuteIfBound(Track->GetOffsetFraction());
}

void SRichScrollBar::HandleArrowButtonPressed(int32 InDirection)
{
	// 기존 홀드 타이머가 남아 있으면 정리하고 새로 시작
	StopArrowHoldTimer();

	const int32 NormalizedDirection = (InDirection < 0) ? -1 : 1;
	ActiveHoldDirection = NormalizedDirection;
	bIsHoldingArrow = true;
	HoldElapsedSincePress = 0.0f;
	HoldAccumulatorSinceLastRepeat = 0.0f;

	// 눌린 순간 즉시 1회 클릭 스텝을 실행
	StepArrowScroll(NormalizedDirection, Config.ArrowConfig.ClickScrollStep);

	if (Config.ArrowConfig.bEnableHoldScroll == true)
	{
		HoldActiveTimerHandle = RegisterActiveTimer(
			0.0f,
			FWidgetActiveTimerDelegate::CreateSP(this, &SRichScrollBar::HandleHoldActiveTimer)
		);
	}
}

void SRichScrollBar::HandleArrowButtonReleasedOrLost()
{
	StopArrowHoldTimer();
	bIsHoldingArrow = false;
	RefreshVisibility();
}

EActiveTimerReturnType SRichScrollBar::HandleHoldActiveTimer(double InCurrentTime, float InDeltaTime)
{
	if (bIsHoldingArrow == false
		|| Config.ArrowConfig.bShowArrowButtons == false
		|| Config.ArrowConfig.bEnableHoldScroll == false)
	{
		HoldActiveTimerHandle.Reset();
		return EActiveTimerReturnType::Stop;
	}

	HoldElapsedSincePress += InDeltaTime;
	if (HoldElapsedSincePress < Config.ArrowConfig.HoldInitialDelay)
	{
		return EActiveTimerReturnType::Continue;
	}

	const float RepeatInterval = FMath::Max(0.0f, Config.ArrowConfig.HoldRepeatInterval);
	if (RepeatInterval <= UE_KINDA_SMALL_NUMBER)
	{
		StepArrowScroll(ActiveHoldDirection, Config.ArrowConfig.HoldScrollSpeed * InDeltaTime);
		return EActiveTimerReturnType::Continue;
	}

	HoldAccumulatorSinceLastRepeat += InDeltaTime;
	while (HoldAccumulatorSinceLastRepeat >= RepeatInterval)
	{
		StepArrowScroll(ActiveHoldDirection, Config.ArrowConfig.HoldScrollSpeed * RepeatInterval);
		HoldAccumulatorSinceLastRepeat -= RepeatInterval;
	}

	return EActiveTimerReturnType::Continue;
}

void SRichScrollBar::StopArrowHoldTimer()
{
	const TSharedPtr<FActiveTimerHandle> PinnedHandle = HoldActiveTimerHandle.Pin();
	if (PinnedHandle.IsValid() == true)
	{
		UnRegisterActiveTimer(PinnedHandle.ToSharedRef());
	}
	HoldActiveTimerHandle.Reset();
}

float SRichScrollBar::ConvertUnitsToNormalizedDelta(float InUnits) const
{
	const float ReferenceExtent = FMath::Max(1.0f, Config.ArrowConfig.StandaloneReferenceExtent);
	return InUnits / ReferenceExtent;
}

bool SRichScrollBar::ShouldPreventAutoCollapse() const
{
	return (bDraggingThumb == true) || (bIsHoldingArrow == true);
}
