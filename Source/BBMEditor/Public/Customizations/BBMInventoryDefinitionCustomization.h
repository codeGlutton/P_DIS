/*****************************************************************//**
 * @file   BBMInventoryDefinitionCustomization.h
 * @brief  UBBMInventoryDefinition 에셋의 디테일 패널 2D 인터랙티브 커스터마이제이션 정의
 * @details BlockedTileShape를 반영한 2D 그리드 시각화 및 타일별 슬롯 강화를 직관적으로 편집하는 UI 제공
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Inventory/BBMInventoryTypes.h"

class IDetailLayoutBuilder;
class IPropertyHandle;
class SGridPanel;
class SVerticalBox;
class UBBMInventoryDefinition;
class UDISEnhancement;

/**
 * UBBMInventoryDefinition 전용 디테일 패널 커스터마이제이션
 * 2D 매트릭스 그리드로 막힌 타일과 사용 가능 타일을 시각화하고,
 * 특정 타일을 클릭하여 슬롯 강화를 추가/수정/삭제하는 인터랙티브 에디터 UI를 제공함
 */
class FBBMInventoryDefinitionCustomization : public IDetailCustomization
{
	/* 생성 및 소멸 */
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual ~FBBMInventoryDefinitionCustomization() override = default;

	/* IDetailCustomization 인터페이스 */
public:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	/* 그리드 및 상세 패널 리프레시 헬퍼 */
private:
	/* 2D 타일 그리드 UI 갱신 */
	void RefreshGridWidget();

	/* 선택된 타일의 슬롯 강화 상세 편집 UI 갱신 */
	void RefreshDetailPanelWidget();

	/* 타일 클릭 이벤트 */
	FReply OnTileClicked(FIntPoint Coord);

	/* 강화 항목 추가 이벤트 */
	FReply OnAddEnhancementClicked();

	/* 특정 인덱스의 강화 항목 제거 이벤트 */
	FReply OnRemoveEnhancementClicked(int32 EntryIndex);

	/* 선택된 타일의 모든 강화 일괄 삭제 이벤트 */
	FReply OnClearSlotEnhancementsClicked();

	/* 프로퍼티 및 에셋 상태 반영 헬퍼 */
private:
	/* 현재 편집 대상 UBBMInventoryDefinition 인스턴스 획득 */
	UBBMInventoryDefinition* GetTargetDefinition() const;

	/* 에셋 변경 사항 트랜잭션 기록 및 더티 마킹 */
	void MarkAssetDirty();

	/* 멤버 변수 */
private:
	/* 디테일 레이아웃 빌더 약참조 */
	IDetailLayoutBuilder* DetailLayout;

	/* 현재 커스터마이징 중인 UBBMInventoryDefinition 객체 */
	TWeakObjectPtr<UBBMInventoryDefinition> TargetDefinition;

	/* SlotEnhancements 프로퍼티 핸들 */
	TSharedPtr<IPropertyHandle> SlotEnhancementsPropertyHandle;

	/* 2D 매트릭스 그리드 컨테이너 */
	TSharedPtr<SGridPanel> GridContainer;

	/* 하단 슬롯 강화 상세 편집 컨테이너 */
	TSharedPtr<SVerticalBox> DetailContainer;

	/* 현재 사용자가 클릭하여 선택한 2D 그리드 좌표 (선택 없으면 NoneValue) */
	FIntPoint SelectedCoord = FIntPoint::NoneValue;

	/* 신규 추가용 임시 슬롯 강화 효과 클래스 선택 */
	TSubclassOf<UDISEnhancementEffect> PendingEffectClass;

	/* 신규 추가용 임시 슬롯 강화 보너스 배율/레벨 */
	float PendingOverallMagnitude = 1.0f;
};

