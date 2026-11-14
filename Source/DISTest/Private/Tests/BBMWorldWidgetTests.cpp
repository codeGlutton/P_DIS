#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/ScopeExit.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "UI/Widget/BBMLayerRootWidget.h"
#include "UI/Widget/BBMUserWidgetBase.h"
#include "UI/BBMWorldUISubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/ViewModel/BBMViewModelBase.h"
#include "UI/ViewModel/BBMSingleAnimTransition.h"
#include "UI/ViewModel/BBMDualAnimTransition.h"
#include "UI/ViewModel/BBMTitleViewModel.h"
#include "Level/BBMGameModeBase.h"
#include "Level/BBMTitleGameMode.h"
#include "Level/BBMStageGameMode.h"
#include "Level/BBMPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Level/BBMCameraActorBase.h"
#include "Blueprint/UserWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "MovieScene.h"
#include "UObject/StrongObjectPtr.h"

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

// 1. EBBMWidgetZOrder 순차 레이어 값(0~6) 일관성 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMWidgetZOrderEnumTest,
	"DIS.BBM.WorldWidget.ZOrderEnumValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMWidgetZOrderEnumTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Background 레이어 값은 0이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::Background), 0);
	TestEqual(TEXT("Base 레이어 값은 1이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::Base), 1);
	TestEqual(TEXT("Content 레이어 값은 2여야 함"), static_cast<uint8>(EBBMWidgetZOrder::Content), 2);
	TestEqual(TEXT("Popup 레이어 값은 3이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::Popup), 3);
	TestEqual(TEXT("Notification 레이어 값은 4여야 함"), static_cast<uint8>(EBBMWidgetZOrder::Notification), 4);
	TestEqual(TEXT("FadeLoading 레이어 값은 5여야 함"), static_cast<uint8>(EBBMWidgetZOrder::FadeLoading), 5);
	TestEqual(TEXT("System 레이어 값은 6이어야 함"), static_cast<uint8>(EBBMWidgetZOrder::System), 6);

	// FBBMWorldWidgetConfig / FBBMWorldViewModelConfig / FBBMWorldUIConfig 기본 설정 검증
	FBBMWorldWidgetConfig DefaultWidgetConfig;
	TestFalse(TEXT("기본 WidgetConfig의 bOverrideZOrder는 false여야 함"), DefaultWidgetConfig.bOverrideZOrder);
	TestEqual(TEXT("기본 WidgetConfig의 ZOrderLayer는 Base(1)여야 함"), DefaultWidgetConfig.ZOrderLayer, EBBMWidgetZOrder::Base);
	TestNull(TEXT("기본 WidgetConfig의 WidgetClass는 nullptr여야 함"), DefaultWidgetConfig.WidgetClass.Get());

	FBBMWorldViewModelConfig DefaultViewModelConfig;
	TestNull(TEXT("기본 ViewModelConfig의 ViewModelClass는 nullptr여야 함"), DefaultViewModelConfig.ViewModelClass.Get());

	FBBMWorldUIConfig DefaultUIConfig;
	TestEqual(TEXT("기본 UIConfig의 WidgetKey는 NAME_None이어야 함"), DefaultUIConfig.WidgetKey, NAME_None);
	TestEqual(TEXT("기본 UIConfig의 CreationPhase는 BeforeEncounter여야 함"), DefaultUIConfig.CreationPhase, EBBMWidgetCreationPhase::BeforeEncounter);

	// EBBMDualAnimState 열거형 값 검증
	TestEqual(TEXT("Ended 열거형 값은 0이어야 함"), static_cast<uint8>(EBBMDualAnimState::Ended), 0);
	TestEqual(TEXT("Starting 열거형 값은 1이어야 함"), static_cast<uint8>(EBBMDualAnimState::Starting), 1);
	TestEqual(TEXT("Started 열거형 값은 2여야 함"), static_cast<uint8>(EBBMDualAnimState::Started), 2);
	TestEqual(TEXT("Ending 열거형 값은 3이어야 함"), static_cast<uint8>(EBBMDualAnimState::Ending), 3);

	// EBBMWidgetAnimPlayMode 5종 및 FBBMWidgetAnimConfig 기본 설정 검증
	TestEqual(TEXT("Forward 열거형 값은 0이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::Forward), 0);
	TestEqual(TEXT("Reverse 열거형 값은 1이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::Reverse), 1);
	TestEqual(TEXT("PingPong 열거형 값은 2이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::PingPong), 2);
	TestEqual(TEXT("RelativeForward 열거형 값은 3이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::RelativeForward), 3);
	TestEqual(TEXT("RelativeReverse 열거형 값은 4이어야 함"), static_cast<uint8>(EBBMWidgetAnimPlayMode::RelativeReverse), 4);

	FBBMWidgetAnimConfig DefaultAnimConfig;
	TestEqual(TEXT("기본 AnimConfig의 AnimationName은 NAME_None이어야 함"), DefaultAnimConfig.AnimationName, NAME_None);
	TestNull(TEXT("nullptr 위젯 조회 시 GetAnimation은 nullptr여야 함"), DefaultAnimConfig.GetAnimation(nullptr));
	TestEqual(TEXT("기본 AnimConfig의 PlayMode는 RelativeForward여야 함"), DefaultAnimConfig.PlayMode, EBBMWidgetAnimPlayMode::RelativeForward);
	TestEqual(TEXT("기본 AnimConfig의 PlaybackSpeed는 1.0f여야 함"), DefaultAnimConfig.PlaybackSpeed, 1.0f);
	TestEqual(TEXT("기본 AnimConfig의 LoopCount는 1이어야 함"), DefaultAnimConfig.LoopCount, 1);

#if WITH_EDITOR
	const FProperty* LoopCountProp = FBBMWidgetAnimConfig::StaticStruct()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(FBBMWidgetAnimConfig, LoopCount));
	TestNotNull(TEXT("LoopCount 프로퍼티 리플렉션 조회 성공"), LoopCountProp);
	if (LoopCountProp != nullptr)
	{
		TestTrue(TEXT("LoopCount는 Relative 계열 숨김용 EditConditionHides 메타를 가져야 함"), LoopCountProp->HasMetaData(TEXT("EditConditionHides")));
		const FString EditConditionStr = LoopCountProp->GetMetaData(TEXT("EditCondition"));
		TestTrue(TEXT("EditCondition은 RelativeForward 제외 조건을 포함해야 함"), EditConditionStr.Contains(TEXT("RelativeForward")));
		TestTrue(TEXT("EditCondition은 RelativeReverse 제외 조건을 포함해야 함"), EditConditionStr.Contains(TEXT("RelativeReverse")));
	}
#endif

	return true;
}

// 2. UBBMViewModelBase 중심 Open/Close, 상태/FieldNotify 및 UBBMUserWidgetBase 프레젠테이션 연동 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMUserWidgetBaseLifecycleTest,
	"DIS.BBM.WorldWidget.LifecyclePolicyAndOpenClose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMUserWidgetBaseLifecycleTest::RunTest(const FString& Parameters)
{
	// 0. 베이스 뷰모델 Abstract 유지 검증
	TestTrue(TEXT("UBBMViewModelBase는 Abstract 클래스여야 함"), UBBMViewModelBase::StaticClass()->HasAnyClassFlags(CLASS_Abstract));

	FBBMWidgetScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMUserWidgetBase* Widget = CreateWidget<UBBMUserWidgetBase>(TestWorld.World, UBBMUserWidgetBase::StaticClass());
	TestNotNull(TEXT("위젯 생성 성공 검증"), Widget);
	UBBMTitleViewModel* ViewModel = NewObject<UBBMTitleViewModel>(TestWorld.World);
	TestNotNull(TEXT("구체 파생 ViewModel 생성 성공 검증"), ViewModel);
	if (Widget == nullptr || ViewModel == nullptr)
	{
		return false;
	}

	ViewModel->FinishInitialization();

	// FieldNotify 통지 기록용 델리게이트 등록
	int32 IsOpenNotifyCount = 0;
	int32 IsShowingNotifyCount = 0;
	TArray<EBBMDualAnimState> NotifiedOpenStates;
	FDelegateHandle IsOpenDelegateHandle = ViewModel->AddFieldValueChangedDelegate(
		UBBMViewModelBase::FFieldNotificationClassDescriptor::IsOpen,
		UBBMViewModelBase::FFieldValueChangedDelegate::CreateLambda([ViewModel, &IsOpenNotifyCount](UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
		{
			if (InObject == ViewModel)
			{
				++IsOpenNotifyCount;
			}
		})
	);
	FDelegateHandle IsShowingDelegateHandle = ViewModel->AddFieldValueChangedDelegate(
		UBBMViewModelBase::FFieldNotificationClassDescriptor::IsShowing,
		UBBMViewModelBase::FFieldValueChangedDelegate::CreateLambda([ViewModel, &IsShowingNotifyCount](UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
		{
			if (InObject == ViewModel)
			{
				++IsShowingNotifyCount;
			}
		})
	);
	FDelegateHandle OpenStateDelegateHandle = ViewModel->AddFieldValueChangedDelegate(
		UBBMViewModelBase::FFieldNotificationClassDescriptor::GetOpenState,
		UBBMViewModelBase::FFieldValueChangedDelegate::CreateLambda([ViewModel, &NotifiedOpenStates](UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
		{
			if (InObject == ViewModel)
			{
				NotifiedOpenStates.Add(ViewModel->GetOpenState());
			}
		})
	);

	// 1. 초기 상태 및 단일 ViewModel 연결 검증
	TestFalse(TEXT("초기 상태에서 ViewModel IsOpen은 false여야 함"), ViewModel->IsOpen());
	TestFalse(TEXT("초기 상태에서 ViewModel IsShowing은 false여야 함"), ViewModel->IsShowing());
	TestEqual(TEXT("초기 상태에서 ViewModel OpenState는 Ended여야 함"), ViewModel->GetOpenState(), EBBMDualAnimState::Ended);
	TestEqual(TEXT("기본 ZOrder는 Base여야 함"), Widget->GetDefaultZOrderLayer(), EBBMWidgetZOrder::Base);
	TestNull(TEXT("연결 전 위젯의 단일 ViewModel은 nullptr여야 함"), Widget->GetViewModel());

	Widget->SetViewModelInstance(ViewModel);
	TestEqual(TEXT("연결 후 위젯의 단일 ViewModel 일치 검증"), Widget->GetViewModel(), Cast<UBBMViewModelBase>(ViewModel));

	// 2. ViewModel Open 및 Close 호출과 FieldNotify 즉시 반영 검증
	ViewModel->Open();
	TestTrue(TEXT("Open() 호출 후 IsOpen은 true여야 함"), ViewModel->IsOpen());
	TestTrue(TEXT("Open() 호출 후 IsShowing은 true여야 함"), ViewModel->IsShowing());
	TestEqual(TEXT("Open() 호출 후 OpenState는 Started여야 함"), ViewModel->GetOpenState(), EBBMDualAnimState::Started);
	TestEqual(TEXT("Open() 호출 시 IsOpen FieldNotify 1회 발생"), IsOpenNotifyCount, 1);
	TestEqual(TEXT("Open() 호출 시 IsShowing FieldNotify 1회 발생"), IsShowingNotifyCount, 1);
	TestEqual(TEXT("Open() 호출 시 위젯 가시성이 SelfHitTestInvisible이어야 함"), Widget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

	ViewModel->Close();
	TestFalse(TEXT("Close() 호출 후 IsOpen은 false여야 함"), ViewModel->IsOpen());
	TestFalse(TEXT("Close() 호출 후 IsShowing은 false여야 함"), ViewModel->IsShowing());
	TestEqual(TEXT("Close() 호출 후 OpenState는 Ended여야 함"), ViewModel->GetOpenState(), EBBMDualAnimState::Ended);
	TestEqual(TEXT("Close() 호출 시 IsOpen FieldNotify 2회 누적"), IsOpenNotifyCount, 2);
	TestEqual(TEXT("Close() 호출 시 IsShowing FieldNotify 2회 누적"), IsShowingNotifyCount, 2);
	TestEqual(TEXT("Close() 호출 시 위젯 가시성이 Collapsed여야 함"), Widget->GetVisibility(), ESlateVisibility::Collapsed);
	TestTrue(TEXT("OpenState FieldNotify 전이 이력 존재 검증"), NotifiedOpenStates.Num() >= 4);
	if (NotifiedOpenStates.Num() >= 4)
	{
		TestEqual(TEXT("전환 1단계는 Starting"), NotifiedOpenStates[0], EBBMDualAnimState::Starting);
		TestEqual(TEXT("전환 2단계는 Started"), NotifiedOpenStates[1], EBBMDualAnimState::Started);
		TestEqual(TEXT("전환 3단계는 Ending"), NotifiedOpenStates[2], EBBMDualAnimState::Ending);
		TestEqual(TEXT("전환 4단계는 Ended"), NotifiedOpenStates[3], EBBMDualAnimState::Ended);
	}

	// 3. 애니메이션이 없는 상태에서 OpenWithAnim 즉시 완료 검증
	bool bOpenCallbackFired = false;
	ViewModel->OpenWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bOpenCallbackFired]()
	{
		bOpenCallbackFired = true;
	}));
	TestTrue(TEXT("OpenWithAnim 호출 시 애니메이션이 없으면 즉시 완료 대리자 호출됨"), bOpenCallbackFired);
	TestTrue(TEXT("OpenWithAnim 호출 후 IsOpen은 true여야 함"), ViewModel->IsOpen());
	TestEqual(TEXT("OpenWithAnim 완료 후 OpenState는 Started여야 함"), ViewModel->GetOpenState(), EBBMDualAnimState::Started);

	// 4. 애니메이션이 없는 상태에서 CloseWithAnim 즉시 완료 검증
	bool bCloseCallbackFired = false;
	ViewModel->CloseWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bCloseCallbackFired]()
	{
		bCloseCallbackFired = true;
	}));
	TestTrue(TEXT("CloseWithAnim 호출 시 애니메이션이 없으면 즉시 완료 대리자 호출됨"), bCloseCallbackFired);
	TestFalse(TEXT("CloseWithAnim 호출 후 IsOpen은 false여야 함"), ViewModel->IsOpen());
	TestEqual(TEXT("CloseWithAnim 완료 후 OpenState는 Ended여야 함"), ViewModel->GetOpenState(), EBBMDualAnimState::Ended);

	// 5. View가 없는 헤드리스 상태에서도 OpenWithAnim/CloseWithAnim 즉시 완료 검증
	Widget->SetViewModelInstance(nullptr);
	bool bHeadlessOpenFired = false;
	ViewModel->OpenWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bHeadlessOpenFired]()
	{
		bHeadlessOpenFired = true;
	}));
	TestTrue(TEXT("헤드리스 OpenWithAnim 즉시 완료 검증"), bHeadlessOpenFired);
	TestTrue(TEXT("헤드리스 OpenWithAnim 후 IsOpen은 true"), ViewModel->IsOpen());

	bool bHeadlessCloseFired = false;
	ViewModel->CloseWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bHeadlessCloseFired]()
	{
		bHeadlessCloseFired = true;
	}));
	TestTrue(TEXT("헤드리스 CloseWithAnim 즉시 완료 검증"), bHeadlessCloseFired);
	TestFalse(TEXT("헤드리스 CloseWithAnim 후 IsOpen은 false"), ViewModel->IsOpen());

	ViewModel->RemoveFieldValueChangedDelegate(UBBMViewModelBase::FFieldNotificationClassDescriptor::IsOpen, IsOpenDelegateHandle);
	ViewModel->RemoveFieldValueChangedDelegate(UBBMViewModelBase::FFieldNotificationClassDescriptor::IsShowing, IsShowingDelegateHandle);
	ViewModel->RemoveFieldValueChangedDelegate(UBBMViewModelBase::FFieldNotificationClassDescriptor::GetOpenState, OpenStateDelegateHandle);

	return true;
}

// 3. UBBMWorldUISubsystem 위젯/ViewModel 독립 등록, 조회, 교체, 분리 제거 및 일괄 클리어 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMWorldUISubsystemTest,
	"DIS.BBM.WorldWidget.SubsystemRegisterAndQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMWorldUISubsystemTest::RunTest(const FString& Parameters)
{
	FBBMWidgetScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMWorldUISubsystem* UISS = TestWorld.World->GetSubsystem<UBBMWorldUISubsystem>();
	TestNotNull(TEXT("UBBMWorldUISubsystem 획득 성공 검증"), UISS);
	if (UISS == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("초기 등록 위젯 수는 0이어야 함"), UISS->GetRegisteredWidgetCount(), 0);
	TestEqual(TEXT("초기 등록 ViewModel 수는 0이어야 함"), UISS->GetRegisteredViewModelCount(), 0);

	// 1. 유효하지 않은 인자 방어 검사 (위젯/ViewModel 및 Abstract 클래스 방어)
	FBBMWorldWidgetConfig InvalidWidgetConfig;
	TestNull(TEXT("nullptr 위젯 클래스 전달 시 nullptr 반환"), UISS->CreateAndRegisterWidgetByConfig(TEXT("Invalid"), InvalidWidgetConfig));

	FBBMWorldWidgetConfig ValidWidgetConfig;
	ValidWidgetConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	TestNull(TEXT("NAME_None 위젯 키 전달 시 nullptr 반환"), UISS->CreateAndRegisterWidgetByConfig(NAME_None, ValidWidgetConfig));

	FBBMWorldViewModelConfig InvalidVMConfig1;
	TestNull(TEXT("nullptr ViewModel 클래스 전달 시 nullptr 반환"), UISS->CreateAndRegisterViewModelByConfig(TEXT("InvalidVM"), InvalidVMConfig1));

	FBBMWorldViewModelConfig InvalidVMConfig2;
	InvalidVMConfig2.ViewModelClass = UBBMViewModelBase::StaticClass();
	TestNull(TEXT("Abstract ViewModel 베이스 클래스 전달 시 nullptr 반환"), UISS->CreateAndRegisterViewModelByConfig(TEXT("AbstractVM"), InvalidVMConfig2));

	FBBMWorldViewModelConfig ValidVMConfig;
	ValidVMConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();
	TestNull(TEXT("NAME_None ViewModel 키 전달 시 nullptr 반환"), UISS->CreateAndRegisterViewModelByConfig(NAME_None, ValidVMConfig));

	// 2. 순수 위젯과 순수 ViewModel 독립 생성 및 수동 연결 검증
	const FName HUDKey(TEXT("MainHUD"));
	FBBMWorldWidgetConfig HUDWidgetConfig;
	HUDWidgetConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	UBBMUserWidgetBase* HUDWidget = UISS->CreateAndRegisterWidgetByConfig(HUDKey, HUDWidgetConfig);
	TestNotNull(TEXT("MainHUD 위젯 등록 성공"), HUDWidget);
	TestEqual(TEXT("위젯 단독 생성 시 ViewModel 개수는 0이어야 함"), UISS->GetRegisteredViewModelCount(), 0);

	FBBMWorldViewModelConfig HUDVMConfig;
	HUDVMConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();
	UBBMViewModelBase* HUDViewModel = UISS->CreateAndRegisterViewModelByConfig(HUDKey, HUDVMConfig);
	TestNotNull(TEXT("MainHUD ViewModel 등록 성공"), HUDViewModel);
	TestEqual(TEXT("등록된 위젯 수는 1이어야 함"), UISS->GetRegisteredWidgetCount(), 1);
	TestEqual(TEXT("등록된 ViewModel 수는 1이어야 함"), UISS->GetRegisteredViewModelCount(), 1);

	if (HUDWidget != nullptr && HUDViewModel != nullptr)
	{
		HUDViewModel->FinishInitialization();
		TestEqual(TEXT("ZOrder 미지정 시 클래스 기본값(Base) 유지"), HUDWidget->GetDefaultZOrderLayer(), EBBMWidgetZOrder::Base);
		TestNull(TEXT("수동 연결 전 위젯의 ViewModel은 nullptr"), HUDWidget->GetViewModel());

		HUDWidget->SetViewModelInstance(HUDViewModel);
		HUDViewModel->Open();
		TestTrue(TEXT("ViewModel Open 후 열림 상태 확인"), HUDViewModel->IsOpen());
		TestEqual(TEXT("ViewModel Open 후 위젯 가시성 확인"), HUDWidget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	}

	TestEqual(TEXT("GetWidget으로 등록된 위젯 조회 일치"), UISS->GetWidget(HUDKey), HUDWidget);
	TestEqual(TEXT("GetWidgetAs 템플릿 조회 일치"), UISS->GetWidgetAs<UBBMUserWidgetBase>(HUDKey), HUDWidget);
	TestEqual(TEXT("GetViewModel로 등록된 ViewModel 조회 일치"), UISS->GetViewModel(HUDKey), HUDViewModel);
	TestEqual(TEXT("GetViewModelAs 템플릿 조회 일치"), UISS->GetViewModelAs<UBBMTitleViewModel>(HUDKey), Cast<UBBMTitleViewModel>(HUDViewModel));

	// 3. 동일 키 위젯 중복 등록 시 기존 위젯만 교체되며 ViewModel은 유지됨 검증
	FBBMWorldWidgetConfig NewHUDConfig;
	NewHUDConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	NewHUDConfig.bOverrideZOrder = true;
	NewHUDConfig.ZOrderLayer = EBBMWidgetZOrder::Notification;
	UBBMUserWidgetBase* NewHUDWidget = UISS->CreateAndRegisterWidgetByConfig(HUDKey, NewHUDConfig);
	TestNotNull(TEXT("새 위젯 교체 등록 성공"), NewHUDWidget);
	if (NewHUDWidget != nullptr && HUDWidget != nullptr)
	{
		TestNotEqual(TEXT("기존 위젯과 다른 새 인스턴스여야 함"), HUDWidget, NewHUDWidget);
		TestFalse(TEXT("교체된 이전 위젯은 레이어에서 분리되었어야 함"), HUDWidget->IsInLayer());
		TestNull(TEXT("교체된 이전 위젯의 ViewModel 참조는 해제되었어야 함"), HUDWidget->GetViewModel());
		TestEqual(TEXT("명시적으로 지정된 ZOrderLayer(Notification)가 적용되어야 함"), NewHUDWidget->GetDefaultZOrderLayer(), EBBMWidgetZOrder::Notification);
		TestEqual(TEXT("동일 키 교체 후에도 총 등록 위젯 수는 1이어야 함"), UISS->GetRegisteredWidgetCount(), 1);
		TestEqual(TEXT("위젯만 교체 시 기존 ViewModel 수는 1로 유지되어야 함"), UISS->GetRegisteredViewModelCount(), 1);
		TestEqual(TEXT("조회 시 새로 등록된 위젯이 반환되어야 함"), UISS->GetWidget(HUDKey), NewHUDWidget);
	}

	// 4. RemoveWidget / RemoveViewModel / RemoveUI 분리 제거 검증
	TestTrue(TEXT("RemoveWidget 단독 제거 성공"), UISS->RemoveWidget(HUDKey));
	TestNull(TEXT("RemoveWidget 후 GetWidget은 nullptr"), UISS->GetWidget(HUDKey));
	TestEqual(TEXT("RemoveWidget 후 등록 위젯 수는 0이어야 함"), UISS->GetRegisteredWidgetCount(), 0);
	TestNotNull(TEXT("RemoveWidget 이후에도 동일 키의 ViewModel은 유지되어야 함"), UISS->GetViewModel(HUDKey));
	TestFalse(TEXT("이미 제거된 위젯 키를 다시 제거하면 false 반환"), UISS->RemoveWidget(HUDKey));

	TestTrue(TEXT("RemoveViewModel 단독 제거 성공"), UISS->RemoveViewModel(HUDKey));
	TestNull(TEXT("RemoveViewModel 후 GetViewModel은 nullptr"), UISS->GetViewModel(HUDKey));
	TestEqual(TEXT("RemoveViewModel 후 등록 ViewModel 수는 0이어야 함"), UISS->GetRegisteredViewModelCount(), 0);
	TestFalse(TEXT("이미 제거된 ViewModel 키를 다시 제거하면 false 반환"), UISS->RemoveViewModel(HUDKey));

	const FName PairKey(TEXT("PairUI"));
	UBBMUserWidgetBase* PairWidget = UISS->CreateAndRegisterWidgetByConfig(PairKey, ValidWidgetConfig);
	UBBMViewModelBase* PairVM = UISS->CreateAndRegisterViewModelByConfig(PairKey, ValidVMConfig);
	if (PairWidget != nullptr && PairVM != nullptr)
	{
		PairVM->FinishInitialization();
		PairWidget->SetViewModelInstance(PairVM);
		PairVM->Open();
	}
	TestTrue(TEXT("RemoveUI 쌍 제거 성공"), UISS->RemoveUI(PairKey));
	TestNull(TEXT("RemoveUI 후 위젯 조회 불가"), UISS->GetWidget(PairKey));
	TestNull(TEXT("RemoveUI 후 ViewModel 조회 불가"), UISS->GetViewModel(PairKey));
	TestFalse(TEXT("RemoveUI 후 PairVM 논리 상태도 닫혀야 함"), PairVM != nullptr ? PairVM->IsOpen() : true);

	// 5. 다중 등록 및 ClearAllWidgets / ClearAllViewModels 분리 정리 검증
	FBBMWorldWidgetConfig ConfigA;
	ConfigA.WidgetClass = UBBMUserWidgetBase::StaticClass();
	ConfigA.bOverrideZOrder = true;
	ConfigA.ZOrderLayer = EBBMWidgetZOrder::Base;
	UISS->CreateAndRegisterWidgetByConfig(TEXT("WidgetA"), ConfigA);
	if (UBBMViewModelBase* WidgetAVM = UISS->CreateAndRegisterViewModelByConfig(TEXT("WidgetA"), ValidVMConfig))
	{
		WidgetAVM->FinishInitialization();
	}

	FBBMWorldWidgetConfig ConfigB;
	ConfigB.WidgetClass = UBBMUserWidgetBase::StaticClass();
	ConfigB.bOverrideZOrder = true;
	ConfigB.ZOrderLayer = EBBMWidgetZOrder::Popup;
	UISS->CreateAndRegisterWidgetByConfig(TEXT("WidgetB"), ConfigB);
	if (UBBMViewModelBase* WidgetBVM = UISS->CreateAndRegisterViewModelByConfig(TEXT("WidgetB"), ValidVMConfig))
	{
		WidgetBVM->FinishInitialization();
	}

	FBBMWorldWidgetConfig ConfigC;
	ConfigC.WidgetClass = UBBMUserWidgetBase::StaticClass();
	ConfigC.bOverrideZOrder = true;
	ConfigC.ZOrderLayer = EBBMWidgetZOrder::FadeLoading;
	UISS->CreateAndRegisterWidgetByConfig(TEXT("WidgetC"), ConfigC);

	TestEqual(TEXT("다중 등록 후 위젯 수는 3이어야 함"), UISS->GetRegisteredWidgetCount(), 3);
	TestEqual(TEXT("다중 등록 후 ViewModel 수는 2여야 함"), UISS->GetRegisteredViewModelCount(), 2);

	UISS->ClearAllWidgets();
	TestEqual(TEXT("ClearAllWidgets 호출 후 위젯 수는 0이어야 함"), UISS->GetRegisteredWidgetCount(), 0);
	TestEqual(TEXT("ClearAllWidgets 호출 후에도 ViewModel 수는 2로 유지되어야 함"), UISS->GetRegisteredViewModelCount(), 2);
	TestNull(TEXT("Clear 후 WidgetA 위젯 조회 불가"), UISS->GetWidget(TEXT("WidgetA")));
	TestNotNull(TEXT("ClearAllWidgets 후 WidgetA ViewModel은 조회 가능"), UISS->GetViewModel(TEXT("WidgetA")));

	UISS->ClearAllViewModels();
	TestEqual(TEXT("ClearAllViewModels 호출 후 ViewModel 수는 0이어야 함"), UISS->GetRegisteredViewModelCount(), 0);
	TestNull(TEXT("ClearAllViewModels 후 WidgetA ViewModel 조회 불가"), UISS->GetViewModel(TEXT("WidgetA")));
	TestNull(TEXT("ClearAllViewModels 후 WidgetB ViewModel 조회 불가"), UISS->GetViewModel(TEXT("WidgetB")));

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

// 4-1. PC 환경에서 ABBMGameModeBase::InitializeLevelSpecificGameMode() 마우스 커서 표시 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMGameModeDesktopCursorTest,
	"DIS.BBM.WorldWidget.GameModeDesktopCursor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMGameModeDesktopCursorTest::RunTest(const FString& Parameters)
{
	UGameInstance* TestGameInstance = NewObject<UGameInstance>(GEngine);
	TestNotNull(TEXT("테스트용 UGameInstance 생성 성공"), TestGameInstance);
	if (TestGameInstance == nullptr)
	{
		return false;
	}

	TestGameInstance->InitializeStandalone();
	UWorld* TestWorld = TestGameInstance->GetWorld();
	TestNotNull(TEXT("테스트 월드가 유효해야 함"), TestWorld);
	if (TestWorld == nullptr)
	{
		TestGameInstance->Shutdown();
		return false;
	}

	ON_SCOPE_EXIT
	{
		TestGameInstance->Shutdown();
		GEngine->DestroyWorldContext(TestWorld);
		TestWorld->DestroyWorld(false);
	};

	TestWorld->InitializeActorsForPlay(FURL());

	APlayerController* TestPC = TestWorld->SpawnActor<APlayerController>();
	TestNotNull(TEXT("기본 APlayerController 스폰 성공"), TestPC);
	if (TestPC == nullptr)
	{
		return false;
	}

	TestFalse(TEXT("초기 기본 APlayerController의 마우스 커서는 비표시 상태여야 함"), TestPC->ShouldShowMouseCursor());

	ABBMTitleGameMode* TitleGameMode = TestWorld->SpawnActor<ABBMTitleGameMode>();
	TestNotNull(TEXT("테스트용 ABBMTitleGameMode 스폰 성공"), TitleGameMode);
	if (TitleGameMode == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("World->GetFirstPlayerController()가 스폰한 PC를 반환해야 함"), TestWorld->GetFirstPlayerController(), TestPC);

	TitleGameMode->DispatchBeginPlay(true);

#if PLATFORM_DESKTOP
	TestTrue(TEXT("PC 환경에서는 InitializeLevelSpecificGameMode() 이후 마우스 커서가 표시되어야 함"), TestPC->ShouldShowMouseCursor());
#else
	TestFalse(TEXT("비PC 환경에서는 기본 APlayerController 마우스 커서가 자동 활성화되지 않아야 함"), TestPC->ShouldShowMouseCursor());
#endif

	return true;
}

// 5. OpenState 파생 IsOpen/IsShowing FieldNotify 및 Opening/Closing/Open/Closed 콜백 누적/즉시실행/스냅 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMViewModelTransitionCallbackQueueTest,
	"DIS.BBM.WorldWidget.ViewModelTransitionCallbackQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMViewModelTransitionCallbackQueueTest::RunTest(const FString& Parameters)
{
	FBBMWidgetScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMTitleViewModel* ViewModel = NewObject<UBBMTitleViewModel>(TestWorld.World);
	TestNotNull(TEXT("테스트용 ViewModel 생성 성공"), ViewModel);
	if (ViewModel == nullptr)
	{
		return false;
	}

	ViewModel->FinishInitialization();

	int32 IsOpenNotifyCount = 0;
	int32 IsShowingNotifyCount = 0;
	FDelegateHandle IsOpenHandle = ViewModel->AddFieldValueChangedDelegate(
		UBBMViewModelBase::FFieldNotificationClassDescriptor::IsOpen,
		UBBMViewModelBase::FFieldValueChangedDelegate::CreateLambda([ViewModel, &IsOpenNotifyCount](UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
		{
			if (InObject == ViewModel)
			{
				++IsOpenNotifyCount;
			}
		})
	);
	FDelegateHandle IsShowingHandle = ViewModel->AddFieldValueChangedDelegate(
		UBBMViewModelBase::FFieldNotificationClassDescriptor::IsShowing,
		UBBMViewModelBase::FFieldValueChangedDelegate::CreateLambda([ViewModel, &IsShowingNotifyCount](UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
		{
			if (InObject == ViewModel)
			{
				++IsShowingNotifyCount;
			}
		})
	);

	// 비동기 애니메이션 재생을 모사하는 프레젠테이션 핸들러 바인딩
	int32 OpenHandlerCallCount = 0;
	int32 CloseHandlerCallCount = 0;
	UBBMDualAnimTransition* OpenCloseTransition = ViewModel->GetOpenCloseTransition();
	TestNotNull(TEXT("OpenCloseTransition 서브 모듈 유효"), OpenCloseTransition);
	if (OpenCloseTransition == nullptr)
	{
		return false;
	}

	OpenCloseTransition->BindPresentationHandlers(
		TestWorld.World,
		FBBMOnPresentationTransitionRequestedSignature::CreateLambda([OpenCloseTransition, &OpenHandlerCallCount](bool bWithAnim)
		{
			++OpenHandlerCallCount;
			if (bWithAnim == false)
			{
				OpenCloseTransition->NotifyStartTransitionFinished();
			}
		}),
		FBBMOnPresentationTransitionRequestedSignature::CreateLambda([OpenCloseTransition, &CloseHandlerCallCount](bool bWithAnim)
		{
			++CloseHandlerCallCount;
			if (bWithAnim == false)
			{
				OpenCloseTransition->NotifyEndTransitionFinished();
			}
		})
	);

	// 1. Closed 상태에서 CloseWithAnim 요청 시 CloseHandler 재실행 없이 콜백만 즉시 처리
	bool bClosedImmediateCallbackFired = false;
	ViewModel->CloseWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bClosedImmediateCallbackFired]()
	{
		bClosedImmediateCallbackFired = true;
	}));
	TestTrue(TEXT("Closed 상태에서 CloseWithAnim 콜백은 즉시 실행되어야 함"), bClosedImmediateCallbackFired);
	TestEqual(TEXT("Closed 상태에서 CloseWithAnim 호출 시 CloseHandler가 호출되면 안 됨"), CloseHandlerCallCount, 0);
	TestFalse(TEXT("Closed 상태에서 IsOpen은 false여야 함"), ViewModel->IsOpen());
	TestFalse(TEXT("Closed 상태에서 IsShowing은 false여야 함"), ViewModel->IsShowing());

	// 2. Opening 진입 및 Opening 중 OpenWithAnim 다중 호출 시 콜백만 누적 검증
	TArray<int32> OpenCallbackOrder;
	ViewModel->OpenWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&OpenCallbackOrder]()
	{
		OpenCallbackOrder.Add(1);
	}));
	TestEqual(TEXT("첫 OpenWithAnim 후 OpenState는 Starting이어야 함"), ViewModel->GetOpenState(), EBBMDualAnimState::Starting);
	TestTrue(TEXT("Opening 상태에서 IsOpen은 true여야 함"), ViewModel->IsOpen());
	TestTrue(TEXT("Opening 상태에서 IsShowing은 true여야 함"), ViewModel->IsShowing());
	TestEqual(TEXT("Ended->Starting 전이 시 IsOpen FieldNotify 1회"), IsOpenNotifyCount, 1);
	TestEqual(TEXT("Ended->Starting 전이 시 IsShowing FieldNotify 1회"), IsShowingNotifyCount, 1);
	TestEqual(TEXT("OpenHandler는 1회 호출되어야 함"), OpenHandlerCallCount, 1);
	TestEqual(TEXT("애니메이션 완료 전까지 Open 콜백은 아직 실행되면 안 됨"), OpenCallbackOrder.Num(), 0);

	ViewModel->OpenWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&OpenCallbackOrder]()
	{
		OpenCallbackOrder.Add(2);
	}));
	ViewModel->OpenWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&OpenCallbackOrder]()
	{
		OpenCallbackOrder.Add(3);
	}));
	TestEqual(TEXT("Opening 중 추가 OpenWithAnim 호출 시 OpenHandler는 재호출되지 않아야 함"), OpenHandlerCallCount, 1);
	TestEqual(TEXT("추가 요청 시점에도 애니메이션 완료 전까지 콜백 미실행"), OpenCallbackOrder.Num(), 0);

	// 3. Open 완료 통지 시 누적 콜백 일괄 순차 실행 및 파생 FieldNotify 중복 미발생 검증
	OpenCloseTransition->NotifyStartTransitionFinished();
	TestEqual(TEXT("완료 후 OpenState는 Started여야 함"), ViewModel->GetOpenState(), EBBMDualAnimState::Started);
	TestEqual(TEXT("Starting->Started 전이 시 IsOpen FieldNotify 추가 발생 없음"), IsOpenNotifyCount, 1);
	TestEqual(TEXT("Starting->Started 전이 시 IsShowing FieldNotify 추가 발생 없음"), IsShowingNotifyCount, 1);
	TestEqual(TEXT("누적된 Open 콜백 3개가 모두 실행되어야 함"), OpenCallbackOrder.Num(), 3);
	if (OpenCallbackOrder.Num() == 3)
	{
		TestEqual(TEXT("콜백 1 순서 일치"), OpenCallbackOrder[0], 1);
		TestEqual(TEXT("콜백 2 순서 일치"), OpenCallbackOrder[1], 2);
		TestEqual(TEXT("콜백 3 순서 일치"), OpenCallbackOrder[2], 3);
	}

	// 4. Open 상태에서 OpenWithAnim 요청 시 재실행 없이 즉시 콜백 처리 검증
	bool bOpenImmediateCallbackFired = false;
	ViewModel->OpenWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bOpenImmediateCallbackFired]()
	{
		bOpenImmediateCallbackFired = true;
	}));
	TestTrue(TEXT("Open 상태에서 OpenWithAnim 콜백은 즉시 실행되어야 함"), bOpenImmediateCallbackFired);
	TestEqual(TEXT("Open 상태에서 OpenWithAnim 호출 시 OpenHandler가 재호출되면 안 됨"), OpenHandlerCallCount, 1);

	// 5. Closing 진입 시 IsOpen == false, IsShowing == true 및 Closing 중 CloseWithAnim 콜백 누적 검증
	TArray<int32> CloseCallbackOrder;
	ViewModel->CloseWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&CloseCallbackOrder]()
	{
		CloseCallbackOrder.Add(1);
	}));
	TestEqual(TEXT("첫 CloseWithAnim 후 OpenState는 Ending이어야 함"), ViewModel->GetOpenState(), EBBMDualAnimState::Ending);
	TestFalse(TEXT("Closing 상태에서 IsOpen은 false여야 함"), ViewModel->IsOpen());
	TestTrue(TEXT("Closing 상태에서 IsShowing은 여전히 true여야 함"), ViewModel->IsShowing());
	TestEqual(TEXT("Started->Ending 전이 시 IsOpen FieldNotify 2회 누적"), IsOpenNotifyCount, 2);
	TestEqual(TEXT("Started->Ending 전이 시 IsShowing FieldNotify는 1회 유지"), IsShowingNotifyCount, 1);
	TestEqual(TEXT("CloseHandler는 1회 호출되어야 함"), CloseHandlerCallCount, 1);

	ViewModel->CloseWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&CloseCallbackOrder]()
	{
		CloseCallbackOrder.Add(2);
	}));
	TestEqual(TEXT("Closing 중 추가 CloseWithAnim 호출 시 CloseHandler는 재호출되지 않아야 함"), CloseHandlerCallCount, 1);
	TestEqual(TEXT("완료 전 Close 콜백 미실행"), CloseCallbackOrder.Num(), 0);

	// 6. Close 완료 통지 시 IsShowing == false, IsShowing FieldNotify 발송 및 누적 콜백 순차 실행 검증
	OpenCloseTransition->NotifyEndTransitionFinished();
	TestEqual(TEXT("완료 후 OpenState는 Ended여야 함"), ViewModel->GetOpenState(), EBBMDualAnimState::Ended);
	TestFalse(TEXT("Closed 상태에서 IsOpen은 false여야 함"), ViewModel->IsOpen());
	TestFalse(TEXT("Closed 상태에서 IsShowing은 false여야 함"), ViewModel->IsShowing());
	TestEqual(TEXT("Ending->Ended 전이 시 IsOpen FieldNotify는 2회 유지"), IsOpenNotifyCount, 2);
	TestEqual(TEXT("Ending->Ended 전이 시 IsShowing FieldNotify 2회 누적"), IsShowingNotifyCount, 2);
	TestEqual(TEXT("누적된 Close 콜백 2개가 모두 실행되어야 함"), CloseCallbackOrder.Num(), 2);
	if (CloseCallbackOrder.Num() == 2)
	{
		TestEqual(TEXT("Close 콜백 1 순서 일치"), CloseCallbackOrder[0], 1);
		TestEqual(TEXT("Close 콜백 2 순서 일치"), CloseCallbackOrder[1], 2);
	}

	// 7. Opening 중 무애님 Open() 호출(옵션 B) 시 즉시 스냅 및 누적 콜백 일괄 호출 검증
	int32 SnapOpenCount = 0;
	ViewModel->OpenWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&SnapOpenCount]()
	{
		++SnapOpenCount;
	}));
	ViewModel->OpenWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&SnapOpenCount]()
	{
		++SnapOpenCount;
	}));
	TestEqual(TEXT("Opening 스냅 전 콜백 수 0"), SnapOpenCount, 0);
	ViewModel->Open();
	TestEqual(TEXT("Opening 중 무애님 Open() 호출 시 즉시 Started 상태 확정"), ViewModel->GetOpenState(), EBBMDualAnimState::Started);
	TestEqual(TEXT("Opening 중 무애님 Open() 호출 시 누적 콜백 2개 모두 호출"), SnapOpenCount, 2);

	// 8. Closing 중 무애님 Close() 호출(옵션 B) 시 즉시 스냅 및 누적 콜백 일괄 호출 검증
	int32 SnapCloseCount = 0;
	ViewModel->CloseWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&SnapCloseCount]()
	{
		++SnapCloseCount;
	}));
	ViewModel->CloseWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&SnapCloseCount]()
	{
		++SnapCloseCount;
	}));
	TestEqual(TEXT("Closing 스냅 전 콜백 수 0"), SnapCloseCount, 0);
	ViewModel->Close();
	TestEqual(TEXT("Closing 중 무애님 Close() 호출 시 즉시 Ended 상태 확정"), ViewModel->GetOpenState(), EBBMDualAnimState::Ended);
	TestEqual(TEXT("Closing 중 무애님 Close() 호출 시 누적 콜백 2개 모두 호출"), SnapCloseCount, 2);

	OpenCloseTransition->ClearPresentationHandlers(TestWorld.World);
	ViewModel->RemoveFieldValueChangedDelegate(UBBMViewModelBase::FFieldNotificationClassDescriptor::IsOpen, IsOpenHandle);
	ViewModel->RemoveFieldValueChangedDelegate(UBBMViewModelBase::FFieldNotificationClassDescriptor::IsShowing, IsShowingHandle);

	return true;
}

