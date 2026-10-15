/*****************************************************************//**
 * @file   DISEnhancement.h
 * @brief  DIS 아이템 강화 스탯 기본 베이스 클래스 정의 (CDO 기반)
 * @date   2026-09-30
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Enhancement/DISActiveEnhancement.h"
#include "DISEnhancement.generated.h"

/**
 * 아이템에 부여되는 강화 스탯의 기본 추상 베이스 클래스 (CDO 기반 불변 정의)
 * 인스턴스를 매번 생성하지 않고 클래스 기본 객체(CDO)로서 동작하며,
 * 런타임 활성화 상태는 FDISActiveEnhancement 객체를 통해 상호작용함
 */
UCLASS(BlueprintType, Blueprintable, Abstract, Const, meta = (DisplayName = "DIS Enhancement Definition"))
class DISRUNTIME_API UDISEnhancement : public UObject
{
	GENERATED_BODY()

public:
	UDISEnhancement();

	/* 강화 라이프사이클 가상함수 (CDO 기반 실행) */
public:
	/* 강화 스탯 생성/등록 (영향을 주는 모디파이어가 생겨 컨테이너의 ActiveEnhancements에 추가될 때 1회 호출) */
	UFUNCTION(Category = "Enhancement", BlueprintNativeEvent)
	void OnEnhancementAdded(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const;
	virtual void OnEnhancementAdded_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const;

	/* 강화 스탯 제거/해제 (자신에게 영향을 주는 모디파이어가 더이상 없어 컨테이너의 ActiveEnhancements에서 내려갈 때 1회 호출) */
	UFUNCTION(Category = "Enhancement", BlueprintNativeEvent)
	void OnEnhancementRemoved(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const;
	virtual void OnEnhancementRemoved_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const;

	/* 강화 효과 활성화 (대상 객체에 강화 효과 적용 - 레벨이 0에서 양수가 될 때 1회 호출) */
	UFUNCTION(Category = "Enhancement", BlueprintNativeEvent)
	void OnEnhancementActivated(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const;
	virtual void OnEnhancementActivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const;

	/* 강화 효과 비활성화 (대상 객체에서 강화 효과 제거 및 정리 - 레벨이 양수에서 0이 될 때 1회 호출) */
	UFUNCTION(Category = "Enhancement", BlueprintNativeEvent)
	void OnEnhancementDeactivated(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const;
	virtual void OnEnhancementDeactivated_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement) const;

	/* 강화 레벨 값 변경 시 호출되는 훅 가상함수 (레벨 갱신 시마다 호출) */
	UFUNCTION(Category = "Enhancement", BlueprintNativeEvent)
	void OnEnhancementLevelChanged(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement, int32 OldLevel, int32 NewLevel) const;
	virtual void OnEnhancementLevelChanged_Implementation(UObject* Target, const FDISActiveEnhancement& ActiveEnhancement, int32 OldLevel, int32 NewLevel) const;

	/* 강화 수식어 정보 및 동적 텍스트 생성 */
public:
	/* 현재 강화 레벨을 반영한 접두사 텍스트 반환 (기본 구현: {0} 포맷팅 또는 고정 텍스트) */
	UFUNCTION(Category = "Enhancement|Affix", BlueprintNativeEvent)
	FText GetPrefixText(int32 Level) const;
	virtual FText GetPrefixText_Implementation(int32 Level) const;

	/* 현재 강화 레벨을 반영한 접미사 텍스트 반환 (기본 구현: {0} 포맷팅 또는 고정 텍스트) */
	UFUNCTION(Category = "Enhancement|Affix", BlueprintNativeEvent)
	FText GetSuffixText(int32 Level) const;
	virtual FText GetSuffixText_Implementation(int32 Level) const;

	/* 접두사 우선순위 반환 (높을수록 더 앞쪽에 배치됨) */
	UFUNCTION(Category = "Enhancement|Affix", BlueprintCallable, BlueprintPure)
	int32 GetPrefixPriority() const;

	/* 접미사 우선순위 반환 (높을수록 기본 이름에 더 가깝게 배치됨) */
	UFUNCTION(Category = "Enhancement|Affix", BlueprintCallable, BlueprintPure)
	int32 GetSuffixPriority() const;

	/* 강화 정보 조회 */
	UFUNCTION(Category = "Enhancement", BlueprintCallable, BlueprintPure)
	const FText& GetDescription() const;

	/* 기본 최대 레벨 반환 */
	UFUNCTION(Category = "Enhancement", BlueprintCallable, BlueprintPure)
	int32 GetDefaultMaxLevel() const;

	/* 강화 속성 프로퍼티 */
protected:
	/* 접두사 우선순위 (높을수록 더 앞쪽에 배치됨) */
	UPROPERTY(Category = "Enhancement|Affix", EditDefaultsOnly, BlueprintReadOnly)
	int32 PrefixPriority;

	/* 접미사 우선순위 (높을수록 기본 이름에 더 가깝게 배치됨) */
	UPROPERTY(Category = "Enhancement|Affix", EditDefaultsOnly, BlueprintReadOnly)
	int32 SuffixPriority;

	/* 강화 효과 상세 설명 텍스트 */
	UPROPERTY(Category = "Enhancement", EditDefaultsOnly, BlueprintReadOnly)
	FText Description;

	/* 스탯 고유 기본 최대 강화 레벨 (기본값 = 10, ClampMin = 1) */
	UPROPERTY(Category = "Enhancement", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", UIMin = "1"))
	int32 DefaultMaxLevel;
};
