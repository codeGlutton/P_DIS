#include "Components/RichHorizontalBox.h"

#include "Components/HorizontalBoxSlot.h"
#include "Widgets/SRichHorizontalBox.h"

#define LOCTEXT_NAMESPACE "ExtensionWidget"

URichHorizontalBox::URichHorizontalBox(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	bReversePaint(false)
{
}

void URichHorizontalBox::SetReversePaint(bool bInReversePaint)
{
	bReversePaint = bInReversePaint;
	ApplyReversePaint();
}

TSharedPtr<SRichHorizontalBox> URichHorizontalBox::GetSlateRichHorizontalBox() const
{
	return StaticCastSharedPtr<SRichHorizontalBox>(MyHorizontalBox);
}

void URichHorizontalBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyReversePaint();
}

#if WITH_EDITOR
const FText URichHorizontalBox::GetPaletteCategory()
{
	return LOCTEXT("ExtensionWidgetPaletteCategory", "Extension Widget");
}
#endif

TSharedRef<SWidget> URichHorizontalBox::RebuildWidget()
{
	const TSharedRef<SRichHorizontalBox> CreatedBox = SNew(SRichHorizontalBox)
		.bReversePaint(bReversePaint);
	MyHorizontalBox = CreatedBox;

	for (UPanelSlot* PanelSlot : Slots)
	{
		if (UHorizontalBoxSlot* TypedSlot = Cast<UHorizontalBoxSlot>(PanelSlot))
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(CreatedBox);
		}
	}

	return CreatedBox;
}

void URichHorizontalBox::ApplyReversePaint()
{
	const TSharedPtr<SRichHorizontalBox> SlateRichBox = GetSlateRichHorizontalBox();
	if (SlateRichBox.IsValid() == true)
	{
		SlateRichBox->SetReversePaint(bReversePaint);
	}
}

#undef LOCTEXT_NAMESPACE
