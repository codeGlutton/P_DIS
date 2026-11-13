/*****************************************************************//**
 * @file   BBMTitleWidget.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 메뉴 메인 위젯 클래스 구현
 * @date   2026-11-07
 *********************************************************************/

#include "UI/Widget/BBMTitleWidget.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "UI/ViewModel/BBMSingleAnimTransition.h"
#include "UI/ViewModel/BBMDualAnimTransition.h"
#include "Animation/WidgetAnimation.h"

UBBMTitleWidget::UBBMTitleWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	// 타이틀 메뉴는 기본적으로 베이스 레이어(10)에 배치
	DefaultZOrderLayer = EBBMWidgetZOrder::Base;

	// Shining은 무한 반복, Blocking은 열림/닫힘 기본 방향 설정
	ShiningAnimConfig.PlayMode = EBBMWidgetAnimPlayMode::Forward;
	ShiningAnimConfig.LoopCount = 0;

	BlockingStartAnimConfig.PlayMode = EBBMWidgetAnimPlayMode::RelativeForward;
	BlockingEndAnimConfig.PlayMode = EBBMWidgetAnimPlayMode::RelativeReverse;
}

void UBBMTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UBBMTitleWidget::BindViewModelPresentationHandlers(UBBMViewModelBase* InViewModel)
{
	Super::BindViewModelPresentationHandlers(InViewModel);

	UBBMTitleViewModel* TitleVM = Cast<UBBMTitleViewModel>(InViewModel);
	if (TitleVM == nullptr)
	{
		return;
	}

	if (UBBMSingleAnimTransition* ShiningTransition = TitleVM->GetShiningTransition())
	{
		ShiningTransition->BindPresentationHandler(
			this,
			FBBMOnPresentationTransitionRequestedSignature::CreateWeakLambda(
				this, 
				[this, ShiningTransition](bool bWithAnim) 
				{
					HandleSingleTransitionRequested(bWithAnim, ShiningTransition, ShiningAnimConfig);
				}
			)
		);
	}

	if (UBBMDualAnimTransition* BlockingTransition = TitleVM->GetBlockingTransition())
	{
		BlockingTransition->BindPresentationHandlers(
			this,
			FBBMOnPresentationTransitionRequestedSignature::CreateWeakLambda(
				this,
				[this, BlockingTransition](bool bWithAnim)
				{
					HandleDualTransitionStartRequested(bWithAnim, BlockingTransition, BlockingStartAnimConfig, BlockingEndAnimConfig);
				}
			),
			FBBMOnPresentationTransitionRequestedSignature::CreateWeakLambda(
				this,
				[this, BlockingTransition](bool bWithAnim)
				{
					HandleDualTransitionEndRequested(bWithAnim, BlockingTransition, BlockingStartAnimConfig, BlockingEndAnimConfig);
				}
			)
		);
	}
}

void UBBMTitleWidget::ClearViewModelPresentationHandlers(UBBMViewModelBase* InViewModel)
{
	if (UBBMTitleViewModel* TitleVM = Cast<UBBMTitleViewModel>(InViewModel))
	{
		if (UBBMSingleAnimTransition* ShiningTransition = TitleVM->GetShiningTransition())
		{
			UnbindAllFromAnimationFinished(ShiningAnimConfig.GetAnimation(this));
			ShiningTransition->ClearPresentationHandler(this);
		}

		if (UBBMDualAnimTransition* BlockingTransition = TitleVM->GetBlockingTransition())
		{
			UnbindAllFromAnimationFinished(BlockingStartAnimConfig.GetAnimation(this));
			UnbindAllFromAnimationFinished(BlockingEndAnimConfig.GetAnimation(this));
			BlockingTransition->ClearPresentationHandlers(this);
		}
	}

	Super::ClearViewModelPresentationHandlers(InViewModel);
}

UBBMTitleViewModel* UBBMTitleWidget::GetTitleViewModel() const
{
	return GetViewModelAs<UBBMTitleViewModel>();
}

