/*****************************************************************//**
 * @file   BBMWorldWidgetSubsystem.cpp
 * @brief  BBM(비트 배낭 상인) 월드 레벨 전역 위젯 관리 서브시스템 구현
 * @date   2026-11-06
 *********************************************************************/

#include "UI/BBMWorldWidgetSubsystem.h"
#include "UI/BBMUserWidgetBase.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "Engine/World.h"

UBBMWorldWidgetSubsystem::UBBMWorldWidgetSubsystem()
{
}

void UBBMWorldWidgetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActiveWidgets.Empty();
	ActiveViewModels.Empty();
}

void UBBMWorldWidgetSubsystem::Deinitialize()
{
	// 월드 종료 또는 레벨 전환 시 모든 위젯 및 뷰모델을 안전하게 정리하여 GC 누수 방지
	ClearAllWidgets();
	Super::Deinitialize();
}

UBBMUserWidgetBase* UBBMWorldWidgetSubsystem::CreateAndRegisterWidgetByConfig(const FBBMWorldWidgetConfig& Config)
{
	if (Config.WidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldWidgetSubsystem::CreateAndRegisterWidgetByConfig] WidgetClass is nullptr."));
		return nullptr;
	}

	if (Config.WidgetKey == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldWidgetSubsystem::CreateAndRegisterWidgetByConfig] WidgetKey is NAME_None."));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldWidgetSubsystem::CreateAndRegisterWidgetByConfig] World is nullptr."));
		return nullptr;
	}

	// 동일 키로 이미 등록된 위젯이 있는 경우 기존 위젯 및 뷰모델을 정리하고 교체
	if (ActiveWidgets.Contains(Config.WidgetKey) == true)
	{
		RemoveWidget(Config.WidgetKey);
	}

	// 1. 위젯 생성 (위젯은 필수)
	UBBMUserWidgetBase* NewWidget = CreateWidget<UBBMUserWidgetBase>(World, Config.WidgetClass);
	if (NewWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMWorldWidgetSubsystem::CreateAndRegisterWidgetByConfig] Failed to create widget for key: %s"), *Config.WidgetKey.ToString());
		return nullptr;
	}

	// ZOrderLayer 오버라이드가 요청된 경우에만 클래스 기본값을 덮어씀
	if (Config.bOverrideZOrder == true)
	{
		NewWidget->SetDefaultZOrderLayer(Config.ZOrderLayer);
	}

	// 2. MVVM ViewModel 선택적 생성 및 Manual 바인딩 주입
	if (Config.ViewModelClass != nullptr)
	{
		UBBMViewModelBase* NewViewModel = NewObject<UBBMViewModelBase>(this, Config.ViewModelClass);
		if (NewViewModel != nullptr)
		{
			ActiveViewModels.Add(Config.WidgetKey, NewViewModel);
			NewWidget->SetViewModelInstance(NewViewModel);
		}
	}

	// 서브시스템 맵에 등록
	ActiveWidgets.Add(Config.WidgetKey, NewWidget);

	return NewWidget;
}

UBBMUserWidgetBase* UBBMWorldWidgetSubsystem::GetWidget(FName WidgetKey) const
{
	if (const TObjectPtr<UBBMUserWidgetBase>* FoundWidget = ActiveWidgets.Find(WidgetKey))
	{
		return FoundWidget->Get();
	}
	return nullptr;
}

UBBMViewModelBase* UBBMWorldWidgetSubsystem::GetViewModel(FName WidgetKey) const
{
	if (const TObjectPtr<UBBMViewModelBase>* FoundVM = ActiveViewModels.Find(WidgetKey))
	{
		return FoundVM->Get();
	}
	return nullptr;
}

bool UBBMWorldWidgetSubsystem::RemoveWidget(FName WidgetKey)
{
	bool bRemoved = false;

	if (TObjectPtr<UBBMUserWidgetBase>* FoundWidget = ActiveWidgets.Find(WidgetKey))
	{
		if (UBBMUserWidgetBase* Widget = FoundWidget->Get())
		{
			Widget->Close();
		}
		ActiveWidgets.Remove(WidgetKey);
		bRemoved = true;
	}

	if (TObjectPtr<UBBMViewModelBase>* FoundVM = ActiveViewModels.Find(WidgetKey))
	{
		ActiveViewModels.Remove(WidgetKey);
		bRemoved = true;
	}

	return bRemoved;
}

void UBBMWorldWidgetSubsystem::ClearAllWidgets()
{
	for (auto& Pair : ActiveWidgets)
	{
		if (UBBMUserWidgetBase* Widget = Pair.Value.Get())
		{
			Widget->Close();
		}
	}
	ActiveWidgets.Empty();

	ActiveViewModels.Empty();
}

int32 UBBMWorldWidgetSubsystem::GetRegisteredWidgetCount() const
{
	return ActiveWidgets.Num();
}

