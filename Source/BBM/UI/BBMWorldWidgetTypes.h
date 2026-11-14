/*****************************************************************//**
 * @file   BBMWorldWidgetTypes.h
 * @brief  BBM(비트 배낭 상인) 월드 위젯 및 UI 공용 타입 정의
 * @details ZOrder 레이어, 수명 주기 정책, 애니메이션 설정 및 위젯 자동 생성 구조체 선언
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BBMWorldWidgetTypes.generated.h"

/* 애니메이션 전환 요청 및 완료 대리자 선언 */
DECLARE_DELEGATE_OneParam(FBBMOnPresentationTransitionRequestedSignature, bool /*bWithAnim*/);
DECLARE_DYNAMIC_DELEGATE(FBBMOnWidgetAnimFinishedDynamicSignature);
DECLARE_DELEGATE(FBBMOnWidgetAnimFinishedSignature);

/* 애니메이션 전환 라이프사이클 브로드캐스트 동적 대리자 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBBMOnSingleAnimTransitionStartedDynamicSignature, bool, bWithAnim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBBMOnTransitionEventDynamicSignature);

/* 독립 전방 선언 */
class UBBMUserWidgetBase;
class UBBMViewModelBase;
class UUserWidget;
class UWidgetAnimation;

/**
 * BBM UI 위젯 ZOrder 레이어 구분 열거형 (순차 0~6 레이어)
 */
UENUM(BlueprintType)
enum class EBBMWidgetZOrder : uint8
{
	/* 최하단 배경 아트 및 맵 오버레이 (0) */
	Background   = 0  UMETA(DisplayName = "Background (0)"),

	/* 기본 인게임 HUD 및 상시 표시 상태바 (1) */
	Base         = 1  UMETA(DisplayName = "Base HUD (1)"),

	/* 인벤토리, 상점, 캐릭터 장비 등 메인 게임플레이 창 (2) */
	Content      = 2  UMETA(DisplayName = "Content / Windows (2)"),

	/* 팝업 창, 모달 대화상자, 확인 창 (3) */
	Popup        = 3  UMETA(DisplayName = "Popups / Modals (3)"),

	/* 알림, 보상 획득 배너, 획득 토스트 메시지 (4) */
	Notification = 4  UMETA(DisplayName = "Notifications / Alerts (4)"),

	/* 화면 암전 FadeInOut 및 레벨/지역 전환 로딩 표기 위젯 (5) */
	FadeLoading  = 5  UMETA(DisplayName = "Fade & Loading (5)"),

	/* 최상단 시스템 메시지, 에러 팝업, 디버그 오버레이 (6) */
	System       = 6  UMETA(DisplayName = "System / Debug (6)")
};

/**
 * BBM 위젯 수명 주기 및 최적화 방식 정책 열거형
 */
UENUM(BlueprintType)
enum class EBBMWidgetLifecyclePolicy : uint8
{
	/* 상시 레이어 유지 (최초 Open 시 AddToLayer, 이후 Open/Close 시 Visibility 및 최상단 ZOrder 전환) */
	PersistentViewport UMETA(DisplayName = "Persistent Viewport (Visibility Toggle)"),

	/* 필요 시에만 레이어 등록 (Open 시 AddToLayer, Close 시 RemoveFromParent하여 렌더 트리 부하 최소화) */
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

public:
	/* 대상 위젯의 WBGC 상속 계층에서 애니메이션을 조회·캐싱하여 반환 */
	UWidgetAnimation* GetAnimation(const UUserWidget* InWidget) const;

	/* 캐싱된 애니메이션 참조 초기화 */
	void InvalidateCachedAnimation() const;

public:
	/* 재생할 위젯 애니메이션 이름 (UMG 에디터 애니메이션 MovieScene 이름) */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	FName AnimationName = NAME_None;

	/* 재생 모드 (절대 시작 3종 + 상대적 반전 2종) */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	EBBMWidgetAnimPlayMode PlayMode = EBBMWidgetAnimPlayMode::RelativeForward;

	/* 재생 속도 배율 (기본값: 1.0f) */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	float PlaybackSpeed = 1.0f;

	/* 재생 반복 횟수 (1: 1회 재생, 0: 무한 반복 / Relative 계열은 엔진 미지원으로 에디터 숨김) */
	UPROPERTY(Category = "BBM|UI|Animation", EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", EditCondition = "PlayMode != EBBMWidgetAnimPlayMode::RelativeForward && PlayMode != EBBMWidgetAnimPlayMode::RelativeReverse", EditConditionHides))
	int32 LoopCount = 1;

private:
	/* 마지막으로 탐색된 애니메이션 약참조 캐시 (Outer WBGC 상속 검사로 타 위젯 혼용 방지) */
	mutable TWeakObjectPtr<UWidgetAnimation> CachedAnimation;
};

/**
 * 단일 UI 애니메이션 전환 상태 열거형
 */
UENUM(BlueprintType)
enum class EBBMSingleAnimState : uint8
{
	/* 대기 및 완료(비재생) 상태 */
	Idle    UMETA(DisplayName = "Idle"),

	/* 애니메이션 전환이 진행 중인 상태 */
	Playing UMETA(DisplayName = "Playing")
};

/**
 * 양방향(Start/End) UI 애니메이션 전환 상태 열거형
 */
UENUM(BlueprintType)
enum class EBBMDualAnimState : uint8
{
	/* 완전히 종료된 초기·대기 상태 */
	Ended    UMETA(DisplayName = "Ended"),

	/* 시작 요청 후 시작 애니메이션 재생 중인 상태 */
	Starting UMETA(DisplayName = "Starting"),

	/* 시작 완료(활성) 상태 */
	Started  UMETA(DisplayName = "Started"),

	/* 종료 요청 후 종료 애니메이션 재생 중인 상태 */
	Ending   UMETA(DisplayName = "Ending")
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

