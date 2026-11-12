/*****************************************************************//**
 * @file   BBMViewModelBase.h
 * @brief  BBM(비트 배낭 상인) 공용 MVVM 뷰모델 베이스 클래스 정의
 * @details 언리얼 엔진 공식 ModelViewViewModel 플러그인을 기반으로 단일 진실 공급원 및 Open/Close 제어 주체 역할을 수행함
 * @date   2026-11-07
 **********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "BBMViewModelBase.generated.h"

/* 프레젠테이션 전환 요청 대리자 선언 (ViewModel -> View 1:1 핸드셰이크) */
DECLARE_DELEGATE_OneParam(FBBMOnPresentationTransitionRequestedSignature, bool /*bWithAnim*/);

/**
 * BBM 공용 MVVM 뷰모델 베이스 클래스
 * 프로젝트 내 모든 MVVM 뷰모델의 최상위 부모로서 Open/Close 제어, 상태 관리(FieldNotify) 및 프레젠테이션 핸드셰이크를 담당함
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisplayName = "BBM ViewModel Base"))
class BBM_API UBBMViewModelBase : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMViewModelBase();

	/* 오픈 및 클로즈 제어 인터페이스 */
public:
	/**
	 * UI를 즉시 엽니다 (논리 상태 bIsOpen을 즉시 true로 갱신하고 연결된 View에 표시 요청)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	virtual void Open();

	/**
	 * UI를 즉시 닫습니다 (논리 상태 bIsOpen을 즉시 false로 갱신하고 연결된 View에 숨김/탈착 요청)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	virtual void Close();

	/**
	 * 오픈 애니메이션 재생과 함께 UI를 열고, 애니메이션 완료 시 대리자를 호출합니다 (블루프린트용)
	 * @param OnComplete 오픈 애니메이션 완료 시 호출될 대리자
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM", meta = (DisplayName = "Open With Anim"))
	virtual void OpenWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete);

	/**
	 * C++ 전용 OpenWithAnim 오버로딩
	 * @param OnComplete 오픈 애니메이션 완료 시 호출될 대리자
	 */
	virtual void OpenWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete = FBBMOnWidgetAnimFinishedSignature());

	/**
	 * 클로즈 애니메이션 재생과 함께 UI를 닫고, 애니메이션 완료 시 대리자를 호출합니다 (블루프린트용)
	 * @param OnComplete 클로즈 애니메이션 완료 시 호출될 대리자
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM", meta = (DisplayName = "Close With Anim"))
	virtual void CloseWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete);

	/**
	 * C++ 전용 CloseWithAnim 오버로딩
	 * @param OnComplete 클로즈 애니메이션 완료 시 호출될 대리자
	 */
	virtual void CloseWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete = FBBMOnWidgetAnimFinishedSignature());

	/* 상태 조회 */
public:
	/**
	 * 현재 UI의 논리적 열림 여부 반환 (Opening 또는 Open 상태일 때 true, Closing 또는 Closed 상태일 때 false)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|MVVM")
	bool IsOpen() const
	{
		return (OpenState == EBBMWidgetOpenState::Opening || OpenState == EBBMWidgetOpenState::Open);
	}

	/**
	 * 현재 UI가 화면에 표시 중(또는 전환 애니메이션 진행 중)인지 여부 반환 (Closed가 아닐 때 true)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|MVVM")
	bool IsShowing() const
	{
		return (OpenState != EBBMWidgetOpenState::Closed);
	}

	/**
	 * 애니메이션 전환 단계를 포함한 세부 열림 상태 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|MVVM")
	EBBMWidgetOpenState GetOpenState() const { return OpenState; }

	/* View 프레젠테이션 핸드셰이크 (UBBMUserWidgetBase 연동용) */
public:
	/**
	 * 단일 주 View(UBBMUserWidgetBase)의 프레젠테이션 요청 핸들러를 바인딩합니다
	 * @param InOwner 바인딩 주체 위젯 포인터
	 * @param InOpenHandler 오픈 요청 수신 델리게이트
	 * @param InCloseHandler 클로즈 요청 수신 델리게이트
	 */
	void BindPresentationHandlers(
		const UObject* InOwner,
		FBBMOnPresentationTransitionRequestedSignature InOpenHandler,
		FBBMOnPresentationTransitionRequestedSignature InCloseHandler
	);

	/**
	 * 지정된 주체가 바인딩한 프레젠테이션 요청 핸들러를 안전하게 해제합니다
	 * @param InOwner 기존 바인딩 주체 위젯 포인터
	 */
	void ClearPresentationHandlers(const UObject* InOwner);

	/**
	 * View 측에서 오픈 프레젠테이션 및 애니메이션 재생이 완료되었음을 회신합니다
	 */
	void NotifyOpenTransitionFinished();

	/**
	 * View 측에서 클로즈 프레젠테이션 및 애니메이션 재생이 완료되었음을 회신합니다
	 */
	void NotifyCloseTransitionFinished();

	/* 오픈/클로즈 및 애니메이션 완료 수명 주기 훅 */
