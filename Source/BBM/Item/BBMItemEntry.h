/*****************************************************************//**
 * @file   BBMItemEntry.h
 * @brief  BBM(비트 배낭 상인) 런타임 개별 아이템 인스턴스 엔트리 클래스 정의
 * @details 런타임 가변 비트 시퀀스 및 통합 강화 스케줄(DynamicBeatSequence)을 소유함
 * @date   2026-10-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Item/DISItemEntry.h"
#include "Item/BBMBeatTypes.h"
#include "Item/BBMItemGameplayEffectTypes.h"
#include "BBMItemEntry.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMItemDefinition;
class UBBMItemEntry;

/* DynamicBeatSequence 재계산 완료 알림 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBBMOnDynamicBeatSequenceChanged, UBBMItemEntry*, ItemEntry);

/**
 * DynamicBeatSequence 배치 갱신을 지원하는 RAII 스코프 가드 구조체
 */
struct BBM_API FBBMScopedBeatSequenceBatch
{
	explicit FBBMScopedBeatSequenceBatch(UBBMItemEntry* InEntry);
	~FBBMScopedBeatSequenceBatch();

private:
	TWeakObjectPtr<UBBMItemEntry> TargetEntry;
};

/**
 * BBM 전용 런타임 가변 아이템 인스턴스 엔트리 클래스
 * 기본 정적 정의(UBBMItemDefinition)로부터 비트 시퀀스와 GAS Effect 구성을 복사하여 소유하며,
 * 아이템 강화 시 비트 Effect 추가/교체 및 비트 구간 효과(EffectRanges) 추가/조작을 전담함
 */
UCLASS(BlueprintType, meta = (DisplayName = "BBM Item Entry"))
class BBM_API UBBMItemEntry : public UDISItemEntry
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMItemEntry();

	/* 델리게이트 이벤트 */
public:
	/* DynamicBeatSequence 재계산이 완료되었을 때 UI 및 외부 리스너에게 알리는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "BBM|Item Entry|Beat")
	FBBMOnDynamicBeatSequenceChanged OnDynamicBeatSequenceChanged;

	/* UDISItemEntry BlueprintNativeEvent 가상함수 오버라이드 */
public:
	virtual void InitializeEntry_Implementation(const UDISItemDefinition* InItemDefinition, float InDurability = -1.0f) override;

	/* 런타임 비트 시퀀스 접근 및 강화 조작 */
public:
	/* 현재 런타임 비트 시퀀스 전체 반환 (읽기 전용) */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable, BlueprintPure)
	const FBBMBeatSequence& GetDynamicBeatSequence() const;

	/* 런타임 비트 시퀀스 가변 참조 반환 */
	FBBMBeatSequence& GetMutableDynamicBeatSequence();

	/* 현재 런타임 비트 목록 반환 (읽기 전용) */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable, BlueprintPure)
	const TArray<FBBMBeatDefinition>& GetDynamicBeats() const;

	/* 런타임 비트 목록 가변 참조 반환 */
	TArray<FBBMBeatDefinition>& GetMutableDynamicBeats();

	/* 특정 비트 인덱스에 Action 인스턴스 구조체 추가 (강화용) */
	bool AddActionToBeat(int32 BeatIndex, const TInstancedStruct<FBBMBeatAction>& InAction);

	/* 블루프린트 연동용 FInstancedStruct 추가 헬퍼 */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable, meta = (DisplayName = "Add Action To Beat"))
	bool K2_AddActionToBeat(int32 BeatIndex, const FInstancedStruct& InAction);

	/* 런타임 구간 효과(EffectRanges) 접근 및 조작 */
public:
	/* 현재 런타임 비트 구간 효과(EffectRanges) 목록 반환 (읽기 전용) */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable, BlueprintPure)
	const TArray<FBBMBeatEffectRange>& GetEffectRanges() const;

	/* 런타임 비트 구간 효과(EffectRanges) 목록 가변 참조 반환 */
	TArray<FBBMBeatEffectRange>& GetMutableEffectRanges();

	/* 신규 EffectRange 추가 (구간 지속 효과 부여용) */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable)
	void AddEffectRange(const FBBMBeatEffectRange& InRange);

	/* 특정 인덱스의 EffectRange 제거 */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable)
	bool RemoveEffectRange(int32 RangeIndex);

	/* 강화 파이프라인 재계산 */
public:
	/* 현재 활성화된 강화 스탯(UBBMBeatEnhancement)을 기반으로 DynamicBeatSequence를 완전히 재빌드 */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable)
	void RebuildDynamicBeatSequence();

	/* DynamicBeatSequence 재계산 보류 (배치 스코프 시작 - 중첩 카운터 증가) */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable)
	void SuspendBeatSequenceRebuild();

	/* DynamicBeatSequence 재계산 보류 해제 (배치 스코프 종료 - 카운터 0 도달 시 더티 상태면 1회 리빌드 실행) */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable)
	void ResumeBeatSequenceRebuild();

	/* 재빌드가 보류 중인지 여부 반환 */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable, BlueprintPure)
	bool IsBeatSequenceRebuildSuspended() const { return SuspendRebuildCounter > 0; }

	/* 특정 강화 클래스에 대해 현재 적용된 슬롯 강화 보너스 레벨 반환 */
	UFUNCTION(Category = "BBM|Item Entry|Enhancement", BlueprintCallable, BlueprintPure)
	int32 GetSlotEnhancementBonusLevel(TSubclassOf<UDISEnhancement> InEnhancementClass) const;

	/* 내부 구현 메서드 */
protected:
	/* 실제 비트 시퀀스 재계산 알고리즘 본문 실행 */
	void ExecuteRebuildDynamicBeatSequence();

	/* 프로퍼티 */
protected:
	/* 런타임 가변 비트 시퀀스 구성 (비트 슬롯, 애니메이션 및 EffectRanges 효과 통합 관리) */
	UPROPERTY(Category = "BBM|Item Entry|Beat", EditAnywhere, BlueprintReadWrite)
	FBBMBeatSequence DynamicBeatSequence;

	/* 재계산 진행 중 재귀 호출 방지 가드 플래그 */
	UPROPERTY(Transient)
	bool bIsRebuilding;

	/* 재계산이 보류되었거나 재귀 중 변경이 발생했음을 나타내는 더티 플래그 */
	UPROPERTY(Transient)
	bool bIsBeatSequenceDirty;

	/* 재계산 보류 카운터 (중첩 배치 지원) */
	UPROPERTY(Transient)
	int32 SuspendRebuildCounter;
};
