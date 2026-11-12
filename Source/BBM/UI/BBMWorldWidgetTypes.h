/*****************************************************************//**
 * @file   BBMWorldWidgetTypes.h
 * @brief  BBM(비트 배낭 상인) 월드 위젯 및 UI 공용 타입 정의
 * @details ZOrder 레이어, 수명 주기 정책, 애니메이션 설정 및 위젯 자동 생성 구조체 선언
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Animation/WidgetAnimation.h"
#include "BBMWorldWidgetTypes.generated.h"

/* 애니메이션 전환 완료 대리자 선언 */
DECLARE_DYNAMIC_DELEGATE(FBBMOnWidgetAnimFinishedDynamicSignature);
DECLARE_DELEGATE(FBBMOnWidgetAnimFinishedSignature);

/* 독립 전방 선언 */
class UBBMUserWidgetBase;
class UBBMViewModelBase;

/**
 * BBM UI 위젯 ZOrder 레이어 구분 열거형 (10 간격)
 */
UENUM(BlueprintType)
enum class EBBMWidgetZOrder : uint8
{
	/* 최하단 배경 아트 및 맵 오버레이 (0) */
	Background   = 0   UMETA(DisplayName = "Background (0)"),

	/* 기본 인게임 HUD 및 상시 표시 상태바 (10) */
	Base         = 10  UMETA(DisplayName = "Base HUD (10)"),

	/* 인벤토리, 상점, 캐릭터 장비 등 메인 게임플레이 창 (20) */
	Content      = 20  UMETA(DisplayName = "Content / Windows (20)"),

	/* 팝업 창, 모달 대화상자, 확인 창 (30) */
	Popup        = 30  UMETA(DisplayName = "Popups / Modals (30)"),

	/* 알림, 보상 획득 배너, 획득 토스트 메시지 (40) */
	Notification = 40  UMETA(DisplayName = "Notifications / Alerts (40)"),

	/* 화면 암전 FadeInOut 및 레벨/지역 전환 로딩 표기 위젯 (50) */
	FadeLoading  = 50  UMETA(DisplayName = "Fade & Loading (50)"),

	/* 최상단 시스템 메시지, 에러 팝업, 디버그 오버레이 (60) */
	System       = 60  UMETA(DisplayName = "System / Debug (60)")
};

/**
 * BBM 위젯 수명 주기 및 최적화 방식 정책 열거형
 */
UENUM(BlueprintType)
enum class EBBMWidgetLifecyclePolicy : uint8
{
	/* 상시 뷰포트 유지 (생성 시 AddToViewport, Open/Close 시 Visibility만 전환하여 재계산 오버헤드 최소화) */
	PersistentViewport UMETA(DisplayName = "Persistent Viewport (Visibility Toggle)"),

	/* 필요 시에만 뷰포트 등록 (Open 시 AddToViewport, Close 시 RemoveFromParent하여 렌더 트리 부하 최소화) */
	DynamicDetach      UMETA(DisplayName = "Dynamic Detach (Attach On Open / Detach On Close)")
};

/**
 * BBM 위젯 애니메이션 재생 모드 열거형
 * (EUMGSequencePlayMode 절대 시작 3종 + 현재 진행 상태 기반 상대 재생 2종)
 */
UENUM(BlueprintType)
enum class EBBMWidgetAnimPlayMode : uint8
{
	/* 0.0초(시작점)부터 정방향 재생 [PlayAnimation] */
	Forward          UMETA(DisplayName = "Forward (From Start)"),

	/* 끝 프레임부터 역방향 재생 [PlayAnimation] */
	Reverse          UMETA(DisplayName = "Reverse (From End)"),

	/* 시작 -> 끝 -> 시작 왕복 재생 [PlayAnimation] */
	PingPong         UMETA(DisplayName = "Ping-Pong (Loop)"),

	/* 현재 진행 프레임을 기준으로 끊김 없이 정방향 재생/반전 [PlayAnimationForward] */
	RelativeForward  UMETA(DisplayName = "Relative Forward (Seamless)"),

	/* 현재 진행 프레임을 기준으로 끊김 없이 역방향 되감기/반전 [PlayAnimationReverse] */
	RelativeReverse  UMETA(DisplayName = "Relative Reverse (Seamless)")
};

