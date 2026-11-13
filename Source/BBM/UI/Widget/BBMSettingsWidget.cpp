#include "UI/Widget/BBMSettingsWidget.h"
#include "UI/ViewModel/BBMSettingsViewModel.h"

UBBMSettingsWidget::UBBMSettingsWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	LifecyclePolicy = EBBMWidgetLifecyclePolicy::PersistentViewport;
	DefaultZOrderLayer = EBBMWidgetZOrder::Popup;
}

UBBMSettingsViewModel* UBBMSettingsWidget::GetSettingsViewModel() const
{
	return GetViewModelAs<UBBMSettingsViewModel>();
}

