/*****************************************************************//**
 * @file   BBMTitleViewModel.h
 * @brief  BBM(비트 배낭 상인) 타이틀 화면 전용 MVVM 뷰모델 클래스 정의
 * @details 이어하기 가시성, 런 데이터 상태 요약 및 타이틀 5대 메뉴(시작/이어하기/옵션/크레딧/종료) 비즈니스 로직 전담
 * @date   2026-11-07
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "Components/SlateWrapperTypes.h"
#include "BBMTitleViewModel.generated.h"

/* 네비게이션 및 메뉴 요청 델리게이트 선언 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBBMOnTitleNavigationSignature);
DECLARE_MULTICAST_DELEGATE(FBBMOnTitleNavigationNativeSignature);

/**
 * BBM 타이틀 화면 전용 MVVM 뷰모델
 * UMG 위젯과 결합되어 데이터 바인딩 및 사용자 입력 처리를 총괄함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Title ViewModel"))
class BBM_API UBBMTitleViewModel : public UBBMViewModelBase
{
	GENERATED_BODY()

	/* 생성자 및 수명 주기 */
public:
	UBBMTitleViewModel();

	virtual void NotifyWidgetOpened() override;

	/* 필드 노티파이 프로퍼티 (Getter / Setter) */
public:
	/* 이어하기 가능 여부 반환 */
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	bool GetCanContinue() const { return bCanContinue; }

	/* 이어하기 가능 여부 설정 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void SetCanContinue(bool bInCanContinue);

	/* 이어하기 버튼 가시성 반환 (세이브 없으면 Collapsed) */
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	ESlateVisibility GetContinueVisibility() const { return ContinueVisibility; }

	/* 이어하기 버튼 가시성 설정 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void SetContinueVisibility(ESlateVisibility InVisibility);

	/* 현재 진행 중인 런 요약 텍스트 반환 */
	UFUNCTION(BlueprintPure, Category = "BBM|UI|Title")
	FText GetActiveRunSummaryText() const { return ActiveRunSummaryText; }

	/* 현재 진행 중인 런 요약 텍스트 설정 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void SetActiveRunSummaryText(const FText& InSummaryText);

	/* 비즈니스 로직 및 메뉴 액션 인터페이스 */
public:
	/**
	 * GameDataSubsystem을 조회하여 현재 런 세이브 유무에 따라 이어하기 버튼 가시성을 갱신
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void RefreshTitleState();

	/**
	 * 게임 시작 버튼 클릭 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void StartNewGame();

	/**
	 * 이어하기 버튼 클릭 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void ContinueGame();

	/**
	 * 옵션(설정) 창 열기 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void OpenSettings();

	/**
	 * 크레딧 창 열기 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void OpenCredits();

	/**
	 * 게임 종료 액션 처리
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|Title")
	void QuitGame();

	/* 델리게이트 이벤트 */
public:
	/* 새 게임 시작 요청 이벤트 (블루프린트용) */
	UPROPERTY(BlueprintAssignable, Category = "BBM|UI|Title|Event")
	FBBMOnTitleNavigationSignature OnStartNewGameRequested;

	/* 새 게임 시작 요청 이벤트 (C++ 네이티브용) */
	FBBMOnTitleNavigationNativeSignature OnStartNewGameRequestedNative;

	/* 이어하기 요청 이벤트 (블루프린트용) */
	UPROPERTY(BlueprintAssignable, Category = "BBM|UI|Title|Event")
	FBBMOnTitleNavigationSignature OnContinueGameRequested;

	/* 이어하기 요청 이벤트 (C++ 네이티브용) */
	FBBMOnTitleNavigationNativeSignature OnContinueGameRequestedNative;

	/* 환경 설정 열기 요청 이벤트 (블루프린트용) */
	UPROPERTY(BlueprintAssignable, Category = "BBM|UI|Title|Event")
	FBBMOnTitleNavigationSignature OnOpenSettingsRequested;

	/* 환경 설정 열기 요청 이벤트 (C++ 네이티브용) */
	FBBMOnTitleNavigationNativeSignature OnOpenSettingsRequestedNative;

	/* 크레딧 열기 요청 이벤트 (블루프린트용) */
	UPROPERTY(BlueprintAssignable, Category = "BBM|UI|Title|Event")
	FBBMOnTitleNavigationSignature OnOpenCreditsRequested;

	/* 크레딧 열기 요청 이벤트 (C++ 네이티브용) */
	FBBMOnTitleNavigationNativeSignature OnOpenCreditsRequestedNative;

	/* 내부 프로퍼티 */
private:
	/* 이어하기 가능 여부 */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter = SetCanContinue, Getter = GetCanContinue, meta = (AllowPrivateAccess = "true"))
	bool bCanContinue = false;

	/* 이어하기 버튼 가시성 (기본값: Collapsed) */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter = SetContinueVisibility, Getter = GetContinueVisibility, meta = (AllowPrivateAccess = "true"))
	ESlateVisibility ContinueVisibility = ESlateVisibility::Collapsed;

	/* 진행 중인 런 요약 텍스트 (예: "지역 1 - 초원 (3/5 조우)") */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter = SetActiveRunSummaryText, Getter = GetActiveRunSummaryText, meta = (AllowPrivateAccess = "true"))
	FText ActiveRunSummaryText = FText::GetEmpty();
};

