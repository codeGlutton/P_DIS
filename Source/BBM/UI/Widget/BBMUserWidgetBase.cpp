#include "UI/Widget/BBMUserWidgetBase.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "UI/ViewModel/BBMSingleAnimTransition.h"
#include "UI/ViewModel/BBMDualAnimTransition.h"
#include "Animation/WidgetAnimation.h"
#include "Animation/WidgetAnimationState.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "MovieScene.h"
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
		const bool bIsViewModelShowing = (ViewModel.IsValid() == true && ViewModel->IsShowing() == true);
		if (IsInViewport() == false && bIsViewModelShowing == false)
		{
			TryAddToGameViewport();
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UBBMUserWidgetBase::BeginDestroy()
{
	OnAnimationFinishedPlaying().RemoveAll(this);

	if (UBBMViewModelBase* CurrentVM = ViewModel.Get())
	{
		ClearViewModelPresentationHandlers(CurrentVM);
		ViewModel.Reset();
	}

	StopAllAnimations();
	Super::BeginDestroy();
}

void UBBMUserWidgetBase::SetViewModelInstance(UBBMViewModelBase* InViewModel)
{
	if (ViewModel.Get() == InViewModel)
	{
		return;
	}

	// 기존 뷰모델의 프레젠테이션 요청 핸들러 안전 해제
	if (UBBMViewModelBase* OldVM = ViewModel.Get())
	{
		ClearViewModelPresentationHandlers(OldVM);
	}

	ViewModel = InViewModel;

	if (InViewModel != nullptr)
	{
		// 언리얼 엔진 공식 UMVVMView 확장에도 동일 뷰모델 주입 유지
		if (UMVVMView* MVVMView = GetExtension<UMVVMView>())
		{
			MVVMView->SetViewModelByClass(InViewModel);
		}

		// 뷰포트 추가 이전 및 DynamicDetach 해제 상태에서도 열기/닫기 명령을 수신할 수 있도록 직접 바인딩
		BindViewModelPresentationHandlers(InViewModel);

		// 이미 열려 있는 ViewModel이 뒤늦게 주입된 경우 View 프레젠테이션 상태 즉시 동기화
		if (InViewModel->IsOpen() == true)
		{
			HandleOpenPresentationRequested(false);
		}
	}

	BP_OnViewModelAssigned(InViewModel);
}

void UBBMUserWidgetBase::DetachPresentationImmediate()
{
	StopAllAnimations();
	ApplyCloseViewportPresentation();
}

void UBBMUserWidgetBase::OnOpened()
{
}

void UBBMUserWidgetBase::OnClosed()
{
}

void UBBMUserWidgetBase::BindViewModelPresentationHandlers(UBBMViewModelBase* InViewModel)
{
	if (InViewModel == nullptr)
	{
		return;
	}

	if (UBBMDualAnimTransition* DualTransition = InViewModel->GetOpenCloseTransition())
	{
		DualTransition->BindPresentationHandlers(
			this,
			FBBMOnPresentationTransitionRequestedSignature::CreateUObject(this, &UBBMUserWidgetBase::HandleOpenPresentationRequested),
			FBBMOnPresentationTransitionRequestedSignature::CreateUObject(this, &UBBMUserWidgetBase::HandleClosePresentationRequested)
		);
	}
}

void UBBMUserWidgetBase::ClearViewModelPresentationHandlers(UBBMViewModelBase* InViewModel)
{
	if (InViewModel == nullptr)
	{
		return;
	}

	if (UBBMDualAnimTransition* DualTransition = InViewModel->GetOpenCloseTransition())
	{
		DualTransition->ClearPresentationHandlers(this);
	}
}

void UBBMUserWidgetBase::HandleSingleTransitionRequested(
	UBBMSingleAnimTransition* InTransition,
	const FBBMWidgetAnimConfig& InConfig,
	bool bWithAnim
)
{
	UWidgetAnimation* Anim = InConfig.Animation.Get();
	if (Anim != nullptr)
	{
		UnbindAllFromAnimationFinished(Anim);
	}

	const bool bCanPlayAnim = (bWithAnim == true) && (Anim != nullptr) && (IsInViewport() == true) && (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(InTransition, &UBBMSingleAnimTransition::NotifyAnimFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimationByConfig(InConfig);
	}
	else
	{
		if (InTransition != nullptr)
		{
			InTransition->NotifyAnimFinished();
		}
	}
}

void UBBMUserWidgetBase::HandleDualTransitionStartRequested(
	UBBMDualAnimTransition* InDualTransition,
	const FBBMWidgetAnimConfig& InStartConfig,
	const FBBMWidgetAnimConfig& InEndConfig,
	bool bWithAnim
)
{
	UWidgetAnimation* EndAnim = InEndConfig.Animation.Get();
	if (EndAnim != nullptr)
	{
		UnbindAllFromAnimationFinished(EndAnim);
		StopAnimation(EndAnim);
	}
	UWidgetAnimation* StartAnim = InStartConfig.Animation.Get();
	if (StartAnim != nullptr)
	{
		UnbindAllFromAnimationFinished(StartAnim);
	}

	const bool bCanPlayAnim = (bWithAnim == true) && (StartAnim != nullptr) && (IsInViewport() == true) && (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(InDualTransition, &UBBMDualAnimTransition::NotifyStartTransitionFinished);
		BindToAnimationFinished(StartAnim, FinishedDelegate);

		PlayAnimationByConfig(InStartConfig);
	}
	else
	{
		if (InDualTransition != nullptr)
		{
			InDualTransition->NotifyStartTransitionFinished();
		}
	}
}

void UBBMUserWidgetBase::HandleDualTransitionEndRequested(
	UBBMDualAnimTransition* InDualTransition,
	const FBBMWidgetAnimConfig& InStartConfig,
	const FBBMWidgetAnimConfig& InEndConfig,
	bool bWithAnim
)
{
	UWidgetAnimation* StartAnim = InStartConfig.Animation.Get();
	if (StartAnim != nullptr)
	{
		UnbindAllFromAnimationFinished(StartAnim);
		StopAnimation(StartAnim);
	}
	UWidgetAnimation* EndAnim = InEndConfig.Animation.Get();
	if (EndAnim != nullptr)
	{
		UnbindAllFromAnimationFinished(EndAnim);
	}

	const bool bCanPlayAnim = (bWithAnim == true) && (EndAnim != nullptr) && (IsInViewport() == true) && (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(InDualTransition, &UBBMDualAnimTransition::NotifyEndTransitionFinished);
		BindToAnimationFinished(EndAnim, FinishedDelegate);

		PlayAnimationByConfig(InStartConfig);
	}
	else
	{
		if (InDualTransition != nullptr)
		{
			InDualTransition->NotifyEndTransitionFinished();
		}
	}
}

void UBBMUserWidgetBase::HandleOpenPresentationRequested(bool bWithAnim)
{
	UWidgetAnimation* EndAnim = CloseAnimConfig.Animation.Get();
	if (EndAnim != nullptr)
	{
		UnbindAllFromAnimationFinished(EndAnim);
		StopAnimation(EndAnim);
	}
	UWidgetAnimation* StartAnim = OpenAnimConfig.Animation.Get();
	if (StartAnim != nullptr)
	{
		UnbindAllFromAnimationFinished(StartAnim);
	}

	ApplyOpenViewportPresentation();

	OnOpened();
	BP_OnOpened();

	const bool bCanPlayAnim = (bWithAnim == true) && (StartAnim != nullptr) && (IsInViewport() == true) && (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleOpenAnimationFinished);
		BindToAnimationFinished(StartAnim, FinishedDelegate);

		PlayAnimationByConfig(OpenAnimConfig);
	}
	else
	{
		HandleOpenAnimationFinished();
	}
}

void UBBMUserWidgetBase::HandleClosePresentationRequested(bool bWithAnim)
{
	UWidgetAnimation* StartAnim = OpenAnimConfig.Animation.Get();
	if (StartAnim != nullptr)
	{
		UnbindAllFromAnimationFinished(StartAnim);
		StopAnimation(StartAnim);
	}
	UWidgetAnimation* EndAnim = CloseAnimConfig.Animation.Get();
	if (EndAnim != nullptr)
	{
		UnbindAllFromAnimationFinished(EndAnim);
	}

	const bool bCanPlayAnim = (bWithAnim == true) && (EndAnim != nullptr) && (IsInViewport() == true) && (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleCloseAnimationFinished);
		BindToAnimationFinished(EndAnim, FinishedDelegate);

		PlayAnimationByConfig(CloseAnimConfig);
	}
	else
	{
		HandleCloseAnimationFinished();
	}
}

