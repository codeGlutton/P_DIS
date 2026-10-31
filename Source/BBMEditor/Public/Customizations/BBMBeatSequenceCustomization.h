/*****************************************************************//**
 * @file   BBMBeatSequenceCustomization.h
 * @brief  FBBMBeatSequence 구조체의 디테일 패널 3단 타임라인 시각화 커스터마이제이션 정의
 * @details 상단에 EffectRanges 구간 바 / 8비트 로직 Effects 바 / 8비트 Animations 바를 렌더링하고 하단에 기본 설정 트리를 제공함
 * @date   2026-11-01
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class IPropertyHandle;
class IPropertyUtilities;
class SBox;
struct FBBMBeatSequence;

/**
 * FBBMBeatSequence 구조체를 위한 디테일 패널 프로퍼티 커스터마이제이션
 * 8비트 턴 시퀀스를 시각적으로 미리보고 검증할 수 있는 인터랙티브 타임라인 제공
 */
class FBBMBeatSequenceCustomization : public IPropertyTypeCustomization
{
	/* 생성 및 소멸 */
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	virtual ~FBBMBeatSequenceCustomization() override = default;

	/* IPropertyTypeCustomization 인터페이스 */
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	/* 타임라인 Slate 위젯 빌드 및 리프레시 헬퍼 */
private:
	/* 3단 타임라인 통합 컨테이너 위젯 생성 */
	TSharedRef<SWidget> BuildTimelineWidget();

	/* 상단 타임라인 시각화 위젯 부분 리프레시 (디테일 뷰 전체 재생성 방지) */
	void RefreshTimelineWidget();

	/* Track 1: EffectRanges 구간 표기 위젯 생성 */
	TSharedRef<SWidget> BuildEffectRangesTrack(const FBBMBeatSequence* Sequence);

	/* Track 2: 8비트 로직 Effects 가로 바 생성 */
	TSharedRef<SWidget> BuildEffectsTrack(const FBBMBeatSequence* Sequence);

	/* Track 3: 8비트 Animations 가로 바 생성 */
	TSharedRef<SWidget> BuildAnimationsTrack(const FBBMBeatSequence* Sequence);

	/* 하단 범례(Legend) 위젯 생성 */
	TSharedRef<SWidget> BuildLegendWidget();

	/* 대상 USTRUCT 포인터 획득 헬퍼 */
	const FBBMBeatSequence* GetBeatSequence() const;

	/* 헤더 요약 텍스트 생성 */
	FText GetSummaryText() const;

	/* 멤버 변수 */
private:
	/* 대상 프로퍼티 핸들 */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	/* 프로퍼티 유틸리티 (UI 강제 리프레시용) */
	TSharedPtr<IPropertyUtilities> PropertyUtilities;

	/* 상단 타임라인 시각화 위젯 컨테이너 박스 (부분 리프레시용) */
	TSharedPtr<SBox> TimelineBox;
};

