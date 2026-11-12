/*****************************************************************//**
 * @file   BBMUserWidgetBase.h
 * @brief  BBM(비트 배낭 상인) 공용 사용자 위젯 베이스 클래스 정의
 * @details 단일 ViewModel 기반 Open/Close 프레젠테이션 수신, 애니메이션 재생 설정 및 뷰포트 수명 주기 최적화 제공
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "BBMUserWidgetBase.generated.h"

/* 독립 전방 선언 */
class UBBMViewModelBase;

/**
 * BBM 공용 사용자 위젯 베이스 클래스
 * 단일 ViewModel(UBBMViewModelBase)로부터 열림/닫힘 프레젠테이션 요청을 전달받아 뷰포트, 가시성 및 애니메이션을 처리함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM User Widget Base"))
class BBM_API UBBMUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()

	/* 생성자 및 UMG 수명 주기 */
public:
	UBBMUserWidgetBase(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void BeginDestroy() override;

	/* 상태 및 설정 조회 */
public:
	/* 최적화 수명 주기 정책 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI")
	EBBMWidgetLifecyclePolicy GetLifecyclePolicy() const { return LifecyclePolicy; }

	/* 기본 ZOrder 레이어 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI")
	EBBMWidgetZOrder GetDefaultZOrderLayer() const { return DefaultZOrderLayer; }

	/* 기본 ZOrder 레이어 설정 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	void SetDefaultZOrderLayer(EBBMWidgetZOrder InZOrderLayer) { DefaultZOrderLayer = InZOrderLayer; }

	/* 단일 ViewModel 연결 및 조회 인터페이스 */
public:
	/**
	 * 위젯에 단일 뷰모델 인스턴스를 수동(Manual) 주입하고 프레젠테이션·MVVMView 확장에 바인딩합니다
	 * @param InViewModel 주입할 뷰모델 인스턴스 (nullptr 전달 시 기존 연결 해제)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	void SetViewModelInstance(UBBMViewModelBase* InViewModel);

	/**
	 * 현재 위젯에 연결된 단일 뷰모델 인스턴스를 반환합니다
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|MVVM")
	UBBMViewModelBase* GetViewModel() const { return ViewModel; }

	/**
	 * 연결된 단일 ViewModel 변수를 지정 타입으로 캐스팅하여 반환하는 C++ 템플릿 헬퍼
	 */
	template<typename T>
	T* GetViewModelAs() const
	{
		return Cast<T>(ViewModel.Get());
	}

	/**
	 * 서브시스템 등에서 위젯 단독 제거 시 뷰포트에서 즉시 탈착/숨김 처리합니다
	 */
	void DetachPresentationImmediate();

	/* 프레젠테이션 이벤트 확장 */
protected:
	/* View 프레젠테이션이 열렸을 때 호출되는 블루프린트 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI", meta = (DisplayName = "On Opened"))
	void BP_OnOpened();

	/* View 프레젠테이션이 닫혔을 때 호출되는 블루프린트 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI", meta = (DisplayName = "On Closed"))
	void BP_OnClosed();

	/**
	 * 뷰모델이 주입되었을 때 블루프린트에서 추가 바인딩이나 처리를 수행할 수 있는 이벤트
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI|MVVM", meta = (DisplayName = "On ViewModel Assigned"))
	void BP_OnViewModelAssigned(UBBMViewModelBase* InViewModel);

	virtual void OnOpened();
	virtual void OnClosed();

	/* 내부 프레젠테이션 요청 및 애니메이션 콜백 핸들러 */
private:
	/* ViewModel의 오픈 요청 수신 핸들러 */
	void HandleOpenPresentationRequested(bool bWithAnim);

	/* ViewModel의 클로즈 요청 수신 핸들러 */
	void HandleClosePresentationRequested(bool bWithAnim);

	/* 수명 주기 정책에 따른 뷰포트 등록 및 가시성 표시 적용 */
	void ApplyOpenViewportPresentation();

	/* 게임 뷰포트가 유효한 경우에만 AddToViewport를 수행하는 안전 헬퍼 */
	void TryAddToGameViewport();

	/* 수명 주기 정책에 따른 뷰포트 탈착 또는 가시성 숨김 적용 */
	void ApplyCloseViewportPresentation();

	UFUNCTION()
	void HandleOpenAnimationFinished();

	UFUNCTION()
	void HandleCloseAnimationFinished();

	/* 현재 등록된 위젯 애니메이션 완료 대리자 바인딩 해제 및 필요 시 재생 중단 */
	void ClearAnimationFinishedBindings(bool bStopRunningAnimations);

	/* 애니메이션 설정(EBBMWidgetAnimPlayMode, 속도)에 따른 UMG 분기 재생 헬퍼 */
	void PlayAnimationByConfig(UWidgetAnimation* InAnim, const FBBMWidgetAnimConfig& InConfig);

	/* 설정 및 연결 멤버 변수 */
protected:
	/* 위젯 최적화 수명 주기 방식 */
	UPROPERTY(Category = "BBM|UI|Optimization", EditDefaultsOnly, BlueprintReadOnly)
	EBBMWidgetLifecyclePolicy LifecyclePolicy;

	/* 기본 뷰포트 ZOrder 레이어 */
	UPROPERTY(Category = "BBM|UI|Optimization", EditDefaultsOnly, BlueprintReadOnly)
	EBBMWidgetZOrder DefaultZOrderLayer;

	/* 오픈 전용 애니메이션 설정 [에셋 포인터, 재생 방향, 속도] */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	FBBMWidgetAnimConfig OpenAnimConfig;

	/* 클로즈 전용 애니메이션 설정 [에셋 포인터, 재생 방향, 속도] */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	FBBMWidgetAnimConfig CloseAnimConfig;

private:
	/* 현재 위젯에 바인딩된 단일 ViewModel 인스턴스 */
	UPROPERTY(Category = "BBM|UI|MVVM", Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMViewModelBase> ViewModel;
};
