/*****************************************************************//**
 * @file   DISActiveEnhancement.h
 * @brief  DIS 강화 스탯의 런타임 활성화 상태 데이터 정의 (GAS 모디파이어 집계 대상)
 * @date   2026-09-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Enhancement/DISEnhancementModifier.h"
#include "Enhancement/DISActiveEnhancementEffect.h"
#include "DISActiveEnhancement.generated.h"

class UDISEnhancement;

/**
 * 활성 강화 스탯에 적용된 개별 강화 효과(Effect)의 모디파이어 인스턴스 데이터
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISActiveEnhancementModEntry
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FDISActiveEnhancementModEntry();
	FDISActiveEnhancementModEntry(
		const FDISActiveEnhancementEffectHandle& InHandle,
		float InMagnitude
	);

	/* 비교 연산자 */
public:
	bool operator==(const FDISActiveEnhancementModEntry& Other) const;
	bool operator==(const FDISActiveEnhancementEffectHandle& InHandle) const;

	/* 런타임 프로퍼티 */
public:
	/* 효과 고유 식별 핸들 */
	UPROPERTY(Category = "Enhancement|Modifier", VisibleAnywhere, BlueprintReadOnly)
	FDISActiveEnhancementEffectHandle Handle;

	/* 적용된 모디파이어 실수 수치 */
	UPROPERTY(Category = "Enhancement|Modifier", VisibleAnywhere, BlueprintReadOnly)
	float Magnitude;
};

/**
 * 동일한 연산 방식(Add, Multiply, Override)을 공유하는 모디파이어 엔트리 목록 채널 (2차원 TArray 래퍼)
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISActiveEnhancementModChannel
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FDISActiveEnhancementModChannel();

	/* 엔트리 관리 */
public:
	/* 모디파이어 엔트리 추가 또는 기존 핸들 갱신 */
	void AddEntry(const FDISActiveEnhancementEffectHandle& InHandle, float InMagnitude);

	/* 핸들에 해당하는 엔트리 제거 */
	bool RemoveEntry(const FDISActiveEnhancementEffectHandle& InHandle);

	/* 핸들에 해당하는 엔트리 검색 */
	const FDISActiveEnhancementModEntry* FindEntry(const FDISActiveEnhancementEffectHandle& InHandle) const;

	/* 모든 엔트리 비우기 */
	void Empty();

	/* 등록된 엔트리 개수 반환 */
	int32 Num() const;

	/* 비어있는지 확인 */
	bool IsEmpty() const;

	/* 엔트리 목록 반환 */
	const TArray<FDISActiveEnhancementModEntry>& GetEntries() const;

	/* 런타임 프로퍼티 */
public:
	/* 채널에 등록된 개별 모디파이어 엔트리 목록 */
	UPROPERTY(Category = "Enhancement|Modifier", VisibleAnywhere, BlueprintReadOnly)
	TArray<FDISActiveEnhancementModEntry> Entries;
};

/**
 * 아이템 등에 활성화되어 있는 단일 강화 스탯의 런타임 상태 구조체
 * (GAS의 Attribute처럼 동작하며, Op별 모디파이어 채널을 유지하고 요청 시 최종 계산값을 반환함)
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISActiveEnhancement
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FDISActiveEnhancement();
	explicit FDISActiveEnhancement(
		TSubclassOf<UDISEnhancement> InEnhancementClass,
		bool bInIsActive = false,
		int32 InBaseMaxLevel = 0
	);

	/* 유효성 및 CDO 조회 */
public:
	/* 유효한 강화 클래스가 설정되어 있는지 확인 */
	bool IsValid() const;

	/* 강화 CDO(Class Default Object) 인스턴스 반환 */
	const UDISEnhancement* GetEnhancementCDO() const;

	/* 강화 클래스 반환 */
	TSubclassOf<UDISEnhancement> GetEnhancementClass() const;

	/* 기본 최대 레벨 설정 및 조회 */
public:
	/* 강화 대상 객체(OwnerInterface)로부터 주입받은 기본 최대 레벨 설정 */
	void SetBaseMaxLevel(int32 InBaseMaxLevel);

	/* 기본 최대 레벨 반환 */
	int32 GetBaseMaxLevel() const;

	/* 모디파이어 조작 및 수치 조회 */
