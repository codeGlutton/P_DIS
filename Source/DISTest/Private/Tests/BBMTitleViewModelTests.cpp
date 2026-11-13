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
#include "UI/BBMWorldUISubsystem.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "UI/ViewModel/BBMSingleAnimTransition.h"
#include "UI/ViewModel/BBMDualAnimTransition.h"
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
	TestEqual(TEXT("Close() 완료 후 BlockingTransition은 Ended로 정리되어야 함"), BlockingTransition->GetState(), EBBMDualAnimState::Ended);
	TestEqual(TEXT("Close() 완료 후 ShiningTransition은 Idle 상태 유지"), ShiningTransition->GetState(), EBBMSingleAnimState::Idle);
	TestTrue(TEXT("OnFinishCloseAnim() 내 Execute(false)로 완료 통지가 정상 수행되어야 함"), ShiningFinishCount >= 1);

	// 3-3. ViewModel 해제 시 핸들러 언바인딩 검증
	TitleWidget->SetViewModelInstance(nullptr);
	TestFalse(TEXT("SetViewModelInstance(nullptr) 후 ShiningTransition 핸들러가 해제되어야 함"), ShiningTransition->IsPresentationHandlerBound());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

