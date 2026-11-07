/*****************************************************************//**
 * @file   BBMViewModelBase.h
 * @brief  BBM(비트 배낭 상인) 공용 MVVM 뷰모델 베이스 클래스 정의
 * @details 언리얼 엔진 공식 ModelViewViewModel 플러그인을 기반으로 단일 진실 공급원 역할을 수행하는 베이스 클래스
 * @date   2026-11-07
 **********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "BBMViewModelBase.generated.h"

/**
 * BBM 공용 MVVM 뷰모델 베이스 클래스
 * 프로젝트 내 모든 MVVM 뷰모델의 최상위 부모로서 공용 타입 및 확장을 위한 베이스 역할을 수행함
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisplayName = "BBM ViewModel Base"))
class BBM_API UBBMViewModelBase : public UMVVMViewModelBase
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMViewModelBase();

	/* 위젯 수명 주기(Open / Close) 연동 인터페이스 */
public:
	/**
	 * 연결된 위젯이 열렸을 때(Open) 호출되는 가상 함수
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	virtual void NotifyWidgetOpened();

	/**
	 * 연결된 위젯이 닫혔을 때(Close) 호출되는 가상 함수
	 */
	UFUNCTION(BlueprintCallable, Category = "BBM|UI|MVVM")
	virtual void NotifyWidgetClosed();

protected:
	/**
	 * 블루프린트에서 위젯 오픈 시 로직을 처리할 수 있는 이벤트
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI|MVVM", meta = (DisplayName = "On Widget Opened"))
	void BP_OnWidgetOpened();

	/**
	 * 블루프린트에서 위젯 클로즈 시 로직을 처리할 수 있는 이벤트
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "BBM|UI|MVVM", meta = (DisplayName = "On Widget Closed"))
	void BP_OnWidgetClosed();
};
