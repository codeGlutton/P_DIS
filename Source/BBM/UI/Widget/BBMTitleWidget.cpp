/*****************************************************************//**
 * @file   BBMTitleWidget.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 메뉴 메인 위젯 클래스 구현
 * @date   2026-11-07
 *********************************************************************/

#include "UI/Widget/BBMTitleWidget.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "ViewModel/SingleAnimTransition.h"
#include "ViewModel/DualAnimTransition.h"
#include "Animation/WidgetAnimation.h"

UBBMTitleWidget::UBBMTitleWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	// 타이틀 메뉴는 기본적으로 베이스 레이어(10)에 배치
	DefaultZOrderLayer = EWorldWidgetZOrder::Base;

	// Shining은 무한 반복, Blocking은 열림/닫힘 기본 방향 설정
	ShiningAnimConfig.PlayMode = EWorldWidgetAnimPlayMode::Forward;
	ShiningAnimConfig.LoopCount = 0;

	BlockingStartAnimConfig.PlayMode = EWorldWidgetAnimPlayMode::RelativeForward;
	BlockingEndAnimConfig.PlayMode = EWorldWidgetAnimPlayMode::RelativeReverse;
}

void UBBMTitleWidget::BindViewModelPresentationHandlers(UWorldViewModelBase* InViewModel)
{
	Super::BindViewModelPresentationHandlers(InViewModel);

	UBBMTitleViewModel* TitleVM = Cast<UBBMTitleViewModel>(InViewModel);
	if (TitleVM == nullptr)
	{
		return;
	}

	if (USingleAnimTransition* ShiningTransition = TitleVM->GetShiningTransition())
	{
		ShiningTransition->BindPresentationHandler(
			this,
			FWorldUIOnPresentationTransitionRequestedSignature::CreateWeakLambda(
				this, 
				[this, ShiningTransition](bool bWithAnim) 
				{
					HandleSingleTransitionRequested(bWithAnim, ShiningTransition, ShiningAnimConfig);
				}
			)
		);
	}

	if (UDualAnimTransition* BlockingTransition = TitleVM->GetBlockingTransition())
	{
		BlockingTransition->BindPresentationHandlers(
			this,
			FWorldUIOnPresentationTransitionRequestedSignature::CreateWeakLambda(
				this,
				[this, BlockingTransition](bool bWithAnim)
				{
					HandleDualTransitionStartRequested(bWithAnim, BlockingTransition, BlockingStartAnimConfig, BlockingEndAnimConfig);
				}
			),
			FWorldUIOnPresentationTransitionRequestedSignature::CreateWeakLambda(
				this,
				[this, BlockingTransition](bool bWithAnim)
				{
					HandleDualTransitionEndRequested(bWithAnim, BlockingTransition, BlockingStartAnimConfig, BlockingEndAnimConfig);
				}
			)
		);
	}
}

void UBBMTitleWidget::ClearViewModelPresentationHandlers(UWorldViewModelBase* InViewModel)
{
	if (UBBMTitleViewModel* TitleVM = Cast<UBBMTitleViewModel>(InViewModel))
	{
		if (USingleAnimTransition* ShiningTransition = TitleVM->GetShiningTransition())
		{
			UnbindAllFromAnimationFinishedByConfig(ShiningAnimConfig);
			StopAnimationByConfig(ShiningAnimConfig);

			ShiningTransition->ClearPresentationHandler(this);
		}

		if (UDualAnimTransition* BlockingTransition = TitleVM->GetBlockingTransition())
		{
			UnbindAllFromAnimationFinishedByConfig(BlockingStartAnimConfig);
			UnbindAllFromAnimationFinishedByConfig(BlockingEndAnimConfig);
			StopAnimationByConfig(BlockingStartAnimConfig);
			StopAnimationByConfig(BlockingEndAnimConfig);

			BlockingTransition->ClearPresentationHandlers(this);
		}
	}

	Super::ClearViewModelPresentationHandlers(InViewModel);
}

UBBMTitleViewModel* UBBMTitleWidget::GetTitleViewModel() const
{
	return GetViewModelAs<UBBMTitleViewModel>();
}

