/*****************************************************************//**
 * @file   SRichScrollBarBridge.h
 * @brief  STableViewBase의 SScrollBar 의존성과 SRichScrollBar를 연결하는 비시각 프록시 위젯
 * @details Option A 아키텍처에 따라 STableViewBase에 ExternalScrollbar로 주입되어
 *          스크롤 상태(SetState)와 사용자 스크롤 이벤트를 양방향 전달함.
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Layout/SScrollBar.h"

/* 독립 전방 선언 */
class SRichScrollBar;

/**
 * STableViewBase와 SRichScrollBar 사이의 양방향 상태 브릿지 위젯
 */
class EXTENSIONWIDGET_API SRichScrollBarBridge : public SScrollBar
{
	/* 생성자 및 연결 API */
public:
	SRichScrollBarBridge();
	void Construct(const FArguments& InArgs);

	/* 상태를 동기화할 대상 RichScrollBar를 연결함 */
	void BindRichScrollBar(const TSharedPtr<SRichScrollBar>& InRichScrollBar);

	/* RichScrollBar에서 발생한 사용자 스크롤 오프셋을 소유 ListView(STableViewBase)로 전달함 */
	void ForwardUserScrollToOwner(float InOffsetFraction);

	/* SScrollBar 인터페이스 오버라이드 */
public:
	virtual void SetState(float InOffsetFraction, float InThumbSizeFraction, bool bCallOnUserScrolled = false) override;

	/* 멤버 변수 */
private:
	TWeakPtr<SRichScrollBar> TargetRichScrollBar;
	float CachedOffsetFraction;
	float CachedThumbSizeFraction;
	bool bIsForwardingToRich;
	bool bIsForwardingToOwner;
};
