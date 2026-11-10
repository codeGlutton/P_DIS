/*****************************************************************//**
 * @file   BBMUserWidgetBase.h
 * @brief  BBM(비트 배낭 상인) 공용 사용자 위젯 베이스 클래스 정의
 * @details Open/Close 라이프사이클, 애니메이션 재생 설정 및 수명 주기 최적화(상시 뷰포트 / 동적 탈착) 제공
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "BBMUserWidgetBase.generated.h"

/* 델리게이트 선언 */
DECLARE_DYNAMIC_DELEGATE(FBBMOnWidgetAnimFinishedDynamicSignature);
DECLARE_DELEGATE(FBBMOnWidgetAnimFinishedSignature);

/* 독립 전방 선언 */
class UBBMViewModelBase;

/**
 * BBM 공용 사용자 위젯 베이스 클래스
 * 오픈/클로즈 상태 전환, 전용 애니메이션 연동 및 뷰포트 최적화 수명 주기 정책을 제공함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM User Widget Base"))
class BBM_API UBBMUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()

	/* 생성자 및 UMG 수명 주기 */
public:
	UBBMUserWidgetBase(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	/* 오픈 및 클로즈 라이프사이클 인터페이스 */
public:
	/**
	 * 위젯을 엽니다 (최적화 정책에 따라 뷰포트 등록 및 가시성 전환)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	virtual void Open();

	/**
	 * 위젯을 닫습니다 (최적화 정책에 따라 뷰포트 탈착 또는 가시성 숨김)
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	virtual void Close();

	/**
	 * 오픈 애니메이션 재생과 함께 위젯을 열고, 완료 시 대리자를 호출합니다 (블루프린트용)
	 * @param OnComplete 애니메이션 재생 완료 시 호출될 대리자
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI", meta = (DisplayName = "Open With Anim"))
	virtual void OpenWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete);

	/**
	 * C++ 전용 OpenWithAnim 오버로딩
	 */
	virtual void OpenWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete = FBBMOnWidgetAnimFinishedSignature());

	/**
	 * 클로즈 애니메이션 재생 후 위젯을 닫고, 완료 시 대리자를 호출합니다 (블루프린트용)
	 * @param OnComplete 애니메이션 재생 완료 시 호출될 대리자
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI", meta = (DisplayName = "Close With Anim"))
	virtual void CloseWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete);

	/**
	 * C++ 전용 CloseWithAnim 오버로딩
	 */
	virtual void CloseWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete = FBBMOnWidgetAnimFinishedSignature());

	/* 상태 조회 */
public:
	/* 위젯이 현재 열려 있는지 여부 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI")
	bool IsOpen() const { return bIsOpen; }

	/* 최적화 수명 주기 정책 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI")
	EBBMWidgetLifecyclePolicy GetLifecyclePolicy() const { return LifecyclePolicy; }

	/* 기본 ZOrder 레이어 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI")
	EBBMWidgetZOrder GetDefaultZOrderLayer() const { return DefaultZOrderLayer; }

	/* 기본 ZOrder 레이어 설정 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI")
	void SetDefaultZOrderLayer(EBBMWidgetZOrder InZOrderLayer) { DefaultZOrderLayer = InZOrderLayer; }

	/* 오픈/클로즈 이벤트 확장 */
protected:
	/* 위젯이 열렸을 때 호출되는 블루프린트 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI", meta = (DisplayName = "On Opened"))
	void BP_OnOpened();

	/* 위젯이 닫혔을 때 호출되는 블루프린트 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI", meta = (DisplayName = "On Closed"))
	void BP_OnClosed();

	virtual void OnOpened();
	virtual void OnClosed();

	/* 내부 애니메이션 완료 콜백 핸들러 */
private:
	UFUNCTION()
	void HandleOpenAnimationFinished();

	UFUNCTION()
	void HandleCloseAnimationFinished();

	/* 현재 등록된 모든 위젯 애니메이션 완료 대리자 바인딩 해제 */
	void ClearAnimationFinishedBindings();

	/* 애니메이션 설정(EBBMWidgetAnimPlayMode, 속도)에 따른 UMG 분기 재생 헬퍼 */
	void PlayAnimationByConfig(UWidgetAnimation* InAnim, const FBBMWidgetAnimConfig& InConfig);

	FBBMOnWidgetAnimFinishedDynamicSignature PendingOpenAnimDynamicCallback;
	FBBMOnWidgetAnimFinishedSignature PendingOpenAnimCallback;

	FBBMOnWidgetAnimFinishedDynamicSignature PendingCloseAnimDynamicCallback;
	FBBMOnWidgetAnimFinishedSignature PendingCloseAnimCallback;

	/* 설정 프로퍼티 */
protected:
	/* 위젯 최적화 수명 주기 방식 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BBM|UI|Optimization")
	EBBMWidgetLifecyclePolicy LifecyclePolicy;

	/* 기본 뷰포트 ZOrder 레이어 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BBM|UI|Optimization")
	EBBMWidgetZOrder DefaultZOrderLayer;

	/* 오픈 전용 애니메이션 설정 [에셋 포인터, 재생 방향, 속도] */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BBM|UI|Animation")
	FBBMWidgetAnimConfig OpenAnimConfig;

	/* 클로즈 전용 애니메이션 설정 [에셋 포인터, 재생 방향, 속도] */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BBM|UI|Animation")
	FBBMWidgetAnimConfig CloseAnimConfig;

	/* 현재 열려 있는지 여부 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BBM|UI")
	bool bIsOpen;

	/* MVVM 뷰모델 수동(Manual) 바인딩 주입 인터페이스 */
public:
	/**
	 * 위젯에 뷰모델 인스턴스를 수동(Manual) 주입하고 UMG MVVM 뷰 확장에 바인딩
	 * @param InViewModel 주입할 뷰모델 인스턴스
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	void SetViewModelInstance(UBBMViewModelBase* InViewModel);

	/**
	 * UMG MVVMView 확장으로부터 특정 클래스 타입에 매칭되는 뷰모델 인스턴스 반환
	 * @param InViewModelClass 조회할 뷰모델 클래스
	 * @return 매칭된 뷰모델 인스턴스 (없을 경우 nullptr)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|MVVM", meta = (DeterminesOutputType = "InViewModelClass"))
	UBBMViewModelBase* GetViewModelByClass(TSubclassOf<UBBMViewModelBase> InViewModelClass) const;

	/**
	 * 특정 타입으로 캐스팅하여 UMG MVVMView에서 뷰모델을 반환하는 C++ 템플릿 헬퍼
	 */
	template<typename T>
	T* GetViewModelAs() const
	{
		return Cast<T>(GetViewModelByClass(T::StaticClass()));
	}

	/**
	 * 위젯의 UMG MVVMView 확장에 등록된 모든 UBBMViewModelBase 인스턴스를 순회하는 헬퍼 함수
	 * @param Callback 각 뷰모델 인스턴스에 대해 실행할 콜백
	 */
	void ForEachViewModel(TFunctionRef<void(UBBMViewModelBase*)> Callback) const;

protected:
	/**
	 * 뷰모델이 주입되었을 때 블루프린트에서 추가 바인딩이나 처리를 수행할 수 있는 이벤트
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI|MVVM", meta = (DisplayName = "On ViewModel Assigned"))
	void BP_OnViewModelAssigned(UBBMViewModelBase* InViewModel);
};
