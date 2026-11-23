#include "UI/Widget/BBMLoadingWidget.h"

UBBMLoadingWidget::UBBMLoadingWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	// 로딩 화면은 FadeLoading 레이어와 상시 뷰포트 유지 정책을 기본값으로 사용함
	LifecyclePolicy = EWorldWidgetLifecyclePolicy::PersistentViewport;
	DefaultZOrderLayer = EWorldWidgetZOrder::FadeLoading;
}
