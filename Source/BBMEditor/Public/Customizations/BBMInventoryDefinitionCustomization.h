/*****************************************************************//**
 * @file   BBMInventoryDefinitionCustomization.h
 * @brief  UBBMInventoryDefinition 에셋의 디테일 패널 2D 시각화 커스터마이제이션 정의
 * @details 전체 타일 규격, BlockedTileShape, 슬롯 강화를 읽기 전용으로 시각화하고 프로퍼티 변경 시 실시간 갱신 제공
 * @date   2026-11-03
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class SGridPanel;
class STextBlock;
class UBBMInventoryDefinition;
struct FPropertyChangedEvent;

/**
 * UBBMInventoryDefinition 전용 디테일 패널 커스터마이제이션
 * 복잡한 편집 버튼을 배제하고 인벤토리의 블록 타일, 전체 타일 크기, 슬롯 강화 위치를 한눈에 보여주는 순수 시각적 프리뷰를 제공함
 */
class FBBMInventoryDefinitionCustomization : public IDetailCustomization
{
	/* 생성 및 소멸 */
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual ~FBBMInventoryDefinitionCustomization() override;

	/* IDetailCustomization 인터페이스 */
public:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	/* 내부 헬퍼 및 이벤트 핸들러 */
private:
	/* 2D 타일 매트릭스 그리드 및 상태 텍스트 갱신 */
	void RefreshGridWidget();

	/* 외부 UObject의 프로퍼티 변경 감지 핸들러 (실시간 리프레시) */
	void OnObjectPropertyChanged(UObject* InObject, FPropertyChangedEvent& InPropertyChangedEvent);

	/* 현재 편집 대상 UBBMInventoryDefinition 인스턴스 획득 */
	UBBMInventoryDefinition* GetTargetDefinition() const;

	/* 멤버 변수 */
private:
	/* 현재 커스터마이징 중인 UBBMInventoryDefinition 객체 */
	TWeakObjectPtr<UBBMInventoryDefinition> TargetDefinition;

	/* 외부 오브젝트 프로퍼티 변경 감지 델리게이트 핸들 */
	FDelegateHandle OnPropertyChangedHandle;

	/* 2D 매트릭스 그리드 컨테이너 */
	TSharedPtr<SGridPanel> GridContainer;

	/* 상단 요약 정보 텍스트 블록 */
	TSharedPtr<STextBlock> SummaryTextBlock;
};
