/*****************************************************************//**
 * @file   BBMTitleViewModelTests.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 MVVM 뷰모델 및 월드 위젯 서브시스템 연동 단위 테스트
 * @details ViewModel 수명 주기, 세이브 유무에 따른 이어하기 버튼 Hidden 가시성 제어 및 네비게이션 액션 검증
 * @date   2026-11-07
 *********************************************************************/

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "UI/Widget/BBMUserWidgetBase.h"
#include "UI/Widget/BBMTitleWidget.h"
#include "UI/Widget/BBMSettingsWidget.h"
#include "UI/Widget/BBMCreditsWidget.h"
#include "UI/BBMWorldUISubsystem.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "UI/ViewModel/BBMSettingsViewModel.h"
#include "UI/ViewModel/BBMCreditsViewModel.h"
#include "UI/ViewModel/BBMSingleAnimTransition.h"
#include "UI/ViewModel/BBMDualAnimTransition.h"
#include "Audio/BBMAudioSettings.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Save/BBMRunSaveGame.h"
#include "Save/BBMUserSaveGame.h"
#include "Save/BBMSaveTypes.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Async/TaskGraphInterfaces.h"

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
			WorldContext.OwningGameInstance = GameInstance;
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

// 1. UBBMWorldUISubsystem의 독립 위젯/ViewModel 생성 및 위젯 수동 연결 검증
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

	UBBMWorldUISubsystem* UISS = TestWorld.World->GetSubsystem<UBBMWorldUISubsystem>();
	TestNotNull(TEXT("UBBMWorldUISubsystem 획득 검증"), UISS);
	if (UISS == nullptr)
	{
		return false;
	}

	// 1-1. 순수 위젯만 등록 시 ViewModel은 생성/연결되지 않아야 함
	const FName PlainWidgetKey(TEXT("PlainWidget"));
	FBBMWorldWidgetConfig PlainConfig;
	PlainConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	UBBMUserWidgetBase* PlainWidget = UISS->CreateAndRegisterWidgetByConfig(PlainWidgetKey, PlainConfig);
	TestNotNull(TEXT("순수 위젯 등록 성공"), PlainWidget);
	if (PlainWidget != nullptr)
	{
		TestNull(TEXT("서브시스템 GetViewModel은 nullptr"), UISS->GetViewModel(PlainWidgetKey));
		TestNull(TEXT("위젯 GetViewModel은 nullptr"), PlainWidget->GetViewModel());
	}

	// 1-2. 위젯과 ViewModel 별도 생성 후 수동 바인딩 주입 검증
	const FName TitleWidgetKey(TEXT("TitleMenu"));
	FBBMWorldWidgetConfig TitleWidgetConfig;
	TitleWidgetConfig.WidgetClass = UBBMTitleWidget::StaticClass();
	UBBMUserWidgetBase* TitleUserWidget = UISS->CreateAndRegisterWidgetByConfig(TitleWidgetKey, TitleWidgetConfig);
	TestNotNull(TEXT("타이틀 위젯 등록 성공"), TitleUserWidget);

	FBBMWorldViewModelConfig TitleVMConfig;
	TitleVMConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();
	UBBMViewModelBase* RegisteredVM = UISS->CreateAndRegisterViewModelByConfig(TitleWidgetKey, TitleVMConfig);
	TestNotNull(TEXT("서브시스템을 통한 ViewModel 독립 생성 및 등록 검증"), RegisteredVM);

	UBBMTitleWidget* TitleWidget = Cast<UBBMTitleWidget>(TitleUserWidget);
	TestNotNull(TEXT("UBBMTitleWidget 캐스팅 성공"), TitleWidget);
	UBBMTitleViewModel* TitleVM = UISS->GetViewModelAs<UBBMTitleViewModel>(TitleWidgetKey);
	TestNotNull(TEXT("서브시스템의 GetViewModelAs 템플릿 조회 성공"), TitleVM);

	if (TitleWidget != nullptr && TitleVM != nullptr)
	{
		// 수동 연결 전에는 위젯 내부 단일 ViewModel이 비어 있어야 함
		TestNull(TEXT("수동 연결 전 GetViewModel은 nullptr"), TitleWidget->GetViewModel());
		TestNull(TEXT("수동 연결 전 TitleWidget->GetTitleViewModel은 nullptr"), TitleWidget->GetTitleViewModel());

		// 수동 연결 이후 단일 ViewModel 및 템플릿 캐스팅 조회가 즉시 동작해야 함
		TitleWidget->SetViewModelInstance(TitleVM);
		TestEqual(TEXT("수동 연결 후 GetViewModel 일치"), TitleWidget->GetViewModel(), Cast<UBBMViewModelBase>(TitleVM));
		TestEqual(TEXT("수동 연결 후 GetViewModelAs 일치"), TitleWidget->GetViewModelAs<UBBMTitleViewModel>(), TitleVM);
		TestEqual(TEXT("수동 연결 후 TitleWidget->GetTitleViewModel 일치"), TitleWidget->GetTitleViewModel(), TitleVM);
	}

	// 1-3. 이미 열린 ViewModel(IsOpen == true)을 위젯에 지연 연결 시 View 프레젠테이션 동기화 검증
	const FName OpenedWidgetKey(TEXT("OpenedWidget"));
	FBBMWorldWidgetConfig OpenedWidgetConfig;
	OpenedWidgetConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	UBBMUserWidgetBase* OpenedWidget = UISS->CreateAndRegisterWidgetByConfig(OpenedWidgetKey, OpenedWidgetConfig);
	TestNotNull(TEXT("지연 연결용 위젯 등록 성공"), OpenedWidget);

	FBBMWorldViewModelConfig DelayedVMConfig;
	DelayedVMConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();
	UBBMTitleViewModel* DelayedTitleVM = Cast<UBBMTitleViewModel>(UISS->CreateAndRegisterViewModelByConfig(OpenedWidgetKey, DelayedVMConfig));
	TestNotNull(TEXT("지연 연결용 ViewModel 등록 성공"), DelayedTitleVM);

	if (OpenedWidget != nullptr && DelayedTitleVM != nullptr)
	{
		// 위젯 연결 전 ViewModel->Open() 호출 시 논리 상태 및 OnOpen/OnFinishOpenAnim이 즉시 반영됨
		OpenedWidget->SetVisibility(ESlateVisibility::Collapsed);
		DelayedTitleVM->Open();
		TestTrue(TEXT("ViewModel Open 후 IsOpen은 true"), DelayedTitleVM->IsOpen());
		TestEqual(TEXT("헤드리스 Open 후 OpenState는 Started"), DelayedTitleVM->GetOpenState(), EBBMDualAnimState::Started);
		TestEqual(TEXT("위젯 연결 전에는 아직 숨김 상태 유지"), OpenedWidget->GetVisibility(), ESlateVisibility::Collapsed);

		// 이미 열린 ViewModel에 위젯을 뒤늦게 연결하면 즉시 프레젠테이션 가시성이 동기화됨
		OpenedWidget->SetViewModelInstance(DelayedTitleVM);
		TestEqual(TEXT("지연 연결 후 위젯 가시성이 SelfHitTestInvisible로 전환되어야 함"), OpenedWidget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

		// ViewModel->Close() 호출 시 논리 상태와 View 프레젠테이션이 모두 닫힘
		DelayedTitleVM->Close();
		TestFalse(TEXT("ViewModel Close 후 IsOpen은 false"), DelayedTitleVM->IsOpen());
		TestEqual(TEXT("Close 후 OpenState는 Ended"), DelayedTitleVM->GetOpenState(), EBBMDualAnimState::Ended);
		TestEqual(TEXT("Close 후 위젯 가시성이 Collapsed로 전환되어야 함"), OpenedWidget->GetVisibility(), ESlateVisibility::Collapsed);

		UISS->RemoveUI(OpenedWidgetKey);
	}

	// 1-4. RemoveUI 호출 시 위젯과 ViewModel 쌍이 모두 정리되는지 검증
	TestTrue(TEXT("TitleMenu RemoveUI 호출 성공"), UISS->RemoveUI(TitleWidgetKey));
	TestNull(TEXT("RemoveUI 후 GetWidget은 nullptr"), UISS->GetWidget(TitleWidgetKey));
	TestNull(TEXT("RemoveUI 후 GetViewModel도 nullptr"), UISS->GetViewModel(TitleWidgetKey));

	return true;
}

