/*****************************************************************//**
 * @file   SRichSlider.h
 * @brief  SSlider에서 파생하여 채움 트랙 렌더링을 지원하는 확장 Slate 슬라이더 위젯
 * @details SSlider의 입력·드래그·스텝·네비게이션·접근성 계약을 그대로 계승하고,
 *          OnPaint()에서 배경 바 → 핸들 정중앙 클리핑 채움 바 → 핸들 순으로 3레이어 렌더링을 수행함.
 * @date   2026-11-17
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Types/RichSliderTypes.h"
#include "Widgets/Input/SSlider.h"

template <>
struct TWidgetTypeTraits<class SRichSlider>
{
	static constexpr bool SupportsInvalidation() { return true; }
};

/**
 * SSlider 파생 확장 슬라이더 위젯 (SRichSlider)
 */
class EXTENSIONWIDGET_API SRichSlider : public SSlider
{
	SLATE_DECLARE_WIDGET(SRichSlider, SSlider)

	/* Slate 선언 인자 */
public:
	SLATE_BEGIN_ARGS(SRichSlider)
		: _IndentHandle(true)
		, _MouseUsesStep(false)
		, _RequiresControllerLock(true)
		, _Locked(false)
		, _Orientation(EOrientation::Orient_Horizontal)
		, _SliderBarColor(FLinearColor::White)
		, _SliderFillBarColor(FLinearColor::White)
		, _SliderHandleColor(FLinearColor::White)
		, _Style(&FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider"))
		, _RichStyle(&FRichSliderStyle::GetDefault())
		, _StepSize(0.01f)
		, _Value(1.0f)
		, _MinValue(0.0f)
		, _MaxValue(1.0f)
		, _IsFocusable(true)
		, _OnMouseCaptureBegin()
		, _OnMouseCaptureEnd()
		, _OnControllerCaptureBegin()
		, _OnControllerCaptureEnd()
		, _OnValueChanged()
		, _PreventThrottling(false)
	{
	}
		/* 핸들 크기에 맞춰 슬라이드 가능 영역을 안쪽으로 들여쓸지 여부 */
		SLATE_ATTRIBUTE(bool, IndentHandle)

		/* 마우스 드래그 시 스텝 크기 단위 반올림 적용 여부 */
		SLATE_ARGUMENT(bool, MouseUsesStep)

		/* 컨트롤러 조작 시 수락 입력으로 캡처 잠금이 필요한지 여부 */
		SLATE_ARGUMENT(bool, RequiresControllerLock)

		/* 핸들 조작 잠금 여부 */
		SLATE_ATTRIBUTE(bool, Locked)

		/* 슬라이더 방향 (수평 / 수직) */
		SLATE_ARGUMENT(EOrientation, Orientation)

		/* 배경 트랙 바 틴트 색상 */
		SLATE_ATTRIBUTE(FSlateColor, SliderBarColor)

		/* 채움 트랙 바 틴트 색상 */
		SLATE_ATTRIBUTE(FSlateColor, SliderFillBarColor)

		/* 핸들 틴트 색상 */
		SLATE_ATTRIBUTE(FSlateColor, SliderHandleColor)

		/* 기본 슬라이더 스타일 (배경 바·핸들·두께) */
		SLATE_STYLE_ARGUMENT(FSliderStyle, Style)

		/* 확장 슬라이더 스타일 (채움 바 브러시·패딩) */
		SLATE_STYLE_ARGUMENT(FRichSliderStyle, RichStyle)

		/* 키보드·컨트롤러·스텝 드래그 시 증감 단위 */
		SLATE_ATTRIBUTE(float, StepSize)

		/* 현재 슬라이더 값 */
		SLATE_ATTRIBUTE(float, Value)

		/* 최소값 */
		SLATE_ARGUMENT(float, MinValue)

		/* 최대값 */
		SLATE_ARGUMENT(float, MaxValue)

		/* 키보드 포커스 허용 여부 */
		SLATE_ARGUMENT(bool, IsFocusable)

		/* 마우스 캡처 시작 대리자 */
		SLATE_EVENT(FSimpleDelegate, OnMouseCaptureBegin)

		/* 마우스 캡처 종료 대리자 */
		SLATE_EVENT(FSimpleDelegate, OnMouseCaptureEnd)

		/* 컨트롤러 캡처 시작 대리자 */
		SLATE_EVENT(FSimpleDelegate, OnControllerCaptureBegin)

		/* 컨트롤러 캡처 종료 대리자 */
		SLATE_EVENT(FSimpleDelegate, OnControllerCaptureEnd)

		/* 값 변경 대리자 */
		SLATE_EVENT(FOnFloatValueChanged, OnValueChanged)

		/* 드래그 중 뷰포트 스로틀링 방지 여부 */
		SLATE_ARGUMENT(bool, PreventThrottling)
	SLATE_END_ARGS()

	/* 생성자 및 소멸자 */
public:
	SRichSlider();
	virtual ~SRichSlider() override = default;

	void Construct(const FArguments& InArgs);

	/* 확장 스타일 및 속성 제어 API */
public:
	/* 채움 바 확장 스타일을 교체함 */
	void SetRichStyle(const FRichSliderStyle* InRichStyle);

	/* 채움 바 틴트 색상을 변경함 */
	void SetSliderFillBarColor(TAttribute<FSlateColor> InSliderFillBarColor);

	/* 현재 상태에 맞는 채움 바 브러시를 폴백 없이 그대로 반환함 */
	const FSlateBrush* GetFillBarImage() const;

	/* SWidget / SSlider 오버라이드 */
public:
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	) const override;

	/* 내부 헬퍼 */
protected:
	TSlateAttributeRef<FSlateColor> GetSliderFillBarColorAttribute() const
	{
		return TSlateAttributeRef<FSlateColor>(SharedThis(this), SliderFillBarColorSlateAttribute);
	}

	/* 확장 스타일 및 Slate Attribute */
private:
	const FRichSliderStyle* RichStyle;

	TSlateAttribute<FSlateColor> SliderFillBarColorSlateAttribute;
};
