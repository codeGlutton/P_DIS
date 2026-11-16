/*****************************************************************//**
 * @file   BBMTitleViewModelTests.cpp
 * @brief  BBM(비트 배낭 상인) 타이틀 MVVM 뷰모델 및 월드 위젯 서브시스템 연동 단위 테스트
 * @details ViewModel 수명 주기, 세이브 유무에 따른 이어하기 버튼 Hidden 가시성 제어 및 네비게이션 액션 검증
 * @date   2026-11-07
 *********************************************************************/

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "WorldUITypes.h"
#include "UI/BBMUITypes.h"
#include "Widget/WorldUserWidgetBase.h"
#include "UI/Widget/BBMTitleWidget.h"
#include "UI/Widget/BBMSettingsWidget.h"
#include "UI/Widget/BBMTabEntryWidget.h"
#include "UI/Widget/BBMSettingsTabEntryWidget.h"
#include "UI/Widget/BBMCreditsWidget.h"
#include "WorldUISubsystem.h"
#include "ViewModel/WorldViewModelBase.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "UI/ViewModel/BBMSettingsViewModel.h"
#include "UI/ViewModel/BBMCharacterSelectViewModel.h"
#include "UI/ViewModel/BBMCreditsViewModel.h"
#include "UI/ViewModel/BBMCreditEntryViewModel.h"
#include "UI/ViewModel/BBMTabEntryViewModel.h"
#include "UI/ViewModel/BBMTabViewModelBase.h"
#include "ViewModel/SingleAnimTransition.h"
#include "ViewModel/DualAnimTransition.h"
#include "Global/BBMGlobalSettings.h"
#include "Save/BBMGameDataSubsystem.h"
#include "Save/BBMRunSaveGame.h"
#include "Save/BBMUserSaveGame.h"
#include "Save/BBMSaveTypes.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Async/TaskGraphInterfaces.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

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

namespace
{
	/**
	 * TitleViewModel 필수 서브 뷰모델 3종을 생성·초기화하여 TitleVM에 주입하는 테스트 헬퍼
	 */
	void AssignDefaultSubViewModels(UObject* Outer, UBBMTitleViewModel* TitleVM)
	{
		if (Outer == nullptr || TitleVM == nullptr)
		{
			return;
		}

		UBBMSettingsViewModel* SettingsVM = NewObject<UBBMSettingsViewModel>(Outer);
		UBBMCharacterSelectViewModel* CharacterSelectVM = NewObject<UBBMCharacterSelectViewModel>(Outer);
		UBBMCreditsViewModel* CreditsVM = NewObject<UBBMCreditsViewModel>(Outer);

		SettingsVM->FinishInitialization();
		CharacterSelectVM->FinishInitialization();
		CreditsVM->FinishInitialization();

		TitleVM->SetSettingsViewModel(SettingsVM);
		TitleVM->SetCharacterSelectViewModel(CharacterSelectVM);
		TitleVM->SetCreditsViewModel(CreditsVM);
	}
}