protected:
	/**
	 * Open/OpenWithAnim 호출 직후 즉시 호출되는 C++ 가상 함수 (OpenState == Opening 시점)
	 */
	virtual void OnOpen();

	/**
	 * Close/CloseWithAnim 호출 직후 즉시 호출되는 C++ 가상 함수 (OpenState == Closing 시점)
	 */
	virtual void OnClosed();

	/**
	 * 오픈 애니메이션 완료 시(애니메이션 미설정 시 즉시) 호출되는 C++ 가상 함수
	 */
	virtual void OnFinishOpenAnim();

	/**
	 * 클로즈 애니메이션 완료 시(애니메이션 미설정 시 즉시) 호출되는 C++ 가상 함수
	 */
	virtual void OnFinishCloseAnim();

	/**
	 * Open/OpenWithAnim 호출 직후 즉시 호출되는 블루프린트 이벤트
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI|MVVM", meta = (DisplayName = "On Open"))
	void BP_OnOpen();

	/**
	 * Close/CloseWithAnim 호출 직후 즉시 호출되는 블루프린트 이벤트
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI|MVVM", meta = (DisplayName = "On Closed"))
	void BP_OnClosed();

	/**
	 * 오픈 애니메이션 완료 시(애니메이션 미설정 시 즉시) 호출되는 블루프린트 이벤트
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI|MVVM", meta = (DisplayName = "On Finish Open Anim"))
	void BP_OnFinishOpenAnim();

	/**
	 * 클로즈 애니메이션 완료 시(애니메이션 미설정 시 즉시) 호출되는 블루프린트 이벤트
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI|MVVM", meta = (DisplayName = "On Finish Close Anim"))
	void BP_OnFinishCloseAnim();

	/* 내부 실행 및 상태 갱신 헬퍼 */
private:
	/* 공통 오픈 진입 처리 */
	void ExecuteOpen(
		bool bWithAnim,
		FBBMOnWidgetAnimFinishedSignature InNativeComplete,
		const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
	);

	/* 공통 클로즈 진입 처리 */
	void ExecuteClose(
		bool bWithAnim,
		FBBMOnWidgetAnimFinishedSignature InNativeComplete,
		const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
	);

	/* 전환 완료 콜백 즉시 실행 */
	static void InvokeTransitionCallbackImmediate(
		FBBMOnWidgetAnimFinishedSignature&& InNativeComplete,
		const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
	);

	/* 대기 콜백 큐에 대리자 추가 */
	static void EnqueuePendingCallbacks(
		TArray<FBBMOnWidgetAnimFinishedDynamicSignature>& TargetDynamicCallbacks,
		TArray<FBBMOnWidgetAnimFinishedSignature>& TargetNativeCallbacks,
		FBBMOnWidgetAnimFinishedSignature&& InNativeComplete,
		const FBBMOnWidgetAnimFinishedDynamicSignature& InDynamicComplete
	);

	/* 대기 중인 콜백 큐 비우기 */
	static void ClearPendingCallbacks(
		TArray<FBBMOnWidgetAnimFinishedDynamicSignature>& TargetDynamicCallbacks,
		TArray<FBBMOnWidgetAnimFinishedSignature>& TargetNativeCallbacks
	);

	/* 대기 중인 콜백 큐 일괄 실행 및 정리 */
	static void ExecutePendingCallbacks(
		TArray<FBBMOnWidgetAnimFinishedDynamicSignature>& TargetDynamicCallbacks,
		TArray<FBBMOnWidgetAnimFinishedSignature>& TargetNativeCallbacks
	);

	/* 세부 전환 상태 설정 및 FieldNotify 통지 (파생 IsOpen/IsShowing 포함) */
	void SetOpenState(EBBMWidgetOpenState InOpenState);

	/* 멤버 변수 */
private:
	/* 애니메이션 전환 단계를 포함한 세부 열림 상태 */
	UPROPERTY(Category = "BBM|UI|MVVM", BlueprintReadOnly, FieldNotify, Getter = GetOpenState, meta = (AllowPrivateAccess = "true"))
	EBBMWidgetOpenState OpenState = EBBMWidgetOpenState::Closed;

	/* 현재 프레젠테이션 핸들러를 등록한 소유 위젯 약참조 */
	TWeakObjectPtr<const UObject> BoundPresentationOwner;

	/* View에 오픈 프레젠테이션을 요청하는 1:1 델리게이트 */
	FBBMOnPresentationTransitionRequestedSignature OpenPresentationHandler;

	/* View에 클로즈 프레젠테이션을 요청하는 1:1 델리게이트 */
	FBBMOnPresentationTransitionRequestedSignature ClosePresentationHandler;

	/* 대기 중인 오픈 애니메이션 완료 블루프린트 대리자 목록 */
	TArray<FBBMOnWidgetAnimFinishedDynamicSignature> PendingOpenAnimDynamicCallbacks;

	/* 대기 중인 오픈 애니메이션 완료 C++ 대리자 목록 */
	TArray<FBBMOnWidgetAnimFinishedSignature> PendingOpenAnimCallbacks;

	/* 대기 중인 클로즈 애니메이션 완료 블루프린트 대리자 목록 */
	TArray<FBBMOnWidgetAnimFinishedDynamicSignature> PendingCloseAnimDynamicCallbacks;

	/* 대기 중인 클로즈 애니메이션 완료 C++ 대리자 목록 */
	TArray<FBBMOnWidgetAnimFinishedSignature> PendingCloseAnimCallbacks;
};
