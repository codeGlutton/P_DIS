#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Components/RichComboBoxKey.h"
#include "Components/RichHorizontalBox.h"
#include "Components/RichInputKeySelector.h"
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
#include "Kismet2/KismetEditorUtilities.h"
#include "Layout/ArrangedChildren.h"
#include "Layout/Geometry.h"
#include "MVVMBlueprintView.h"
#include "MVVMBlueprintViewModelContext.h"
#include "MVVMSubsystem.h"
#include "MVVMWidgetBlueprintExtension_View.h"
#include "Rendering/DrawElements.h"
#include "RichWidgetTestHelpers.h"
#include "Slate/SObjectWidget.h"
#include "Types/PaintArgs.h"
#include "Types/RichComboBoxTypes.h"
#include "Types/RichScrollBarTypes.h"
#include "Types/RichSliderTypes.h"
#include "UI/ViewModel/BBMTabEntryViewModel.h"
#include "UI/Widget/BBMCreditEntryWidget.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "View/MVVMView.h"
#include "View/MVVMViewClass.h"
#include "WidgetBlueprint.h"
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

	UClass* CreateTransientMVVMEntryWidgetBlueprintClass(const TCHAR* InAssetBaseName, FName InViewModelName)
	{
		const FName UniqueName = MakeUniqueObjectName(GetTransientPackage(), UWidgetBlueprint::StaticClass(), FName(InAssetBaseName));
		UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
			UUserWidget::StaticClass(),
			GetTransientPackage(),
			UniqueName,
			BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(),
			UWidgetBlueprintGeneratedClass::StaticClass()
		));
		if (WidgetBlueprint == nullptr)
		{
			return nullptr;
		}

		if (WidgetBlueprint->WidgetTree == nullptr)
		{
			WidgetBlueprint->WidgetTree = NewObject<UWidgetTree>(WidgetBlueprint);
		}
		if (WidgetBlueprint->WidgetTree != nullptr && WidgetBlueprint->WidgetTree->RootWidget == nullptr)
		{
			WidgetBlueprint->WidgetTree->RootWidget = WidgetBlueprint->WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		}

		UMVVMWidgetBlueprintExtension_View* ViewExtension = UWidgetBlueprintExtension::RequestExtension<UMVVMWidgetBlueprintExtension_View>(WidgetBlueprint);
		if (ViewExtension == nullptr)
		{
			return nullptr;
		}

		if (ViewExtension->GetBlueprintView() == nullptr)
		{
			ViewExtension->CreateBlueprintViewInstance();
		}

		UMVVMBlueprintView* BlueprintView = ViewExtension->GetBlueprintView();
		if (BlueprintView == nullptr)
		{
			return nullptr;
		}

		if (UMVVMBlueprintViewSettings* ViewSettings = BlueprintView->GetSettings())
		{
			ViewSettings->bCreateViewWithoutBindings = true;
		}

		FMVVMBlueprintViewModelContext ViewModelContext(UBBMTabEntryViewModel::StaticClass(), InViewModelName);
		ViewModelContext.CreationType = EMVVMBlueprintViewModelContextCreationType::Manual;
		BlueprintView->AddViewModel(ViewModelContext);

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		return WidgetBlueprint->GeneratedClass;
	}

	UUserWidget* FindFirstChildUserWidget(const TSharedRef<SWidget>& InSlateWidget, const UUserWidget* InIgnoredRoot)
	{
		if (InSlateWidget->GetType() == SObjectWidget::StaticWidgetClass().GetWidgetType())
		{
			UUserWidget* CandidateWidget = StaticCastSharedRef<SObjectWidget>(InSlateWidget)->GetWidgetObject();
			if (CandidateWidget != nullptr && CandidateWidget != InIgnoredRoot)
			{
				return CandidateWidget;
			}
		}

		FChildren* Children = const_cast<SWidget&>(InSlateWidget.Get()).GetChildren();
		if (Children != nullptr)
		{
			const int32 NumChildren = Children->Num();
			for (int32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
			{
				if (UUserWidget* FoundWidget = FindFirstChildUserWidget(Children->GetChildAt(ChildIndex), InIgnoredRoot))
				{
					return FoundWidget;
				}
			}
		}

		return nullptr;
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

// =================================================================================================
// 6. URichComboBoxKey ViewModel Extension 모방 자동 생성 · VM 주입 · SelectedKey TwoWay 동기화 검증
// =================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRichComboBoxKeyViewModelExtensionTest,
	"DIS.ExtensionWidget.RichComboBoxKey.ViewModelExtension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FRichComboBoxKeyViewModelExtensionTest::RunTest(const FString& Parameters)
{
	FExtensionWidgetTestWorldScope WorldScope;
	TestNotNull(TEXT("Test world valid for URichComboBoxKey"), WorldScope.World);
	if (WorldScope.World == nullptr)
	{
		return false;
	}

	UClass* ContentEntryClass = CreateTransientMVVMEntryWidgetBlueprintClass(TEXT("TransientComboContentBP"), FName(TEXT("ContentEntryVM")));
	UClass* ItemEntryClass = CreateTransientMVVMEntryWidgetBlueprintClass(TEXT("TransientComboItemBP"), FName(TEXT("ItemEntryVM")));
	TestNotNull(TEXT("Content entry WBP compiled"), ContentEntryClass);
	TestNotNull(TEXT("Item entry WBP compiled"), ItemEntryClass);
	if (ContentEntryClass == nullptr || ItemEntryClass == nullptr)
	{
		return false;
	}

	UWidgetBlueprintGeneratedClass* ContentWBPGC = Cast<UWidgetBlueprintGeneratedClass>(ContentEntryClass);
	UWidgetBlueprintGeneratedClass* ItemWBPGC = Cast<UWidgetBlueprintGeneratedClass>(ItemEntryClass);
	TestNotNull(TEXT("Content WBP generated class has UMVVMViewClass"), ContentWBPGC != nullptr ? ContentWBPGC->GetExtension<UMVVMViewClass>() : nullptr);
	TestNotNull(TEXT("Item WBP generated class has UMVVMViewClass"), ItemWBPGC != nullptr ? ItemWBPGC->GetExtension<UMVVMViewClass>() : nullptr);

	// 헤드리스 테스트 월드(LocalPlayer 미생성)에서도 UMVVMViewClass::Initialize가 실행되도록 허용함
	if (ContentWBPGC != nullptr)
	{
		ContentWBPGC->bCanCallInitializedWithoutPlayerContext = true;
	}
	if (ItemWBPGC != nullptr)
	{
		ItemWBPGC->bCanCallInitializedWithoutPlayerContext = true;
	}

	UUserWidget* HostWidget = CreateWidget<UBBMCreditEntryWidget>(WorldScope.World);
	TestNotNull(TEXT("Host widget created for URichComboBoxKey"), HostWidget);
	if (HostWidget == nullptr)
	{
		return false;
	}
	if (HostWidget->WidgetTree == nullptr)
	{
		HostWidget->WidgetTree = NewObject<UWidgetTree>(HostWidget);
	}

	URichComboBoxKey* ComboBox = HostWidget->WidgetTree->ConstructWidget<URichComboBoxKey>(URichComboBoxKey::StaticClass());
	TestNotNull(TEXT("URichComboBoxKey constructed"), ComboBox);
	if (ComboBox == nullptr)
	{
		return false;
	}
	HostWidget->WidgetTree->RootWidget = ComboBox;

	ComboBox->SetUseViewModelExtension(true);
	ComboBox->SetContentWidgetClass(ContentEntryClass);
	ComboBox->SetContentViewModelName(FName(TEXT("ContentEntryVM")));
	ComboBox->SetContentViewModelClass(UBBMTabEntryViewModel::StaticClass());
	ComboBox->SetItemWidgetClass(ItemEntryClass);
	ComboBox->SetItemViewModelName(FName(TEXT("ItemEntryVM")));
	ComboBox->SetItemViewModelClass(UBBMTabEntryViewModel::StaticClass());

	UBBMTabEntryViewModel* KoreanVM = NewObject<UBBMTabEntryViewModel>(HostWidget);
	UBBMTabEntryViewModel* EnglishVM = NewObject<UBBMTabEntryViewModel>(HostWidget);
	UBBMTabEntryViewModel* UpdatedKoreanVM = NewObject<UBBMTabEntryViewModel>(HostWidget);
	KoreanVM->SetTabText(FText::FromString(TEXT("한국어")));
	EnglishVM->SetTabText(FText::FromString(TEXT("English")));
	UpdatedKoreanVM->SetTabText(FText::FromString(TEXT("한국어 (갱신)")));

	int32 SelectedOptionNotifyCount = 0;
	FName LastNotifiedSelectedOption = NAME_None;
	int32 SelectedOptionDelegateCount = 0;
	FName LastDelegateOption = NAME_None;
	int32 LastDelegateIndex = INDEX_NONE;
	const FDelegateHandle SelectedOptionNotifyHandle = ComboBox->AddFieldValueChangedDelegate(
		URichComboBoxKey::FFieldNotificationClassDescriptor::SelectedOption,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([ComboBox, &SelectedOptionNotifyCount, &LastNotifiedSelectedOption](UObject* /*InObj*/, UE::FieldNotification::FFieldId /*InId*/)
		{
			++SelectedOptionNotifyCount;
			LastNotifiedSelectedOption = ComboBox->GetSelectedOption();
		})
	);
	const FDelegateHandle SelectedOptionDelegateHandle = ComboBox->OnSelectedOptionChangedNative.AddLambda([&SelectedOptionDelegateCount, &LastDelegateOption, &LastDelegateIndex](FName InKey, int32 InIndex)
	{
		++SelectedOptionDelegateCount;
		LastDelegateOption = InKey;
		LastDelegateIndex = InIndex;
	});
	TestTrue(TEXT("SelectedOption FieldNotify delegate registered"), SelectedOptionNotifyHandle.IsValid());
	TestTrue(TEXT("OnSelectedOptionChangedNative delegate registered"), SelectedOptionDelegateHandle.IsValid());

	// 1. SetViewModelItems 호출 시 기존 SelectedOption 초기화(Slate 생성 전/후 일관 동작) 및 이후 선택 유지 검증
	const FName KoreanKey(TEXT("ko_KR"));
	const FName EnglishKey(TEXT("en"));
	ComboBox->SetSelectedOption(KoreanKey);
	TestEqual(TEXT("SelectedOption set before items"), ComboBox->GetSelectedOption(), KoreanKey);
	TestEqual(TEXT("SelectedOption notify fired once on initial set"), SelectedOptionNotifyCount, 1);
	TestEqual(TEXT("Last notified key inside initial callback matches KoreanKey"), LastNotifiedSelectedOption, KoreanKey);
	TestEqual(TEXT("OnSelectedOptionChangedNative fired once on initial set"), SelectedOptionDelegateCount, 1);
	TestEqual(TEXT("Initial delegate index before items is INDEX_NONE"), LastDelegateIndex, static_cast<int32>(INDEX_NONE));

	TArray<FRichComboBoxKeyViewModelItem> InitialItems;
	InitialItems.Add({ KoreanKey, KoreanVM });
	InitialItems.Add({ EnglishKey, EnglishVM });
	ComboBox->SetViewModelItems(InitialItems);

	TestEqual(TEXT("SetViewModelItems resets SelectedOption to NAME_None before Slate build"), ComboBox->GetSelectedOption(), FName(NAME_None));
	TestEqual(TEXT("SelectedOption notify incremented to 2 on SetViewModelItems clear"), SelectedOptionNotifyCount, 2);
	TestEqual(TEXT("OnSelectedOptionChangedNative incremented to 2 on SetViewModelItems clear"), SelectedOptionDelegateCount, 2);
	TestEqual(TEXT("Cleared delegate index is INDEX_NONE"), LastDelegateIndex, static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("CachedKeyIndices resolves KoreanKey index 0 in O(1)"), ComboBox->FindItemIndexByKey(KoreanKey), 0);
	TestEqual(TEXT("CachedKeyIndices resolves EnglishKey index 1 in O(1)"), ComboBox->FindItemIndexByKey(EnglishKey), 1);
	TestEqual(TEXT("Cached TMap returns KoreanVM in O(1)"), ComboBox->FindViewModelByKey(KoreanKey), static_cast<UObject*>(KoreanVM));
	TestEqual(TEXT("Cached TMap returns EnglishVM in O(1)"), ComboBox->FindViewModelByKey(EnglishKey), static_cast<UObject*>(EnglishVM));

	// 항목 설정 이후 선택된 키는 TakeWidget() 시 Content 위젯 초기 생성까지 유지되어야 함
	ComboBox->SetSelectedOption(KoreanKey);
	TestEqual(TEXT("SelectedOption set after items resolves KoreanKey"), ComboBox->GetSelectedOption(), KoreanKey);
	TestEqual(TEXT("SelectedOption notify incremented to 3"), SelectedOptionNotifyCount, 3);
	TestEqual(TEXT("OnSelectedOptionChangedNative incremented to 3"), SelectedOptionDelegateCount, 3);
	TestEqual(TEXT("Delegate index resolves KoreanKey index 0"), LastDelegateIndex, 0);

	// 2. bUseViewModelExtension == true 시 OnGenerateContentWidget 무시 및 Content 위젯 자동 VM 주입 검증
	URichWidgetTestDelegateHelper* DelegateHelper = NewObject<URichWidgetTestDelegateHelper>(HostWidget);
	TestNotNull(TEXT("Delegate helper constructed"), DelegateHelper);
	if (DelegateHelper == nullptr)
	{
		return false;
	}
	ComboBox->OnGenerateContentWidget.BindUFunction(DelegateHelper, FName(TEXT("HandleGenerateContentWidget")));
	TestTrue(TEXT("Manual OnGenerateContentWidget delegate bound"), ComboBox->OnGenerateContentWidget.IsBound());

	TSharedRef<SWidget> ComboSlate = ComboBox->TakeWidget();
	TestEqual(TEXT("bUseViewModelExtension ignores manual OnGenerateContentWidget"), DelegateHelper->GenerateContentCallCount, 0);

	UUserWidget* GeneratedContentWidget = FindFirstChildUserWidget(ComboSlate, HostWidget);
	TestNotNull(TEXT("Content entry widget generated into ComboBoxContent by extension"), GeneratedContentWidget);
	if (GeneratedContentWidget != nullptr)
	{
		UMVVMView* ContentView = UMVVMSubsystem::GetViewFromUserWidget(GeneratedContentWidget);
		TestNotNull(TEXT("Content widget has UMVVMView"), ContentView);
		if (ContentView != nullptr)
		{
			TestEqual(TEXT("Content UMVVMView receives KoreanVM by name"), ContentView->GetViewModel(FName(TEXT("ContentEntryVM"))).GetObject(), static_cast<UObject*>(KoreanVM));
		}
	}

	// 3. SetViewModelItems 교체 시 선택 초기화 후 SetViewModelByClass 폴백 주입 검증
	ComboBox->SetContentViewModelName(NAME_None);
	TArray<FRichComboBoxKeyViewModelItem> ReplacedItems;
	ReplacedItems.Add({ KoreanKey, UpdatedKoreanVM });
	ReplacedItems.Add({ EnglishKey, EnglishVM });
	ComboBox->SetViewModelItems(ReplacedItems);

	TestEqual(TEXT("SetViewModelItems resets SelectedOption to NAME_None after Slate build"), ComboBox->GetSelectedOption(), FName(NAME_None));
	TestEqual(TEXT("SelectedOption notify incremented to 4 on Slate SetViewModelItems clear"), SelectedOptionNotifyCount, 4);
	TestEqual(TEXT("OnSelectedOptionChangedNative incremented to 4 on Slate SetViewModelItems clear"), SelectedOptionDelegateCount, 4);

	ComboBox->SetSelectedOption(KoreanKey);
	TestEqual(TEXT("SelectedOption restored to KoreanKey after item replacement"), ComboBox->GetSelectedOption(), KoreanKey);
	TestEqual(TEXT("SelectedOption notify incremented to 5"), SelectedOptionNotifyCount, 5);
	TestEqual(TEXT("OnSelectedOptionChangedNative incremented to 5"), SelectedOptionDelegateCount, 5);

	UUserWidget* FallbackContentWidget = FindFirstChildUserWidget(ComboSlate, HostWidget);
	TestNotNull(TEXT("Fallback Content widget generated after selecting KoreanKey"), FallbackContentWidget);
	if (FallbackContentWidget != nullptr && FallbackContentWidget != GeneratedContentWidget)
	{
		UMVVMView* FallbackView = UMVVMSubsystem::GetViewFromUserWidget(FallbackContentWidget);
		TestNotNull(TEXT("Fallback Content widget has UMVVMView"), FallbackView);
		if (FallbackView != nullptr)
		{
			TestEqual(TEXT("Fallback SetViewModelByClass injects UpdatedKoreanVM"), FallbackView->GetViewModel(FName(TEXT("ContentEntryVM"))).GetObject(), static_cast<UObject*>(UpdatedKoreanVM));
		}
	}

	// 4. 선택 변경(SetSelectedOption -> SelectedOption 선 대입 후 TwoWay 브로드캐스트 + OnSelectedOptionChangedNative) 및 TwoWay 무순환 검증
	FName SimulatedVMKey = KoreanKey;
	int32 ReverseTwoWayCount = 0;
	const FDelegateHandle TwoWayHandle = ComboBox->AddFieldValueChangedDelegate(
		URichComboBoxKey::FFieldNotificationClassDescriptor::SelectedOption,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([ComboBox, &SimulatedVMKey, &ReverseTwoWayCount](UObject* /*InObj*/, UE::FieldNotification::FFieldId /*InId*/)
		{
			++ReverseTwoWayCount;
			const FName PulledKey = ComboBox->GetSelectedOption();
			if (SimulatedVMKey != PulledKey)
			{
				SimulatedVMKey = PulledKey;
				ComboBox->SetSelectedOption(SimulatedVMKey);
			}
		})
	);

	ComboBox->SetSelectedOption(EnglishKey);
	TestEqual(TEXT("SelectedOption synced from SetSelectedOption"), ComboBox->GetSelectedOption(), EnglishKey);
	TestEqual(TEXT("Simulated TwoWay VM received EnglishKey"), SimulatedVMKey, EnglishKey);
	TestEqual(TEXT("TwoWay loop terminated in 1 hop with zero recursion"), ReverseTwoWayCount, 1);
	TestEqual(TEXT("SelectedOption notify incremented once on external selection change"), SelectedOptionNotifyCount, 6);
	TestEqual(TEXT("Last notified SelectedOption matches EnglishKey"), LastNotifiedSelectedOption, EnglishKey);
	TestEqual(TEXT("OnSelectedOptionChangedNative incremented once on external selection change"), SelectedOptionDelegateCount, 6);
	TestEqual(TEXT("OnSelectedOptionChangedNative passed EnglishKey"), LastDelegateOption, EnglishKey);
	TestEqual(TEXT("OnSelectedOptionChangedNative passed EnglishKey index 1"), LastDelegateIndex, 1);
	ComboBox->RemoveFieldValueChangedDelegate(URichComboBoxKey::FFieldNotificationClassDescriptor::SelectedOption, TwoWayHandle);

	// 5. 새로운 옵션 목록 전달 시 SelectedOption이 NAME_None으로 정리되는지 검증
	TArray<FRichComboBoxKeyViewModelItem> OnlyKoreanItems;
	OnlyKoreanItems.Add({ KoreanKey, KoreanVM });
	ComboBox->SetViewModelItems(OnlyKoreanItems);
	TestEqual(TEXT("SelectedOption resets to NAME_None when SetViewModelItems updates items"), ComboBox->GetSelectedOption(), FName(NAME_None));
	TestEqual(TEXT("SelectedOption notify increments when cleared by SetViewModelItems"), SelectedOptionNotifyCount, 7);
	TestEqual(TEXT("OnSelectedOptionChangedNative increments when cleared by SetViewModelItems"), SelectedOptionDelegateCount, 7);
	TestEqual(TEXT("OnSelectedOptionChangedNative fires NAME_None and INDEX_NONE when invalidated"), LastDelegateIndex, static_cast<int32>(INDEX_NONE));

	// 6. bUseViewModelExtension == false 전환 시 수동 OnGenerateContentWidget 및 수동 옵션 API(AddOption/ClearOptions) 동작 검증
	ComboBox->SetUseViewModelExtension(false);
	TestEqual(TEXT("Manual OnGenerateContentWidget called once when disabling ViewModelExtension"), DelegateHelper->GenerateContentCallCount, 1);
	ComboBox->AddOption(EnglishKey);
	ComboBox->SetSelectedOption(EnglishKey);
	TestEqual(TEXT("Manual OnGenerateContentWidget invoked again on SetSelectedOption when bUseViewModelExtension == false"), DelegateHelper->GenerateContentCallCount, 2);
	TestEqual(TEXT("Manual OnGenerateContentWidget received EnglishKey"), DelegateHelper->LastGeneratedContentKey, EnglishKey);
	TestNull(TEXT("No Extension UUserWidget child when manual delegate returns USpacer"), FindFirstChildUserWidget(ComboSlate, HostWidget));

	ComboBox->ClearOptions();
	TestEqual(TEXT("ClearOptions resets SelectedOption to NAME_None"), ComboBox->GetSelectedOption(), FName(NAME_None));

	ComboBox->RemoveFieldValueChangedDelegate(URichComboBoxKey::FFieldNotificationClassDescriptor::SelectedOption, SelectedOptionNotifyHandle);
	ComboBox->OnSelectedOptionChangedNative.Remove(SelectedOptionDelegateHandle);
	ComboBox->ReleaseSlateResources(true);
	TestTrue(TEXT("Manual OnGenerateContentWidget delegate preserved across ReleaseSlateResources"), ComboBox->OnGenerateContentWidget.IsBoundToObject(DelegateHelper));

	return true;
}

/**
 * 7. URichInputKeySelector FieldNotify 선 대입-후 알림 및 TwoWay 무순환 · 에코 억제 검증 테스트
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRichInputKeySelectorTwoWayTest,
	"DIS.ExtensionWidget.RichInputKeySelector.TwoWayAndEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FRichInputKeySelectorTwoWayTest::RunTest(const FString& Parameters)
{
	FExtensionWidgetTestWorldScope WorldScope;
	TestNotNull(TEXT("Test world valid for URichInputKeySelector"), WorldScope.World);
	if (WorldScope.World == nullptr)
	{
		return false;
	}

	UUserWidget* HostWidget = CreateWidget<UBBMCreditEntryWidget>(WorldScope.World);
	TestNotNull(TEXT("Host widget created for URichInputKeySelector"), HostWidget);
	if (HostWidget == nullptr || HostWidget->WidgetTree == nullptr)
	{
		return false;
	}

	URichInputKeySelector* KeySelector = HostWidget->WidgetTree->ConstructWidget<URichInputKeySelector>(URichInputKeySelector::StaticClass());
	TestNotNull(TEXT("URichInputKeySelector constructed"), KeySelector);
	if (KeySelector == nullptr)
	{
		return false;
	}
	HostWidget->WidgetTree->RootWidget = KeySelector;

	URichWidgetTestDelegateHelper* DelegateHelper = NewObject<URichWidgetTestDelegateHelper>(HostWidget);
	TestNotNull(TEXT("Delegate helper constructed for URichInputKeySelector"), DelegateHelper);
	if (DelegateHelper == nullptr)
	{
		return false;
	}
	KeySelector->OnKeySelected.AddDynamic(DelegateHelper, &URichWidgetTestDelegateHelper::HandleKeySelected);

	// Slate 위젯 생성 상태에서 키 변경 시 FieldNotify 및 OnKeySelected가 각 1회씩 발행되고 동일 값 재설정/SynchronizeProperties에서는 중복 발행되지 않아야 함
	KeySelector->TakeWidget();

	FInputChord SimulatedVMChord(EKeys::W);
	int32 FieldNotifyCount = 0;
	int32 ReverseTwoWayCount = 0;
	FInputChord LastNotifiedChord;

	const FDelegateHandle SelectedKeyNotifyHandle = KeySelector->AddFieldValueChangedDelegate(
		URichInputKeySelector::FFieldNotificationClassDescriptor::SelectedKey,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([KeySelector, &SimulatedVMChord, &FieldNotifyCount, &ReverseTwoWayCount, &LastNotifiedChord](UObject* /*InObj*/, UE::FieldNotification::FFieldId /*InId*/)
		{
			++FieldNotifyCount;
			// 콜백 실행 시점에 이미 GetSelectedKey()가 신값을 반환해야 함 (엔진 UInputKeySelector 버그 부재 검증)
			LastNotifiedChord = KeySelector->GetSelectedKey();
			if (ReverseTwoWayCount < 8 && SimulatedVMChord != LastNotifiedChord)
			{
				++ReverseTwoWayCount;
				SimulatedVMChord = LastNotifiedChord;
				KeySelector->SetSelectedKey(SimulatedVMChord);
			}
		})
	);

	// 1. 초기 키 설정: W
	KeySelector->SetSelectedKey(SimulatedVMChord);
	TestEqual(TEXT("Initial SetSelectedKey notifies once"), FieldNotifyCount, 1);
	TestEqual(TEXT("GetSelectedKey inside FieldNotify already equals W"), LastNotifiedChord, FInputChord(EKeys::W));
	TestEqual(TEXT("OnKeySelected fires once on initial W update"), DelegateHelper->KeySelectedCallCount, 1);
	TestEqual(TEXT("OnKeySelected received W"), DelegateHelper->LastSelectedKeyChord, FInputChord(EKeys::W));

	// 2. TwoWay 변경: W -> SpaceBar
	SimulatedVMChord = FInputChord(EKeys::SpaceBar);
	KeySelector->SetSelectedKey(SimulatedVMChord);
	TestEqual(TEXT("TwoWay update W -> SpaceBar notifies once"), FieldNotifyCount, 2);
	TestEqual(TEXT("GetSelectedKey inside FieldNotify equals SpaceBar"), LastNotifiedChord, FInputChord(EKeys::SpaceBar));
	TestEqual(TEXT("Simulated TwoWay VM kept SpaceBar without rollback"), SimulatedVMChord, FInputChord(EKeys::SpaceBar));
	TestEqual(TEXT("No recursive re-entry occurred after SpaceBar update"), ReverseTwoWayCount, 0);
	TestEqual(TEXT("OnKeySelected fires once for SpaceBar update"), DelegateHelper->KeySelectedCallCount, 2);
	TestEqual(TEXT("OnKeySelected received SpaceBar"), DelegateHelper->LastSelectedKeyChord, FInputChord(EKeys::SpaceBar));

	// 3. ResetToDefaults 시나리오 재현: SpaceBar -> W 복원 시 순환 없이 1회 종료
	SimulatedVMChord = FInputChord(EKeys::W);
	KeySelector->SetSelectedKey(SimulatedVMChord);
	TestEqual(TEXT("TwoWay reset SpaceBar -> W notifies once"), FieldNotifyCount, 3);
	TestEqual(TEXT("GetSelectedKey inside FieldNotify equals W"), LastNotifiedChord, FInputChord(EKeys::W));
	TestEqual(TEXT("Simulated TwoWay VM kept W without rollback"), SimulatedVMChord, FInputChord(EKeys::W));
	TestEqual(TEXT("No recursive re-entry occurred after reset"), ReverseTwoWayCount, 0);
	TestEqual(TEXT("OnKeySelected fires once on reset to W"), DelegateHelper->KeySelectedCallCount, 3);
	TestEqual(TEXT("OnKeySelected received W after reset"), DelegateHelper->LastSelectedKeyChord, FInputChord(EKeys::W));

	// 4. 동일 키 재설정 및 SynchronizeProperties 호출 시 중복 알림 차단 검증
	KeySelector->SetSelectedKey(SimulatedVMChord);
	KeySelector->SynchronizeProperties();
	TestEqual(TEXT("Same key and SynchronizeProperties do not emit extra FieldNotify"), FieldNotifyCount, 3);
	TestEqual(TEXT("Same key and SynchronizeProperties do not emit extra OnKeySelected"), DelegateHelper->KeySelectedCallCount, 3);

	KeySelector->RemoveFieldValueChangedDelegate(URichInputKeySelector::FFieldNotificationClassDescriptor::SelectedKey, SelectedKeyNotifyHandle);
	static_cast<UVisual*>(KeySelector)->ReleaseSlateResources(true);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
