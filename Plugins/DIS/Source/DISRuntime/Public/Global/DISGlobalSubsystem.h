/*****************************************************************//**
 * @file   DISGlobalSubsystem.h
 * @brief  DIS 플러그인 전역 런타임 관리 및 레지스트리 서브시스템
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Item/DISItemTypes.h"
#include "Enhancement/DISActiveEnhancementEffect.h"
#include "DISGlobalSubsystem.generated.h"

struct FStreamableHandle;
class UDISGlobalDefinition;

/**
 * DIS 플러그인의 전역 런타임 관리 및 레지스트리를 담당하는 게임 인스턴스 서브시스템
 * 초기 실행 시 UDISGlobalDefinition을 로드하여 기본 규칙/프로파일을 초기화하며,
 * GameFeatures 플러그인의 동적 프로파일/규칙 주입 및 해제를 지원함
 */
UCLASS(DisplayName = "DIS Global Subsystem")
class DISRUNTIME_API UDISGlobalSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UDISGlobalSubsystem();

	/* 서브시스템 라이프사이클 */
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* 전역 정의 에셋 동적 등록 및 해제 (GameFeatures 지원) */
public:
	UFUNCTION(Category = "DIS|Global", BlueprintCallable)
	bool RegisterGlobalDefinition(const UDISGlobalDefinition* GlobalDefinition);

	UFUNCTION(Category = "DIS|Global", BlueprintCallable)
	bool UnregisterGlobalDefinition(const UDISGlobalDefinition* GlobalDefinition);

	/* 아이템 프로파일 동적 등록 및 해제 (GameFeatures 지원) */
public:
	UFUNCTION(Category = "DIS|Item", BlueprintCallable)
	bool RegisterItemTypeProfile(const FGameplayTag& ItemTypeTag, const FDISItemTypeProfile& Profile);

	UFUNCTION(Category = "DIS|Item", BlueprintCallable)
	bool UnregisterItemTypeProfile(const FGameplayTag& ItemTypeTag);

	/* 아이템 프로파일 조회 */
public:
	UFUNCTION(Category = "DIS|Item", BlueprintCallable, BlueprintPure)
	const FDISItemTypeProfile& GetItemTypeProfile(const FGameplayTag& ItemTypeTag) const;

	/* 희귀도 프로파일 동적 등록 및 해제 (GameFeatures 지원) */
public:
	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable)
	bool RegisterRarityProfile(const FGameplayTag& RarityTag, const FDISRarityProfile& Profile);

	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable)
	bool UnregisterRarityProfile(const FGameplayTag& RarityTag);

	/* 희귀도 프로파일 조회 및 서열(Rank) 비교 */
public:
	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable, BlueprintPure)
	const FDISRarityProfile& GetRarityProfile(const FGameplayTag& RarityTag) const;

	/* 지정된 희귀도 태그의 서열 반환 (미등록 시 0) */
	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable, BlueprintPure)
	int32 GetRarityRank(const FGameplayTag& RarityTag) const;

	/* 두 희귀도의 서열 비교 (TagA > TagB: 1, TagA == TagB: 0, TagA < TagB: -1) */
	UFUNCTION(Category = "DIS|Item|Rarity", BlueprintCallable, BlueprintPure)
	int32 CompareRarity(const FGameplayTag& TagA, const FGameplayTag& TagB) const;


	/* 강화 효과(Effect) 활성화 핸들 발급 및 관리 */
public:
	/* 전역에서 고유하게 증가하는 정수 래퍼 핸들을 발급하고 TMap에 등록 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable)
	FDISActiveEnhancementEffectHandle AllocateEnhancementEffectHandle(class UDISEnhancementContainer* InOwnerContainer = nullptr);

	/* 강화 효과 식별 핸들 등록 해제 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable)
	bool ReleaseEnhancementEffectHandle(const FDISActiveEnhancementEffectHandle& Handle);

	/* 지정된 핸들이 현재 서브시스템에 유효하게 등록되어 있는지 확인 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable, BlueprintPure)
	bool IsEnhancementEffectHandleActive(const FDISActiveEnhancementEffectHandle& Handle) const;

	/* 지정된 핸들을 소유하고 있는 컨테이너 조회 */
	UFUNCTION(Category = "DIS|Enhancement", BlueprintCallable, BlueprintPure)
	class UDISEnhancementContainer* GetEnhancementContainerByEffectHandle(const FDISActiveEnhancementEffectHandle& Handle) const;

protected:
	/* 런타임에 동적으로 등록된 아이템 타입 프로파일 맵 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FDISItemTypeProfile> RegisteredItemTypeProfiles;

	/* 등록되지 않은 아이템 타입 요청 시 반환할 기본 폴백 프로파일 (UDISGlobalSettings 기반) */
	UPROPERTY(Transient)
	FDISItemTypeProfile FallbackItemProfile;

	/* 런타임에 동적으로 등록된 희귀도 프로파일 맵 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FDISRarityProfile> RegisteredRarityProfiles;

	/* 등록되지 않은 희귀도 요청 시 반환할 기본 폴백 프로파일 */
	UPROPERTY(Transient)
	FDISRarityProfile FallbackRarityProfile;

	/* 현재 활성화된 강화 효과 핸들 및 소유 컨테이너 매핑 (TMap 중복 관리) */
	UPROPERTY(Transient)
	TMap<FDISActiveEnhancementEffectHandle, TWeakObjectPtr<class UDISEnhancementContainer>> ActiveEnhancementEffectHandles;

	/* 전역 고유 증가용 강화 효과 핸들 정수 카운터 */
	int32 NextEnhancementEffectHandleId;

private:
	/* 코어 Data 번들로 프리로드되어 상주 중인 전역 정의 PrimaryDataAsset 인스턴스 */
	UPROPERTY(Transient)
	TObjectPtr<const UDISGlobalDefinition> LoadedCoreGlobalDefinition;
	
	/* 코어 Data 번들 사전 로드 및 상시 유지를 위한 스트리밍 핸들 */
	TSharedPtr<FStreamableHandle> CoreGlobalDefinitionHandle;
};
