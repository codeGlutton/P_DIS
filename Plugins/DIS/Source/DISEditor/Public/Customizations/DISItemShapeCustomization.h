/*****************************************************************//**
 * @file   DISItemShapeCustomization.h
 * @brief  FDISItemShape 2D 그리드 타일 시각화 및 편집용 프로퍼티 커스터마이제이션
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class SGridPanel;
class SVerticalBox;

/**
 * FDISItemShape 구조체를 에디터 디테일 패널에서 2D 타일 매트릭스 그리드로 표시/편집하는 커스터마이제이션
 */
class FDISItemShapeCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

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

	/* 기본 1x1 단일 점유로 초기화 (Reset) */
	FReply OnResetClicked();

	/* 슬레이트 타일 매트릭스 그리드 갱신 */
	void RefreshGridWidget();

	/* 우측 Points 좌표 읽기 전용 목록 위젯 갱신 */
	void RefreshPointsListWidget();

	/* 개별 타일 배경 브러시 색상 반환 */
	FSlateColor GetTileColor(int32 X, int32 Y) const;

	/* 요약 정보 텍스트 반환 (예: "3 tiles (2x2)") */
	FText GetSummaryText() const;

	/* 내부 프로퍼티 핸들 및 슬레이트 위젯 */
private:
	/* Shape 구조체 루트 프로퍼티 핸들 */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	/* 좌측 슬레이트 타일 그리드가 동적으로 빌드되는 컨테이너 */
	TSharedPtr<SGridPanel> GridContainer;

	/* 우측 Points 좌표 읽기 전용 목록이 동적으로 빌드되는 컨테이너 */
	TSharedPtr<SVerticalBox> PointsListContainer;
};
