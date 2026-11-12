/*****************************************************************//**
 * @file   BBMUserWidgetBase.cpp
 * @brief  BBM(비트 배낭 상인) 공용 사용자 위젯 베이스 클래스 구현
 * @date   2026-11-06
 *********************************************************************/

#include "UI/Widget/BBMUserWidgetBase.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "View/MVVMView.h"

UBBMUserWidgetBase::UBBMUserWidgetBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	LifecyclePolicy(EBBMWidgetLifecyclePolicy::PersistentViewport),
	DefaultZOrderLayer(EBBMWidgetZOrder::Base),
	ViewModel(nullptr)
{
	OpenAnimConfig.PlayMode = EBBMWidgetAnimPlayMode::RelativeForward;
	CloseAnimConfig.PlayMode = EBBMWidgetAnimPlayMode::RelativeReverse;
}

void UBBMUserWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// 상시 뷰포트 유지 정책인 경우 현재 표시 중이 아니라면 기본 숨김 상태로 뷰포트에 등록
	if (LifecyclePolicy == EBBMWidgetLifecyclePolicy::PersistentViewport)
	{
		const bool bIsViewModelShowing = (ViewModel != nullptr && ViewModel->IsShowing() == true);
		if (IsInViewport() == false && bIsViewModelShowing == false)
		{
			TryAddToGameViewport();
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UBBMUserWidgetBase::BeginDestroy()
{
	if (ViewModel != nullptr)
	{
		ViewModel->ClearPresentationHandlers(this);
		ViewModel = nullptr;
	}

	ClearAnimationFinishedBindings(false);
	Super::BeginDestroy();
}

void UBBMUserWidgetBase::SetViewModelInstance(UBBMViewModelBase* InViewModel)
{
	if (ViewModel == InViewModel)
	{
		return;
	}

	// 기존 뷰모델의 프레젠테이션 요청 핸들러 안전 해제
	if (ViewModel != nullptr)
	{
		ViewModel->ClearPresentationHandlers(this);
	}

	ViewModel = InViewModel;

	if (ViewModel != nullptr)
	{
		// 언리얼 엔진 공식 UMVVMView 확장에도 동일 뷰모델 주입 유지
		if (UMVVMView* MVVMView = GetExtension<UMVVMView>())
		{
			MVVMView->SetViewModelByClass(ViewModel);
		}

		// 뷰포트 추가 이전 및 DynamicDetach 해제 상태에서도 열기/닫기 명령을 수신할 수 있도록 직접 바인딩
		ViewModel->BindPresentationHandlers(
			this,
			FBBMOnPresentationTransitionRequestedSignature::CreateUObject(this, &UBBMUserWidgetBase::HandleOpenPresentationRequested),
			FBBMOnPresentationTransitionRequestedSignature::CreateUObject(this, &UBBMUserWidgetBase::HandleClosePresentationRequested)
		);

		// 이미 열려 있는 ViewModel이 뒤늦게 주입된 경우 View 프레젠테이션 상태 즉시 동기화
		if (ViewModel->IsOpen() == true)
		{
			HandleOpenPresentationRequested(false);
		}
	}

	BP_OnViewModelAssigned(ViewModel);
}

void UBBMUserWidgetBase::DetachPresentationImmediate()
{
	ClearAnimationFinishedBindings(true);
	ApplyCloseViewportPresentation();
}

void UBBMUserWidgetBase::OnOpened()
{
}

void UBBMUserWidgetBase::OnClosed()
{
}

void UBBMUserWidgetBase::HandleOpenPresentationRequested(bool bWithAnim)
{
	ClearAnimationFinishedBindings(true);
	ApplyOpenViewportPresentation();

	OnOpened();
	BP_OnOpened();

	UWidgetAnimation* Anim = OpenAnimConfig.Animation.Get();
	if (bWithAnim == true && Anim != nullptr)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleOpenAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimationByConfig(Anim, OpenAnimConfig);
	}
	else
	{
		// 애니메이션 미사용 또는 에셋 미설정 시 즉시 완료 회신
		if (ViewModel != nullptr)
		{
			ViewModel->NotifyOpenTransitionFinished();
		}
	}
}

void UBBMUserWidgetBase::HandleClosePresentationRequested(bool bWithAnim)
{
	ClearAnimationFinishedBindings(true);

	UWidgetAnimation* Anim = CloseAnimConfig.Animation.Get();
	const bool bCanPlayCloseAnim = (bWithAnim == true) && (Anim != nullptr) && (IsInViewport() == true) && (IsVisible() == true);

	if (bCanPlayCloseAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleCloseAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimationByConfig(Anim, CloseAnimConfig);
	}
	else
	{
		// 애니메이션 미설정 또는 비표시 상태에서는 즉시 숨김/탈착 및 완료 회신
		ApplyCloseViewportPresentation();
		OnClosed();
		BP_OnClosed();

		if (ViewModel != nullptr)
		{
			ViewModel->NotifyCloseTransitionFinished();
		}
	}
}

void UBBMUserWidgetBase::ApplyOpenViewportPresentation()
{
	TryAddToGameViewport();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UBBMUserWidgetBase::TryAddToGameViewport()
{
	UWorld* World = GetWorld();
	if (IsInViewport() == false && World != nullptr && World->IsGameWorld() == true && World->GetGameViewport() != nullptr)
	{
		AddToViewport(static_cast<int32>(DefaultZOrderLayer));
	}
}

void UBBMUserWidgetBase::ApplyCloseViewportPresentation()
{
	if (LifecyclePolicy == EBBMWidgetLifecyclePolicy::PersistentViewport)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (LifecyclePolicy == EBBMWidgetLifecyclePolicy::DynamicDetach)
	{
		if (IsInViewport() == true)
		{
			RemoveFromParent();
		}
	}
}

void UBBMUserWidgetBase::HandleOpenAnimationFinished()
{
	if (UWidgetAnimation* Anim = OpenAnimConfig.Animation.Get())
	{
		UnbindAllFromAnimationFinished(Anim);
	}

	if (ViewModel != nullptr)
	{
		ViewModel->NotifyOpenTransitionFinished();
	}
}

void UBBMUserWidgetBase::HandleCloseAnimationFinished()
{
	if (UWidgetAnimation* Anim = CloseAnimConfig.Animation.Get())
	{
		UnbindAllFromAnimationFinished(Anim);
	}

	// 애니메이션 종료 후 실제 뷰포트 탈착 또는 가시성 숨김 수행
	ApplyCloseViewportPresentation();

	OnClosed();
	BP_OnClosed();

	if (ViewModel != nullptr)
	{
		ViewModel->NotifyCloseTransitionFinished();
	}
}

void UBBMUserWidgetBase::ClearAnimationFinishedBindings(bool bStopRunningAnimations)
{
	if (UWidgetAnimation* OpenAnim = OpenAnimConfig.Animation.Get())
	{
		UnbindAllFromAnimationFinished(OpenAnim);
		if (bStopRunningAnimations == true)
		{
			StopAnimation(OpenAnim);
		}
	}

	if (UWidgetAnimation* CloseAnim = CloseAnimConfig.Animation.Get())
	{
		UnbindAllFromAnimationFinished(CloseAnim);
		if (bStopRunningAnimations == true)
		{
			StopAnimation(CloseAnim);
		}
	}
}

void UBBMUserWidgetBase::PlayAnimationByConfig(UWidgetAnimation* InAnim, const FBBMWidgetAnimConfig& InConfig)
{
	if (InAnim == nullptr)
	{
		return;
	}

	switch (InConfig.PlayMode)
	{
	case EBBMWidgetAnimPlayMode::RelativeForward:
		PlayAnimationForward(InAnim, InConfig.PlaybackSpeed);
		break;

	case EBBMWidgetAnimPlayMode::RelativeReverse:
		PlayAnimationReverse(InAnim, InConfig.PlaybackSpeed);
		break;

	case EBBMWidgetAnimPlayMode::Forward:
		PlayAnimation(InAnim, 0.0f, 1, EUMGSequencePlayMode::Forward, InConfig.PlaybackSpeed);
		break;

	case EBBMWidgetAnimPlayMode::Reverse:
		PlayAnimation(InAnim, 0.0f, 1, EUMGSequencePlayMode::Reverse, InConfig.PlaybackSpeed);
		break;

	case EBBMWidgetAnimPlayMode::PingPong:
		PlayAnimation(InAnim, 0.0f, 1, EUMGSequencePlayMode::PingPong, InConfig.PlaybackSpeed);
		break;
	}
}