// 2. UBBMTitleViewModel의 세이브 상태 조회 및 이어하기 가시성(Hidden/Visible) 제어 검증
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

	// 2-1. 초기 상태: 활성 런 세이브가 없을 때 -> ContinueVisibility == Hidden, bCanContinue == false
	GameDataSS->EnsureSaveDataLoaded();
	UBBMRunSaveGame* RunData = GameDataSS->GetCurrentRunData();
	if (RunData != nullptr)
	{
		RunData->bHasActiveRun = false;
	}

	TitleVM->RefreshTitleState();
	TestFalse(TEXT("활성 런 없을 시 bCanContinue는 false"), TitleVM->GetCanContinue());
	TestEqual(TEXT("활성 런 없을 시 현재 조우 인덱스는 0"), TitleVM->GetActiveRunEncounterIndex(), 0);

	// 2-2. 활성 런 세이브가 존재할 때 -> ContinueVisibility == Visible, bCanContinue == true
	if (RunData != nullptr)
	{
		RunData->bHasActiveRun = true;
		RunData->CurrentEncounterIndex = 3;
	}

	TitleVM->RefreshTitleState();
	TestTrue(TEXT("활성 런 존재 시 bCanContinue는 true"), TitleVM->GetCanContinue());
	TestEqual(TEXT("활성 런 존재 시 현재 조우 인덱스는 3"), TitleVM->GetActiveRunEncounterIndex(), 3);

	return true;
}

