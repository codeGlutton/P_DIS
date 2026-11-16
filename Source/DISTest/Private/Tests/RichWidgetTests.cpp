#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Components/RichHorizontalBox.h"
#include "Components/RichListView.h"
#include "Components/RichScrollBar.h"
#include "Components/RichSlider.h"
#include "Components/RichVerticalBox.h"
#include "Components/Spacer.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Input/HittestGrid.h"
#include "Layout/ArrangedChildren.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Types/PaintArgs.h"
#include "Types/RichScrollBarTypes.h"
#include "Types/RichSliderTypes.h"
#include "UI/Widget/BBMCreditEntryWidget.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SRichHorizontalBox.h"
#include "Widgets/SRichListView.h"
#include "Widgets/SRichScrollBar.h"
#include "Widgets/SRichScrollBarBridge.h"
#include "Widgets/SRichScrollTrack.h"
#include "Widgets/SRichSlider.h"
#include "Widgets/SRichVerticalBox.h"
#include "Widgets/SWindow.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FExtensionWidgetTestWorldScope
	{
		UWorld* World = nullptr;
		UGameInstance* GameInstance = nullptr;

		FExtensionWidgetTestWorldScope()
		{
			GameInstance = NewObject<UGameInstance>(GEngine);
			if (GameInstance != nullptr)
			{
				GameInstance->InitializeStandalone();
				World = GameInstance->GetWorld();
				if (World != nullptr)
				{
					World->InitializeActorsForPlay(FURL());
				}
			}
		}

		~FExtensionWidgetTestWorldScope()
		{
			if (GameInstance != nullptr)
			{
				GameInstance->Shutdown();
				GameInstance = nullptr;
			}

			if (World != nullptr)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
				World = nullptr;
			}
		}
	};

	UClass* CreateTransientEntryWidgetBlueprintClass()
	{
		UBlueprint* OwnerBlueprint = NewObject<UBlueprint>(GetTransientPackage(), NAME_None, RF_Transient);
		UWidgetBlueprintGeneratedClass* GeneratedClass = NewObject<UWidgetBlueprintGeneratedClass>(
			GetTransientPackage(),
			MakeUniqueObjectName(GetTransientPackage(), UWidgetBlueprintGeneratedClass::StaticClass(), TEXT("TransientRichEntryWidget_C")),
			RF_Public | RF_Transient
		);
		if (GeneratedClass == nullptr)
		{
			return nullptr;
		}

		GeneratedClass->ClassGeneratedBy = OwnerBlueprint;
		GeneratedClass->SetSuperStruct(UBBMCreditEntryWidget::StaticClass());
		GeneratedClass->ClassFlags |= CLASS_CompiledFromBlueprint;
		GeneratedClass->Bind();
		GeneratedClass->StaticLink(true);
		GeneratedClass->AssembleReferenceTokenStream();
		GeneratedClass->GetDefaultObject();
		return GeneratedClass;
	}
}

