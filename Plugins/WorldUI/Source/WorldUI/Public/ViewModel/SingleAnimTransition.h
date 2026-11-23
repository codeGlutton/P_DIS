/*****************************************************************//**
 * @file   SingleAnimTransition.h
 * @brief  단일 UI 애니메이션 상태·다중 콜백·위젯 핸드셰이크 전환 모듈 정의
 * @details FieldNotify State, 다중 완료 콜백 큐, 위젯 프레젠테이션 대리자, Execute/Clear/NotifyAnimFinished를 자체 완결 형태로 제공함
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "WorldUITypes.h"
#include "SingleAnimTransition.generated.h"

/**
 * 단일 UI 애니메이션의 상태(FieldNotify), 대기 콜백 및 위젯 핸드셰이크를 전담하는 경량 MVVM 모듈
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Single Anim Transition"))
class WORLDUI_API USingleAnimTransition : public UMVVMViewModelBase
{
	GENERATED_BODY()

	friend class UDualAnimTransition;

	/* 생성자 */
public:
	USingleAnimTransition();

	/* 상태 조회 및 설정 (FieldNotify) */
public:
	/**
	 * 현재 단일 애니메이션 전환 상태 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "WorldUI|MVVM")
	ESingleAnimState GetState() const { return State; }

	/**
	 * 현재 단일 애니메이션이 재생(진행) 중인지 여부 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "WorldUI|MVVM")
	bool IsPlaying() const { return (State == ESingleAnimState::Playing); }

	/**
	 * 단일 애니메이션 상태를 변경하고 실제 변화 시 FieldNotify 및 네이티브 통지를 발송
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldUI|MVVM")
	void SetState(ESingleAnimState InNewState);

	/* 실행, 초기화 및 위젯 애님 종료 콜백 */
public:
	/**
	 * 단일 애니메이션 전환을 실행하고 완료 시 호출할 블루프린트 콜백을 등록함
	 * 이미 Playing 상태인 경우 신규 콜백을 누적하며, bWithAnim == false 요청 시 즉시 완료 스냅을 수행함
	 * @param bWithAnim 애니메이션 재생 여부 (false면 즉시 완료 처리 요청)
	 * @param OnComplete 전환 완료 시 실행될 블루프린트 대리자
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldUI|MVVM", meta = (DisplayName = "Execute"))
	void ExecuteDynamic(bool bWithAnim, FWorldUIOnWidgetAnimFinishedDynamicSignature OnComplete);

	/**
	 * C++ 전용 단일 애니메이션 전환 실행 인터페이스
	 * @param bWithAnim 애니메이션 재생 여부 (false면 즉시 완료 처리 요청)
	 * @param OnComplete 전환 완료 시 실행될 C++ 대리자
	 */
	void Execute(
		bool bWithAnim = true,
		FWorldUIOnWidgetAnimFinishedSignature OnComplete = FWorldUIOnWidgetAnimFinishedSignature()
	);

	/**
	 * 진행 중이던 전환을 중단 처리하고 대기 콜백에 중단 여부(bInterrupted=true)를 통지한 뒤 상태를 Idle로 초기화함
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldUI|MVVM")
	void Clear();

	/**
	 * 상태 및 훅 통지 없이 대기 중인 완료 콜백 목록만 비움
	 */
	void ClearPendingCallbacks();

	/**
	 * 위젯 측 애니메이션/프레젠테이션 완료 시 호출되어 상태를 Idle로 되돌리고 누적 콜백을 실행함
	 */
	UFUNCTION()
	void NotifyAnimFinished();

	/* 위젯 핸드셰이크 대리자 바인딩 */
