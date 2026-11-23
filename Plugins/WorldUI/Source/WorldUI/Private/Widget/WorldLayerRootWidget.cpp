#include "Widget/WorldLayerRootWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Widgets/Layout/SConstraintCanvas.h"

namespace
{
	/* UCanvasPanelSlot이 내부 float로 무손실 표현 가능한 최대 정수 상한 (2^24 - 1) */
	constexpr int32 WorldUIMaxExactFloatZOrder = 16777215;

	/* 실서비스 기본 레이어 ZOrder 자동 정규화 임계치 */
	constexpr int32 WorldUIDefaultMaxLayerZOrderThreshold = 1000000;
}

UWorldLayerRootWidget::UWorldLayerRootWidget(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	RootCanvas(nullptr),
	MaxLayerZOrderThreshold(WorldUIDefaultMaxLayerZOrderThreshold)
{
}

TSharedRef<SWidget> UWorldLayerRootWidget::RebuildWidget()
{
	InitializeLayerStructure();
	return Super::RebuildWidget();
}

void UWorldLayerRootWidget::InitializeLayerStructure()
{
	if (RootCanvas != nullptr)
	{
		return;
	}

	// WidgetTree가 아직 생성되지 않은 특수 생성 경로를 대비해 방어 생성함
	if (WidgetTree == nullptr)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootLayerCanvas"));
	if (RootCanvas == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UWorldLayerRootWidget::InitializeLayerStructure] Failed to construct RootLayerCanvas."));
		return;
	}

	WidgetTree->RootWidget = RootCanvas;

	UEnum* ZOrderEnum = StaticEnum<EWorldWidgetZOrder>();
	if (ZOrderEnum == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[UWorldLayerRootWidget::InitializeLayerStructure] Failed to find EWorldWidgetZOrder UEnum."));
		return;
	}

	// UENUM 마지막 항목인 _MAX는 제외하고 순차적으로 레이어별 UCanvasPanel 및 초기 상태를 생성함
	// (UCanvasPanel 기본 가시성은 생성자에서 SelfHitTestInvisible로 초기화됨)
	LayerStates.Empty();
	const int32 EnumCount = ZOrderEnum->NumEnums() - 1;
	for (int32 EnumIndex = 0; EnumIndex < EnumCount; ++EnumIndex)
	{
		const int64 RawEnumValue = ZOrderEnum->GetValueByIndex(EnumIndex);
		const EWorldWidgetZOrder LayerEnum = static_cast<EWorldWidgetZOrder>(RawEnumValue);
		const FName LayerCanvasName(*FString::Printf(TEXT("LayerCanvas_%s"), *ZOrderEnum->GetNameStringByIndex(EnumIndex)));

		UCanvasPanel* LayerCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), LayerCanvasName);
		if (LayerCanvas == nullptr)
		{
			continue;
		}

		UCanvasPanelSlot* LayerSlot = RootCanvas->AddChildToCanvas(LayerCanvas);
		ConfigureFullScreenCanvasSlot(LayerSlot, static_cast<int32>(RawEnumValue));

		FWorldUILayerState NewState;
		NewState.LayerCanvas = LayerCanvas;
		NewState.NextZOrder = 0;
		LayerStates.Add(LayerEnum, MoveTemp(NewState));
	}
}

bool UWorldLayerRootWidget::AddWidgetToLayer(UUserWidget* InWidget, EWorldWidgetZOrder InLayer)
{
	if (InWidget == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldLayerRootWidget::AddWidgetToLayer] InWidget is nullptr."));
		return false;
	}

	InitializeLayerStructure();

	// AddToViewport와 동일하게 이미 뷰포트 또는 레이어 위에 존재하면 중복 추가 및 타 레이어 이동을 거부함
	if (InWidget->IsInViewport() == true || IsWidgetInAnyLayer(InWidget) == true)
	{
		return false;
	}

	FWorldUILayerState* LayerState = LayerStates.Find(InLayer);
	if (LayerState == nullptr || LayerState->LayerCanvas == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldLayerRootWidget::AddWidgetToLayer] Target layer canvas was not found."));
		return false;
	}

	const int32 AssignedZOrder = AllocateNextLayerZOrder(InLayer, *LayerState);

	UCanvasPanelSlot* NewSlot = LayerState->LayerCanvas->AddChildToCanvas(InWidget);
	if (NewSlot == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldLayerRootWidget::AddWidgetToLayer] AddChildToCanvas returned nullptr."));
		return false;
	}

	ConfigureFullScreenCanvasSlot(NewSlot, AssignedZOrder);

	return true;
}