/**
 * RichScrollBar Dynamic / Fixed 썸 크기 및 드래그 감도 역산 수학 검증 테스트
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRichScrollBarThumbModeTest,
	"DIS.ExtensionWidget.RichScrollBar.ThumbModeAndSensitivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FRichScrollBarThumbModeTest::RunTest(const FString& Parameters)
{
	FRichScrollBarConfig DynamicConfig;
	DynamicConfig.ThumbMode = ERichScrollBarThumbMode::Dynamic;
	DynamicConfig.MinDynamicThumbLength = 10.0f;

	TSharedRef<SRichScrollTrack> Track = SNew(SRichScrollTrack)
		.Orientation(Orient_Vertical)
		.Config(DynamicConfig);

	const float TrackLengthPx = 200.0f;

	// 1. Dynamic 모드: 콘텐츠 비율 변화에 따라 썸 비율도 함께 변함
	Track->SetSizes(0.0f, 0.5f);
	TestEqual(TEXT("Dynamic thumb fraction at 0.5"), Track->GetVisualThumbFraction(TrackLengthPx), 0.5f, 0.001f);

	Track->SetSizes(0.0f, 0.2f);
	TestEqual(TEXT("Dynamic thumb fraction at 0.2"), Track->GetVisualThumbFraction(TrackLengthPx), 0.2f, 0.001f);

	// 2. Fixed 모드: 콘텐츠가 길어져 ThumbSizeFraction이 줄어도 시각적 썸 길이는 40px(0.2)로 고정됨
	FRichScrollBarConfig FixedConfig;
	FixedConfig.ThumbMode = ERichScrollBarThumbMode::Fixed;
	FixedConfig.FixedThumbLength = 40.0f;
	Track->SetConfig(FixedConfig);

	Track->SetSizes(0.0f, 0.5f);
	TestEqual(TEXT("Fixed thumb fraction stays 40/200 at 0.5 content"), Track->GetVisualThumbFraction(TrackLengthPx), 0.2f, 0.001f);

	Track->SetSizes(0.0f, 0.1f);
	TestEqual(TEXT("Fixed thumb fraction stays 40/200 at 0.1 content"), Track->GetVisualThumbFraction(TrackLengthPx), 0.2f, 0.001f);

	Track->SetSizes(0.0f, 0.02f);
	TestEqual(TEXT("Fixed thumb fraction stays 40/200 at 0.02 content"), Track->GetVisualThumbFraction(TrackLengthPx), 0.2f, 0.001f);

	// 3. Fixed 모드 드래그 역산: 끝단 이동 시 ActualOffsetFraction이 정확히 1 - ThumbSizeFraction(0.98)에 도달함
	const float TopDragOffset = Track->ConvertTrackDragToOffsetFraction(20.0f, 0.5f, TrackLengthPx);
	const float MidDragOffset = Track->ConvertTrackDragToOffsetFraction(100.0f, 0.5f, TrackLengthPx);
	const float EndDragOffset = Track->ConvertTrackDragToOffsetFraction(180.0f, 0.5f, TrackLengthPx);

	TestEqual(TEXT("Fixed drag start offset"), TopDragOffset, 0.0f, 0.001f);
	TestEqual(TEXT("Fixed drag mid offset"), MidDragOffset, 0.49f, 0.001f);
	TestEqual(TEXT("Fixed drag end offset"), EndDragOffset, 0.98f, 0.001f);

	return true;
}

/**
 * RichScrollBar 일체형 가시성 및 화살표 홀드 중 Collapsed 지연 보호 테스트
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRichScrollBarVisibilityAndArrowHoldTest,
	"DIS.ExtensionWidget.RichScrollBar.VisibilityAndArrowHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FRichScrollBarVisibilityAndArrowHoldTest::RunTest(const FString& Parameters)
{
	FRichScrollBarConfig Config;
	Config.ThumbMode = ERichScrollBarThumbMode::Fixed;
	Config.FixedThumbLength = 32.0f;
	Config.ArrowConfig.bShowArrowButtons = true;
	Config.ArrowConfig.ClickScrollStep = 50.0f;
	Config.ArrowConfig.StandaloneReferenceExtent = 500.0f;
	Config.ArrowConfig.bEnableHoldScroll = true;

	float LastUserScrolledOffset = -1.0f;
	int32 UserScrolledCount = 0;

	TSharedRef<SRichScrollBar> ScrollBar = SNew(SRichScrollBar)
		.Orientation(Orient_Vertical)
		.AlwaysShowScrollbar(false)
		.Config(Config)
		.OnUserScrolled_Lambda([&](float InOffset)
		{
			LastUserScrolledOffset = InOffset;
			UserScrolledCount += 1;
		});

	// 1. 스크롤 불필요 + 비활성 시 루트 전체가 Collapsed 됨
	ScrollBar->SetState(0.0f, 1.0f, false);
	TestEqual(TEXT("Unneeded scrollbar collapses when AlwaysShowScrollbar is false"), ScrollBar->GetVisibility(), EVisibility::Collapsed);
	TestFalse(TEXT("IsNeeded false when thumb is 1.0"), ScrollBar->IsNeeded());

	// 2. 스크롤 필요 시 Visible 로 전환되고 화살표 클릭 스텝이 정규화 비율(+0.1)만큼 이동함
	ScrollBar->SetState(0.0f, 0.5f, false);
	TestEqual(TEXT("Needed scrollbar becomes Visible"), ScrollBar->GetVisibility(), EVisibility::Visible);
	TestTrue(TEXT("IsNeeded true when thumb is 0.5"), ScrollBar->IsNeeded());

	ScrollBar->StepArrowScroll(1, 50.0f);
	TestEqual(TEXT("Single forward arrow step updates offset"), ScrollBar->GetOffsetFraction(), 0.1f, 0.001f);
	TestEqual(TEXT("OnUserScrolled called on arrow step"), UserScrolledCount, 1);
	TestEqual(TEXT("OnUserScrolled offset value"), LastUserScrolledOffset, 0.1f, 0.001f);

	// 3. 화살표 버튼 홀드 중에는 스크롤이 끝까지 도달하거나 불필요 상태가 되어도 떼기 전까지 Collapsed 되지 않음
	ScrollBar->BeginArrowHold(1);
	TestTrue(TEXT("IsHoldingArrow active after BeginArrowHold"), ScrollBar->IsHoldingArrow());
	ScrollBar->SetState(0.0f, 1.0f, false);
	TestEqual(TEXT("Holding arrow prevents auto collapse before mouse release"), ScrollBar->GetVisibility(), EVisibility::Visible);

	ScrollBar->EndArrowHold();
	TestFalse(TEXT("IsHoldingArrow cleared after EndArrowHold"), ScrollBar->IsHoldingArrow());
	TestEqual(TEXT("Collapses cleanly once arrow hold is released while unneeded"), ScrollBar->GetVisibility(), EVisibility::Collapsed);

	// 4. AlwaysShowScrollbar=true 이면 불필요 상태에서도 Visible 유지
	ScrollBar->SetAlwaysShowScrollbar(true);
	TestEqual(TEXT("AlwaysShowScrollbar keeps bar visible"), ScrollBar->GetVisibility(), EVisibility::Visible);

	// 5. 화살표 패딩이 반대편 화살표에서 축 기준으로 자동 반전되는지 검증
	Config.ArrowConfig.ArrowPadding = FMargin(1.0f, 2.0f, 3.0f, 6.0f);
	ScrollBar->SetConfig(Config);
	TestEqual(TEXT("Vertical Start arrow padding preserves authored margin"),
		ScrollBar->GetStartArrowSlotPadding(),
		FMargin(1.0f, 2.0f, 3.0f, 6.0f));
	TestEqual(TEXT("Vertical End arrow padding flips Top and Bottom"),
		ScrollBar->GetEndArrowSlotPadding(),
		FMargin(1.0f, 6.0f, 3.0f, 2.0f));

	ScrollBar->SetOrientation(Orient_Horizontal);
	TestEqual(TEXT("Horizontal End arrow padding flips Left and Right"),
		ScrollBar->GetEndArrowSlotPadding(),
		FMargin(3.0f, 2.0f, 1.0f, 6.0f));

	return true;
}

/**
 * UMG URichScrollBar 및 URichListView 위젯 구축·프로퍼티 동기화·리소스 해제 검증 테스트
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRichListViewUMGParityTest,
	"DIS.ExtensionWidget.RichListView.UMGParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FRichListViewUMGParityTest::RunTest(const FString& Parameters)
{
	FExtensionWidgetTestWorldScope ScopedWorld;
	TestNotNull(TEXT("Test world initialized"), ScopedWorld.World);

	UUserWidget* HostWidget = CreateWidget<UBBMCreditEntryWidget>(ScopedWorld.World);
	TestNotNull(TEXT("Host UBBMCreditEntryWidget created"), HostWidget);
	if (HostWidget != nullptr && HostWidget->WidgetTree == nullptr)
	{
		HostWidget->WidgetTree = NewObject<UWidgetTree>(HostWidget);
	}

	URichScrollBar* RichScrollBarWidget = (HostWidget != nullptr && HostWidget->WidgetTree != nullptr)
		? HostWidget->WidgetTree->ConstructWidget<URichScrollBar>(URichScrollBar::StaticClass())
		: NewObject<URichScrollBar>(ScopedWorld.World);
	TestNotNull(TEXT("URichScrollBar created"), RichScrollBarWidget);

	FRichScrollBarConfig Config;
	Config.ThumbMode = ERichScrollBarThumbMode::Fixed;
	Config.FixedThumbLength = 44.0f;
	Config.ArrowConfig.bShowArrowButtons = true;
	Config.ArrowConfig.ClickScrollStep = 24.0f;
	RichScrollBarWidget->SetRichConfig(Config);
	RichScrollBarWidget->SetAlwaysShowScrollbar(false);

	const TSharedRef<SWidget> ScrollBarSlate = RichScrollBarWidget->TakeWidget();
	TestTrue(TEXT("URichScrollBar built Slate widget"), RichScrollBarWidget->GetSlateRichScrollBar().IsValid());

	RichScrollBarWidget->SetState(0.2f, 0.4f);
	TestEqual(TEXT("URichScrollBar offset synchronized"), RichScrollBarWidget->GetScrollOffsetFraction(), 0.2f, 0.001f);
	TestEqual(TEXT("URichScrollBar thumb fraction synchronized"), RichScrollBarWidget->GetThumbSizeFraction(), 0.4f, 0.001f);
	TestTrue(TEXT("URichScrollBar IsScrollNeeded true"), RichScrollBarWidget->IsScrollNeeded());

	URichListView* RichListView = (HostWidget != nullptr && HostWidget->WidgetTree != nullptr)
		? HostWidget->WidgetTree->ConstructWidget<URichListView>(URichListView::StaticClass())
		: NewObject<URichListView>(ScopedWorld.World);
	TestNotNull(TEXT("URichListView created"), RichListView);

	UClass* TransientEntryClass = CreateTransientEntryWidgetBlueprintClass();
	TestNotNull(TEXT("Transient EntryWidget Blueprint class created"), TransientEntryClass);
	RichListView->SetEntryWidgetClass(TransientEntryClass);
	RichListView->SetRichScrollBarConfig(Config);
	RichListView->SetAlwaysShowRichScrollbar(false);
	RichListView->SetScrollBarPadding(FMargin(6.0f, 4.0f, 8.0f, 5.0f));

	FScrollBarStyle CustomStyle = FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("Scrollbar");
	CustomStyle.Thickness = 24.0f;
	RichListView->SetRichScrollBarStyle(CustomStyle);

	const TSharedRef<SWidget> ListViewSlate = RichListView->TakeWidget();
	const TSharedPtr<SRichScrollBar> EmbeddedRichScrollBar = RichListView->GetRichScrollBar();
	TestTrue(TEXT("URichListView constructed internal SRichScrollBar"), EmbeddedRichScrollBar.IsValid());

	if (EmbeddedRichScrollBar.IsValid() == true)
	{
		TestEqual(TEXT("URichListView RichScrollBar starts Collapsed when empty and AlwaysShow=false"),
			EmbeddedRichScrollBar->GetVisibility(),
			EVisibility::Collapsed);

		RichListView->SetAlwaysShowRichScrollbar(true);
		TestEqual(TEXT("URichListView RichScrollBar becomes Visible when AlwaysShow=true"),
			EmbeddedRichScrollBar->GetVisibility(),
			EVisibility::Visible);
		TestNotEqual(TEXT("URichListView AlwaysShow=true keeps thumb brush drawable even when unneeded"),
			EmbeddedRichScrollBar->GetThumbBrush()->DrawAs.GetValue(),
			ESlateBrushDrawType::NoDrawType);

		// 실제 Slate 레이아웃 패스(400x300) 수행 시 RichScrollBar가 비영(non-zero) 영역으로 배치되는지 검증
		ListViewSlate->SlatePrepass(1.0f);
		const FGeometry RootGeometry = FGeometry::MakeRoot(FVector2D(400.0f, 300.0f), FSlateLayoutTransform());
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		ListViewSlate->ArrangeChildren(RootGeometry, ArrangedChildren);

		FVector2D ArrangedScrollBarSize = FVector2D::ZeroVector;
		for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
		{
			const TSharedRef<SWidget>& ChildWidget = ArrangedChildren[ChildIndex].Widget;
			if (ChildWidget == EmbeddedRichScrollBar.ToSharedRef())
			{
				ArrangedScrollBarSize = ArrangedChildren[ChildIndex].Geometry.GetLocalSize();
				break;
			}
			// ChildSlot 래퍼(SHorizontalBox) 내부까지 1단계 재귀 확인
			FArrangedChildren GrandChildren(EVisibility::Visible);
			ChildWidget->ArrangeChildren(ArrangedChildren[ChildIndex].Geometry, GrandChildren);
			for (int32 GrandIndex = 0; GrandIndex < GrandChildren.Num(); ++GrandIndex)
			{
				if (GrandChildren[GrandIndex].Widget == EmbeddedRichScrollBar.ToSharedRef())
				{
					ArrangedScrollBarSize = GrandChildren[GrandIndex].Geometry.GetLocalSize();
					break;
				}
			}
		}

		TestEqual(TEXT("URichListView ScrollBarStyle.Thickness controls arranged scrollbar width"),
			static_cast<float>(ArrangedScrollBarSize.X),
			24.0f,
			0.5f);
		TestEqual(TEXT("URichListView ScrollBarPadding Top/Bottom insets arranged scrollbar height"),
			static_cast<float>(ArrangedScrollBarSize.Y),
			291.0f,
			0.5f);
	}

	// Slate SRichListView + SRichScrollBarBridge 양방향 상태 전달 검증
	TArray<TSharedPtr<FString>> DummyItems;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		DummyItems.Add(MakeShared<FString>(FString::Printf(TEXT("Item_%d"), Index)));
	}

	TSharedRef<SRichListView<TSharedPtr<FString>>> SlateRichListView =
		SNew(SRichListView<TSharedPtr<FString>>)
		.ListItemsSource(&DummyItems);
	SlateRichListView->ConfigureRichScrollBar(
		nullptr,
		Orient_Vertical,
		false,
		FMargin(0.0f),
		Config,
		ERichScrollBarPlacement::End
	);

	const TSharedPtr<SRichScrollBarBridge> BridgeScrollBar = SlateRichListView->GetBridgeScrollBar();
	const TSharedPtr<SRichScrollBar> SlateRichScrollBar = SlateRichListView->GetRichScrollBar();
	TestTrue(TEXT("Slate SRichListView created BridgeScrollBar"), BridgeScrollBar.IsValid());
	TestTrue(TEXT("Slate SRichListView created RichScrollBar"), SlateRichScrollBar.IsValid());

	if (BridgeScrollBar.IsValid() == true && SlateRichScrollBar.IsValid() == true)
	{
		BridgeScrollBar->SetState(0.25f, 0.35f, false);
		TestEqual(TEXT("Bridge SetState forwards OffsetFraction to RichScrollBar"), SlateRichScrollBar->GetOffsetFraction(), 0.25f, 0.001f);
		TestEqual(TEXT("Bridge SetState forwards ThumbSizeFraction to RichScrollBar"), SlateRichScrollBar->GetThumbSizeFraction(), 0.35f, 0.001f);
		TestEqual(TEXT("Bridge SetState makes RichScrollBar Visible when needed"), SlateRichScrollBar->GetVisibility(), EVisibility::Visible);
	}

	RichListView->ReleaseSlateResources(true);
	TestFalse(TEXT("URichListView releases SRichScrollBar cleanly"), RichListView->GetRichScrollBar().IsValid());

	RichScrollBarWidget->ReleaseSlateResources(true);
	TestFalse(TEXT("URichScrollBar releases Slate widget cleanly"), RichScrollBarWidget->GetSlateRichScrollBar().IsValid());

	return true;
}

namespace
{
	struct FReverseBoxPaintRecord
	{
		int32 ChildId = INDEX_NONE;
		int32 InputLayerId = INDEX_NONE;
	};

	/**
	 * 다중 레이어(LayerId + 1)를 소비하며 OnPaint 호출 순서와 입력 LayerId를 기록하는 테스트용 리프 위젯
	 */
	class SReversePaintProbeWidget : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SReversePaintProbeWidget)
			: _ChildId(0)
			, _DesiredSize(FVector2D(100.0f, 40.0f))
			, _Records(nullptr)
		{
		}
			SLATE_ARGUMENT(int32, ChildId)
			SLATE_ARGUMENT(FVector2D, DesiredSize)
			SLATE_ARGUMENT(TArray<FReverseBoxPaintRecord>*, Records)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ChildId = InArgs._ChildId;
			ProbeDesiredSize = InArgs._DesiredSize;
			Records = InArgs._Records;
			SetVisibility(EVisibility::Visible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return ProbeDesiredSize;
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			bool bParentEnabled
		) const override
		{
			if (Records != nullptr)
			{
				Records->Add(FReverseBoxPaintRecord{ ChildId, LayerId });
			}

			// 복합 위젯(예: 버튼 배경 + 텍스트)처럼 2개 레이어를 사용하는 경우를 모사함
			return LayerId + 1;
		}

	private:
		int32 ChildId = 0;
		FVector2D ProbeDesiredSize = FVector2D::ZeroVector;
		TArray<FReverseBoxPaintRecord>* Records = nullptr;
	};
}

