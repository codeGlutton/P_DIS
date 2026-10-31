/*****************************************************************//**
 * @file   BBMItemGameplayEffectTypes.h
 * @brief  BBM(비트 배낭 상인) 아이템의 GAS GameplayEffect 및 SetByCaller 설정 타입 정의
 * @date   2026-10-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "BBMItemGameplayEffectTypes.generated.h"

/**
 * 아이템이 비트 시작 시 캐릭터(ASC)에 부여할 GameplayEffect 및 SetByCaller 수치 파라미터 구성 구조체
 */
USTRUCT(BlueprintType)
struct DIS_API FBBMItemGameplayEffectConfig
{
	GENERATED_BODY()

	/* 생성자 */
public:
	FBBMItemGameplayEffectConfig();
	FBBMItemGameplayEffectConfig(TSubclassOf<UGameplayEffect> InEffectClass);

	/* 유효성 검사 */
public:
	/* 유효한 Effect 클래스가 등록되어 있는지 검사 */
	bool IsValid() const;

	/* 프로퍼티 */
public:
	/* 캐릭터에 적용할 GameplayEffect 클래스 (소프트 클래스 레퍼런스, Data 번들 로드 연동) */
	UPROPERTY(Category = "BBM|Effect", EditAnywhere, BlueprintReadWrite, meta = (AssetBundles = "Data"))
	TSoftClassPtr<UGameplayEffect> EffectClass;

	/* GameplayEffect 실행 시 SetByCallerMagnitude로 동적 주입할 태그 및 수치 맵 */
	UPROPERTY(Category = "BBM|Effect", EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, float> SetByCallerMagnitudes;
};

/**
 * 아이템의 기본 스탯 증감을 캐릭터 ASC에 부여하기 위한 범용 무한(Infinite) 지속 GameplayEffect
 * AttackPower, DefensePower, Armor 어트리뷰트를 SetByCallerMagnitude로 동적 조절함
 */
UCLASS(BlueprintType, meta = (DisplayName = "BBM Item Stat Gameplay Effect"))
class DIS_API UBBMItemStatGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMItemStatGameplayEffect();
};