// 3. UBBMTitleViewModel Shining/Blocking 전환 모듈 구성 및 UBBMTitleWidget 설정·바인딩·정리 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMTitleViewModelTransitionsTest,
	"DIS.BBM.UI.TitleViewModelTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMTitleViewModelTransitionsTest::RunTest(const FString& Parameters)
{
	FBBMMVVMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드 유효 검증"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMTitleWidget* TitleWidget = CreateWidget<UBBMTitleWidget>(TestWorld.World, UBBMTitleWidget::StaticClass());
	UBBMTitleViewModel* TitleVM = NewObject<UBBMTitleViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMTitleWidget 생성 성공"), TitleWidget);
	TestNotNull(TEXT("UBBMTitleViewModel 생성 성공"), TitleVM);
	if (TitleWidget == nullptr || TitleVM == nullptr)
	{
		return false;
	}

	UBBMSingleAnimTransition* ShiningTransition = TitleVM->GetShiningTransition();
	UBBMDualAnimTransition* BlockingTransition = TitleVM->GetBlockingTransition();
	TestNotNull(TEXT("ShiningTransition 서브오브젝트 유효"), ShiningTransition);
	TestNotNull(TEXT("BlockingTransition 서브오브젝트 유효"), BlockingTransition);
	if (ShiningTransition == nullptr || BlockingTransition == nullptr)
	{
		return false;
	}

	// 3-1. UBBMTitleWidget의 Shining(무한 반복) 및 Blocking 애니메이션 기본 설정 검증
	FStructProperty* ShiningConfigProp = FindFProperty<FStructProperty>(UBBMTitleWidget::StaticClass(), TEXT("ShiningAnimConfig"));
	FStructProperty* BlockingStartProp = FindFProperty<FStructProperty>(UBBMTitleWidget::StaticClass(), TEXT("BlockingStartAnimConfig"));
	FStructProperty* BlockingEndProp = FindFProperty<FStructProperty>(UBBMTitleWidget::StaticClass(), TEXT("BlockingEndAnimConfig"));
	TestNotNull(TEXT("ShiningAnimConfig 프로퍼티 조회 성공"), ShiningConfigProp);
	TestNotNull(TEXT("BlockingStartAnimConfig 프로퍼티 조회 성공"), BlockingStartProp);
	TestNotNull(TEXT("BlockingEndAnimConfig 프로퍼티 조회 성공"), BlockingEndProp);
	if (ShiningConfigProp != nullptr && BlockingStartProp != nullptr && BlockingEndProp != nullptr)
	{
		const FBBMWidgetAnimConfig* ShiningConfig = ShiningConfigProp->ContainerPtrToValuePtr<FBBMWidgetAnimConfig>(TitleWidget);
		const FBBMWidgetAnimConfig* BlockingStartConfig = BlockingStartProp->ContainerPtrToValuePtr<FBBMWidgetAnimConfig>(TitleWidget);
		const FBBMWidgetAnimConfig* BlockingEndConfig = BlockingEndProp->ContainerPtrToValuePtr<FBBMWidgetAnimConfig>(TitleWidget);
		TestEqual(TEXT("ShiningAnimConfig 기본 PlayMode는 Forward여야 함"), ShiningConfig->PlayMode, EBBMWidgetAnimPlayMode::Forward);
		TestEqual(TEXT("ShiningAnimConfig 기본 LoopCount는 무한 반복(0)이어야 함"), ShiningConfig->LoopCount, 0);
		TestEqual(TEXT("BlockingStartAnimConfig 기본 PlayMode는 RelativeForward여야 함"), BlockingStartConfig->PlayMode, EBBMWidgetAnimPlayMode::RelativeForward);
		TestEqual(TEXT("BlockingEndAnimConfig 기본 PlayMode는 RelativeReverse여야 함"), BlockingEndConfig->PlayMode, EBBMWidgetAnimPlayMode::RelativeReverse);
	}

	// 3-2. ViewModel 주입 시 Shining/Blocking 핸들러 바인딩 및 Open/Close 수명 주기 연동 검증
	TitleWidget->SetViewModelInstance(TitleVM);
	TestTrue(TEXT("SetViewModelInstance 후 ShiningTransition 핸들러가 바인딩되어야 함"), ShiningTransition->IsPresentationHandlerBound());

	int32 ShiningStartCount = 0;
	int32 ShiningFinishCount = 0;
	ShiningTransition->OnTransitionStartedNative.AddLambda([&ShiningStartCount](bool bWithAnim)
	{
		if (bWithAnim == true)
		{
			++ShiningStartCount;
		}
	});
	ShiningTransition->OnFinishedBeforeCallbacksNative.AddLambda([&ShiningFinishCount]()
	{
		++ShiningFinishCount;
	});

	// Open() 호출 시 OnOpen()에서 ShiningTransition->Execute(true) 호출 및 헤드리스 환경 안전 완료 검증
	TitleVM->Open();
	TestEqual(TEXT("Open() 시 OnOpen()에서 ShiningTransition 시작 호출이 발생해야 함"), ShiningStartCount, 1);
	TestEqual(TEXT("헤드리스 뷰포트 없는 환경에서는 보류된 Shining 요청이 고착되지 않고 Idle로 정리되어야 함"), ShiningTransition->GetState(), EBBMSingleAnimState::Idle);

	// BlockingTransition 실행 후 Close() 시 OnFinishCloseAnim()에서 정리되는지 검증
	BlockingTransition->Start();
	TestEqual(TEXT("BlockingTransition Start 후 Started 상태"), BlockingTransition->GetState(), EBBMDualAnimState::Started);

	TitleVM->Close();
	TestEqual(TEXT("Close() 완료 후 ShiningTransition은 Idle 상태 유지"), ShiningTransition->GetState(), EBBMSingleAnimState::Idle);
	TestTrue(TEXT("OnFinishCloseAnim() 내 Execute(false)로 완료 통지가 정상 수행되어야 함"), ShiningFinishCount >= 1);

	// 3-3. ViewModel 해제 시 핸들러 언바인딩 검증
	TitleWidget->SetViewModelInstance(nullptr);
	TestFalse(TEXT("SetViewModelInstance(nullptr) 후 ShiningTransition 핸들러가 해제되어야 함"), ShiningTransition->IsPresentationHandlerBound());

	return true;
}

