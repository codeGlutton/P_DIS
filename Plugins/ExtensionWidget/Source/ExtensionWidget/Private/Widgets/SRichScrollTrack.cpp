#include "Widgets/SRichScrollTrack.h"

#include "Widgets/Layout/SBorder.h"

FRichScrollBarArrowConfig::FRichScrollBarArrowConfig() :
	bShowArrowButtons(false),
	ArrowButtonLength(20.0f),
	ArrowPadding(0.0f),
	ClickScrollStep(32.0f),
	bEnableHoldScroll(true),
	HoldScrollSpeed(256.0f),
	HoldInitialDelay(0.35f),
	HoldRepeatInterval(0.016f),
	StandaloneReferenceExtent(500.0f),
	DefaultArrowGlyphColor(FLinearColor::White),
	bUseCustomArrowStyle(false),
	StartArrowStyle(FButtonStyle::GetDefault()),
	EndArrowStyle(FButtonStyle::GetDefault())
{
}

FRichScrollBarConfig::FRichScrollBarConfig() :
	ThumbMode(ERichScrollBarThumbMode::Dynamic),
	FixedThumbLength(48.0f),
	MinDynamicThumbLength(16.0f),
	ArrowConfig()
{
}

SRichScrollTrack::SRichScrollTrack() :
	Children(this),
	Orientation(Orient_Vertical),
	Config(),
	ActualOffsetFraction(0.0f),
	ActualThumbSizeFraction(1.0f)
{
}

void SRichScrollTrack::Construct(const FArguments& InArgs)
{
	Orientation = InArgs._Orientation;
	Config = InArgs._Config;

	// SScrollBarTrack과 동일하게 3개 슬롯(TopTrack, Thumb, BottomTrack) 순서로 생성
	FTrackSlot::FSlotArguments TopArgs(MakeUnique<FTrackSlot>());
	TopArgs.AttachWidget(
		SNew(SBorder)
		.Padding(0.0f)
		.BorderImage(InArgs._TopSlotImage)
	);
	Children.AddSlot(MoveTemp(TopArgs));

	FTrackSlot::FSlotArguments ThumbArgs(MakeUnique<FTrackSlot>());
	ThumbArgs.AttachWidget(
		SNew(SBorder)
		.Padding(0.0f)
		.BorderImage(InArgs._ThumbImage)
	);
	Children.AddSlot(MoveTemp(ThumbArgs));

	FTrackSlot::FSlotArguments BottomArgs(MakeUnique<FTrackSlot>());
	BottomArgs.AttachWidget(
		SNew(SBorder)
		.Padding(0.0f)
		.BorderImage(InArgs._BottomSlotImage)
	);
	Children.AddSlot(MoveTemp(BottomArgs));
}

void SRichScrollTrack::SetSizes(float InOffsetFraction, float InThumbSizeFraction)
{
	const float ClampedThumb = FMath::Clamp(InThumbSizeFraction, 0.0f, 1.0f);
	const float MaxOffset = FMath::Max(0.0f, 1.0f - ClampedThumb);
	const float ClampedOffset = FMath::Clamp(InOffsetFraction, 0.0f, MaxOffset);

	ActualThumbSizeFraction = ClampedThumb;
	ActualOffsetFraction = ClampedOffset;
}

void SRichScrollTrack::SetConfig(const FRichScrollBarConfig& InConfig)
{
	Config = InConfig;
}

void SRichScrollTrack::SetOrientation(EOrientation InOrientation)
{
	Orientation = InOrientation;
}

void SRichScrollTrack::SetImages(
	const TAttribute<const FSlateBrush*>& InTopSlotImage,
	const TAttribute<const FSlateBrush*>& InThumbImage,
	const TAttribute<const FSlateBrush*>& InBottomSlotImage)
{
	if (Children.Num() < 3)
	{
		return;
	}

	StaticCastSharedRef<SBorder>(Children[0].GetWidget())->SetBorderImage(InTopSlotImage);
	StaticCastSharedRef<SBorder>(Children[1].GetWidget())->SetBorderImage(InThumbImage);
	StaticCastSharedRef<SBorder>(Children[2].GetWidget())->SetBorderImage(InBottomSlotImage);
}

bool SRichScrollTrack::IsNeeded() const
{
	return ActualThumbSizeFraction < (1.0f - UE_KINDA_SMALL_NUMBER);
}

float SRichScrollTrack::DistanceFromTop() const
{
	return ActualOffsetFraction;
}

float SRichScrollTrack::DistanceFromBottom() const
{
	return FMath::Max(0.0f, 1.0f - (ActualOffsetFraction + ActualThumbSizeFraction));
}

float SRichScrollTrack::GetOffsetFraction() const
{
	return ActualOffsetFraction;
}

float SRichScrollTrack::GetThumbSizeFraction() const
{
	return ActualThumbSizeFraction;
}

