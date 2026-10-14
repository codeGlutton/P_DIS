/*****************************************************************//**
 * @file   DISEnhancementContainer.h
 * @brief  DIS 강화 시스템을 전담하여 관리 및 보관하는 컨테이너 객체 정의
 * @date   2026-09-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Enhancement/DISActiveEnhancement.h"
#include "Enhancement/DISActiveEnhancementEffect.h"
#include "Enhancement/DISEnhancementModifier.h"
#include "Enhancement/DISEnhancementEffectSpec.h"
#include "Enhancement/DISActiveEnhancementEffectIterator.h"
#include "DISEnhancementContainer.generated.h"

class UDISEnhancement;
class UDISEnhancementEffect;
class IDISEnhanceableInterface;
class UDISGlobalSubsystem;
class UDISEnhancementContainer;

/**
 * 컨테이너 변경 작업 시 재귀적 호출 안전성을 보장하는 RAII Scope Lock 구조체
 */
struct DISRUNTIME_API FScopedEnhancementContainerLock
{
	explicit FScopedEnhancementContainerLock(const UDISEnhancementContainer& InContainer);
	~FScopedEnhancementContainerLock();

private:
	const UDISEnhancementContainer& Container;
};

#define ENHANCEMENT_SCOPE_LOCK() FScopedEnhancementContainerLock ActiveScopeLock(*this);

/**
 * 아이템 등의 강화 스탯 및 장착/해제 가능한 활성 강화 효과 목록을 전담 관리하는 컨테이너 객체
 * (GAS의 FActiveGameplayEffectsContainer와 유사하게 활성 효과들을 보관하고 스탯 레벨을 산출하며 라이프사이클을 통제함)
 */
UCLASS(BlueprintType, meta = (DisplayName = "DIS Enhancement Container"))
class DISRUNTIME_API UDISEnhancementContainer : public UObject
{
	GENERATED_BODY()

	friend struct FScopedEnhancementContainerLock;
	template<typename ElementType, typename ContainerType> friend class TDISActiveEnhancementEffectIterator;

public:
	typedef TDISActiveEnhancementEffectIterator<const FDISActiveEnhancementEffect, UDISEnhancementContainer> FConstEffectIterator;
	typedef TDISActiveEnhancementEffectIterator<FDISActiveEnhancementEffect, UDISEnhancementContainer> FEffectIterator;

public:
	UDISEnhancementContainer();

	/* 초기화 및 소유자/대상 관리 */
public:
	/* 강화 컨테이너 소유자 및 효과 적용 대상 설정 및 초기화 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable)
	void Initialize(const TScriptInterface<IDISEnhanceableInterface>& InOwner, UObject* InEffectTarget = nullptr);

	/* 현재 소유자 인터페이스 반환 (명시적 소유자 또는 Outer 기반 탐색) */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable, BlueprintPure)
	TScriptInterface<IDISEnhanceableInterface> GetOwnerInterface() const;

	/* 효과 적용 대상 동적 변경 (장착/해제 라이프사이클 처리) */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable)
	void SetEffectTarget(UObject* InEffectTarget);

	/* 현재 효과 적용 대상 반환 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable, BlueprintPure)
	UObject* GetEffectTarget() const;

	/* 강화 효과(Effect) 장착 및 해제 */
public:
	/* 효과 클래스를 기반으로 강화 효과를 적용하고 고유 핸들 반환 */
	UFUNCTION(Category = "DIS|Enhancement|Effect", BlueprintCallable)
	FDISActiveEnhancementEffectHandle ApplyEnhancementEffectByClass(TSubclassOf<UDISEnhancementEffect> InEffectClass);

	/* 명세(Spec)를 기반으로 강화 효과를 적용하고 고유 핸들 반환 */
	UFUNCTION(Category = "DIS|Enhancement|Effect", BlueprintCallable)
	FDISActiveEnhancementEffectHandle ApplyEnhancementEffectSpec(const FDISEnhancementEffectSpec& InSpec);

	/* 고유 핸들에 해당하는 활성 강화 효과 제거 */
	UFUNCTION(Category = "DIS|Enhancement|Effect", BlueprintCallable)
	bool RemoveEnhancementEffect(const FDISActiveEnhancementEffectHandle& InHandle);

	/* 지정된 효과 클래스에 해당하는 모든 활성 강화 효과를 일괄 제거하고 제거된 수량 반환 */
	UFUNCTION(Category = "DIS|Enhancement|Effect", BlueprintCallable)
	int32 RemoveEnhancementEffectsByClass(TSubclassOf<UDISEnhancementEffect> InEffectClass);

	/* 핸들을 기반으로 활성 강화 효과 인스턴스 검색 (외부 수정 방지 const 포인터 반환, 없으면 nullptr) */
	const FDISActiveEnhancementEffect* FindActiveEnhancementEffect(const FDISActiveEnhancementEffectHandle& Handle) const;

	/* 핸들을 기반으로 활성 강화 효과 인스턴스 검색 (블루프린트 지원용) */
	UFUNCTION(Category = "DIS|Enhancement|Effect", BlueprintCallable, BlueprintPure, meta = (DisplayName = "Find Active Enhancement Effect"))
	bool K2_FindActiveEnhancementEffect(const FDISActiveEnhancementEffectHandle& Handle, FDISActiveEnhancementEffect& OutActiveEffect) const;

	/* 현재 적용된 모든 활성 강화 효과 인스턴스 목록 반환 */
	UFUNCTION(Category = "DIS|Enhancement|Effect", BlueprintCallable, BlueprintPure)
	const TArray<FDISActiveEnhancementEffect>& GetActiveEnhancementEffects() const;

	/* 강화 스탯(Enhancement) 관리 및 조회 */
