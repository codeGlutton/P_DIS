/*****************************************************************//**
 * @file   BBMWorldUISubsystem.cpp
 * @brief  BBM(비트 배낭 상인) 월드 레벨 전역 UI(위젯 및 ViewModel) 관리 서브시스템 구현
 * @date   2026-11-12
 *********************************************************************/

#include "UI/BBMWorldUISubsystem.h"
#include "UI/Widget/BBMUserWidgetBase.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "Engine/World.h"

UBBMWorldUISubsystem::UBBMWorldUISubsystem()
{
}

void UBBMWorldUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActiveWidgets.Empty();
	ActiveViewModels.Empty();
}

void UBBMWorldUISubsystem::Deinitialize()
{
	// 위젯을 먼저 분리한 뒤 ViewModel을 닫아 소멸 시 불필요한 프레젠테이션 역참조를 방지
	ClearAllWidgets();
	ClearAllViewModels();
	Super::Deinitialize();
}

UBBMUserWidgetBase* UBBMWorldUISubsystem::CreateAndRegisterWidgetByConfig(FName WidgetKey, const FBBMWorldWidgetConfig& Config)
{
	if (WidgetKey == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldUISubsystem::CreateAndRegisterWidgetByConfig] WidgetKey is NAME_None."));
		return nullptr;
	}

	if (Config.WidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldUISubsystem::CreateAndRegisterWidgetByConfig] WidgetClass is nullptr."));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldUISubsystem::CreateAndRegisterWidgetByConfig] World is nullptr."));
		return nullptr;
	}

	// 동일 키로 이미 등록된 위젯이 있으면 기존 위젯만 분리하고 교체
	if (ActiveWidgets.Contains(WidgetKey) == true)
	{
		RemoveWidget(WidgetKey);
	}

	UBBMUserWidgetBase* NewWidget = CreateWidget<UBBMUserWidgetBase>(World, Config.WidgetClass);
	if (NewWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMWorldUISubsystem::CreateAndRegisterWidgetByConfig] Failed to create widget for key: %s"), *WidgetKey.ToString());
		return nullptr;
	}

	// ZOrderLayer 오버라이드가 요청된 경우에만 클래스 기본값을 덮어씀
	if (Config.bOverrideZOrder == true)
	{
		NewWidget->SetDefaultZOrderLayer(Config.ZOrderLayer);
	}

	ActiveWidgets.Add(WidgetKey, NewWidget);
	return NewWidget;
}

UBBMViewModelBase* UBBMWorldUISubsystem::CreateAndRegisterViewModelByConfig(FName WidgetKey, const FBBMWorldViewModelConfig& Config)
{
	if (WidgetKey == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldUISubsystem::CreateAndRegisterViewModelByConfig] WidgetKey is NAME_None."));
		return nullptr;
	}

	if (Config.ViewModelClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldUISubsystem::CreateAndRegisterViewModelByConfig] ViewModelClass is nullptr."));
		return nullptr;
	}

	if (Config.ViewModelClass->HasAnyClassFlags(CLASS_Abstract) == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldUISubsystem::CreateAndRegisterViewModelByConfig] ViewModelClass (%s) is Abstract."), *Config.ViewModelClass->GetName());
		return nullptr;
	}

	// 동일 키로 이미 등록된 ViewModel이 있으면 기존 ViewModel만 해제하고 교체
	if (ActiveViewModels.Contains(WidgetKey) == true)
	{
		RemoveViewModel(WidgetKey);
	}

	UBBMViewModelBase* NewViewModel = NewObject<UBBMViewModelBase>(this, Config.ViewModelClass);
	if (NewViewModel == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMWorldUISubsystem::CreateAndRegisterViewModelByConfig] Failed to create ViewModel for key: %s"), *WidgetKey.ToString());
		return nullptr;
	}

	ActiveViewModels.Add(WidgetKey, NewViewModel);
	return NewViewModel;
}

UBBMUserWidgetBase* UBBMWorldUISubsystem::GetWidget(FName WidgetKey) const
{
	if (const TObjectPtr<UBBMUserWidgetBase>* FoundWidget = ActiveWidgets.Find(WidgetKey))
	{
		return FoundWidget->Get();
	}
	return nullptr;
}

UBBMViewModelBase* UBBMWorldUISubsystem::GetViewModel(FName WidgetKey) const
{
	if (const TObjectPtr<UBBMViewModelBase>* FoundVM = ActiveViewModels.Find(WidgetKey))
	{
		return FoundVM->Get();
	}
	return nullptr;
}

bool UBBMWorldUISubsystem::RemoveWidget(FName WidgetKey)
{
	TObjectPtr<UBBMUserWidgetBase>* FoundWidget = ActiveWidgets.Find(WidgetKey);
	if (FoundWidget == nullptr)
	{
		return false;
	}

	ReleaseWidgetInstance(FoundWidget->Get());
	ActiveWidgets.Remove(WidgetKey);
	return true;
}

bool UBBMWorldUISubsystem::RemoveViewModel(FName WidgetKey)
{
	TObjectPtr<UBBMViewModelBase>* FoundViewModel = ActiveViewModels.Find(WidgetKey);
	if (FoundViewModel == nullptr)
	{
		return false;
	}

	ReleaseViewModelInstance(FoundViewModel->Get());
	ActiveViewModels.Remove(WidgetKey);
	return true;
}

bool UBBMWorldUISubsystem::RemoveUI(FName WidgetKey)
{
	const bool bRemovedWidget = RemoveWidget(WidgetKey);
	const bool bRemovedViewModel = RemoveViewModel(WidgetKey);
	return (bRemovedWidget == true) || (bRemovedViewModel == true);
}

void UBBMWorldUISubsystem::ClearAllWidgets()
{
	for (TPair<FName, TObjectPtr<UBBMUserWidgetBase>>& Pair : ActiveWidgets)
	{
		ReleaseWidgetInstance(Pair.Value.Get());
	}
	ActiveWidgets.Empty();
}

void UBBMWorldUISubsystem::ClearAllViewModels()
{
	for (TPair<FName, TObjectPtr<UBBMViewModelBase>>& Pair : ActiveViewModels)
	{
		ReleaseViewModelInstance(Pair.Value.Get());
	}
	ActiveViewModels.Empty();
}

int32 UBBMWorldUISubsystem::GetRegisteredWidgetCount() const
{
	return ActiveWidgets.Num();
}

int32 UBBMWorldUISubsystem::GetRegisteredViewModelCount() const
{
	return ActiveViewModels.Num();
}

void UBBMWorldUISubsystem::ReleaseWidgetInstance(UBBMUserWidgetBase* InWidget)
{
	if (InWidget == nullptr)
	{
		return;
	}

	InWidget->DetachPresentationImmediate();
	InWidget->SetViewModelInstance(nullptr);
}

void UBBMWorldUISubsystem::ReleaseViewModelInstance(UBBMViewModelBase* InViewModel)
{
	if (InViewModel == nullptr)
	{
		return;
	}

	// 제거 대상 ViewModel을 참조 중인 활성 위젯의 바인딩을 먼저 해제함
	for (TPair<FName, TObjectPtr<UBBMUserWidgetBase>>& Pair : ActiveWidgets)
	{
		UBBMUserWidgetBase* Widget = Pair.Value.Get();
		if (Widget != nullptr && Widget->GetViewModel() == InViewModel)
		{
			Widget->SetViewModelInstance(nullptr);
		}
	}

	InViewModel->Close();
}