public:
	/**
	 * 위젯이 단일 애니메이션 전환 요청을 수신할 대리자를 바인딩함
	 * @param InOwner 바인딩 주체 오브젝트 포인터
	 * @param InHandler 프레젠테이션 요청 수신 델리게이트
	 */
	void BindPresentationHandler(
		const UObject* InOwner,
		FWorldUIOnPresentationTransitionRequestedSignature InHandler
	);

	/**
	 * 지정된 소유자가 바인딩한 프레젠테이션 요청 대리자를 안전하게 해제함
	 * 해제 시점에 Playing 상태라면 상태·대기 콜백 고착을 막기 위해 즉시 완료 확정(NotifyAnimFinished)함
	 * @param InOwner 기존 바인딩 주체 오브젝트 포인터
	 */
	void ClearPresentationHandler(const UObject* InOwner);

	/**
	 * 프레젠테이션 요청 대리자 바인딩 여부 반환
	 */
	bool IsPresentationHandlerBound() const { return PresentationHandler.IsBound(); }

	/* 공용 콜백 유틸리티 */
public:
	/**
	 * 전달된 Native/Dynamic 완료 콜백을 큐에 넣지 않고 즉시 실행함
	 */
	static void InvokeCallbacksImmediate(
		FWorldUIOnWidgetAnimFinishedSignature&& InNativeComplete,
		FWorldUIOnWidgetAnimFinishedDynamicSignature&& InDynamicComplete,
		bool bInterrupted = false
	);

	/* 내부 실행 헬퍼 */
private:
	/* 내부 및 컴포짓 모듈용 통합 실행 진입점 */
	void ExecuteInternal(
		bool bWithAnim,
		FWorldUIOnWidgetAnimFinishedSignature&& InNativeComplete,
		FWorldUIOnWidgetAnimFinishedDynamicSignature&& InDynamicComplete
	);

	void FinishTransitionInternal(bool bInterrupted);

	void EnqueuePendingCallbacks(
		FWorldUIOnWidgetAnimFinishedSignature&& InNativeComplete,
		FWorldUIOnWidgetAnimFinishedDynamicSignature&& InDynamicComplete
	);

	void ExecutePendingCallbacks(bool bInterrupted);

	/* C++ 컴포짓 조합용 네이티브 이벤트 */
public:
	/* Idle -> Playing 전이 직후 발송되는 시작 훅 */
	FWorldUIOnAnimTransitionStartedNativeSignature OnTransitionStartedNative;

	/* Playing -> Idle 상태 전이 직후, 누적 사용자 콜백 실행 직전에 발송되는 완료 훅 */
	FWorldUIOnAnimTransitionFinishedNativeSignature OnFinishedBeforeCallbacksNative;

	/* 블루프린트 연동용 동적 멀티캐스트 대리자 */
public:
	/* Idle -> Playing 전이 직후 발송되는 시작 블루프린트 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "WorldUI|MVVM|Event")
	FWorldUIOnTransitionStartedDynamicSignature OnTransitionStarted;

	/* Playing -> Idle 전이 직후, 대기 콜백 실행 직전에 발송되는 완료 블루프린트 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "WorldUI|MVVM|Event")
	FWorldUIOnTransitionFinishedDynamicSignature OnTransitionFinished;

	/* 멤버 변수 */
private:
	/* 현재 단일 애니메이션 전환 상태 (FieldNotify) */
	UPROPERTY(Category = "WorldUI|MVVM", BlueprintReadOnly, FieldNotify, Getter = GetState, meta = (AllowPrivateAccess = "true"))
	ESingleAnimState State = ESingleAnimState::Idle;

	/* 현재 프레젠테이션 핸들러를 등록한 소유 위젯 약참조 */
	TWeakObjectPtr<const UObject> BoundPresentationOwner;

	/* 위젯에 단일 애니메이션 실행을 요청하는 1:1 대리자 */
	FWorldUIOnPresentationTransitionRequestedSignature PresentationHandler;

	/* 대기 중인 애니메이션 완료 블루프린트 대리자 목록 */
	TArray<FWorldUIOnWidgetAnimFinishedDynamicSignature> PendingDynamicCallbacks;

	/* 대기 중인 애니메이션 완료 C++ 대리자 목록 */
	TArray<FWorldUIOnWidgetAnimFinishedSignature> PendingNativeCallbacks;
};