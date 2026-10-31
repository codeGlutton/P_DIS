/*****************************************************************//**
 * @file   DISGlobalLogic.h
 * @brief  DIS 플러그인의 전역 비즈니스 로직 및 프로파일/상태 관리 서브오브젝트
 * @date   2026-10-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "Item/DISItemTypes.h"
#include "Enhancement/DISActiveEnhancementEffect.h"
#include "DISGlobalLogic.generated.h"

class UDISGlobalDefinition;
class UDISGlobalSubsystem;
class UDISEnhancementContainer;
struct FStreamableHandle;

/**
 * DIS 플러그인의 전역 비즈니스 로직, 아이템/희귀도 프로파일 레지스트리, 
 * 계층 상속 캐시 및 활성 강화 효과 핸들을 관리하는 서브오브젝트 클래스
 * UDISGlobalSubsystem에 의해 단일 인스턴스로 생성 및 수명 관리되며, 
 * 프로젝트 요구사항에 따라 C++ 또는 Blueprint로 상속받아 커스터마이징 가능함
 */
UCLASS(Blueprintable, BlueprintType, DisplayName = "DIS Global Logic")
class DISRUNTIME_API UDISGlobalLogic : public UObject
{
	GENERATED_BODY()

public:
	UDISGlobalLogic();

	/* 라이프사이클 (가상 함수 확장 훅) */
public:
	/**
	 * 로직 서브오브젝트 초기화 라이프사이클 훅
	 * @param InSubsystem 소유 UDISGlobalSubsystem 인스턴스
	 */
	virtual void Initialize(UDISGlobalSubsystem* InSubsystem);

	/**
	 * 로직 서브오브젝트 해제 및 리소스 정리 라이프사이클 훅
	 */
	virtual void Deinitialize();

	/* 전역 정의 에셋 동적 등록 및 해제 */
public:
	/* UDISGlobalDefinition 에셋 내 프로파일들을 일괄 등록 */
	UFUNCTION(Category = "DIS|Global", BlueprintCallable)
	bool RegisterGlobalDefinition(const UDISGlobalDefinition* GlobalDefinition);

	/* UDISGlobalDefinition 에셋 내 프로파일들을 일괄 해제 */
	UFUNCTION(Category = "DIS|Global", BlueprintCallable)
	bool UnregisterGlobalDefinition(const UDISGlobalDefinition* GlobalDefinition);

	/* 아이템 프로파일 동적 등록 및 계층 조회 */
public:
	/* 아이템 타입 프로파일 등록 */
	UFUNCTION(Category = "DIS|Item", BlueprintCallable)
	bool RegisterItemTypeProfile(const FGameplayTag& ItemTypeTag, const FDISItemTypeProfile& Profile);

	/* 아이템 타입 프로파일 등록 해제 */
	UFUNCTION(Category = "DIS|Item", BlueprintCallable)
	bool UnregisterItemTypeProfile(const FGameplayTag& ItemTypeTag);

	/* 아이템 타입 프로파일 계층 조회 (캐시 기반) */
	UFUNCTION(Category = "DIS|Item", BlueprintCallable, BlueprintPure)
	const FDISItemTypeProfile& GetItemTypeProfile(const FGameplayTag& ItemTypeTag) const;

	/* 아이템 프로파일 해석 캐시 무효화 */
	UFUNCTION(Category = "DIS|Item", BlueprintCallable)
	void InvalidateProfileCache();

	/* 아이템 타입 태그의 루트부터 리프까지 계층 순서 태그 배열 획득 */
	void GetItemTypeTagHierarchy(const FGameplayTag& InItemTypeTag, TArray<FGameplayTag>& OutHierarchyTags) const;

	/* 특정 아이템 타입이 대상 카테고리 태그에 부합하는지 여부 검사 */
	bool MatchesItemCategory(const FGameplayTag& InItemTypeTag, const FGameplayTag& InCategoryTag) const;

