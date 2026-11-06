/*****************************************************************//**
 * @file   BBMUserWidgetBase.cpp
 * @brief  BBM(비트 배낭 상인) 공용 사용자 위젯 베이스 클래스 구현
 * @date   2026-11-06
 *********************************************************************/

#include "UI/BBMUserWidgetBase.h"

UBBMUserWidgetBase::UBBMUserWidgetBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	LifecyclePolicy(EBBMWidgetLifecyclePolicy::PersistentViewport),
	DefaultZOrderLayer(EBBMWidgetZOrder::Base),
	bIsOpen(false)
{
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

	OnClosed();
	BP_OnClosed();
}

void UBBMUserWidgetBase::OpenWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	PendingOpenAnimDynamicCallback = OnComplete;
	PendingOpenAnimCallback.Unbind();

	// 가시성 및 뷰포트 상태 먼저 확보
	Open();

	UWidgetAnimation* Anim = OpenAnimConfig.Animation.Get();
	if (Anim != nullptr)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleOpenAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimation(Anim, 0.0f, 1, OpenAnimConfig.PlayMode, OpenAnimConfig.PlaybackSpeed);
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

	// 가시성 및 뷰포트 상태 먼저 확보
	Open();

	UWidgetAnimation* Anim = OpenAnimConfig.Animation.Get();
	if (Anim != nullptr)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleOpenAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimation(Anim, 0.0f, 1, OpenAnimConfig.PlayMode, OpenAnimConfig.PlaybackSpeed);
	}
	else
	{
		HandleOpenAnimationFinished();
	}
}

void UBBMUserWidgetBase::CloseWithAnimDynamic(const FBBMOnWidgetAnimFinishedDynamicSignature& OnComplete)
{
	PendingCloseAnimDynamicCallback = OnComplete;
	PendingCloseAnimCallback.Unbind();

	UWidgetAnimation* Anim = CloseAnimConfig.Animation.Get();
	if (Anim != nullptr)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleCloseAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimation(Anim, 0.0f, 1, CloseAnimConfig.PlayMode, CloseAnimConfig.PlaybackSpeed);
	}
	else
	{
		HandleCloseAnimationFinished();
	}
}

void UBBMUserWidgetBase::CloseWithAnim(FBBMOnWidgetAnimFinishedSignature OnComplete)
{
	PendingCloseAnimCallback = OnComplete;
	PendingCloseAnimDynamicCallback.Clear();

	UWidgetAnimation* Anim = CloseAnimConfig.Animation.Get();
	if (Anim != nullptr)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UBBMUserWidgetBase::HandleCloseAnimationFinished);
		BindToAnimationFinished(Anim, FinishedDelegate);

		PlayAnimation(Anim, 0.0f, 1, CloseAnimConfig.PlayMode, CloseAnimConfig.PlaybackSpeed);
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