// 6. UBBMSingleAnimTransition 단독 상태, FieldNotify, 다중 콜백 누적, Clear 및 핸드셰이크 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMSingleAnimTransitionTest,
	"DIS.BBM.WorldWidget.SingleAnimTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMSingleAnimTransitionTest::RunTest(const FString& Parameters)
{
	FBBMWidgetScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMSingleAnimTransition* SingleTransition = NewObject<UBBMSingleAnimTransition>(TestWorld.World);
	TestNotNull(TEXT("UBBMSingleAnimTransition 생성 성공"), SingleTransition);
	if (SingleTransition == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("초기 State는 Idle이어야 함"), SingleTransition->GetState(), EBBMSingleAnimState::Idle);
	TestFalse(TEXT("초기 IsPlaying은 false여야 함"), SingleTransition->IsPlaying());

	int32 StateNotifyCount = 0;
	int32 IsPlayingNotifyCount = 0;
	FDelegateHandle StateHandle = SingleTransition->AddFieldValueChangedDelegate(
		UBBMSingleAnimTransition::FFieldNotificationClassDescriptor::State,
		UBBMSingleAnimTransition::FFieldValueChangedDelegate::CreateLambda([SingleTransition, &StateNotifyCount](UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
		{
			if (InObject == SingleTransition)
			{
				++StateNotifyCount;
			}
		})
	);
	FDelegateHandle PlayingHandle = SingleTransition->AddFieldValueChangedDelegate(
		UBBMSingleAnimTransition::FFieldNotificationClassDescriptor::IsPlaying,
		UBBMSingleAnimTransition::FFieldValueChangedDelegate::CreateLambda([SingleTransition, &IsPlayingNotifyCount](UObject* InObject, UE::FieldNotification::FFieldId InFieldId)
		{
			if (InObject == SingleTransition)
			{
				++IsPlayingNotifyCount;
			}
		})
	);

	int32 HandlerCallCount = 0;
	SingleTransition->BindPresentationHandler(
		TestWorld.World,
		FBBMOnPresentationTransitionRequestedSignature::CreateLambda([SingleTransition, &HandlerCallCount](bool bWithAnim)
		{
			++HandlerCallCount;
			if (bWithAnim == false)
			{
				SingleTransition->NotifyAnimFinished();
			}
		})
	);

	// 1. 비동기 실행 중 다중 콜백 누적 및 NotifyAnimFinished 일괄 실행 검증
	TArray<int32> CallbackOrder;
	SingleTransition->Execute(true, FBBMOnWidgetAnimFinishedSignature::CreateLambda([&CallbackOrder]()
	{
		CallbackOrder.Add(1);
	}));
	TestEqual(TEXT("Execute 후 State는 Playing이어야 함"), SingleTransition->GetState(), EBBMSingleAnimState::Playing);
	TestTrue(TEXT("Execute 후 IsPlaying은 true여야 함"), SingleTransition->IsPlaying());
	TestEqual(TEXT("HandlerCallCount 1회"), HandlerCallCount, 1);
	TestEqual(TEXT("State FieldNotify 1회"), StateNotifyCount, 1);
	TestEqual(TEXT("IsPlaying FieldNotify 1회"), IsPlayingNotifyCount, 1);

	SingleTransition->Execute(true, FBBMOnWidgetAnimFinishedSignature::CreateLambda([&CallbackOrder]()
	{
		CallbackOrder.Add(2);
	}));
	TestEqual(TEXT("Playing 중 재실행 시 Handler는 추가 호출되지 않아야 함"), HandlerCallCount, 1);
	TestEqual(TEXT("완료 전 콜백 미실행"), CallbackOrder.Num(), 0);

	SingleTransition->NotifyAnimFinished();
	TestEqual(TEXT("NotifyAnimFinished 후 State는 Idle이어야 함"), SingleTransition->GetState(), EBBMSingleAnimState::Idle);
	TestFalse(TEXT("NotifyAnimFinished 후 IsPlaying은 false여야 함"), SingleTransition->IsPlaying());
	TestEqual(TEXT("State FieldNotify 2회 누적"), StateNotifyCount, 2);
	TestEqual(TEXT("IsPlaying FieldNotify 2회 누적"), IsPlayingNotifyCount, 2);
	TestEqual(TEXT("누적된 콜백 2개 모두 순차 실행"), CallbackOrder.Num(), 2);
	if (CallbackOrder.Num() == 2)
	{
		TestEqual(TEXT("콜백 1 순서"), CallbackOrder[0], 1);
		TestEqual(TEXT("콜백 2 순서"), CallbackOrder[1], 2);
	}

	// 2. Playing 중 무애님 Execute(false) 호출 시 즉시 스냅 및 누적 콜백 일괄 호출 검증
	int32 SnapCount = 0;
	SingleTransition->Execute(true, FBBMOnWidgetAnimFinishedSignature::CreateLambda([&SnapCount]()
	{
		++SnapCount;
	}));
	SingleTransition->Execute(false, FBBMOnWidgetAnimFinishedSignature::CreateLambda([&SnapCount]()
	{
		++SnapCount;
	}));
	TestEqual(TEXT("무애님 스냅 후 State는 Idle"), SingleTransition->GetState(), EBBMSingleAnimState::Idle);
	TestEqual(TEXT("무애님 스냅 후 누적+신규 콜백 2개 실행"), SnapCount, 2);

	// 3. Clear() 호출 시 미실행 콜백 폐기 및 상태 리셋 검증
	bool bClearedCallbackFired = false;
	SingleTransition->Execute(true, FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bClearedCallbackFired]()
	{
		bClearedCallbackFired = true;
	}));
	TestEqual(TEXT("Clear 전 Playing 상태"), SingleTransition->GetState(), EBBMSingleAnimState::Playing);
	SingleTransition->Clear();
	TestEqual(TEXT("Clear 후 Idle 복귀"), SingleTransition->GetState(), EBBMSingleAnimState::Idle);
	SingleTransition->NotifyAnimFinished();
	TestFalse(TEXT("Clear로 폐기된 콜백은 실행되면 안 됨"), bClearedCallbackFired);

	// 4. Playing 중 ClearPresentationHandler 호출 시 타 소유자 방어 및 즉시 완료 스냅 검증
	bool bClearedWhilePlayingFired = false;
	SingleTransition->Execute(true, FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bClearedWhilePlayingFired]()
	{
		bClearedWhilePlayingFired = true;
	}));
	TestEqual(TEXT("ClearPresentationHandler 전 Playing 상태"), SingleTransition->GetState(), EBBMSingleAnimState::Playing);

	UObject* WrongOwner = NewObject<UBBMSingleAnimTransition>(TestWorld.World);
	SingleTransition->ClearPresentationHandler(WrongOwner);
	TestTrue(TEXT("타 소유자 해제 시도 시 기존 바인딩 유지"), SingleTransition->IsPresentationHandlerBound());
	TestEqual(TEXT("타 소유자 해제 시도 시 Playing 상태 유지"), SingleTransition->GetState(), EBBMSingleAnimState::Playing);
	TestFalse(TEXT("타 소유자 해제 시도 시 대기 콜백 미실행"), bClearedWhilePlayingFired);

	SingleTransition->ClearPresentationHandler(TestWorld.World);
	TestFalse(TEXT("소유자 해제 후 핸들러 바인딩 해제"), SingleTransition->IsPresentationHandlerBound());
	TestEqual(TEXT("Playing 중 핸들러 해제 시 Idle로 즉시 완료 확정"), SingleTransition->GetState(), EBBMSingleAnimState::Idle);
	TestTrue(TEXT("Playing 중 핸들러 해제 시 대기 콜백 즉시 실행"), bClearedWhilePlayingFired);

	// 5. 핸들러 미바인딩(헤드리스) 시 즉시 완료 처리 검증
	bool bHeadlessFired = false;
	SingleTransition->Execute(true, FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bHeadlessFired]()
	{
		bHeadlessFired = true;
	}));
	TestTrue(TEXT("핸들러 미바인딩 시 즉시 완료 콜백 호출"), bHeadlessFired);
	TestEqual(TEXT("핸들러 미바인딩 완료 후 Idle 복귀"), SingleTransition->GetState(), EBBMSingleAnimState::Idle);

	SingleTransition->RemoveFieldValueChangedDelegate(UBBMSingleAnimTransition::FFieldNotificationClassDescriptor::State, StateHandle);
	SingleTransition->RemoveFieldValueChangedDelegate(UBBMSingleAnimTransition::FFieldNotificationClassDescriptor::IsPlaying, PlayingHandle);

	return true;
}

