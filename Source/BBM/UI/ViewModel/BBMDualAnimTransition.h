/*****************************************************************//**
 * @file   BBMDualAnimTransition.h
 * @brief  Start/End 양방향 UI 애니메이션 전환 조합 모듈 정의
 * @details 단일 애님 전환 모듈 2개(StartTransition/EndTransition)를 내부에 캡슐화해 EBBMDualAnimState, IsStarted/IsActive 및 상호 취소 규칙을 관리함
 * @date   2026-11-13
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "BBMDualAnimTransition.generated.h"

class UBBMSingleAnimTransition;

/**
 * Start와 End 한 쌍의 애니메이션 전환을 조율하는 경량 MVVM 컴포짓 모듈
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Dual Anim Transition"))
class BBM_API UBBMDualAnimTransition : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMDualAnimTransition();

	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

	/* 상태 조회 (FieldNotify) */
public:
	/**
	 * 애니메이션 전환 단계를 포함한 양방향 세부 상태 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|MVVM")
	EBBMDualAnimState GetState() const { return State; }

	/**
	 * 현재 시작 논리 상태 여부 반환 (Starting 또는 Started면 true, Ending 또는 Ended면 false)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|MVVM")
	bool IsStarted() const
	{
		return (State == EBBMDualAnimState::Starting || State == EBBMDualAnimState::Started);
	}

	/**
	 * 현재 전환이 시작된 이후 완전히 종료되기 전인지 여부 반환 (Ended가 아닐 때 true)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|MVVM")
	bool IsActive() const
	{
		return (State != EBBMDualAnimState::Ended);
	}

	/* Start/End 제어 및 초기화 인터페이스 */
public:
	/**
	 * 애니메이션 없이 즉시 시작 전환을 수행함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	void Start();

	/**
	 * 애니메이션 없이 즉시 종료 전환을 수행함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	void End();

	/**
	 * 시작 애니메이션과 함께 Start 전환을 실행하고 완료 대리자를 등록함 (블루프린트용)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM", meta = (DisplayName = "Start With Anim"))
	void StartWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete);

	/**
	 * C++ 전용 StartWithAnim 실행 인터페이스
	 */
	void StartWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete = FBBMOnWidgetAnimFinishedSignature());

	/**
	 * 종료 애니메이션과 함께 End 전환을 실행하고 완료 대리자를 등록함 (블루프린트용)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM", meta = (DisplayName = "End With Anim"))
	void EndWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete);

	/**
	 * C++ 전용 EndWithAnim 실행 인터페이스
	 */
	void EndWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete = FBBMOnWidgetAnimFinishedSignature());

	/**
	 * C++ 통합 시작 진입점
	 */
	void ExecuteStart(
		bool bWithAnim,
		FBBMOnWidgetAnimFinishedSignature InNativeComplete,
		const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
	);

	/**
	 * C++ 통합 종료 진입점
	 */
	void ExecuteEnd(
		bool bWithAnim,
		FBBMOnWidgetAnimFinishedSignature InNativeComplete,
		const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
	);

	/**
	 * 양방향 대기 콜백을 비우고 State를 Ended로 초기화함
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	void Clear();

	/**
	 * 상태 변경 없이 양방향 대기 콜백만 비움
	 */
	void ClearPendingCallbacks();

	/* 위젯 핸드셰이크 위임 인터페이스 */
public:
	/**
	 * Start 및 End 전환 요청을 수신할 위젯 핸들러 쌍을 바인딩함
	 */
	void BindPresentationHandlers(
		const UObject* InOwner,
		FBBMOnPresentationTransitionRequestedSignature InStartHandler,
		FBBMOnPresentationTransitionRequestedSignature InEndHandler
	);

	/**
	 * 지정 소유자가 바인딩한 양방향 핸들러를 안전하게 해제함
	 */
	void ClearPresentationHandlers(const UObject* InOwner);

	/**
	 * View 측 시작 프레젠테이션·애니메이션 완료 시 호출
	 */
	UFUNCTION()
	void NotifyStartTransitionFinished();

	/**
	 * View 측 종료 프레젠테이션·애니메이션 완료 시 호출
	 */
	UFUNCTION()
	void NotifyEndTransitionFinished();

	/* 내부 전이 조율 헬퍼 */
private:
	void ExecuteDirectionalTransition(
		UBBMSingleAnimTransition* TargetSlot,
		UBBMSingleAnimTransition* OppositeSlot,
		EBBMDualAnimState CompletedState,
		EBBMDualAnimState TransitioningState,
		FSimpleMulticastDelegate& OnTransitioningNative,
		bool bWithAnim,
		FBBMOnWidgetAnimFinishedSignature&& InNativeComplete,
		const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
	);

	void HandleStartSlotFinishedBeforeCallbacks();
	void HandleEndSlotFinishedBeforeCallbacks();
	void SetState(EBBMDualAnimState InNewState);

	/* 상위 ViewModel 연동용 수명 주기 C++ 훅 */
public:
	/* Starting 진입 직후 발송되는 시작 훅 */
	FSimpleMulticastDelegate OnStartingNative;

	/* Ending 진입 직후 발송되는 종료 시작 훅 */
	FSimpleMulticastDelegate OnEndingNative;

	/* Started 상태 확정 직후, 대기 콜백 실행 직전에 발송되는 완료 훅 */
	FSimpleMulticastDelegate OnStartedNative;

	/* Ended 상태 확정 직후, 대기 콜백 실행 직전에 발송되는 완료 훅 */
	FSimpleMulticastDelegate OnEndedNative;

	/* 블루프린트 연동용 동적 멀티캐스트 대리자 */
public:
	/* Starting 진입 직후 발송되는 시작 블루프린트 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "BBM|UI|MVVM|Event")
	FBBMOnTransitionEventDynamicSignature OnStarting;

	/* Ending 진입 직후 발송되는 종료 시작 블루프린트 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "BBM|UI|MVVM|Event")
	FBBMOnTransitionEventDynamicSignature OnEnding;

	/* Started 상태 확정 직후, 대기 콜백 실행 직전에 발송되는 완료 블루프린트 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "BBM|UI|MVVM|Event")
	FBBMOnTransitionEventDynamicSignature OnStarted;

	/* Ended 상태 확정 직후, 대기 콜백 실행 직전에 발송되는 완료 블루프린트 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "BBM|UI|MVVM|Event")
	FBBMOnTransitionEventDynamicSignature OnEnded;

	/* 멤버 변수 */
private:
	/* 애니메이션 전환 단계를 포함한 양방향 세부 상태 */
	UPROPERTY(Category = "BBM|UI|MVVM", BlueprintReadOnly, FieldNotify, Getter = GetState, meta = (AllowPrivateAccess = "true"))
	EBBMDualAnimState State = EBBMDualAnimState::Ended;

	/* Start 전환 담당 내부 단일 애님 모듈 (외부·BP 비공개 캡슐화, GC 보호 전용) */
	UPROPERTY()
	TObjectPtr<UBBMSingleAnimTransition> StartTransition;

	/* End 전환 담당 내부 단일 애님 모듈 (외부·BP 비공개 캡슐화, GC 보호 전용) */
	UPROPERTY()
	TObjectPtr<UBBMSingleAnimTransition> EndTransition;
};
