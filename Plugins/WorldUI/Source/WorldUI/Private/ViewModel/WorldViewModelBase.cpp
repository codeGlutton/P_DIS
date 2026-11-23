#include "ViewModel/WorldViewModelBase.h"
#include "ViewModel/DualAnimTransition.h"

UWorldViewModelBase::UWorldViewModelBase()
{
	OpenCloseTransition = CreateDefaultSubobject<UDualAnimTransition>(TEXT("OpenCloseTransition"));
}

void UWorldViewModelBase::BeginDestroy()
{
	if (IsTemplate() == false && bIsInitialized == true)
	{
		EndViewModel();
	}

	Super::BeginDestroy();
}

void UWorldViewModelBase::Open()
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->Start();
	}
}

void UWorldViewModelBase::Close()
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->End();
	}
}

void UWorldViewModelBase::OpenWithAnimDynamic(FWorldUIOnWidgetAnimFinishedDynamicSignature OnComplete)
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->StartWithAnimDynamic(MoveTemp(OnComplete));
	}
}

void UWorldViewModelBase::OpenWithAnim(FWorldUIOnWidgetAnimFinishedSignature OnComplete)
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->StartWithAnim(MoveTemp(OnComplete));
	}
}

void UWorldViewModelBase::CloseWithAnimDynamic(FWorldUIOnWidgetAnimFinishedDynamicSignature OnComplete)
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->EndWithAnimDynamic(MoveTemp(OnComplete));
	}
}

void UWorldViewModelBase::CloseWithAnim(FWorldUIOnWidgetAnimFinishedSignature OnComplete)
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->EndWithAnim(MoveTemp(OnComplete));
	}
}

bool UWorldViewModelBase::IsOpen() const
{
	return (OpenCloseTransition != nullptr) ? OpenCloseTransition->IsStarted() : false;
}

bool UWorldViewModelBase::IsShowing() const
{
	return (OpenCloseTransition != nullptr) ? OpenCloseTransition->IsActive() : false;
}

EDualAnimState UWorldViewModelBase::GetOpenState() const
{
	return (OpenCloseTransition != nullptr) ? OpenCloseTransition->GetState() : EDualAnimState::Ended;
}

void UWorldViewModelBase::FinishInitialization()
{
	if (IsTemplate() == true || bIsInitialized == true)
	{
		return;
	}

	bIsInitialized = true;
	BeginViewModel();
}

void UWorldViewModelBase::BeginViewModel()
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->OnStartingNative.AddUObject(this, &UWorldViewModelBase::HandleTransitionOpenStarted);
		OpenCloseTransition->OnEndingNative.AddUObject(this, &UWorldViewModelBase::HandleTransitionCloseStarted);
		OpenCloseTransition->OnStartedNative.AddUObject(this, &UWorldViewModelBase::HandleTransitionOpenFinished);
		OpenCloseTransition->OnEndedNative.AddUObject(this, &UWorldViewModelBase::HandleTransitionCloseFinished);

		OpenCloseTransition->AddFieldValueChangedDelegate(
			UDualAnimTransition::FFieldNotificationClassDescriptor::State,
			FFieldValueChangedDelegate::CreateUObject(this, &UWorldViewModelBase::HandleTransitionStateChanged)
		);
		OpenCloseTransition->AddFieldValueChangedDelegate(
			UDualAnimTransition::FFieldNotificationClassDescriptor::IsStarted,
			FFieldValueChangedDelegate::CreateUObject(this, &UWorldViewModelBase::HandleTransitionStartedChanged)
		);
		OpenCloseTransition->AddFieldValueChangedDelegate(
			UDualAnimTransition::FFieldNotificationClassDescriptor::IsActive,
			FFieldValueChangedDelegate::CreateUObject(this, &UWorldViewModelBase::HandleTransitionActiveChanged)
		);
	}
}

void UWorldViewModelBase::EndViewModel()
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->OnStartingNative.RemoveAll(this);
		OpenCloseTransition->OnEndingNative.RemoveAll(this);
		OpenCloseTransition->OnStartedNative.RemoveAll(this);
		OpenCloseTransition->OnEndedNative.RemoveAll(this);
		OpenCloseTransition->RemoveAllFieldValueChangedDelegates(this);
	}
}

void UWorldViewModelBase::OnOpen()
{
}

void UWorldViewModelBase::OnClosed()
{
}

void UWorldViewModelBase::OnFinishOpenAnim(bool /*bInterrupted*/)
{
}

void UWorldViewModelBase::OnFinishCloseAnim(bool /*bInterrupted*/)
{
}

void UWorldViewModelBase::HandleTransitionOpenStarted(bool /*bWithAnim*/)
{
	OnOpen();
}

void UWorldViewModelBase::HandleTransitionCloseStarted(bool /*bWithAnim*/)
{
	OnClosed();
}

void UWorldViewModelBase::HandleTransitionOpenFinished(bool bInterrupted)
{
	OnFinishOpenAnim(bInterrupted);
}

void UWorldViewModelBase::HandleTransitionCloseFinished(bool bInterrupted)
{
	OnFinishCloseAnim(bInterrupted);
}

void UWorldViewModelBase::HandleTransitionStateChanged(UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
{
	if (InObject == OpenCloseTransition)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetOpenState);
	}
}

void UWorldViewModelBase::HandleTransitionStartedChanged(UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
{
	if (InObject == OpenCloseTransition)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsOpen);
	}
}

void UWorldViewModelBase::HandleTransitionActiveChanged(UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
{
	if (InObject == OpenCloseTransition)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsShowing);
	}
}