// 7. UBBMDualAnimTransition 단독 콜백-상태 확정 순서 및 반대 방향 상호 취소 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMDualAnimTransitionTest,
	"DIS.BBM.WorldWidget.DualAnimTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMDualAnimTransitionTest::RunTest(const FString& Parameters)
{
	FBBMWidgetScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMDualAnimTransition* DualTransition = NewObject<UBBMDualAnimTransition>(TestWorld.World);
	TestNotNull(TEXT("UBBMDualAnimTransition 생성 성공"), DualTransition);
	if (DualTransition == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("초기 State는 Ended"), DualTransition->GetState(), EBBMDualAnimState::Ended);
	TestFalse(TEXT("초기 IsStarted는 false"), DualTransition->IsStarted());
	TestFalse(TEXT("초기 IsActive는 false"), DualTransition->IsActive());

	DualTransition->BindPresentationHandlers(
		TestWorld.World,
		FBBMOnPresentationTransitionRequestedSignature::CreateLambda([DualTransition](bool bWithAnim)
		{
			if (bWithAnim == false)
			{
				DualTransition->NotifyStartTransitionFinished();
			}
		}),
		FBBMOnPresentationTransitionRequestedSignature::CreateLambda([DualTransition](bool bWithAnim)
		{
			if (bWithAnim == false)
			{
				DualTransition->NotifyEndTransitionFinished();
			}
		})
	);

	// 1. 콜백 실행 시점에 이미 State == Started 및 OnStartedNative 완료가 선행되어 있는지 검증
	bool bFinishHookBeforeCallback = false;
	bool bObservedStartedInsideCallback = false;
	DualTransition->OnStartedNative.AddLambda([&bFinishHookBeforeCallback]()
	{
		bFinishHookBeforeCallback = true;
	});

	DualTransition->StartWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([DualTransition, &bFinishHookBeforeCallback, &bObservedStartedInsideCallback]()
	{
		bObservedStartedInsideCallback = (bFinishHookBeforeCallback == true)
			&& (DualTransition->GetState() == EBBMDualAnimState::Started)
			&& (DualTransition->IsStarted() == true);
	}));
	TestEqual(TEXT("비동기 Start 진행 중 State는 Starting"), DualTransition->GetState(), EBBMDualAnimState::Starting);
	DualTransition->NotifyStartTransitionFinished();
	TestTrue(TEXT("콜백 실행 시점에는 OnStartedNative 및 Started 상태 확정이 완료되어 있어야 함"), bObservedStartedInsideCallback);

	// 2. Ending 도중 Start 전환 요청 시 End 대기 콜백이 취소되는지 검증
	bool bCanceledEndFired = false;
	DualTransition->EndWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bCanceledEndFired]()
	{
		bCanceledEndFired = true;
	}));
	TestEqual(TEXT("Ending 진입 확인"), DualTransition->GetState(), EBBMDualAnimState::Ending);
	TestFalse(TEXT("Ending 중 IsStarted는 false"), DualTransition->IsStarted());
	TestTrue(TEXT("Ending 중 IsActive는 true"), DualTransition->IsActive());

	DualTransition->Start();
	TestEqual(TEXT("Ending 중 즉시 Start 시 Started 상태 확정"), DualTransition->GetState(), EBBMDualAnimState::Started);
	DualTransition->NotifyEndTransitionFinished();
	TestFalse(TEXT("Start로 전환되며 취소된 End 콜백은 추후 실행되면 안 됨"), bCanceledEndFired);

	// 3. Clear 호출 시 양방향 초기화 검증
	DualTransition->Clear();
	TestEqual(TEXT("Clear 후 State는 Ended"), DualTransition->GetState(), EBBMDualAnimState::Ended);
	TestFalse(TEXT("Clear 후 IsActive는 false"), DualTransition->IsActive());

	// 4. Starting/Ending 중 ClearPresentationHandlers 호출 시 완료 확정 및 Started 안정 상태 보존 검증
	bool bSnapStartedByClear = false;
	DualTransition->StartWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bSnapStartedByClear]()
	{
		bSnapStartedByClear = true;
	}));
	TestEqual(TEXT("ClearPresentationHandlers 전 Starting 확인"), DualTransition->GetState(), EBBMDualAnimState::Starting);
	DualTransition->ClearPresentationHandlers(TestWorld.World);
	TestEqual(TEXT("Starting 중 핸들러 해제 시 Started 확정"), DualTransition->GetState(), EBBMDualAnimState::Started);
	TestTrue(TEXT("Starting 중 핸들러 해제 시 대기 콜백 실행"), bSnapStartedByClear);

	// 안정 상태(Started)에서 재차 핸들러 해제를 호출해도 Ended로 덮어쓰지 않아야 함
	DualTransition->ClearPresentationHandlers(TestWorld.World);
	TestEqual(TEXT("안정 상태 Started는 핸들러 해제 시 유지되어야 함"), DualTransition->GetState(), EBBMDualAnimState::Started);

	// 다시 바인딩 후 Ending 중 ClearPresentationHandlers 호출 시 Ended 확정 검증
	DualTransition->BindPresentationHandlers(
		TestWorld.World,
		FBBMOnPresentationTransitionRequestedSignature::CreateLambda([](bool) {}),
		FBBMOnPresentationTransitionRequestedSignature::CreateLambda([](bool) {})
	);
	bool bSnapEndedByClear = false;
	DualTransition->EndWithAnim(FBBMOnWidgetAnimFinishedSignature::CreateLambda([&bSnapEndedByClear]()
	{
		bSnapEndedByClear = true;
	}));
	TestEqual(TEXT("ClearPresentationHandlers 전 Ending 확인"), DualTransition->GetState(), EBBMDualAnimState::Ending);
	DualTransition->ClearPresentationHandlers(TestWorld.World);
	TestEqual(TEXT("Ending 중 핸들러 해제 시 Ended 확정"), DualTransition->GetState(), EBBMDualAnimState::Ended);
	TestTrue(TEXT("Ending 중 핸들러 해제 시 대기 콜백 실행"), bSnapEndedByClear);
	TestFalse(TEXT("Ending 완료 확정 후 IsActive는 false"), DualTransition->IsActive());

	return true;
}