// 1. UWorldUISubsystem의 독립 위젯/ViewModel 생성 및 위젯 수동 연결 검증
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

	UWorldUISubsystem* UISS = TestWorld.World->GetSubsystem<UWorldUISubsystem>();
	TestNotNull(TEXT("UWorldUISubsystem 획득 검증"), UISS);
	if (UISS == nullptr)
	{
		return false;
	}

	// 1-1. 순수 위젯만 등록 시 ViewModel은 생성/연결되지 않아야 함
	const FName PlainWidgetKey(TEXT("PlainWidget"));
	FWorldWidgetConfig PlainConfig;
	PlainConfig.WidgetClass = UWorldUserWidgetBase::StaticClass();
	UWorldUserWidgetBase* PlainWidget = UISS->CreateAndRegisterWidgetByConfig(PlainWidgetKey, PlainConfig);
	TestNotNull(TEXT("순수 위젯 등록 성공"), PlainWidget);
	if (PlainWidget != nullptr)
	{
		TestNull(TEXT("서브시스템 GetViewModel은 nullptr"), UISS->GetViewModel(PlainWidgetKey));
		TestNull(TEXT("위젯 GetViewModel은 nullptr"), PlainWidget->GetViewModel());
	}

	// 1-2. 위젯과 ViewModel 별도 생성 후 수동 바인딩 주입 검증
	const FName TitleWidgetKey(TEXT("TitleMenu"));
	FWorldWidgetConfig TitleWidgetConfig;
	TitleWidgetConfig.WidgetClass = UBBMTitleWidget::StaticClass();
	UWorldUserWidgetBase* TitleUserWidget = UISS->CreateAndRegisterWidgetByConfig(TitleWidgetKey, TitleWidgetConfig);
	TestNotNull(TEXT("타이틀 위젯 등록 성공"), TitleUserWidget);

	FWorldViewModelConfig TitleVMConfig;
	TitleVMConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();
	UWorldViewModelBase* RegisteredVM = UISS->CreateAndRegisterViewModelByConfig(TitleWidgetKey, TitleVMConfig);
	TestNotNull(TEXT("서브시스템을 통한 ViewModel 독립 생성 및 등록 검증"), RegisteredVM);

	UBBMTitleWidget* TitleWidget = Cast<UBBMTitleWidget>(TitleUserWidget);
	TestNotNull(TEXT("UBBMTitleWidget 캐스팅 성공"), TitleWidget);
	UBBMTitleViewModel* TitleVM = UISS->GetViewModelAs<UBBMTitleViewModel>(TitleWidgetKey);
	TestNotNull(TEXT("서브시스템의 GetViewModelAs 템플릿 조회 성공"), TitleVM);

	if (TitleWidget != nullptr && TitleVM != nullptr)
	{
		AssignDefaultSubViewModels(TestWorld.World, TitleVM);
		TitleVM->FinishInitialization();

		// 수동 연결 전에는 위젯 내부 단일 ViewModel이 비어 있어야 함
		TestNull(TEXT("수동 연결 전 GetViewModel은 nullptr"), TitleWidget->GetViewModel());
		TestNull(TEXT("수동 연결 전 TitleWidget->GetTitleViewModel은 nullptr"), TitleWidget->GetTitleViewModel());

		// 수동 연결 이후 단일 ViewModel 및 템플릿 캐스팅 조회가 즉시 동작해야 함
		TitleWidget->SetViewModelInstance(TitleVM);
		TestEqual(TEXT("수동 연결 후 GetViewModel 일치"), TitleWidget->GetViewModel(), Cast<UWorldViewModelBase>(TitleVM));
		TestEqual(TEXT("수동 연결 후 GetViewModelAs 일치"), TitleWidget->GetViewModelAs<UBBMTitleViewModel>(), TitleVM);
		TestEqual(TEXT("수동 연결 후 TitleWidget->GetTitleViewModel 일치"), TitleWidget->GetTitleViewModel(), TitleVM);
	}

	// 1-3. 이미 열린 ViewModel(IsOpen == true)을 위젯에 지연 연결 시 View 프레젠테이션 동기화 검증
	const FName OpenedWidgetKey(TEXT("OpenedWidget"));
	FWorldWidgetConfig OpenedWidgetConfig;
	OpenedWidgetConfig.WidgetClass = UWorldUserWidgetBase::StaticClass();
	UWorldUserWidgetBase* OpenedWidget = UISS->CreateAndRegisterWidgetByConfig(OpenedWidgetKey, OpenedWidgetConfig);
	TestNotNull(TEXT("지연 연결용 위젯 등록 성공"), OpenedWidget);

	FWorldViewModelConfig DelayedVMConfig;
	DelayedVMConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();
	UBBMTitleViewModel* DelayedTitleVM = Cast<UBBMTitleViewModel>(UISS->CreateAndRegisterViewModelByConfig(OpenedWidgetKey, DelayedVMConfig));
	TestNotNull(TEXT("지연 연결용 ViewModel 등록 성공"), DelayedTitleVM);

	if (OpenedWidget != nullptr && DelayedTitleVM != nullptr)
	{
		AssignDefaultSubViewModels(TestWorld.World, DelayedTitleVM);
		DelayedTitleVM->FinishInitialization();

		// 위젯 연결 전 ViewModel->Open() 호출 시 논리 상태 및 OnOpen/OnFinishOpenAnim이 즉시 반영됨
		OpenedWidget->SetVisibility(ESlateVisibility::Collapsed);
		DelayedTitleVM->Open();
		TestTrue(TEXT("ViewModel Open 후 IsOpen은 true"), DelayedTitleVM->IsOpen());
		TestEqual(TEXT("헤드리스 Open 후 OpenState는 Started"), DelayedTitleVM->GetOpenState(), EDualAnimState::Started);
		TestEqual(TEXT("위젯 연결 전에는 아직 숨김 상태 유지"), OpenedWidget->GetVisibility(), ESlateVisibility::Collapsed);

		// 이미 열린 ViewModel에 위젯을 뒤늦게 연결하면 즉시 프레젠테이션 가시성이 동기화됨
		OpenedWidget->SetViewModelInstance(DelayedTitleVM);
		TestEqual(TEXT("지연 연결 후 위젯 가시성이 SelfHitTestInvisible로 전환되어야 함"), OpenedWidget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

		// ViewModel->Close() 호출 시 논리 상태와 View 프레젠테이션이 모두 닫힘
		DelayedTitleVM->Close();
		TestFalse(TEXT("ViewModel Close 후 IsOpen은 false"), DelayedTitleVM->IsOpen());
		TestEqual(TEXT("Close 후 OpenState는 Ended"), DelayedTitleVM->GetOpenState(), EDualAnimState::Ended);
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

	// 뷰모델 생성 및 필수 서브 뷰모델 주입
	UBBMTitleViewModel* TitleVM = NewObject<UBBMTitleViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMTitleViewModel 생성 성공"), TitleVM);
	if (TitleVM == nullptr)
	{
		return false;
	}
	AssignDefaultSubViewModels(TestWorld.World, TitleVM);
	TitleVM->FinishInitialization();

	// 2-1. 초기 상태: 활성 런 세이브가 없을 때 -> ContinueVisibility == Hidden, bCanContinue == false
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
	AssignDefaultSubViewModels(TestWorld.World, TitleVM);
	TitleVM->FinishInitialization();

	USingleAnimTransition* ShiningTransition = TitleVM->GetShiningTransition();
	UDualAnimTransition* BlockingTransition = TitleVM->GetBlockingTransition();
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
		const FWorldWidgetAnimConfig* ShiningConfig = ShiningConfigProp->ContainerPtrToValuePtr<FWorldWidgetAnimConfig>(TitleWidget);
		const FWorldWidgetAnimConfig* BlockingStartConfig = BlockingStartProp->ContainerPtrToValuePtr<FWorldWidgetAnimConfig>(TitleWidget);
		const FWorldWidgetAnimConfig* BlockingEndConfig = BlockingEndProp->ContainerPtrToValuePtr<FWorldWidgetAnimConfig>(TitleWidget);
		TestEqual(TEXT("ShiningAnimConfig 기본 PlayMode는 Forward여야 함"), ShiningConfig->PlayMode, EWorldWidgetAnimPlayMode::Forward);
		TestEqual(TEXT("ShiningAnimConfig 기본 LoopCount는 무한 반복(0)이어야 함"), ShiningConfig->LoopCount, 0);
		TestEqual(TEXT("BlockingStartAnimConfig 기본 PlayMode는 RelativeForward여야 함"), BlockingStartConfig->PlayMode, EWorldWidgetAnimPlayMode::RelativeForward);
		TestEqual(TEXT("BlockingEndAnimConfig 기본 PlayMode는 RelativeReverse여야 함"), BlockingEndConfig->PlayMode, EWorldWidgetAnimPlayMode::RelativeReverse);
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
	ShiningTransition->OnFinishedBeforeCallbacksNative.AddLambda([&ShiningFinishCount](bool /*bInterrupted*/)
	{
		++ShiningFinishCount;
	});

	// Open() 호출 시 OnFinishOpenAnim()에서 ShiningTransition->Execute(true) 호출 및 헤드리스 환경 안전 완료 검증
	TitleVM->Open();
	TestEqual(TEXT("Open() 시 OnFinishOpenAnim()에서 ShiningTransition 시작 호출이 발생해야 함"), ShiningStartCount, 1);
	TestEqual(TEXT("헤드리스 애니메이션 미설정 환경에서는 Shining 요청이 고착되지 않고 Idle로 정리되어야 함"), ShiningTransition->GetState(), ESingleAnimState::Idle);

	// BlockingTransition 실행 후 Close() 시 OnClosed()에서 ShiningTransition 정리 검증
	BlockingTransition->Start();
	TestEqual(TEXT("BlockingTransition Start 후 Started 상태"), BlockingTransition->GetState(), EDualAnimState::Started);

	const int32 ShiningFinishCountBeforeClose = ShiningFinishCount;
	TitleVM->Close();
	TestEqual(TEXT("Close() 완료 후 ShiningTransition은 Idle 상태 유지"), ShiningTransition->GetState(), ESingleAnimState::Idle);
	TestTrue(TEXT("OnClosed() 내 Execute(false)로 완료 통지가 추가 수행되어야 함"), ShiningFinishCount > ShiningFinishCountBeforeClose);

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
	TestEqual(TEXT("SettingsWidget 수명 주기 정책은 PersistentViewport여야 함"), SettingsWidget->GetLifecyclePolicy(), EWorldWidgetLifecyclePolicy::PersistentViewport);
	TestEqual(TEXT("SettingsWidget 기본 ZOrder는 Popup이어야 함"), SettingsWidget->GetDefaultZOrderLayer(), EWorldWidgetZOrder::Popup);
	TestEqual(TEXT("CreditsWidget 수명 주기 정책은 PersistentViewport여야 함"), CreditsWidget->GetLifecyclePolicy(), EWorldWidgetLifecyclePolicy::PersistentViewport);
	TestEqual(TEXT("CreditsWidget 기본 ZOrder는 Popup이어야 함"), CreditsWidget->GetDefaultZOrderLayer(), EWorldWidgetZOrder::Popup);

	// ViewModel 바인딩 및 타입 접근자 검증
	UBBMSettingsViewModel* SettingsVM = NewObject<UBBMSettingsViewModel>(TestWorld.World);
	UBBMCreditsViewModel* CreditsVM = NewObject<UBBMCreditsViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMSettingsViewModel 생성 성공"), SettingsVM);
	TestNotNull(TEXT("UBBMCreditsViewModel 생성 성공"), CreditsVM);
	if (SettingsVM == nullptr || CreditsVM == nullptr)
	{
		return false;
	}
	SettingsVM->FinishInitialization();
	CreditsVM->FinishInitialization();

	SettingsWidget->SetViewModelInstance(SettingsVM);
	CreditsWidget->SetViewModelInstance(CreditsVM);
	TestTrue(TEXT("GetSettingsViewModel()은 바인딩된 SettingsViewModel을 반환해야 함"), SettingsWidget->GetSettingsViewModel() == SettingsVM);
	TestTrue(TEXT("GetCreditsViewModel()은 바인딩된 CreditsViewModel을 반환해야 함"), CreditsWidget->GetCreditsViewModel() == CreditsVM);

	// UBBMSettingsViewModel BeginViewModel() 4종 탭 엔트리(Game, Audio, Graphics, Controls) 자동 생성·선택 검증
	TestEqual(TEXT("SettingsVM BeginViewModel 후 TabCount는 4여야 함"), SettingsVM->GetTabCount(), 4);
	TestEqual(TEXT("SettingsVM 초기 SelectedTabIndex는 0이어야 함"), SettingsVM->GetSelectedTabIndex(), 0);
	TestEqual(TEXT("SettingsVM 초기 PreSelectedTabIndex는 INDEX_NONE이어야 함"), SettingsVM->GetPreSelectedTabIndex(), INDEX_NONE);

	const TArray<TObjectPtr<UBBMTabEntryViewModel>>& SettingsTabEntries = SettingsVM->GetTabEntries();
	for (int32 TabIdx = 0; TabIdx < SettingsTabEntries.Num(); ++TabIdx)
	{
		UBBMTabEntryViewModel* Entry = SettingsTabEntries[TabIdx];
		TestNotNull(TEXT("Settings 탭 엔트리 VM 유효"), Entry);
		if (Entry != nullptr)
		{
			TestEqual(TEXT("Settings 탭 엔트리 TabIndex 일치"), Entry->GetTabIndex(), TabIdx);
			TestFalse(TEXT("Settings 탭 엔트리 TabText가 비어있으면 안 됨"), Entry->GetTabText().IsEmpty());
			TestEqual(TEXT("초기 0번 탭만 선택 상태여야 함"), Entry->IsSelected(), TabIdx == 0);
		}
	}

	if (SettingsTabEntries.IsValidIndex(2) == true && SettingsTabEntries[2] != nullptr)
	{
		SettingsTabEntries[2]->RequestSelect();
		TestEqual(TEXT("2번 탭 선택 요청 시 SelectedTabIndex == 2"), SettingsVM->GetSelectedTabIndex(), 2);
		TestEqual(TEXT("2번 탭 선택 요청 시 PreSelectedTabIndex == 0"), SettingsVM->GetPreSelectedTabIndex(), 0);
	}

	// 범용 UBBMTabEntryWidget 및 파생 UBBMSettingsTabEntryWidget 생성·상속·인터페이스 검증
	UBBMTabEntryWidget* TabEntryWidget = CreateWidget<UBBMTabEntryWidget>(TestWorld.World, UBBMTabEntryWidget::StaticClass());
	UBBMSettingsTabEntryWidget* SettingsTabEntryWidget = CreateWidget<UBBMSettingsTabEntryWidget>(TestWorld.World, UBBMSettingsTabEntryWidget::StaticClass());
	TestNotNull(TEXT("UBBMTabEntryWidget 생성 성공"), TabEntryWidget);
	TestNotNull(TEXT("UBBMSettingsTabEntryWidget 생성 성공"), SettingsTabEntryWidget);
	if (SettingsTabEntryWidget != nullptr)
	{
		TestTrue(TEXT("UBBMSettingsTabEntryWidget는 UBBMTabEntryWidget에서 파생되어야 함"), SettingsTabEntryWidget->IsA<UBBMTabEntryWidget>());
		TestTrue(TEXT("UBBMSettingsTabEntryWidget는 IUserObjectListEntry를 구현해야 함"), SettingsTabEntryWidget->Implements<UUserObjectListEntry>());
	}

	// EndViewModel() 호출 시 세팅 탭 엔트리 정리 확인
	SettingsVM->ConditionalBeginDestroy();
	TestEqual(TEXT("SettingsVM EndViewModel 후 TabCount == 0"), SettingsVM->GetTabCount(), 0);
	TestEqual(TEXT("SettingsVM EndViewModel 후 SelectedTabIndex == INDEX_NONE"), SettingsVM->GetSelectedTabIndex(), INDEX_NONE);

	// UBBMGlobalSettings 기본 객체 및 통합 설정 검증
	const UBBMGlobalSettings* GlobalSettings = GetDefault<UBBMGlobalSettings>();
	TestNotNull(TEXT("UBBMGlobalSettings CDO 조회 성공"), GlobalSettings);
	if (GlobalSettings != nullptr)
	{
		TestEqual(TEXT("UBBMGlobalSettings 섹션명은 BBM Global Settings여야 함"), GlobalSettings->GetSectionName(), FName(TEXT("BBM Global Settings")));
		TestTrue(TEXT("DefaultGame.ini에서 TeamRelations가 정상 로드되어야 함"), GlobalSettings->TeamRelations.Num() > 0);
	}

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
	SettingsVM->FinishInitialization();

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

/**
 * 테스트 케이스 6: UBBMCreditEntryViewModel 및 UBBMCreditsViewModel 초기화/FieldNotify 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMCreditsViewModelTest,
	"DIS.BBM.UI.CreditsViewModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FBBMCreditsViewModelTest::RunTest(const FString& Parameters)
{
	FBBMMVVMScopedTestWorld TestWorld;
	TestNotNull(TEXT("테스트 월드 생성 성공"), TestWorld.World);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 6-1. UBBMCreditEntryViewModel 단독 동작 및 FieldNotify 검증
	UBBMCreditEntryViewModel* EntryVM = NewObject<UBBMCreditEntryViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMCreditEntryViewModel 생성 성공"), EntryVM);
	if (EntryVM == nullptr)
	{
		return false;
	}

	int32 CategoryNameNotifyCount = 0;
	int32 EntriesTextNotifyCount = 0;
	int32 StructNotifyCount = 0;

	EntryVM->AddFieldValueChangedDelegate(
		UBBMCreditEntryViewModel::FFieldNotificationClassDescriptor::GetCategoryName,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&CategoryNameNotifyCount](UObject* /*InObject*/, UE::FieldNotification::FFieldId /*InFieldId*/)
		{
			++CategoryNameNotifyCount;
		})
	);
	EntryVM->AddFieldValueChangedDelegate(
		UBBMCreditEntryViewModel::FFieldNotificationClassDescriptor::GetEntriesText,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&EntriesTextNotifyCount](UObject* /*InObject*/, UE::FieldNotification::FFieldId /*InFieldId*/)
		{
			++EntriesTextNotifyCount;
		})
	);
	EntryVM->AddFieldValueChangedDelegate(
		UBBMCreditEntryViewModel::FFieldNotificationClassDescriptor::CreditCategory,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&StructNotifyCount](UObject* /*InObject*/, UE::FieldNotification::FFieldId /*InFieldId*/)
		{
			++StructNotifyCount;
		})
	);

	FBBMCreditCategory SampleCategory;
	SampleCategory.CategoryName = FText::FromString(TEXT("Programming"));
	SampleCategory.Entries.Add(FText::FromString(TEXT("Alice")));
	SampleCategory.Entries.Add(FText::FromString(TEXT("Bob")));

	EntryVM->SetCreditCategory(SampleCategory);

	TestTrue(TEXT("GetCategoryName()은 설정한 카테고리 이름을 반환해야 함"), EntryVM->GetCategoryName().EqualTo(FText::FromString(TEXT("Programming"))));
	TestTrue(TEXT("GetEntriesText()는 각 항목을 개행(\\n)으로 결합해 반환해야 함"), EntryVM->GetEntriesText().EqualTo(FText::FromString(TEXT("Alice\nBob"))));
	TestEqual(TEXT("SetCreditCategory 최초 변경 시 GetCategoryName FieldNotify 1회 발송"), CategoryNameNotifyCount, 1);
	TestEqual(TEXT("SetCreditCategory 최초 변경 시 GetEntriesText FieldNotify 1회 발송"), EntriesTextNotifyCount, 1);
	TestEqual(TEXT("SetCreditCategory 최초 변경 시 CreditCategory FieldNotify 1회 발송"), StructNotifyCount, 1);

	// 동일 값 재설정 시 중복 FieldNotify가 발송되지 않아야 함
	EntryVM->SetCreditCategory(SampleCategory);
	TestEqual(TEXT("동일 구조체 재설정 시 GetCategoryName FieldNotify 미발송"), CategoryNameNotifyCount, 1);
	TestEqual(TEXT("동일 구조체 재설정 시 GetEntriesText FieldNotify 미발송"), EntriesTextNotifyCount, 1);

	// 빈 Entries 설정 시 빈 FText 반환 검증
	FBBMCreditCategory EmptyEntriesCategory;
	EmptyEntriesCategory.CategoryName = FText::FromString(TEXT("Empty"));
	EntryVM->SetCreditCategory(EmptyEntriesCategory);
	TestTrue(TEXT("빈 Entries 설정 시 GetEntriesText()는 빈 텍스트여야 함"), EntryVM->GetEntriesText().IsEmpty());

	// 6-2. UBBMCreditsViewModel::BeginViewModel()의 UBBMGlobalSettings 연동 검증
	UBBMGlobalSettings* MutableSettings = GetMutableDefault<UBBMGlobalSettings>();
	TestNotNull(TEXT("UBBMGlobalSettings CDO 조회 성공"), MutableSettings);
	if (MutableSettings == nullptr)
	{
		return false;
	}

	const TArray<FBBMCreditCategory> SavedCreditCategories = MutableSettings->CreditCategories;

	FBBMCreditCategory CategoryOne;
	CategoryOne.CategoryName = FText::FromString(TEXT("Director"));
	CategoryOne.Entries.Add(FText::FromString(TEXT("Lead Dev")));

	FBBMCreditCategory CategoryTwo;
	CategoryTwo.CategoryName = FText::FromString(TEXT("Fonts"));
	CategoryTwo.Entries.Add(FText::FromString(TEXT("Pretendard Bold")));
	CategoryTwo.Entries.Add(FText::FromString(TEXT("Pretendard Regular")));

	MutableSettings->CreditCategories = { CategoryOne, CategoryTwo };

	UBBMCreditsViewModel* CreditsVM = NewObject<UBBMCreditsViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMCreditsViewModel 생성 성공"), CreditsVM);
	if (CreditsVM != nullptr)
	{
		int32 CreditEntriesNotifyCount = 0;
		CreditsVM->AddFieldValueChangedDelegate(
			UBBMCreditsViewModel::FFieldNotificationClassDescriptor::CreditEntries,
			INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&CreditEntriesNotifyCount](UObject* /*InObject*/, UE::FieldNotification::FFieldId /*InFieldId*/)
			{
				++CreditEntriesNotifyCount;
			})
		);

		CreditsVM->FinishInitialization();

		const TArray<TObjectPtr<UBBMCreditEntryViewModel>>& Entries = CreditsVM->GetCreditEntries();
		TestEqual(TEXT("BeginViewModel() 후 CreditEntries 개수가 Settings와 일치해야 함"), Entries.Num(), 2);
		TestEqual(TEXT("BeginViewModel() 후 CreditEntries FieldNotify 1회 발송"), CreditEntriesNotifyCount, 1);

		if (Entries.Num() == 2)
		{
			TestNotNull(TEXT("첫 번째 CreditEntryViewModel 유효"), Entries[0].Get());
			TestNotNull(TEXT("두 번째 CreditEntryViewModel 유효"), Entries[1].Get());
			if (Entries[0] != nullptr && Entries[1] != nullptr)
			{
				TestTrue(TEXT("엔트리 VM Outer는 CreditsVM이어야 함"), Entries[0]->GetOuter() == CreditsVM);
				TestTrue(TEXT("첫 번째 카테고리 이름 일치"), Entries[0]->GetCategoryName().EqualTo(FText::FromString(TEXT("Director"))));
				TestTrue(TEXT("첫 번째 항목 텍스트 일치"), Entries[0]->GetEntriesText().EqualTo(FText::FromString(TEXT("Lead Dev"))));
				TestTrue(TEXT("두 번째 카테고리 이름 일치"), Entries[1]->GetCategoryName().EqualTo(FText::FromString(TEXT("Fonts"))));
				TestTrue(TEXT("두 번째 항목 개행 결합 텍스트 일치"), Entries[1]->GetEntriesText().EqualTo(FText::FromString(TEXT("Pretendard Bold\nPretendard Regular"))));
			}
		}

		// SetCreditCategories 수동 재호출 시 교체 및 FieldNotify 재발송 검증
		CreditsVM->SetCreditCategories({ CategoryTwo });
		TestEqual(TEXT("SetCreditCategories 재호출 시 개수가 1로 갱신되어야 함"), CreditsVM->GetCreditEntries().Num(), 1);
		TestEqual(TEXT("SetCreditCategories 재호출 시 CreditEntries FieldNotify 추가 발송"), CreditEntriesNotifyCount, 2);
	}

	// CDO 원본 데이터 복원
	MutableSettings->CreditCategories = SavedCreditCategories;

	return true;
}

