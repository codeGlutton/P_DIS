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

protected:
	/* 런타임에 동적으로 등록된 아이템 타입 프로파일 맵 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FDISItemTypeProfile> RegisteredItemTypeProfiles;

	/* 등록되지 않은 아이템 타입 요청 시 반환할 기본 폴백 프로파일 (UDISGlobalSettings 기반) */
	UPROPERTY(Transient)
	FDISItemTypeProfile FallbackItemProfile;

private:
	/* 코어 Data 번들로 프리로드되어 상주 중인 전역 정의 PrimaryDataAsset 인스턴스 */
	UPROPERTY(Transient)
	TObjectPtr<const UDISGlobalDefinition> LoadedCoreGlobalDefinition;
	
	/* 코어 Data 번들 사전 로드 및 상시 유지를 위한 스트리밍 핸들 */
	TSharedPtr<FStreamableHandle> CoreGlobalDefinitionHandle;
};
