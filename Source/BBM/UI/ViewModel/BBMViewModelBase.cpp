#include "UI/ViewModel/BBMViewModelBase.h"
#include "UI/ViewModel/BBMDualAnimTransition.h"

UBBMViewModelBase::UBBMViewModelBase()
{
	OpenCloseTransition = CreateDefaultSubobject<UBBMDualAnimTransition>(TEXT("OpenCloseTransition"));
}

void UBBMViewModelBase::BeginDestroy()
{
	if (IsTemplate() == false && bIsInitialized == true)
	{
		BP_EndViewModel();
		EndViewModel();
	}

	Super::BeginDestroy();
}

void UBBMViewModelBase::Open()
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->Start();
	}
}

void UBBMViewModelBase::Close()
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->End();
	}
}

void UBBMViewModelBase::OpenWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->StartWithAnimDynamic(OnComplete);
	}
}

void UBBMViewModelBase::OpenWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->StartWithAnim(MoveTemp(OnComplete));
	}
}

void UBBMViewModelBase::CloseWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->EndWithAnimDynamic(OnComplete);
	}
}

void UBBMViewModelBase::CloseWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->EndWithAnim(MoveTemp(OnComplete));
	}
}

bool UBBMViewModelBase::IsOpen() const
{
	return (OpenCloseTransition != nullptr) ? OpenCloseTransition->IsStarted() : false;
}

bool UBBMViewModelBase::IsShowing() const
{
	return (OpenCloseTransition != nullptr) ? OpenCloseTransition->IsActive() : false;
}

EBBMDualAnimState UBBMViewModelBase::GetOpenState() const
{
	return (OpenCloseTransition != nullptr) ? OpenCloseTransition->GetState() : EBBMDualAnimState::Ended;
}

void UBBMViewModelBase::FinishInitialization()
{
	if (IsTemplate() == true || bIsInitialized == true)
	{
		return;
	}

	bIsInitialized = true;
	BeginViewModel();
	BP_BeginViewModel();
}

void UBBMViewModelBase::BeginViewModel()
{
	if (OpenCloseTransition != nullptr)
	{
		OpenCloseTransition->OnStartingNative.AddUObject(this, &UBBMViewModelBase::HandleTransitionOpenStarted);
		OpenCloseTransition->OnEndingNative.AddUObject(this, &UBBMViewModelBase::HandleTransitionCloseStarted);
		OpenCloseTransition->OnStartedNative.AddUObject(this, &UBBMViewModelBase::HandleTransitionOpenFinished);
		OpenCloseTransition->OnEndedNative.AddUObject(this, &UBBMViewModelBase::HandleTransitionCloseFinished);

		OpenCloseTransition->AddFieldValueChangedDelegate(
			UBBMDualAnimTransition::FFieldNotificationClassDescriptor::State,
			FFieldValueChangedDelegate::CreateUObject(this, &UBBMViewModelBase::HandleTransitionStateChanged)
		);
		OpenCloseTransition->AddFieldValueChangedDelegate(
			UBBMDualAnimTransition::FFieldNotificationClassDescriptor::IsStarted,
			FFieldValueChangedDelegate::CreateUObject(this, &UBBMViewModelBase::HandleTransitionStartedChanged)
		);
		OpenCloseTransition->AddFieldValueChangedDelegate(
			UBBMDualAnimTransition::FFieldNotificationClassDescriptor::IsActive,
			FFieldValueChangedDelegate::CreateUObject(this, &UBBMViewModelBase::HandleTransitionActiveChanged)
		);
	}
}

void UBBMViewModelBase::EndViewModel()
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

void UBBMViewModelBase::OnOpen()
{
}

void UBBMViewModelBase::OnClosed()
{
}

void UBBMViewModelBase::OnFinishOpenAnim()
{
}

void UBBMViewModelBase::OnFinishCloseAnim()
{
}

void UBBMViewModelBase::HandleTransitionOpenStarted()
{
	OnOpen();
	BP_OnOpen();
}

void UBBMViewModelBase::HandleTransitionCloseStarted()
{
	OnClosed();
	BP_OnClosed();
}

void UBBMViewModelBase::HandleTransitionOpenFinished()
{
	OnFinishOpenAnim();
	BP_OnFinishOpenAnim();
}

void UBBMViewModelBase::HandleTransitionCloseFinished()
{
	OnFinishCloseAnim();
	BP_OnFinishCloseAnim();
}

void UBBMViewModelBase::HandleTransitionStateChanged(UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
{
	if (InObject == OpenCloseTransition)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetOpenState);
	}
}

void UBBMViewModelBase::HandleTransitionStartedChanged(UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
{
	if (InObject == OpenCloseTransition)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsOpen);
	}
}

void UBBMViewModelBase::HandleTransitionActiveChanged(UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
{
	if (InObject == OpenCloseTransition)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsShowing);
	}
}