// 7. UBBMGameDataSubsystem 세이브 시작/완료 대리자 및 UBBMTitleViewModel Begin/EndViewModel 수명 주기 바인딩 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMTitleViewModelSaveDelegateBindingTest,
	"DIS.BBM.UI.TitleViewModelSaveDelegateBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMTitleViewModelSaveDelegateBindingTest::RunTest(const FString& Parameters)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString SavPath = UBBMGameDataSubsystem::GetSaveFilePath(UBBMRunSaveGame::SlotName);
	const FString BakPath = UBBMGameDataSubsystem::GetBackupFilePath(UBBMRunSaveGame::SlotName);

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

	ON_SCOPE_EXIT
	{
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
	TestTrue(TEXT("테스트 월드 유효 검증"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMGameDataSubsystem* GameDataSS = (TestWorld.GameInstance != nullptr)
		? TestWorld.GameInstance->GetSubsystem<UBBMGameDataSubsystem>()
		: nullptr;
	TestNotNull(TEXT("UBBMGameDataSubsystem 획득 검증"), GameDataSS);
	if (GameDataSS == nullptr)
	{
		return false;
	}

	// 블루프린트 바인딩용 동적 멀티캐스트 대리자 프로퍼티 노출 확인
	FMulticastDelegateProperty* SaveStartedProp = FindFProperty<FMulticastDelegateProperty>(UBBMGameDataSubsystem::StaticClass(), TEXT("OnSaveStarted"));
	FMulticastDelegateProperty* SaveCompletedProp = FindFProperty<FMulticastDelegateProperty>(UBBMGameDataSubsystem::StaticClass(), TEXT("OnSaveCompleted"));
	TestNotNull(TEXT("OnSaveStarted 블루프린트 멀티캐스트 대리자 프로퍼티 존재"), SaveStartedProp);
	TestNotNull(TEXT("OnSaveCompleted 블루프린트 멀티캐스트 대리자 프로퍼티 존재"), SaveCompletedProp);

	UBBMTitleViewModel* TitleVM = NewObject<UBBMTitleViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMTitleViewModel 생성 성공"), TitleVM);
	if (TitleVM == nullptr)
	{
		return false;
	}

	// BeginViewModel()에서 OnSaveStartedNative 바인딩 수행
	AssignDefaultSubViewModels(TestWorld.World, TitleVM);
	TitleVM->FinishInitialization();
	TestTrue(TEXT("BeginViewModel() 이후 OnSaveStartedNative에 바인딩되어야 함"), GameDataSS->OnSaveStartedNative.IsBound());

	UBBMRunSaveGame* RunData = GameDataSS->GetCurrentRunData();
	TestNotNull(TEXT("CurrentRunData 유효 검증"), RunData);
	if (RunData == nullptr)
	{
		return false;
	}

	// 7-1. OnSaveStartedNative 직접 브로드캐스트 시 HandleSaveStarted -> RefreshTitleState() 연동 검증
	RunData->bHasActiveRun = true;
	RunData->CurrentEncounterIndex = 5;
	GameDataSS->OnSaveStartedNative.Broadcast();
	TestTrue(TEXT("OnSaveStartedNative 브로드캐스트 시 bCanContinue가 true로 갱신되어야 함"), TitleVM->GetCanContinue());
	TestEqual(TEXT("OnSaveStartedNative 브로드캐스트 시 ActiveRunEncounterIndex가 5로 갱신되어야 함"), TitleVM->GetActiveRunEncounterIndex(), 5);

	// 7-2. 실제 SaveCurrentRunAsync 호출 시 세이브 시작/완료 대리자 순서 및 TitleViewModel 상태 자동 동기화 검증
	TArray<FString> EventSequence;
	bool bObservedSaveSuccess = false;
	const FDelegateHandle StartHandle = GameDataSS->OnSaveStartedNative.AddLambda([&EventSequence]()
	{
		EventSequence.Add(TEXT("Started"));
	});
	const FDelegateHandle CompleteHandle = GameDataSS->OnSaveCompletedNative.AddLambda([&EventSequence, &bObservedSaveSuccess](bool bSuccess)
	{
		bObservedSaveSuccess = bSuccess;
		EventSequence.Add(TEXT("Completed"));
	});

	RunData->bHasActiveRun = true;
	RunData->CurrentEncounterIndex = 7;
	GameDataSS->SaveCurrentRunAsync(FSimpleDelegate());

	const double WaitStartTime = FPlatformTime::Seconds();
	while (GameDataSS->IsAsyncOperationInProgress() == true && (FPlatformTime::Seconds() - WaitStartTime) < 2.0)
	{
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FPlatformProcess::Sleep(0.01f);
	}
	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);

	GameDataSS->OnSaveStartedNative.Remove(StartHandle);
	GameDataSS->OnSaveCompletedNative.Remove(CompleteHandle);

	TestEqual(TEXT("비동기 세이브 1회당 시작/완료 이벤트 총 2건 발생"), EventSequence.Num(), 2);
	if (EventSequence.Num() == 2)
	{
		TestEqual(TEXT("첫 번째 이벤트는 Started여야 함"), EventSequence[0], FString(TEXT("Started")));
		TestEqual(TEXT("두 번째 이벤트는 Completed여야 함"), EventSequence[1], FString(TEXT("Completed")));
	}
	TestTrue(TEXT("OnSaveCompletedNative 성공 여부가 true여야 함"), bObservedSaveSuccess);
	TestTrue(TEXT("비동기 세이브 완료 후 IsAsyncOperationInProgress()는 false여야 함"), GameDataSS->IsAsyncOperationInProgress() == false);
	TestTrue(TEXT("SaveCurrentRunAsync 시작 대리자를 통해 TitleVM bCanContinue가 true여야 함"), TitleVM->GetCanContinue());
	TestEqual(TEXT("SaveCurrentRunAsync 시작 대리자를 통해 TitleVM 조우 인덱스가 7로 갱신되어야 함"), TitleVM->GetActiveRunEncounterIndex(), 7);

	// 7-3. EndViewModel() 호출(BeginDestroy 경로) 시 OnSaveStartedNative 언바인딩 검증
	TitleVM->ConditionalBeginDestroy();
	TestFalse(TEXT("EndViewModel() 호출 이후 OnSaveStartedNative 바인딩이 해제되어야 함"), GameDataSS->OnSaveStartedNative.IsBound());

	// 언바인딩 이후 브로드캐스트 재호출 시 크래시 없이 안전 동작 검증
	RunData->bHasActiveRun = false;
	RunData->CurrentEncounterIndex = 0;
	GameDataSS->OnSaveStartedNative.Broadcast();

	return true;
}

// 8. UBBMTitleViewModel BlockingTransition 가시성, 3대 서브 패널 카운팅, Interrupt 파라미터 및 미할당 에러 차단 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMTitleViewModelBlockingPanelControlTest,
	"DIS.BBM.UI.TitleViewModelBlockingPanelControl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMTitleViewModelBlockingPanelControlTest::RunTest(const FString& Parameters)
{
	FBBMMVVMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드 유효 검증"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	// 8-1. 필수 서브 뷰모델 미할당 상태에서 FinishInitialization 호출 시 에러 로그 발생 및 훅 미바인딩 검증
	{
		AddExpectedError(TEXT("필수 서브 뷰모델이 할당되지 않음"), EAutomationExpectedErrorFlags::Contains, 1);

		UBBMTitleViewModel* IncompleteTitleVM = NewObject<UBBMTitleViewModel>(TestWorld.World);
		TestNotNull(TEXT("미완성 TitleVM 생성 성공"), IncompleteTitleVM);
		if (IncompleteTitleVM != nullptr)
		{
			IncompleteTitleVM->FinishInitialization();
			TestFalse(
				TEXT("미할당 차단 시 BlockingTransition OnStartingNative 바인딩이 수행되지 않아야 함"),
				IncompleteTitleVM->GetBlockingTransition()->OnStartingNative.IsBound()
			);
			TestFalse(
				TEXT("미할당 차단 시 BlockingTransition OnEndedNative 바인딩이 수행되지 않아야 함"),
				IncompleteTitleVM->GetBlockingTransition()->OnEndedNative.IsBound()
			);
		}
	}

	// 8-2. 3종 서브 뷰모델 사전 할당 후 BlockingPanelVisibility 및 다중 서브 패널 카운팅 연동 검증
	UBBMTitleViewModel* TitleVM = NewObject<UBBMTitleViewModel>(TestWorld.World);
	TestNotNull(TEXT("UBBMTitleViewModel 생성 성공"), TitleVM);
	if (TitleVM == nullptr)
	{
		return false;
	}

	AssignDefaultSubViewModels(TestWorld.World, TitleVM);
	TitleVM->FinishInitialization();

	UBBMSettingsViewModel* SettingsVM = TitleVM->GetSettingsViewModel();
	UBBMCharacterSelectViewModel* CharacterSelectVM = TitleVM->GetCharacterSelectViewModel();
	UBBMCreditsViewModel* CreditsVM = TitleVM->GetCreditsViewModel();
	UDualAnimTransition* BlockingTransition = TitleVM->GetBlockingTransition();
	USingleAnimTransition* ShiningTransition = TitleVM->GetShiningTransition();
	TestNotNull(TEXT("SettingsVM 유효"), SettingsVM);
	TestNotNull(TEXT("CharacterSelectVM 유효"), CharacterSelectVM);
	TestNotNull(TEXT("CreditsVM 유효"), CreditsVM);
	TestNotNull(TEXT("BlockingTransition 유효"), BlockingTransition);
	TestNotNull(TEXT("ShiningTransition 유효"), ShiningTransition);
	if (SettingsVM == nullptr || CharacterSelectVM == nullptr || CreditsVM == nullptr || BlockingTransition == nullptr || ShiningTransition == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("초기 BlockingPanelVisibility는 Collapsed여야 함"), TitleVM->GetBlockingPanelVisibility(), ESlateVisibility::Collapsed);
	TestTrue(TEXT("BeginViewModel() 이후 BlockingTransition OnStartingNative 바인딩 확인"), BlockingTransition->OnStartingNative.IsBound());
	TestTrue(TEXT("BeginViewModel() 이후 BlockingTransition OnEndedNative 바인딩 확인"), BlockingTransition->OnEndedNative.IsBound());

	// 프레젠테이션 핸들러를 모의 연결해 Starting/Ending 비동기 상태를 수동 완료 가능하게 구성
	auto BindManualHandlers = [Outer = TestWorld.World](UDualAnimTransition* Transition)
	{
		Transition->BindPresentationHandlers(
			Outer,
			FWorldUIOnPresentationTransitionRequestedSignature::CreateLambda([Transition](bool bWithAnim)
			{
				if (bWithAnim == false)
				{
					Transition->NotifyStartTransitionFinished();
				}
			}),
			FWorldUIOnPresentationTransitionRequestedSignature::CreateLambda([Transition](bool bWithAnim)
			{
				if (bWithAnim == false)
				{
					Transition->NotifyEndTransitionFinished();
				}
			})
		);
	};

	BindManualHandlers(BlockingTransition);
	BindManualHandlers(SettingsVM->GetOpenCloseTransition());
	BindManualHandlers(CharacterSelectVM->GetOpenCloseTransition());
	BindManualHandlers(CreditsVM->GetOpenCloseTransition());

	int32 BlockingVisibilityNotifyCount = 0;
	const FDelegateHandle VisibilityNotifyHandle = TitleVM->AddFieldValueChangedDelegate(
		UBBMTitleViewModel::FFieldNotificationClassDescriptor::BlockingPanelVisibility,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&BlockingVisibilityNotifyCount](UObject* /*InObject*/, UE::FieldNotification::FFieldId /*InFieldId*/)
		{
			++BlockingVisibilityNotifyCount;
		})
	);

	// TitleVM 오픈 중단(bInterrupted=true) 시 OnFinishOpenAnim(true)가 ShiningTransition을 실행하지 않는지 검증
	int32 TitleShiningStartCount = 0;
	ShiningTransition->OnTransitionStartedNative.AddLambda([&TitleShiningStartCount](bool bWithAnim)
	{
		// OnClosed()의 정지 호출(Execute(false))은 제외하고 오픈 완료 루프 시작(Execute(true))만 집계
		if (bWithAnim == true)
		{
			++TitleShiningStartCount;
		}
	});
	BindManualHandlers(TitleVM->GetOpenCloseTransition());
	TitleVM->OpenWithAnim();
	TitleVM->Close();
	TestEqual(TEXT("오픈 중단(bInterrupted=true) 시 ShiningTransition이 시작되면 안 됨"), TitleShiningStartCount, 0);
	TitleVM->Open();
	TestEqual(TEXT("정상 오픈 완료(bInterrupted=false) 시 ShiningTransition이 1회 시작되어야 함"), TitleShiningStartCount, 1);

	// 1) Settings 열기 (0 -> 1) -> BlockingTransition StartWithAnim 시작 및 즉시 Visible 전환
	SettingsVM->OpenWithAnim();
	TestEqual(TEXT("첫 서브 패널 시작 시 BlockingTransition은 Starting이어야 함"), BlockingTransition->GetState(), EDualAnimState::Starting);
	TestEqual(TEXT("BlockingTransition 시작 시 BlockingPanelVisibility는 Visible이어야 함"), TitleVM->GetBlockingPanelVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("BlockingPanelVisibility FieldNotify 1회 발송"), BlockingVisibilityNotifyCount, 1);

	SettingsVM->GetOpenCloseTransition()->NotifyStartTransitionFinished();
	BlockingTransition->NotifyStartTransitionFinished();
	TestEqual(TEXT("Blocking 시작 완료 후 Started 상태 유지"), BlockingTransition->GetState(), EDualAnimState::Started);
	TestEqual(TEXT("Blocking Started 상태에서 Visible 유지"), TitleVM->GetBlockingPanelVisibility(), ESlateVisibility::Visible);

	// 2) Credits 추가 열기 (1 -> 2) -> Blocking 유지 및 1개 닫기(2 -> 1) 시 Collapsed 전환 안 됨 검증
	CreditsVM->OpenWithAnim();
	CreditsVM->GetOpenCloseTransition()->NotifyStartTransitionFinished();
	TestEqual(TEXT("2개 패널 열림 중에도 Blocking Started 유지"), BlockingTransition->GetState(), EDualAnimState::Started);

	CreditsVM->CloseWithAnim();
	CreditsVM->GetOpenCloseTransition()->NotifyEndTransitionFinished();
	TestEqual(TEXT("1개 패널만 닫혀도(2->1) Blocking은 Started 유지"), BlockingTransition->GetState(), EDualAnimState::Started);
	TestEqual(TEXT("잔여 패널 존재 시 BlockingPanelVisibility는 Visible 유지"), TitleVM->GetBlockingPanelVisibility(), ESlateVisibility::Visible);

	// 3) Settings 닫기 (1 -> 0) -> BlockingTransition EndWithAnim 시작 후 종료 완료 시 Collapsed 복귀
	SettingsVM->CloseWithAnim();
	TestEqual(TEXT("Settings Ending 중 아직 OnEndedNative 전이므로 Blocking Started 유지"), BlockingTransition->GetState(), EDualAnimState::Started);
	SettingsVM->GetOpenCloseTransition()->NotifyEndTransitionFinished();
	TestEqual(TEXT("마지막 서브 패널 종료 시 BlockingTransition은 Ending이어야 함"), BlockingTransition->GetState(), EDualAnimState::Ending);
	TestEqual(TEXT("Blocking Ending 중에는 아직 Visible 유지"), TitleVM->GetBlockingPanelVisibility(), ESlateVisibility::Visible);

	BlockingTransition->NotifyEndTransitionFinished();
	TestEqual(TEXT("Blocking 종료 완료 후 Ended 상태"), BlockingTransition->GetState(), EDualAnimState::Ended);
	TestEqual(TEXT("Blocking 종료 완료 후 BlockingPanelVisibility는 Collapsed여야 함"), TitleVM->GetBlockingPanelVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("BlockingPanelVisibility FieldNotify 2회 누적"), BlockingVisibilityNotifyCount, 2);

	// 4) 닫힘 중 재오픈(Ending -> Starting) 인터럽트 시 OnEndedNative(true) -> OnStartingNative 순서 및 카운팅 정합성 검증
	bool bObservedInterruptedOnReopen = false;
	const FDelegateHandle SettingsEndedTrackHandle = SettingsVM->GetOpenCloseTransition()->OnEndedNative.AddLambda([&bObservedInterruptedOnReopen](bool bInterrupted)
	{
		bObservedInterruptedOnReopen = bInterrupted;
	});
	SettingsVM->OpenWithAnim();
	SettingsVM->GetOpenCloseTransition()->NotifyStartTransitionFinished();
	BlockingTransition->NotifyStartTransitionFinished();
	SettingsVM->CloseWithAnim();
	SettingsVM->OpenWithAnim();
	TestTrue(TEXT("닫힘 도중 재오픈 인터럽트 시 OnEndedNative(true)가 통지되어야 함"), bObservedInterruptedOnReopen);
	TestEqual(TEXT("재오픈 후 BlockingTransition은 Starting/Started(Visible)를 유지해야 함"), TitleVM->GetBlockingPanelVisibility(), ESlateVisibility::Visible);
	SettingsVM->GetOpenCloseTransition()->OnEndedNative.Remove(SettingsEndedTrackHandle);

	// 재오픈된 Settings 정상 닫기(1 -> 0) 후 Collapsed 최종 복귀 확인
	SettingsVM->GetOpenCloseTransition()->NotifyStartTransitionFinished();
	BlockingTransition->NotifyStartTransitionFinished();
	SettingsVM->CloseWithAnim();
	SettingsVM->GetOpenCloseTransition()->NotifyEndTransitionFinished();
	BlockingTransition->NotifyEndTransitionFinished();
	TestEqual(TEXT("재오픈 인터럽트 후 정상 닫기 시 Collapsed 복귀"), TitleVM->GetBlockingPanelVisibility(), ESlateVisibility::Collapsed);

	TitleVM->RemoveFieldValueChangedDelegate(UBBMTitleViewModel::FFieldNotificationClassDescriptor::BlockingPanelVisibility, VisibilityNotifyHandle);

	// 8-3. EndViewModel() 호출(BeginDestroy 경로) 후 훅 전부 언바인딩 확인
	TitleVM->ConditionalBeginDestroy();
	TestFalse(TEXT("EndViewModel() 후 BlockingTransition OnStartingNative 해제"), BlockingTransition->OnStartingNative.IsBound());
	TestFalse(TEXT("EndViewModel() 후 BlockingTransition OnEndedNative 해제"), BlockingTransition->OnEndedNative.IsBound());

	// 언바인딩 후 서브 패널을 열어도 BlockingTransition이 반응하지 않아야 함
	CharacterSelectVM->OpenWithAnim();
	TestEqual(TEXT("EndViewModel() 후 서브 패널 열림 시 BlockingTransition은 Ended를 유지해야 함"), BlockingTransition->GetState(), EDualAnimState::Ended);

	return true;
}

