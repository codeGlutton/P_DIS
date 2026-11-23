#include "Widgets/SRichVerticalBox.h"

#include "Widgets/RichBoxPanelHelper.h"

SLATE_IMPLEMENT_WIDGET(SRichVerticalBox)
void SRichVerticalBox::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

SRichVerticalBox::SRichVerticalBox() :
	bReversePaint(false)
{
}

void SRichVerticalBox::Construct(const FArguments& InArgs)
{
	bReversePaint = InArgs._bReversePaint;

	Children.Reserve(InArgs._Slots.Num());
	for (const FSlot::FSlotArguments& Arg : InArgs._Slots)
	{
		const FSlotBase::FSlotArguments& ChildSlotArgument = static_cast<const FSlotBase::FSlotArguments&>(Arg);
		const SBoxPanel::FSlot::FSlotArguments& BoxSlotArgument = static_cast<const SBoxPanel::FSlot::FSlotArguments&>(ChildSlotArgument);
		Children.AddSlot(MoveTemp(const_cast<SBoxPanel::FSlot::FSlotArguments&>(BoxSlotArgument)));
	}
}

void SRichVerticalBox::SetReversePaint(bool bInReversePaint)
{
	if (bReversePaint != bInReversePaint)
	{
		bReversePaint = bInReversePaint;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

int32 SRichVerticalBox::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const
{
	// 옵션이 꺼진 경우 기본 SBoxPanel의 동일 LayerId 배칭 경로를 유지함
	if (bReversePaint == false)
	{
		return SVerticalBox::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	return FRichBoxPanelHelper::PaintArrangedChildrenReversed(
		*this,
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled
	);
}
