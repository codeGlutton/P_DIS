/*****************************************************************//**
 * @file   BBMItemDefinition.h
 * @brief  BBM(비트 배낭 상인) 확장 아이템 정의 클래스 정의
 * @date   2026-10-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Item/DISItemDefinition.h"
#include "Item/BBMBeatTypes.h"
#include "Item/BBMItemGameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "BBMItemDefinition.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class FDataValidationContext;
class UAnimInstance;

/**
 * BBM(비트 배낭 상인) 전용 확장 아이템 정의 PrimaryDataAsset
 * 단일 아이템에 복수의 그룹 태그, 가격, 비트 구성, 링크드 애님 레이어 및 기본 GAS Effect를 지정할 수 있음
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "BBM Item Definition"))
class BBM_API UBBMItemDefinition : public UDISItemDefinition
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMItemDefinition();

#if WITH_EDITOR
	/* 데이터 유효성 검사 */
public:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/* 비트 정보 조회 헬퍼 */
public:
	/* 아이템 실행에 필요한 총 비트 수 반환 (점유 타일 수와 동일) */
	UFUNCTION(Category = "BBM|Item|Beat", BlueprintCallable, BlueprintPure)
	int32 GetBeatCount() const;

	/* 그룹 소속 여부 확인 헬퍼 */
public:
	/* 특정 그룹 태그에 소속되어 있는지 여부 검사 */
	UFUNCTION(Category = "BBM|Item|Group", BlueprintCallable, BlueprintPure)
	bool HasItemGroup(const FGameplayTag& InGroupTag) const;

	/* 아이템이 속한 모든 그룹 태그 반환 */
	UFUNCTION(Category = "BBM|Item|Group", BlueprintCallable, BlueprintPure)
	const FGameplayTagContainer& GetItemGroupTags() const;

	/* 프로퍼티 */
public:
	/* 상품의 가격 (상점 구매/판매 기본 가격 단위) */
	UPROPERTY(Category = "Item Definition|Economy", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0"))
	int32 Price;

	/* 비트 단위 실행 시퀀스 정의 목록 (점유 타일 수와 크기가 동일해야 함) */
	UPROPERTY(Category = "Item Definition|Beat", EditDefaultsOnly, BlueprintReadOnly)
	TArray<FBBMBeatDefinition> Beats;

	/* 아이템 실행 시 캐릭터에 동적 바인딩할 링크드 애님 레이어 클래스 목록 (World 번들, 비동기 프리로드 지원) */
	UPROPERTY(Category = "Item Definition|Animation", EditDefaultsOnly, BlueprintReadOnly, meta = (AssetBundles = "World"))
	TArray<TSoftClassPtr<UAnimInstance>> LinkedAnimLayers;

	/* 비트 시작 시 캐릭터(ASC)에 부여할 기본 GameplayEffect 및 SetByCaller 수치 목록 (Data 번들, 비동기 프리로드 지원) */
	UPROPERTY(Category = "Item Definition|Combat", EditDefaultsOnly, BlueprintReadOnly, meta = (AssetBundles = "Data"))
	TArray<FBBMItemGameplayEffectConfig> DefaultGrantedEffects;

	/* 아이템이 속한 그룹 태그 컨테이너 (복수 그룹 지원, 예: Group.Noble, Group.Undead) */
	UPROPERTY(Category = "Item Definition|Group", EditDefaultsOnly, BlueprintReadOnly, AssetRegistrySearchable)
	FGameplayTagContainer ItemGroupTags;
};
