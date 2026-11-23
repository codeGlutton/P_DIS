/*****************************************************************//**
 * @file   RichBoxPanelHelper.h
 * @brief  SRichVerticalBox / SRichHorizontalBox 공용 역순 OnPaint 렌더링 헬퍼
 * @details ArrangedChildren 역순 순회 및 자식별 LayerId 증분 처리를 공통화하여 중복을 제거함.
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/ArrangedChildren.h"
#include "Layout/Visibility.h"
#include "Rendering/DrawElements.h"
#include "Types/PaintArgs.h"

/**
 * Rich Box 패널 계열의 역순 OnPaint 실행을 담당하는 내부 유틸리티
 */
struct FRichBoxPanelHelper
{
	template <typename TPanelType>
	static int32 PaintArrangedChildrenReversed(
		const TPanelType& OwnerPanel,
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	)
	{
		// 배치 좌표·크기는 기본 박스 레이아웃 순서 그대로 계산한 뒤 순회 순서만 역전시킴
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		OwnerPanel.ArrangeChildren(AllottedGeometry, ArrangedChildren);
		ArrangedChildren.Reverse();

		int32 MaxLayerId = LayerId;
		bool bHasPaintedChild = false;

		const FPaintArgs NewArgs = Args.WithNewParent(&OwnerPanel);
		const bool bShouldBeEnabled = OwnerPanel.ShouldBeEnabled(bParentEnabled);

		for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
		{
			const FArrangedWidget& CurWidget = ArrangedChildren[ChildIndex];
			if (OwnerPanel.IsChildWidgetCulled(MyCullingRect, CurWidget) == false)
			{
				// 앞쪽에서 그린 자식의 상위 레이어가 뒤쪽 자식을 덮지 않고 FHittestGrid PrimarySort도 역전되도록 자식별 LayerId를 분리 증가시킴
				const int32 ChildInputLayerId = (bHasPaintedChild == true) ? (MaxLayerId + 1) : LayerId;
				const int32 CurChildMaxLayerId = CurWidget.Widget->Paint(
					NewArgs,
					CurWidget.Geometry,
					MyCullingRect,
					OutDrawElements,
					ChildInputLayerId,
					InWidgetStyle,
					bShouldBeEnabled
				);
				MaxLayerId = FMath::Max(MaxLayerId, CurChildMaxLayerId);
				bHasPaintedChild = true;
			}
		}

		return MaxLayerId;
	}
};
