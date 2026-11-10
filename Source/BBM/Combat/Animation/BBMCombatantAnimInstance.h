/*****************************************************************//**
 * @file   BBMCombatantAnimInstance.h
 * @brief  BBM(비트 배낭 상인) 전투원 캐릭터 공용 애님 인스턴스 기본 클래스 정의
 * @details 캐릭터별 전용 Linked Anim Layer들을 GameplayTag 매핑 테이블(AnimLayerMap)로 관리함
 * @date   2026-11-06
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "BBMCombatantAnimInstance.generated.h"

/**
 * BBM 전투원 캐릭터 공용 애님 인스턴스 기본 클래스
 * 태그(FGameplayTag)를 키로 캐릭터 전용 Linked Anim Layer 클래스를 보관하며, 비트 시퀀스 연동 시 동적 조회를 지원함
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Combatant Anim Instance"))
class BBM_API UBBMCombatantAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMCombatantAnimInstance();

	/* UAnimInstance 가상 함수 */
public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/* 애니메이션 상태 태그 조회 및 조작 */
public:
	/* 현재 활성화된 애니메이션 상태 태그 반환 */
	UFUNCTION(Category = "BBM|Animation", BlueprintCallable, BlueprintPure)
	FGameplayTag GetCurrentAnimationStateTag() const;

	/* 현재 애니메이션 상태 태그 직접 설정 */
	UFUNCTION(Category = "BBM|Animation", BlueprintCallable)
	void SetCurrentAnimationStateTag(const FGameplayTag& InTag);

	/* 현재 애니메이션 상태 태그 비우기 */
	UFUNCTION(Category = "BBM|Animation", BlueprintCallable)
	void ClearAnimationStateTag();

	/* 애님 레이어 조회 및 검사 헬퍼 */
public:
	/* 특정 태그에 해당하는 Linked Anim Layer 클래스 조회 (미등록 시 nullptr) */
	UFUNCTION(Category = "BBM|Animation", BlueprintCallable, BlueprintPure)
	TSubclassOf<UAnimInstance> FindAnimLayerClass(const FGameplayTag& LayerTag) const;

	/* 특정 태그에 해당하는 Linked Anim Layer 등록 여부 검사 */
	UFUNCTION(Category = "BBM|Animation", BlueprintCallable, BlueprintPure)
	bool HasAnimLayer(const FGameplayTag& LayerTag) const;

	/* 등록된 전체 애님 레이어 매핑 테이블 반환 */
	UFUNCTION(Category = "BBM|Animation", BlueprintCallable, BlueprintPure)
	const TMap<FGameplayTag, TSubclassOf<UAnimInstance>>& GetAnimLayerMap() const;

	/* 프로퍼티 */
protected:
	/* 현재 활성화된 애니메이션 상태 태그 (스테이트 머신 트랜지션 규칙 평가에 사용) */
	UPROPERTY(Category = "Animation|State", VisibleInstanceOnly, BlueprintReadOnly)
	FGameplayTag CurrentAnimationStateTag;

	/* 태그별 Linked Anim Layer 클래스 매핑 테이블 (예: BBM.AnimLayer.Weapon.Sword -> ABP_SwordLayer) */
	UPROPERTY(Category = "Animation|Layers", EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, TSubclassOf<UAnimInstance>> AnimLayerMap;
};

