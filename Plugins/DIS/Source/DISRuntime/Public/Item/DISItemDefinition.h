/*****************************************************************//**
 * @file   DISItemDefinition.h
 * @brief  DIS 정적 아이템 정의(PrimaryDataAsset) 클래스 정의
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Item/DISItemTypes.h"
#include "DISItemDefinition.generated.h"

class UTexture2D;

/**
 * DIS 플러그인 전역에서 사용하는 PrimaryAssetType 식별자 모음
 */
namespace DISPrimaryAssetTypes
{
	/* 아이템 정의 에셋 식별자 ("DISItem") */
	constexpr const TCHAR* Item = TEXT("DISItem");
}

/**
 * 아이템의 불변(Immutable) 정적 스펙 및 메타데이터를 정의하는 PrimaryDataAsset
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "DIS Item Definition"))
class DISRUNTIME_API UDISItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDISItemDefinition();

	/* PrimaryAssetId 오버라이드 */
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	/* 데이터 유효성 검사 */
public:
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	/* 아이템 정의 속성 (프로퍼티) */
public:
	/* 아이템 타입 태그 (에셋 레지스트리 검색 및 필터링 메타데이터) */
	UPROPERTY(Category = "Item Definition|Metadata", EditDefaultsOnly, BlueprintReadOnly, AssetRegistrySearchable)
	FGameplayTag ItemTypeTag;

	/* 아이템 표시 이름 (에셋 레지스트리 검색 및 필터링 메타데이터) */
	UPROPERTY(Category = "Item Definition|Metadata", EditDefaultsOnly, BlueprintReadOnly, AssetRegistrySearchable)
	FText ItemName;

	/* 아이템 희귀도 태그 (에셋 레지스트리 검색 및 필터링 메타데이터) */
	UPROPERTY(Category = "Item Definition|Metadata", EditDefaultsOnly, BlueprintReadOnly, AssetRegistrySearchable)
	FGameplayTag RarityTag;

	/* 아이템 2D 아이콘 (메모리 최적화를 위한 소프트 레퍼런스, UI 번들) */
	UPROPERTY(Category = "Item Definition|Visual", EditDefaultsOnly, BlueprintReadOnly, meta = (AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> Icon;

	/* 아이템 상세 설명 텍스트 */
	UPROPERTY(Category = "Item Definition|Visual", EditDefaultsOnly, BlueprintReadOnly)
	FText Description;

	/* 2D 인벤토리 그리드 상의 상대 타일 점유 형상 */
	UPROPERTY(Category = "Item Definition|Spatial", EditDefaultsOnly, BlueprintReadOnly)
	FDISItemShape Shape;

	/* 공간 그리드 내 초기 회전 각도 */
	UPROPERTY(Category = "Item Definition|Spatial", EditDefaultsOnly, BlueprintReadOnly)
	EDISItemRotation InitialRotation;

	/* 공간 그리드 내 회전 허용 여부 */
	UPROPERTY(Category = "Item Definition|Spatial", EditDefaultsOnly, BlueprintReadOnly)
	bool bCanRotate;

	/* 아이템 기본 무게 단위 */
	UPROPERTY(Category = "Item Definition|Stats", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight;

	/* 아이템 최대 내구도 (0.0일 경우 내구도 미적용/무제한) */
	UPROPERTY(Category = "Item Definition|Stats", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDurability;
};
