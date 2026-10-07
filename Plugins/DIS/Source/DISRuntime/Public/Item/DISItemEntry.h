/*****************************************************************//**
 * @file   DISItemEntry.h
 * @brief  DIS 인벤토리 런타임 개별 아이템 인스턴스 엔트리 클래스 정의
 * @date   2026-09-29
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Enhancement/DISEnhanceableInterface.h"
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
	UFUNCTION(Category = "DIS|Item Entry", BlueprintCallable)
	void InitializeEntry(const UDISItemDefinition* InItemDefinition, float InDurability = -1.0f);

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
};
