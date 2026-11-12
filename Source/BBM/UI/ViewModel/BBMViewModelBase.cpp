/*****************************************************************//**
 * @file   BBMViewModelBase.cpp
 * @brief  BBM(비트 배낭 상인) 공용 MVVM 뷰모델 베이스 클래스 구현
 * @date   2026-11-07
 *********************************************************************/

#include "UI/ViewModel/BBMViewModelBase.h"

UBBMViewModelBase::UBBMViewModelBase()
{
}

void UBBMViewModelBase::Open()
{
	ExecuteOpen(false, FBBMOnWidgetAnimFinishedSignature(), FBBMOnWidgetAnimFinishedDynamicSignature());
}

void UBBMViewModelBase::Close()
{
	ExecuteClose(false, FBBMOnWidgetAnimFinishedSignature(), FBBMOnWidgetAnimFinishedDynamicSignature());
}

void UBBMViewModelBase::OpenWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	ExecuteOpen(true, FBBMOnWidgetAnimFinishedSignature(), OnComplete);
}

void UBBMViewModelBase::OpenWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	ExecuteOpen(true, MoveTemp(OnComplete), FBBMOnWidgetAnimFinishedDynamicSignature());
}

void UBBMViewModelBase::CloseWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	ExecuteClose(true, FBBMOnWidgetAnimFinishedSignature(), OnComplete);
}

void UBBMViewModelBase::CloseWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	ExecuteClose(true, MoveTemp(OnComplete), FBBMOnWidgetAnimFinishedDynamicSignature());
}

void UBBMViewModelBase::BindPresentationHandlers(
	const UObject* InOwner,
	FBBMOnPresentationTransitionRequestedSignature InOpenHandler,
	FBBMOnPresentationTransitionRequestedSignature InCloseHandler
)
{
	if (InOwner == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMViewModelBase::BindPresentationHandlers] InOwner is nullptr."));
		return;
	}

	BoundPresentationOwner = InOwner;
	OpenPresentationHandler = MoveTemp(InOpenHandler);
	ClosePresentationHandler = MoveTemp(InCloseHandler);
}

void UBBMViewModelBase::ClearPresentationHandlers(const UObject* InOwner)
{
	if (InOwner == nullptr)
	{
		return;
	}

	// 현재 등록된 소유 위젯과 일치할 때만 해제하여 새 위젯의 핸들러 덮어쓰기 방지
	if (BoundPresentationOwner.IsValid() == false || BoundPresentationOwner.Get() == InOwner)
	{
		BoundPresentationOwner.Reset();
		OpenPresentationHandler.Unbind();
		ClosePresentationHandler.Unbind();
	}
}

void UBBMViewModelBase::NotifyOpenTransitionFinished()
{
	if (OpenState != EBBMWidgetOpenState::Opening)
	{
		return;
	}

	SetOpenState(EBBMWidgetOpenState::Open);

	OnFinishOpenAnim();
	BP_OnFinishOpenAnim();

	ExecutePendingCallbacks(PendingOpenAnimDynamicCallbacks, PendingOpenAnimCallbacks);
}

