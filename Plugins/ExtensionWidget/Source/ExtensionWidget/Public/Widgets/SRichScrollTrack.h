/*****************************************************************//**
 * @file   SRichScrollTrack.h
 * @brief  동적/고정 썸 크기를 모두 지원하는 3-슬롯 SPanel 기반 스크롤 트랙 위젯
 * @details 언리얼 SScrollBarTrack과 동일한 SPanel 3-슬롯(Top/Thumb/Bottom) 설계를 따르면서,
 *          논리적 ActualOffsetFraction/ActualThumbSizeFraction과 시각적 썸 비율을 분리 계산함.
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Children.h"
#include "SlotBase.h"
#include "Types/RichScrollBarTypes.h"
#include "Widgets/SPanel.h"

/* 독립 전방 선언 */
struct FSlateBrush;

/**
 * TopTrack / Thumb / BottomTrack 3개의 슬롯을 배치하는 SPanel 파생 스크롤 트랙
 */
class EXTENSIONWIDGET_API SRichScrollTrack : public SPanel
{
	/* 슬롯 정의 */
public:
	class FTrackSlot : public TSlotBase<FTrackSlot>
	{
	public:
		FTrackSlot() = default;
	};

	SLATE_BEGIN_ARGS(SRichScrollTrack)
		: _TopSlotImage(nullptr)
		, _ThumbImage(nullptr)
		, _BottomSlotImage(nullptr)
		, _Orientation(Orient_Vertical)
		, _Config(FRichScrollBarConfig())
	{
	}
		SLATE_ATTRIBUTE(const FSlateBrush*, TopSlotImage)
		SLATE_ATTRIBUTE(const FSlateBrush*, ThumbImage)
		SLATE_ATTRIBUTE(const FSlateBrush*, BottomSlotImage)
		SLATE_ARGUMENT(EOrientation, Orientation)
		SLATE_ARGUMENT(FRichScrollBarConfig, Config)
	SLATE_END_ARGS()

	/* 생성자 및 위젯 초기화 */
public:
	SRichScrollTrack();
	void Construct(const FArguments& InArgs);

	/* 스크롤 상태 및 설정 제어 */
public:
	/* 논리적 오프셋과 썸 비율(0~1)을 갱신함 */
	void SetSizes(float InOffsetFraction, float InThumbSizeFraction);

	/* 동적/고정 썸 등 트랙 설정을 갱신함 */
	void SetConfig(const FRichScrollBarConfig& InConfig);

	/* 스크롤 축 방향을 변경함 */
	void SetOrientation(EOrientation InOrientation);

	/* 트랙 및 썸 브러시 애트리뷰트를 갱신함 */
	void SetImages(
		const TAttribute<const FSlateBrush*>& InTopSlotImage,
		const TAttribute<const FSlateBrush*>& InThumbImage,
		const TAttribute<const FSlateBrush*>& InBottomSlotImage);

	/* 상태 질의 및 좌표 변환 */
public:
	bool IsNeeded() const;
	float DistanceFromTop() const;
	float DistanceFromBottom() const;
	float GetOffsetFraction() const;
	float GetThumbSizeFraction() const;
	float GetMinThumbFraction(float TrackLength) const;
	float GetVisualThumbFraction(float TrackLength) const;
	float GetTrackLength(const FGeometry& TrackGeometry) const;
	void GetThumbBounds(const FGeometry& TrackGeometry, float& OutThumbStartPx, float& OutThumbLengthPx) const;
	float ConvertTrackDragToOffsetFraction(float CursorLocalAxisPx, float GrabOffsetOnThumbFraction, float TrackLength) const;

	/* SPanel / SWidget 인터페이스 */
public:
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual FChildren* GetChildren() override;

	/* 내부 헬퍼 */
private:
	float ComputeNormalizedProgress() const;
	float ComputeMaxOffsetFraction() const;

	/* 멤버 변수 */
private:
	TPanelChildren<FTrackSlot> Children;
	EOrientation Orientation;
	FRichScrollBarConfig Config;
	float ActualOffsetFraction;
	float ActualThumbSizeFraction;
};