/**
 * BBM 위젯 애니메이션 재생 설정 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMWidgetAnimConfig
{
	GENERATED_BODY()

	/* 재생할 위젯 애니메이션 에셋 포인터 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BBM|UI|Animation")
	TObjectPtr<UWidgetAnimation> Animation = nullptr;

	/* 재생 모드 (절대 시작 3종 + 상대적 반전 2종) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BBM|UI|Animation")
	EBBMWidgetAnimPlayMode PlayMode = EBBMWidgetAnimPlayMode::RelativeForward;

	/* 재생 속도 배율 (기본값: 1.0f) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BBM|UI|Animation")
	float PlaybackSpeed = 1.0f;
};

/**
 * BBM UI 열림 및 애니메이션 전환 상태 열거형
 */
UENUM(BlueprintType)
enum class EBBMWidgetOpenState : uint8
{
	/* 완전히 닫혀 있는 상태 */
	Closed  UMETA(DisplayName = "Closed"),

	/* 열림 요청 후 오픈 애니메이션 재생 중인 상태 */
	Opening UMETA(DisplayName = "Opening"),

	/* 완전히 열려 있는 상태 */
	Open    UMETA(DisplayName = "Open"),

	/* 닫힘 요청 후 클로즈 애니메이션 재생 중인 상태 */
	Closing UMETA(DisplayName = "Closing")
};

/**
 * 인카운터 초기화 전/후 UI 생성 타이밍 열거형
 */
UENUM(BlueprintType)
enum class EBBMWidgetCreationPhase : uint8
{
	/* 인카운터 배치(상인/몬스터 스폰 및 세이브 복원) 전 생성 (예: FadeLoading 위젯) */
	BeforeEncounter UMETA(DisplayName = "Before Encounter"),

	/* 인카운터 배치 완료 후 생성 (예: 인게임 전투 HUD) */
	AfterEncounter  UMETA(DisplayName = "After Encounter")
};

/**
 * 순수 월드 위젯 생성 설정 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMWorldWidgetConfig
{
	GENERATED_BODY()

public:
	/* 생성할 위젯 클래스 */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UBBMUserWidgetBase> WidgetClass = nullptr;

	/* 뷰포트 ZOrder 레이어 오버라이드 여부 (false일 경우 위젯 클래스 자체의 DefaultZOrderLayer 유지) */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly)
	bool bOverrideZOrder = false;

	/* 오버라이드할 뷰포트 ZOrder 레이어 (bOverrideZOrder가 true일 때만 수정 및 적용) */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverrideZOrder"))
	EBBMWidgetZOrder ZOrderLayer = EBBMWidgetZOrder::Base;
};

/**
 * 순수 월드 ViewModel 생성 설정 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMWorldViewModelConfig
{
	GENERATED_BODY()

public:
	/* 생성할 MVVM ViewModel 클래스 */
	UPROPERTY(Category = "BBM|UI|MVVM", EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UBBMViewModelBase> ViewModelClass = nullptr;
};

/**
 * 게임모드 자동 등록 및 생성용 통합 UI(위젯 + ViewModel + 타이밍) 설정 구조체
 */
USTRUCT(BlueprintType)
struct BBM_API FBBMWorldUIConfig
{
	GENERATED_BODY()

public:
	/* 위젯 및 ViewModel 공용 식별 키 (예: "LoadingScreen", "MainHUD", "TitleMenu") */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly)
	FName WidgetKey = NAME_None;

	/* UI 생성 타이밍 */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly)
	EBBMWidgetCreationPhase CreationPhase = EBBMWidgetCreationPhase::BeforeEncounter;

	/* 순수 위젯 생성 설정 */
	UPROPERTY(Category = "BBM|UI", EditDefaultsOnly, BlueprintReadOnly)
	FBBMWorldWidgetConfig WidgetConfig;

	/* 순수 ViewModel 생성 설정 */
	UPROPERTY(Category = "BBM|UI|MVVM", EditDefaultsOnly, BlueprintReadOnly)
	FBBMWorldViewModelConfig ViewModelConfig;
};

