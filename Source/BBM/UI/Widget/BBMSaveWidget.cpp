#include "UI/Widget/BBMSaveWidget.h"

UBBMSaveWidget::UBBMSaveWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	// 세이브 인디케이터는 로딩 상단 알림 레이어와 상시 뷰포트 유지 정책을 기본값으로 사용함
	LifecyclePolicy = EWorldWidgetLifecyclePolicy::PersistentViewport;
	DefaultZOrderLayer = EWorldWidgetZOrder::OverlayNotification;
}
