/*****************************************************************//**
 * @file   BBMItemEntry.h
 * @brief  BBM(비트 배낭 상인) 런타임 개별 아이템 인스턴스 엔트리 클래스 정의
 * @details 런타임 가변 비트 구성(DynamicBeats) 및 강화 가능한 GAS Effect(DynamicGrantedEffects)를 소유함
 * @date   2026-10-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Item/DISItemEntry.h"
#include "Item/BBMBeatTypes.h"
#include "Item/BBMItemGameplayEffectTypes.h"
#include "BBMItemEntry.generated.h"

/* 독립 전방 선언 (상단에 단독 배치) */
class UBBMItemDefinition;

/**
 * BBM 전용 런타임 가변 아이템 인스턴스 엔트리 클래스
 * 기본 정적 정의(UBBMItemDefinition)로부터 비트 시퀀스와 GAS Effect 구성을 복사하여 소유하며,
 * 아이템 강화 시 비트 Effect 추가/교체 및 GameplayEffect SetByCaller 수치 증폭을 전담함
 */
UCLASS(BlueprintType, meta = (DisplayName = "BBM Item Entry"))
class DIS_API UBBMItemEntry : public UDISItemEntry
{
	GENERATED_BODY()

	/* 생성자 */
public:
	UBBMItemEntry();

	/* UDISItemEntry BlueprintNativeEvent 가상함수 오버라이드 */
public:
	virtual void InitializeEntry_Implementation(const UDISItemDefinition* InItemDefinition, float InDurability = -1.0f) override;

	/* 런타임 비트 시퀀스 접근 및 강화 조작 */
public:
	/* 현재 런타임 비트 시퀀스 목록 반환 (읽기 전용) */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable, BlueprintPure)
	const TArray<FBBMBeatDefinition>& GetDynamicBeats() const;

	/* 런타임 비트 시퀀스 가변 참조 반환 */
	TArray<FBBMBeatDefinition>& GetMutableDynamicBeats();

	/* 특정 비트 인덱스에 Effect 인스턴스 구조체 추가 (강화용) */
	bool AddEffectToBeat(int32 BeatIndex, const TInstancedStruct<FBBMEffectBase>& InEffect);

	/* 블루프린트 연동용 FInstancedStruct 추가 헬퍼 */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable, meta = (DisplayName = "Add Effect To Beat"))
	bool K2_AddEffectToBeat(int32 BeatIndex, const FInstancedStruct& InEffect);

	/* 런타임 비트 1개 추가 (강화/슬롯 확장용) */
	UFUNCTION(Category = "BBM|Item Entry|Beat", BlueprintCallable)
	void AddDynamicBeat(const FBBMBeatDefinition& InBeat);

	/* 런타임 GAS GameplayEffect 접근 및 강화 조작 */
public:
	/* 현재 런타임에 부여될 GameplayEffect 구성 목록 반환 (읽기 전용) */
	UFUNCTION(Category = "BBM|Item Entry|Combat", BlueprintCallable, BlueprintPure)
	const TArray<FBBMItemGameplayEffectConfig>& GetDynamicGrantedEffects() const;

	/* 런타임 GameplayEffect 구성 가변 참조 반환 */
	TArray<FBBMItemGameplayEffectConfig>& GetMutableDynamicGrantedEffects();

	/* 런타임 GameplayEffect 구성 신규 추가 (강화 시 새로운 효과 부여용) */
	UFUNCTION(Category = "BBM|Item Entry|Combat", BlueprintCallable)
	void AddDynamicGrantedEffect(const FBBMItemGameplayEffectConfig& InConfig);

	/* 모든 동적 효과에서 특정 태그의 SetByCallerMagnitude 수치를 찾아 갱신/가산 (강화용) */
	UFUNCTION(Category = "BBM|Item Entry|Combat", BlueprintCallable)
	bool SetByCallerMagnitude(const FGameplayTag& InTag, float InValue);

	/* 특정 태그의 SetByCallerMagnitude 수치 조회 */
	UFUNCTION(Category = "BBM|Item Entry|Combat", BlueprintCallable, BlueprintPure)
	float GetSetByCallerMagnitude(const FGameplayTag& InTag, float DefaultValue = 0.0f) const;

	/* 프로퍼티 */
protected:
	/* 런타임 가변 비트 구성 목록 (강화 시 슬롯 추가 및 Effect 클래스 변경 가능) */
	UPROPERTY(Category = "BBM|Item Entry|Beat", EditAnywhere, BlueprintReadWrite)
	TArray<FBBMBeatDefinition> DynamicBeats;

	/* 런타임 가변 GAS GameplayEffect 구성 목록 (강화 시 새 GE 추가 및 SetByCaller 수치 증가 가능) */
	UPROPERTY(Category = "BBM|Item Entry|Combat", EditAnywhere, BlueprintReadWrite)
	TArray<FBBMItemGameplayEffectConfig> DynamicGrantedEffects;
};
