#include "Types/RichSliderTypes.h"

const FName FRichSliderStyle::TypeName(TEXT("FRichSliderStyle"));

FRichSliderStyle::FRichSliderStyle() :
	NormalFillBarImage(FSlateNoResource()),
	HoveredFillBarImage(FSlateNoResource()),
	DisabledFillBarImage(FSlateNoResource()),
	FillBarBorderPadding(0.0f)
{
}

void FRichSliderStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&NormalFillBarImage);
	OutBrushes.Add(&HoveredFillBarImage);
	OutBrushes.Add(&DisabledFillBarImage);
}

const FRichSliderStyle& FRichSliderStyle::GetDefault()
{
	static FRichSliderStyle DefaultStyle;
	return DefaultStyle;
}
