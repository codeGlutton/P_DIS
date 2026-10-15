/*****************************************************************//**
 * @file   DISActiveEnhancementEffect.h
 * @brief  DIS 활성화된 강화 효과(Effect) 인스턴스 상태 및 정수형 래퍼 핸들 정의
 * @date   2026-09-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Enhancement/DISEnhancementModifier.h"
#include "Enhancement/DISEnhancementEffectSpec.h"
#include "DISActiveEnhancementEffect.generated.h"

class UDISEnhancement;
class UDISEnhancementEffect;

/**
 * UDISGlobalSubsystem에서 발급받아 중복 없이 고유하게 관리되는 활성 강화 효과 식별 정수 래퍼 핸들
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISActiveEnhancementEffectHandle
{
	GENERATED_BODY()

public:
	FDISActiveEnhancementEffectHandle();
	explicit FDISActiveEnhancementEffectHandle(int32 InHandleId);

	/* 핸들 유효성 검사 */
public:
	bool IsValid() const;

	/* 정수 핸들 값 반환 */
	int32 GetHandleId() const;

	/* 비교 연산자 */
public:
	bool operator==(const FDISActiveEnhancementEffectHandle& Other) const;
	bool operator!=(const FDISActiveEnhancementEffectHandle& Other) const;

	/* 해시 함수 (TMap Key 지원) */
	friend uint32 GetTypeHash(const FDISActiveEnhancementEffectHandle& InHandle)
	{
		return ::GetTypeHash(InHandle.HandleId);
	}

	/* 디버그 문자열 */
	FString ToString() const;

	/* 런타임 멤버 상태 프로퍼티 */
protected:
	/* 발급된 고유 정수 식별자 (기본값 INDEX_NONE) */
	UPROPERTY(Category = "Enhancement|Effect", VisibleAnywhere, BlueprintReadOnly)
	int32 HandleId;
};

/**
 * 실제로 컨테이너에 적용되어 런타임 상태를 유지하는 활성화된 강화 효과 데이터 구조체
 * (EffectClass CDO의 모디파이어들에 Spec.OverallMagnitude를 곱한 CalculatedModifiers를 캐싱/보관)
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISActiveEnhancementEffect
{
	GENERATED_BODY()

public:
	FDISActiveEnhancementEffect();
	explicit FDISActiveEnhancementEffect(
		const FDISEnhancementEffectSpec& InSpec,
		const FDISActiveEnhancementEffectHandle& InHandle = FDISActiveEnhancementEffectHandle()
	);

	/* 유효성 및 정보 조회 */
public:
	/* 유효한 명세와 핸들이 설정되어 있는지 확인 */
	bool IsValid() const;

	/* 원본 효과 CDO 인스턴스 반환 */
	const UDISEnhancementEffect* GetEffectCDO() const;

	/* 원본 효과 클래스 반환 */
	TSubclassOf<UDISEnhancementEffect> GetEffectClass() const;

	/* 영향을 미치는 모든 대상 강화 스탯 클래스 목록 반환 */
	TArray<TSubclassOf<UDISEnhancement>> GetTargetEnhancementClasses() const;

	/* 특정 대상 강화 클래스에 대한 계산된 모디파이어 검색 */
	const FDISEnhancementModifier* FindCalculatedModifier(TSubclassOf<UDISEnhancement> InEnhancementClass) const;

	/* 계산된 전체 모디파이어 맵 반환 */
	const TMap<TSubclassOf<UDISEnhancement>, FDISEnhancementModifier>& GetCalculatedModifiers() const;

	/* 모디파이어 값 재계산 (CDO Magnitude * Spec.OverallMagnitude) */
	void RecalculateModifiers();

	/* 인스턴스 고유 핸들 반환 */
	const FDISActiveEnhancementEffectHandle& GetHandle() const;

	/* 요청 명세(Spec) 반환 */
	const FDISEnhancementEffectSpec& GetSpec() const;

	/* 삭제 대기 상태인지 확인 */
	bool IsPendingRemove() const { return bPendingRemove; }

	/* 삭제 대기 플래그 설정 */
	void MarkPendingRemove() { bPendingRemove = true; }

	/* 비교 연산자 */
public:
	bool operator==(const FDISActiveEnhancementEffect& Other) const;

	/* 런타임 상태 프로퍼티 */
public:
	/* 강화 효과 요청 명세 */
	UPROPERTY(Category = "Enhancement|Effect", EditAnywhere, BlueprintReadWrite)
	FDISEnhancementEffectSpec Spec;

	/* 인스턴스 고유 식별 핸들 (int32 래퍼 구조체) */
	UPROPERTY(Category = "Enhancement|Effect", VisibleAnywhere, BlueprintReadOnly)
	FDISActiveEnhancementEffectHandle Handle;

	/* 활성화 여부 */
	UPROPERTY(Category = "Enhancement|Effect", VisibleAnywhere, BlueprintReadOnly)
	bool bIsActive;

	/* 컨테이너 락 상태에서 삭제 대기 중인지 여부 (중복 삭제 방지용) */
	UPROPERTY(Category = "Enhancement|Effect", VisibleAnywhere, BlueprintReadOnly)
	bool bPendingRemove;

	/* CDO의 각 모디파이어에 Spec.OverallMagnitude가 곱해져 최종 산출된 런타임 모디파이어 캐시 맵 */
	UPROPERTY(Category = "Enhancement|Effect", VisibleAnywhere, BlueprintReadOnly)
	TMap<TSubclassOf<UDISEnhancement>, FDISEnhancementModifier> CalculatedModifiers;
};
