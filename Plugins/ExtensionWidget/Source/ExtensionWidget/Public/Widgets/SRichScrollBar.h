/*****************************************************************//**
 * @file   SRichScrollBar.h
 * @brief  SBorder에서 직접 파생한 확장 Slate 스크롤바 위젯
 * @details 언리얼 SScrollBar와 동일한 부모(SBorder) 및 인터페이스 규격을 유지하면서
 *          고정/동적 썸 크기, 화살표 버튼, 홀드 연속 스크롤, 일체형 가시성 제어를 지원함.
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Styling/CoreStyle.h"
#include "Types/RichScrollBarTypes.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBar.h"

/* 독립 전방 선언 */
class FActiveTimerHandle;
class SBox;
class SRichArrowButton;
class SRichScrollTrack;
class SWidget;

DECLARE_DELEGATE_OneParam(FOnRichArrowScrolled, float /* SignedScrollDelta */);

/**
 * SBorder 파생 확장 스크롤바 위젯 (SRichScrollBar)
 */
class EXTENSIONWIDGET_API SRichScrollBar : public SBorder
{
	/* Slate 선언 인자 */
public:
	SLATE_BEGIN_ARGS(SRichScrollBar)
		: _Style(&FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("Scrollbar"))
		, _Orientation(Orient_Vertical)
		, _AlwaysShowScrollbar(false)
		, _Padding(FMargin(0.0f))
		, _Config(FRichScrollBarConfig())
	{
	}
		/* 스크롤바 트랙 및 썸 시각 스타일 */
		SLATE_STYLE_ARGUMENT(FScrollBarStyle, Style)

		/* 사용자가 썸 드래그 또는 단독 화살표 클릭으로 스크롤할 때 호출되는 델리게이트 (정규화 오프셋 전달) */
		SLATE_EVENT(FOnUserScrolled, OnUserScrolled)

		/* 화살표 버튼 조작 시 픽셀(Slate 단위) 기반 델타를 전달받는 선택적 델리게이트 */
		SLATE_EVENT(FOnRichArrowScrolled, OnArrowScrolled)

		/* 스크롤 축 방향 */
		SLATE_ARGUMENT(EOrientation, Orientation)

		/* 스크롤이 불필요할 때도 스크롤바(트랙·썸·화살표)를 항상 표시할지 여부 */
		SLATE_ARGUMENT(bool, AlwaysShowScrollbar)

		/* 스크롤바 두께 (Vertical: X 사용, Horizontal: Y 사용) */
		SLATE_ATTRIBUTE(FVector2D, Thickness)

		/* 외곽 보더 내부 패딩 */
		SLATE_ATTRIBUTE(FMargin, Padding)

		/* 고정/동적 썸 및 화살표 설정 */
		SLATE_ARGUMENT(FRichScrollBarConfig, Config)
	SLATE_END_ARGS()

	/* 생성자 및 소멸자 */
public:
	SRichScrollBar();
	virtual ~SRichScrollBar() override;

	void Construct(const FArguments& InArgs);

	/* SScrollBar 호환 인터페이스 및 설정 API */
public:
	virtual void SetState(float InOffsetFraction, float InThumbSizeFraction, bool bCallOnUserScrolled = false);
	void SetOnUserScrolled(const FOnUserScrolled& InHandler);
	void SetOnArrowScrolled(const FOnRichArrowScrolled& InHandler);
	void SetUserVisibility(TAttribute<EVisibility> InUserVisibility);
	void SetThickness(TAttribute<FVector2D> InThickness);
	void ClearThicknessOverride();
	void SetTrackPadding(TAttribute<FMargin> InPadding);
	void SetStyle(const FScrollBarStyle* InStyle);
	void SetOrientation(EOrientation InOrientation);
	void SetAlwaysShowScrollbar(bool bInAlwaysShow);
	void SetConfig(const FRichScrollBarConfig& InConfig);

	/* 상태 조회 API */
public:
	bool IsNeeded() const;
	float DistanceFromTop() const;
	float DistanceFromBottom() const;
	bool AlwaysShowScrollbar() const;
	float GetOffsetFraction() const;
	float GetThumbSizeFraction() const;
	bool IsDraggingThumb() const;
	bool IsHoldingArrow() const;
	const FRichScrollBarConfig& GetConfig() const;
	const FSlateBrush* GetTrackBackgroundBrush() const;
	const FSlateBrush* GetTopSlotBrush() const;
	const FSlateBrush* GetThumbBrush() const;
	const FSlateBrush* GetBottomSlotBrush() const;
	FMargin GetStartArrowSlotPadding() const;
	FMargin GetEndArrowSlotPadding() const;

	/* 화살표 조작 제어 (직접 제어 및 테스트용) */
public:
	void BeginArrowHold(int32 InDirection);
	void EndArrowHold();
	void StepArrowScroll(int32 InDirection, float InStepUnits);

	/* SWidget 입력 및 레이아웃 오버라이드 */
public:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	/* 내부 레이아웃·홀드 타이머 헬퍼 */
private:
	void RebuildOrientationLayout();
	EVisibility ComputeScrollbarVisibility() const;
	EVisibility ComputeArrowButtonVisibility() const;
	void RefreshVisibility();
	void UpdateHoverState(const FPointerEvent& MouseEvent);
	void ExecuteDragFromPointer(const FPointerEvent& MouseEvent);
	void ExecuteScrollFractionAndNotify(float InOffsetFraction);
	void HandleArrowButtonPressed(int32 InDirection);
	void HandleArrowButtonReleasedOrLost();
	EActiveTimerReturnType HandleHoldActiveTimer(double InCurrentTime, float InDeltaTime);
	void StopArrowHoldTimer();
	float ConvertUnitsToNormalizedDelta(float InUnits) const;
	bool ShouldPreventAutoCollapse() const;

	/* 멤버 변수 */
private:
	const FScrollBarStyle* ScrollBarStyle;
	TSharedPtr<SBorder> TrackBorder;
	TSharedPtr<SRichScrollTrack> Track;
	TSharedPtr<SRichArrowButton> StartArrowButton;
	TSharedPtr<SRichArrowButton> EndArrowButton;
	TSharedPtr<SBox> StartArrowSizeBox;
	TSharedPtr<SBox> EndArrowSizeBox;

	EOrientation Orientation;
	bool bAlwaysShowScrollbar;
	TAttribute<FVector2D> ThicknessAttribute;
	TAttribute<EVisibility> UserVisibility;
	FRichScrollBarConfig Config;

	FOnUserScrolled OnUserScrolled;
	FOnRichArrowScrolled OnArrowScrolled;

	bool bDraggingThumb;
	bool bIsThumbHovered;
	float DragGrabThumbFraction;

	bool bIsHoldingArrow;
	int32 ActiveHoldDirection;
	float HoldElapsedSincePress;
	float HoldAccumulatorSinceLastRepeat;
	TWeakPtr<FActiveTimerHandle> HoldActiveTimerHandle;
};
