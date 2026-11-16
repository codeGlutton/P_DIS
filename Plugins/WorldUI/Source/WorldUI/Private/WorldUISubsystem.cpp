#include "WorldUISubsystem.h"
#include "Widget/WorldLayerRootWidget.h"
#include "Widget/WorldUserWidgetBase.h"
#include "ViewModel/WorldViewModelBase.h"
#include "Engine/World.h"

UWorldUISubsystem::UWorldUISubsystem() :
	LayerRootWidget(nullptr)
{
}

void UWorldUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActiveWidgets.Empty();
	ActiveViewModels.Empty();

	if (UWorld* World = GetWorld())
	{
		if (World->IsGameWorld() == true)
		{
			EnsureLayerRootInitialized();
		}
	}
}

void UWorldUISubsystem::Deinitialize()
{
	// 위젯을 먼저 분리한 뒤 ViewModel을 닫아 소멸 시 불필요한 프레젠테이션 역참조를 방지
	ClearAllWidgets();
	ClearAllViewModels();

	if (LayerRootWidget != nullptr)
	{
		if (LayerRootWidget->IsInViewport() == true)
		{
			LayerRootWidget->RemoveFromParent();
		}
		LayerRootWidget = nullptr;
	}

	Super::Deinitialize();
}

UWorldUserWidgetBase* UWorldUISubsystem::CreateAndRegisterWidgetByConfig(FName WidgetKey, const FWorldWidgetConfig& Config)
{
	if (WidgetKey == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldUISubsystem::CreateAndRegisterWidgetByConfig] WidgetKey is NAME_None."));
		return nullptr;
	}

	if (Config.WidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldUISubsystem::CreateAndRegisterWidgetByConfig] WidgetClass is nullptr."));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldUISubsystem::CreateAndRegisterWidgetByConfig] World is nullptr."));
		return nullptr;
	}

	// 동일 키로 이미 등록된 위젯이 있으면 기존 위젯만 분리하고 교체
	if (ActiveWidgets.Contains(WidgetKey) == true)
	{
		RemoveWidget(WidgetKey);
	}

	UWorldUserWidgetBase* NewWidget = CreateWidget<UWorldUserWidgetBase>(World, Config.WidgetClass);
	if (NewWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UWorldUISubsystem::CreateAndRegisterWidgetByConfig] Failed to create widget for key: %s"), *WidgetKey.ToString());
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

UWorldViewModelBase* UWorldUISubsystem::CreateAndRegisterViewModelByConfig(FName WidgetKey, const FWorldViewModelConfig& Config)
{
	if (WidgetKey == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldUISubsystem::CreateAndRegisterViewModelByConfig] WidgetKey is NAME_None."));
		return nullptr;
	}

	if (Config.ViewModelClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldUISubsystem::CreateAndRegisterViewModelByConfig] ViewModelClass is nullptr."));
		return nullptr;
	}

	if (Config.ViewModelClass->HasAnyClassFlags(CLASS_Abstract) == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldUISubsystem::CreateAndRegisterViewModelByConfig] ViewModelClass (%s) is Abstract."), *Config.ViewModelClass->GetName());
		return nullptr;
	}

	// 동일 키로 이미 등록된 ViewModel이 있으면 기존 ViewModel만 해제하고 교체
	if (ActiveViewModels.Contains(WidgetKey) == true)
	{
		RemoveViewModel(WidgetKey);
	}

	UWorldViewModelBase* NewViewModel = NewObject<UWorldViewModelBase>(this, Config.ViewModelClass);
	if (NewViewModel == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UWorldUISubsystem::CreateAndRegisterViewModelByConfig] Failed to create ViewModel for key: %s"), *WidgetKey.ToString());
		return nullptr;
	}

	ActiveViewModels.Add(WidgetKey, NewViewModel);
	return NewViewModel;
}

UWorldUserWidgetBase* UWorldUISubsystem::GetWidget(FName WidgetKey) const
{
	if (const TObjectPtr<UWorldUserWidgetBase>* FoundWidget = ActiveWidgets.Find(WidgetKey))
	{
		return FoundWidget->Get();
	}
	return nullptr;
}

UWorldViewModelBase* UWorldUISubsystem::GetViewModel(FName WidgetKey) const
{
	if (const TObjectPtr<UWorldViewModelBase>* FoundVM = ActiveViewModels.Find(WidgetKey))
	{
		return FoundVM->Get();
	}
	return nullptr;
}

bool UWorldUISubsystem::RemoveWidget(FName WidgetKey)
{
	TObjectPtr<UWorldUserWidgetBase>* FoundWidget = ActiveWidgets.Find(WidgetKey);
	if (FoundWidget == nullptr)
	{
		return false;
	}

	ReleaseWidgetInstance(FoundWidget->Get());
	ActiveWidgets.Remove(WidgetKey);
	return true;
}