	/* 희귀도 프로파일 동적 등록 및 서열 비교 */
public:
	/* 희귀도 프로파일 등록 */
	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable)
	bool RegisterRarityProfile(const FGameplayTag& RarityTag, const FDISRarityProfile& Profile);

	/* 희귀도 프로파일 등록 해제 */
	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable)
	bool UnregisterRarityProfile(const FGameplayTag& RarityTag);

	/* 희귀도 프로파일 조회 */
	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable, BlueprintPure)
	const FDISRarityProfile& GetRarityProfile(const FGameplayTag& RarityTag) const;

	/* 지정된 희귀도 태그의 서열 반환 (미등록 시 0) */
	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable, BlueprintPure)
	int32 GetRarityRank(const FGameplayTag& RarityTag) const;

	/* 두 희귀도의 서열 비교 (TagA > TagB: 1, TagA == TagB: 0, TagA < TagB: -1) */
	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable, BlueprintPure)
	int32 CompareRarity(const FGameplayTag& TagA, const FGameplayTag& TagB) const;

	/* 강화 효과 핸들 발급 및 관리 */
public:
	/* 전역에서 고유하게 증가하는 정수 래퍼 핸들을 발급하고 TMap에 등록 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable)
	FDISActiveEnhancementEffectHandle AllocateEnhancementEffectHandle(UDISEnhancementContainer* InOwnerContainer = nullptr);

	/* 강화 효과 식별 핸들 등록 해제 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable)
	bool ReleaseEnhancementEffectHandle(const FDISActiveEnhancementEffectHandle& Handle);

	/* 지정된 핸들이 현재 유효하게 등록되어 있는지 확인 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable, BlueprintPure)
	bool IsEnhancementEffectHandleActive(const FDISActiveEnhancementEffectHandle& Handle) const;

	/* 지정된 핸들을 소유하고 있는 컨테이너 조회 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable, BlueprintPure)
	UDISEnhancementContainer* GetEnhancementContainerByEffectHandle(const FDISActiveEnhancementEffectHandle& Handle) const;

	/* 서브시스템 접근자 */
public:
	/* 소유 UDISGlobalSubsystem 참조 반환 */
	UFUNCTION(Category = "DIS|Global", BlueprintCallable, BlueprintPure)
	UDISGlobalSubsystem* GetOwningSubsystem() const;

	/* 프로파일 및 핸들 보관 데이터 */
protected:
	/* 런타임에 동적으로 등록된 아이템 타입 프로파일 맵 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FDISItemTypeProfile> RegisteredItemTypeProfiles;

	/* 등록되지 않은 아이템 타입 요청 시 반환할 기본 폴백 프로파일 */
	UPROPERTY(Transient)
	FDISItemTypeProfile FallbackItemProfile;

	/* 상속 및 병합이 완료된 아이템 타입 프로파일 런타임 캐시 */
	mutable TMap<FGameplayTag, FDISItemTypeProfile> ResolvedProfileCache;

	/* 런타임에 동적으로 등록된 희귀도 프로파일 맵 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FDISRarityProfile> RegisteredRarityProfiles;

	/* 등록되지 않은 희귀도 요청 시 반환할 기본 폴백 프로파일 */
	UPROPERTY(Transient)
	FDISRarityProfile FallbackRarityProfile;

	/* 현재 활성화된 강화 효과 핸들 및 소유 컨테이너 매핑 */
	UPROPERTY(Transient)
	TMap<FDISActiveEnhancementEffectHandle, TWeakObjectPtr<UDISEnhancementContainer>> ActiveEnhancementEffectHandles;

	/* 전역 고유 증가용 강화 효과 핸들 정수 카운터 */
	int32 NextEnhancementEffectHandleId;

	/* 내부 수명 및 코어 데이터 참조 */
private:
	/* 소유 UDISGlobalSubsystem 약참조 */
	UPROPERTY(Transient)
	TWeakObjectPtr<UDISGlobalSubsystem> OwningSubsystem;

	/* 코어 Data 번들로 프리로드되어 상주 중인 전역 정의 PrimaryDataAsset 인스턴스 */
	UPROPERTY(Transient)
	TObjectPtr<const UDISGlobalDefinition> LoadedCoreGlobalDefinition;

	/* 코어 Data 번들 사전 로드 및 상시 유지를 위한 스트리밍 핸들 */
	TSharedPtr<FStreamableHandle> CoreGlobalDefinitionHandle;
};
