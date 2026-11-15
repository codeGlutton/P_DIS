#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/RichListView.h"
#include "Components/RichScrollBar.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Layout/ArrangedChildren.h"
#include "Layout/Geometry.h"
#include "Types/RichScrollBarTypes.h"
#include "UI/Widget/BBMCreditEntryWidget.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/SRichListView.h"
#include "Widgets/SRichScrollBar.h"
#include "Widgets/SRichScrollBarBridge.h"
#include "Widgets/SRichScrollTrack.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
