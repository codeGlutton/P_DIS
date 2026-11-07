#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "UI/BBMUserWidgetBase.h"
#include "UI/BBMWorldWidgetSubsystem.h"
#include "Level/BBMGameModeBase.h"
#include "Level/BBMTitleGameMode.h"
#include "Level/BBMStageGameMode.h"
#include "Level/BBMPlayerController.h"
#include "Level/BBMCameraActorBase.h"
#include "Blueprint/UserWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FBBMWidgetScopedTestWorld
{
	UWorld* World = nullptr;

	FBBMWidgetScopedTestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("BBMWidgetTestWorld"));
		if (World != nullptr)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
		}
	}

	~FBBMWidgetScopedTestWorld()
	{
		if (World != nullptr)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			World = nullptr;
		}
	}
};

// 1. EBBMWidgetZOrder 레이어 값 및 10 간격 일관성 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMWidgetZOrderEnumTest,
	"DIS.BBM.WorldWidget.ZOrderEnumValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMWidgetZOrderEnumTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Background 레이어 값은 0이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::Background), 0);
	TestEqual(TEXT("Base 레이어 값은 10이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::Base), 10);
	TestEqual(TEXT("Content 레이어 값은 20이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::Content), 20);
	TestEqual(TEXT("Popup 레이어 값은 30이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::Popup), 30);
	TestEqual(TEXT("Notification 레이어 값은 40이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::Notification), 40);
	TestEqual(TEXT("FadeLoading 레이어 값은 50이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::FadeLoading), 50);
	TestEqual(TEXT("System 레이어 값은 60이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::System), 60);

	// FBBMWorldWidgetConfig 기본 설정 검증
	FBBMWorldWidgetConfig DefaultConfig;
	TestFalse(TEXT("기본 Config의 bOverrideZOrder는 false여야 함"), DefaultConfig.bOverrideZOrder);
	TestEqual(TEXT("기본 Config의 ZOrderLayer는 Base(10)여야 함"), DefaultConfig.ZOrderLayer, EBBMWidgetZOrder::Base);

	// EBBMWidgetAnimPlayMode 5종 및 FBBMWidgetAnimConfig 기본 설정 검증
	TestEqual(TEXT("Forward 열거형 값은 0이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::Forward), 0);
	TestEqual(TEXT("Reverse 열거형 값은 1이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::Reverse), 1);
	TestEqual(TEXT("PingPong 열거형 값은 2이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::PingPong), 2);
	TestEqual(TEXT("RelativeForward 열거형 값은 3이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::RelativeForward), 3);
	TestEqual(TEXT("RelativeReverse 열거형 값은 4이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::RelativeReverse), 4);

	FBBMWidgetAnimConfig DefaultAnimConfig;
	TestEqual(TEXT("기본 AnimConfig의 PlayMode는 RelativeForward여야 함"), DefaultAnimConfig.PlayMode, EBBMWidgetAnimPlayMode::RelativeForward);
	TestEqual(TEXT("기본 AnimConfig의 PlaybackSpeed는 1.0f여야 함"), DefaultAnimConfig.PlaybackSpeed, 1.0f);

	return true;
}

// 2. UBBMUserWidgetBase Open/Close 및 애니메이션 라이프사이클 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMUserWidgetBaseLifecycleTest,
	"DIS.BBM.WorldWidget.LifecyclePolicyAndOpenClose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMUserWidgetBaseLifecycleTest::RunTest(const FString& Parameters)
{
	FBBMWidgetScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMUserWidgetBase* Widget = CreateWidget<UBBMUserWidgetBase>(TestWorld.World, UBBMUserWidgetBase::StaticClass());
	TestNotNull(TEXT("위젯 생성 성공 검증"), Widget);
	if (Widget == nullptr)
	{
		return false;
	}

	// 1. 초기 상태 검증
	TestFalse(TEXT("초기 상태에서 IsOpen은 false여야 함"), Widget->IsOpen());
	TestEqual(TEXT("기본 ZOrder는 Base여야 함"), Widget->GetDefaultZOrderLayer(), EBBMWidgetZOrder::Base);

	// 2. Open 및 Close 호출 검증
	Widget->Open();
	TestTrue(TEXT("Open() 호출 후 IsOpen은 true여야 함"), Widget->IsOpen());

	Widget->Close();
	TestFalse(TEXT("Close() 호출 후 IsOpen은 false여야 함"), Widget->IsOpen());

	// 3. 애니메이션이 없는 상태에서 OpenWithAnim 즉시 완료 검증
	bool bOpenCallbackFired = false;
	Widget->OpenWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bOpenCallbackFired]()
	{
		bOpenCallbackFired = true;
	}));
	TestTrue(TEXT("OpenWithAnim 호출 시 애니메이션이 없으면 즉시 완료 대리자 호출됨"), bOpenCallbackFired);
	TestTrue(TEXT("OpenWithAnim 호출 후 IsOpen은 true여야 함"), Widget->IsOpen());

	// 4. 애니메이션이 없는 상태에서 CloseWithAnim 즉시 완료 검증
	bool bCloseCallbackFired = false;
	Widget->CloseWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bCloseCallbackFired]()
	{
		bCloseCallbackFired = true;
	}));
	TestTrue(TEXT("CloseWithAnim 호출 시 애니메이션이 없으면 즉시 완료 대리자 호출됨"), bCloseCallbackFired);
	TestFalse(TEXT("CloseWithAnim 호출 후 IsOpen은 false여야 함"), Widget->IsOpen());

	return true;
}

// 3. UBBMWorldWidgetSubsystem 위젯 등록, 조회, 교체, 제거 및 일괄 클리어 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMWorldWidgetSubsystemTest,
	"DIS.BBM.WorldWidget.SubsystemRegisterAndQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMWorldWidgetSubsystemTest::RunTest(const FString& Parameters)
{
	FBBMWidgetScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMWorldWidgetSubsystem* WidgetSS = TestWorld.World->GetSubsystem<UBBMWorldWidgetSubsystem>();
	TestNotNull(TEXT("UBBMWorldWidgetSubsystem 획득 성공 검증"), WidgetSS);
	if (WidgetSS == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("초기 등록 위젯 수는 0이어야 함"), WidgetSS->GetRegisteredWidgetCount(), 0);

	// 1. 유효하지 않은 인자 방어 검사
	FBBMWorldWidgetConfig InvalidConfig1;
	InvalidConfig1.WidgetKey = TEXT("Invalid");
	TestNull(TEXT("nullptr 클래스 전달 시 nullptr 반환"), WidgetSS->CreateAndRegisterWidgetByConfig(InvalidConfig1));

	FBBMWorldWidgetConfig InvalidConfig2;
	InvalidConfig2.WidgetClass = UBBMUserWidgetBase::StaticClass();
	InvalidConfig2.WidgetKey = NAME_None;
	TestNull(TEXT("NAME_None 키 전달 시 nullptr 반환"), WidgetSS->CreateAndRegisterWidgetByConfig(InvalidConfig2));

	// 2. 기본값 등록 검증 (bOverrideZOrder false 시 클래스 자체의 DefaultZOrderLayer=Base 유지, AutoOpen 제거로 닫힌 채 등록)
	const FName HUDKey(TEXT("MainHUD"));
	FBBMWorldWidgetConfig HUDConfig;
	HUDConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	HUDConfig.WidgetKey = HUDKey;
	UBBMUserWidgetBase* HUDWidget = WidgetSS->CreateAndRegisterWidgetByConfig(HUDConfig);
	TestNotNull(TEXT("MainHUD 등록 성공"), HUDWidget);
	if (HUDWidget != nullptr)
	{
		TestFalse(TEXT("등록 직후에는 열려있지 않아야 함"), HUDWidget->IsOpen());
		TestEqual(TEXT("ZOrder 미지정 시 클래스 기본값(Base) 유지"), HUDWidget->GetDefaultZOrderLayer(), EBBMWidgetZOrder::Base);
		HUDWidget->Open();
		TestTrue(TEXT("수동 Open() 후 열림 상태 확인"), HUDWidget->IsOpen());
	}

	TestEqual(TEXT("등록된 위젯 수는 1이어야 함"), WidgetSS->GetRegisteredWidgetCount(), 1);
	TestEqual(TEXT("GetWidget으로 등록된 위젯 조회 일치"), WidgetSS->GetWidget(HUDKey), HUDWidget);
	TestEqual(TEXT("GetWidgetAs 템플릿 조회 일치"), WidgetSS->GetWidgetAs<UBBMUserWidgetBase>(HUDKey), HUDWidget);

	// 3. 동일 키 중복 등록 시 기존 위젯 교체 및 명시적 ZOrder 적용 검증
	FBBMWorldWidgetConfig NewHUDConfig;
	NewHUDConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	NewHUDConfig.WidgetKey = HUDKey;
	NewHUDConfig.bOverrideZOrder = true;
	NewHUDConfig.ZOrderLayer = EBBMWidgetZOrder::Notification;
	UBBMUserWidgetBase* NewHUDWidget = WidgetSS->CreateAndRegisterWidgetByConfig(NewHUDConfig);
	TestNotNull(TEXT("새 위젯 교체 등록 성공"), NewHUDWidget);
	if (NewHUDWidget != nullptr && HUDWidget != nullptr)
	{
		TestNotEqual(TEXT("기존 위젯과 다른 새 인스턴스여야 함"), HUDWidget, NewHUDWidget);
		TestFalse(TEXT("등록 직후에는 열려있지 않아야 함"), NewHUDWidget->IsOpen());
		TestEqual(TEXT("명시적으로 지정된 ZOrderLayer(Notification)가 적용되어야 함"), NewHUDWidget->GetDefaultZOrderLayer(), EBBMWidgetZOrder::Notification);
		TestEqual(TEXT("동일 키 교체 후에도 총 등록 위젯 수는 1이어야 함"), WidgetSS->GetRegisteredWidgetCount(), 1);
		TestEqual(TEXT("조회 시 새로 등록된 위젯이 반환되어야 함"), WidgetSS->GetWidget(HUDKey), NewHUDWidget);
		TestFalse(TEXT("교체된 이전 위젯은 Close되었어야 함"), HUDWidget->IsOpen());
	}

	// 4. 단일 위젯 제거 검증
	TestTrue(TEXT("RemoveWidget 성공"), WidgetSS->RemoveWidget(HUDKey));
	TestNull(TEXT("제거 후 GetWidget은 nullptr"), WidgetSS->GetWidget(HUDKey));
	TestEqual(TEXT("제거 후 등록 위젯 수는 0이어야 함"), WidgetSS->GetRegisteredWidgetCount(), 0);
	TestFalse(TEXT("이미 제거된 키를 다시 제거하면 false 반환"), WidgetSS->RemoveWidget(HUDKey));

	// 5. 다중 위젯 등록 및 ClearAllWidgets 검증
	FBBMWorldWidgetConfig ConfigA;
	ConfigA.WidgetClass = UBBMUserWidgetBase::StaticClass();
	ConfigA.WidgetKey = TEXT("WidgetA");
	ConfigA.bOverrideZOrder = true;
	ConfigA.ZOrderLayer = EBBMWidgetZOrder::Base;
	WidgetSS->CreateAndRegisterWidgetByConfig(ConfigA);

	FBBMWorldWidgetConfig ConfigB;
	ConfigB.WidgetClass = UBBMUserWidgetBase::StaticClass();
	ConfigB.WidgetKey = TEXT("WidgetB");
	ConfigB.bOverrideZOrder = true;
	ConfigB.ZOrderLayer = EBBMWidgetZOrder::Popup;
	WidgetSS->CreateAndRegisterWidgetByConfig(ConfigB);

	FBBMWorldWidgetConfig ConfigC;
	ConfigC.WidgetClass = UBBMUserWidgetBase::StaticClass();
	ConfigC.WidgetKey = TEXT("WidgetC");
	ConfigC.bOverrideZOrder = true;
	ConfigC.ZOrderLayer = EBBMWidgetZOrder::FadeLoading;
	WidgetSS->CreateAndRegisterWidgetByConfig(ConfigC);

	TestEqual(TEXT("다중 등록 후 위젯 수는 3이어야 함"), WidgetSS->GetRegisteredWidgetCount(), 3);

	WidgetSS->ClearAllWidgets();
	TestEqual(TEXT("ClearAllWidgets 호출 후 위젯 수는 0이어야 함"), WidgetSS->GetRegisteredWidgetCount(), 0);
	TestNull(TEXT("Clear 후 WidgetA 조회 불가"), WidgetSS->GetWidget(TEXT("WidgetA")));
	TestNull(TEXT("Clear 후 WidgetB 조회 불가"), WidgetSS->GetWidget(TEXT("WidgetB")));
	TestNull(TEXT("Clear 후 WidgetC 조회 불가"), WidgetSS->GetWidget(TEXT("WidgetC")));

	return true;
}

// 4. 게임모드 계층 구조(Hierarchy) 및 기본 설정 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMGameModeHierarchyTest,
	"DIS.BBM.GameMode.HierarchyAndPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMGameModeHierarchyTest::RunTest(const FString& Parameters)
{
	// 1. 상속 계층 검증
	TestTrue(
		TEXT("ABBMTitleGameMode는 ABBMGameModeBase를 상속해야 함"),
		ABBMTitleGameMode::StaticClass()->IsChildOf(ABBMGameModeBase::StaticClass())
	);

	TestTrue(
		TEXT("ABBMStageGameMode는 ABBMGameModeBase를 상속해야 함"),
		ABBMStageGameMode::StaticClass()->IsChildOf(ABBMGameModeBase::StaticClass())
	);

	// 2. ABBMGameModeBase 기본 CDO 설정 검증 (베이스 클래스는 폰/컨트롤러 비움)
	const ABBMGameModeBase* GameModeBaseCDO = GetDefault<ABBMGameModeBase>();
	TestNotNull(TEXT("GameModeBase CDO가 유효해야 함"), GameModeBaseCDO);
	if (GameModeBaseCDO != nullptr)
	{
		TestNull(TEXT("ABBMGameModeBase의 PlayerControllerClass는 nullptr여야 함"), GameModeBaseCDO->PlayerControllerClass.Get());
		TestNull(TEXT("ABBMGameModeBase의 DefaultPawnClass는 nullptr여야 함"), GameModeBaseCDO->DefaultPawnClass.Get());
	}

	// 3. ABBMTitleGameMode 기본 CDO 설정 검증 (타이틀은 폰/컨트롤러 비움)
	const ABBMTitleGameMode* TitleGameModeCDO = GetDefault<ABBMTitleGameMode>();
	TestNotNull(TEXT("TitleGameMode CDO가 유효해야 함"), TitleGameModeCDO);
	if (TitleGameModeCDO != nullptr)
	{
		TestNull(TEXT("ABBMTitleGameMode의 PlayerControllerClass는 nullptr여야 함"), TitleGameModeCDO->PlayerControllerClass.Get());
		TestNull(TEXT("ABBMTitleGameMode의 DefaultPawnClass는 nullptr여야 함"), TitleGameModeCDO->DefaultPawnClass.Get());
	}

	// 4. ABBMStageGameMode 기본 CDO 설정 검증 (스테이지만 컨트롤러와 카메라 폰 설정)
	const ABBMStageGameMode* StageGameModeCDO = GetDefault<ABBMStageGameMode>();
	TestNotNull(TEXT("StageGameMode CDO가 유효해야 함"), StageGameModeCDO);
	if (StageGameModeCDO != nullptr)
	{
		TestEqual(
			TEXT("ABBMStageGameMode의 PlayerControllerClass는 ABBMPlayerController여야 함"),
			StageGameModeCDO->PlayerControllerClass.Get(),
			ABBMPlayerController::StaticClass()
		);

		TestEqual(
			TEXT("ABBMStageGameMode의 DefaultPawnClass는 ABBMCameraActorBase여야 함"),
			StageGameModeCDO->DefaultPawnClass.Get(),
			ABBMCameraActorBase::StaticClass()
		);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
