#include "Widget/WorldUserWidgetBase.h"
#include "WorldUISubsystem.h"
#include "ViewModel/WorldViewModelBase.h"
#include "ViewModel/SingleAnimTransition.h"
#include "ViewModel/DualAnimTransition.h"
#include "Animation/WidgetAnimation.h"
#include "Animation/WidgetAnimationState.h"
#include "Engine/World.h"
#include "View/MVVMView.h"

UWorldUserWidgetBase::UWorldUserWidgetBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	LifecyclePolicy(EWorldWidgetLifecyclePolicy::PersistentViewport),
	DefaultZOrderLayer(EWorldWidgetZOrder::Base),
	ViewModel(nullptr)
{
	OpenAnimConfig.PlayMode = EWorldWidgetAnimPlayMode::RelativeForward;
	CloseAnimConfig.PlayMode = EWorldWidgetAnimPlayMode::RelativeReverse;
}

void UWorldUserWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// 상시 뷰포트 유지 정책인 경우 현재 표시 중이 아니라면 기본 숨김 상태
	if (LifecyclePolicy == EWorldWidgetLifecyclePolicy::PersistentViewport)
	{
		const bool bIsViewModelShowing = (ViewModel.IsValid() == true && ViewModel->IsShowing() == true);
		if (bIsViewModelShowing == false)
		{
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UWorldUserWidgetBase::BeginDestroy()
{
	if (UWorldViewModelBase* CurrentVM = ViewModel.Get())
	{
		ClearViewModelPresentationHandlers(CurrentVM);
		ViewModel.Reset();
	}

	Super::BeginDestroy();
}

void UWorldUserWidgetBase::SetViewModelInstance(UWorldViewModelBase* InViewModel)
{
	if (ViewModel.Get() == InViewModel)
	{
		return;
	}

	// 기존 뷰모델의 프레젠테이션 요청 핸들러 안전 해제
	if (UWorldViewModelBase* OldVM = ViewModel.Get())
	{
		ClearViewModelPresentationHandlers(OldVM);
		ViewModel.Reset();
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

void UWorldUserWidgetBase::OnOpened()
{
}

void UWorldUserWidgetBase::OnClosed()
{
}

void UWorldUserWidgetBase::BindViewModelPresentationHandlers(UWorldViewModelBase* InViewModel)
{
	if (InViewModel == nullptr)
	{
		return;
	}

	if (UDualAnimTransition* DualTransition = InViewModel->GetOpenCloseTransition())
	{
		DualTransition->BindPresentationHandlers(
			this,
			FWorldUIOnPresentationTransitionRequestedSignature::CreateUObject(this, &UWorldUserWidgetBase::HandleOpenPresentationRequested),
			FWorldUIOnPresentationTransitionRequestedSignature::CreateUObject(this, &UWorldUserWidgetBase::HandleClosePresentationRequested)
		);
	}
}

void UWorldUserWidgetBase::ClearViewModelPresentationHandlers(UWorldViewModelBase* InViewModel)
{
	if (InViewModel == nullptr)
	{
		return;
	}

	if (UDualAnimTransition* DualTransition = InViewModel->GetOpenCloseTransition())
	{
		UnbindAllFromAnimationFinishedByConfig(OpenAnimConfig);
		UnbindAllFromAnimationFinishedByConfig(CloseAnimConfig);
		StopAnimationByConfig(OpenAnimConfig);
		StopAnimationByConfig(CloseAnimConfig);

		DualTransition->ClearPresentationHandlers(this);
	}
}

void UWorldUserWidgetBase::HandleSingleTransitionRequested(
	bool bWithAnim,
	USingleAnimTransition* InTransition,
	const FWorldWidgetAnimConfig& InConfig
)
{
	UnbindAllFromAnimationFinishedByConfig(InConfig);

	const bool bCanPlayAnim = (bWithAnim == true)
		&& (IsInLayer() == true)
		&& (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(InTransition, &USingleAnimTransition::NotifyAnimFinished);

		BindToAnimationFinishedByConfig(InConfig, FinishedDelegate);
		PlayAnimationByConfig(InConfig);
	}
	else
	{
		StopAnimationByConfig(InConfig);
		if (InTransition != nullptr)
		{
			InTransition->NotifyAnimFinished();
		}
	}
}

void UWorldUserWidgetBase::HandleDualTransitionStartRequested(
	bool bWithAnim,
	UDualAnimTransition* InDualTransition,
	const FWorldWidgetAnimConfig& InStartConfig,
	const FWorldWidgetAnimConfig& InEndConfig
)
{
	UnbindAllFromAnimationFinishedByConfig(InEndConfig);
	UnbindAllFromAnimationFinishedByConfig(InStartConfig);
	StopAnimationByConfig(InEndConfig);

	const bool bCanPlayAnim = (bWithAnim == true)
		&& (IsInLayer() == true)
		&& (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(InDualTransition, &UDualAnimTransition::NotifyStartTransitionFinished);

		BindToAnimationFinishedByConfig(InStartConfig, FinishedDelegate);
		PlayAnimationByConfig(InStartConfig);
	}
	else
	{
		StopAnimationByConfig(InStartConfig);
		if (InDualTransition != nullptr)
		{
			InDualTransition->NotifyStartTransitionFinished();
		}
	}
}

void UWorldUserWidgetBase::HandleDualTransitionEndRequested(
	bool bWithAnim,
	UDualAnimTransition* InDualTransition,
	const FWorldWidgetAnimConfig& InStartConfig,
	const FWorldWidgetAnimConfig& InEndConfig
)
{
	UnbindAllFromAnimationFinishedByConfig(InStartConfig);
	UnbindAllFromAnimationFinishedByConfig(InEndConfig);
	StopAnimationByConfig(InStartConfig);

	const bool bCanPlayAnim = (bWithAnim == true)
		&& (IsInLayer() == true)
		&& (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(InDualTransition, &UDualAnimTransition::NotifyEndTransitionFinished);

		BindToAnimationFinishedByConfig(InEndConfig, FinishedDelegate);
		PlayAnimationByConfig(InEndConfig);
	}
	else
	{
		StopAnimationByConfig(InEndConfig);
		if (InDualTransition != nullptr)
		{
			InDualTransition->NotifyEndTransitionFinished();
		}
	}
}

void UWorldUserWidgetBase::HandleOpenPresentationRequested(bool bWithAnim)
{
	UnbindAllFromAnimationFinishedByConfig(CloseAnimConfig);
	UnbindAllFromAnimationFinishedByConfig(OpenAnimConfig);
	StopAnimationByConfig(CloseAnimConfig);

	ApplyOpenViewportPresentation();

	OnOpened();
	BP_OnOpened();

	const bool bCanPlayAnim = (bWithAnim == true)
		&& (IsInLayer() == true)
		&& (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UWorldUserWidgetBase::HandleOpenAnimationFinished);

		BindToAnimationFinishedByConfig(OpenAnimConfig, FinishedDelegate);
		PlayAnimationByConfig(OpenAnimConfig);
	}
	else
	{
		StopAnimationByConfig(OpenAnimConfig);
		HandleOpenAnimationFinished();
	}
}

void UWorldUserWidgetBase::HandleClosePresentationRequested(bool bWithAnim)
{
	UnbindAllFromAnimationFinishedByConfig(OpenAnimConfig);
	UnbindAllFromAnimationFinishedByConfig(CloseAnimConfig);
	StopAnimationByConfig(OpenAnimConfig);

	const bool bCanPlayAnim = (bWithAnim == true)
		&& (IsInLayer() == true)
		&& (IsVisible() == true);

	if (bCanPlayAnim == true)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &UWorldUserWidgetBase::HandleCloseAnimationFinished);

		BindToAnimationFinishedByConfig(CloseAnimConfig, FinishedDelegate);
		PlayAnimationByConfig(CloseAnimConfig);
	}
	else
	{
		StopAnimationByConfig(CloseAnimConfig);
		HandleCloseAnimationFinished();
	}
}

void UWorldUserWidgetBase::HandleOpenAnimationFinished()
{
	if (ViewModel != nullptr && ViewModel->GetOpenCloseTransition() != nullptr)
	{
		ViewModel->GetOpenCloseTransition()->NotifyStartTransitionFinished();
	}
}

void UWorldUserWidgetBase::HandleCloseAnimationFinished()
{
	ApplyCloseViewportPresentation();

	OnClosed();
	BP_OnClosed();

	if (ViewModel != nullptr && ViewModel->GetOpenCloseTransition() != nullptr)
	{
		ViewModel->GetOpenCloseTransition()->NotifyEndTransitionFinished();
	}
}

bool UWorldUserWidgetBase::AddToLayer(EWorldWidgetZOrder InLayer)
{
	// 이미 뷰포트 또는 레이어 위에 존재하면 중복 등록 및 타 레이어 이동을 허용하지 않음
	if (IsInLayer() == true)
	{
		return false;
	}

	UWorldUISubsystem* UIWorldSubsystem = ResolveWorldUISubsystem();
	if (UIWorldSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldUserWidgetBase::AddToLayer] UWorldUISubsystem is unavailable."));
		return false;
	}

	return UIWorldSubsystem->AddWidgetToLayer(this, InLayer);
}

