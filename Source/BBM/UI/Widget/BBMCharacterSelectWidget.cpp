#include "UI/Widget/BBMCharacterSelectWidget.h"
#include "UI/ViewModel/BBMCharacterSelectViewModel.h"

UBBMCharacterSelectWidget::UBBMCharacterSelectWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	LifecyclePolicy = EWorldWidgetLifecyclePolicy::PersistentViewport;
	DefaultZOrderLayer = EWorldWidgetZOrder::Popup;
}

UBBMCharacterSelectViewModel* UBBMCharacterSelectWidget::GetCharacterSelectViewModel() const
{
	return GetViewModelAs<UBBMCharacterSelectViewModel>();
}
