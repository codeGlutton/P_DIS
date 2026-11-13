#include "UI/Widget/BBMCreditsWidget.h"
#include "UI/ViewModel/BBMCreditsViewModel.h"

UBBMCreditsWidget::UBBMCreditsWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	LifecyclePolicy = EBBMWidgetLifecyclePolicy::PersistentViewport;
	DefaultZOrderLayer = EBBMWidgetZOrder::Popup;
}

UBBMCreditsViewModel* UBBMCreditsWidget::GetCreditsViewModel() const
{
	return GetViewModelAs<UBBMCreditsViewModel>();
}