void UBBMViewModelBase::NotifyCloseTransitionFinished()
{
	if (OpenState != EBBMWidgetOpenState::Closing)
	{
		return;
	}

	SetOpenState(EBBMWidgetOpenState::Closed);

	OnFinishCloseAnim();
	BP_OnFinishCloseAnim();

	ExecutePendingCallbacks(PendingCloseAnimDynamicCallbacks, PendingCloseAnimCallbacks);
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

void UBBMViewModelBase::ExecuteOpen(
	bool bWithAnim,
	FBBMOnWidgetAnimFinishedSignature InNativeComplete,
	const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
)
{
	// 반대 방향(클로즈) 대기 콜백 정리
	ClearPendingCallbacks(PendingCloseAnimDynamicCallbacks, PendingCloseAnimCallbacks);

	// 이미 완전히 열린 상태면 재실행 없이 전달된 콜백만 즉시 처리
	if (OpenState == EBBMWidgetOpenState::Open)
	{
		InvokeTransitionCallbackImmediate(MoveTemp(InNativeComplete), InDynamicComplete);
		return;
	}

	// 이미 열기 전환 진행 중일 때의 처리
	if (OpenState == EBBMWidgetOpenState::Opening)
	{
		EnqueuePendingCallbacks(
			PendingOpenAnimDynamicCallbacks,
			PendingOpenAnimCallbacks,
			MoveTemp(InNativeComplete),
			InDynamicComplete
		);

		// 무애님 Open() 요청 시 진행 중 애니메이션을 즉시 스냅 완료하고 누적 콜백까지 일괄 호출
		if (bWithAnim == false)
		{
			if (OpenPresentationHandler.IsBound() == true)
			{
				OpenPresentationHandler.Execute(false);
			}
			else
			{
				NotifyOpenTransitionFinished();
			}
		}
		return;
	}

	ClearPendingCallbacks(PendingOpenAnimDynamicCallbacks, PendingOpenAnimCallbacks);

	if (bWithAnim == true)
	{
		EnqueuePendingCallbacks(
			PendingOpenAnimDynamicCallbacks,
			PendingOpenAnimCallbacks,
			MoveTemp(InNativeComplete),
			InDynamicComplete
		);
	}

	// 애니메이션 진행 여부와 무관하게 세부 상태 전이 및 Open 이벤트는 즉시 반영/호출
	SetOpenState(EBBMWidgetOpenState::Opening);

	OnOpen();
	BP_OnOpen();

	// 훅 실행 중 상태가 다른 전이로 변경된 경우 후속 프레젠테이션 요청 중단
	if (OpenState != EBBMWidgetOpenState::Opening)
	{
		return;
	}

	// 연결된 View가 있으면 프레젠테이션 요청을 전달하고, 없으면 즉시 완료 처리
	if (OpenPresentationHandler.IsBound() == true)
	{
		OpenPresentationHandler.Execute(bWithAnim);
	}
	else
	{
		NotifyOpenTransitionFinished();
	}
}

void UBBMViewModelBase::ExecuteClose(
	bool bWithAnim,
	FBBMOnWidgetAnimFinishedSignature InNativeComplete,
	const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
)
{
	// 반대 방향(오픈) 대기 콜백 정리
	ClearPendingCallbacks(PendingOpenAnimDynamicCallbacks, PendingOpenAnimCallbacks);

	// 이미 완전히 닫힌 상태면 재실행 없이 전달된 콜백만 즉시 처리
	if (OpenState == EBBMWidgetOpenState::Closed)
	{
		InvokeTransitionCallbackImmediate(MoveTemp(InNativeComplete), InDynamicComplete);
		return;
	}

	// 이미 닫기 전환 진행 중일 때의 처리
	if (OpenState == EBBMWidgetOpenState::Closing)
	{
		EnqueuePendingCallbacks(
			PendingCloseAnimDynamicCallbacks,
			PendingCloseAnimCallbacks,
			MoveTemp(InNativeComplete),
			InDynamicComplete
		);

		// 무애님 Close() 요청 시 진행 중 애니메이션을 즉시 스냅 완료하고 누적 콜백까지 일괄 호출
		if (bWithAnim == false)
		{
			if (ClosePresentationHandler.IsBound() == true)
			{
				ClosePresentationHandler.Execute(false);
			}
			else
			{
				NotifyCloseTransitionFinished();
			}
		}
		return;
	}

	ClearPendingCallbacks(PendingCloseAnimDynamicCallbacks, PendingCloseAnimCallbacks);

	if (bWithAnim == true)
	{
		EnqueuePendingCallbacks(
			PendingCloseAnimDynamicCallbacks,
			PendingCloseAnimCallbacks,
			MoveTemp(InNativeComplete),
			InDynamicComplete
		);
	}

	// 애니메이션 진행 여부와 무관하게 세부 상태 전이 및 Closed 이벤트는 즉시 반영/호출
	SetOpenState(EBBMWidgetOpenState::Closing);

	OnClosed();
	BP_OnClosed();

	// 훅 실행 중 상태가 다른 전이로 변경된 경우 후속 프레젠테이션 요청 중단
	if (OpenState != EBBMWidgetOpenState::Closing)
	{
		return;
	}

	// 연결된 View가 있으면 프레젠테이션 요청을 전달하고, 없으면 즉시 완료 처리
	if (ClosePresentationHandler.IsBound() == true)
	{
		ClosePresentationHandler.Execute(bWithAnim);
	}
	else
	{
		NotifyCloseTransitionFinished();
	}
}

void UBBMViewModelBase::InvokeTransitionCallbackImmediate(
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

void UBBMViewModelBase::EnqueuePendingCallbacks(
	TArray<FBBMOnWidgetAnimFinishedDynamicSignature>& TargetDynamicCallbacks,
	TArray<FBBMOnWidgetAnimFinishedSignature>& TargetNativeCallbacks,
	FBBMOnWidgetAnimFinishedSignature&& InNativeComplete,
	const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
)
{
	if (InDynamicComplete.IsBound() == true)
	{
		TargetDynamicCallbacks.Add(InDynamicComplete);
	}

	if (InNativeComplete.IsBound() == true)
	{
		TargetNativeCallbacks.Add(MoveTemp(InNativeComplete));
	}
}

void UBBMViewModelBase::ClearPendingCallbacks(
	TArray<FBBMOnWidgetAnimFinishedDynamicSignature>& TargetDynamicCallbacks,
	TArray<FBBMOnWidgetAnimFinishedSignature>& TargetNativeCallbacks
)
{
	TargetDynamicCallbacks.Reset();
	TargetNativeCallbacks.Reset();
}

void UBBMViewModelBase::ExecutePendingCallbacks(
	TArray<FBBMOnWidgetAnimFinishedDynamicSignature>& TargetDynamicCallbacks,
	TArray<FBBMOnWidgetAnimFinishedSignature>& TargetNativeCallbacks
)
{
	// 콜백 재진입 시 안전을 위해 로컬 배열로 이동 후 순차 실행
	TArray<FBBMOnWidgetAnimFinishedDynamicSignature> DynamicCallbacks = MoveTemp(TargetDynamicCallbacks);
	TArray<FBBMOnWidgetAnimFinishedSignature> NativeCallbacks = MoveTemp(TargetNativeCallbacks);
	TargetDynamicCallbacks.Reset();
	TargetNativeCallbacks.Reset();

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

void UBBMViewModelBase::SetOpenState(EBBMWidgetOpenState InOpenState)
{
	const bool bWasOpen = IsOpen();
	const bool bWasShowing = IsShowing();

	if (UE_MVVM_SET_PROPERTY_VALUE(OpenState, InOpenState) == false)
	{
		return;
	}

	if (bWasOpen != IsOpen())
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsOpen);
	}

	if (bWasShowing != IsShowing())
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsShowing);
	}
}
