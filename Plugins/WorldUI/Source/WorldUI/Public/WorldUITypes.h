/*****************************************************************//**
 * @file   WorldUITypes.h
 * @brief  월드 위젯 및 MVVM 전환 프레임워크 공용 타입 정의
 * @details ZOrder 레이어, 수명 주기 정책, 애니메이션 설정/재생 모드, 전환 상태 및 생성 설정 구조체 선언
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldUITypes.generated.h"

/* 애니메이션 전환 요청 및 완료 대리자 선언 */
DECLARE_DELEGATE_OneParam(FWorldUIOnPresentationTransitionRequestedSignature, bool /*bWithAnim*/);
DECLARE_DYNAMIC_DELEGATE_OneParam(FWorldUIOnWidgetAnimFinishedDynamicSignature, bool, bInterrupted);
DECLARE_DELEGATE_OneParam(FWorldUIOnWidgetAnimFinishedSignature, bool /*bInterrupted*/);

/* 애니메이션 전환 라이프사이클 브로드캐스트 네이티브·동적 대리자 */
DECLARE_MULTICAST_DELEGATE_OneParam(FWorldUIOnAnimTransitionStartedNativeSignature, bool /*bWithAnim*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FWorldUIOnAnimTransitionFinishedNativeSignature, bool /*bInterrupted*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWorldUIOnTransitionStartedDynamicSignature, bool, bWithAnim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWorldUIOnTransitionFinishedDynamicSignature, bool, bInterrupted);

/* 독립 전방 선언 */
class UWorldUserWidgetBase;
class UWorldViewModelBase;
class UUserWidget;
class UWidgetAnimation;

/**
 * 월드 UI 위젯 ZOrder 레이어 구분 열거형 (순차 0~7 레이어)
 */
UENUM(BlueprintType)
enum class EWorldWidgetZOrder : uint8
{
	/* 최하단 배경 아트 및 맵 오버레이 (0) */
	Background          = 0  UMETA(DisplayName = "Background (0)"),

	/* 기본 인게임 HUD 및 상시 표시 상태바 (1) */
	Base                = 1  UMETA(DisplayName = "Base HUD (1)"),

	/* 인벤토리, 상점, 캐릭터 장비 등 메인 게임플레이 창 (2) */
	Content             = 2  UMETA(DisplayName = "Content / Windows (2)"),

	/* 팝업 창, 모달 대화상자, 확인 창 (3) */
	Popup               = 3  UMETA(DisplayName = "Popups / Modals (3)"),

	/* 알림, 보상 획득 배너, 획득 토스트 메시지 (4) */
	Notification        = 4  UMETA(DisplayName = "Notifications / Alerts (4)"),

	/* 화면 암전 FadeInOut 및 레벨/지역 전환 로딩 표기 위젯 (5) */
	FadeLoading         = 5  UMETA(DisplayName = "Fade & Loading (5)"),

	/* 로딩 화면 위에서도 노출되어야 하는 상단 알림 및 진행 인디케이터 (예: 저장 중 표기 UI) (6) */
	OverlayNotification = 6  UMETA(DisplayName = "Overlay Notification (6)"),

	/* 최상단 시스템 메시지, 에러 팝업, 디버그 오버레이 (7) */
	System              = 7  UMETA(DisplayName = "System / Debug (7)")
};

/**
 * 위젯 수명 주기 및 최적화 방식 정책 열거형
 */
UENUM(BlueprintType)
enum class EWorldWidgetLifecyclePolicy : uint8
{
	/* 상시 레이어 유지 (최초 Open 시 AddToLayer, 이후 Open/Close 시 Visibility 및 최상단 ZOrder 전환) */
	PersistentViewport UMETA(DisplayName = "Persistent Viewport (Visibility Toggle)"),

	/* 필요 시에만 레이어 등록 (Open 시 AddToLayer, Close 시 RemoveFromParent하여 렌더 트리 부하 최소화) */
	DynamicDetach      UMETA(DisplayName = "Dynamic Detach (Attach On Open / Detach On Close)")
};

/**
 * 위젯 애니메이션 재생 모드 열거형
 * (EUMGSequencePlayMode 절대 시작 3종 + 현재 진행 상태 기반 상대 재생 2종)
 */
UENUM(BlueprintType)
enum class EWorldWidgetAnimPlayMode : uint8
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
 * 위젯 애니메이션 재생 설정 구조체
 */
USTRUCT(BlueprintType)
struct WORLDUI_API FWorldWidgetAnimConfig
{
	GENERATED_BODY()

	/* 애니메이션 조회 및 캐시 제어 */
public:
	/* 대상 위젯 자신 및 하위 위젯 트리를 순회하며 애니메이션과 소유 위젯을 조회·캐싱하여 반환 */
	UWidgetAnimation* GetAnimation(UUserWidget* InWidget) const;

	/* 캐싱된(또는 탐색된) 애니메이션의 실제 소유 위젯 인스턴스를 반환 */
	UUserWidget* GetOwnerWidget(UUserWidget* InWidget) const;

	/* 캐싱된 애니메이션·대상·소유 위젯 참조 초기화 */
	void InvalidateCachedAnimation() const;

	/* 내부 탐색 헬퍼 */
private:
	/* TargetWidget 일치 시 캐시 히트, 불일치 시 자기 자신 및 하위 위젯 트리를 재귀 탐색해 캐시 갱신 */
	bool ResolveAnimation(UUserWidget* InWidget) const;

	/* 설정 프로퍼티 */
public:
	/* 재생할 위젯 애니메이션 이름 (UMG 에디터 애니메이션 MovieScene 이름) */
	UPROPERTY(Category = "WorldUI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	FName AnimationName = NAME_None;

	/* 재생 모드 (절대 시작 3종 + 상대적 반전 2종) */
	UPROPERTY(Category = "WorldUI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	EWorldWidgetAnimPlayMode PlayMode = EWorldWidgetAnimPlayMode::RelativeForward;

	/* 재생 속도 배율 (기본값: 1.0f) */
	UPROPERTY(Category = "WorldUI|Animation", EditDefaultsOnly, BlueprintReadWrite)
	float PlaybackSpeed = 1.0f;

	/* 재생 반복 횟수 (1: 1회 재생, 0: 무한 반복 / Relative 계열은 엔진 미지원으로 에디터 숨김) */
	UPROPERTY(Category = "WorldUI|Animation", EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", EditCondition = "PlayMode != EWorldWidgetAnimPlayMode::RelativeForward && PlayMode != EWorldWidgetAnimPlayMode::RelativeReverse", EditConditionHides))
	int32 LoopCount = 1;

	/* 캐시 참조 */
private:
	/* 캐시 적중 판별용 루트 탐색 대상 위젯 약참조 */
	mutable TWeakObjectPtr<UUserWidget> CachedTargetWidget;

	/* 마지막으로 탐색된 애니메이션을 실제로 소유한 위젯 인스턴스 약참조 */
	mutable TWeakObjectPtr<UUserWidget> CachedOwnerWidget;

	/* 마지막으로 탐색된 애니메이션 약참조 캐시 */
	mutable TWeakObjectPtr<UWidgetAnimation> CachedAnimation;
};

/**
 * 단일 UI 애니메이션 전환 상태 열거형
 */
UENUM(BlueprintType)
enum class ESingleAnimState : uint8
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
enum class EDualAnimState : uint8
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
 * 순수 월드 위젯 생성 설정 구조체
 */
USTRUCT(BlueprintType)
struct WORLDUI_API FWorldWidgetConfig
{
	GENERATED_BODY()

	/* 프로퍼티 */
public:
	/* 생성할 위젯 클래스 */
	UPROPERTY(Category = "WorldUI", EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UWorldUserWidgetBase> WidgetClass = nullptr;

	/* 뷰포트 ZOrder 레이어 오버라이드 여부 (false일 경우 위젯 클래스 자체의 DefaultZOrderLayer 유지) */
	UPROPERTY(Category = "WorldUI", EditDefaultsOnly, BlueprintReadOnly)
	bool bOverrideZOrder = false;

	/* 오버라이드할 뷰포트 ZOrder 레이어 (bOverrideZOrder가 true일 때만 수정 및 적용) */
	UPROPERTY(Category = "WorldUI", EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverrideZOrder"))
	EWorldWidgetZOrder ZOrderLayer = EWorldWidgetZOrder::Base;
};

/**
 * 순수 월드 ViewModel 생성 설정 구조체
 */
USTRUCT(BlueprintType)
struct WORLDUI_API FWorldViewModelConfig
{
	GENERATED_BODY()

	/* 프로퍼티 */
public:
	/* 생성할 MVVM ViewModel 클래스 */
	UPROPERTY(Category = "WorldUI|MVVM", EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UWorldViewModelBase> ViewModelClass = nullptr;
};