// 6. FBBMWidgetAnimConfig의 WBGC 상속 계층 기반 애니메이션 탐색 및 캐시 무효화 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMWidgetAnimConfigCacheTest,
	"DIS.BBM.WorldWidget.AnimConfigCacheLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMWidgetAnimConfigCacheTest::RunTest(const FString& Parameters)
{
	FBBMWidgetScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	TArray<TStrongObjectPtr<UObject>> KeepAliveObjects;

	auto CreateTestWBGC = [&KeepAliveObjects](UClass* SuperClass, const TCHAR* ClassName) -> UWidgetBlueprintGeneratedClass*
	{
		UWidgetBlueprintGeneratedClass* NewWBGC = NewObject<UWidgetBlueprintGeneratedClass>(GetTransientPackage(), ClassName);
		NewWBGC->SetSuperStruct(SuperClass);
		NewWBGC->ClassWithin = UObject::StaticClass();
		NewWBGC->Bind();
		NewWBGC->StaticLink(true);
		NewWBGC->GetDefaultObject(true);
		NewWBGC->UpdateCustomPropertyListForPostConstruction();
		KeepAliveObjects.Emplace(NewWBGC);
		return NewWBGC;
	};

	auto CreateTestWidgetAnim = [&KeepAliveObjects](UWidgetBlueprintGeneratedClass* OwnerWBGC, const FName& MovieSceneName) -> UWidgetAnimation*
	{
		const FName AnimObjName = FName(*FString::Printf(TEXT("%s_INST"), *MovieSceneName.ToString()));
		UWidgetAnimation* Anim = NewObject<UWidgetAnimation>(OwnerWBGC, AnimObjName);
		UMovieScene* MovieScene = NewObject<UMovieScene>(Anim, MovieSceneName);
		Anim->MovieScene = MovieScene;
		OwnerWBGC->Animations.Add(Anim);
		KeepAliveObjects.Emplace(Anim);
		KeepAliveObjects.Emplace(MovieScene);
		return Anim;
	};

	UWidgetBlueprintGeneratedClass* ParentWBGC = CreateTestWBGC(UBBMUserWidgetBase::StaticClass(), TEXT("BBMTestParentWBGC"));
	UWidgetBlueprintGeneratedClass* ChildWBGC = CreateTestWBGC(ParentWBGC, TEXT("BBMTestChildWBGC"));
	UWidgetBlueprintGeneratedClass* OtherWBGC = CreateTestWBGC(UBBMUserWidgetBase::StaticClass(), TEXT("BBMTestOtherWBGC"));

	UWidgetAnimation* ParentFadeAnim = CreateTestWidgetAnim(ParentWBGC, FName(TEXT("ParentFade")));
	UWidgetAnimation* ParentAltAnim = CreateTestWidgetAnim(ParentWBGC, FName(TEXT("ParentAlt")));
	UWidgetAnimation* OtherFadeAnim = CreateTestWidgetAnim(OtherWBGC, FName(TEXT("ParentFade")));

	UBBMUserWidgetBase* NativeWidget = NewObject<UBBMUserWidgetBase>(TestWorld.World);
	UBBMUserWidgetBase* ParentWidgetA = NewObject<UBBMUserWidgetBase>(TestWorld.World, ParentWBGC);
	UBBMUserWidgetBase* ParentWidgetB = NewObject<UBBMUserWidgetBase>(TestWorld.World, ParentWBGC);
	UBBMUserWidgetBase* ChildWidget = NewObject<UBBMUserWidgetBase>(TestWorld.World, ChildWBGC);
	UBBMUserWidgetBase* OtherWidget = NewObject<UBBMUserWidgetBase>(TestWorld.World, OtherWBGC);

	KeepAliveObjects.Emplace(NativeWidget);
	KeepAliveObjects.Emplace(ParentWidgetA);
	KeepAliveObjects.Emplace(ParentWidgetB);
	KeepAliveObjects.Emplace(ChildWidget);
	KeepAliveObjects.Emplace(OtherWidget);

	FBBMWidgetAnimConfig Config;

	// 1. 빈 이름 / nullptr / 비-WBGC 위젯 방어 검증
	TestNull(TEXT("AnimationName이 NAME_None이면 nullptr 반환"), Config.GetAnimation(ParentWidgetA));
	Config.AnimationName = FName(TEXT("ParentFade"));
	TestNull(TEXT("nullptr 위젯 전달 시 nullptr 반환"), Config.GetAnimation(nullptr));
	TestNull(TEXT("비-WBGC 네이티브 위젯 전달 시 nullptr 반환"), Config.GetAnimation(NativeWidget));

	// 2. 최초 탐색 및 동일 WBGC·상속 ChildWBGC 캐시 히트 검증
	TestEqual(TEXT("최초 조회 시 ParentFadeAnim 탐색 성공"), Config.GetAnimation(ParentWidgetA), ParentFadeAnim);

	// WBGC->Animations를 임시로 비워 캐시 히트 여부를 직접 검증
	TArray<TObjectPtr<UWidgetAnimation>> SavedParentAnims = ParentWBGC->Animations;
	ParentWBGC->Animations.Empty();

	TestEqual(TEXT("동일 위젯 재호출 시 캐시 히트 유지"), Config.GetAnimation(ParentWidgetA), ParentFadeAnim);
	TestEqual(TEXT("같은 WBGC 다른 위젯 인스턴스도 캐시 히트 유지"), Config.GetAnimation(ParentWidgetB), ParentFadeAnim);
	TestEqual(TEXT("부모 WBGC 애님 캐시는 자식 WBGC 위젯에서도 IsChildOf로 캐시 히트"), Config.GetAnimation(ChildWidget), ParentFadeAnim);

	// 3. 캐시 무효화 후 ChildWidget에서 GetSuperClass() 순회로 부모 애님 탐색 검증
	Config.InvalidateCachedAnimation();
	ParentWBGC->Animations = MoveTemp(SavedParentAnims);
	TestEqual(TEXT("캐시 무효화 후 ChildWidget에서 부모 WBGC 애님 계층 탐색 성공"), Config.GetAnimation(ChildWidget), ParentFadeAnim);

	// 4. 전혀 다른 WBGC 위젯 진입 시 Outer 계층 불일치를 감지해 재탐색·재캐싱 검증
	TestEqual(TEXT("타 WBGC 위젯 전달 시 OtherFadeAnim으로 재캐싱되어야 함"), Config.GetAnimation(OtherWidget), OtherFadeAnim);

	// 5. AnimationName 변경 시 MovieScene 이름 불일치로 재탐색 검증
	Config.AnimationName = FName(TEXT("ParentAlt"));
	TestNull(TEXT("OtherWidget에는 ParentAlt 애님이 없으므로 nullptr 반환"), Config.GetAnimation(OtherWidget));
	TestEqual(TEXT("ParentWidgetA에서 변경된 이름 ParentAlt 탐색 성공"), Config.GetAnimation(ParentWidgetA), ParentAltAnim);

	return true;
}

