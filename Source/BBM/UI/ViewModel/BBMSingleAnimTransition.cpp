#include "UI/ViewModel/BBMSingleAnimTransition.h"

UBBMSingleAnimTransition::UBBMSingleAnimTransition() :
	State(EBBMSingleAnimState::Idle)
{
}

void UBBMSingleAnimTransition::SetState(EBBMSingleAnimState InNewState)
{
	const bool bWasPlaying = IsPlaying();
	if (UE_MVVM_SET_PROPERTY_VALUE(State, InNewState) == false)
	{
		return;
	}

	if (bWasPlaying != IsPlaying())
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsPlaying);
	}
}

void UBBMSingleAnimTransition::ExecuteDynamic(bool bWithAnim, const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	ExecuteInternal(bWithAnim, FBBMOnWidgetAnimFinishedSignature(), OnComplete);
}

void UBBMSingleAnimTransition::Execute(bool bWithAnim, FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	ExecuteInternal(bWithAnim, MoveTemp(OnComplete), FBBMOnWidgetAnimFinishedDynamicSignature());
}

void UBBMSingleAnimTransition::ExecuteInternal(
	bool bWithAnim,
	FBBMOnWidgetAnimFinishedSignature&& InNativeComplete,
	const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
)
{
	// 이미 재생 중이면 콜백을 누적하고, 무애님 요청 시 즉시 완료 스냅 처리
	if (State == EBBMSingleAnimState::Playing)
	{
		EnqueuePendingCallbacks(MoveTemp(InNativeComplete), InDynamicComplete);

		if (bWithAnim == false)
		{
			if (PresentationHandler.IsBound() == true)
			{
				PresentationHandler.Execute(false);
			}
			else
			{
				NotifyAnimFinished();
			}
		}
		return;
	}

	ClearPendingCallbacks();
	EnqueuePendingCallbacks(MoveTemp(InNativeComplete), InDynamicComplete);

	SetState(EBBMSingleAnimState::Playing);
	OnTransitionStartedNative.Broadcast(bWithAnim);
	OnTransitionStarted.Broadcast(bWithAnim);

	// 시작 훅 실행 도중 Clear() 등으로 상태가 변경되었으면 후속 프레젠테이션 요청 중단
	if (State != EBBMSingleAnimState::Playing)
	{
		return;
	}

	if (PresentationHandler.IsBound() == true)
	{
		PresentationHandler.Execute(bWithAnim);
	}
	else
	{
		NotifyAnimFinished();
	}
}

void UBBMSingleAnimTransition::Clear()
{
	ClearPendingCallbacks();
	SetState(EBBMSingleAnimState::Idle);
}

void UBBMSingleAnimTransition::ClearPendingCallbacks()
{
	PendingDynamicCallbacks.Reset();
	PendingNativeCallbacks.Reset();
}

void UBBMSingleAnimTransition::NotifyAnimFinished()
{
	if (State != EBBMSingleAnimState::Playing)
	{
		return;
	}

	SetState(EBBMSingleAnimState::Idle);

	// 상위 모듈이 최종 상태를 먼저 확정한 뒤 사용자 콜백이 상태를 조회할 수 있도록 선행 브로드캐스트
	OnFinishedBeforeCallbacksNative.Broadcast();
	OnTransitionFinished.Broadcast();

	ExecutePendingCallbacks();
}

void UBBMSingleAnimTransition::BindPresentationHandler(
	const UObject* InOwner,
	FBBMOnPresentationTransitionRequestedSignature InHandler
)
{
	if (InOwner == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMSingleAnimTransition::BindPresentationHandler] InOwner is nullptr."));
		return;
	}

	BoundPresentationOwner = InOwner;
	PresentationHandler = MoveTemp(InHandler);
}

void UBBMSingleAnimTransition::ClearPresentationHandler(const UObject* InOwner)
{
	if (InOwner == nullptr)
	{
		return;
	}

	const bool bIsSameOwner = (BoundPresentationOwner.Get() == InOwner);
	const bool bIsStaleOwner = (BoundPresentationOwner.IsExplicitlyNull() == false && BoundPresentationOwner.IsValid() == false);

	if (bIsSameOwner == true || bIsStaleOwner == true)
	{
		BoundPresentationOwner.Reset();
		PresentationHandler.Unbind();

		// 진행 중이던 전환의 핸들러가 해제되면 상태·대기 콜백 고착을 막기 위해 즉시 완료 확정
		if (State == EBBMSingleAnimState::Playing)
		{
			NotifyAnimFinished();
		}
	}
}

void UBBMSingleAnimTransition::InvokeCallbacksImmediate(
	FBBMOnWidgetAnimFinishedSignature&& InNativeComplete,
	const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
)
{
	if (InDynamicComplete.IsBound() == true)
	{
		InDynamicComplete.Execute();
	}

	if (InNativeComplete.IsBound() == true)
	{
		InNativeComplete.Execute();
	}
}

void UBBMSingleAnimTransition::EnqueuePendingCallbacks(
	FBBMOnWidgetAnimFinishedSignature&& InNativeComplete,
	const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
)
{
	if (InDynamicComplete.IsBound() == true)
	{
		PendingDynamicCallbacks.Add(InDynamicComplete);
	}

	if (InNativeComplete.IsBound() == true)
	{
		PendingNativeCallbacks.Add(MoveTemp(InNativeComplete));
	}
}

void UBBMSingleAnimTransition::ExecutePendingCallbacks()
{
	// 콜백 재진입 시 안전을 위해 로컬 배열로 이동 후 순차 실행
	TArray<FBBMOnWidgetAnimFinishedDynamicSignature> DynamicCallbacks = MoveTemp(PendingDynamicCallbacks);
	TArray<FBBMOnWidgetAnimFinishedSignature> NativeCallbacks = MoveTemp(PendingNativeCallbacks);
	PendingDynamicCallbacks.Reset();
	PendingNativeCallbacks.Reset();

	for (const FBBMOnWidgetAnimFinishedDynamicSignature& DynamicCallback : DynamicCallbacks)
	{
		if (DynamicCallback.IsBound() == true)
		{
			DynamicCallback.Execute();
		}
	}

	for (const FBBMOnWidgetAnimFinishedSignature& NativeCallback : NativeCallbacks)
	{
		if (NativeCallback.IsBound() == true)
		{
			NativeCallback.Execute();
		}
	}
}

