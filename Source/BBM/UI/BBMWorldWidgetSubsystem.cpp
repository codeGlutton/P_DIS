/*****************************************************************//**
 * @file   BBMWorldWidgetSubsystem.cpp
 * @brief  BBM(비트 배낭 상인) 월드 레벨 전역 위젯 관리 서브시스템 구현
 * @date   2026-11-06
 *********************************************************************/

#include "UI/BBMWorldWidgetSubsystem.h"
#include "UI/BBMUserWidgetBase.h"
#include "Engine/World.h"

UBBMWorldWidgetSubsystem::UBBMWorldWidgetSubsystem()
{
}

void UBBMWorldWidgetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActiveWidgets.Empty();
}

void UBBMWorldWidgetSubsystem::Deinitialize()
{
	// 월드 종료 또는 레벨 전환 시 모든 위젯을 뷰포트에서 안전하게 정리하여 GC 누수 방지
	ClearAllWidgets();
	Super::Deinitialize();
}

UBBMUserWidgetBase* UBBMWorldWidgetSubsystem::CreateAndRegisterWidget(
	TSubclassOf<UBBMUserWidgetBase> WidgetClass,
	FName WidgetKey,
	EBBMWidgetZOrder ZOrderLayer,
	bool bAutoOpen)
{
	if (WidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldWidgetSubsystem::CreateAndRegisterWidget] WidgetClass is nullptr."));
		return nullptr;
	}

	if (WidgetKey == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldWidgetSubsystem::CreateAndRegisterWidget] WidgetKey is NAME_None."));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMWorldWidgetSubsystem::CreateAndRegisterWidget] World is nullptr."));
		return nullptr;
	}

	// 동일 키로 이미 등록된 위젯이 있는 경우 기존 위젯을 정리하고 교체
	if (ActiveWidgets.Contains(WidgetKey) == true)
	{
		RemoveWidget(WidgetKey);
	}

	// 위젯 생성
	UBBMUserWidgetBase* NewWidget = CreateWidget<UBBMUserWidgetBase>(World, WidgetClass);
	if (NewWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UBBMWorldWidgetSubsystem::CreateAndRegisterWidget] Failed to create widget for key: %s"), *WidgetKey.ToString());
		return nullptr;
	}

	// 서브시스템 맵에 등록
	ActiveWidgets.Add(WidgetKey, NewWidget);

	// 자동 오픈 옵션이 켜져 있는 경우 Open() 호출
	if (bAutoOpen == true)
	{
		NewWidget->Open();
	}

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

bool UBBMWorldWidgetSubsystem::RemoveWidget(FName WidgetKey)
{
	if (TObjectPtr<UBBMUserWidgetBase>* FoundWidget = ActiveWidgets.Find(WidgetKey))
	{
		if (UBBMUserWidgetBase* Widget = FoundWidget->Get())
		{
			Widget->Close();
		}
		ActiveWidgets.Remove(WidgetKey);
		return true;
	}
	return false;
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
}

int32 UBBMWorldWidgetSubsystem::GetRegisteredWidgetCount() const
{
	return ActiveWidgets.Num();
}