bool UWorldUserWidgetBase::BringToLayerTop()
{
	UWorldUISubsystem* UIWorldSubsystem = ResolveWorldUISubsystem();
	if (UIWorldSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldUserWidgetBase::BringToLayerTop] UWorldUISubsystem is unavailable."));
		return false;
	}

	return UIWorldSubsystem->BringWidgetToLayerTop(this);
}

bool UWorldUserWidgetBase::IsInLayer() const
{
	if (IsInViewport() == true)
	{
		return true;
	}

	const UWorldUISubsystem* UIWorldSubsystem = ResolveWorldUISubsystem();
	return (UIWorldSubsystem != nullptr) && (UIWorldSubsystem->IsWidgetInAnyLayer(this) == true);
}

UWorldUISubsystem* UWorldUserWidgetBase::ResolveWorldUISubsystem() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	return World->GetSubsystem<UWorldUISubsystem>();
}

void UWorldUserWidgetBase::ApplyOpenViewportPresentation()
{
	if (IsInLayer() == false)
	{
		AddToLayer(DefaultZOrderLayer);
	}
	else if (LifecyclePolicy == EWorldWidgetLifecyclePolicy::PersistentViewport)
	{
		// 재오픈 시 동일 레이어 캔버스 내 형제 위젯들보다 상위에 배치되도록 ZOrder를 최상단으로 승격함
		BringToLayerTop();
	}

	if (LifecyclePolicy == EWorldWidgetLifecyclePolicy::PersistentViewport)
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UWorldUserWidgetBase::ApplyCloseViewportPresentation()
{
	if (LifecyclePolicy == EWorldWidgetLifecyclePolicy::PersistentViewport)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		if (IsInLayer() == true)
		{
			RemoveFromParent();
		}
	}
}

