/*****************************************************************//**
 * @file   SRichHorizontalBox.h
 * @brief  SHorizontalBox에서 파생한 렌더링·HitTest 역순 지원 Slate 수평 박스 위젯
 * @details 레이아웃 배치 좌표는 SHorizontalBox와 동일하게 유지하면서 bReversePaint 옵션에 따라
 *          OnPaint 순회 순서와 자식별 LayerId를 역순으로 처리함.
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SBoxPanel.h"

/* 독립 전방 선언 */
class SRichHorizontalBox;
struct FRichBoxPanelHelper;

template <>
struct TWidgetTypeTraits<SRichHorizontalBox>
{
	static constexpr bool SupportsInvalidation() { return true; }
};

/**
 * SHorizontalBox 파생 확장 수평 박스 위젯 (SRichHorizontalBox)
 */
class EXTENSIONWIDGET_API SRichHorizontalBox : public SHorizontalBox
{
	SLATE_DECLARE_WIDGET(SRichHorizontalBox, SHorizontalBox)

	friend struct FRichBoxPanelHelper;

	/* Slate 선언 인자 */
public:
	SLATE_BEGIN_ARGS(SRichHorizontalBox)
		: _bReversePaint(false)
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}
		/* 렌더링 및 HitTest 우선순위를 역순(앞쪽 슬롯이 최상위)으로 처리할지 여부 */
		SLATE_ARGUMENT(bool, bReversePaint)

		/* 자식 슬롯 인자 */
		SLATE_SLOT_ARGUMENT(SHorizontalBox::FSlot, Slots)
	SLATE_END_ARGS()

	/* 생성자 및 슬롯 추가 인터페이스 */
public:
	SRichHorizontalBox();
	void Construct(const FArguments& InArgs);

	/* 렌더링 순서 제어 API */
public:
	/* 역순 렌더링·HitTest 활성화 여부를 변경함 */
	void SetReversePaint(bool bInReversePaint);

	/* 현재 역순 렌더링·HitTest 활성화 여부를 반환함 */
	bool GetReversePaint() const { return bReversePaint; }

	/* SWidget 인터페이스 */
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

	/* 멤버 변수 */
private:
	/* 역순 렌더링 및 자식 LayerId 분리 활성화 여부 */
	bool bReversePaint = false;
};