/**
 * SRichVerticalBox / SRichHorizontalBox 및 URichVerticalBox / URichHorizontalBox의
 * 레이아웃 불변성, 역순 OnPaint, 자식별 LayerId 증가, FHittestGrid Hit 우선순위 역전 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRichBoxReversePaintAndHitTest,
	"DIS.ExtensionWidget.RichBox.ReversePaintAndHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FRichBoxReversePaintAndHitTest::RunTest(const FString& Parameters)
{
	FExtensionWidgetTestWorldScope WorldScope;
	TestNotNull(TEXT("Test world must exist for URichVerticalBox/URichHorizontalBox"), WorldScope.World);
	if (WorldScope.World == nullptr)
	{
		return false;
	}

	const FGeometry RootGeometry = FGeometry::MakeRoot(FVector2D(400.0f, 300.0f), FSlateLayoutTransform());
	const FSlateRect CullingRect(FVector2f::ZeroVector, FVector2f(400.0f, 300.0f));

	// 1. SRichVerticalBox: 음수 상단 패딩으로 겹치는 3개 자식 배치 후 정순/역순 검증
	{
		TArray<FReverseBoxPaintRecord> PaintRecords;
		TArray<TSharedRef<SReversePaintProbeWidget>> Probes;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Probes.Add(
				SNew(SReversePaintProbeWidget)
				.ChildId(Index)
				.DesiredSize(FVector2D(100.0f, 40.0f))
				.Records(&PaintRecords)
			);
		}

		TSharedRef<SRichVerticalBox> RichVerticalBox = SNew(SRichVerticalBox)
			.bReversePaint(false);
		RichVerticalBox->AddSlot().AutoHeight()[Probes[0]];
		RichVerticalBox->AddSlot().AutoHeight().Padding(0.0f, -20.0f, 0.0f, 0.0f)[Probes[1]];
		RichVerticalBox->AddSlot().AutoHeight().Padding(0.0f, -20.0f, 0.0f, 0.0f)[Probes[2]];

		TSharedRef<SWindow> TestWindow = SNew(SWindow)
			.ClientSize(FVector2D(400.0f, 300.0f))
			.AutoCenter(EAutoCenter::None);
		TestWindow->SetContent(RichVerticalBox);
		TestWindow->SlatePrepass(1.0f);

		// 정순 상태 ArrangeChildren 좌표 확인 (Y: 0, 20, 40)
		FArrangedChildren ArrangedNormal(EVisibility::Visible);
		RichVerticalBox->ArrangeChildren(RootGeometry, ArrangedNormal);
		TestEqual(TEXT("Normal SRichVerticalBox arranges 3 children"), ArrangedNormal.Num(), 3);
		if (ArrangedNormal.Num() == 3)
		{
			TestEqual(TEXT("Normal Child 0 stays at slot index 0"), ArrangedNormal[0].Widget, StaticCastSharedRef<SWidget>(Probes[0]));
			TestEqual(TEXT("Normal Child 0 Y position is 0"), static_cast<float>(ArrangedNormal[0].Geometry.GetAbsolutePosition().Y), 0.0f, 0.5f);
			TestEqual(TEXT("Normal Child 1 Y position is 20"), static_cast<float>(ArrangedNormal[1].Geometry.GetAbsolutePosition().Y), 20.0f, 0.5f);
			TestEqual(TEXT("Normal Child 2 Y position is 40"), static_cast<float>(ArrangedNormal[2].Geometry.GetAbsolutePosition().Y), 40.0f, 0.5f);
		}

		if (FSlateApplicationBase::IsInitialized() == true && FSlateApplicationBase::Get().GetRenderer() != nullptr)
		{
			TSharedRef<FHittestGrid> HittestGrid = MakeShared<FHittestGrid>();
			HittestGrid->SetHittestArea(FVector2f::ZeroVector, FVector2f(400.0f, 300.0f), FVector2f::ZeroVector);
			FSlateWindowElementList DrawElements(TestWindow);
			FPaintArgs PaintArgs(nullptr, HittestGrid.Get(), HittestGrid.Get(), FVector2f::ZeroVector, 0.0, 0.0f);

			// 1-1. bReversePaint = false (정순 Paint + 동일 LayerId + 나중 슬롯 Hit)
			PaintRecords.Reset();
			TestWindow->Paint(PaintArgs, RootGeometry, CullingRect, DrawElements, 0, FWidgetStyle(), true);
			TestEqual(TEXT("Normal VerticalBox paints 3 probes"), PaintRecords.Num(), 3);
			if (PaintRecords.Num() == 3)
			{
				TestEqual(TEXT("Normal VerticalBox paint order[0] == 0"), PaintRecords[0].ChildId, 0);
				TestEqual(TEXT("Normal VerticalBox paint order[1] == 1"), PaintRecords[1].ChildId, 1);
				TestEqual(TEXT("Normal VerticalBox paint order[2] == 2"), PaintRecords[2].ChildId, 2);
				TestEqual(TEXT("Normal VerticalBox shares same input LayerId across children"), PaintRecords[1].InputLayerId, PaintRecords[0].InputLayerId);
				TestEqual(TEXT("Normal VerticalBox shares same input LayerId[2]"), PaintRecords[2].InputLayerId, PaintRecords[0].InputLayerId);
			}

			// 실제 PaintSpaceGeometry 기준 Child 0과 Child 1 겹침 지점(타이틀바 오프셋 무관) 히트 검증
			const FVector2f NormalOverlapPoint = FVector2f(Probes[0]->GetPaintSpaceGeometry().GetAbsolutePosition()) + FVector2f(50.0f, 25.0f);
			const TArray<FWidgetAndPointer> NormalHitPath = HittestGrid->GetBubblePath(NormalOverlapPoint, 0.0f, false, 0);
			TestTrue(TEXT("Normal VerticalBox overlap point produces hit path"), NormalHitPath.Num() > 0);
			if (NormalHitPath.Num() > 0)
			{
				TestEqual(TEXT("Normal VerticalBox overlap hits Child 1 (drawn later)"),
					NormalHitPath.Last().Widget,
					StaticCastSharedRef<SWidget>(Probes[1]));
			}

			// 박스 단독 Paint 시 MaxLayerId 검증 (정순: 모든 자식 LayerId 0 공유 -> Max 1)
			PaintRecords.Reset();
			FSlateWindowElementList DirectNormalDrawElements(TestWindow);
			const int32 DirectNormalMaxLayerId = RichVerticalBox->OnPaint(PaintArgs, RootGeometry, CullingRect, DirectNormalDrawElements, 0, FWidgetStyle(), true);
			TestEqual(TEXT("Normal VerticalBox direct OnPaint MaxLayerId == 1"), DirectNormalMaxLayerId, 1);

			// 1-2. bReversePaint = true (레이아웃 불변 + 역순 Paint + 자식별 LayerId 증가 + 앞쪽 슬롯 Hit)
			RichVerticalBox->SetReversePaint(true);
			TestTrue(TEXT("SetReversePaint(true) updates SRichVerticalBox getter"), RichVerticalBox->GetReversePaint());

			FArrangedChildren ArrangedAfterReverse(EVisibility::Visible);
			RichVerticalBox->ArrangeChildren(RootGeometry, ArrangedAfterReverse);
			TestEqual(TEXT("ReversePaint preserves ArrangeChildren count"), ArrangedAfterReverse.Num(), 3);
			if (ArrangedAfterReverse.Num() == 3)
			{
				TestEqual(TEXT("ReversePaint does NOT alter ArrangeChildren order[0]"), ArrangedAfterReverse[0].Widget, StaticCastSharedRef<SWidget>(Probes[0]));
				TestEqual(TEXT("ReversePaint keeps Child 0 Y position at 0"), static_cast<float>(ArrangedAfterReverse[0].Geometry.GetAbsolutePosition().Y), 0.0f, 0.5f);
				TestEqual(TEXT("ReversePaint keeps Child 1 Y position at 20"), static_cast<float>(ArrangedAfterReverse[1].Geometry.GetAbsolutePosition().Y), 20.0f, 0.5f);
				TestEqual(TEXT("ReversePaint keeps Child 2 Y position at 40"), static_cast<float>(ArrangedAfterReverse[2].Geometry.GetAbsolutePosition().Y), 40.0f, 0.5f);
			}

			PaintRecords.Reset();
			HittestGrid->Clear();
			FSlateWindowElementList ReverseDrawElements(TestWindow);
			TestWindow->Paint(PaintArgs, RootGeometry, CullingRect, ReverseDrawElements, 0, FWidgetStyle(), true);
			TestEqual(TEXT("Reversed VerticalBox paints 3 probes"), PaintRecords.Num(), 3);
			if (PaintRecords.Num() == 3)
			{
				TestEqual(TEXT("Reversed VerticalBox paint order[0] == 2"), PaintRecords[0].ChildId, 2);
				TestEqual(TEXT("Reversed VerticalBox paint order[1] == 1"), PaintRecords[1].ChildId, 1);
				TestEqual(TEXT("Reversed VerticalBox paint order[2] == 0"), PaintRecords[2].ChildId, 0);
				TestEqual(TEXT("Reversed VerticalBox increments LayerId after 2-layer Child 2"), PaintRecords[1].InputLayerId, PaintRecords[0].InputLayerId + 2);
				TestEqual(TEXT("Reversed VerticalBox increments LayerId after 2-layer Child 1"), PaintRecords[2].InputLayerId, PaintRecords[1].InputLayerId + 2);
			}

			const FVector2f ReversedOverlapPoint = FVector2f(Probes[0]->GetPaintSpaceGeometry().GetAbsolutePosition()) + FVector2f(50.0f, 25.0f);
			const TArray<FWidgetAndPointer> ReversedHitPath = HittestGrid->GetBubblePath(ReversedOverlapPoint, 0.0f, false, 0);
			TestTrue(TEXT("Reversed VerticalBox overlap point produces hit path"), ReversedHitPath.Num() > 0);
			if (ReversedHitPath.Num() > 0)
			{
				TestEqual(TEXT("Reversed VerticalBox overlap hits Child 0 (painted last with highest LayerId)"),
					ReversedHitPath.Last().Widget,
					StaticCastSharedRef<SWidget>(Probes[0]));
			}

			// 박스 단독 Paint 시 MaxLayerId 검증 (역순 3자식 각 2레이어: 0~1, 2~3, 4~5 -> Max 5)
			PaintRecords.Reset();
			FSlateWindowElementList DirectReverseDrawElements(TestWindow);
			const int32 DirectReverseMaxLayerId = RichVerticalBox->OnPaint(PaintArgs, RootGeometry, CullingRect, DirectReverseDrawElements, 0, FWidgetStyle(), true);
			TestEqual(TEXT("Reversed VerticalBox direct OnPaint MaxLayerId == 5"), DirectReverseMaxLayerId, 5);
		}
	}

	// 2. SRichHorizontalBox: 음수 좌측 패딩으로 겹치는 3개 자식 배치 후 역순 Paint + Hit 검증
	{
		TArray<FReverseBoxPaintRecord> PaintRecords;
		TArray<TSharedRef<SReversePaintProbeWidget>> Probes;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Probes.Add(
				SNew(SReversePaintProbeWidget)
				.ChildId(Index)
				.DesiredSize(FVector2D(40.0f, 100.0f))
				.Records(&PaintRecords)
			);
		}

		TSharedRef<SRichHorizontalBox> RichHorizontalBox = SNew(SRichHorizontalBox)
			.bReversePaint(true);
		RichHorizontalBox->AddSlot().AutoWidth()[Probes[0]];
		RichHorizontalBox->AddSlot().AutoWidth().Padding(-20.0f, 0.0f, 0.0f, 0.0f)[Probes[1]];
		RichHorizontalBox->AddSlot().AutoWidth().Padding(-20.0f, 0.0f, 0.0f, 0.0f)[Probes[2]];

		TSharedRef<SWindow> TestWindow = SNew(SWindow)
			.ClientSize(FVector2D(400.0f, 300.0f))
			.AutoCenter(EAutoCenter::None);
		TestWindow->SetContent(RichHorizontalBox);
		TestWindow->SlatePrepass(1.0f);

		FArrangedChildren ArrangedHorizontal(EVisibility::Visible);
		RichHorizontalBox->ArrangeChildren(RootGeometry, ArrangedHorizontal);
		TestEqual(TEXT("SRichHorizontalBox ArrangeChildren preserves left-to-right coordinates"), ArrangedHorizontal.Num(), 3);
		if (ArrangedHorizontal.Num() == 3)
		{
			TestEqual(TEXT("Horizontal Child 0 X position is 0"), static_cast<float>(ArrangedHorizontal[0].Geometry.GetAbsolutePosition().X), 0.0f, 0.5f);
			TestEqual(TEXT("Horizontal Child 1 X position is 20"), static_cast<float>(ArrangedHorizontal[1].Geometry.GetAbsolutePosition().X), 20.0f, 0.5f);
			TestEqual(TEXT("Horizontal Child 2 X position is 40"), static_cast<float>(ArrangedHorizontal[2].Geometry.GetAbsolutePosition().X), 40.0f, 0.5f);
		}

		if (FSlateApplicationBase::IsInitialized() == true && FSlateApplicationBase::Get().GetRenderer() != nullptr)
		{
			TSharedRef<FHittestGrid> HittestGrid = MakeShared<FHittestGrid>();
			HittestGrid->SetHittestArea(FVector2f::ZeroVector, FVector2f(400.0f, 300.0f), FVector2f::ZeroVector);
			FSlateWindowElementList DrawElements(TestWindow);
			FPaintArgs PaintArgs(nullptr, HittestGrid.Get(), HittestGrid.Get(), FVector2f::ZeroVector, 0.0, 0.0f);

			PaintRecords.Reset();
			TestWindow->Paint(PaintArgs, RootGeometry, CullingRect, DrawElements, 0, FWidgetStyle(), true);
			TestEqual(TEXT("Reversed HorizontalBox paints 3 probes"), PaintRecords.Num(), 3);
			if (PaintRecords.Num() == 3)
			{
				TestEqual(TEXT("Reversed HorizontalBox paint order[0] == 2"), PaintRecords[0].ChildId, 2);
				TestEqual(TEXT("Reversed HorizontalBox paint order[1] == 1"), PaintRecords[1].ChildId, 1);
				TestEqual(TEXT("Reversed HorizontalBox paint order[2] == 0"), PaintRecords[2].ChildId, 0);
				TestTrue(TEXT("Reversed HorizontalBox increases input LayerId strictly"),
					PaintRecords[0].InputLayerId < PaintRecords[1].InputLayerId && PaintRecords[1].InputLayerId < PaintRecords[2].InputLayerId);
			}

			// Child 0(X:0~40)과 Child 1(X:20~60)만 겹치는 상대 X=25 지점 히트 시 Child 0 우선 확인
			const FVector2f HorizontalOverlapPoint = FVector2f(Probes[0]->GetPaintSpaceGeometry().GetAbsolutePosition()) + FVector2f(25.0f, 50.0f);
			const TArray<FWidgetAndPointer> HorizontalHitPath = HittestGrid->GetBubblePath(HorizontalOverlapPoint, 0.0f, false, 0);
			TestTrue(TEXT("Reversed HorizontalBox overlap point produces hit path"), HorizontalHitPath.Num() > 0);
			if (HorizontalHitPath.Num() > 0)
			{
				TestEqual(TEXT("Reversed HorizontalBox overlap at X=25 hits Child 0"),
					HorizontalHitPath.Last().Widget,
					StaticCastSharedRef<SWidget>(Probes[0]));
			}
		}
	}

	// 3. UMG URichVerticalBox / URichHorizontalBox 래퍼 동기화 및 슬롯 호환성 검증
	{
		URichVerticalBox* UMGVerticalBox = NewObject<URichVerticalBox>(WorldScope.World);
		URichHorizontalBox* UMGHorizontalBox = NewObject<URichHorizontalBox>(WorldScope.World);
		TestNotNull(TEXT("URichVerticalBox created"), UMGVerticalBox);
		TestNotNull(TEXT("URichHorizontalBox created"), UMGHorizontalBox);
		if (UMGVerticalBox == nullptr || UMGHorizontalBox == nullptr)
		{
			return false;
		}

		USpacer* ChildA = NewObject<USpacer>(UMGVerticalBox);
		USpacer* ChildB = NewObject<USpacer>(UMGHorizontalBox);
		TestNotNull(TEXT("URichVerticalBox AddChildToVerticalBox returns UVerticalBoxSlot"), UMGVerticalBox->AddChildToVerticalBox(ChildA));
		TestNotNull(TEXT("URichHorizontalBox AddChildToHorizontalBox returns UHorizontalBoxSlot"), UMGHorizontalBox->AddChildToHorizontalBox(ChildB));

		UMGVerticalBox->bReversePaint = true;
		UMGHorizontalBox->bReversePaint = false;

		UMGVerticalBox->TakeWidget();
		UMGHorizontalBox->TakeWidget();
		UMGVerticalBox->SynchronizeProperties();
		UMGHorizontalBox->SynchronizeProperties();

		const TSharedPtr<SRichVerticalBox> SlateVBox = UMGVerticalBox->GetSlateRichVerticalBox();
		const TSharedPtr<SRichHorizontalBox> SlateHBox = UMGHorizontalBox->GetSlateRichHorizontalBox();
		TestTrue(TEXT("URichVerticalBox builds SRichVerticalBox"), SlateVBox.IsValid());
		TestTrue(TEXT("URichHorizontalBox builds SRichHorizontalBox"), SlateHBox.IsValid());
		if (SlateVBox.IsValid() == true && SlateHBox.IsValid() == true)
		{
			TestTrue(TEXT("URichVerticalBox SynchronizeProperties syncs bReversePaint=true"), SlateVBox->GetReversePaint());
			TestFalse(TEXT("URichHorizontalBox SynchronizeProperties syncs bReversePaint=false"), SlateHBox->GetReversePaint());

			UMGHorizontalBox->SetReversePaint(true);
			TestTrue(TEXT("URichHorizontalBox SetReversePaint(true) updates Slate widget"), SlateHBox->GetReversePaint());
		}

		static_cast<UWidget*>(UMGVerticalBox)->ReleaseSlateResources(true);
		UMGHorizontalBox->ReleaseSlateResources(true);
		TestFalse(TEXT("URichVerticalBox releases Slate widget cleanly"), UMGVerticalBox->GetSlateRichVerticalBox().IsValid());
		TestFalse(TEXT("URichHorizontalBox releases Slate widget cleanly"), UMGHorizontalBox->GetSlateRichHorizontalBox().IsValid());
	}

	return true;
}

/**
 * SRichSlider / URichSlider 채움 트랙 핸들 정중앙 클리핑·무폴백 상태 브러시·UMG 동기화 검증 테스트
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRichSliderWidgetTest,
	"DIS.ExtensionWidget.RichSlider",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FRichSliderWidgetTest::RunTest(const FString& Parameters)
{
	// 1. FRichSliderStyle 기본값 및 GetFillBarImage() 무폴백 상태 선택 검증
	FSliderStyle BaseStyle;
	BaseStyle.BarThickness = 14.0f;
	BaseStyle.NormalBarImage = FSlateColorBrush(FLinearColor::Black);
	BaseStyle.HoveredBarImage = FSlateColorBrush(FLinearColor::Gray);
	BaseStyle.DisabledBarImage = FSlateColorBrush(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
	BaseStyle.NormalThumbImage = FSlateColorBrush(FLinearColor::White);
	BaseStyle.NormalThumbImage.ImageSize = FVector2D(24.0f, 24.0f);
	BaseStyle.HoveredThumbImage = BaseStyle.NormalThumbImage;
	BaseStyle.DisabledThumbImage = BaseStyle.NormalThumbImage;

	FRichSliderStyle RichStyle;
	RichStyle.NormalFillBarImage = FSlateColorBrush(FLinearColor::Yellow);
	RichStyle.HoveredFillBarImage = FSlateColorBrush(FLinearColor::Green);
	RichStyle.DisabledFillBarImage = FSlateNoResource();

	TSharedRef<SRichSlider> RichSlider = SNew(SRichSlider)
		.Style(&BaseStyle)
		.RichStyle(&RichStyle)
		.IndentHandle(true)
		.Value(0.5f);

	TestTrue(TEXT("Normal FillBar returns NormalFillBarImage"), RichSlider->GetFillBarImage() == &RichStyle.NormalFillBarImage);

	RichSlider->SetLocked(true);
	TestTrue(TEXT("Locked FillBar returns DisabledFillBarImage directly without fallback"), RichSlider->GetFillBarImage() == &RichStyle.DisabledFillBarImage);

	// 2. OnPaint 3레이어 및 핸들 정중앙 클리핑(FSlateClippingZone) 검증
	const FGeometry HorizontalGeometry = FGeometry::MakeRoot(FVector2D(320.0f, 32.0f), FSlateLayoutTransform());
	TSharedRef<SWindow> DummyWindow = SNew(SWindow).ClientSize(FVector2D(320.0f, 320.0f));
	TSharedRef<FHittestGrid> HittestGrid = MakeShared<FHittestGrid>();
	HittestGrid->SetHittestArea(FVector2f::ZeroVector, FVector2f(320.0f, 320.0f), FVector2f::ZeroVector);
	const FPaintArgs PaintArgs(nullptr, HittestGrid.Get(), HittestGrid.Get(), FVector2f::ZeroVector, 0.0, 0.0f);
	const FSlateRect CullingRect(0.0f, 0.0f, 320.0f, 320.0f);

	// NoDrawType으로 설정된 DisabledFillBarImage는 폴백 없이 비표시 처리됨
	{
		FSlateWindowElementList DrawElements(DummyWindow);
		RichSlider->OnPaint(PaintArgs, HorizontalGeometry, CullingRect, DrawElements, 0, FWidgetStyle(), true);

		const auto& BoxElements = DrawElements.GetUncachedDrawElements().Get<static_cast<uint8>(EElementType::ET_Box)>();
		TestEqual(TEXT("Disabled NoDrawType FillBar omits fill rendering without fallback (2 boxes)"), BoxElements.Num(), 2);
		TestEqual(TEXT("Disabled NoDrawType FillBar pushes 0 clipping states"), DrawElements.GetClippingManager().GetClippingStates().Num(), 0);
	}
	RichSlider->SetLocked(false);

	// 2-1. 0%: 채움 바 생략 -> Box 2개(BackgroundBar, Thumb), ClippingState 0개
	{
		RichSlider->SetValue(0.0f);
		FSlateWindowElementList DrawElements(DummyWindow);
		RichSlider->OnPaint(PaintArgs, HorizontalGeometry, CullingRect, DrawElements, 0, FWidgetStyle(), true);

		const auto& BoxElements = DrawElements.GetUncachedDrawElements().Get<static_cast<uint8>(EElementType::ET_Box)>();
		TestEqual(TEXT("At 0% Value, SRichSlider paints 2 boxes (Background + Thumb)"), BoxElements.Num(), 2);
		TestEqual(TEXT("At 0% Value, SRichSlider pushes 0 clipping states"), DrawElements.GetClippingManager().GetClippingStates().Num(), 0);
	}

	// 2-2. 50% (IndentHandle=true): HandleCenterX = 0.5 * (320 - 48) + 24 = 160
	{
		RichSlider->SetValue(0.5f);
		FSlateWindowElementList DrawElements(DummyWindow);
		RichSlider->OnPaint(PaintArgs, HorizontalGeometry, CullingRect, DrawElements, 0, FWidgetStyle(), true);

		const auto& BoxElements = DrawElements.GetUncachedDrawElements().Get<static_cast<uint8>(EElementType::ET_Box)>();
		const TArray<FSlateClippingState>& ClipStates = DrawElements.GetClippingManager().GetClippingStates();
		TestEqual(TEXT("At 50% Value, SRichSlider paints 3 boxes (Background + Fill + Thumb)"), BoxElements.Num(), 3);
		TestEqual(TEXT("At 50% Value, SRichSlider pushes 1 clipping state"), ClipStates.Num(), 1);
		if (ClipStates.Num() == 1 && ClipStates[0].ScissorRect.IsSet() == true)
		{
			TestEqual(TEXT("Fill clip starts at BarLeft (12.0)"), static_cast<float>(ClipStates[0].ScissorRect->TopLeft.X), 12.0f, 0.05f);
			TestEqual(TEXT("Fill clip ends at HandleCenterX (160.0)"), static_cast<float>(ClipStates[0].ScissorRect->TopRight.X), 160.0f, 0.05f);
		}
	}

	// 2-3. 100% (IndentHandle=true): HandleCenterX = 272 + 24 = 296 (BarRight=308보다 앞선 핸들 정중앙에서 정확히 정지)
	{
		RichSlider->SetValue(1.0f);
		FSlateWindowElementList DrawElements(DummyWindow);
		RichSlider->OnPaint(PaintArgs, HorizontalGeometry, CullingRect, DrawElements, 0, FWidgetStyle(), true);

		const TArray<FSlateClippingState>& ClipStates = DrawElements.GetClippingManager().GetClippingStates();
		TestEqual(TEXT("At 100% Value, SRichSlider pushes 1 clipping state"), ClipStates.Num(), 1);
		if (ClipStates.Num() == 1 && ClipStates[0].ScissorRect.IsSet() == true)
		{
			TestEqual(TEXT("At 100% IndentHandle=true, Fill clip stops at HandleCenterX (296.0)"), static_cast<float>(ClipStates[0].ScissorRect->TopRight.X), 296.0f, 0.05f);
		}
	}

	// 2-4. 수직(Orient_Vertical) OnPaint 검증
	{
		TSharedRef<SRichSlider> VerticalSlider = SNew(SRichSlider)
			.Style(&BaseStyle)
			.RichStyle(&RichStyle)
			.Orientation(Orient_Vertical)
			.Value(0.5f);
		const FGeometry VerticalGeometry = FGeometry::MakeRoot(FVector2D(32.0f, 320.0f), FSlateLayoutTransform());
		FSlateWindowElementList DrawElements(DummyWindow);
		VerticalSlider->OnPaint(PaintArgs, VerticalGeometry, CullingRect, DrawElements, 0, FWidgetStyle(), true);

		const auto& BoxElements = DrawElements.GetUncachedDrawElements().Get<static_cast<uint8>(EElementType::ET_Box)>();
		TestEqual(TEXT("Vertical SRichSlider paints 3 boxes (Background + Fill + Thumb)"), BoxElements.Num(), 3);
		TestEqual(TEXT("Vertical SRichSlider pushes 1 clipping state"), DrawElements.GetClippingManager().GetClippingStates().Num(), 1);
	}

	// 3. UMG URichSlider 파생 프로퍼티 동기화, USlider FieldNotify 계승 및 리소스 해제 검증
	{
		FExtensionWidgetTestWorldScope WorldScope;
		TestNotNull(TEXT("Test world valid for URichSlider"), WorldScope.World);
		if (WorldScope.World == nullptr)
		{
			return false;
		}

		// UWidget FieldNotify는 UUserWidget 소유 UWidgetTree 내 위젯에서만 확장 객체가 생성됨
		UUserWidget* HostUserWidget = CreateWidget<UBBMCreditEntryWidget>(WorldScope.World);
		TestNotNull(TEXT("Host UUserWidget created for URichSlider"), HostUserWidget);
		if (HostUserWidget == nullptr)
		{
			return false;
		}
		if (HostUserWidget->WidgetTree == nullptr)
		{
			HostUserWidget->WidgetTree = NewObject<UWidgetTree>(HostUserWidget);
		}

		URichSlider* UMGRichSlider = HostUserWidget->WidgetTree->ConstructWidget<URichSlider>(URichSlider::StaticClass());
		TestNotNull(TEXT("URichSlider created"), UMGRichSlider);
		if (UMGRichSlider == nullptr)
		{
			return false;
		}
		HostUserWidget->WidgetTree->RootWidget = UMGRichSlider;

		int32 ValueNotifyCount = 0;
		const FDelegateHandle NotifyHandle = UMGRichSlider->AddFieldValueChangedDelegate(
			USlider::FFieldNotificationClassDescriptor::Value,
			INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&ValueNotifyCount](UObject* /*InObj*/, UE::FieldNotification::FFieldId /*InId*/)
			{
				++ValueNotifyCount;
			})
		);
		TestTrue(TEXT("FieldNotify delegate handle valid inside WidgetTree"), NotifyHandle.IsValid());

		UMGRichSlider->SetWidgetStyle(BaseStyle);
		UMGRichSlider->SetRichStyle(RichStyle);
		UMGRichSlider->SetSliderFillBarColor(FLinearColor::Green);
		UMGRichSlider->SetMinValue(0.0f);
		UMGRichSlider->SetMaxValue(10.0f);
		UMGRichSlider->SetValue(5.0f);

		UMGRichSlider->TakeWidget();
		UMGRichSlider->SynchronizeProperties();

		const TSharedPtr<SRichSlider> SlateSlider = UMGRichSlider->GetSlateRichSlider();
		TestTrue(TEXT("URichSlider builds SRichSlider"), SlateSlider.IsValid());
		if (SlateSlider.IsValid() == true)
		{
			TestEqual(TEXT("URichSlider SynchronizeProperties forwards NormalizedValue 0.5"), SlateSlider->GetNormalizedValue(), 0.5f, 0.001f);
			TestTrue(TEXT("URichSlider forwards NormalFillBarImage after SynchronizeProperties"), SlateSlider->GetFillBarImage() == &UMGRichSlider->RichStyle.NormalFillBarImage);
		}
		TestEqual(TEXT("URichSlider inherits USlider FieldNotify for Value"), ValueNotifyCount, 1);

		UMGRichSlider->RemoveFieldValueChangedDelegate(USlider::FFieldNotificationClassDescriptor::Value, NotifyHandle);
		UMGRichSlider->ReleaseSlateResources(true);
		TestFalse(TEXT("URichSlider releases Slate widget cleanly"), UMGRichSlider->GetSlateRichSlider().IsValid());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