/* 4. 환경설정 및 크레딧 위젯 껍데기 정책/ZOrder 및 오디오 설정 테스트 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMSettingsAndCreditsWidgetsTest,
	"DIS.BBM.UI.SettingsAndCreditsWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FBBMSettingsAndCreditsWidgetsTest::RunTest(const FString& Parameters)
{
	FBBMMVVMScopedTestWorld TestWorld;
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMSettingsWidget* SettingsWidget = CreateWidget<UBBMSettingsWidget>(TestWorld.World, UBBMSettingsWidget::StaticClass());
	UBBMCreditsWidget* CreditsWidget = CreateWidget<UBBMCreditsWidget>(TestWorld.World, UBBMCreditsWidget::StaticClass());
	TestNotNull(TEXT("UBBMSettingsWidget 생성 성공"), SettingsWidget);
	TestNotNull(TEXT("UBBMCreditsWidget 생성 성공"), CreditsWidget);
	if (SettingsWidget == nullptr || CreditsWidget == nullptr)
	{
		return false;
	}

	// PersistentViewport 및 Popup(30) 레이어 확인
	TestEqual(TEXT("SettingsWidget 수명 주기 정책은 PersistentViewport여야 함"), SettingsWidget->GetLifecyclePolicy(), EBBMWidgetLifecyclePolicy::PersistentViewport);
	TestEqual(TEXT("SettingsWidget 기본 ZOrder는 Popup이어야 함"), SettingsWidget->GetDefaultZOrderLayer(), EBBMWidgetZOrder::Popup);
	TestEqual(TEXT("CreditsWidget 수명 주기 정책은 PersistentViewport여야 함"), CreditsWidget->GetLifecyclePolicy(), EBBMWidgetLifecyclePolicy::PersistentViewport);
	TestEqual(TEXT("CreditsWidget 기본 ZOrder는 Popup이어야 함"), CreditsWidget->GetDefaultZOrderLayer(), EBBMWidgetZOrder::Popup);

	// ViewModel 바인딩 및 타입 접근자 검증
	UBBMSettingsViewModel* SettingsVM = NewObject<UBBMSettingsViewModel>(TestWorld.World);
	UBBMCreditsViewModel* CreditsVM = NewObject<UBBMCreditsViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMSettingsViewModel 생성 성공"), SettingsVM);
	TestNotNull(TEXT("UBBMCreditsViewModel 생성 성공"), CreditsVM);

	SettingsWidget->SetViewModelInstance(SettingsVM);
	CreditsWidget->SetViewModelInstance(CreditsVM);
	TestTrue(TEXT("GetSettingsViewModel()은 바인딩된 SettingsViewModel을 반환해야 함"), SettingsWidget->GetSettingsViewModel() == SettingsVM);
	TestTrue(TEXT("GetCreditsViewModel()은 바인딩된 CreditsViewModel을 반환해야 함"), CreditsWidget->GetCreditsViewModel() == CreditsVM);

	// UBBMAudioSettings 기본 객체 접근 검증
	const UBBMAudioSettings* AudioSettings = GetDefault<UBBMAudioSettings>();
	TestNotNull(TEXT("UBBMAudioSettings CDO 조회 성공"), AudioSettings);

	return true;
}

/* 5. 환경설정 ViewModel <-> GameDataSubsystem 동기화, 즉각 적용, Apply/Cancel/Reset 및 Init 복원 테스트 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMSettingsViewModelAndSubsystemTest,
	"DIS.BBM.UI.SettingsViewModelAndSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FBBMSettingsViewModelAndSubsystemTest::RunTest(const FString& Parameters)
{
	// 기존 유저 세이브 파일(.sav/.bak) 백업 후 테스트 종료 시 복구
	const FString SavPath = UBBMGameDataSubsystem::GetSaveFilePath(UBBMUserSaveGame::SlotName);
	const FString BakPath = UBBMGameDataSubsystem::GetBackupFilePath(UBBMUserSaveGame::SlotName);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	const bool bHadSav = PlatformFile.FileExists(*SavPath);
	const bool bHadBak = PlatformFile.FileExists(*BakPath);
	TArray<uint8> PrevSavBytes;
	TArray<uint8> PrevBakBytes;
	if (bHadSav == true)
	{
		FFileHelper::LoadFileToArray(PrevSavBytes, *SavPath);
	}
	if (bHadBak == true)
	{
		FFileHelper::LoadFileToArray(PrevBakBytes, *BakPath);
	}

	// 테스트 전 기존 세이브가 Init에 간섭하지 않도록 임시 제거
	if (bHadSav == true)
	{
		PlatformFile.DeleteFile(*SavPath);
	}
	if (bHadBak == true)
	{
		PlatformFile.DeleteFile(*BakPath);
	}

	// 엔진 UGameUserSettings 이전 상태 보존 (비-저장 인메모리 복원)
	UGameUserSettings* EngineSettings = (GEngine != nullptr) ? GEngine->GetGameUserSettings() : nullptr;
	const bool bPrevVSync = (EngineSettings != nullptr) ? EngineSettings->IsVSyncEnabled() : false;
	const EWindowMode::Type PrevWindowMode = (EngineSettings != nullptr) ? EngineSettings->GetFullscreenMode() : EWindowMode::WindowedFullscreen;
	const FIntPoint PrevResolution = (EngineSettings != nullptr) ? EngineSettings->GetScreenResolution() : FIntPoint(1920, 1080);

	ON_SCOPE_EXIT
	{
		if (EngineSettings != nullptr)
		{
			EngineSettings->SetVSyncEnabled(bPrevVSync);
#if PLATFORM_DESKTOP
			EngineSettings->SetFullscreenMode(PrevWindowMode);
			EngineSettings->SetScreenResolution(PrevResolution);
#endif
		}

		if (bHadSav == true)
		{
			FFileHelper::SaveArrayToFile(PrevSavBytes, *SavPath);
		}
		else if (PlatformFile.FileExists(*SavPath) == true)
		{
			PlatformFile.DeleteFile(*SavPath);
		}

		if (bHadBak == true)
		{
			FFileHelper::SaveArrayToFile(PrevBakBytes, *BakPath);
		}
		else if (PlatformFile.FileExists(*BakPath) == true)
		{
			PlatformFile.DeleteFile(*BakPath);
		}
	};

	FBBMMVVMScopedTestWorld TestWorld;
	if (TestWorld.GameInstance == nullptr)
	{
		return false;
	}

	UBBMGameDataSubsystem* GameDataSS = TestWorld.GameInstance->GetSubsystem<UBBMGameDataSubsystem>();
	TestNotNull(TEXT("UBBMGameDataSubsystem 조회 성공"), GameDataSS);
	if (GameDataSS == nullptr)
	{
		return false;
	}

	// 5-1. FBBMUserSettingsData / UBBMUserSaveGame 직렬화 구조 및 클램핑 검증
	{
		FBBMUserSettingsData ClampedTest;
		ClampedTest.MasterVolume = 2.5f;
		ClampedTest.DialogueVolume = -0.3f;
		ClampedTest.ScreenResolution = FIntPoint(100, 50);
		ClampedTest.ClampValues();
		TestEqual(TEXT("MasterVolume 상한 클램핑 (1.0)"), ClampedTest.MasterVolume, 1.0f);
		TestEqual(TEXT("DialogueVolume 하한 클램핑 (0.0)"), ClampedTest.DialogueVolume, 0.0f);
		TestEqual(TEXT("ScreenResolution 최소 폭 클램핑 (320)"), ClampedTest.ScreenResolution.X, 320);
		TestEqual(TEXT("ScreenResolution 최소 높이 클램핑 (240)"), ClampedTest.ScreenResolution.Y, 240);

		UBBMUserSaveGame* TempSave = NewObject<UBBMUserSaveGame>();
		FBBMUserSettingsData CustomSnapshot;
		CustomSnapshot.MasterVolume = 0.45f;
		CustomSnapshot.BGMVolume = 0.35f;
		CustomSnapshot.SFXVolume = 0.65f;
		CustomSnapshot.DialogueVolume = 0.55f;
		CustomSnapshot.bEnableBeatVisualAssist = false;
		CustomSnapshot.WindowMode = EWindowMode::Windowed;
		CustomSnapshot.ScreenResolution = FIntPoint(1600, 900);
		CustomSnapshot.bVSyncEnabled = true;
		TempSave->ApplySettingsData(CustomSnapshot);

		TestTrue(TEXT("UBBMUserSaveGame 왕복 스냅샷 일치"), TempSave->ToSettingsData() == CustomSnapshot);
	}

	// 5-2. UBBMGameDataSubsystem::Initialize() 직후 초기 설정 복원 검증
	const FBBMUserSettingsData DefaultSnapshot;
	TestTrue(TEXT("초기화 직후 GameDataSubsystem 설정은 기본값과 일치해야 함"), GameDataSS->GetUserSettingsData() == DefaultSnapshot);
	if (EngineSettings != nullptr)
	{
		TestEqual(TEXT("초기화 복원으로 UGameUserSettings VSync가 기본값과 일치해야 함"), EngineSettings->IsVSyncEnabled(), DefaultSnapshot.bVSyncEnabled);
#if PLATFORM_DESKTOP
		TestTrue(TEXT("초기화 복원으로 UGameUserSettings 창모드가 기본값과 일치해야 함"), EngineSettings->GetFullscreenMode() == DefaultSnapshot.WindowMode.GetValue());
		TestTrue(TEXT("초기화 복원으로 UGameUserSettings 해상도가 기본값과 일치해야 함"), EngineSettings->GetScreenResolution() == DefaultSnapshot.ScreenResolution);
#endif
	}

	// 5-3. UBBMSettingsViewModel 진입 캐시 -> 즉각 적용 -> 대리자 통지 -> Cancel 롤백 검증
	UBBMSettingsViewModel* SettingsVM = NewObject<UBBMSettingsViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMSettingsViewModel 생성 성공"), SettingsVM);
	if (SettingsVM == nullptr)
	{
		return false;
	}

	int32 SettingsChangedCount = 0;
	FBBMUserSettingsData LastBroadcastedSettings;
	const FDelegateHandle DelegateHandle = GameDataSS->OnUserSettingsChanged.AddLambda([&SettingsChangedCount, &LastBroadcastedSettings](const FBBMUserSettingsData& NewSettings)
	{
		++SettingsChangedCount;
		LastBroadcastedSettings = NewSettings;
	});
	ON_SCOPE_EXIT
	{
		GameDataSS->OnUserSettingsChanged.Remove(DelegateHandle);
	};

	// bHasPendingEdit FieldNotify 브로드캐스트 추적
	int32 PendingEditNotifyCount = 0;
	const FDelegateHandle PendingEditDelegateHandle = SettingsVM->AddFieldValueChangedDelegate(
		UBBMSettingsViewModel::FFieldNotificationClassDescriptor::bHasPendingEdit,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&PendingEditNotifyCount](UObject* /*Object*/, UE::FieldNotification::FFieldId /*FieldId*/)
		{
			++PendingEditNotifyCount;
		})
	);
	ON_SCOPE_EXIT
	{
		SettingsVM->RemoveFieldValueChangedDelegate(
			UBBMSettingsViewModel::FFieldNotificationClassDescriptor::bHasPendingEdit,
			PendingEditDelegateHandle
		);
	};

	// 변경 없이 Open -> Cancel/Close 시 불필요한 런타임 재적용(브로드캐스트)이 발생하지 않는지 검증
	const int32 CountBeforeIdleOpenClose = SettingsChangedCount;
	SettingsVM->Open();
	TestFalse(TEXT("Open() 직후 bHasPendingEdit은 false여야 함"), SettingsVM->GetHasPendingEdit());
	SettingsVM->CancelSettings();
	SettingsVM->Close();
	TestEqual(TEXT("변경 없이 Open/Cancel/Close 시 불필요한 OnUserSettingsChanged 호출이 없어야 함"), SettingsChangedCount, CountBeforeIdleOpenClose);

	// 값 변경 후 원복 시 bHasPendingEdit이 FieldNotify와 함께 true -> false로 자동 복귀하는지 검증
	SettingsVM->Open();
	SettingsVM->SetMasterVolume(0.3f);
	TestTrue(TEXT("값 변경 직후 bHasPendingEdit은 true여야 함"), SettingsVM->GetHasPendingEdit());
	TestTrue(TEXT("값 변경 직후 bHasPendingEdit FieldNotify가 호출되어야 함"), PendingEditNotifyCount >= 1);
	SettingsVM->SetMasterVolume(DefaultSnapshot.MasterVolume);
	TestFalse(TEXT("원래 값으로 되돌리면 bHasPendingEdit은 false로 자동 복귀해야 함"), SettingsVM->GetHasPendingEdit());

	SettingsVM->SetMasterVolume(0.3f);
	SettingsVM->SetBGMVolume(0.4f);
	SettingsVM->SetSFXVolume(0.5f);
	SettingsVM->SetDialogueVolume(0.25f);
	SettingsVM->SetEnableBeatVisualAssist(false);
	SettingsVM->SetWindowMode(EWindowMode::Windowed);
	SettingsVM->SetScreenResolution(FIntPoint(1280, 720));
	SettingsVM->SetVSyncEnabled(true);

	TestTrue(TEXT("즉각 설정 변경 시 OnUserSettingsChanged 대리자가 호출되어야 함"), SettingsChangedCount >= 8);
	TestEqual(TEXT("ViewModel MasterVolume 즉각 반영"), GameDataSS->GetUserSettingsData().MasterVolume, 0.3f);
	TestEqual(TEXT("ViewModel DialogueVolume 즉각 반영"), GameDataSS->GetUserSettingsData().DialogueVolume, 0.25f);
	TestEqual(TEXT("ViewModel bEnableBeatVisualAssist 즉각 반영"), GameDataSS->GetUserSettingsData().bEnableBeatVisualAssist, false);
	if (EngineSettings != nullptr)
	{
		TestTrue(TEXT("VSync 즉각 UGameUserSettings 반영"), EngineSettings->IsVSyncEnabled());
#if PLATFORM_DESKTOP
		TestTrue(TEXT("창모드 즉각 UGameUserSettings 반영"), EngineSettings->GetFullscreenMode() == EWindowMode::Windowed);
		TestTrue(TEXT("해상도 즉각 UGameUserSettings 반영"), EngineSettings->GetScreenResolution() == FIntPoint(1280, 720));
#endif
	}

	// CancelSettings 호출 시 진입 전 캐싱된 기본값으로 복원 (디스크 저장 없음)
	SettingsVM->CancelSettings();
	TestTrue(TEXT("Cancel 후 Subsystem 설정이 진입 전 값으로 복원되어야 함"), GameDataSS->GetUserSettingsData() == DefaultSnapshot);
	TestEqual(TEXT("Cancel 후 ViewModel MasterVolume 복원"), SettingsVM->GetMasterVolume(), DefaultSnapshot.MasterVolume);
	TestEqual(TEXT("Cancel 후 ViewModel DialogueVolume 복원"), SettingsVM->GetDialogueVolume(), DefaultSnapshot.DialogueVolume);
	TestEqual(TEXT("Cancel 후 ViewModel bEnableBeatVisualAssist 복원"), SettingsVM->GetEnableBeatVisualAssist(), DefaultSnapshot.bEnableBeatVisualAssist);
	TestFalse(TEXT("Cancel만으로는 유저 세이브 .sav 파일이 디스크에 쓰이지 않아야 함"), PlatformFile.FileExists(*SavPath));
	SettingsVM->Close();

	// 5-4. Apply 미호출 상태로 Close() 호출 시 자동 롤백 검증
	SettingsVM->Open();
	SettingsVM->SetBGMVolume(0.15f);
	TestEqual(TEXT("편집 중 BGMVolume 즉각 반영"), GameDataSS->GetUserSettingsData().BGMVolume, 0.15f);
	SettingsVM->Close();
	TestEqual(TEXT("Apply/Cancel 없이 Close() 시 진입 전 BGMVolume으로 자동 롤백되어야 함"), GameDataSS->GetUserSettingsData().BGMVolume, DefaultSnapshot.BGMVolume);

	// 5-5. ApplySettings 확정 저장 후 Cancel 비-롤백 + Apply 후 재편집 Cancel 롤백 + ResetToDefaults 검증
	SettingsVM->Open();
	SettingsVM->SetDialogueVolume(0.6f);
	SettingsVM->SetEnableBeatVisualAssist(false);
	SettingsVM->ApplySettings();

	// 비동기 저장 파이프라인 완료 대기 (최대 2초)
	const double WaitStartTime = FPlatformTime::Seconds();
	while (GameDataSS->IsAsyncOperationInProgress() == true && (FPlatformTime::Seconds() - WaitStartTime) < 2.0)
	{
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FPlatformProcess::Sleep(0.01f);
	}
	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);

	TestTrue(TEXT("ApplySettings() 호출 후 유저 세이브 .sav 파일이 생성되어야 함"), PlatformFile.FileExists(*SavPath));

	// Apply 이후 추가 편집 없이 CancelSettings를 호출해도 방금 커밋한 값이 유지되어야 함
	TestFalse(TEXT("ApplySettings() 직후 bHasPendingEdit은 false여야 함"), SettingsVM->GetHasPendingEdit());
	SettingsVM->CancelSettings();
	TestEqual(TEXT("Apply 직후 CancelSettings()는 커밋된 DialogueVolume을 유지해야 함"), GameDataSS->GetUserSettingsData().DialogueVolume, 0.6f);
	TestEqual(TEXT("Apply 직후 CancelSettings()는 커밋된 BeatVisualAssist를 유지해야 함"), GameDataSS->GetUserSettingsData().bEnableBeatVisualAssist, false);

	// Apply 이후 패널이 열린 상태에서 추가 수정 뒤 Cancel 시 직전 Apply 커밋값(0.6)으로 롤백되어야 함
	SettingsVM->SetDialogueVolume(0.1f);
	TestTrue(TEXT("Apply 이후 추가 편집 시 bHasPendingEdit은 다시 true가 되어야 함"), SettingsVM->GetHasPendingEdit());
	TestEqual(TEXT("Apply 후 추가 편집 DialogueVolume 즉각 반영"), GameDataSS->GetUserSettingsData().DialogueVolume, 0.1f);
	SettingsVM->CancelSettings();
	TestFalse(TEXT("Cancel 이후 bHasPendingEdit은 false여야 함"), SettingsVM->GetHasPendingEdit());
	TestEqual(TEXT("Apply 후 재편집 Cancel 시 직전 Apply 값(0.6)으로 롤백되어야 함"), GameDataSS->GetUserSettingsData().DialogueVolume, 0.6f);

	// 커스텀 상태(DialogueVolume=0.6)가 기준일 때 ResetToDefaults 호출 시 bHasPendingEdit이 true로 전환되는지 검증
	SettingsVM->ResetToDefaults();
	TestTrue(TEXT("커스텀 기준 상태에서 ResetToDefaults 호출 시 bHasPendingEdit은 true여야 함"), SettingsVM->GetHasPendingEdit());
	SettingsVM->CancelSettings();
	TestEqual(TEXT("ResetToDefaults 후 Cancel 시 직전 Apply 값(0.6)으로 롤백되어야 함"), GameDataSS->GetUserSettingsData().DialogueVolume, 0.6f);
	SettingsVM->Close();

	// ResetToDefaults 호출 시 GameDataSubsystem 기본값 복원 및 ViewModel 대리자 동기화 검증
	SettingsVM->ResetToDefaults();
	TestTrue(TEXT("ResetToDefaults 후 Subsystem 설정이 기본값과 일치해야 함"), GameDataSS->GetUserSettingsData() == DefaultSnapshot);
	TestEqual(TEXT("ResetToDefaults 후 ViewModel DialogueVolume이 대리자를 통해 기본값으로 동기화되어야 함"), SettingsVM->GetDialogueVolume(), DefaultSnapshot.DialogueVolume);
	TestEqual(TEXT("ResetToDefaults 후 ViewModel BeatVisualAssist가 대리자를 통해 기본값으로 동기화되어야 함"), SettingsVM->GetEnableBeatVisualAssist(), DefaultSnapshot.bEnableBeatVisualAssist);

	// 5-6. Apply된 .sav가 존재하는 상태에서 새 Subsystem 초기화 시 자동 초기 복원 검증
	{
		FBBMMVVMScopedTestWorld SecondWorld;
		UBBMGameDataSubsystem* SecondSS = (SecondWorld.GameInstance != nullptr) ? SecondWorld.GameInstance->GetSubsystem<UBBMGameDataSubsystem>() : nullptr;
		TestNotNull(TEXT("두 번째 테스트 월드 GameDataSubsystem 조회 성공"), SecondSS);
		if (SecondSS != nullptr)
		{
			TestEqual(TEXT("Initialize() 시 디스크에 저장된 DialogueVolume(0.6)이 자동 로드·복원되어야 함"), SecondSS->GetUserSettingsData().DialogueVolume, 0.6f);
			TestEqual(TEXT("Initialize() 시 디스크에 저장된 BeatVisualAssist(false)가 자동 로드·복원되어야 함"), SecondSS->GetUserSettingsData().bEnableBeatVisualAssist, false);
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

