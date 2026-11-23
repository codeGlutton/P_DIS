/*****************************************************************//**
 * @file   RichHorizontalBox.h
 * @brief  UHorizontalBox에서 파생한 렌더링·HitTest 역순 지원 UMG 수평 박스 위젯
 * @details 기존 UHorizontalBoxSlot 배치를 그대로 유지하면서 bReversePaint 옵션을 통해
 *          자식 위젯의 그리기 순서와 마우스 Hit 우선순위만 역전시킴.
 * @date   2026-11-16
 *********************************************************************/

#pragma once

#include "Components/HorizontalBox.h"
#include "CoreMinimal.h"
#include "RichHorizontalBox.generated.h"

/* 독립 전방 선언 */
class SRichHorizontalBox;

/**
 * UHorizontalBox 파생 확장 수평 박스 위젯 (URichHorizontalBox)
 */
UCLASS()
class EXTENSIONWIDGET_API URichHorizontalBox : public UHorizontalBox
{
	GENERATED_BODY()

	/* 생성자 */
public:
	URichHorizontalBox(const FObjectInitializer& ObjectInitializer);

	/* 렌더링 순서 제어 API */
public:
	/* 역순 렌더링·HitTest 활성화 여부를 런타임에 변경하고 즉시 반영함 */
	UFUNCTION(Category = "Rich Box|Behavior", BlueprintCallable)
	void SetReversePaint(bool bInReversePaint);

	/* 내부 Slate RichHorizontalBox 인스턴스 조회 (C++ 및 테스트용) */
	TSharedPtr<SRichHorizontalBox> GetSlateRichHorizontalBox() const;

	/* UWidget 인터페이스 */
public:
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/* 내부 동기화 헬퍼 */
private:
	void ApplyReversePaint();

	/* 확장 박스 설정 프로퍼티 */
public:
	/* 앞쪽 슬롯이 나중에 그려지고 먼저 Hit 되도록 그리기·Hit 순서를 역전할지 여부 */
	UPROPERTY(Category = "Rich Box|Behavior", EditAnywhere, BlueprintReadWrite)
	bool bReversePaint;
};
