#include "Widgets/SRichScrollBarBridge.h"

#include "Widgets/SRichScrollBar.h"

SRichScrollBarBridge::SRichScrollBarBridge() :
	CachedOffsetFraction(0.0f),
	CachedThumbSizeFraction(1.0f),
	bIsForwardingToRich(false),
	bIsForwardingToOwner(false)
{
}

void SRichScrollBarBridge::Construct(const FArguments& InArgs)
{
	// SScrollBar 기본 생성 경로를 호출해 내부 Track이 유효하도록 유지하고 화면에는 렌더링하지 않음
	SScrollBar::Construct(
		SScrollBar::FArguments()
		.AlwaysShowScrollbar(false)
	);
	SetVisibility(EVisibility::Collapsed);
}

void SRichScrollBarBridge::BindRichScrollBar(const TSharedPtr<SRichScrollBar>& InRichScrollBar)
{
	TargetRichScrollBar = InRichScrollBar;

	if (InRichScrollBar.IsValid() == true)
	{
		InRichScrollBar->SetState(CachedOffsetFraction, CachedThumbSizeFraction, false);
	}
}

void SRichScrollBarBridge::ForwardUserScrollToOwner(float InOffsetFraction)
{
	if (bIsForwardingToOwner == true)
	{
		return;
	}

	TGuardValue<bool> ForwardGuard(bIsForwardingToOwner, true);
	// SScrollBar::SetState(bCallOnUserScrolled=true)를 호출하면 STableViewBase가 바인딩한 OnUserScrolled가 안전하게 실행됨
	SScrollBar::SetState(InOffsetFraction, CachedThumbSizeFraction, true);
	CachedOffsetFraction = InOffsetFraction;
}

void SRichScrollBarBridge::SetState(float InOffsetFraction, float InThumbSizeFraction, bool bCallOnUserScrolled)
{
	SScrollBar::SetState(InOffsetFraction, InThumbSizeFraction, bCallOnUserScrolled);

	CachedOffsetFraction = InOffsetFraction;
	CachedThumbSizeFraction = InThumbSizeFraction;

	if (bIsForwardingToRich == true)
	{
		return;
	}

	const TSharedPtr<SRichScrollBar> PinnedRich = TargetRichScrollBar.Pin();
	if (PinnedRich.IsValid() == true)
	{
		TGuardValue<bool> ForwardGuard(bIsForwardingToRich, true);
		PinnedRich->SetState(InOffsetFraction, InThumbSizeFraction, false);
	}
}
