/*****************************************************************//**
 * @file   BBMViewModelBase.cpp
 * @brief  BBM(비트 배낭 상인) 공용 MVVM 뷰모델 베이스 클래스 구현
 * @date   2026-11-07
 *********************************************************************/

#include "UI/ViewModel/BBMViewModelBase.h"

UBBMViewModelBase::UBBMViewModelBase()
{
}

void UBBMViewModelBase::NotifyWidgetOpened()
{
	BP_OnWidgetOpened();
}

void UBBMViewModelBase::NotifyWidgetClosed()
{
	BP_OnWidgetClosed();
}