float SRichScrollTrack::GetMinThumbFraction(float TrackLength) const
{
	if (TrackLength <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float MinPx = FMath::Max(1.0f, Config.MinDynamicThumbLength);
	return FMath::Clamp(MinPx / TrackLength, 0.0f, 1.0f);
}

float SRichScrollTrack::GetVisualThumbFraction(float TrackLength) const
{
	// 스크롤이 불필요하면 썸이 트랙 전체를 채움
	if (IsNeeded() == false)
	{
		return 1.0f;
	}

	if (TrackLength <= UE_KINDA_SMALL_NUMBER)
	{
		return ActualThumbSizeFraction;
	}

	if (Config.ThumbMode == ERichScrollBarThumbMode::Fixed)
	{
		const float FixedPx = FMath::Max(1.0f, Config.FixedThumbLength);
		return FMath::Clamp(FixedPx / TrackLength, 0.0f, 1.0f);
	}

	const float MinFraction = GetMinThumbFraction(TrackLength);
	return FMath::Clamp(FMath::Max(ActualThumbSizeFraction, MinFraction), 0.0f, 1.0f);
}

float SRichScrollTrack::GetTrackLength(const FGeometry& TrackGeometry) const
{
	const FVector2D LocalSize = TrackGeometry.GetLocalSize();
	if (Orientation == Orient_Vertical)
	{
		return FMath::Max(0.0f, static_cast<float>(LocalSize.Y));
	}

	return FMath::Max(0.0f, static_cast<float>(LocalSize.X));
}

void SRichScrollTrack::GetThumbBounds(const FGeometry& TrackGeometry, float& OutThumbStartPx, float& OutThumbLengthPx) const
{
	const float TrackLength = GetTrackLength(TrackGeometry);
	if (TrackLength <= UE_KINDA_SMALL_NUMBER)
	{
		OutThumbStartPx = 0.0f;
		OutThumbLengthPx = 0.0f;
		return;
	}

	const float VisualThumbFraction = GetVisualThumbFraction(TrackLength);
	const float NormalizedProgress = ComputeNormalizedProgress();
	const float VisualTopFraction = NormalizedProgress * FMath::Max(0.0f, 1.0f - VisualThumbFraction);

	OutThumbStartPx = VisualTopFraction * TrackLength;
	OutThumbLengthPx = VisualThumbFraction * TrackLength;
}

float SRichScrollTrack::ConvertTrackDragToOffsetFraction(
	float CursorLocalAxisPx,
	float GrabOffsetOnThumbFraction,
	float TrackLength) const
{
	const float MaxOffset = ComputeMaxOffsetFraction();
	if (MaxOffset <= UE_KINDA_SMALL_NUMBER || TrackLength <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float VisualThumbFraction = GetVisualThumbFraction(TrackLength);
	const float VisualThumbLengthPx = VisualThumbFraction * TrackLength;
	const float MovableTrackPx = FMath::Max(0.0f, TrackLength - VisualThumbLengthPx);

	if (MovableTrackPx <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	// 커서 위치에서 잡은 지점만큼 보정해 썸 시작 좌표를 구한 뒤 정규화 진행률(0~1)로 환산
	const float ClampedGrabFraction = FMath::Clamp(GrabOffsetOnThumbFraction, 0.0f, 1.0f);
	const float VisualThumbStartPx = CursorLocalAxisPx - (ClampedGrabFraction * VisualThumbLengthPx);
	const float NewProgress = FMath::Clamp(VisualThumbStartPx / MovableTrackPx, 0.0f, 1.0f);

	return NewProgress * MaxOffset;
}

void SRichScrollTrack::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	if (Children.Num() < 3)
	{
		return;
	}

	const FVector2D TotalSize = AllottedGeometry.GetLocalSize();
	const float TrackLength = GetTrackLength(AllottedGeometry);
	const float VisualThumbFraction = GetVisualThumbFraction(TrackLength);
	const float NormalizedProgress = ComputeNormalizedProgress();

	const float RemainingFraction = FMath::Max(0.0f, 1.0f - VisualThumbFraction);
	const float TopFraction = NormalizedProgress * RemainingFraction;
	const float BottomFraction = FMath::Max(0.0f, RemainingFraction - TopFraction);

	const float TopPx = TopFraction * TrackLength;
	const float ThumbPx = VisualThumbFraction * TrackLength;
	const float BottomPx = BottomFraction * TrackLength;

	const auto AddChildSlot = [&](int32 SlotIndex, float AxisOffset, float AxisSize)
	{
		const FVector2D ChildOffset = (Orientation == Orient_Vertical)
			? FVector2D(0.0f, AxisOffset)
			: FVector2D(AxisOffset, 0.0f);
		const FVector2D ChildSize = (Orientation == Orient_Vertical)
			? FVector2D(TotalSize.X, AxisSize)
			: FVector2D(AxisSize, TotalSize.Y);

		ArrangedChildren.AddWidget(
			AllottedGeometry.MakeChild(Children[SlotIndex].GetWidget(), ChildOffset, ChildSize)
		);
	};

	AddChildSlot(0, 0.0f, TopPx);
	AddChildSlot(1, TopPx, ThumbPx);
	AddChildSlot(2, TopPx + ThumbPx, BottomPx);
}

FVector2D SRichScrollTrack::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	// SScrollBarTrack과 동일하게 트랙 자체의 최소 요구 크기를 0으로 두어 외부 Thickness에 따르도록 함
	return FVector2D::ZeroVector;
}

FChildren* SRichScrollTrack::GetChildren()
{
	return &Children;
}

float SRichScrollTrack::ComputeNormalizedProgress() const
{
	const float MaxOffset = ComputeMaxOffsetFraction();
	if (MaxOffset <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(ActualOffsetFraction / MaxOffset, 0.0f, 1.0f);
}

float SRichScrollTrack::ComputeMaxOffsetFraction() const
{
	return FMath::Max(0.0f, 1.0f - ActualThumbSizeFraction);
}
