/*****************************************************************//**
 * @file   BBMUserWidgetBase.cpp
 * @brief  BBM(비트 배낭 상인) 공용 사용자 위젯 베이스 클래스 구현
 * @date   2026-11-06
 *********************************************************************/

#include "UI/BBMUserWidgetBase.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "View/MVVMView.h"

UBBMUserWidgetBase::UBBMUserWidgetBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	LifecyclePolicy(EBBMWidgetLifecyclePolicy::PersistentViewport),
	DefaultZOrderLayer(EBBMWidgetZOrder::Base),
	bIsOpen(false)
{
	OpenAnimConfig.PlayMode = EBBMWidgetAnimPlayMode::RelativeForward;
	CloseAnimConfig.PlayMode = EBBMWidgetAnimPlayMode::RelativeReverse;
}

void UBBMUserWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// 상시 뷰포트 유지 정책인 경우 Construction 시점에 뷰포트에 추가하고 기본 숨김 상태로 대기
	if (LifecyclePolicy == EBBMWidgetLifecyclePolicy::PersistentViewport)
	{
		if (IsInViewport() == false && bIsOpen == false)
		{
			AddToViewport(static_cast<int32>(DefaultZOrderLayer));
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UBBMUserWidgetBase::Open()
{
	bIsOpen = true;

	// 최적화 수명 주기 정책에 따른 뷰포트 등록 및 가시성 전환
	if (LifecyclePolicy == EBBMWidgetLifecyclePolicy::PersistentViewport)
	{
		if (IsInViewport() == false)
		{
			AddToViewport(static_cast<int32>(DefaultZOrderLayer));
		}
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else if (LifecyclePolicy == EBBMWidgetLifecyclePolicy::DynamicDetach)
	{
		if (IsInViewport() == false)
		{
			AddToViewport(static_cast<int32>(DefaultZOrderLayer));
		}
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 연결된 뷰모델들에 위젯 오픈 알림 전달
	ForEachViewModel([](UBBMViewModelBase* VM)
	{
		if (VM != nullptr)
		{
			VM->NotifyWidgetOpened();
		}
	});

	OnOpened();
	BP_OnOpened();
}

void UBBMUserWidgetBase::Close()
{
	bIsOpen = false;

	// 최적화 수명 주기 정책에 따른 뷰포트 탈착 또는 가시성 숨김
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

	// 연결된 뷰모델들에 위젯 클로즈 알림 전달
	ForEachViewModel([](UBBMViewModelBase* VM)
	{
		if (VM != nullptr)
		{
			VM->NotifyWidgetClosed();
		}
	});

	OnClosed();
	BP_OnClosed();
}

void UBBMUserWidgetBase::ClearAnimationFinishedBindings()
{
	if (UWidgetAnimation* OpenAnim = OpenAnimConfig.Animation.Get())
	{
		UnbindAllFromAnimationFinished(OpenAnim);
	}
	if (UWidgetAnimation* CloseAnim = CloseAnimConfig.Animation.Get())
	{
		UnbindAllFromAnimationFinished(CloseAnim);
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

void UBBMUserWidgetBase::OpenWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	PendingOpenAnimDynamicCallback = OnComplete;
	PendingOpenAnimCallback.Unbind();

	PendingCloseAnimCallback.Unbind();
	PendingCloseAnimDynamicCallback.Clear();

	// 가시성 및 뷰포트 상태 먼저 확보
	Open();

	UWidgetAnimation* Anim = OpenAnimConfig.Animation.Get();
	if (Anim != nullptr)
	{
		ClearAnimationFinishedBindings();

		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleOpenAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimationByConfig(Anim, OpenAnimConfig);
	}
	else
	{
		HandleOpenAnimationFinished();
	}
}

void UBBMUserWidgetBase::OpenWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	PendingOpenAnimCallback = OnComplete;
	PendingOpenAnimDynamicCallback.Clear();

	PendingCloseAnimCallback.Unbind();
	PendingCloseAnimDynamicCallback.Clear();

	// 가시성 및 뷰포트 상태 먼저 확보
	Open();

	UWidgetAnimation* Anim = OpenAnimConfig.Animation.Get();
	if (Anim != nullptr)
	{
		ClearAnimationFinishedBindings();

		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleOpenAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimationByConfig(Anim, OpenAnimConfig);
	}
	else
	{
		HandleOpenAnimationFinished();
	}
}

void UBBMUserWidgetBase::CloseWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	PendingOpenAnimCallback.Unbind();
	PendingOpenAnimDynamicCallback.Clear();

	PendingCloseAnimDynamicCallback = OnComplete;
	PendingCloseAnimCallback.Unbind();

	UWidgetAnimation* Anim = CloseAnimConfig.Animation.Get();
	if (Anim != nullptr)
	{
		ClearAnimationFinishedBindings();

		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleCloseAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimationByConfig(Anim, CloseAnimConfig);
	}
	else
	{
		HandleCloseAnimationFinished();
	}
}

void UBBMUserWidgetBase::CloseWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	PendingOpenAnimCallback.Unbind();
	PendingOpenAnimDynamicCallback.Clear();

	PendingCloseAnimCallback = OnComplete;
	PendingCloseAnimDynamicCallback.Clear();

	UWidgetAnimation* Anim = CloseAnimConfig.Animation.Get();
	if (Anim != nullptr)
	{
		ClearAnimationFinishedBindings();

		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleCloseAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimationByConfig(Anim, CloseAnimConfig);
	}
	else
	{
		HandleCloseAnimationFinished();
	}
}

void UBBMUserWidgetBase::OnOpened()
{
}

void UBBMUserWidgetBase::OnClosed()
{
}

void UBBMUserWidgetBase::HandleOpenAnimationFinished()
{
	if (UWidgetAnimation* Anim = OpenAnimConfig.Animation.Get())
	{
		UnbindAllFromAnimationFinished(Anim);
	}

	if (PendingOpenAnimDynamicCallback.IsBound() == true)
	{
		PendingOpenAnimDynamicCallback.Execute();
		PendingOpenAnimDynamicCallback.Clear();
	}

	if (PendingOpenAnimCallback.IsBound() == true)
	{
		PendingOpenAnimCallback.Execute();
		PendingOpenAnimCallback.Unbind();
	}
}

void UBBMUserWidgetBase::HandleCloseAnimationFinished()
{
	if (UWidgetAnimation* Anim = CloseAnimConfig.Animation.Get())
	{
		UnbindAllFromAnimationFinished(Anim);
	}

	// 애니메이션이 종료된 시점에 실제 위젯 닫기(뷰포트 탈착 또는 가시성 숨김) 수행
	Close();

	if (PendingCloseAnimDynamicCallback.IsBound() == true)
	{
		PendingCloseAnimDynamicCallback.Execute();
		PendingCloseAnimDynamicCallback.Clear();
	}

	if (PendingCloseAnimCallback.IsBound() == true)
	{
		PendingCloseAnimCallback.Execute();
		PendingCloseAnimCallback.Unbind();
	}
}

void UBBMUserWidgetBase::SetViewModelInstance(UBBMViewModelBase* InViewModel)
{
	// 언리얼 엔진 공식 UMVVMView 확장에 Manual 슬롯 바인딩 주입
	if (InViewModel != nullptr)
	{
		if (UMVVMView* MVVMView = GetExtension<UMVVMView>())
		{
			MVVMView->SetViewModelByClass(InViewModel);
		}

		// 이미 위젯이 열려 있는 상태에서 뷰모델이 주입된 경우, 즉시 오픈 이벤트 통지
		if (bIsOpen == true)
		{
			InViewModel->NotifyWidgetOpened();
		}
	}

	BP_OnViewModelAssigned(InViewModel);
}

UBBMViewModelBase* UBBMUserWidgetBase::GetViewModelByClass(TSubclassOf<UBBMViewModelBase> InViewModelClass) const
{
	if (InViewModelClass == nullptr)
	{
		return nullptr;
	}

	if (const UMVVMView* MVVMView = GetExtension<UMVVMView>())
	{
		for (const FMVVMView_Source& ViewSource : MVVMView->GetSources())
		{
			if (ViewSource.Source != nullptr && ViewSource.Source->IsA(InViewModelClass))
			{
				return Cast<UBBMViewModelBase>(ViewSource.Source.Get());
			}
		}
	}

	return nullptr;
}

void UBBMUserWidgetBase::ForEachViewModel(TFunctionRef<void(UBBMViewModelBase*)> Callback) const
{
	if (const UMVVMView* MVVMView = GetExtension<UMVVMView>())
	{
		for (const FMVVMView_Source& ViewSource : MVVMView->GetSources())
		{
			if (UBBMViewModelBase* VM = Cast<UBBMViewModelBase>(ViewSource.Source.Get()))
			{
				Callback(VM);
			}
		}
	}
}


