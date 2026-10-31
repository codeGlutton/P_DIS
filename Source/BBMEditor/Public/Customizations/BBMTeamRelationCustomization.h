/*****************************************************************//**
 * @file   BBMTeamRelationCustomization.h
 * @brief  FBBMTeamRelation 구조체의 디테일 패널 프로퍼티 커스터마이제이션 정의
 * @details 누락된 팀 열거형 항목을 일괄 추가하는 버튼 UI 및 태도 매핑 디테일 뷰 제공
 * @date   2026-10-31
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class IPropertyHandle;
class IPropertyUtilities;
enum class EBBMTeamType : uint8;

/**
 * FBBMTeamRelation 구조체를 위한 디테일 패널 프로퍼티 커스터마이제이션
 */
class FBBMTeamRelationCustomization : public IPropertyTypeCustomization
{
	/* 생성 및 소멸 */
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	virtual ~FBBMTeamRelationCustomization() override = default;

	/* IPropertyTypeCustomization 인터페이스 */
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	/* 내부 헬퍼 및 이벤트 핸들러 */
private:
	/* 버튼 클릭 이벤트: 누락된 팀 관계 일괄 생성 */
	FReply OnFillMissingTeamsClicked();

	/* 헤더 요약 텍스트 생성 (예: "3개 태도 설정됨") */
	FText GetSummaryText() const;

	/* 상위 맵 키로부터 현재 주체 팀(ContextTeam) 파악 */
	EBBMTeamType GetContextTeam() const;

	/* 멤버 변수 */
private:
	/* 대상 프로퍼티 핸들 */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	/* 프로퍼티 유틸리티 (UI 강제 리프레시용) */
	TSharedPtr<IPropertyUtilities> PropertyUtilities;
};

