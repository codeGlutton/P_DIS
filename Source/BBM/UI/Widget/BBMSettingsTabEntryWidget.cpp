#include "UI/Widget/BBMSettingsTabEntryWidget.h"
#include "Components/Button.h"

void UBBMSettingsTabEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (MainButtonName == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMSettingsTabEntryWidget::NativeOnInitialized] MainButtonName이 NAME_None으로 설정되지 않음."));
		return;
	}

	CachedMainButton = Cast<UButton>(GetWidgetFromName(MainButtonName));
	if (CachedMainButton == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBBMSettingsTabEntryWidget::NativeOnInitialized] MainButtonName(%s)에 해당하는 UButton을 찾을 수 없음."), *MainButtonName.ToString());
		return;
	}

	// 초기 기본 스타일을 보존해 비선택 복원 시 그대로 되돌림
	CachedDefaultButtonStyle = CachedMainButton->GetStyle();
	CachedMainButton->OnClicked.AddUniqueDynamic(this, &UBBMSettingsTabEntryWidget::HandleMainButtonClicked);
}

void UBBMSettingsTabEntryWidget::BeginDestroy()
{
	if (CachedMainButton != nullptr)
	{
		CachedMainButton->OnClicked.RemoveDynamic(this, &UBBMSettingsTabEntryWidget::HandleMainButtonClicked);
		CachedMainButton = nullptr;
	}

	Super::BeginDestroy();
}

void UBBMSettingsTabEntryWidget::SetTabSelected(bool bInIsSelected)
{
	if (CachedMainButton == nullptr || bIsSelected == bInIsSelected)
	{
		return;
	}

	bIsSelected = bInIsSelected;
	ApplySelectionState();
}

void UBBMSettingsTabEntryWidget::HandleMainButtonClicked()
{
	if (CachedMainButton == nullptr || bIsSelected == true)
	{
		return;
	}

	OnRequestSelectNative.Broadcast();
	OnRequestSelect.Broadcast();
}

void UBBMSettingsTabEntryWidget::ApplySelectionState()
{
	if (CachedMainButton == nullptr)
	{
		return;
	}

	// 활성화(IsEnabled) 변경 시 발생하는 색상 변조를 피하기 위해 가시성으로 사용자 입력을 차단·허용함
	CachedMainButton->SetVisibility(bIsSelected == true ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Visible);
	CachedMainButton->SetStyle(bIsSelected == true ? SelectedButtonStyle : CachedDefaultButtonStyle);
}