public:
	/* 특정 강화 클래스의 활성 상태 인스턴스 검색 (외부 수정 방지 const 포인터 반환, 없으면 nullptr) */
	const FDISActiveEnhancement* FindActiveEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass) const;

	/* 특정 강화 클래스의 활성 상태 인스턴스 검색 (블루프린트 지원용) */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable, BlueprintPure, meta = (DisplayName = "Find Active Enhancement"))
	bool K2_FindActiveEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass, FDISActiveEnhancement& OutActiveEnhancement) const;

	/* 현재 관리 중인 모든 활성 강화 스탯 목록 반환 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable, BlueprintPure)
	const TArray<FDISActiveEnhancement>& GetActiveEnhancements() const;

protected:
	/* 특정 강화 클래스에 대한 최종 레벨 및 라이프사이클 재계산 */
	void RecalculateEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass);

	/* 정보 조회 헬퍼 */
public:
	/* 기본 이름에 활성화된 강화들의 우선순위 정렬된 접두사 및 접미사를 결합하여 최종 표시 이름 생성 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable, BlueprintPure)
	FText GetEnhancedDisplayName(const FText& BaseName) const;

	/* 락 관리 및 이터레이터 */
public:
	/* 컨테이너 조작 락 카운터 증가 */
	void IncrementLock() const;

	/* 컨테이너 조작 락 카운터 감소 및 0 도달 시 지연 작업 일괄 반영 */
	void DecrementLock();

	/* 현재 락이 걸려있는지 여부 반환 */
	bool IsLocked() const;

	/* 활성 효과 순회용 불변 이터레이터 생성 */
	FConstEffectIterator CreateConstEffectIterator() const;

	/* 활성 효과 순회용 가변 이터레이터 생성 */
	FEffectIterator CreateEffectIterator();

	/* 내부 유틸리티 및 라이프사이클 디스패처 */
protected:
	/* 유효한 전역 서브시스템 참조 획득 시도 */
	UDISGlobalSubsystem* GetGlobalSubsystem() const;

protected:
	/* 고유한 활성 강화 효과 핸들 발급 (서브시스템 또는 로컬 폴백) */
	FDISActiveEnhancementEffectHandle AllocateEffectHandle();

	/* 활성 강화 효과 핸들 해제 */
	void ReleaseEffectHandle(const FDISActiveEnhancementEffectHandle& Handle);

protected:
	/* 활성 강화 효과 인스턴스 검색 포인터 반환 (없으면 nullptr) */
	FDISActiveEnhancementEffect* FindActiveEnhancementEffect(const FDISActiveEnhancementEffectHandle& Handle);

	/* 활성 강화 스탯 인스턴스 검색 포인터 반환 (없으면 nullptr) */
	FDISActiveEnhancement* FindActiveEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass);

protected:
	/* 특정 강화 스탯에 모디파이어 추가 및 라이프사이클 전파 */
	void AddModifierToEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass, EDISEnhancementLevelOp InLevelOp, const FDISActiveEnhancementEffectHandle& InHandle, float InMagnitude);

	/* 특정 강화 스탯에서 모디파이어 제거 및 라이프사이클(제거 판단 포함) 전파 */
	void RemoveModifierFromEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass, const FDISActiveEnhancementEffectHandle& InHandle);

protected:
	/* 새로운 강화 스탯을 추가/활성화 훅 호출 후 컨테이너에 추가 */
	FDISActiveEnhancement* RegisterEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass);

	/* 더 이상 모디파이어가 없는 강화 스탯을 비활성화/제거 훅 호출 후 컨테이너에서 완전 해제 */
	void UnregisterEnhancement(TSubclassOf<UDISEnhancement> InEnhancementClass);

protected:
	/* 레벨 값 변경 및 활성/비활성 라이프사이클 훅 전파 헬퍼 */
	void OnEnhancementLevelChange(FDISActiveEnhancement& Enhancement, int32 OldLevel, int32 NewLevel);

	/* 대상 객체(EffectTarget) 변경 시 바인딩/언바인딩 훅 일괄 전파 헬퍼 */
	void OnEffectTargetChange(UObject* OldTarget, UObject* NewTarget);

	/* 런타임 멤버 상태 프로퍼티 */
protected:
	/* 강화를 소유한 대상 객체 인터페이스 참조 */
	UPROPERTY(Transient)
	TScriptInterface<IDISEnhanceableInterface> OwnerInterface;

	/* 강화 효과가 실제로 적용될 대상 객체 (약참조로 GC 댕글링 방지) */
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> EffectTarget;

	/* 현재 적용되어 있는 활성 강화 스탯 목록 (Unique 스탯 집계 대상) */
	UPROPERTY(Category = "Enhancement", VisibleAnywhere, BlueprintReadOnly)
	TArray<FDISActiveEnhancement> ActiveEnhancements;

	/* 현재 장착되어 레벨에 변동을 주고 있는 활성 강화 효과 목록 */
	UPROPERTY(Category = "Enhancement|Effect", VisibleAnywhere, BlueprintReadOnly)
	TArray<FDISActiveEnhancementEffect> ActiveEffects;

	/* 락 상태 동안 추가 요청된 지연 활성 강화 효과 목록 */
	UPROPERTY(Transient)
	TArray<FDISActiveEnhancementEffect> PendingAddEffects;

	/* 락 상태 동안 추가 요청된 지연 활성 강화 스탯 목록 */
	UPROPERTY(Transient)
	TArray<FDISActiveEnhancement> PendingAddEnhancements;

	/* 재귀 호출 보호용 스코프 락 카운터 */
	mutable int32 ScopedLockCount;

	/* 삭제 대기 중인 효과 개수 */
	int32 PendingEffectRemoves;

	/* 삭제 대기 중인 스탯 개수 */
	int32 PendingEnhancementRemoves;
};
