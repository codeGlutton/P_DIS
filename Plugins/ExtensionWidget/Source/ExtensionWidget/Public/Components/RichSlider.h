/*****************************************************************//**
 * @file   RichSlider.h
 * @brief  USlider에서 파생하여 채움 트랙 렌더링을 지원하는 UMG 슬라이더 위젯
 * @details USlider의 기존 프로퍼티·바인딩·FieldNotify·접근성 계약을 중복 없이 그대로 계승하면서
 *          SRichSlider를 생성·동기화하고 RichStyle(채움 바 브러시/여백) 및 SliderFillBarColor를 제공함.
 * @date   2026-11-17
 *********************************************************************/

#pragma once

#include "Components/Slider.h"
#include "CoreMinimal.h"
#include "Types/RichSliderTypes.h"
#include "RichSlider.generated.h"

/* 독립 전방 선언 */
class SRichSlider;

/**
 * USlider 파생 확장 슬라이더 위젯 (URichSlider)
 */
UCLASS()
class EXTENSIONWIDGET_API URichSlider : public USlider
{
	GENERATED_BODY()

	/* 생성자 */
public:
	URichSlider(const FObjectInitializer& ObjectInitializer);

	/* 확장 스타일 및 채움 색상 제어 API */
public:
	/* 채움 바 확장 스타일을 반환함 */
	const FRichSliderStyle& GetRichStyle() const { return RichStyle; }

	/* 채움 바 확장 스타일을 갱신하고 Slate 위젯에 즉시 반영함 */
	UFUNCTION(Category = "Style", BlueprintCallable)
	void SetRichStyle(const FRichSliderStyle& InStyle);

	/* 채움 바 틴트 색상을 반환함 */
	FLinearColor GetSliderFillBarColor() const { return SliderFillBarColor; }

	/* 채움 바 틴트 색상을 변경하고 Slate 위젯에 즉시 반영함 */
	UFUNCTION(Category = "Appearance", BlueprintCallable)
	void SetSliderFillBarColor(FLinearColor InValue);

	/* 내부 Slate RichSlider 인스턴스 조회 (C++ 및 테스트용) */
	TSharedPtr<SRichSlider> GetSlateRichSlider() const;

	/* UWidget 인터페이스 */
public:
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/* 확장 스타일 및 채움 색상 프로퍼티 (USlider 기존 프로퍼티와 중복 없음) */
public:
	/* 채움 바 확장 스타일 */
	UPROPERTY(Category = "Style", EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetRichStyle")
	FRichSliderStyle RichStyle;

	/* 채움 바 틴트 색상 */
	UPROPERTY(Category = "Appearance", EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetSliderFillBarColor")
	FLinearColor SliderFillBarColor;
};