bool UWorldLayerRootWidget::BringWidgetToLayerTop(UUserWidget* InWidget)
{
	if (InWidget == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UWorldLayerRootWidget::BringWidgetToLayerTop] InWidget is nullptr."));
		return false;
	}

	EWorldWidgetZOrder FoundLayer = EWorldWidgetZOrder::Base;
	if (TryGetWidgetLayer(InWidget, FoundLayer) == false)
	{
		return false;
	}

	FWorldUILayerState* LayerState = LayerStates.Find(FoundLayer);
	UCanvasPanelSlot* TargetSlot = Cast<UCanvasPanelSlot>(InWidget->Slot);
	if (LayerState == nullptr || LayerState->LayerCanvas == nullptr || TargetSlot == nullptr)
	{
		return false;
	}

	// 이미 해당 레이어에서 다른 모든 형제 위젯보다 엄밀히 최상단이면 카운터를 불필요하게 증가시키지 않음
	const int32 TargetZOrder = TargetSlot->GetZOrder();
	bool bIsAlreadyStrictlyTop = true;
	for (const UPanelSlot* CurSlot : LayerState->LayerCanvas->GetSlots())
	{
		const UCanvasPanelSlot* CurCanvasSlot = Cast<const UCanvasPanelSlot>(CurSlot);
		if (CurCanvasSlot == nullptr || CurCanvasSlot == TargetSlot)
		{
			continue;
		}

		if (CurCanvasSlot->GetZOrder() >= TargetZOrder)
		{
			bIsAlreadyStrictlyTop = false;
			break;
		}
	}

	if (bIsAlreadyStrictlyTop == true)
	{
		return true;
	}

	const int32 PromotedZOrder = AllocateNextLayerZOrder(FoundLayer, *LayerState);
	TargetSlot->SetZOrder(PromotedZOrder);

	return true;
}

bool UWorldLayerRootWidget::IsWidgetInAnyLayer(const UWidget* InWidget) const
{
	EWorldWidgetZOrder UnusedLayer = EWorldWidgetZOrder::Base;
	return TryGetWidgetLayer(InWidget, UnusedLayer);
}

bool UWorldLayerRootWidget::TryGetWidgetLayer(const UWidget* InWidget, EWorldWidgetZOrder& OutLayer) const
{
	if (InWidget == nullptr)
	{
		return false;
	}

	const UPanelWidget* ParentPanel = InWidget->GetParent();
	if (ParentPanel == nullptr)
	{
		return false;
	}

	for (const TPair<EWorldWidgetZOrder, FWorldUILayerState>& Pair : LayerStates)
	{
		if (Pair.Value.LayerCanvas.Get() == ParentPanel)
		{
			OutLayer = Pair.Key;
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
void UWorldLayerRootWidget::SetLayerZOrderMaxThresholdForTesting(int32 InThreshold)
{
	MaxLayerZOrderThreshold = FMath::Clamp(InThreshold, 1, WorldUIMaxExactFloatZOrder);
}
#endif

int32 UWorldLayerRootWidget::AllocateNextLayerZOrder(EWorldWidgetZOrder InLayer, FWorldUILayerState& InOutLayerState)
{
	if (InOutLayerState.NextZOrder >= MaxLayerZOrderThreshold)
	{
		NormalizeLayerZOrders(InLayer);
	}

	const int32 AllocatedZOrder = FMath::Min(InOutLayerState.NextZOrder, WorldUIMaxExactFloatZOrder);
	InOutLayerState.NextZOrder = AllocatedZOrder + 1;
	return AllocatedZOrder;
}

void UWorldLayerRootWidget::NormalizeLayerZOrders(EWorldWidgetZOrder InLayer)
{
	FWorldUILayerState* LayerState = LayerStates.Find(InLayer);
	if (LayerState == nullptr || LayerState->LayerCanvas == nullptr)
	{
		return;
	}

	struct FSlotZOrderEntry
	{
		UCanvasPanelSlot* CanvasSlot = nullptr;
		int32 ZOrder = 0;
		int32 OriginalIndex = 0;
	};

	const TArray<UPanelSlot*>& Slots = LayerState->LayerCanvas->GetSlots();
	TArray<FSlotZOrderEntry> SortedEntries;
	SortedEntries.Reserve(Slots.Num());

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slots[SlotIndex]))
		{
			SortedEntries.Add({ CanvasSlot, CanvasSlot->GetZOrder(), SlotIndex });
		}
	}

	// SetZOrder는 UPanelWidget::Slots 순서를 변경하지 않으므로 현재 ZOrder 기준 오름차순으로 안정 정렬함
	SortedEntries.Sort([](const FSlotZOrderEntry& Left, const FSlotZOrderEntry& Right)
	{
		if (Left.ZOrder != Right.ZOrder)
		{
			return Left.ZOrder < Right.ZOrder;
		}
		return Left.OriginalIndex < Right.OriginalIndex;
	});

	for (int32 NormalizedZOrder = 0; NormalizedZOrder < SortedEntries.Num(); ++NormalizedZOrder)
	{
		SortedEntries[NormalizedZOrder].CanvasSlot->SetZOrder(NormalizedZOrder);
	}

	LayerState->NextZOrder = SortedEntries.Num();

	// 순서 이동 없이 수치만 압축된 경우에도 Slate 레이어 캐시와 일관성을 유지하도록 ChildOrder를 무효화함
	if (TSharedPtr<SConstraintCanvas> NativeCanvas = LayerState->LayerCanvas->GetCanvasWidget())
	{
		NativeCanvas->Invalidate(EInvalidateWidgetReason::ChildOrder);
	}
}

void UWorldLayerRootWidget::ConfigureFullScreenCanvasSlot(UCanvasPanelSlot* InSlot, int32 InZOrder)
{
	if (InSlot == nullptr)
	{
		return;
	}

	// AddToViewport 기본값(FGameViewportWidgetSlot)과 동일하게 전체 화면 스트레치로 배치함
	InSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	InSlot->SetOffsets(FMargin(0.0f));
	InSlot->SetAlignment(FVector2D(0.0f, 0.0f));
	InSlot->SetAutoSize(false);
	InSlot->SetZOrder(InZOrder);
}