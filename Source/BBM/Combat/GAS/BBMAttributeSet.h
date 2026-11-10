/*****************************************************************//**
 * @file   BBMAttributeSet.h
 * @brief  BBM(비트 배낭 상인) 전투 핵심 GAS 어트리뷰트 세트 클래스 정의
 * @date   2026-10-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Combat/BBMCombatTypes.h"
#include "BBMAttributeSet.generated.h"

#define BBM_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * BBM 캐릭터 및 전투원의 5대 핵심 전투 스탯을 관리하는 AttributeSet
 */
UCLASS(BlueprintType)
class BBM_API UBBMAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMAttributeSet();

	/* UAttributeSet 가상함수 오버라이드 */
public:
	/* 속성 변경 직전 수치 클램핑 가상함수 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/* GameplayEffect 실행 완료 후 수치 보정 가상함수 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/* 언리얼 프로퍼티 리플렉션을 순회하여 현재 모든 어트리뷰트 수치를 담은 스냅샷 생성 */
	FBBMCombatantSnapshot CreateAttributeSnapshot() const;

	/* 어트리뷰트 프로퍼티 */
public:
	/* 현재 체력 */
	UPROPERTY(Category = "Attributes|Health", EditDefaultsOnly, BlueprintReadOnly)
	FGameplayAttributeData Health;
	BBM_ATTRIBUTE_ACCESSORS(UBBMAttributeSet, Health)

	/* 최대 체력 */
	UPROPERTY(Category = "Attributes|Health", EditDefaultsOnly, BlueprintReadOnly)
	FGameplayAttributeData MaxHealth;
	BBM_ATTRIBUTE_ACCESSORS(UBBMAttributeSet, MaxHealth)

	/* 현재 지속 방어도 (전투 내내 누적 유지되며 턴이 바뀌어도 보존되는 방어 수치) */
	UPROPERTY(Category = "Attributes|Combat", EditDefaultsOnly, BlueprintReadOnly)
	FGameplayAttributeData Armor;
	BBM_ATTRIBUTE_ACCESSORS(UBBMAttributeSet, Armor)

	/* 턴 방어도 (해당 턴(8비트) 동안만 유효하며 턴 종료 시 0으로 초기화되는 임시 방어 수치) */
	UPROPERTY(Category = "Attributes|Combat", EditDefaultsOnly, BlueprintReadOnly)
	FGameplayAttributeData TurnArmor;
	BBM_ATTRIBUTE_ACCESSORS(UBBMAttributeSet, TurnArmor)

	/* 기본 공격력 (아이템의 공격 버프 GE로 증폭되며 비트 데미지 산출 시 계수가 적용됨) */
	UPROPERTY(Category = "Attributes|Combat", EditDefaultsOnly, BlueprintReadOnly)
	FGameplayAttributeData AttackPower;
	BBM_ATTRIBUTE_ACCESSORS(UBBMAttributeSet, AttackPower)

	/* 기본 방어력 (아이템의 방어 버프 GE로 증폭되며 비트 방어도 획득 시 계수가 적용됨) */
	UPROPERTY(Category = "Attributes|Combat", EditDefaultsOnly, BlueprintReadOnly)
	FGameplayAttributeData DefensePower;
	BBM_ATTRIBUTE_ACCESSORS(UBBMAttributeSet, DefensePower)
};

