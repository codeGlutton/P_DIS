#include "ViewModel/DualAnimTransition.h"
#include "ViewModel/SingleAnimTransition.h"

UDualAnimTransition::UDualAnimTransition() :
	State(EDualAnimState::Ended)
{
	StartTransition = CreateDefaultSubobject<USingleAnimTransition>(TEXT("StartTransition"));
	EndTransition = CreateDefaultSubobject<USingleAnimTransition>(TEXT("EndTransition"));
}

void UDualAnimTransition::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		if (StartTransition != nullptr)
		{
			StartTransition->OnFinishedBeforeCallbacksNative.AddUObject(
				this,
				&UDualAnimTransition::HandleStartSlotFinishedBeforeCallbacks
			);
		}

		if (EndTransition != nullptr)
		{
			EndTransition->OnFinishedBeforeCallbacksNative.AddUObject(
				this,
				&UDualAnimTransition::HandleEndSlotFinishedBeforeCallbacks
			);
		}
	}
}

void UDualAnimTransition::BeginDestroy()
{
	if (StartTransition != nullptr)
	{
		StartTransition->OnFinishedBeforeCallbacksNative.RemoveAll(this);
	}

	if (EndTransition != nullptr)
	{
		EndTransition->OnFinishedBeforeCallbacksNative.RemoveAll(this);
	}

	Super::BeginDestroy();
}

void UDualAnimTransition::Start()
{
	ExecuteStart(false, FWorldUIOnWidgetAnimFinishedSignature(), FWorldUIOnWidgetAnimFinishedDynamicSignature());
}

void UDualAnimTransition::End()
{
	ExecuteEnd(false, FWorldUIOnWidgetAnimFinishedSignature(), FWorldUIOnWidgetAnimFinishedDynamicSignature());
}

void UDualAnimTransition::StartWithAnimDynamic(FWorldUIOnWidgetAnimFinishedDynamicSignature OnComplete)
{
	ExecuteStart(true, FWorldUIOnWidgetAnimFinishedSignature(), MoveTemp(OnComplete));
}

void UDualAnimTransition::StartWithAnim(FWorldUIOnWidgetAnimFinishedSignature OnComplete)
{
	ExecuteStart(true, MoveTemp(OnComplete), FWorldUIOnWidgetAnimFinishedDynamicSignature());
}

void UDualAnimTransition::EndWithAnimDynamic(FWorldUIOnWidgetAnimFinishedDynamicSignature OnComplete)
{
	ExecuteEnd(true, FWorldUIOnWidgetAnimFinishedSignature(), MoveTemp(OnComplete));
}

void UDualAnimTransition::EndWithAnim(FWorldUIOnWidgetAnimFinishedSignature OnComplete)
{
	ExecuteEnd(true, MoveTemp(OnComplete), FWorldUIOnWidgetAnimFinishedDynamicSignature());
}

void UDualAnimTransition::Clear()
{
	SetState(EDualAnimState::Ended);

	if (StartTransition != nullptr)
	{
		StartTransition->Clear();
	}

	if (EndTransition != nullptr)
	{
		EndTransition->Clear();
	}
}

void UDualAnimTransition::ClearPendingCallbacks()
{
	if (StartTransition != nullptr)
	{
		StartTransition->ClearPendingCallbacks();
	}

	if (EndTransition != nullptr)
	{
		EndTransition->ClearPendingCallbacks();
	}
}

void UDualAnimTransition::BindPresentationHandlers(
	const UObject* InOwner,
	FWorldUIOnPresentationTransitionRequestedSignature InStartHandler,
	FWorldUIOnPresentationTransitionRequestedSignature InEndHandler
)
{
	if (StartTransition != nullptr)
	{
		StartTransition->BindPresentationHandler(InOwner, MoveTemp(InStartHandler));
	}

	if (EndTransition != nullptr)
	{
		EndTransition->BindPresentationHandler(InOwner, MoveTemp(InEndHandler));
	}
}

void UDualAnimTransition::ClearPresentationHandlers(const UObject* InOwner)
{
	if (StartTransition != nullptr)
	{
		StartTransition->ClearPresentationHandler(InOwner);
	}

	if (EndTransition != nullptr)
	{
		EndTransition->ClearPresentationHandler(InOwner);
	}
}

void UDualAnimTransition::NotifyStartTransitionFinished()
{
	if (StartTransition != nullptr)
	{
		StartTransition->NotifyAnimFinished();
	}
}

void UDualAnimTransition::NotifyEndTransitionFinished()
{
	if (EndTransition != nullptr)
	{
		EndTransition->NotifyAnimFinished();
	}
}

