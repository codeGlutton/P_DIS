/*****************************************************************//**
 * @file   BBMGlobalLogic.h
 * @brief  BBM(비트 배낭 상인) 전역 로직 서브오브젝트 클래스
 * @date   2026-10-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Global/DISGlobalLogic.h"
#include "Item/BBMItemGroupTypes.h"
#include "Item/BBMItemProfileTypes.h"
#include "BBMGlobalLogic.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMGlobalDefinition;

/**
 * BBM(비트 배낭 상인) 전용 전역 로직 서브오브젝트
 * 기본 DIS 전역 로직 외에 아이템 그룹 프로파일 레지스트리를 추가 관리함
 */
UCLASS(Blueprintable, BlueprintType, DisplayName = "BBM Global Logic")
class DIS_API UBBMGlobalLogic : public UDISGlobalLogic
{
	GENERATED_BODY()

public:
	UBBMGlobalLogic();

	/* 라이프사이클 */
public:
	virtual void Deinitialize() override;

	/* 아이템 타입 커스텀 규칙 조회 */
public:
	/* 아이템 타입 태그에 해석된 BBM 전투 사용 규칙 반환 (미지정 시 기본값 Unlimited 반환) */
	UFUNCTION(Category = "BBM|Item", BlueprintCallable, BlueprintPure)
	EBBMItemUsageRule GetItemUsageRule(const FGameplayTag& ItemTypeTag) const;

	/* 아이템 그룹 프로파일 동적 등록 및 해제 */
public:
	/* 아이템 그룹 프로파일 등록 */
	UFUNCTION(Category = "BBM|Item|Group", BlueprintCallable)
	bool RegisterItemGroupProfile(const FGameplayTag& GroupTag, const FBBMItemGroupProfile& Profile);

	/* 아이템 그룹 프로파일 해제 */
	UFUNCTION(Category = "BBM|Item|Group", BlueprintCallable)
	bool UnregisterItemGroupProfile(const FGameplayTag& GroupTag);

	/* 아이템 그룹 프로파일 조회 */
	UFUNCTION(Category = "BBM|Item|Group", BlueprintCallable, BlueprintPure)
	const FBBMItemGroupProfile& GetItemGroupProfile(const FGameplayTag& GroupTag) const;

	/* 특정 그룹 태그의 프로파일 등록 여부 확인 */
	UFUNCTION(Category = "BBM|Item|Group", BlueprintCallable, BlueprintPure)
	bool HasItemGroupProfile(const FGameplayTag& GroupTag) const;

	/* UBBMGlobalDefinition 에셋 내 그룹 프로파일 일괄 등록 및 해제 */
public:
	/* UBBMGlobalDefinition 에셋 내 그룹 프로파일 일괄 등록 */
	UFUNCTION(Category = "BBM|Global", BlueprintCallable)
	bool RegisterBBMGlobalDefinition(const UBBMGlobalDefinition* BBMDefinition);

	/* UBBMGlobalDefinition 에셋 내 그룹 프로파일 일괄 해제 */
	UFUNCTION(Category = "BBM|Global", BlueprintCallable)
	bool UnregisterBBMGlobalDefinition(const UBBMGlobalDefinition* BBMDefinition);

	/* 프로파일 보관 데이터 */
protected:
	/* 런타임에 동적으로 등록된 아이템 그룹 프로파일 맵 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FBBMItemGroupProfile> RegisteredItemGroupProfiles;

	/* 미등록 시 반환할 기본 폴백 프로파일 */
	UPROPERTY(Transient)
	FBBMItemGroupProfile FallbackGroupProfile;
};
