/*****************************************************************//**
 * @file   DISTileShapeCustomization.h
 * @brief  FDISTileShape 2D 그리드 타일 시각화 및 편집용 프로퍼티 커스터마이제이션
 * @date   2026-10-14
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Layout/Visibility.h"

class IPropertyHandle;
class IPropertyUtilities;
class SGridPanel;
class SVerticalBox;

/**
 * FDISTileShape 구조체를 에디터 디테일 패널에서 2D 타일 매트릭스 그리드로 표시/편집하는 커스터마이제이션
 */
class FDISTileShapeCustomization : public IPropertyTypeCustomization
{
	/* 생성 및 소멸 */
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	virtual ~FDISTileShapeCustomization() override;

	/* IPropertyTypeCustomization 인터페이스 */
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	/* 내부 헬퍼 및 이벤트 핸들러 */
private:
	/* 현재 프로퍼티 핸들에서 Points 배열 읽어오기 */
	TArray<FIntPoint> GetPointsFromHandle() const;

	/* Points 배열을 프로퍼티 핸들에 저장 */
	void SetPointsToHandle(const TArray<FIntPoint>& NewPoints);

	/* 타일 클릭 토글 핸들러 */
	FReply OnTileClicked(int32 X, int32 Y);

	/* 상/하/좌/우 빈 여백 제거 및 좌상단 밀착 정렬 (Trim / Normalize) */
	FReply OnNormalizeClicked();

	/* 모든 타일 점유 비우기 (Reset) */
	FReply OnResetClicked();

	/* 슬레이트 타일 매트릭스 그리드 갱신 */
	void RefreshGridWidget();

	/* 우측 Points 좌표 읽기 전용 목록 위젯 갱신 */
	void RefreshPointsListWidget();

	/* 개별 타일 배경 브러시 색상 반환 */
	FSlateColor GetTileColor(int32 X, int32 Y) const;

	/* 요약 정보 텍스트 반환 (예: "3 tiles (2x2)") */
	FText GetSummaryText() const;

	/* Trim 버튼 표시 여부 반환 (EditorFixedGridDimensions가 설정되어 있지 않을 때만 Visible) */
	EVisibility GetTrimButtonVisibility() const;

	/* 고정 그리드 규격이 유효하게 설정되어 있는지 여부 */
	bool HasFixedGridDimensions() const;

	/* 설정된 고정 그리드 규격 반환 (미설정 시 FIntPoint::ZeroValue) */
	FIntPoint GetFixedGridDimensions() const;

	/* 현재 편집 대상에 유효하게 허용되는 최대 2D 그리드 규격 반환 (오버라이드, 인벤토리 컨텍스트, 전역 설정 순으로 평가) */
	FIntPoint GetMaxAllowedDimensions() const;

	/* 외부 UObject의 프로퍼티 변경 감지 핸들러 */
	void OnObjectPropertyChanged(UObject* InObject, struct FPropertyChangedEvent& InPropertyChangedEvent);

	/* 내부 프로퍼티 핸들 및 슬레이트 위젯 */
private:
	/* Shape 구조체 루트 프로퍼티 핸들 */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	/* 프로퍼티 에디터 유틸리티 (디테일 뷰 리프레시 등에 활용) */
	TSharedPtr<IPropertyUtilities> PropertyUtilities;

	/* 외부 오브젝트 프로퍼티 변경 감지 델리게이트 핸들 */
	FDelegateHandle OnPropertyChangedHandle;

	/* 좌측 슬레이트 타일 그리드가 동적으로 빌드되는 컨테이너 */
	TSharedPtr<SGridPanel> GridContainer;

	/* 우측 Points 좌표 읽기 전용 목록이 동적으로 빌드되는 컨테이너 */
	TSharedPtr<SVerticalBox> PointsListContainer;
};
