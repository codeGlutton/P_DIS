/*****************************************************************//**
 * @file   DISGlobalDefinition.h
 * @brief  DIS 플러그인 전역 런타임 설정(PrimaryDataAsset) 클래스 정의
 * @date   2026-09-28
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Item/DISItemTypes.h"
#include "DISGlobalDefinition.generated.h"

/**
 * DIS 플러그인 전역에서 사용하는 PrimaryAssetType 식별자 모음
 */
namespace DISPrimaryAssetTypes
{
	/* DIS 전역 런타임 세팅 에셋 식별자 ("DISGlobal") */
	constexpr const TCHAR* Global= TEXT("DISGlobal");
}

/**
 * DIS 플러그인의 전역 런타임 세팅(아이템 프로파일, 기본 규칙 등)을 정의하는 PrimaryDataAsset
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "DIS Global Definition"))
class DISRUNTIME_API UDISGlobalDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDISGlobalDefinition();

	/* PrimaryAssetId 오버라이드 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	/* 아이템 타입 태그별 프로파일 설정 맵 */
	UPROPERTY(Category = "Item Profiles", EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FDISItemTypeProfile> ItemTypeProfiles;
};