public:
	/* 타깃(현재 레벨 vs 최대 레벨)과 Op 채널에 모디파이어 엔트리 추가 */
	void AddModifier(EDISEnhancementModifierTarget InTarget, EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle, float InMagnitude);

	/* 모든 채널(현재 레벨 및 최대 레벨)에서 해당 핸들의 모디파이어 엔트리 탐색 및 제거 (제거 성공 여부 반환) */
	bool RemoveModifier(const FDISActiveEnhancementEffectHandle& InHandle);

	/* 특정 타깃 및 Op 채널에서 해당 핸들의 모디파이어 엔트리 제거 (제거 성공 여부 반환) */
	bool RemoveModifier(EDISEnhancementModifierTarget InTarget, EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle);

	/* 모든 타깃의 모디파이어 엔트리 초기화 */
	void ResetModifiers();

	/* 등록된 모든 모디파이어 및 MaxLevel 상한 클램프가 적용된 최종 실수 계산값 반환 */
	float GetCalculatedValue() const;

	/* MaxLevel 상한 클램프 이전의 순수 실수 계산값 반환 */
	float GetRawCalculatedValue() const;

	/* 모디파이어가 반영된 최종 산출 최대 레벨 실수값 반환 */
	float GetCalculatedMaxValue() const;

	/* 최종 강화 레벨(int32, 0 이상 MaxLevel 이하) 반환 */
	int32 GetLevel() const;

	/* 최종 최대 강화 레벨(int32, 0 이상) 반환 */
	int32 GetMaxLevel() const;

	/* 특정 타깃(현재 레벨 vs 최대 레벨)에 덮어쓰기(Override) 모디파이어가 등록되어 있는지 여부 반환 */
	bool HasOverride(EDISEnhancementModifierTarget InTarget = EDISEnhancementModifierTarget::CurrentLevel) const;

	/* 특정 타깃 및 Op 채널에 모디파이어가 등록되어 있는지 확인 */
	bool HasModifierOp(EDISEnhancementModifierTarget InTarget, EDISEnhancementLevelOp InOp) const;

	/* 자신에게 영향을 미치는 모디파이어가 하나라도 존재하는지 여부 반환 */
	bool HasAnyModifiers() const;

	/* 등록된 전체 모디파이어 엔트리 총 개수 반환 */
	int32 GetTotalModifierCount() const;

	/* 현재 레벨의 특정 Op 채널 포인터 반환 */
	const FDISActiveEnhancementModChannel* GetLevelModChannel(EDISEnhancementLevelOp InOp) const;

	/* 최대 레벨의 특정 Op 채널 포인터 반환 */
	const FDISActiveEnhancementModChannel* GetMaxLevelModChannel(EDISEnhancementLevelOp InOp) const;

	/* 현재 레벨 모디파이어 채널 배열 반환 */
	const TArray<FDISActiveEnhancementModChannel>& GetLevelModifierOpValues() const;

	/* 최대 레벨 모디파이어 채널 배열 반환 */
	const TArray<FDISActiveEnhancementModChannel>& GetMaxLevelModifierOpValues() const;


	/* 비교 연산자 */
public:
	bool operator==(const FDISActiveEnhancement& Other) const;

	/* 삭제 대기 상태 관리 */
public:
	/* 삭제 대기 상태인지 확인 */
	bool IsPendingRemove() const { return bPendingRemove; }

	/* 삭제 대기 플래그 설정 */
	void MarkPendingRemove() { bPendingRemove = true; }

	/* 내부 계산 헬퍼 */
private:
	/* 등록된 모디파이어 채널들을 재집계하여 캐시된 실수 계산값 및 최대 레벨 갱신 */
	void UpdateCalculatedValue();

	/* 런타임 상태 프로퍼티 */
public:
	/* 대상 강화 정의 클래스 (고유 스탯 식별자) */
	UPROPERTY(Category = "Enhancement", EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDISEnhancement> EnhancementClass;

	/* 강화 활성화 여부 */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	bool bIsActive;

	/* 컨테이너 락 상태에서 스탯 등록 해제 대기 중인지 여부 */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	bool bPendingRemove;

	/* OwnerInterface 또는 CDO에서 제공하는 기본 최대 레벨 */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	int32 BaseMaxLevel;

	/* 현재 레벨 연산용 모디파이어 채널 배열 (Add=0, Multiply=1, Override=2) */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	TArray<FDISActiveEnhancementModChannel> LevelModifierOpValues;

	/* 최대 레벨 연산용 모디파이어 채널 배열 (Add=0, Multiply=1, Override=2) */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	TArray<FDISActiveEnhancementModChannel> MaxLevelModifierOpValues;

	/* 클램프 이전의 현재 레벨 실수 계산값 */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	float CachedCalculatedRawLevel;

	/* 모디파이어 연산이 적용된 최대 레벨 실수 계산값 */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	float CachedCalculatedMaxLevel;

	/* MaxLevel로 클램핑된 최종 실수 계산값 */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	float CachedCalculatedValue;
};
