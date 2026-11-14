/*****************************************************************//**
 * @file   BBMLayerRootWidget.h
 * @brief  BBM(비트 배낭 상인) UI 레이어 루트 컨테이너 위젯 클래스 정의
 * @details 뷰포트에 단 하나만 등록되어 EBBMWidgetZOrder별 UCanvasPanel 레이어 계층, ZOrder 증감 및 자동 정규화를 총괄함
 * @date   2026-11-14
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/BBMWorldWidgetTypes.h"
#include "BBMLayerRootWidget.generated.h"

/* 독립 전방 선언 */
class UCanvasPanel;
class UCanvasPanelSlot;
class UWidget;

/**
 * 레이어 캔버스별 ZOrder 할당 상태 구조체
 */
USTRUCT()
struct FBBMUILayerState
{
	GENERATED_BODY()

public:
	/* 해당 EBBMWidgetZOrder 레이어에 대응하는 캔버스 패널 */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> LayerCanvas = nullptr;

	/* 해당 레이어에서 다음에 할당할 정수형 슬롯 ZOrder 카운터 */
	UPROPERTY(Transient)
	int32 NextZOrder = 0;
};

/**
 * BBM 루트 UI 레이어 컨테이너 위젯
 * EBBMWidgetZOrder 열거형 항목마다 독립된 UCanvasPanel과 ZOrder 카운터를 유지하여 레이어 정렬과 자동 정규화를 수행함
 */
UCLASS(NotBlueprintable, Transient, meta = (DisplayName = "BBM Layer Root Widget"))
class BBM_API UBBMLayerRootWidget : public UUserWidget
{
	GENERATED_BODY()

	/* 생성자 및 UUserWidget 오버라이드 */
public:
	UBBMLayerRootWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/* 레이어 구축 및 위젯 배치·정렬 인터페이스 */
public:
	/**
	 * 루트 UCanvasPanel과 EBBMWidgetZOrder별 자식 UCanvasPanel 계층 및 초기 상태를 1회 구축함
	 */
	void InitializeLayerStructure();

	/**
	 * 대상 위젯을 지정된 레이어 캔버스 자식으로 추가하고 전체 스트레치 레이아웃과 순차 ZOrder를 부여함
	 * (이미 뷰포트 또는 레이어에 올라가 있으면 중복 추가하지 않고 false 반환)
	 * @param InWidget 추가할 위젯 인스턴스
	 * @param InLayer 배치할 대상 ZOrder 레이어
	 * @return 레이어 추가 성공 여부
	 */
	bool AddWidgetToLayer(UUserWidget* InWidget, EBBMWidgetZOrder InLayer);

	/**
	 * 이미 레이어에 올라가 있는 위젯의 슬롯 ZOrder를 해당 레이어 최상단으로 승격함
	 * @param InWidget 최상단으로 이동할 위젯 인스턴스
	 * @return 최상단 이동 성공 여부 (레이어 미소속 시 false)
	 */
	bool BringWidgetToLayerTop(UUserWidget* InWidget);

	/**
	 * 지정 위젯이 루트 레이어 캔버스 중 하나의 직계 자식인지 확인함
	 */
	bool IsWidgetInAnyLayer(const UWidget* InWidget) const;

	/**
	 * 지정 위젯이 소속된 레이어 열거형을 탐색해 반환함
	 */
	bool TryGetWidgetLayer(const UWidget* InWidget, EBBMWidgetZOrder& OutLayer) const;

#if WITH_EDITOR
	/**
	 * 에디터 자동화 테스트 검증을 위해 레이어 ZOrder 정규화 임계치를 임시 조정함
	 */
	void SetLayerZOrderMaxThresholdForTesting(int32 InThreshold);
#endif

	/* 내부 정규화 및 슬롯 헬퍼 */
private:
	/* 필요 시 ZOrder를 정규화한 뒤 다음 정수 ZOrder 값을 발급하고 카운터를 증가시킴 */
	int32 AllocateNextLayerZOrder(EBBMWidgetZOrder InLayer, FBBMUILayerState& InOutLayerState);

	/* 지정 레이어 내 모든 위젯 슬롯의 ZOrder 순서를 유지한 채 0부터 재할당(정규화)함 */
	void NormalizeLayerZOrders(EBBMWidgetZOrder InLayer);

	/* 캔버스 슬롯을 AddToViewport 기본값과 동일한 전체 화면 스트레치 레이아웃으로 설정함 */
	static void ConfigureFullScreenCanvasSlot(UCanvasPanelSlot* InSlot, int32 InZOrder);

	/* 내부 레이어 계층 및 상태 */
private:
	/* 모든 레이어 캔버스를 수용하는 최상위 루트 캔버스 */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	/* EBBMWidgetZOrder별 캔버스 및 다음 ZOrder 정수 카운터 상태 맵 */
	UPROPERTY(Transient)
	TMap<EBBMWidgetZOrder, FBBMUILayerState> LayerStates;

	/* 자동 ZOrder 정규화가 발동되는 정수 카운터 상한 임계치 (float 정확 정수 범위 내) */
	int32 MaxLayerZOrderThreshold;
};
