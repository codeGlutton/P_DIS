/*****************************************************************//**
 * @file   BBMTitleViewModelTests.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 MVVM 뷰모델 및 월드 위젯 서브시스템 연동 단위 테스트
 * @details ViewModel 수명 주기, 세이브 유무에 따른 이어하기 버튼 Collapsed 가시성 제어 및 네비게이션 액션 검증
 * @date   2026-11-07
 *********************************************************************/

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "UI/BBMUserWidgetBase.h"
#include "UI/BBMTitleWidget.h"
#include "UI/BBMWorldWidgetSubsystem.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Save/BBMRunSaveGame.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * GameInstance 및 서브시스템을 포함하는 테스트 월드 스코프 구조체
 */
struct FBBMMVVMScopedTestWorld
{
	UWorld* World = nullptr;
	UGameInstance* GameInstance = nullptr;

	FBBMMVVMScopedTestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("BBM_MVVM_TestWorld"));
		if (World != nullptr)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);

			GameInstance = NewObject<UGameInstance>(GEngine);
			GameInstance->Init();
			World->SetGameInstance(GameInstance);

			World->InitializeActorsForPlay(FURL());
		}
	}

	~FBBMMVVMScopedTestWorld()
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

// 1. UBBMWorldWidgetSubsystem의 선택적 ViewModel 생성 및 위젯 Manual 주입 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMWorldWidgetViewModelInjectionTest,
	"DIS.BBM.UI.WorldWidgetViewModelInjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMWorldWidgetViewModelInjectionTest::RunTest(const FString& Parameters)
{
	FBBMMVVMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드 유효 검증"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMWorldWidgetSubsystem* WidgetSS = TestWorld.World->GetSubsystem<UBBMWorldWidgetSubsystem>();
	TestNotNull(TEXT("UBBMWorldWidgetSubsystem 획득 검증"), WidgetSS);
	if (WidgetSS == nullptr)
	{
		return false;
	}

	// 1-1. ViewModelClass 미지정 시: 위젯만 생성되고 ViewModel은 null이어야 함 (위젯 필수, MVVM 선택적)
	const FName PlainWidgetKey(TEXT("PlainWidget"));
	FBBMWorldWidgetConfig PlainConfig;
	PlainConfig.WidgetKey = PlainWidgetKey;
	PlainConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	UBBMUserWidgetBase* PlainWidget = WidgetSS->CreateAndRegisterWidgetByConfig(PlainConfig);
	TestNotNull(TEXT("순수 위젯 등록 성공"), PlainWidget);
	if (PlainWidget != nullptr)
	{
		TestNull(TEXT("서브시스템 GetViewModel도 nullptr"), WidgetSS->GetViewModel(PlainWidgetKey));
	}

	// 1-2. ViewModelClass 지정 시: 위젯 생성과 동시에 ViewModel 생성 및 주입 완료 검증
	const FName TitleWidgetKey(TEXT("TitleMenu"));
	FBBMWorldWidgetConfig TitleConfig;
	TitleConfig.WidgetKey = TitleWidgetKey;
	TitleConfig.WidgetClass = UBBMTitleWidget::StaticClass();
	TitleConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();
	UBBMUserWidgetBase* TitleUserWidget = WidgetSS->CreateAndRegisterWidgetByConfig(TitleConfig);
	TestNotNull(TEXT("타이틀 위젯 등록 성공"), TitleUserWidget);

	UBBMTitleWidget* TitleWidget = Cast<UBBMTitleWidget>(TitleUserWidget);
	TestNotNull(TEXT("UBBMTitleWidget 캐스팅 성공"), TitleWidget);

	// 서브시스템을 통한 ViewModel 생성 및 보관 검증
	UBBMViewModelBase* RegisteredVM = WidgetSS->GetViewModel(TitleWidgetKey);
	TestNotNull(TEXT("서브시스템을 통한 ViewModel 생성 및 등록 검증"), RegisteredVM);
	UBBMTitleViewModel* TitleVM = WidgetSS->GetViewModelAs<UBBMTitleViewModel>(TitleWidgetKey);
	TestNotNull(TEXT("서브시스템의 GetViewModelAs 템플릿 조회 성공"), TitleVM);

	if (TitleWidget != nullptr)
	{
		// 순수 C++ 위젯 인스턴스는 블루프린트 UMVVMViewClass 확장이 없으므로 GetViewModelByClass/GetTitleViewModel은 안전하게 nullptr 반환
		TestNull(TEXT("순수 C++ 인스턴스에서 MVVMView 미부착 시 GetViewModelByClass는 안전하게 nullptr 반환"), TitleWidget->GetViewModelByClass(UBBMTitleViewModel::StaticClass()));
		TestNull(TEXT("TitleWidget->GetTitleViewModel 안전 조회 검증"), TitleWidget->GetTitleViewModel());
	}

	// 1-3. 이미 열린 위젯(bIsOpen == true)에 ViewModel 지연 주입 시 NotifyWidgetOpened 즉시 호출 검증
	const FName OpenedWidgetKey(TEXT("OpenedWidget"));
	FBBMWorldWidgetConfig OpenedConfig;
	OpenedConfig.WidgetKey = OpenedWidgetKey;
	OpenedConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	UBBMUserWidgetBase* OpenedWidget = WidgetSS->CreateAndRegisterWidgetByConfig(OpenedConfig);
	TestNotNull(TEXT("오픈 위젯 등록 성공"), OpenedWidget);
	if (OpenedWidget != nullptr)
	{
		OpenedWidget->Open();
		TestTrue(TEXT("위젯 IsOpen true 검증"), OpenedWidget->IsOpen());

		UBBMTitleViewModel* DelayedTitleVM = NewObject<UBBMTitleViewModel>(TestWorld.World);
		TestNotNull(TEXT("지연 주입용 뷰모델 생성"), DelayedTitleVM);
		if (DelayedTitleVM != nullptr)
		{
			// 위젯이 이미 열린 상태에서 주입 시 SetViewModelInstance 내부에서 즉시 InViewModel->NotifyWidgetOpened()를 호출함
			// 세이브가 없는 초기 상태이므로 RefreshTitleState()에 의해 ContinueVisibility가 Collapsed로 동기화됨
			OpenedWidget->SetViewModelInstance(DelayedTitleVM);
			TestEqual(TEXT("지연 주입 시 NotifyWidgetOpened 즉시 호출로 ContinueVisibility 동기화 완료"), DelayedTitleVM->GetContinueVisibility(), ESlateVisibility::Collapsed);

			// 위젯 Close 호출 시 크래시 없이 닫힘 검증
			OpenedWidget->Close();
			TestFalse(TEXT("위젯 IsOpen false 검증"), OpenedWidget->IsOpen());
		}
		WidgetSS->RemoveWidget(OpenedWidgetKey);
	}

	// 1-4. 위젯 제거 시 연관된 ViewModel도 함께 정리 검증
	TestTrue(TEXT("TitleMenu 위젯 제거 성공"), WidgetSS->RemoveWidget(TitleWidgetKey));
	TestNull(TEXT("제거 후 GetWidget은 nullptr"), WidgetSS->GetWidget(TitleWidgetKey));
	TestNull(TEXT("제거 후 GetViewModel도 nullptr"), WidgetSS->GetViewModel(TitleWidgetKey));

	return true;
}

