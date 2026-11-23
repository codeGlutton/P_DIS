/*****************************************************************//**
 * @file   DISEnhancementEffect.h
 * @brief  DIS 장착/해제 가능한 강화 효과 기본 베이스 클래스 정의 (CDO 기반)
 * @date   2026-09-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Templates/SubclassOf.h"
#include "Enhancement/DISEnhancementModifier.h"
#include "Enhancement/DISEnhancementEffectSpec.h"
#include "DISEnhancementEffect.generated.h"

class UDISEnhancement;

/**
 * 아이템에 장착/해제되어 여러 강화 스탯(UDISEnhancement)의 레벨에 복합적인 변동을 주는 효과 기본 클래스
 * (GAS의 UGameplayEffect와 유사하게 CDO 기반으로 정적 정의되며 모디파이어 맵을 제공함)
 */
UCLASS(BlueprintType, Blueprintable, Abstract, Const, meta = (DisplayName = "DIS Enhancement Effect Definition"))
class DISRUNTIME_API UDISEnhancementEffect : public UObject
{
	GENERATED_BODY()

public:
	UDISEnhancementEffect();

	/* 정보 조회 */
public:
	/* 대상 강화 스탯별 모디파이어 맵 반환 */
	UFUNCTION(Category = "Enhancement|Effect", BlueprintCallable, BlueprintPure)
	const TMap<TSubclassOf<UDISEnhancement>, FDISEnhancementModifier>& GetModifiers() const;

	/* 특정 강화 클래스에 대한 모디파이어 검색 */
	const FDISEnhancementModifier* FindModifier(TSubclassOf<UDISEnhancement> InEnhancementClass) const;

	/* 속성 프로퍼티 */
protected:
	/* 대상 강화 스탯(Key)별 레벨 연산 모디파이어(Value: LevelOp + LevelMagnitude) 설정 맵 */
	UPROPERTY(Category = "Enhancement|Effect", EditDefaultsOnly, BlueprintReadOnly)
	TMap<TSubclassOf<UDISEnhancement>, FDISEnhancementModifier> Modifiers;
};
