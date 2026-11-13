/*****************************************************************//**
 * @file   BBMTitleWidget.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 메뉴 메인 위젯 클래스 구현
 * @date   2026-11-07
 *********************************************************************/

#include "UI/Widget/BBMTitleWidget.h"
#include "UI/ViewModel/BBMTitleViewModel.h"

UBBMTitleWidget::UBBMTitleWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	// 타이틀 메뉴는 기본적으로 베이스 레이어(10)에 배치
	DefaultZOrderLayer = EBBMWidgetZOrder::Base;
}

void UBBMTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

UBBMTitleViewModel* UBBMTitleWidget::GetTitleViewModel() const
{
	return GetViewModelAs<UBBMTitleViewModel>();
}