void UWorldUserWidgetBase::PlayAnimationByConfig(const FWorldWidgetAnimConfig& InConfig)
{
	UWidgetAnimation* Anim = InConfig.GetAnimation(this);
	UUserWidget* OwnerWidget = InConfig.GetOwnerWidget(this);
	if (Anim == nullptr || OwnerWidget == nullptr)
	{
		return;
	}

	const int32 SafeLoopCount = FMath::Max(0, InConfig.LoopCount);

	switch (InConfig.PlayMode)
	{
	case EWorldWidgetAnimPlayMode::RelativeForward:
		OwnerWidget->PlayAnimationForward(Anim, InConfig.PlaybackSpeed);
		break;

	case EWorldWidgetAnimPlayMode::RelativeReverse:
		OwnerWidget->PlayAnimationReverse(Anim, InConfig.PlaybackSpeed);
		break;

	case EWorldWidgetAnimPlayMode::Forward:
		OwnerWidget->PlayAnimation(Anim, 0.0f, SafeLoopCount, EUMGSequencePlayMode::Forward, InConfig.PlaybackSpeed);
		break;

	case EWorldWidgetAnimPlayMode::Reverse:
		OwnerWidget->PlayAnimation(Anim, 0.0f, SafeLoopCount, EUMGSequencePlayMode::Reverse, InConfig.PlaybackSpeed);
		break;

	case EWorldWidgetAnimPlayMode::PingPong:
		OwnerWidget->PlayAnimation(Anim, 0.0f, SafeLoopCount, EUMGSequencePlayMode::PingPong, InConfig.PlaybackSpeed);
		break;
	}
}

void UWorldUserWidgetBase::StopAnimationByConfig(const FWorldWidgetAnimConfig& InConfig)
{
	UWidgetAnimation* Anim = InConfig.GetAnimation(this);
	UUserWidget* OwnerWidget = InConfig.GetOwnerWidget(this);
	if (Anim == nullptr || OwnerWidget == nullptr)
	{
		return;
	}

	OwnerWidget->StopAnimation(Anim);
}

void UWorldUserWidgetBase::BindToAnimationFinishedByConfig(const FWorldWidgetAnimConfig& InConfig, const FWidgetAnimationDynamicEvent& InFinishedDelegate)
{
	UWidgetAnimation* Anim = InConfig.GetAnimation(this);
	UUserWidget* OwnerWidget = InConfig.GetOwnerWidget(this);
	if (Anim == nullptr || OwnerWidget == nullptr)
	{
		return;
	}

	OwnerWidget->BindToAnimationFinished(Anim, InFinishedDelegate);
}

void UWorldUserWidgetBase::UnbindAllFromAnimationFinishedByConfig(const FWorldWidgetAnimConfig& InConfig)
{
	UWidgetAnimation* Anim = InConfig.GetAnimation(this);
	UUserWidget* OwnerWidget = InConfig.GetOwnerWidget(this);
	if (Anim == nullptr || OwnerWidget == nullptr)
	{
		return;
	}

	OwnerWidget->UnbindAllFromAnimationFinished(Anim);
}