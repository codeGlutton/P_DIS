/*****************************************************************//**
 * @file   BBMUserWidgetBase.h
 * @brief  BBM(비트 배낭 상인) 공용 사용자 위젯 베이스 클래스 정의
 * @details 단일 ViewModel 기반 Open/Close 프레젠테이션 수신, 범용 애니메이션 전환 헬퍼 및 뷰포트 수명 주기 최적화 제공
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "BBMUserWidgetBase.generated.h"

/* 독립 전방 선언 */
class UBBMViewModelBase;
class UBBMSingleAnimTransition;
class UBBMDualAnimTransition;
class UBBMWorldUISubsystem;
struct FWidgetAnimationState;

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

	/* 레이어 부착 및 최상단 ZOrder 관리 */
public:
	/**
	 * UBBMWorldUISubsystem의 루트 위젯 내 지정된 ZOrder 레이어 캔버스 자식으로 추가함
	 * (이미 뷰포트 또는 레이어 위에 올라가 있거나 서브시스템이 없으면 false 반환)
	 * @param InLayer 배치할 대상 ZOrder 레이어
	 * @return 레이어 추가 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Layer")
	bool AddToLayer(EBBMWidgetZOrder InLayer);

	/**
	 * 현재 소속된 레이어 캔버스 내에서 이 위젯의 ZOrder를 최상단으로 승격함
	 * @return 최상단 이동 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Layer")
	bool BringToLayerTop();

	/**
	 * 위젯이 뷰포트에 직접 등록되었거나 루트 레이어 캔버스 내부에 위치하는지 여부를 반환함
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|Layer")
	bool IsInLayer() const;

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
	UBBMViewModelBase* GetViewModel() const { return ViewModel.Get(); }

	/**
	 * 연결된 단일 ViewModel 변수를 지정 타입으로 캐스팅하여 반환하는 C++ 템플릿 헬퍼
	 */
	template<typename T>
	T* GetViewModelAs() const
	{
		return Cast<T>(ViewModel.Get());
	}

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

	/* 뷰모델 프레젠테이션 바인딩 확장 가상 함수 */
protected:
	/**
	 * 뷰모델 인스턴스 주입 시 프레젠테이션 및 애니메이션 전환 핸들러를 일괄 등록하는 확장 가상 함수
	 * @param InViewModel 새로 연결되는 뷰모델 인스턴스 (non-null 보장)
	 */
	virtual void BindViewModelPresentationHandlers(UBBMViewModelBase* InViewModel);

	/**
	 * 뷰모델 인스턴스 해제 시 프레젠테이션 및 애니메이션 전환 핸들러를 일괄 해제하는 확장 가상 함수
	 * @param InViewModel 연결 해제 대상 뷰모델 인스턴스 (non-null 보장)
	 */
	virtual void ClearViewModelPresentationHandlers(UBBMViewModelBase* InViewModel);

	/* 범용 애니메이션 재생 및 전환 헬퍼 */
protected:
	/**
	 * 단일 애니메이션 전환 모듈(UBBMSingleAnimTransition) 연동 전용 범용 요청 핸들러
	 * @param bWithAnim 애니메이션 재생 여부
	 * @param InTransition 완료 통지를 수신할 단일 전환 모듈
	 * @param InConfig 애니메이션 설정
	 */
	void HandleSingleTransitionRequested(
		bool bWithAnim,
		UBBMSingleAnimTransition* InTransition,
		const FBBMWidgetAnimConfig& InConfig
	);

	/**
	 * 이중 애니메이션 전환 모듈(UBBMDualAnimTransition)의 Start(열기/시작) 전용 범용 요청 핸들러
	 * @param bWithAnim 애니메이션 재생 여부
	 * @param InDualTransition 완료 통지를 수신할 이중 전환 모듈
	 * @param InStartConfig 시작 애니메이션 설정
	 * @param InEndConfig 종료 애니메이션 설정
	 */
	void HandleDualTransitionStartRequested(
		bool bWithAnim,
		UBBMDualAnimTransition* InDualTransition,
		const FBBMWidgetAnimConfig& InStartConfig,
		const FBBMWidgetAnimConfig& InEndConfig
	);

	/**
	 * 이중 애니메이션 전환 모듈(UBBMDualAnimTransition)의 End(닫기/종료) 전용 범용 요청 핸들러
	 * @param bWithAnim 애니메이션 재생 여부
	 * @param InDualTransition 완료 통지를 수신할 이중 전환 모듈
	 * @param InStartConfig 시작 애니메이션 설정
	 * @param InEndConfig 종료 애니메이션 설정
	 */
	void HandleDualTransitionEndRequested(
		bool bWithAnim,
		UBBMDualAnimTransition* InDualTransition,
		const FBBMWidgetAnimConfig& InStartConfig,
		const FBBMWidgetAnimConfig& InEndConfig
	);

	/* 오픈/클로즈 프레젠테이션 요청 핸들러 및 완료 훅 */
private:
	/* ViewModel의 오픈 요청 수신 핸들러 (기본: 가시성 켜기 + HandleDualTransitionStartRequested) */
	void HandleOpenPresentationRequested(bool bWithAnim);

	/* ViewModel의 클로즈 요청 수신 핸들러 (기본: HandleDualTransitionEndRequested + 가시성 끄기) */
	void HandleClosePresentationRequested(bool bWithAnim);

	/* 오픈 애니메이션 종료 직후(완료 통지 직전) 호출 */
	UFUNCTION()
	void HandleOpenAnimationFinished();

	/* 클로즈 애니메이션 종료 직후(가시성 숨김 직후, 완료 통지 직전) 호출 */
	UFUNCTION()
	void HandleCloseAnimationFinished();

	/* 내부 헬퍼 */
private:
	/* 현재 월드에서 UBBMWorldUISubsystem을 조회하는 공통 헬퍼 */
	UBBMWorldUISubsystem* ResolveWorldUISubsystem() const;

	/* 수명 주기 정책에 따른 레이어 등록, 최상단 ZOrder 승격 및 가시성 표시 적용 */
	void ApplyOpenViewportPresentation();

	/* 수명 주기 정책에 따른 레이어 분리 또는 가시성 숨김 적용 */
	void ApplyCloseViewportPresentation();

	/* 애니메이션 설정(EBBMWidgetAnimPlayMode, 속도, 반복 횟수)에 따른 UMG 분기 재생 헬퍼 */
	void PlayAnimationByConfig(const FBBMWidgetAnimConfig& InConfig);

	/* 설정 및 연결 멤버 변수 */
protected:
	/* 위젯 최적화 수명 주기 방식 */
	UPROPERTY(Category = "BBM|UI|Optimization", EditDefaultsOnly, BlueprintReadOnly)
	EBBMWidgetLifecyclePolicy LifecyclePolicy;

	/* 기본 뷰포트 ZOrder 레이어 */
	UPROPERTY(Category = "BBM|UI|Optimization", EditDefaultsOnly, BlueprintReadOnly)
	EBBMWidgetZOrder DefaultZOrderLayer;

	/* 오픈 전용 애니메이션 설정 [애니메이션 이름, 재생 방향, 속도] */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	FBBMWidgetAnimConfig OpenAnimConfig;

	/* 클로즈 전용 애니메이션 설정 [애니메이션 이름, 재생 방향, 속도] */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	FBBMWidgetAnimConfig CloseAnimConfig;

private:
	/* 현재 위젯에 바인딩된 단일 ViewModel 인스턴스 (비소유 약참조) */
	UPROPERTY(Category = "BBM|UI|MVVM", Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<UBBMViewModelBase> ViewModel;
};
