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
		bool bInIsActive = false
	);

	/* 유효성 및 CDO 조회 */
public:
	/* 유효한 강화 클래스가 설정되어 있는지 확인 */
	bool IsValid() const;

	/* 강화 CDO(Class Default Object) 인스턴스 반환 */
	const UDISEnhancement* GetEnhancementCDO() const;

	/* 강화 클래스 반환 */
	TSubclassOf<UDISEnhancement> GetEnhancementClass() const;

	/* 모디파이어 조작 및 수치 조회 */
public:
	/* 이펙트가 진입할 때 해당 Op 채널에 핸들과 실수 크기를 가진 모디파이어 엔트리 추가 */
	void AddModifier(EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle, float InMagnitude);

	/* 이펙트가 제거될 때 모든 Op 채널에서 해당 핸들의 모디파이어 엔트리 탐색 및 제거 (제거 성공 여부 반환) */
	bool RemoveModifier(const FDISActiveEnhancementEffectHandle& InHandle);

	/* 특정 Op 채널에서 해당 핸들의 모디파이어 엔트리 제거 (제거 성공 여부 반환) */
	bool RemoveModifier(EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle);

	/* 모든 Op 채널의 모디파이어 엔트리 초기화 */
	void ResetModifiers();

	/* 등록된 모든 모디파이어 채널들을 조합하여 최종 실수 계산값 반환 (요청 시 동적 산출) */
	float GetCalculatedValue() const;

	/* 등록된 모든 모디파이어 채널들을 조합하여 최종 강화 레벨(int32, 0 이상 반올림) 반환 (요청 시 동적 산출) */
	int32 GetLevel() const;

	/* 덮어쓰기(Override) 모디파이어가 등록되어 있는지 여부 반환 */
	bool HasOverride() const;

	/* 특정 Op 채널이 유효하고 모디파이어가 등록되어 있는지 확인 */
	bool HasModifierOp(EDISEnhancementLevelOp InOp) const;

	/* 자신에게 영향을 미치는 모디파이어가 하나라도 존재하는지 여부 반환 */
	bool HasAnyModifiers() const;

	/* 등록된 전체 모디파이어 엔트리 총 개수 반환 */
	int32 GetTotalModifierCount() const;

	/* 특정 Op 채널 포인터 반환 */
	const FDISActiveEnhancementModChannel* GetModChannel(EDISEnhancementLevelOp InOp) const;

	/* 전체 모디파이어 채널 배열 반환 (EDISEnhancementLevelOp 인덱스) */
	const TArray<FDISActiveEnhancementModChannel>& GetModifierOpValues() const;

	/* 비교 연산자 */
public:
	bool operator==(const FDISActiveEnhancement& Other) const;

	/* 내부 계산 헬퍼 */
private:
	/* 등록된 모디파이어 채널들을 재집계하여 캐시된 실수 계산값 갱신 */
	void UpdateCalculatedValue();

	/* 런타임 상태 프로퍼티 */
public:
	/* 대상 강화 정의 클래스 (고유 스탯 식별자) */
	UPROPERTY(Category = "Enhancement", EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDISEnhancement> EnhancementClass;

	/* 강화 활성화 여부 */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	bool bIsActive;

	/* EDISEnhancementLevelOp를 인덱스로 하여 각 Op별 모디파이어 채널들을 보관하는 2차원 구조 배열 (Add=0, Multiply=1, Override=2) */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	TArray<FDISActiveEnhancementModChannel> ModifierOpValues;

	/* 모디파이어 연산이 적용된 캐싱된 최종 실수 계산값 */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	float CachedCalculatedValue;
};
