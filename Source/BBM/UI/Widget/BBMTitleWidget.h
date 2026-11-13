/*****************************************************************//**
 * @file   BBMTitleWidget.h
 * @brief  BBM(비트 배낭 상인) 타이틀 메뉴 메인 위젯 클래스 정의
 * @details UBBMUserWidgetBase를 상속하며, UBBMTitleViewModel과 연동되어 타이틀 화면의 MVVM 뷰 역할을 수행함
 * @date   2026-11-07
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/BBMUserWidgetBase.h"
#include "BBMTitleWidget.generated.h"

/* 독립 전방 선언 */
class UBBMTitleViewModel;

/**
 * BBM 타이틀 메뉴 메인 위젯
 * WBP_TitleMenu 블루프린트의 부모 클래스로 활용되며, Manual 모드로 주입된 UBBMTitleViewModel과 바인딩됨
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Title Widget"))
class BBM_API UBBMTitleWidget : public UBBMUserWidgetBase
{
	GENERATED_BODY()

	/* 생성자 및 UMG 수명 주기 */
public:
	UBBMTitleWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

protected:
	virtual void BindViewModelPresentationHandlers(UBBMViewModelBase* InViewModel) override;
	virtual void ClearViewModelPresentationHandlers(UBBMViewModelBase* InViewModel) override;

	/* 뷰모델 접근자 */
public:
	/**
	 * 주입된 타이틀 전용 뷰모델을 캐스팅하여 반환
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BBM|UI|Title")
	UBBMTitleViewModel* GetTitleViewModel() const;

	/* 애니메이션 설정 및 상태 */
protected:
	/* 타이틀 화면 오픈 중 무한 반복 재생할 반짝임 애니메이션 설정 */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	FBBMWidgetAnimConfig ShiningAnimConfig;

	/* 타이틀 블로킹 오버레이 시작 애니메이션 설정 */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	FBBMWidgetAnimConfig BlockingStartAnimConfig;

	/* 타이틀 블로킹 오버레이 종료 애니메이션 설정 */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	FBBMWidgetAnimConfig BlockingEndAnimConfig;
};

