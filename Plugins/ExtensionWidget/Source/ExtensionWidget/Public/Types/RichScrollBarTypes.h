/*****************************************************************//**
 * @file   RichScrollBarTypes.h
 * @brief  RichScrollBar 및 RichListView 설정용 공용 열거형과 구조체 정의
 * @details 썸 크기 모드(Dynamic/Fixed), 화살표 버튼 옵션(감도·홀드·스타일), 배치 방향 설정을 정의함.
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "RichScrollBarTypes.generated.h"

/**
 * 스크롤바 썸(Thumb) 크기 동작 모드
 */
UENUM(BlueprintType)
enum class ERichScrollBarThumbMode : uint8
{
	/* 콘텐츠 길이에 비례하여 썸 크기가 동적으로 조절되는 기존 방식 */
	Dynamic UMETA(DisplayName = "Dynamic"),

	/* 썸 크기를 고정 픽셀로 유지하고 콘텐츠 길이에 따라 드래그 감도가 스케일링되는 방식 */
	Fixed UMETA(DisplayName = "Fixed")
};

/**
 * RichListView 내부에서 RichScrollBar가 배치될 방향
 */
UENUM(BlueprintType)
enum class ERichScrollBarPlacement : uint8
{
	/* 수직 리스트에서는 우측, 수평 리스트에서는 하단에 배치 (기본값) */
	End UMETA(DisplayName = "End (Right / Bottom)"),

	/* 수직 리스트에서는 좌측, 수평 리스트에서는 상단에 배치 */
	Start UMETA(DisplayName = "Start (Left / Top)")
};

/**
 * 스크롤바 양단 화살표 버튼 및 홀드 연속 스크롤 설정 구조체
 */
USTRUCT(BlueprintType)
struct EXTENSIONWIDGET_API FRichScrollBarArrowConfig
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FRichScrollBarArrowConfig();

	/* 화살표 버튼 표시 및 감도 설정 */
public:
	/* 양단에 상하/좌우 화살표 버튼을 표시할지 여부 */
	UPROPERTY(Category = "Arrow", EditAnywhere, BlueprintReadWrite)
	bool bShowArrowButtons;

	/* 스크롤 축 방향으로 화살표 버튼이 차지하는 길이 (Slate 단위) */
	UPROPERTY(Category = "Arrow", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons", ClampMin = "4.0"))
	float ArrowButtonLength;

	/* 화살표 버튼 주변 여백 (Start 화살표 기준, 반대쪽 End 화살표는 진행 축 방향으로 자동 반전됨) */
	UPROPERTY(Category = "Arrow|Appearance", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons"))
	FMargin ArrowPadding;

	/* 화살표 버튼 1회 클릭 시 이동할 스크롤 양 (ListView에서는 Slate 픽셀 단위, 단독 ScrollBar에서는 StandaloneReferenceExtent 기준) */
	UPROPERTY(Category = "Arrow", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons", ClampMin = "1.0"))
	float ClickScrollStep;

	/* 화살표 버튼을 누르고 있을 때 연속 스크롤을 수행할지 여부 */
	UPROPERTY(Category = "Arrow", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons"))
	bool bEnableHoldScroll;

	/* 버튼 홀드 시 초당 이동할 연속 스크롤 속도 (Slate 픽셀 단위) */
	UPROPERTY(Category = "Arrow", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons && bEnableHoldScroll", ClampMin = "1.0"))
	float HoldScrollSpeed;

	/* 버튼을 누른 직후 연속 홀드 스크롤이 시작되기까지의 초기 대기 시간 (초) */
	UPROPERTY(Category = "Arrow", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons && bEnableHoldScroll", ClampMin = "0.0"))
	float HoldInitialDelay;

	/* 연속 홀드 상태에서 스크롤 틱을 적용할 최소 간격 (초, 0이면 매 프레임 적용) */
	UPROPERTY(Category = "Arrow", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons && bEnableHoldScroll", ClampMin = "0.0"))
	float HoldRepeatInterval;

	/* URichScrollBar 단독 사용 시 픽셀 기반 Step/Speed를 정규화 비율(0~1)로 환산할 기준 전체 길이 */
	UPROPERTY(Category = "Arrow", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons", ClampMin = "10.0"))
	float StandaloneReferenceExtent;

	/* 커스텀 스타일 미사용 시 기본 그려지는 벡터 화살표 심볼 색상 */
	UPROPERTY(Category = "Arrow|Appearance", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons"))
	FLinearColor DefaultArrowGlyphColor;

	/* 별도 Start/End FButtonStyle 브러시를 사용할지 여부 (false면 기본 벡터 화살표 심볼을 사용) */
	UPROPERTY(Category = "Arrow|Appearance", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons"))
	bool bUseCustomArrowStyle;

	/* 상단(수직) 또는 좌측(수평) 화살표 버튼 스타일 */
	UPROPERTY(Category = "Arrow|Appearance", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons && bUseCustomArrowStyle"))
	FButtonStyle StartArrowStyle;

	/* 하단(수직) 또는 우측(수평) 화살표 버튼 스타일 */
	UPROPERTY(Category = "Arrow|Appearance", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bShowArrowButtons && bUseCustomArrowStyle"))
	FButtonStyle EndArrowStyle;
};

/**
 * RichScrollBar의 썸 모드 및 화살표 구성을 묶는 최상위 설정 구조체
 */
USTRUCT(BlueprintType)
struct EXTENSIONWIDGET_API FRichScrollBarConfig
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FRichScrollBarConfig();

	/* 스크롤바 동작 옵션 */
public:
	/* 썸 크기 결정 모드 (Dynamic: 비례 크기 / Fixed: 고정 크기) */
	UPROPERTY(Category = "Thumb", EditAnywhere, BlueprintReadWrite)
	ERichScrollBarThumbMode ThumbMode;

	/* Fixed 모드일 때 썸의 고정 길이 (Slate 단위, 트랙 길이를 넘으면 자동 클램핑됨) */
	UPROPERTY(Category = "Thumb", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ThumbMode == ERichScrollBarThumbMode::Fixed", ClampMin = "8.0"))
	float FixedThumbLength;

	/* Dynamic 모드일 때 보장할 최소 썸 길이 (Slate 단위) */
	UPROPERTY(Category = "Thumb", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ThumbMode == ERichScrollBarThumbMode::Dynamic", ClampMin = "8.0"))
	float MinDynamicThumbLength;

	/* 양단 화살표 버튼 옵션 */
	UPROPERTY(Category = "Arrow", EditAnywhere, BlueprintReadWrite)
	FRichScrollBarArrowConfig ArrowConfig;
};
