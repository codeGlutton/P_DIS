/*****************************************************************//**
 * @file   RichSliderTypes.h
 * @brief  RichSlider의 채움 바 확장 스타일 구조체 정의
 * @details FSliderStyle과 동일한 상태 체계(Normal/Hovered/Disabled)로 채움 바(FillBar) 브러시와
 *          내부 여백(FillBarBorderPadding)만 확장 정의하며 중복 프로퍼티를 두지 않음.
 * @date   2026-11-17
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateWidgetStyle.h"
#include "RichSliderTypes.generated.h"

/**
 * RichSlider 전용 확장 스타일 구조체 (FSliderStyle과 함께 사용)
 */
USTRUCT(BlueprintType)
struct EXTENSIONWIDGET_API FRichSliderStyle : public FSlateWidgetStyle
{
	GENERATED_BODY()

	/* 생성자 및 FSlateWidgetStyle 인터페이스 */
public:
	FRichSliderStyle();
	virtual ~FRichSliderStyle() override = default;

	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	virtual const FName GetTypeName() const override { return TypeName; }

	/* 기본 확장 스타일 싱글톤 반환 */
	static const FRichSliderStyle& GetDefault();

	/* 채움 바 확장 브러시 */
public:
	static const FName TypeName;

	/* 기본 상태 채움 바 브러시 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite)
	FSlateBrush NormalFillBarImage;

	/* 호버 상태 채움 바 브러시 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite)
	FSlateBrush HoveredFillBarImage;

	/* 비활성/잠금 상태 채움 바 브러시 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite)
	FSlateBrush DisabledFillBarImage;

	/* 배경 트랙 내부에 채움 바를 배치할 때 적용할 선택적 여백 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite)
	FMargin FillBarBorderPadding;
};
