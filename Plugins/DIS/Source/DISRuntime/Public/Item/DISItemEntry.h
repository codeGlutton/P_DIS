/*****************************************************************//**
 * @file   DISItemEntry.h
 * @brief  DIS 인벤토리 런타임 개별 아이템 인스턴스 엔트리 클래스 정의
 * @date   2026-09-29
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Enhancement/DISEnhanceableInterface.h"
#include "Item/DISItemPlacementRule.h"
#include "Item/DISItemTypes.h"
#include "DISItemEntry.generated.h"

class UDISItemDefinition;
class UDISEnhancementContainer;

/**
 * 인벤토리 내 개별 아이템 인스턴스의 런타임 가변 상태(내구도, 소유 중인 강화 컨테이너 등)를 관리하는 객체
 */
UCLASS(BlueprintType, meta = (DisplayName = "DIS Item Entry"))
class DISRUNTIME_API UDISItemEntry : public UObject, public IDISEnhanceableInterface
{
	GENERATED_BODY()

public:
	UDISItemEntry();

	/* 초기화 */
public:
	/* 아이템 정의 및 초기 내구도를 기반으로 인스턴스 초기화 */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable, BlueprintNativeEvent)
	void InitializeEntry(const UDISItemDefinition* InItemDefinition, float InDurability = -1.0f);
	virtual void InitializeEntry_Implementation(const UDISItemDefinition* InItemDefinition, float InDurability = -1.0f);

	/* IDISEnhanceableInterface 인터페이스 구현 */
public:
	/* 강화를 전담 관리하는 컨테이너 객체 포인터 반환 */
	virtual UDISEnhancementContainer* GetEnhancementContainer() const override;

	/* 아이템 고유 속성 접근 메서드 */
public:
	/* 원본 정적 아이템 정의(DataAsset) 반환 */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable, BlueprintPure)
	const UDISItemDefinition* GetItemDefinition() const;

	/* 현재 내구도 반환 */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable, BlueprintPure)
	float GetCurrentDurability() const;

	/* 현재 내구도 수치 갱신 (0.0 ~ MaxDurability 범위로 클램핑) */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable)
	void SetCurrentDurability(float NewDurability);

	/* 정보 조회 헬퍼 */
public:
	/* 강화 컨테이너를 통해 우선순위 기반 접두사/접미사가 결합된 최종 표시 이름 반환 */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable, BlueprintPure)
	FText GetEnhancedDisplayName() const;

	/* 배치 제한 규칙 관리 */
public:
	/* 아이템에 등록 가능한 TInstancedStruct 배치 제한 규칙 추가 */
	void AddPlacementRule(const TInstancedStruct<FDISItemPlacementRule>& InRule);

	/* 템플릿 기반 구체 배치 제한 규칙 생성 및 추가 */
	template <typename TRuleType, typename... TArgs>
	TRuleType& EmplacePlacementRule(TArgs&&... Args)
	{
		TInstancedStruct<FDISItemPlacementRule>& AddedRule = PlacementRules.Add_GetRef(TInstancedStruct<FDISItemPlacementRule>::Make<TRuleType>(Forward<TArgs>(Args)...));
		return AddedRule.GetMutable<TRuleType>();
	}

	/* 아이템의 모든 배치 제한 규칙 제거 */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable)
	void ClearPlacementRules();

	/* 아이템에 등록된 모든 배치 제한 규칙 목록 반환 */
	const TArray<TInstancedStruct<FDISItemPlacementRule>>& GetPlacementRules() const;

	/* 아이템에 등록된 모든 배치 제한 규칙 목록 가변 참조 반환 */
	TArray<TInstancedStruct<FDISItemPlacementRule>>& GetMutablePlacementRules();

	/* 배치 가능 여부 평가 */
public:
	/* 주어진 슬롯 컨텍스트에 아이템이 배치될 수 있는지 모든 규칙을 검증 (모두 통과해야 true 반환) */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable, BlueprintPure)
	virtual bool CanPlaceAtSlot(const FDISItemPlacementContext& Context) const;

	/* 스태킹 동등성 검증 */
public:
	/* 다른 아이템 엔트리와 스택으로 합쳐질 수 있는지 여부 검증 (정의 일치 및 강화 종류/레벨 완전 일치, 내구도는 무관) */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable, BlueprintPure)
	bool CanStackWith(const UDISItemEntry* OtherEntry) const;

	/* 회전 규칙 질의 */
public:
	/* 아이템 정의에 기반하여 회전이 허용되는지 여부 반환 */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable, BlueprintPure)
	bool CanRotate() const;

	/* 아이템 정의에 지정된 초기 회전 각도 반환 */
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable, BlueprintPure)
	EDISItemRotation GetInitialRotation() const;

	/* 런타임 멤버 상태 프로퍼티 */
protected:
	/* 정적 아이템 메타데이터 정의 (PrimaryDataAsset) */
	UPROPERTY(Category = "Item Entry", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<const UDISItemDefinition> ItemDefinition;

	/* 현재 남은 내구도 수치 */
	UPROPERTY(Category = "Item Entry", VisibleAnywhere, BlueprintReadOnly)
	float CurrentDurability;

	/* 강화를 전담 관리하는 컨테이너 인스턴스 */
	UPROPERTY(Category = "Item Entry", VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDISEnhancementContainer> EnhancementContainer;

	/* 아이템 인스턴스 전용 배치 제한 규칙 목록 (TInstancedStruct 다형성 지원) */
	UPROPERTY(Category = "Item Entry", VisibleAnywhere, BlueprintReadOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FDISItemPlacementRule>> PlacementRules;
};