// 2. UBBMTitleViewModel의 세이브 상태 조회 및 이어하기 가시성(Collapsed/Visible) 제어 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMTitleViewModelSaveContinueVisibilityTest,
	"DIS.BBM.UI.TitleViewModelContinueVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMTitleViewModelSaveContinueVisibilityTest::RunTest(const FString& Parameters)
{
	FBBMMVVMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드 유효 검증"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr || TestWorld.GameInstance == nullptr)
	{
		return false;
	}

	UBBMGameDataSubsystem* GameDataSS = TestWorld.GameInstance->GetSubsystem<UBBMGameDataSubsystem>();
	TestNotNull(TEXT("UBBMGameDataSubsystem 획득 검증"), GameDataSS);
	if (GameDataSS == nullptr)
	{
		return false;
	}

	// 뷰모델 생성
	UBBMTitleViewModel* TitleVM = NewObject<UBBMTitleViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMTitleViewModel 생성 성공"), TitleVM);
	if (TitleVM == nullptr)
	{
		return false;
	}

	// 2-1. 초기 상태: 활성 런 세이브가 없을 때 -> ContinueVisibility == Collapsed, bCanContinue == false
	GameDataSS->EnsureSaveDataLoaded();
	UBBMRunSaveGame* RunData = GameDataSS->GetCurrentRunData();
	if (RunData != nullptr)
	{
		RunData->bHasActiveRun = false;
	}

	TitleVM->RefreshTitleState();
	TestFalse(TEXT("활성 런 없을 시 bCanContinue는 false"), TitleVM->GetCanContinue());
	TestEqual(TEXT("활성 런 없을 시 ContinueVisibility는 Collapsed"), TitleVM->GetContinueVisibility(), ESlateVisibility::Collapsed);
	TestTrue(TEXT("활성 런 없을 시 요약 텍스트는 Empty"), TitleVM->GetActiveRunSummaryText().IsEmpty());

	// 2-2. 활성 런 세이브가 존재할 때 -> ContinueVisibility == Visible, bCanContinue == true
	if (RunData != nullptr)
	{
		RunData->bHasActiveRun = true;
		RunData->CurrentEncounterIndex = 3;
	}

	TitleVM->RefreshTitleState();
	TestTrue(TEXT("활성 런 존재 시 bCanContinue는 true"), TitleVM->GetCanContinue());
	TestEqual(TEXT("활성 런 존재 시 ContinueVisibility는 Visible"), TitleVM->GetContinueVisibility(), ESlateVisibility::Visible);
	TestFalse(TEXT("활성 런 존재 시 요약 텍스트는 비어있지 않아야 함"), TitleVM->GetActiveRunSummaryText().IsEmpty());

	// 2-3. 네비게이션 델리게이트 브로드캐스트 검증
	bool bStartGameRequested = false;
	bool bContinueGameRequested = false;
	bool bSettingsRequested = false;
	bool bCreditsRequested = false;

	TitleVM->OnStartNewGameRequestedNative.AddLambda([&bStartGameRequested]() { bStartGameRequested = true; });
	TitleVM->OnContinueGameRequestedNative.AddLambda([&bContinueGameRequested]() { bContinueGameRequested = true; });
	TitleVM->OnOpenSettingsRequestedNative.AddLambda([&bSettingsRequested]() { bSettingsRequested = true; });
	TitleVM->OnOpenCreditsRequestedNative.AddLambda([&bCreditsRequested]() { bCreditsRequested = true; });

	TitleVM->StartNewGame();
	TestTrue(TEXT("StartNewGame 호출 시 OnStartNewGameRequested 브로드캐스트 검증"), bStartGameRequested);

	TitleVM->ContinueGame();
	TestTrue(TEXT("ContinueGame 호출 시 OnContinueGameRequested 브로드캐스트 검증"), bContinueGameRequested);

	TitleVM->OpenSettings();
	TestTrue(TEXT("OpenSettings 호출 시 OnOpenSettingsRequested 브로드캐스트 검증"), bSettingsRequested);

	TitleVM->OpenCredits();
	TestTrue(TEXT("OpenCredits 호출 시 OnOpenCreditsRequested 브로드캐스트 검증"), bCreditsRequested);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