bool UWorldUISubsystem::RemoveViewModel(FName WidgetKey)
{
	TObjectPtr<UWorldViewModelBase>* FoundViewModel = ActiveViewModels.Find(WidgetKey);
	if (FoundViewModel == nullptr)
	{
		return false;
	}

	ReleaseViewModelInstance(FoundViewModel->Get());
	ActiveViewModels.Remove(WidgetKey);
	return true;
}

bool UWorldUISubsystem::RemoveUI(FName WidgetKey)
{
	const bool bRemovedWidget = RemoveWidget(WidgetKey);
	const bool bRemovedViewModel = RemoveViewModel(WidgetKey);
	return (bRemovedWidget == true) || (bRemovedViewModel == true);
}

void UWorldUISubsystem::ClearAllWidgets()
{
	for (TPair<FName, TObjectPtr<UWorldUserWidgetBase>>& Pair : ActiveWidgets)
	{
		ReleaseWidgetInstance(Pair.Value.Get());
	}
	ActiveWidgets.Empty();
}

void UWorldUISubsystem::ClearAllViewModels()
{
	for (TPair<FName, TObjectPtr<UWorldViewModelBase>>& Pair : ActiveViewModels)
	{
		ReleaseViewModelInstance(Pair.Value.Get());
	}
	ActiveViewModels.Empty();
}

int32 UWorldUISubsystem::GetRegisteredWidgetCount() const
{
	return ActiveWidgets.Num();
}

int32 UWorldUISubsystem::GetRegisteredViewModelCount() const
{
	return ActiveViewModels.Num();
}

bool UWorldUISubsystem::AddWidgetToLayer(UUserWidget* InWidget, EWorldWidgetZOrder InLayer)
{
	if (EnsureLayerRootInitialized() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldUISubsystem::AddWidgetToLayer] Failed to initialize LayerRootWidget."));
		return false;
	}

	return LayerRootWidget->AddWidgetToLayer(InWidget, InLayer);
}

bool UWorldUISubsystem::BringWidgetToLayerTop(UUserWidget* InWidget)
{
	if (LayerRootWidget == nullptr)
	{
		return false;
	}

	return LayerRootWidget->BringWidgetToLayerTop(InWidget);
}

bool UWorldUISubsystem::IsWidgetInAnyLayer(const UWidget* InWidget) const
{
	if (LayerRootWidget == nullptr)
	{
		return false;
	}

	return LayerRootWidget->IsWidgetInAnyLayer(InWidget);
}

#if WITH_EDITOR
void UWorldUISubsystem::SetLayerZOrderMaxThresholdForTesting(int32 InThreshold)
{
	if (EnsureLayerRootInitialized() == true)
	{
		LayerRootWidget->SetLayerZOrderMaxThresholdForTesting(InThreshold);
	}
}
#endif

bool UWorldUISubsystem::EnsureLayerRootInitialized()
{
	UWorld* World = GetWorld();
	if (World == nullptr || World->IsGameWorld() == false)
	{
		return false;
	}

	if (LayerRootWidget == nullptr)
	{
		LayerRootWidget = CreateWidget<UWorldLayerRootWidget>(World, UWorldLayerRootWidget::StaticClass());
		if (LayerRootWidget == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("[UWorldUISubsystem::EnsureLayerRootInitialized] Failed to create UWorldLayerRootWidget."));
			return false;
		}

		LayerRootWidget->InitializeLayerStructure();
	}

	// 헤드리스 테스트 월드처럼 뷰포트 클라이언트가 없을 때는 AddToViewport 경고 없이 레이어 구조만 유지함
	if (LayerRootWidget->IsInViewport() == false && World->GetGameViewport() != nullptr)
	{
		LayerRootWidget->AddToViewport(0);
	}

	return LayerRootWidget != nullptr;
}

void UWorldUISubsystem::ReleaseWidgetInstance(UWorldUserWidgetBase* InWidget)
{
	if (InWidget == nullptr)
	{
		return;
	}

	InWidget->SetViewModelInstance(nullptr);

	if (InWidget->IsInLayer() == true)
	{
		InWidget->RemoveFromParent();
	}
}

void UWorldUISubsystem::ReleaseViewModelInstance(UWorldViewModelBase* InViewModel)
{
	if (InViewModel == nullptr)
	{
		return;
	}

	// 제거 대상 ViewModel을 참조 중인 활성 위젯의 바인딩을 먼저 해제함
	for (TPair<FName, TObjectPtr<UWorldUserWidgetBase>>& Pair : ActiveWidgets)
	{
		UWorldUserWidgetBase* Widget = Pair.Value.Get();
		if (Widget != nullptr && Widget->GetViewModel() == InViewModel)
		{
			Widget->SetViewModelInstance(nullptr);
		}
	}

	InViewModel->Close();
}