void UDualAnimTransition::ExecuteStart(
	bool bWithAnim,
	FWorldUIOnWidgetAnimFinishedSignature InNativeComplete,
	FWorldUIOnWidgetAnimFinishedDynamicSignature InDynamicComplete
)
{
	ExecuteDirectionalTransition(
		StartTransition,
		EndTransition,
		EDualAnimState::Started,
		EDualAnimState::Starting,
		OnStartingNative,
		bWithAnim,
		MoveTemp(InNativeComplete),
		MoveTemp(InDynamicComplete)
	);
}

void UDualAnimTransition::ExecuteEnd(
	bool bWithAnim,
	FWorldUIOnWidgetAnimFinishedSignature InNativeComplete,
	FWorldUIOnWidgetAnimFinishedDynamicSignature InDynamicComplete
)
{
	ExecuteDirectionalTransition(
		EndTransition,
		StartTransition,
		EDualAnimState::Ended,
		EDualAnimState::Ending,
		OnEndingNative,
		bWithAnim,
		MoveTemp(InNativeComplete),
		MoveTemp(InDynamicComplete)
	);
}

void UDualAnimTransition::ExecuteDirectionalTransition(
	USingleAnimTransition* TargetSlot,
	USingleAnimTransition* OppositeSlot,
	EDualAnimState CompletedState,
	EDualAnimState TransitioningState,
	FWorldUIOnAnimTransitionStartedNativeSignature& OnTransitioningNative,
	bool bWithAnim,
	FWorldUIOnWidgetAnimFinishedSignature&& InNativeComplete,
	FWorldUIOnWidgetAnimFinishedDynamicSignature&& InDynamicComplete
)
{
	if (TargetSlot == nullptr || OppositeSlot == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UDualAnimTransition::ExecuteDirectionalTransition] Transition slot is nullptr."));
		return;
	}

	// 이미 목표 완료 상태면 재실행 없이 전달된 콜백만 즉시 처리
	if (State == CompletedState)
	{
		USingleAnimTransition::InvokeCallbacksImmediate(MoveTemp(InNativeComplete), MoveTemp(InDynamicComplete), false);
		return;
	}

	// 이미 동일 방향 전환 진행 중이면 대상 슬롯에 콜백 누적(또는 무애님 즉시 스냅)을 위임
	if (State == TransitioningState)
	{
		TargetSlot->ExecuteInternal(bWithAnim, MoveTemp(InNativeComplete), MoveTemp(InDynamicComplete));
		return;
	}

	// 신규 전환 진입 시 세부 상태 선반영 후 반대 방향 슬롯을 중단(Clear) 처리하고 시작 훅 발송
	SetState(TransitioningState);
	OppositeSlot->Clear();

	OnTransitioningNative.Broadcast(bWithAnim);
	if (TransitioningState == EDualAnimState::Starting)
	{
		OnStarting.Broadcast(bWithAnim);
	}
	else if (TransitioningState == EDualAnimState::Ending)
	{
		OnEnding.Broadcast(bWithAnim);
	}

	// 시작 훅 실행 중 상태가 재변경되었으면 프레젠테이션 실행 중단
	if (State != TransitioningState)
	{
		return;
	}

	TargetSlot->ExecuteInternal(bWithAnim, MoveTemp(InNativeComplete), MoveTemp(InDynamicComplete));
}

void UDualAnimTransition::HandleStartSlotFinishedBeforeCallbacks(bool bInterrupted)
{
	if (bInterrupted == true)
	{
		OnStartedNative.Broadcast(true);
		OnStarted.Broadcast(true);
		return;
	}

	if (State != EDualAnimState::Starting)
	{
		return;
	}

	SetState(EDualAnimState::Started);
	OnStartedNative.Broadcast(false);
	OnStarted.Broadcast(false);
}

void UDualAnimTransition::HandleEndSlotFinishedBeforeCallbacks(bool bInterrupted)
{
	if (bInterrupted == true)
	{
		OnEndedNative.Broadcast(true);
		OnEnded.Broadcast(true);
		return;
	}

	if (State != EDualAnimState::Ending)
	{
		return;
	}

	SetState(EDualAnimState::Ended);
	OnEndedNative.Broadcast(false);
	OnEnded.Broadcast(false);
}

void UDualAnimTransition::SetState(EDualAnimState InNewState)
{
	const bool bWasStarted = IsStarted();
	const bool bWasActive = IsActive();

	if (UE_MVVM_SET_PROPERTY_VALUE(State, InNewState) == false)
	{
		return;
	}

	if (bWasStarted != IsStarted())
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsStarted);
	}

	if (bWasActive != IsActive())
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsActive);
	}
}