// 9. UBBMTabViewModelBase / UBBMTabEntryViewModel 탭 선택·PreSelectedTabIndex·추가·제거·스위치·재갱신 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMTabViewModelTest,
	"DIS.BBM.UI.TabViewModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMTabViewModelTest::RunTest(const FString& Parameters)
{
	FBBMMVVMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드 유효 검증"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMTabViewModelBase* TabVM = NewObject<UBBMTabViewModelBase>(TestWorld.World);
	TestNotNull(TEXT("UBBMTabViewModelBase 생성 성공"), TabVM);
	if (TabVM == nullptr)
	{
		return false;
	}

	TabVM->FinishInitialization();

	// 9-1. 빈 상태 초기 인덱스(INDEX_NONE) 검증
	TestEqual(TEXT("빈 상태 초기 SelectedTabIndex는 INDEX_NONE"), TabVM->GetSelectedTabIndex(), INDEX_NONE);
	TestEqual(TEXT("빈 상태 초기 PreSelectedTabIndex는 INDEX_NONE"), TabVM->GetPreSelectedTabIndex(), INDEX_NONE);
	TestEqual(TEXT("초기 탭 개수 0"), TabVM->GetTabCount(), 0);
	TestNull(TEXT("빈 상태 GetSelectedTabEntry()는 nullptr"), TabVM->GetSelectedTabEntry());

	// 9-2. AddTab 최상위(맨 뒤) 추가 + 자동 선택 + PreSelectedTabIndex + 중복 대리자 차단 검증
	TArray<int32> SelectedEvents;
	TArray<int32> DeselectedEvents;
	auto HookEntryEvents = [&SelectedEvents, &DeselectedEvents](UBBMTabEntryViewModel* Entry)
	{
		Entry->OnSelectedNative.AddLambda([&SelectedEvents](int32 TabIdx)
		{
			SelectedEvents.Add(TabIdx);
		});
		Entry->OnDeselectedNative.AddLambda([&DeselectedEvents](int32 TabIdx)
		{
			DeselectedEvents.Add(TabIdx);
		});
	};

	UBBMTabEntryViewModel* Entry0 = NewObject<UBBMTabEntryViewModel>(TabVM);
	UBBMTabEntryViewModel* Entry1 = NewObject<UBBMTabEntryViewModel>(TabVM);
	UBBMTabEntryViewModel* Entry2 = NewObject<UBBMTabEntryViewModel>(TabVM);
	Entry0->SetTabText(FText::FromString(TEXT("Tab0")));
	Entry1->SetTabText(FText::FromString(TEXT("Tab1")));
	Entry2->SetTabText(FText::FromString(TEXT("Tab2")));
	HookEntryEvents(Entry0);
	HookEntryEvents(Entry1);
	HookEntryEvents(Entry2);

	TestEqual(TEXT("첫 탭 추가 인덱스 == 0"), TabVM->AddTab(Entry0), 0);
	TestEqual(TEXT("첫 탭 추가 후 SelectedTabIndex == 0"), TabVM->GetSelectedTabIndex(), 0);
	TestEqual(TEXT("첫 탭 추가 후 PreSelectedTabIndex == INDEX_NONE"), TabVM->GetPreSelectedTabIndex(), INDEX_NONE);
	TestTrue(TEXT("Entry0 IsSelected == true"), Entry0->IsSelected());
	TestEqual(TEXT("첫 탭 선택 이벤트 1회(인덱스 0)"), SelectedEvents.Num(), 1);
	TestEqual(TEXT("첫 탭 선택 이벤트 전달 인덱스 == 0"), SelectedEvents[0], 0);

	TestEqual(TEXT("두 번째 탭 추가 인덱스 == 1"), TabVM->AddTab(Entry1), 1);
	TestEqual(TEXT("두 번째 탭 추가 시 SelectedTabIndex == 1 자동 이동"), TabVM->GetSelectedTabIndex(), 1);
	TestEqual(TEXT("두 번째 탭 추가 후 PreSelectedTabIndex == 0"), TabVM->GetPreSelectedTabIndex(), 0);
	TestFalse(TEXT("Entry0 자동 해제됨"), Entry0->IsSelected());
	TestTrue(TEXT("Entry1 자동 선택됨"), Entry1->IsSelected());
	TestEqual(TEXT("해제 이벤트[0] == 0"), DeselectedEvents.Last(), 0);
	TestEqual(TEXT("선택 이벤트[1] == 1"), SelectedEvents.Last(), 1);

	TestEqual(TEXT("세 번째 탭 추가 인덱스 == 2"), TabVM->AddTab(Entry2), 2);
	TestEqual(TEXT("세 번째 탭 추가 시 SelectedTabIndex == 2"), TabVM->GetSelectedTabIndex(), 2);
	TestEqual(TEXT("세 번째 탭 추가 후 PreSelectedTabIndex == 1"), TabVM->GetPreSelectedTabIndex(), 1);

	// 9-3. 동일 인덱스 선택 시도 무시 및 다른 인덱스 RequestSelect() 전환 검증
	int32 SelectedIndexNotifyCount = 0;
	const FDelegateHandle SelectedIndexNotifyHandle = TabVM->AddFieldValueChangedDelegate(
		UBBMTabViewModelBase::FFieldNotificationClassDescriptor::SelectedTabIndex,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&SelectedIndexNotifyCount](UObject* /*Obj*/, UE::FieldNotification::FFieldId /*Id*/)
		{
			++SelectedIndexNotifyCount;
		})
	);

	const int32 PrevSelectedEventNum = SelectedEvents.Num();
	const int32 PrevDeselectedEventNum = DeselectedEvents.Num();
	Entry2->RequestSelect();
	TestEqual(TEXT("동일 인덱스(2) 선택 시 SelectedTabIndex 유지"), TabVM->GetSelectedTabIndex(), 2);
	TestEqual(TEXT("동일 인덱스 선택 시 PreSelectedTabIndex 유지"), TabVM->GetPreSelectedTabIndex(), 1);
	TestEqual(TEXT("동일 인덱스 선택 시 FieldNotify 미발송"), SelectedIndexNotifyCount, 0);
	TestEqual(TEXT("동일 인덱스 선택 시 OnSelected 미호출"), SelectedEvents.Num(), PrevSelectedEventNum);
	TestEqual(TEXT("동일 인덱스 선택 시 OnDeselected 미호출"), DeselectedEvents.Num(), PrevDeselectedEventNum);

	Entry0->RequestSelect();
	TestEqual(TEXT("Entry0 RequestSelect() 후 SelectedTabIndex == 0"), TabVM->GetSelectedTabIndex(), 0);
	TestEqual(TEXT("Entry0 RequestSelect() 후 PreSelectedTabIndex == 2"), TabVM->GetPreSelectedTabIndex(), 2);
	TestEqual(TEXT("SelectedTabIndex FieldNotify 1회 발송"), SelectedIndexNotifyCount, 1);
	TestFalse(TEXT("Entry2 해제됨"), Entry2->IsSelected());
	TestTrue(TEXT("Entry0 선택됨"), Entry0->IsSelected());
	TestEqual(TEXT("해제 이벤트 마지막 전달 인덱스 == 2"), DeselectedEvents.Last(), 2);
	TestEqual(TEXT("선택 이벤트 마지막 전달 인덱스 == 0"), SelectedEvents.Last(), 0);

	// 9-4. SwapTabs(0, 2) 시 SelectedTabIndex 및 PreSelectedTabIndex 동반 이동 및 TabIndex 재갱신 검증
	const int32 EventCountBeforeSwap = SelectedEvents.Num() + DeselectedEvents.Num();
	TestTrue(TEXT("SwapTabs(0, 2) 성공"), TabVM->SwapTabs(0, 2));
	TestEqual(TEXT("스왑 후 인덱스 0은 Entry2"), TabVM->GetTabEntries()[0].Get(), Entry2);
	TestEqual(TEXT("스왑 후 인덱스 2는 Entry0"), TabVM->GetTabEntries()[2].Get(), Entry0);
	TestEqual(TEXT("스왑 후 Entry2 TabIndex == 0"), Entry2->GetTabIndex(), 0);
	TestEqual(TEXT("스왑 후 Entry0 TabIndex == 2"), Entry0->GetTabIndex(), 2);
	TestEqual(TEXT("스왑 후 SelectedTabIndex는 Entry0을 따라 2로 이동"), TabVM->GetSelectedTabIndex(), 2);
	TestEqual(TEXT("스왑 후 PreSelectedTabIndex는 Entry2를 따라 0으로 이동"), TabVM->GetPreSelectedTabIndex(), 0);
	TestEqual(TEXT("스왑 시 동일 엔트리 유지로 선택/해제 중복 대리자 미호출"), SelectedEvents.Num() + DeselectedEvents.Num(), EventCountBeforeSwap);

	// 스왑 후 Entry2 클릭 시 새 인덱스 0으로 전환되는지 확인 (Selected=0, PreSelected=2)
	Entry2->RequestSelect();
	TestEqual(TEXT("스왑된 Entry2 클릭 시 SelectedTabIndex == 0"), TabVM->GetSelectedTabIndex(), 0);
	TestEqual(TEXT("스왑된 Entry2 클릭 후 PreSelectedTabIndex == 2"), TabVM->GetPreSelectedTabIndex(), 2);

	// 9-5. RemoveTabAt(SelectedTabIndex) 시 PreSelectedTabIndex 복귀 + 시프트 + INDEX_NONE 폴백 검증
	// 현재 배열: [Entry2(0, Selected), Entry1(1), Entry0(2, PreSelected)] -> RemoveTabAt(0) 시 Entry0이 인덱스 1로 당겨지며 선택됨
	TestTrue(TEXT("선택된 0번 탭(Entry2) 제거 성공"), TabVM->RemoveTabAt(0));
	TestEqual(TEXT("제거 후 탭 개수 == 2"), TabVM->GetTabCount(), 2);
	TestEqual(TEXT("선택 탭 제거 시 PreSelectedTabIndex(보정 후 1: Entry0)로 전환"), TabVM->GetSelectedTabIndex(), 1);
	TestEqual(TEXT("제거된 탭이 PreSelected에서 지워져 PreSelectedTabIndex == INDEX_NONE"), TabVM->GetPreSelectedTabIndex(), INDEX_NONE);
	TestEqual(TEXT("현재 선택 엔트리는 Entry0"), TabVM->GetSelectedTabEntry(), Entry0);
	TestTrue(TEXT("Entry0 IsSelected == true"), Entry0->IsSelected());
	TestEqual(TEXT("남은 Entry0 TabIndex == 1"), Entry0->GetTabIndex(), 1);

	// PreSelectedTabIndex가 INDEX_NONE인 상태에서 선택 탭(1번 Entry0) 제거 -> 남은 Entry1(0번) 폴백 선택
	TestTrue(TEXT("1번 탭(Entry0) 제거 성공"), TabVM->RemoveTabAt(1));
	TestEqual(TEXT("PreSelected가 INDEX_NONE일 때 남은 탭(0)으로 폴백"), TabVM->GetSelectedTabIndex(), 0);
	TestEqual(TEXT("남은 탭 선택 엔트리는 Entry1"), TabVM->GetSelectedTabEntry(), Entry1);
	TestTrue(TEXT("Entry1 IsSelected == true"), Entry1->IsSelected());

	// 마지막 1개 탭 제거 -> INDEX_NONE 복귀
	TestTrue(TEXT("마지막 0번 탭(Entry1) 제거 성공"), TabVM->RemoveTabAt(0));
	TestEqual(TEXT("모든 탭 제거 후 SelectedTabIndex == INDEX_NONE"), TabVM->GetSelectedTabIndex(), INDEX_NONE);
	TestEqual(TEXT("모든 탭 제거 후 PreSelectedTabIndex == INDEX_NONE"), TabVM->GetPreSelectedTabIndex(), INDEX_NONE);
	TestFalse(TEXT("제거된 Entry1 IsSelected == false"), Entry1->IsSelected());

	// 9-6. SetTabEntries 재갱신(SelectedTabIndex 범위 유지 및 대리자 재바인딩) + RemoveAllTabs 검증
	UBBMTabEntryViewModel* NewA = NewObject<UBBMTabEntryViewModel>(TabVM);
	UBBMTabEntryViewModel* NewB = NewObject<UBBMTabEntryViewModel>(TabVM);
	UBBMTabEntryViewModel* NewC = NewObject<UBBMTabEntryViewModel>(TabVM);
	TabVM->SetTabEntries({ NewA, NewB, NewC });
	TestEqual(TEXT("빈 상태에서 SetTabEntries 후 SelectedTabIndex == 0"), TabVM->GetSelectedTabIndex(), 0);
	TestTrue(TEXT("SetTabEntries 후 0번 NewA 선택"), NewA->IsSelected());
	TestEqual(TEXT("NewB TabIndex == 1"), NewB->GetTabIndex(), 1);
	TestEqual(TEXT("NewC TabIndex == 2"), NewC->GetTabIndex(), 2);

	NewB->RequestSelect();
	TestEqual(TEXT("재바인딩된 NewB 선택 후 SelectedTabIndex == 1"), TabVM->GetSelectedTabIndex(), 1);

	UBBMTabEntryViewModel* ReboundX = NewObject<UBBMTabEntryViewModel>(TabVM);
	UBBMTabEntryViewModel* ReboundY = NewObject<UBBMTabEntryViewModel>(TabVM);
	TabVM->SetTabEntries({ ReboundX, ReboundY });
	TestEqual(TEXT("SetTabEntries 재갱신 시 SelectedTabIndex는 기본 0으로 초기화됨"), TabVM->GetSelectedTabIndex(), 0);
	TestEqual(TEXT("SetTabEntries 재갱신 시 PreSelectedTabIndex는 INDEX_NONE으로 초기화됨"), TabVM->GetPreSelectedTabIndex(), INDEX_NONE);
	TestFalse(TEXT("이전 배열의 NewB 선택 해제됨"), NewB->IsSelected());
	TestTrue(TEXT("새 배열 ReboundX(인덱스 0) 자동 선택 대입"), ReboundX->IsSelected());
	TestFalse(TEXT("새 배열 ReboundY 비선택"), ReboundY->IsSelected());

	ReboundY->RequestSelect();
	TestEqual(TEXT("재갱신된 ReboundY 선택 시 SelectedTabIndex == 1 전환"), TabVM->GetSelectedTabIndex(), 1);
	TestEqual(TEXT("ReboundY 선택 후 PreSelectedTabIndex == 0"), TabVM->GetPreSelectedTabIndex(), 0);

	TabVM->SetTabEntries({});
	TestEqual(TEXT("빈 SetTabEntries 호출 시 SelectedTabIndex == INDEX_NONE"), TabVM->GetSelectedTabIndex(), INDEX_NONE);
	TestEqual(TEXT("빈 SetTabEntries 호출 시 PreSelectedTabIndex == INDEX_NONE"), TabVM->GetPreSelectedTabIndex(), INDEX_NONE);
	TestFalse(TEXT("빈 SetTabEntries 호출 후 ReboundY 해제됨"), ReboundY->IsSelected());

	TabVM->SetTabEntries({ ReboundX, ReboundY });
	TabVM->RemoveAllTabs();
	TestEqual(TEXT("RemoveAllTabs 후 탭 개수 == 0"), TabVM->GetTabCount(), 0);
	TestEqual(TEXT("RemoveAllTabs 후 SelectedTabIndex == INDEX_NONE"), TabVM->GetSelectedTabIndex(), INDEX_NONE);
	TestEqual(TEXT("RemoveAllTabs 후 PreSelectedTabIndex == INDEX_NONE"), TabVM->GetPreSelectedTabIndex(), INDEX_NONE);
	TestFalse(TEXT("RemoveAllTabs 후 ReboundX 해제됨"), ReboundX->IsSelected());

	// 9-7. EndViewModel() 호출 시 모든 엔트리 대리자 언바인딩 확인
	UBBMTabEntryViewModel* FinalTab = TabVM->AddTabWithText(FText::FromString(TEXT("FinalTab")));
	TestNotNull(TEXT("AddTabWithText 생성 성공"), FinalTab);
	if (FinalTab != nullptr)
	{
		TestTrue(TEXT("추가 직후 OnSelectRequestedNative 바인딩 확인"), FinalTab->OnSelectRequestedNative.IsBound());
		TabVM->RemoveFieldValueChangedDelegate(UBBMTabViewModelBase::FFieldNotificationClassDescriptor::SelectedTabIndex, SelectedIndexNotifyHandle);
		TabVM->ConditionalBeginDestroy();
		TestFalse(TEXT("EndViewModel() 후 OnSelectRequestedNative 언바인딩 확인"), FinalTab->OnSelectRequestedNative.IsBound());
	}

	return true;
}

