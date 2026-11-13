#include "UI/ViewModel/BBMDualAnimTransition.h"
#include "UI/ViewModel/BBMSingleAnimTransition.h"

UBBMDualAnimTransition::UBBMDualAnimTransition() :
	State(EBBMDualAnimState::Ended)
{
	StartTransition = CreateDefaultSubobject<UBBMSingleAnimTransition>(TEXT("StartTransition"));
	EndTransition = CreateDefaultSubobject<UBBMSingleAnimTransition>(TEXT("EndTransition"));
}

void UBBMDualAnimTransition::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		if (StartTransition != nullptr)
		{
			StartTransition->OnFinishedBeforeCallbacksNative.AddUObject(
				this,
				&UBBMDualAnimTransition::HandleStartSlotFinishedBeforeCallbacks
			);
		}

		if (EndTransition != nullptr)
		{
			EndTransition->OnFinishedBeforeCallbacksNative.AddUObject(
				this,
				&UBBMDualAnimTransition::HandleEndSlotFinishedBeforeCallbacks
			);
		}
	}
}

void UBBMDualAnimTransition::BeginDestroy()
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

void UBBMDualAnimTransition::Start()
{
	ExecuteStart(false, FBBMOnWidgetAnimFinishedSignature(), FBBMOnWidgetAnimFinishedDynamicSignature());
}

void UBBMDualAnimTransition::End()
{
	ExecuteEnd(false, FBBMOnWidgetAnimFinishedSignature(), FBBMOnWidgetAnimFinishedDynamicSignature());
}

void UBBMDualAnimTransition::StartWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	ExecuteStart(true, FBBMOnWidgetAnimFinishedSignature(), OnComplete);
}

void UBBMDualAnimTransition::StartWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	ExecuteStart(true, MoveTemp(OnComplete), FBBMOnWidgetAnimFinishedDynamicSignature());
}

void UBBMDualAnimTransition::EndWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	ExecuteEnd(true, FBBMOnWidgetAnimFinishedSignature(), OnComplete);
}

void UBBMDualAnimTransition::EndWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	ExecuteEnd(true, MoveTemp(OnComplete), FBBMOnWidgetAnimFinishedDynamicSignature());
}

void UBBMDualAnimTransition::ExecuteStart(
	bool bWithAnim,
	FBBMOnWidgetAnimFinishedSignature InNativeComplete,
	const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
)
{
	ExecuteDirectionalTransition(
		StartTransition,
		EndTransition,
		EBBMDualAnimState::Started,
		EBBMDualAnimState::Starting,
		OnStartingNative,
		bWithAnim,
		MoveTemp(InNativeComplete),
		InDynamicComplete
	);
}

void UBBMDualAnimTransition::ExecuteEnd(
	bool bWithAnim,
	FBBMOnWidgetAnimFinishedSignature InNativeComplete,
	const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
)
{
	ExecuteDirectionalTransition(
		EndTransition,
		StartTransition,
		EBBMDualAnimState::Ended,
		EBBMDualAnimState::Ending,
		OnEndingNative,
		bWithAnim,
		MoveTemp(InNativeComplete),
		InDynamicComplete
	);
}

void UBBMDualAnimTransition::Clear()
{
	if (StartTransition != nullptr)
	{
		StartTransition->Clear();
	}

	if (EndTransition != nullptr)
	{
		EndTransition->Clear();
	}

	SetState(EBBMDualAnimState::Ended);
}

void UBBMDualAnimTransition::ClearPendingCallbacks()
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

void UBBMDualAnimTransition::BindPresentationHandlers(
	const UObject* InOwner,
	FBBMOnPresentationTransitionRequestedSignature InStartHandler,
	FBBMOnPresentationTransitionRequestedSignature InEndHandler
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

void UBBMDualAnimTransition::ClearPresentationHandlers(const UObject* InOwner)
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

void UBBMDualAnimTransition::NotifyStartTransitionFinished()
{
	if (StartTransition != nullptr)
	{
		StartTransition->NotifyAnimFinished();
	}
}

void UBBMDualAnimTransition::NotifyEndTransitionFinished()
{
	if (EndTransition != nullptr)
	{
		EndTransition->NotifyAnimFinished();
	}
}

void UBBMDualAnimTransition::ExecuteDirectionalTransition(
	UBBMSingleAnimTransition* TargetSlot,
	UBBMSingleAnimTransition* OppositeSlot,
	EBBMDualAnimState CompletedState,
	EBBMDualAnimState TransitioningState,
	FSimpleMulticastDelegate& OnTransitioningNative,
	bool bWithAnim,
	FBBMOnWidgetAnimFinishedSignature&& InNativeComplete,
	const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
)
{
	if (TargetSlot == nullptr || OppositeSlot == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMDualAnimTransition::ExecuteDirectionalTransition] Transition slot is nullptr."));
		return;
	}

	// 반대 방향 전환의 대기 콜백 및 재생 상태를 선제 정리
	OppositeSlot->Clear();

	// 이미 목표 완료 상태면 재실행 없이 전달된 콜백만 즉시 처리
	if (State == CompletedState)
	{
		UBBMSingleAnimTransition::InvokeCallbacksImmediate(MoveTemp(InNativeComplete), InDynamicComplete);
		return;
	}

	// 이미 동일 방향 전환 진행 중이면 대상 슬롯에 콜백 누적(또는 무애님 즉시 스냅)을 위임
	if (State == TransitioningState)
	{
		TargetSlot->ExecuteInternal(bWithAnim, MoveTemp(InNativeComplete), InDynamicComplete);
		return;
	}

	// 신규 전환 진입 시 세부 상태 선반영 및 시작 훅 발송
	SetState(TransitioningState);
	OnTransitioningNative.Broadcast();
	if (TransitioningState == EBBMDualAnimState::Starting) { OnStarting.Broadcast(); }
	else if (TransitioningState == EBBMDualAnimState::Ending) { OnEnding.Broadcast(); }

	// 시작 훅 실행 중 상태가 재변경되었으면 프레젠테이션 실행 중단
	if (State != TransitioningState)
	{
		return;
	}

	TargetSlot->ExecuteInternal(bWithAnim, MoveTemp(InNativeComplete), InDynamicComplete);
}

void UBBMDualAnimTransition::HandleStartSlotFinishedBeforeCallbacks()
{
	if (State != EBBMDualAnimState::Starting)
	{
		return;
	}

	SetState(EBBMDualAnimState::Started);
	OnStartedNative.Broadcast();
	OnStarted.Broadcast();
}

void UBBMDualAnimTransition::HandleEndSlotFinishedBeforeCallbacks()
{
	if (State != EBBMDualAnimState::Ending)
	{
		return;
	}

	SetState(EBBMDualAnimState::Ended);
	OnEndedNative.Broadcast();
	OnEnded.Broadcast();
}

void UBBMDualAnimTransition::SetState(EBBMDualAnimState InNewState)
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
