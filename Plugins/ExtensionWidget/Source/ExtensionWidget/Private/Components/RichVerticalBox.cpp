#include "Components/RichVerticalBox.h"

#include "Components/VerticalBoxSlot.h"
#include "Widgets/SRichVerticalBox.h"

#define LOCTEXT_NAMESPACE "ExtensionWidget"

URichVerticalBox::URichVerticalBox(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	bReversePaint(false)
{
}

void URichVerticalBox::SetReversePaint(bool bInReversePaint)
{
	bReversePaint = bInReversePaint;
	ApplyReversePaint();
}

TSharedPtr<SRichVerticalBox> URichVerticalBox::GetSlateRichVerticalBox() const
{
	return StaticCastSharedPtr<SRichVerticalBox>(MyVerticalBox);
}

void URichVerticalBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyReversePaint();
}

#if WITH_EDITOR
const FText URichVerticalBox::GetPaletteCategory()
{
	return LOCTEXT("ExtensionWidgetPaletteCategory", "Extension Widget");
}
#endif

TSharedRef<SWidget> URichVerticalBox::RebuildWidget()
{
	const TSharedRef<SRichVerticalBox> CreatedBox = SNew(SRichVerticalBox)
		.bReversePaint(bReversePaint);
	MyVerticalBox = CreatedBox;

	for (UPanelSlot* PanelSlot : Slots)
	{
		if (UVerticalBoxSlot* TypedSlot = Cast<UVerticalBoxSlot>(PanelSlot))
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(CreatedBox);
		}
	}

	return CreatedBox;
}

void URichVerticalBox::ApplyReversePaint()
{
	const TSharedPtr<SRichVerticalBox> SlateRichBox = GetSlateRichVerticalBox();
	if (SlateRichBox.IsValid() == true)
	{
		SlateRichBox->SetReversePaint(bReversePaint);
	}
}

#undef LOCTEXT_NAMESPACE