void UBBMUserWidgetBase::HandleOpenAnimationFinished()
{
	if (ViewModel != nullptr && ViewModel->GetOpenCloseTransition() != nullptr)
	{
		ViewModel->GetOpenCloseTransition()->NotifyStartTransitionFinished();
	}
}

void UBBMUserWidgetBase::HandleCloseAnimationFinished()
{
	ApplyCloseViewportPresentation();

	OnClosed();
	BP_OnClosed();

	if (ViewModel != nullptr && ViewModel->GetOpenCloseTransition() != nullptr)
	{
		ViewModel->GetOpenCloseTransition()->NotifyEndTransitionFinished();
	}
}

void UBBMUserWidgetBase::TryAddToGameViewport()
{
	UWorld* World = GetWorld();
	if (IsInViewport() == false && World != nullptr && World->IsGameWorld() == true && World->GetGameViewport() != nullptr)
	{
		AddToViewport(static_cast<int32>(DefaultZOrderLayer));
	}
}

void UBBMUserWidgetBase::ApplyOpenViewportPresentation()
{
	TryAddToGameViewport();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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

void UBBMUserWidgetBase::PlayAnimationByConfig(const FBBMWidgetAnimConfig& InConfig)
{
	UWidgetAnimation* Anim = InConfig.Animation;
	if (Anim == nullptr)
	{
		return;
	}

	switch (InConfig.PlayMode)
	{
	case EBBMWidgetAnimPlayMode::RelativeForward:
		PlayAnimationForward(Anim, InConfig.PlaybackSpeed);
		break;

	case EBBMWidgetAnimPlayMode::RelativeReverse:
		PlayAnimationReverse(Anim, InConfig.PlaybackSpeed);
		break;

	case EBBMWidgetAnimPlayMode::Forward:
		PlayAnimation(Anim, 0.0f, 1, EUMGSequencePlayMode::Forward, InConfig.PlaybackSpeed);
		break;

	case EBBMWidgetAnimPlayMode::Reverse:
		PlayAnimation(Anim, 0.0f, 1, EUMGSequencePlayMode::Reverse, InConfig.PlaybackSpeed);
		break;

	case EBBMWidgetAnimPlayMode::PingPong:
		PlayAnimation(Anim, 0.0f, 1, EUMGSequencePlayMode::PingPong, InConfig.PlaybackSpeed);
		break;
	}
}
