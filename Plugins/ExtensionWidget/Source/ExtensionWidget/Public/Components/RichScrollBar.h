/*****************************************************************//**
 * @file   RichScrollBar.h
 * @brief  UWidget에서 직접 파생한 확장 UMG 스크롤바 컴포넌트
 * @details 언리얼 UScrollBar와 동일한 부모(UWidget)에서 파생하여 기존 프로퍼티 계약
 *          (WidgetStyle, Orientation, Thickness, Padding, bAlwaysShowScrollbar, SetState)과
 *          고정/동적 썸 및 화살표 버튼 구성을 함께 제공함.
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Types/RichScrollBarTypes.h"
#include "RichScrollBar.generated.h"

/* 독립 전방 선언 */
class SRichScrollBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRichScrollBarUserScrolledEvent, float, CurrentOffsetFraction);

/**
 * UWidget 파생 확장 UMG 스크롤바 (URichScrollBar)
 */
UCLASS()
class EXTENSIONWIDGET_API URichScrollBar : public UWidget
{
	GENERATED_BODY()

	/* 생성자 */
public:
	URichScrollBar(const FObjectInitializer& ObjectInitializer);

	/* 스크롤바 제어 및 상태 조회 API */
public:
	/* 썸 위치와 크기 비율(0~1)을 갱신함 */
	UFUNCTION(Category = "Rich ScrollBar", BlueprintCallable)
	void SetState(float InOffsetFraction, float InThumbSizeFraction);

	/* 확장 스크롤바 설정을 런타임에 갱신함 */
	UFUNCTION(Category = "Rich ScrollBar", BlueprintCallable)
	void SetRichConfig(const FRichScrollBarConfig& InConfig);

	/* 스크롤바 상시 표기 여부를 변경함 */
	UFUNCTION(Category = "Rich ScrollBar", BlueprintCallable)
	void SetAlwaysShowScrollbar(bool bInAlwaysShow);

	/* 스크롤 축 방향을 변경함 */
	UFUNCTION(Category = "Rich ScrollBar", BlueprintCallable)
	void SetOrientation(EOrientation InOrientation);

	/* 스크롤바 두께를 변경함 */
	UFUNCTION(Category = "Rich ScrollBar", BlueprintCallable)
	void SetThickness(const FVector2D& InThickness);

	/* 내부 패딩을 변경함 */
	UFUNCTION(Category = "Rich ScrollBar", BlueprintCallable)
	void SetPadding(const FMargin& InPadding);

	/* 현재 정규화 스크롤 오프셋을 반환함 */
	UFUNCTION(Category = "Rich ScrollBar", BlueprintPure)
	float GetScrollOffsetFraction() const;

	/* 현재 정규화 썸 크기 비율을 반환함 */
	UFUNCTION(Category = "Rich ScrollBar", BlueprintPure)
	float GetThumbSizeFraction() const;

	/* 스크롤이 실제로 필요한 상태인지 반환함 */
	UFUNCTION(Category = "Rich ScrollBar", BlueprintPure)
	bool IsScrollNeeded() const;

	TSharedPtr<SRichScrollBar> GetSlateRichScrollBar() const;

	/* UWidget 인터페이스 */
public:
	virtual void SetVisibility(ESlateVisibility InVisibility) override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/* 내부 이벤트 중계 */
private:
	void HandleSlateUserScrolled(float InNewOffsetFraction);

	/* 스크롤바 스타일 및 동작 설정 프로퍼티 */
public:
	/* 트랙과 썸의 시각 브러시 스타일 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadWrite)
	FScrollBarStyle WidgetStyle;

	/* 콘텐츠가 적을 때도 스크롤바(트랙·썸·화살표)를 항상 표시할지 여부 */
	UPROPERTY(Category = "Behavior", EditAnywhere, BlueprintReadWrite)
	bool bAlwaysShowScrollbar;

	/* 스크롤바 배치 축 (Vertical / Horizontal) */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadOnly)
	TEnumAsByte<EOrientation> Orientation;

	/* 스크롤바 두께 (Vertical: X, Horizontal: Y) */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite)
	FVector2D Thickness;

	/* 스크롤바 내부 여백 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite)
	FMargin Padding;

	/* 고정/동적 썸 및 화살표 버튼 설정 */
	UPROPERTY(Category = "Rich ScrollBar", EditAnywhere, BlueprintReadWrite)
	FRichScrollBarConfig RichConfig;

	/* 사용자 스크롤 시 발행되는 블루프린트 이벤트 */
	UPROPERTY(Category = "Widget|Event", BlueprintAssignable)
	FOnRichScrollBarUserScrolledEvent OnUserScrolled;

	/* 내부 Slate 포인터 */
protected:
	TSharedPtr<SRichScrollBar> MyRichScrollBar;
};
