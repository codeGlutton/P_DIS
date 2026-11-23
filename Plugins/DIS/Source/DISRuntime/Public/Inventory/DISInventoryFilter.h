/*****************************************************************//**
 * @file   DISInventoryFilter.h
 * @brief  DIS 인벤토리 및 슬롯 필터링을 위한 베이스 구조체 및 태그 필터 구조체 정의
 * @details TInstancedStruct 기반으로 다형성 및 메모리 최적화를 지원하는 런타임 확장형 필터
 * @date   2026-10-14
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DISInventoryFilter.generated.h"

class UDISItemStack;

/**
 * 인벤토리 및 슬롯에 진입하는 아이템 스택을 검증하기 위한 필터 베이스 구조체 (TInstancedStruct 다형성 지원)
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISInventoryFilter
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FDISInventoryFilter();
	virtual ~FDISInventoryFilter() = default;

	/* 필터링 평가 메서드 */
public:
	/* 아이템 스택이 해당 필터 조건을 통과하는지 여부 검증 (가상 함수 오버라이드 지원) */
	virtual bool PassesFilter(const UDISItemStack* ItemStack) const;
};

/**
 * GameplayTag 기반으로 아이템 스택을 검증하는 기본 태그 필터 구조체
 */
USTRUCT(BlueprintType)
struct DISRUNTIME_API FDISTagInventoryFilter : public FDISInventoryFilter
{
	GENERATED_BODY()

	/* 생성자 및 가상 소멸자 */
public:
	FDISTagInventoryFilter();
	virtual ~FDISTagInventoryFilter() override = default;

	/* FDISInventoryFilter 가상 함수 구현 */
public:
	virtual bool PassesFilter(const UDISItemStack* ItemStack) const override;

	/* 필터 태그 속성 */
public:
	/* 아이템이 반드시 포함해야 하는 필수 태그 컨테이너 */
	UPROPERTY(Category = "Inventory Filter|Tags", EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer RequiredTags;

	/* 아이템에 포함되어서는 안 되는 거부 태그 컨테이너 */
	UPROPERTY(Category = "Inventory Filter|Tags", EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer DeniedTags;
};