// 9. 루트 레이어 위젯(UCanvasPanel), AddToLayer, BringToLayerTop, PersistentViewport 재오픈 승격 및 자동 ZOrder 정규화 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBBMWorldUILayerManagementTest,
	"DIS.BBM.WorldWidget.LayerAndZOrderNormalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FBBMWorldUILayerManagementTest::RunTest(const FString& Parameters)
{
	// 1. EBBMWidgetZOrder 7개 레이어별 독립 UCanvasPanel 및 루트 ZOrder(0~6) 생성 검증
	{
		FBBMWidgetScopedTestWorld ProbeWorld;
		TestTrue(TEXT("프로브 테스트 월드가 유효해야 함"), ProbeWorld.World != nullptr);
		if (ProbeWorld.World == nullptr)
		{
			return false;
		}

		const TArray<EBBMWidgetZOrder> ExpectedLayers = {
			EBBMWidgetZOrder::Background,
			EBBMWidgetZOrder::Base,
			EBBMWidgetZOrder::Content,
			EBBMWidgetZOrder::Popup,
			EBBMWidgetZOrder::Notification,
			EBBMWidgetZOrder::FadeLoading,
			EBBMWidgetZOrder::System
		};

		TSet<UPanelWidget*> DistinctLayerCanvases;
		UBBMUserWidgetBase* ProbeWidget = CreateWidget<UBBMUserWidgetBase>(ProbeWorld.World, UBBMUserWidgetBase::StaticClass());
		TestNotNull(TEXT("프로브 위젯 생성 성공"), ProbeWidget);
		if (ProbeWidget == nullptr)
		{
			return false;
		}

		for (const EBBMWidgetZOrder Layer : ExpectedLayers)
		{
			TestTrue(TEXT("각 EBBMWidgetZOrder 레이어로 AddToLayer 성공해야 함"), ProbeWidget->AddToLayer(Layer));
			TestTrue(TEXT("AddToLayer 후 IsInLayer는 true여야 함"), ProbeWidget->IsInLayer());

			UCanvasPanel* LayerCanvas = Cast<UCanvasPanel>(ProbeWidget->GetParent());
			TestNotNull(TEXT("레이어에 추가된 위젯의 부모는 UCanvasPanel이어야 함"), LayerCanvas);
			if (LayerCanvas != nullptr)
			{
				TestFalse(TEXT("각 EBBMWidgetZOrder 레이어는 서로 다른 UCanvasPanel 인스턴스여야 함"), DistinctLayerCanvases.Contains(LayerCanvas));
				DistinctLayerCanvases.Add(LayerCanvas);

				const UCanvasPanelSlot* LayerSlot = Cast<UCanvasPanelSlot>(LayerCanvas->Slot);
				TestNotNull(TEXT("레이어 캔버스는 루트 캔버스 슬롯에 배치되어 있어야 함"), LayerSlot);
				if (LayerSlot != nullptr)
				{
					TestEqual(TEXT("레이어 캔버스 루트 ZOrder는 열거형 정수값(0~6)과 일치해야 함"), LayerSlot->GetZOrder(), static_cast<int32>(Layer));
					TestEqual(TEXT("레이어 캔버스는 전체 화면 앵커를 가져야 함"), LayerSlot->GetAnchors(), FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				}
			}

			ProbeWidget->RemoveFromParent();
			TestFalse(TEXT("RemoveFromParent 후 IsInLayer는 false여야 함"), ProbeWidget->IsInLayer());
		}

		TestEqual(TEXT("총 7개의 독립 레이어 캔버스가 확인되어야 함"), DistinctLayerCanvases.Num(), ExpectedLayers.Num());
	}

	FBBMWidgetScopedTestWorld TestWorld;
	TestTrue(TEXT("테스트 월드가 유효해야 함"), TestWorld.World != nullptr);
	if (TestWorld.World == nullptr)
	{
		return false;
	}

	UBBMWorldUISubsystem* UISS = TestWorld.World->GetSubsystem<UBBMWorldUISubsystem>();
	TestNotNull(TEXT("UBBMWorldUISubsystem 획득 성공 검증"), UISS);
	if (UISS == nullptr)
	{
		return false;
	}

	// 2. AddToLayer 슬롯 레이아웃, ZOrder 순차 증가 및 중복 부착 방지 검증
	FBBMWorldWidgetConfig PopupConfig;
	PopupConfig.WidgetClass = UBBMUserWidgetBase::StaticClass();
	PopupConfig.bOverrideZOrder = true;
	PopupConfig.ZOrderLayer = EBBMWidgetZOrder::Popup;

	FBBMWorldViewModelConfig VMConfig;
	VMConfig.ViewModelClass = UBBMTitleViewModel::StaticClass();

	UBBMUserWidgetBase* PopupA = UISS->CreateAndRegisterWidgetByConfig(TEXT("PopupA"), PopupConfig);
	UBBMUserWidgetBase* PopupB = UISS->CreateAndRegisterWidgetByConfig(TEXT("PopupB"), PopupConfig);
	UBBMUserWidgetBase* PopupC = UISS->CreateAndRegisterWidgetByConfig(TEXT("PopupC"), PopupConfig);
	UBBMViewModelBase* VMA = UISS->CreateAndRegisterViewModelByConfig(TEXT("PopupA"), VMConfig);
	UBBMViewModelBase* VMB = UISS->CreateAndRegisterViewModelByConfig(TEXT("PopupB"), VMConfig);

	TestNotNull(TEXT("PopupA 생성 성공"), PopupA);
	TestNotNull(TEXT("PopupB 생성 성공"), PopupB);
	TestNotNull(TEXT("PopupC 생성 성공"), PopupC);
	if (PopupA == nullptr || PopupB == nullptr || PopupC == nullptr || VMA == nullptr || VMB == nullptr)
	{
		return false;
	}

	VMA->FinishInitialization();
	VMB->FinishInitialization();
	PopupA->SetViewModelInstance(VMA);
	PopupB->SetViewModelInstance(VMB);

	TestFalse(TEXT("최초 부착 전 PopupA IsInLayer는 false여야 함"), PopupA->IsInLayer());

	// PersistentViewport 최초 Open 시 ApplyOpenViewportPresentation에서 AddToLayer 처리
	VMA->Open();
	VMB->Open();

	TestTrue(TEXT("Open 후 PopupA IsInLayer는 true여야 함"), PopupA->IsInLayer());
	TestTrue(TEXT("Open 후 PopupB IsInLayer는 true여야 함"), PopupB->IsInLayer());
	TestEqual(TEXT("PopupA와 PopupB는 동일한 Popup UCanvasPanel 부모를 공유해야 함"), PopupA->GetParent(), PopupB->GetParent());

	const UCanvasPanelSlot* SlotA = Cast<UCanvasPanelSlot>(PopupA->Slot);
	const UCanvasPanelSlot* SlotB = Cast<UCanvasPanelSlot>(PopupB->Slot);
	TestNotNull(TEXT("PopupA 슬롯은 UCanvasPanelSlot이어야 함"), SlotA);
	TestNotNull(TEXT("PopupB 슬롯은 UCanvasPanelSlot이어야 함"), SlotB);
	if (SlotA == nullptr || SlotB == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("AddToLayer 후 슬롯 Anchors는 전체 스트레치여야 함"), SlotA->GetAnchors(), FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	TestEqual(TEXT("AddToLayer 후 슬롯 Offsets는 0이어야 함"), SlotA->GetOffsets(), FMargin(0.0f));
	TestEqual(TEXT("AddToLayer 후 슬롯 Alignment는 0,0이어야 함"), SlotA->GetAlignment(), FVector2D(0.0f, 0.0f));
	TestFalse(TEXT("AddToLayer 후 슬롯 AutoSize는 false여야 함"), SlotA->GetAutoSize());
	TestEqual(TEXT("첫 번째 위젯 ZOrder는 0이어야 함"), SlotA->GetZOrder(), 0);
	TestEqual(TEXT("두 번째 위젯 ZOrder는 1이어야 함"), SlotB->GetZOrder(), 1);

	// 이미 올라간 위젯은 동일/타 레이어로 재추가 불가, RemoveFromParent 후에만 재부착 가능
	TestFalse(TEXT("이미 레이어에 있는 위젯은 같은 레이어 재추가 불가"), PopupA->AddToLayer(EBBMWidgetZOrder::Popup));
	TestFalse(TEXT("이미 레이어에 있는 위젯은 다른 레이어 이동 추가 불가"), PopupA->AddToLayer(EBBMWidgetZOrder::Content));

	TestTrue(TEXT("PopupC 레이어 추가 성공"), PopupC->AddToLayer(EBBMWidgetZOrder::Content));
	TestTrue(TEXT("PopupC 레이어 부착 확인"), PopupC->IsInLayer());
	PopupC->RemoveFromParent();
	TestFalse(TEXT("RemoveFromParent 후 PopupC IsInLayer는 false여야 함"), PopupC->IsInLayer());

	// 3. PersistentViewport 닫았다 다시 열 때 최상단 승격(BringToLayerTop) 검증
	VMA->Close();
	TestEqual(TEXT("Close 후 PersistentViewport 위젯은 Collapsed여야 함"), PopupA->GetVisibility(), ESlateVisibility::Collapsed);
	TestTrue(TEXT("Close 후에도 PersistentViewport 위젯은 레이어에 남아 있어야 함"), PopupA->IsInLayer());

	VMA->Open();
	TestEqual(TEXT("재오픈 후 가시성이 SelfHitTestInvisible로 복구되어야 함"), PopupA->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("재오픈 후 PopupA ZOrder가 2로 승격되어야 함"), SlotA->GetZOrder(), 2);

	// 이미 최상단인 위젯에 BringToLayerTop 재호출 시 카운터 불변 검증 (다음 추가 슬롯이 3이어야 함)
	TestTrue(TEXT("이미 최상단인 위젯 BringToLayerTop 호출은 true 반환"), PopupA->BringToLayerTop());
	TestTrue(TEXT("PopupC를 Popup 레이어에 추가"), PopupC->AddToLayer(EBBMWidgetZOrder::Popup));
	const UCanvasPanelSlot* SlotC = Cast<UCanvasPanelSlot>(PopupC->Slot);
	TestNotNull(TEXT("PopupC 슬롯 유효 검증"), SlotC);
	if (SlotC == nullptr)
	{
		return false;
	}
	TestEqual(TEXT("이미 최상단 재호출 후 추가된 PopupC ZOrder는 3이어야 함"), SlotC->GetZOrder(), 3);

#if WITH_EDITOR
	// 4. ZOrder 임계치 도달 시 순서 유지 압축 정규화(예: 1,2,3 -> 0,1,2 후 최상단 3 할당) 검증
	UISS->SetLayerZOrderMaxThresholdForTesting(4);
	// 현재 ZOrder: PopupB(1) < PopupA(2) < PopupC(3), 다음 할당값 == Threshold(4)
	TestTrue(TEXT("임계치 도달 상태에서 PopupB BringToLayerTop 성공"), PopupB->BringToLayerTop());
	TestEqual(TEXT("정규화 후 PopupA ZOrder는 1이어야 함"), SlotA->GetZOrder(), 1);
	TestEqual(TEXT("정규화 후 PopupC ZOrder는 2여야 함"), SlotC->GetZOrder(), 2);
	TestEqual(TEXT("정규화 직후 최상단 승격된 PopupB ZOrder는 3이어야 함"), SlotB->GetZOrder(), 3);
#endif

	// 5. UBBMWorldUISubsystem이 없는 위젯에서 AddToLayer 호출 시 경고 로그 + false 반환 검증
	AddExpectedMessagePlain(TEXT("UBBMWorldUISubsystem is unavailable"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	UBBMUserWidgetBase* OrphanWidget = NewObject<UBBMUserWidgetBase>(GetTransientPackage());
	TestNotNull(TEXT("고립 위젯 생성 성공"), OrphanWidget);
	if (OrphanWidget != nullptr)
	{
		TestFalse(TEXT("서브시스템 없는 환경에서 AddToLayer는 false를 반환해야 함"), OrphanWidget->AddToLayer(EBBMWidgetZOrder::Base));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
