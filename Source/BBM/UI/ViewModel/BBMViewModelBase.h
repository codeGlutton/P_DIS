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

class UBBMDualAnimTransition;

/**
 * BBM 공용 MVVM 뷰모델 베이스 클래스
 * 프로젝트 내 모든 MVVM 뷰모델의 최상위 부모로서 Open/Close 제어, 상태 관리(FieldNotify) 및 프레젠테이션 핸드셰이크를 담당함
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisplayName = "BBM ViewModel Base"))
class BBM_API UBBMViewModelBase : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMViewModelBase();

	virtual void BeginDestroy() override;

	/* 오픈 및 클로즈 제어 인터페이스 */
public:
	/**
	 * UI를 즉시 엽니다 (논리 상태 bIsOpen을 즉시 true로 갱신하고 연결된 View에 표시 요청)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	void Open();

	/**
	 * UI를 즉시 닫습니다 (논리 상태 bIsOpen을 즉시 false로 갱신하고 연결된 View에 숨김/탈착 요청)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	void Close();

	/**
	 * 오픈 애니메이션 재생과 함께 UI를 열고, 애니메이션 완료 시 대리자를 호출합니다 (블루프린트용)
	 * @param OnComplete 오픈 애니메이션 완료 시 호출될 대리자
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM", meta = (DisplayName = "Open With Anim"))
	void OpenWithAnimDynamic(FBBMOnWidgetAnimFinishedDynamicSignature OnComplete);

	/**
	 * C++ 전용 OpenWithAnim 오버로딩
	 * @param OnComplete 오픈 애니메이션 완료 시 호출될 대리자
	 */
	void OpenWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete = FBBMOnWidgetAnimFinishedSignature());

	/**
	 * 클로즈 애니메이션 재생과 함께 UI를 닫고, 애니메이션 완료 시 대리자를 호출합니다 (블루프린트용)
	 * @param OnComplete 클로즈 애니메이션 완료 시 호출될 대리자
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM", meta = (DisplayName = "Close With Anim"))
	void CloseWithAnimDynamic(FBBMOnWidgetAnimFinishedDynamicSignature OnComplete);

	/**
	 * C++ 전용 CloseWithAnim 오버로딩
	 * @param OnComplete 클로즈 애니메이션 완료 시 호출될 대리자
	 */
	void CloseWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete = FBBMOnWidgetAnimFinishedSignature());

	/* 상태 조회 */
public:
	/**
	 * 현재 UI의 논리적 열림 여부 반환 (Opening 또는 Open 상태일 때 true, Closing 또는 Closed 상태일 때 false)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|MVVM")
	bool IsOpen() const;

	/**
	 * 현재 UI가 화면에 표시 중(또는 전환 애니메이션 진행 중)인지 여부 반환 (Closed가 아닐 때 true)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|MVVM")
	bool IsShowing() const;

	/**
	 * 애니메이션 전환 단계를 포함한 세부 열림 상태 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, FieldNotify, Category = "BBM|UI|MVVM")
	EBBMDualAnimState GetOpenState() const;

	/**
	 * Open/Close 전환 모듈 인스턴스 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|MVVM")
	UBBMDualAnimTransition* GetOpenCloseTransition() const { return OpenCloseTransition; }

	/* 뷰모델 초기화 완료 알림 */
public:
	void FinishInitialization();

	/* 뷰모델 초기화 및 오픈/클로즈 수명 주기 훅 */
protected:
	/**
	 * 실제 뷰모델 인스턴스 초기화 직후 호출되는 C++ 가상 함수
	 */
	virtual void BeginViewModel();
	virtual void EndViewModel();

	/**
	 * Open/OpenWithAnim 호출 직후 즉시 호출되는 C++ 가상 함수 (OpenState == Starting 시점)
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

	/* 내부 전환 모듈 연동 핸들러 */
private:
	void HandleTransitionOpenStarted();
	void HandleTransitionCloseStarted();
	void HandleTransitionOpenFinished();
	void HandleTransitionCloseFinished();
	void HandleTransitionStateChanged(UObject* InObject, UE::FieldNotification::FFieldId InFieldId);
	void HandleTransitionStartedChanged(UObject* InObject, UE::FieldNotification::FFieldId InFieldId);
	void HandleTransitionActiveChanged(UObject* InObject, UE::FieldNotification::FFieldId InFieldId);

	/* 멤버 변수 */
private:
	/* Open/Close 양방향 전환 상태·콜백·핸드셰이크 전담 모듈 */
	UPROPERTY(Category = "BBM|UI|MVVM", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMDualAnimTransition> OpenCloseTransition;

	/* VM의 초기화 여부 */
	bool bIsInitialized = false;
};
