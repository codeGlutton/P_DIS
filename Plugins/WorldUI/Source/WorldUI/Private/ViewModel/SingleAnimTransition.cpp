#include "ViewModel/SingleAnimTransition.h"

USingleAnimTransition::USingleAnimTransition() :
	State(ESingleAnimState::Idle)
{
}

void USingleAnimTransition::SetState(ESingleAnimState InNewState)
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

void USingleAnimTransition::ExecuteDynamic(bool bWithAnim, FWorldUIOnWidgetAnimFinishedDynamicSignature OnComplete)
{
	ExecuteInternal(bWithAnim, FWorldUIOnWidgetAnimFinishedSignature(), MoveTemp(OnComplete));
}

void USingleAnimTransition::Execute(bool bWithAnim, FWorldUIOnWidgetAnimFinishedSignature OnComplete)
{
	ExecuteInternal(bWithAnim, MoveTemp(OnComplete), FWorldUIOnWidgetAnimFinishedDynamicSignature());
}

void USingleAnimTransition::Clear()
{
	if (State == ESingleAnimState::Playing)
	{
		FinishTransitionInternal(true);
		return;
	}

	ClearPendingCallbacks();
	SetState(ESingleAnimState::Idle);
}

void USingleAnimTransition::ClearPendingCallbacks()
{
	PendingDynamicCallbacks.Reset();
	PendingNativeCallbacks.Reset();
}

void USingleAnimTransition::NotifyAnimFinished()
{
	FinishTransitionInternal(false);
}

void USingleAnimTransition::BindPresentationHandler(
	const UObject* InOwner,
	FWorldUIOnPresentationTransitionRequestedSignature InHandler
)
{
	if (InOwner == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[USingleAnimTransition::BindPresentationHandler] InOwner is nullptr."));
		return;
	}

	BoundPresentationOwner = InOwner;
	PresentationHandler = MoveTemp(InHandler);
}

void USingleAnimTransition::ClearPresentationHandler(const UObject* InOwner)
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
		if (State == ESingleAnimState::Playing)
		{
			NotifyAnimFinished();
		}
	}
}

void USingleAnimTransition::InvokeCallbacksImmediate(
	FWorldUIOnWidgetAnimFinishedSignature&& InNativeComplete,
	FWorldUIOnWidgetAnimFinishedDynamicSignature&& InDynamicComplete,
	bool bInterrupted
)
{
	if (InDynamicComplete.IsBound() == true)
	{
		InDynamicComplete.Execute(bInterrupted);
	}

	if (InNativeComplete.IsBound() == true)
	{
		InNativeComplete.Execute(bInterrupted);
	}
}

void USingleAnimTransition::ExecuteInternal(
	bool bWithAnim,
	FWorldUIOnWidgetAnimFinishedSignature&& InNativeComplete,
	FWorldUIOnWidgetAnimFinishedDynamicSignature&& InDynamicComplete
)
{
	// 이미 재생 중이면 콜백을 누적하고, 무애님 요청 시 즉시 완료 스냅 처리
	if (State == ESingleAnimState::Playing)
	{
		EnqueuePendingCallbacks(MoveTemp(InNativeComplete), MoveTemp(InDynamicComplete));

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
	EnqueuePendingCallbacks(MoveTemp(InNativeComplete), MoveTemp(InDynamicComplete));

	SetState(ESingleAnimState::Playing);
	OnTransitionStartedNative.Broadcast(bWithAnim);
	OnTransitionStarted.Broadcast(bWithAnim);

	// 시작 훅 실행 도중 Clear() 등으로 상태가 변경되었으면 후속 프레젠테이션 요청 중단
	if (State != ESingleAnimState::Playing)
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

void USingleAnimTransition::FinishTransitionInternal(bool bInterrupted)
{
	if (State != ESingleAnimState::Playing)
	{
		return;
	}

	SetState(ESingleAnimState::Idle);

	// 상위 모듈이 최종 상태를 먼저 확정한 뒤 사용자 콜백이 상태를 조회할 수 있도록 선행 브로드캐스트
	OnFinishedBeforeCallbacksNative.Broadcast(bInterrupted);
	OnTransitionFinished.Broadcast(bInterrupted);

	ExecutePendingCallbacks(bInterrupted);
}

void USingleAnimTransition::EnqueuePendingCallbacks(
	FWorldUIOnWidgetAnimFinishedSignature&& InNativeComplete,
	FWorldUIOnWidgetAnimFinishedDynamicSignature&& InDynamicComplete
)
{
	if (InDynamicComplete.IsBound() == true)
	{
		PendingDynamicCallbacks.Add(MoveTemp(InDynamicComplete));
	}

	if (InNativeComplete.IsBound() == true)
	{
		PendingNativeCallbacks.Add(MoveTemp(InNativeComplete));
	}
}

void USingleAnimTransition::ExecutePendingCallbacks(bool bInterrupted)
{
	// 콜백 재진입 시 안전을 위해 로컬 배열로 이동 후 순차 실행
	TArray<FWorldUIOnWidgetAnimFinishedDynamicSignature> DynamicCallbacks = MoveTemp(PendingDynamicCallbacks);
	TArray<FWorldUIOnWidgetAnimFinishedSignature> NativeCallbacks = MoveTemp(PendingNativeCallbacks);
	PendingDynamicCallbacks.Reset();
	PendingNativeCallbacks.Reset();

	for (const FWorldUIOnWidgetAnimFinishedDynamicSignature& DynamicCallback : DynamicCallbacks)
	{
		if (DynamicCallback.IsBound() == true)
		{
			DynamicCallback.Execute(bInterrupted);
		}
	}

	for (const FWorldUIOnWidgetAnimFinishedSignature& NativeCallback : NativeCallbacks)
	{
		if (NativeCallback.IsBound() == true)
		{
			NativeCallback.Execute(bInterrupted);
		}
	}
}