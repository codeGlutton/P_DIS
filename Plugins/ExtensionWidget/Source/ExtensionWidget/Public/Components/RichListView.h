/*****************************************************************//**
 * @file   RichListView.h
 * @brief  UListView를 확장하여 SRichScrollBar를 통합한 UMG 리스트 뷰 컴포넌트
 * @details Option A 아키텍처를 따라 UListView의 가상화, 선택, MVVM EntryViewModel 바인딩을
 *          100% 그대로 유지하면서 SRichScrollBar(고정/동적 썸·화살표 버튼·일체형 가시성)를 결합함.
 * @date   2026-11-15
 *********************************************************************/

#pragma once

#include "Components/ListView.h"
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Types/RichScrollBarTypes.h"
#include "RichListView.generated.h"

/* 독립 전방 선언 */
class SRichScrollBar;

template <typename ItemType>
class SRichListView;

/**
 * UListView 파생 확장 리스트 뷰 위젯 (URichListView)
 */
UCLASS()
class EXTENSIONWIDGET_API URichListView : public UListView
{
	GENERATED_BODY()

	/* 생성자 */
public:
	URichListView(const FObjectInitializer& Initializer);

	/* 확장 스크롤바 런타임 제어 API */
public:
	/* RichScrollBar의 썸 모드 및 화살표 설정을 런타임에 갱신함 */
	UFUNCTION(Category = "Rich ListView", BlueprintCallable)
	void SetRichScrollBarConfig(const FRichScrollBarConfig& InConfig);

	/* RichScrollBar의 상시 표기 여부를 런타임에 변경함 */
	UFUNCTION(Category = "Rich ListView", BlueprintCallable)
	void SetAlwaysShowRichScrollbar(bool bInAlwaysShow);

	/* UListView의 ScrollBarStyle을 런타임에 변경하고 RichScrollBar에 즉시 반영함 */
	UFUNCTION(Category = "Rich ListView", BlueprintCallable)
	void SetRichScrollBarStyle(const FScrollBarStyle& InStyle);

	/* 런타임/테스트에서 엔트리 위젯 클래스를 지정할 때 사용함 (RebuildWidget 이전 호출 권장) */
	void SetEntryWidgetClass(TSubclassOf<UUserWidget> InEntryWidgetClass);

	/* 내부 Slate RichScrollBar 인스턴스 조회 (C++ 및 테스트용) */
	TSharedPtr<SRichScrollBar> GetRichScrollBar() const;

	/* UWidget / UListView 인터페이스 */
public:
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<STableViewBase> RebuildListWidget() override;

	/* 내부 동기화 헬퍼 */
private:
	TSharedPtr<SRichListView<UObject*>> GetSlateRichListView() const;
	void ApplyRichScrollBarProperties();

	/* 확장 스크롤바 설정 프로퍼티 */
public:
	/* 썸 크기 모드(Dynamic/Fixed) 및 화살표 버튼 동작 설정 */
	UPROPERTY(Category = "Rich ScrollBar", EditAnywhere, BlueprintReadWrite)
	FRichScrollBarConfig RichScrollBarConfig;

	/* 스크롤할 항목이 부족할 때도 RichScrollBar(트랙·썸·화살표)를 항상 표시할지 여부 */
	UPROPERTY(Category = "Rich ScrollBar|Behavior", EditAnywhere, BlueprintReadWrite)
	bool bAlwaysShowRichScrollbar;

	/* 리스트 대비 스크롤바 배치 방향 (Vertical: 우/좌, Horizontal: 하/상) */
	UPROPERTY(Category = "Rich ScrollBar|Appearance", EditAnywhere, BlueprintReadOnly)
	ERichScrollBarPlacement RichScrollbarPlacement;
};
