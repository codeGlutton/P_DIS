/*****************************************************************//**
 * @file   BBMSlotEnhancement.h
 * @brief  BBM(비트 배낭 상인) 슬롯 전용 UDISEnhancement 강화 스탯 구체 클래스 정의
 * @details 슬롯 자체 EnhancementContainer에 활성화되어 소유 아이템에게 비트 강화 효과를 주입함
 * @date   2026-11-02
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Enhancement/DISEnhancement.h"
#include "Enhancement/DISActiveEnhancementEffect.h"
#include "BBMSlotEnhancement.generated.h"

class UDISItemEntry;
class UDISEnhancementEffect;

/**
 * 슬롯 자체에 부여되는 강화 스탯 기본 베이스 클래스
 * 슬롯에 활성화되어 소유 아이템에게 부여할 BBMBeatEnhancementEffect 클래스를 정의함
 */
UCLASS(BlueprintType, Blueprintable, Abstract, meta = (DisplayName = "BBM Slot Enhancement"))
class BBM_API UBBMSlotEnhancement : public UDISEnhancement
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMSlotEnhancement();

	/* UDISEnhancement 라이프사이클 가상함수 오버라이드 */
public:
	virtual void OnEnhancementActivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const override;
	virtual void OnEnhancementDeactivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const override;
	virtual void OnEnhancementLevelChanged_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement, int32 OldLevel, int32 NewLevel) const override;

	/* 아이템 강화 효과 주입 인터페이스 */
public:
	/* 슬롯에 배치된 소유 아이템에게 부여할 비트 강화 효과 클래스 반환 */
	UFUNCTION(Category = "BBM|Slot Enhancement", BlueprintCallable, BlueprintPure)
	virtual TSubclassOf<UDISEnhancementEffect> GetGrantedBeatEffectClass() const;

	/* 프로퍼티 */
protected:
	/* 소유 아이템에게 부여할 BBM 비트 강화 효과 클래스 */
	UPROPERTY(Category = "BBM|Slot Enhancement", EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UDISEnhancementEffect> GrantedBeatEffectClass;
};

/**
 * 슬롯에 구간 효과 비트 강화(UBBMEffectRangeBeatEnhancement)를 부여하는 슬롯 강화 스탯 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Slot Effect Range Enhancement"))
class BBM_API UBBMSlotEffectRangeEnhancement : public UBBMSlotEnhancement
{
	GENERATED_BODY()

public:
	UBBMSlotEffectRangeEnhancement();
};

/**
 * 슬롯에 조건부 액션 주입 비트 강화(UBBMConditionalActionBeatEnhancement)를 부여하는 슬롯 강화 스탯 클래스
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "BBM Slot Conditional Action Enhancement"))
class BBM_API UBBMSlotConditionalActionEnhancement : public UBBMSlotEnhancement
{
	GENERATED_BODY()

public:
	UBBMSlotConditionalActionEnhancement();
};

