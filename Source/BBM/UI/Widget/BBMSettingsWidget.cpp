#include "UI/Widget/BBMSettingsWidget.h"
#include "UI/ViewModel/BBMSettingsViewModel.h"

UBBMSettingsWidget::UBBMSettingsWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	LifecyclePolicy = EWorldWidgetLifecyclePolicy::PersistentViewport;
	DefaultZOrderLayer = EWorldWidgetZOrder::Popup;
}

UBBMSettingsViewModel* UBBMSettingsWidget::GetSettingsViewModel() const
{
	return GetViewModelAs<UBBMSettingsViewModel>();
}