// 10. UBBMSettingsTabEntryWidget MainButtonName 캐싱, SetTabSelected 가시성/스타일 치환 및 OnRequestSelect 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMSettingsTabEntryWidgetTest,
	"DIS.BBM.UI.SettingsTabEntryWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMSettingsTabEntryWidgetTest::RunTest(const FString& Parameters)
{
	FBBMMVVMScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드 유효 검증"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	APlayerController* TestPC = TestWorld.World->SpawnActor<APlayerController>();
	TestNotNull(TEXT("테스트용 PlayerController 스폰 성공"), TestPC);
	if (TestPC == nullptr)
	{
		return false;
	}

	ULocalPlayer* TestLocalPlayer = NewObject<ULocalPlayer>(GEngine);
	TestNotNull(TEXT("테스트용 LocalPlayer 생성 성공"), TestLocalPlayer);
	if (TestLocalPlayer == nullptr)
	{
		return false;
	}

	// SetPlayer를 호출해야 IsLocalPlayerController() 및 FLocalPlayerContext::IsValid()가 성립해 Initialize()가 NativeOnInitialized()를 호출함
	TestPC->SetPlayer(TestLocalPlayer);
	ON_SCOPE_EXIT
	{
		TestPC->Player = nullptr;
		TestLocalPlayer->PlayerController = nullptr;
	};

	// 10-1. MainButtonName 미설정(NAME_None) 시 CachedMainButton 누락 방어 및 SetTabSelected 무변경 조기 반환 검증
	{
		AddExpectedMessagePlain(TEXT("MainButtonName이 NAME_None으로 설정되지 않음"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

		UBBMSettingsTabEntryWidget* UnconfiguredWidget = NewObject<UBBMSettingsTabEntryWidget>(TestWorld.World);
		TestNotNull(TEXT("미설정 SettingsTabEntryWidget 생성 성공"), UnconfiguredWidget);
		if (UnconfiguredWidget != nullptr)
		{
			UnconfiguredWidget->SetPlayerContext(FLocalPlayerContext(TestPC));
			UnconfiguredWidget->Initialize();

			UnconfiguredWidget->SetTabSelected(true);
			TestFalse(TEXT("CachedMainButton == nullptr이면 SetTabSelected(true) 호출 시 상태가 변경되지 않아야 함"), UnconfiguredWidget->IsTabSelected());
		}
	}

	// 10-2. MainButtonName 지정 후 NativeOnInitialized에서 버튼·기본 스타일 캐싱 및 SetTabSelected 상태 전환 검증
	UBBMSettingsTabEntryWidget* EntryWidget = NewObject<UBBMSettingsTabEntryWidget>(TestWorld.World);
	TestNotNull(TEXT("SettingsTabEntryWidget 생성 성공"), EntryWidget);
	if (EntryWidget == nullptr)
	{
		return false;
	}

	EntryWidget->WidgetTree = NewObject<UWidgetTree>(EntryWidget, TEXT("SettingsTabEntryWidgetTree"));
	UButton* SelectButton = EntryWidget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Select"));
	EntryWidget->WidgetTree->RootWidget = SelectButton;

	FButtonStyle DefaultStyle;
	DefaultStyle.Normal.TintColor = FSlateColor(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
	DefaultStyle.NormalForeground = FSlateColor(FLinearColor::White);
	SelectButton->SetStyle(DefaultStyle);

	FButtonStyle SelectedStyle;
	SelectedStyle.Normal.TintColor = FSlateColor(FLinearColor(1.0f, 0.8f, 0.2f, 1.0f));
	SelectedStyle.NormalForeground = FSlateColor(FLinearColor::Black);

	// protected 프로퍼티(MainButtonName, SelectedButtonStyle)를 리플렉션으로 WBP 파생 설정과 동일하게 주입함
	FNameProperty* MainButtonNameProp = FindFProperty<FNameProperty>(UBBMSettingsTabEntryWidget::StaticClass(), TEXT("MainButtonName"));
	FStructProperty* SelectedStyleProp = FindFProperty<FStructProperty>(UBBMSettingsTabEntryWidget::StaticClass(), TEXT("SelectedButtonStyle"));
	TestNotNull(TEXT("MainButtonName 프로퍼티 리플렉션 조회 성공"), MainButtonNameProp);
	TestNotNull(TEXT("SelectedButtonStyle 프로퍼티 리플렉션 조회 성공"), SelectedStyleProp);
	if (MainButtonNameProp == nullptr || SelectedStyleProp == nullptr)
	{
		return false;
	}

	MainButtonNameProp->SetPropertyValue_InContainer(EntryWidget, FName(TEXT("Button_Select")));
	SelectedStyleProp->CopyCompleteValue(SelectedStyleProp->ContainerPtrToValuePtr<void>(EntryWidget), &SelectedStyle);

	EntryWidget->SetPlayerContext(FLocalPlayerContext(TestPC));
	EntryWidget->Initialize();

	TestFalse(TEXT("초기 IsTabSelected는 false여야 함"), EntryWidget->IsTabSelected());
	TestEqual(TEXT("초기 버튼 가시성은 Visible이어야 함"), SelectButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("초기 버튼 스타일 Normal.TintColor 보존 확인"), SelectButton->GetStyle().Normal.TintColor.GetSpecifiedColor(), DefaultStyle.Normal.TintColor.GetSpecifiedColor());

	// 10-3. OnClicked -> bIsSelected == false일 때만 OnRequestSelectNative 발송 검증
	int32 RequestSelectCount = 0;
	EntryWidget->OnRequestSelectNative.AddLambda([&RequestSelectCount]()
	{
		++RequestSelectCount;
	});

	SelectButton->OnClicked.Broadcast();
	TestEqual(TEXT("비선택 상태에서 버튼 클릭 시 OnRequestSelectNative 1회 발송"), RequestSelectCount, 1);

	// 10-4. SetTabSelected(true) -> HitTestInvisible + SelectedButtonStyle 전체 치환 및 클릭 요청 차단 검증
	EntryWidget->SetTabSelected(true);
	TestTrue(TEXT("SetTabSelected(true) 후 IsTabSelected == true"), EntryWidget->IsTabSelected());
	TestEqual(TEXT("선택 상태 버튼 가시성은 HitTestInvisible이어야 함"), SelectButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("선택 상태 버튼 스타일 Normal.TintColor 치환 확인"), SelectButton->GetStyle().Normal.TintColor.GetSpecifiedColor(), SelectedStyle.Normal.TintColor.GetSpecifiedColor());
	TestEqual(TEXT("선택 상태 버튼 전경색 치환 확인"), SelectButton->GetStyle().NormalForeground.GetSpecifiedColor(), SelectedStyle.NormalForeground.GetSpecifiedColor());

	// 동일 상태 재호출 시 불필요한 스타일 덮어쓰기 없이 조기 반환 검증
	FButtonStyle ProbeMarkerStyle = SelectedStyle;
	ProbeMarkerStyle.Hovered.TintColor = FSlateColor(FLinearColor::Green);
	SelectButton->SetStyle(ProbeMarkerStyle);
	EntryWidget->SetTabSelected(true);
	TestEqual(TEXT("이미 true 상태에서 SetTabSelected(true) 재호출 시 스타일이 덮어쓰이지 않고 조기 반환되어야 함"), SelectButton->GetStyle().Hovered.TintColor.GetSpecifiedColor(), FLinearColor::Green);

	SelectButton->OnClicked.Broadcast();
	TestEqual(TEXT("선택 상태(bIsSelected == true)에서 OnClicked 발생 시 OnRequestSelectNative가 호출되면 안 됨"), RequestSelectCount, 1);

	// 10-5. SetTabSelected(false) -> Visible + 초기 기본 스타일 전체 복원 및 클릭 재허용 검증
	EntryWidget->SetTabSelected(false);
	TestFalse(TEXT("SetTabSelected(false) 후 IsTabSelected == false"), EntryWidget->IsTabSelected());
	TestEqual(TEXT("비선택 복원 후 버튼 가시성은 Visible이어야 함"), SelectButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("비선택 복원 후 초기 Normal.TintColor 복원 확인"), SelectButton->GetStyle().Normal.TintColor.GetSpecifiedColor(), DefaultStyle.Normal.TintColor.GetSpecifiedColor());
	TestEqual(TEXT("비선택 복원 후 초기 전경색 복원 확인"), SelectButton->GetStyle().NormalForeground.GetSpecifiedColor(), DefaultStyle.NormalForeground.GetSpecifiedColor());

	SelectButton->OnClicked.Broadcast();
	TestEqual(TEXT("비선택 복원 후 버튼 클릭 시 OnRequestSelectNative 추가 발송(총 2회)"), RequestSelectCount, 2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

